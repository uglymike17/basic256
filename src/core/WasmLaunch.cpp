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
// keep them to a shape that cannot walk out of the resource prefix.
bool isSafeExampleName(const QString &name) {
    static const QRegularExpression rx("^[A-Za-z0-9_\\-]+(\\.kbs)?$");
    return rx.match(name).hasMatch();
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
        QString name = req.value.endsWith(".kbs") ? req.value : req.value + ".kbs";
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
        g_pending = done;                       // completed by wasmLaunchOnFetched()
        wasmLaunchFetch(req.value.toUtf8().constData());
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
