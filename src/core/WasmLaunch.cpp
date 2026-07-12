/** Copyright (C) 2026, BASIC256 contributors
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/

#include "WasmLaunch.h"

#ifdef Q_OS_WASM

#include "Constants.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <cstdint>
#include <emscripten.h>
#include <emscripten/em_js.h>

// ---------------------------------------------------------------------------
// JS bridge.
//
// Everything here moves bytes across the JS/wasm boundary using only HEAPU8 and
// UTF8ToString -- the two Emscripten runtime helpers this build has actually
// proven in a browser (WasmAudioSink's PCM copy, RunController's SAY). It
// deliberately does NOT reach for stringToNewUTF8/_malloc/Module.ccall: this
// build sets no EXPORTED_RUNTIME_METHODS, and the makeDynCall episode (WASM.md,
// 2026-07-10) showed that a helper which merely *links* can still be missing at
// runtime. So JS never allocates wasm memory. Instead it parks the bytes in a
// JS-side holder, tells C++ how many there are, and C++ -- which can allocate
// perfectly well -- hands back a pointer to copy into.
// ---------------------------------------------------------------------------

// Stash location.search as UTF-8 bytes; return the byte count.
EM_JS(int, wasmLaunchQuerySize, (), {
    var s = (typeof location !== "undefined" && location.search) ? location.search : "";
    Module.__wasmLaunchQuery = new TextEncoder().encode(s);
    return Module.__wasmLaunchQuery.length;
});

// Copy the stashed query bytes into a C++-owned buffer.
EM_JS(void, wasmLaunchQueryCopy, (int ptr, int len), {
    if (!Module.__wasmLaunchQuery) return;
    HEAPU8.set(Module.__wasmLaunchQuery.subarray(0, len), ptr);
    Module.__wasmLaunchQuery = null;
});

// Fetch a program over the network. Reports completion by calling the
// EMSCRIPTEN_KEEPALIVE export below *directly* -- same reason as above.
EM_JS(void, wasmLaunchFetch, (const char *urlUtf8), {
    var url = UTF8ToString(urlUtf8);
    var report = function (ok, len) {
        if (typeof _wasmLaunchOnFetched !== "undefined") { _wasmLaunchOnFetched(ok, len); }
        else if (typeof Module !== "undefined" && Module._wasmLaunchOnFetched) { Module._wasmLaunchOnFetched(ok, len); }
    };
    fetch(url).then(function (r) {
        if (!r.ok) throw new Error("HTTP " + r.status);
        return r.arrayBuffer();
    }).then(function (ab) {
        Module.__wasmLaunchData = new Uint8Array(ab);
        report(1, Module.__wasmLaunchData.length);
    }).catch(function (e) {
        // A CORS rejection surfaces here as an opaque TypeError; the console
        // note is the only way the user learns why an otherwise valid URL
        // produced nothing.
        console.error("BASIC-256: could not fetch program from " + url + ":", e);
        Module.__wasmLaunchData = null;
        report(0, 0);
    });
});

// Copy the fetched bytes into a C++-owned buffer.
EM_JS(void, wasmLaunchDataCopy, (int ptr, int len), {
    if (!Module.__wasmLaunchData) return;
    HEAPU8.set(Module.__wasmLaunchData.subarray(0, len), ptr);
    Module.__wasmLaunchData = null;
});

namespace {

// The in-flight fetch's completion handler. Only one launch fetch can ever be
// outstanding (it happens once, at startup, before the user can ask for
// another), so a single slot is enough.
std::function<void(bool, QByteArray)> g_pending;

QByteArray queryString() {
    const int len = wasmLaunchQuerySize();
    if (len <= 0) return QByteArray();
    QByteArray buf(len, char(0));
    wasmLaunchQueryCopy(static_cast<int>(reinterpret_cast<intptr_t>(buf.data())), len);
    return buf;
}

// Bundled example names are pasted straight into a ":/examples/..." path, so
// keep them to a shape that cannot walk out of the resource prefix. The
// extension match is case-insensitive to match resolveExampleName() below --
// otherwise ".KBS" would be rejected here before the forgiving lookup ever ran.
bool isSafeExampleName(const QString &name) {
    static const QRegularExpression rx("^[A-Za-z0-9_\\-]+(\\.kbs)?$",
                                       QRegularExpression::CaseInsensitiveOption);
    return rx.match(name).hasMatch();
}

// ?mode= -> the guimode MainWindow is constructed with, plus whether this is a
// "player" (chrome stripped) or a working IDE. Every mode here already exists
// for a command-line switch; the URL just gives wasm a way to ask for one, since
// a browser supplies no argv.
//
// The default (and the fallback for an unrecognised value) is the **full IDE,
// running** -- maintainer decision 2026-07-12, reversing the graph-only default
// this shipped with. A bare ?run=<file> is far more often "show me this program"
// than "embed this demo", and the IDE is the honest landing place: the program is
// visible, editable and stoppable. The chrome-free player is still one parameter
// away (&mode=graph), which is the right way round -- embedding is the deliberate
// act, not the accident.
void applyMode(const QString &mode, WasmLaunch::Request &req) {
    const QString m = mode.trimmed().toLower();

    if (m == QLatin1String("graph")) {
        req.guimode = GUISTATEGRAPH;        // -g : graphics only -- the player
        req.playerChrome = true;
    } else if (m == QLatin1String("text")) {
        req.guimode = GUISTATETEXT;         // -t
        req.playerChrome = true;
    } else if (m == QLatin1String("app")) {
        req.guimode = GUISTATEAPP;          // -a : text + graphics, no editor
        req.playerChrome = true;
    } else if (m == QLatin1String("edit")) {
        req.guimode = GUISTATENORMAL;       // full IDE, loaded but not run
        req.playerChrome = false;           // (ifGuiStateRun() is a no-op here)
    } else {
        req.guimode = GUISTATERUN;          // -r : full IDE, auto-run -- the default
        req.playerChrome = false;
    }
}

// Map a ?run= name onto the actual file in :/examples, case-insensitively.
//
// Qt resource paths are case-sensitive and every bundled example is lowercase,
// so ":/examples/Mandelbrot.kbs" simply does not exist -- ?run=Mandelbrot failed
// with "unable to load" while ?run=mandelbrot worked. These links get typed and
// shared by hand, so an exact-case requirement is a papercut with no upside.
// Try the name as given first, then fall back to a case-insensitive sweep of the
// directory. Returns the real, correctly-cased file name, or empty if no match.
QString resolveExampleName(const QString &requested) {
    const QString wanted = requested.endsWith(".kbs", Qt::CaseInsensitive)
                               ? requested
                               : requested + ".kbs";

    if (QFile::exists(":/examples/" + wanted)) return wanted;

    QDir dir(":/examples");
    const QStringList files = dir.entryList(QStringList() << "*.kbs", QDir::Files, QDir::Name);
    for (const QString &f : files) {
        if (f.compare(wanted, Qt::CaseInsensitive) == 0) return f;
    }
    return QString();
}

// Normalise a ?url= value into exactly the string fetch() will parse.
//
// This is not cosmetic. The WHATWG URL parser *removes* every ASCII tab and
// newline from a URL before parsing it, and trims leading/trailing C0 controls
// and spaces. So "https:%09//evil.com" arrives here (percent-decoded) as
// "https:\t//evil.com", which any sane reading calls a relative path -- while
// fetch() strips the tab, sees the scheme, and goes cross-origin. Validating the
// raw string would therefore check something the browser never parses. Strip the
// same characters the browser does, validate *that*, and fetch *that*.
QString sanitizeUrlPath(const QString &raw) {
    QString s = raw;
    s.remove(QLatin1Char('\t'));
    s.remove(QLatin1Char('\n'));
    s.remove(QLatin1Char('\r'));
    while (!s.isEmpty() && s.at(0).unicode() <= 0x20)                s.remove(0, 1);
    while (!s.isEmpty() && s.at(s.size() - 1).unicode() <= 0x20)     s.chop(1);
    return s;
}

// ?url= is same-origin only (maintainer decision 2026-07-12): it must be a path
// relative to the deployed page, so it can serve its actual purpose -- "drop a
// .kbs next to index.html and link to it" -- without a link being able to name
// someone else's origin. A fetched program runs in the page's origin, where it
// can reach the IDBFS /persist store; CORS would gate the *read*, but any origin
// willing to serve the file (most static hosts, permissively) would clear it --
// so CORS is not the boundary we want.
//
// Takes an already-sanitized path. Rejects an authority in either spelling
// ("//host/x" and "\\host\x" -- the URL parser folds backslash onto slash for
// special schemes, so fetch() reads the backslash form as protocol-relative
// too), and any scheme at all ("https:", but equally "data:", "javascript:").
bool isSameOriginPath(const QString &path) {
    if (path.isEmpty()) return false;

    const auto isSep = [](QChar c){
        return c == QLatin1Char('/') || c == QLatin1Char('\\');
    };
    if (path.size() >= 2 && isSep(path.at(0)) && isSep(path.at(1))) return false;
    if (path.startsWith(QLatin1Char('\\'))) return false;

    const QUrl u(path);
    if (!u.isRelative()) return false;        // carries a scheme
    if (!u.host().isEmpty()) return false;    // carries an authority
    return true;
}

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE void wasmLaunchOnFetched(int ok, int len)
{
    if (!g_pending) return;
    // Move the handler out first: it must run exactly once even if it throws or
    // re-enters, and the slot has to be clear either way.
    auto done = g_pending;
    g_pending = nullptr;

    if (!ok || len <= 0) {
        done(false, QByteArray());
        return;
    }
    QByteArray src(len, char(0));
    wasmLaunchDataCopy(static_cast<int>(reinterpret_cast<intptr_t>(src.data())), len);
    done(true, src);
}

namespace WasmLaunch {

Request parseQuery() {
    Request req;

    QByteArray raw = queryString();
    if (raw.isEmpty()) return req;

    QString q = QString::fromUtf8(raw);
    if (q.startsWith('?')) q.remove(0, 1);
    if (q.isEmpty()) return req;

    QUrlQuery query(q);

    // ?mode= says how to show it, independently of which source names it, so it
    // is resolved once up front and applies to whichever branch below matches.
    // (It is inert if no program parameter is present: no launch, normal IDE.)
    applyMode(query.queryItemValue("mode", QUrl::FullyDecoded), req);

    // Precedence: a bundled example beats inline source beats a fetched URL.
    QString name = query.queryItemValue("run", QUrl::FullyDecoded);
    if (name.isEmpty()) name = query.queryItemValue("program", QUrl::FullyDecoded);
    if (!name.isEmpty()) {
        req.source = Source::Example;
        req.value = name;
        req.title = name.endsWith(".kbs") ? name : name + ".kbs";
        return req;
    }

    QString src = query.queryItemValue("src", QUrl::FullyDecoded);
    if (!src.isEmpty()) {
        req.source = Source::Inline;
        req.value = src;
        req.title = QStringLiteral("program.kbs");
        return req;
    }

    QString url = query.queryItemValue("url", QUrl::FullyDecoded);
    if (!url.isEmpty()) {
        req.source = Source::Url;
        req.value = url;
        const QString base = QFileInfo(QUrl(url).path()).fileName();
        req.title = base.isEmpty() ? QStringLiteral("program.kbs") : base;
        return req;
    }

    return req;
}

void resolve(const Request &req, std::function<void(bool, QByteArray)> done) {
    switch (req.source) {

    case Source::Example: {
        if (!isSafeExampleName(req.value)) { done(false, QByteArray()); return; }
        const QString name = resolveExampleName(req.value);
        if (name.isEmpty()) { done(false, QByteArray()); return; }
        QFile f(":/examples/" + name);
        if (!f.open(QIODevice::ReadOnly)) { done(false, QByteArray()); return; }
        QByteArray src = f.readAll();
        f.close();
        done(true, src);
        return;
    }

    case Source::Inline: {
        // Accept both alphabets: '+' and '/' are awkward in a URL, so a
        // hand-built link is likely to use the URL-safe one, while anything
        // built with btoa() will not be. Decode strictly (AbortOnBase64...):
        // the lenient overload silently drops characters outside the chosen
        // alphabet, which would turn standard base64 read as URL-safe into
        // plausible-looking garbage instead of an honest failure.
        const QByteArray b64 = req.value.toUtf8();
        auto r = QByteArray::fromBase64Encoding(
            b64, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
        if (!r) {
            r = QByteArray::fromBase64Encoding(
                b64, QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
        }
        if (!r) { done(false, QByteArray()); return; }
        done(true, r.decoded);
        return;
    }

    case Source::Url: {
        // Checked here rather than in parseQuery() so a cross-origin link fails
        // visibly ("unable to load...") instead of silently opening the IDE.
        // Fetch the sanitized string, not req.value -- validating one string and
        // fetching another is exactly the bug sanitizeUrlPath() exists to avoid.
        const QString path = sanitizeUrlPath(req.value);
        if (!isSameOriginPath(path)) { done(false, QByteArray()); return; }
        g_pending = done;                       // completed by wasmLaunchOnFetched()
        const QByteArray utf8 = path.toUtf8();
        wasmLaunchFetch(utf8.constData());
        return;
    }

    case Source::None:
    default:
        done(false, QByteArray());
        return;
    }
}

} // namespace WasmLaunch

#endif // Q_OS_WASM
