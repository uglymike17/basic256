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

#include "WasmAudioSink.h"

#ifdef Q_OS_WASM

#include <cstdint>
#include <QMap>
#include <emscripten.h>
#include <emscripten/em_js.h>

// One JS-side registry (Module.__wasmAudio) shared by every WasmAudioSink
// instance: a single AudioContext (browsers only want one), keyed by
// nodeId -> {gain node, decoded AudioBuffer, current source node, timing}.
//
// Pause/resume/seek all go through the same w.startFrom()/w.stopSource()
// JS helpers (defined once, lazily, inside wasmAudioSinkCreate): stop the
// current AudioBufferSourceNode (Web Audio's nodes are one-shot, no native
// pause) and remember an elapsed-seconds offset, then start a fresh node
// from that offset. `onended` is detached before every *explicit* stop, so
// it only ever fires for genuine natural end-of-playback -- the C++ side
// treats that as the sole source of the async IdleState transition; every
// other state change is synchronous (see WasmAudioSink::setState()).

EM_JS(void, wasmAudioSinkSetEndedCallback, (int fnPtr), {
    if (!Module.__wasmAudio) Module.__wasmAudio = { ctx: null, nodes: new Map() };
    Module.__wasmAudio.onEndedPtr = fnPtr;
});

EM_JS(void, wasmAudioSinkCreate, (int nodeId), {
    if (!Module.__wasmAudio) Module.__wasmAudio = { ctx: null, nodes: new Map() };
    var w = Module.__wasmAudio;
    if (!w.ctx) {
        var AC = window.AudioContext || window.webkitAudioContext;
        w.ctx = new AC();
        w.startFrom = function(entry, offsetSeconds) {
            if (entry.source) {
                entry.source.onended = null;
                try { entry.source.stop(); } catch (e) {}
                entry.source.disconnect();
                entry.source = null;
            }
            var src = w.ctx.createBufferSource();
            src.buffer = entry.buffer;
            src.connect(entry.gain);
            src.onended = function() {
                if (entry.source !== src) return; // stale callback from an already-replaced node
                entry.playing = false;
                // makeDynCall is a build-time macro that did not expand inside this
                // EM_JS body on Qt 6.11.1/emsdk 4.0.7; call the KEEPALIVE export
                // directly, with fallbacks that don't depend on macro expansion.
                if (typeof _wasmAudioSinkOnEnded !== "undefined") { _wasmAudioSinkOnEnded(entry.id); }
                else if (typeof Module !== "undefined" && Module._wasmAudioSinkOnEnded) { Module._wasmAudioSinkOnEnded(entry.id); }
                else if (typeof getWasmTableEntry !== "undefined" && w.onEndedPtr) { getWasmTableEntry(w.onEndedPtr)(entry.id); }
                else if (typeof wasmTable !== "undefined" && w.onEndedPtr) { wasmTable.get(w.onEndedPtr)(entry.id); }
            };
            src.start(0, offsetSeconds);
            entry.source = src;
            entry.startedAt = w.ctx.currentTime;
            entry.baseOffset = offsetSeconds;
            entry.playing = true;
        };
        w.stopSource = function(entry) {
            if (entry.source) {
                entry.source.onended = null;
                try { entry.source.stop(); } catch (e) {}
                entry.source.disconnect();
                entry.source = null;
            }
            entry.playing = false;
        };
        w.elapsed = function(entry) {
            if (entry.playing) return (w.ctx.currentTime - entry.startedAt) + entry.baseOffset;
            return entry.baseOffset;
        };
    }
    if (w.ctx.state === "suspended") { w.ctx.resume(); }
    var gain = w.ctx.createGain();
    gain.connect(w.ctx.destination);
    w.nodes.set(nodeId, { id: nodeId, gain: gain, buffer: null, source: null, startedAt: 0, baseOffset: 0, playing: false });
});

// samplesPtr==0 means "reuse the already-decoded buffer" (used by
// resume()/seekTo() restarts); otherwise it points at frameCount
// mono Int16 samples in wasm memory, valid only for the duration of this
// synchronous call (the caller's QByteArray does not outlive it).
EM_JS(void, wasmAudioSinkPlay, (int nodeId, int samplesPtr, int frameCount, int sampleRate, double offsetSeconds), {
    var w = Module.__wasmAudio;
    var entry = w && w.nodes.get(nodeId);
    if (!entry) return;
    if (samplesPtr !== 0) {
        var i16 = HEAP16.subarray(samplesPtr >> 1, (samplesPtr >> 1) + frameCount);
        var audioBuffer = w.ctx.createBuffer(1, frameCount, sampleRate);
        var channel = audioBuffer.getChannelData(0);
        for (var i = 0; i < frameCount; i++) { channel[i] = i16[i] / 32768; }
        entry.buffer = audioBuffer;
    }
    if (!entry.buffer) return;
    w.startFrom(entry, offsetSeconds);
});

EM_JS(void, wasmAudioSinkStop, (int nodeId), {
    var w = Module.__wasmAudio;
    var entry = w && w.nodes.get(nodeId);
    if (!entry) return;
    w.stopSource(entry);
    entry.baseOffset = 0;
});

EM_JS(double, wasmAudioSinkPause, (int nodeId), {
    var w = Module.__wasmAudio;
    var entry = w && w.nodes.get(nodeId);
    if (!entry) return 0;
    var elapsed = w.elapsed(entry);
    w.stopSource(entry);
    entry.baseOffset = elapsed;
    return elapsed;
});

EM_JS(void, wasmAudioSinkResume, (int nodeId, double offsetSeconds), {
    var w = Module.__wasmAudio;
    var entry = w && w.nodes.get(nodeId);
    if (!entry || !entry.buffer) return;
    w.startFrom(entry, offsetSeconds);
});

EM_JS(void, wasmAudioSinkSeek, (int nodeId, double offsetSeconds, int wasPlaying), {
    var w = Module.__wasmAudio;
    var entry = w && w.nodes.get(nodeId);
    if (!entry) return;
    w.stopSource(entry);
    entry.baseOffset = offsetSeconds;
    if (wasPlaying && entry.buffer) { w.startFrom(entry, offsetSeconds); }
});

EM_JS(double, wasmAudioSinkPosition, (int nodeId), {
    var w = Module.__wasmAudio;
    var entry = w && w.nodes.get(nodeId);
    if (!entry) return 0;
    return w.elapsed(entry);
});

EM_JS(void, wasmAudioSinkSetVolume, (int nodeId, double vol), {
    var w = Module.__wasmAudio;
    var entry = w && w.nodes.get(nodeId);
    if (!entry) return;
    entry.gain.gain.value = vol;
});

EM_JS(void, wasmAudioSinkDestroy, (int nodeId), {
    var w = Module.__wasmAudio;
    if (!w) return;
    var entry = w.nodes.get(nodeId);
    if (!entry) return;
    w.stopSource(entry);
    entry.gain.disconnect();
    w.nodes.delete(nodeId);
});

extern "C" EMSCRIPTEN_KEEPALIVE void wasmAudioSinkOnEnded(int nodeId)
{
    WasmAudioSink::handleEnded(nodeId);
}

static QMap<int, WasmAudioSink*> *s_wasmAudioSinkInstances = nullptr;
static int s_wasmAudioSinkNextId = 1;
static bool s_wasmAudioSinkCallbackRegistered = false;

WasmAudioSink::WasmAudioSink(const QAudioFormat &format, QObject *parent) :
    QObject(parent),
    m_nodeId(s_wasmAudioSinkNextId++),
    m_sampleRate(format.sampleRate()),
    m_pendingOffsetSeconds(0.0),
    m_state(QAudio::StoppedState)
{
    if (!s_wasmAudioSinkInstances) s_wasmAudioSinkInstances = new QMap<int, WasmAudioSink*>();
    s_wasmAudioSinkInstances->insert(m_nodeId, this);
    if (!s_wasmAudioSinkCallbackRegistered) {
        s_wasmAudioSinkCallbackRegistered = true;
        wasmAudioSinkSetEndedCallback(static_cast<int>(reinterpret_cast<intptr_t>(&wasmAudioSinkOnEnded)));
    }
    wasmAudioSinkCreate(m_nodeId);
}

WasmAudioSink::~WasmAudioSink()
{
    wasmAudioSinkDestroy(m_nodeId);
    if (s_wasmAudioSinkInstances) s_wasmAudioSinkInstances->remove(m_nodeId);
}

void WasmAudioSink::start(QIODevice *device)
{
    device->seek(0);
    QByteArray bytes = device->readAll();
    int frameCount = bytes.size() / (int)sizeof(int16_t);
    const int16_t *samples = reinterpret_cast<const int16_t*>(bytes.constData());
    wasmAudioSinkPlay(m_nodeId, static_cast<int>(reinterpret_cast<intptr_t>(samples)), frameCount, m_sampleRate, 0.0);
    m_pendingOffsetSeconds = 0.0;
    setState(QAudio::ActiveState);
}

void WasmAudioSink::stop()
{
    wasmAudioSinkStop(m_nodeId);
    m_pendingOffsetSeconds = 0.0;
    setState(QAudio::StoppedState);
}

void WasmAudioSink::suspend()
{
    m_pendingOffsetSeconds = wasmAudioSinkPause(m_nodeId);
    setState(QAudio::SuspendedState);
}

void WasmAudioSink::resume()
{
    // Reuses the AudioBuffer already decoded by start() -- no samples to pass.
    wasmAudioSinkResume(m_nodeId, m_pendingOffsetSeconds);
    setState(QAudio::ActiveState);
}

void WasmAudioSink::setVolume(qreal volume)
{
    wasmAudioSinkSetVolume(m_nodeId, (double)volume);
}

double WasmAudioSink::positionSeconds() const
{
    return wasmAudioSinkPosition(m_nodeId);
}

bool WasmAudioSink::seekTo(double seconds)
{
    bool wasPlaying = (m_state == QAudio::ActiveState);
    wasmAudioSinkSeek(m_nodeId, seconds, wasPlaying ? 1 : 0);
    m_pendingOffsetSeconds = seconds;
    return true;
}

void WasmAudioSink::setState(QAudio::State s)
{
    m_state = s;
    emit stateChanged(m_state);
}

void WasmAudioSink::onEnded()
{
    // Natural end of playback only -- explicit stop()/suspend() never reach
    // here, the JS side detaches `onended` before every explicit stop.
    m_pendingOffsetSeconds = 0.0;
    setState(QAudio::IdleState);
}

void WasmAudioSink::handleEnded(int nodeId)
{
    if (!s_wasmAudioSinkInstances) return;
    WasmAudioSink *sink = s_wasmAudioSinkInstances->value(nodeId, nullptr);
    if (sink) sink->onEnded();
}

#endif // Q_OS_WASM
