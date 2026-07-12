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

#pragma once

#include <QtGlobal>

#ifdef Q_OS_WASM

#include <QByteArray>
#include <QString>
#include <functional>

// Deep-link launch for the WebAssembly build ("player" mode).
//
// A browser gives main() no argv, so the command-line switches that pick a
// reduced GUI (-g/--graph, Main.cpp guimode 3 == GUISTATEGRAPH: "only the
// Graphics Output window") and name a program to auto-run can never fire on
// WASM. This reads the same intent out of the page URL instead, so that
//
//     https://<host>/basic256/?run=mandelbrot
//
// launches straight into a running, graphics-only demo -- no new build target,
// no second binary, just a different launch path into the existing mode.
//
// Three ways to name the program, in precedence order:
//   ?run=<name>   a program bundled in DemoWASM/examples.qrc (":/examples").
//                 ".kbs" is optional; the name is restricted to
//                 [A-Za-z0-9_-] so it cannot escape the resource prefix.
//   ?src=<b64>    the program source itself, base64 (standard or URL-safe
//                 alphabet). Self-contained -- no server needed.
//   ?url=<path>   fetched with the browser's fetch(). **Same-origin only**: a
//                 path relative to the deployed page ("?url=demos/fractal.kbs"),
//                 so you can host your own programs beside index.html. Anything
//                 carrying a scheme or an authority is rejected -- a fetched
//                 program runs in the page's origin, and a link should not be
//                 able to point that at somebody else's server.
//
// ?program= is accepted as a synonym for ?run=.
namespace WasmLaunch {

enum class Source {
    None,       // no launch parameters -- start the normal IDE
    Example,    // ?run= / ?program=  : bundled qrc example
    Inline,     // ?src=              : base64 source in the URL
    Url         // ?url=              : fetched over the network
};

struct Request {
    Source source = Source::None;
    QString value;              // example name, base64 blob, or URL
    QString title;              // display name for the editor tab / title bar
};

// Parse location.search. Synchronous and safe to call before the GUI exists --
// Main.cpp needs the answer before it constructs MainWindow, since guimode is a
// constructor argument.
Request parseQuery();

// Resolve a request to program source.
//
// Example/Inline are decoded in-process and `done` is invoked *before* resolve()
// returns. Url is fetched asynchronously and `done` runs later, from the
// browser's fetch callback -- i.e. after the Qt event loop is running. Callers
// must therefore be happy either way; the auto-run path in Main.cpp is, since
// loadFileContent()+ifGuiStateRun() work equally well before or during exec().
//
// `done` is called exactly once. ok==false means the program could not be
// loaded (unknown example, undecodable base64, fetch/CORS/HTTP failure).
void resolve(const Request &req, std::function<void(bool ok, QByteArray source)> done);

} // namespace WasmLaunch

#endif // Q_OS_WASM
