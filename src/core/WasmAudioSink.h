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

// Q_OS_WASM is defined by Qt's own qsystemdetection.h (based on the real
// compiler builtin __EMSCRIPTEN__), not by the compiler directly -- it only
// becomes visible once some Qt header has been processed. Sound.h always
// includes several Qt headers before reaching its own "#ifdef Q_OS_WASM"
// (which is what pulls this header in), so it sees Q_OS_WASM correctly. But
// WasmAudioSink.cpp includes this header first, with nothing Qt-related
// processed yet -- <QtGlobal> below guarantees Q_OS_WASM is defined before
// either file's own "#ifdef Q_OS_WASM" is evaluated.
#include <QtGlobal>

// QAudioSink hangs the WASM main thread indefinitely on construction (found
// during Phase 4/5 browser testing) -- its single-QAudioFormat-arg
// overload resolves the default audio device the same way
// QMediaDevices::defaultAudioOutput() does, which is also broken there.
// WasmAudioSink is a QAudioSink-shaped facade over the Web Audio API
// (bridged via emscripten's EM_JS) exposing exactly the subset of
// QAudioSink's surface that Sound.cpp calls, so Sound.{h,cpp} can use it as
// a drop-in replacement (see the Sound.h AudioSinkType alias).
//
// Playback model: Web Audio's AudioBufferSourceNode is one-shot (no native
// pause/resume/seek). suspend()/resume()/seekTo() are emulated by stopping
// the current source node and remembering an elapsed-seconds offset, then
// starting a fresh node from that offset -- a standard Web Audio pattern.
//
// Every explicit command (stop/suspend/resume) updates state() and emits
// stateChanged() *synchronously*, not on the async browser round-trip --
// Sound::~Sound()'s `while(audio->state()!=QAudio::StoppedState)
// audio->stop();` busy loop would spin forever otherwise, and the rest of
// Sound.cpp already treats an explicit stop as authoritative without
// waiting for real hardware confirmation. The async `onended` callback from
// JS is only used for *natural* end-of-playback (buffer exhausted on its
// own), matching QAudioSink's transition to QAudio::IdleState.

#ifdef Q_OS_WASM

#include <QObject>
#include <QAudio>
#include <QAudioFormat>
#include <QIODevice>
#include <QByteArray>

// Forward-declared here (matching their real, extern "C" definitions in
// WasmAudioSink.cpp) so the friend declarations below bind to the same
// entities instead of silently declaring second, C++-linkage overloads --
// the two linkages are not interchangeable and Clang rejects the mismatch.
extern "C" void wasmAudioSinkOnEnded(int nodeId);
extern "C" void wasmAudioSinkOnDecoded(int nodeId, int ok, double durationMs);

class WasmAudioSink : public QObject
{
    Q_OBJECT
    public:
        explicit WasmAudioSink(const QAudioFormat &format, QObject *parent = nullptr);
        ~WasmAudioSink();

        void start(QIODevice *device);
        void stop();
        void suspend();
        void resume();
        void setVolume(qreal volume);
        QAudio::State state() const { return m_state; }
        QAudio::Error error() const { return QAudio::NoError; }

        // Compressed in-memory playback (SOUND resources loaded via SOUNDLOAD,
        // the desktop QMediaPlayer::setSourceDevice() path, which Qt for
        // WebAssembly does not support). Asynchronously decodes the compressed
        // bytes to a Web Audio AudioBuffer via ctx.decodeAudioData(); on
        // completion emits decodeFinished(ok, durationMs). Once decoded, the
        // ordinary start()/suspend()/resume()/seekTo() surface plays it -- the
        // decoded AudioBuffer is stored JS-side and reused, so start() does not
        // re-read the QIODevice as raw PCM. The bytes are copied out during
        // this synchronous call and need not outlive it (same as the PCM path).
        void decode(const QByteArray &bytes);
        bool hasDecodedBuffer() const { return m_hasDecoded; }

        // Used by Sound::position() instead of buffer->pos() on WASM --
        // WasmAudioSink reads the whole QIODevice up front (Web Audio needs
        // a fully decoded AudioBuffer, it can't stream from a pull-model
        // QIODevice the way QAudioSink does), so the buffer's read cursor no
        // longer tracks real playback progress. Returns elapsed seconds.
        double positionSeconds() const;

        // Used by Sound::seek() instead of buffer->seek() on WASM, for the
        // same reason -- repositioning the QIODevice's read cursor has no
        // effect once WasmAudioSink has already consumed it.
        bool seekTo(double seconds);

    signals:
        void stateChanged(QAudio::State state);
        // Emitted once when decode() finishes (ok==false on a decodeAudioData
        // reject -- i.e. the bytes were not a valid/supported audio file).
        void decodeFinished(bool ok, double durationMs);

    private:
        static void handleEnded(int nodeId);
        void onEnded();
        static void handleDecoded(int nodeId, int ok, double durationMs);
        void onDecoded(int ok, double durationMs);
        void setState(QAudio::State s);

        int m_nodeId;
        int m_sampleRate;
        double m_pendingOffsetSeconds; // valid while Suspended/Stopped: where a future resume()/seekTo() should start from
        QAudio::State m_state;
        bool m_hasDecoded;             // true once decode() produced a usable AudioBuffer (JS-side)
        double m_decodedDurationMs;    // decoded length; <0 while a decode is still pending

        friend void wasmAudioSinkOnEnded(int nodeId);
        friend void wasmAudioSinkOnDecoded(int nodeId, int ok, double durationMs);
};

#endif // Q_OS_WASM
