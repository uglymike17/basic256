/** Copyright (C) 2016, Florin Oprea <florinoprea.contact@gmail.com>
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

#include <math.h>
#include <stdio.h>
#include <unordered_map>

#include <QObject>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QBuffer>
#include <QEventLoop>
#include <QFileInfo>
#include <QMap>
#include <QThread>

#include "Error.h"
#include "Constants.h"
#include "Settings.h"

#define SOUND_HALFWAVE 0x7ffe // sound editors detect as overflow using max value 0x7fff

#define SOUND_MAX_INSTANCES 100
//max number of sound instances in the same time
//I will search a way to detect the correct number
//http://stackoverflow.com/questions/42272618/qt-qmediaplayer-qthreadstart-failed-to-create-thread
//http://stackoverflow.com/questions/42272618/qt-qmediaplayer-qthreadstart-failed-to-create-thread

#define SOUNDFADEMS             25

#define SOUNDTYPE_EMPTY         0
#define SOUNDTYPE_GENERATED     1
#define SOUNDTYPE_FILE          2
#define SOUNDTYPE_WEB           3
#define SOUNDTYPE_MEMORY        4

struct LoadedSound{
        QByteArray* byteArray;
        //validate loaded media file into memory if it is a playable file
        //to speed up things, we do this check only at first play
        bool needValidation;
        bool isValidated; //the result of validation
        double individualVolume;
        bool scheduledFade;
        double fadeVolume;
        int fadeCountdown;
        int fadeDelayCountdown;
        int loop;
};

class Sound : public QObject
{
    Q_OBJECT
    public:
        Sound(QObject * parent = 0);
        ~Sound();

        bool isPlayer; //true if this is a player, false if this is a ordinary sound
        int id;
        int type; //0-no sound, 1-generated sound, 2-file, 3-web, 4-memory //(for debug)
        QString source;
        int soundState; //
        int soundStateExpected; //
        qint64 media_duration;
        double individualVolume;
        bool isStopping; //flag
        bool isPausedBySystem; //flag
        bool needValidation; //when play a loaded file and we don't know if is a valid file... otherwise player remains in Playing state forever
        bool isValidated;
        int loopCountdown; //used for loop counting
        int loopSaved; //used for players to remember the initial number of loops
        QMediaPlayer* media;
        QAudioOutput* audio;
        QBuffer *buffer;
        QByteArray *byteArray;
        Error **error;

        QBasicTimer fadeTimer;
        bool scheduledFade;
        double fadeVolume;
        double masterVolume;
        int fadeCountdown;
        int fadeDelayCountdown;

        int state();
        void play();
        void stop();
        void pause();
        void wait();
        double position();
        bool seek(double);
        double length();
        void updatedMasterVolume(double);
        int lastError();
        void prepareConnections();
        int sound_samplerate;
        bool waitLoadedMediaValidation();
private:
        bool isPositionChanged;
        void waitLastMediaSeekTakeAction();
        bool isReady;
        void waitMediaStatusChanged();

signals:
        void exitWaitingLoop();
        void deleteMe(int);
        void validateLoadedSound(QString, bool);

public slots:
        void stopsSoundsAndWaiting();
        void systemMassCommand(int);

private slots:
        void handleAudioStateChanged(QAudio::State);
        void handleMediaStateChanged(QMediaPlayer::State);
        void handleMediaDurationChanged(qint64);
        void handleMediaStatusChanged(QMediaPlayer::MediaStatus);
        void handleMediaError(QMediaPlayer::Error);
        void handlePositionChanged(qint64 position);

protected:
        void timerEvent(QTimerEvent *event) override;
};



class SoundSystem : public QObject
{
Q_OBJECT
    public:
        SoundSystem();
        ~SoundSystem();

        int soundID; //this is the last sound ID... it can be 0 when someting goes wrong
        Error **error;

        int playSound(std::vector<std::vector<double>> sounddata, bool isPlayer);
        int playSound(QString s, bool isPlayer);
        void loadSoundFromArray(QString, QByteArray*);
        void pause(int sound);
        double position(int sound);
        void seek(int sound, double pos);
        int state(int sound);
        void stop(int sound);
        void wait(int sound);
        double length(int sound);
        void volume(int sound, double vol);
        void volume(QString id, double vol);
        void fade(int sound, double vol, int ms, int delay);
        void fade(QString id, double vol, int ms, int delay);
        void loop(int sound, int loop);
        void loop(QString id, int loop);
        void play(int sound);
        void waveform(int wave);
        void setMasterVolume(double);
        double getMasterVolume();
        void exit();
        QString loadSoundFromVector(std::vector<std::vector<double>>);
        bool unloadSound(QString);
        void envelope(std::vector<double> envelope);
        void noenvelope();
        void noharmonics();
        void harmonics(int h, double a);
        void playerOff(int sound);
        void system(int);
        int samplerate();
        void customWaveform(std::vector<double> wave, bool logic);
        QString loadRaw(std::vector<double> raw);

signals:
        void stopsSoundsAndWaiting();
        void systemMassCommand(int);

private slots:
        void deleteMe(int);
        void validateLoadedSound(QString, bool);

private:
        bool soundSystemIsStopping;
        int lastIdUsed; //this is the unique ID
        int lastBeepLoaded;
        std::unordered_map<int, Sound*> soundsmap;
        QMap <QString, LoadedSound> loadedsounds;
        QByteArray* generateSound(std::vector<std::vector<double>>);
        QAudioFormat format;
        double masterVolume;
        int sound_samplerate;
        int sound_normalize_ms;
        int sound_fade_ms;
        int16_t *currentwaveform; //chosen waveform
        int16_t *waveformsin;
        int16_t *waveformsquare;
        int16_t *waveformtriangle;
        int16_t *waveformsaw;
        int16_t *waveformpulse;
        int16_t *waveformcustom;
        int16_t *waveformmixedwithharmonics;
        int16_t *waveformsample;
        std::vector<double> sound_envelope;
        int sound_envelope_length;
        int sound_envelope_release;
        double sound_envelope_release_bit;
        std::map<int,double> sound_harmonics;
        void updateWaveformSample(void);
        bool mustUpdateWaveformSample;
};

