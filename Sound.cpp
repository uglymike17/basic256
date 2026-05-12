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

#include "Sound.h"
#include <QDebug>


Sound::Sound(QObject *parent) :
	QObject(parent)
{
	type = SOUNDTYPE_EMPTY;
	soundState = 0;
	soundStateExpected = 0;
	//We need soundStateExpected because media sounds that are resumed are a bit lazy to
	//update state to QMediaPlayer::PlayingState
	//And if we call wait() just after pause()/play() we can have a chance to miss the waiting
	//Sequence: play(), pause(), play()/resume, wait()
	//A solution was to wait after each play() or pause() by loosing speed
	byteArray = NULL;
	buffer = NULL;
	audio = NULL;
	media = NULL;
	source = "";
	isStopping = false;
	needValidation = true;
	isValidated = false;
	individualVolume = 1.0;
	media_duration = -1; //used only by mediaplayer because duration may not be available when initial playback begins
	masterVolume = 10;
	scheduledFade = false;
	fadeVolume = 1.0;
	fadeCountdown = 0;
	fadeDelayCountdown = 0;
	loopCountdown = 1; //play only once
	loopSaved = 1; //play only once
	isPositionChanged = true; //first position is 0
	isReady = false;
	error = NULL;
	isPausedBySystem = false;
}

Sound::~Sound() {
	if(audio){
		while(audio->state()!=QAudio::StoppedState) audio->stop();
		delete(audio);
		audio=NULL;
	}
	if(media){
		delete(media);
		media=NULL;
	}
	if(buffer){
		buffer->close();
		delete(buffer);
		buffer=NULL;
	}
	delete(byteArray);
	byteArray=NULL;
}

int Sound::state() {
	return soundState;
}

void Sound::play() {
	if(!isStopping){
		if(audio){
			if(soundStateExpected == 2){
				soundStateExpected = 1;
				audio->resume();
				isPausedBySystem = false;
				if(scheduledFade){
					scheduledFade=false;
					fadeTimer.start(SOUNDFADEMS, this);
				}
			}else if(soundStateExpected == 0){
				soundStateExpected = 1;
				audio->start(buffer);
				isPausedBySystem = false;
				if(scheduledFade){
					scheduledFade=false;
					fadeTimer.start(SOUNDFADEMS, this);
				}
			}
			if(audio->error()!=QAudio::NoError && audio->error()!=QAudio::UnderrunError){
				isStopping=true;
				if(*error)(*error)->q(WARNING_SOUNDERROR);
				audio->disconnect(this);
				emit(deleteMe(id));
				this->disconnect();
				return;
			}
		}else if(media){
			if(needValidation){
				isValidated=waitLoadedMediaValidation();
				//qDebug() << "isValidated" << isValidated;
				needValidation=false;
				emit(validateLoadedSound(source, isValidated));
			}
			if(!isValidated){
				if(type == SOUNDTYPE_MEMORY){
					if(*error)(*error)->q(WARNING_SOUNDERROR);
				}else{
					if(*error)(*error)->q(WARNING_SOUNDFILEFORMAT);
				}
				return;
			}
			if(soundStateExpected != 1){
				isPausedBySystem = false;
				if(media_duration>0 && media_duration==media->position() && soundStateExpected != 0){
					//this is needed because QMediaPlayer has a bug
					//When user send a huge amount of play/pause/play/pause commands to a QMediaPlayer, then sound do not
					//emit QMediaPlayer::StoppedState or QMediaPlayer::EndOfMedia when reach the end because of stack
					//of commands. So we can not detect when sound is finished.
					//In this situation QMediaPlayer remain in QMediaPlayer::PlayingState like forever.
					//This is a trick to detect if QMediaPlayer reach the end of sound and stop it.
					//soundStateExpected = 0;
					media->stop();
				}else{
					soundStateExpected = 1;
					media->play();
					if(scheduledFade){
						scheduledFade=false;
						fadeTimer.start(SOUNDFADEMS, this);
					}
				}
			}
		}
	}
}

void Sound::stop() {
	if(!isPlayer) isStopping = true;
	soundStateExpected = 0;
	loopCountdown=1; //this is the last loop
	if(audio){
		audio->stop();
		isPausedBySystem = false;
	}else if(media){
		media->stop();
		isPausedBySystem = false;
	}
}

void Sound::pause() {
	if(soundStateExpected==1 && !isStopping){
		if(audio){
			soundStateExpected = 2;
			audio->suspend();
			isPausedBySystem = false;
		}else if(media){
			if(isValidated){
				soundStateExpected = 2;
				media->pause();
				isPausedBySystem = false;
			}
		}
	}
}

void Sound::wait() {
	//Wait till the end of sound or till the end of program
	if(soundStateExpected==1){
		QEventLoop *loop = new QEventLoop();
		QObject::connect(this, SIGNAL(exitWaitingLoop()), loop, SLOT(quit()));
		if(audio){
			QObject::connect(audio, SIGNAL(stateChanged(QAudio::State)), loop, SLOT(quit()));
			while(!isStopping && audio && audio->state() == QAudio::ActiveState){
				loop->exec(QEventLoop::WaitForMoreEvents);
			}
		}else if(media){
			QObject::connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), loop, SLOT(quit()));
			while(!isStopping && media && (media->state() == QMediaPlayer::PlayingState || (media->state() == QMediaPlayer::PausedState && soundStateExpected==1))){
				loop->exec(QEventLoop::WaitForMoreEvents);
			}
		}
		delete (loop);
	}
}

bool Sound::waitLoadedMediaValidation() {
	//this function waits a bit after play command to check if buffer contain a valid media file
	//because QMediaPlayer::play remains in Play state when a fake sound is loaded into memory and played
	//The only trick I found is that when first mediaStatusChanged is emitted, the duration is 0 for fake media
	//http://stackoverflow.com/questions/42168280/qmediaplayer-play-a-sound-loaded-into-memory
	//PS This validation should be fired only once, at first play

	QTimer *timer = new QTimer();
	timer->setSingleShot(true);
	QEventLoop *loop = new QEventLoop();
	QObject::connect(this, SIGNAL(exitWaitingLoop()), loop, SLOT(quit()));
	QObject::connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));
	timer->start(3000);
	//QObject::connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), loop, SLOT(quit()));
	QObject::connect(media, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), loop, SLOT(quit()));
	QObject::connect(media, SIGNAL(durationChanged(qint64)), loop, SLOT(quit()));
	if(!isStopping){
		loop->exec(QEventLoop::ExcludeUserInputEvents);
	}
	//qDebug() << "Sound::waitLoadedMediaValidation() timer" << timer->isActive();
	delete (loop);
	delete (timer);
	return (media_duration>0);
}

double Sound::position() {
	//Return the position of the played sound
	//Attention: elapsedUSecs returns the microseconds since start() was called, including time
	//in Idle and Suspend states. So, in this case, we gonna use buffer->pos() instead.
	if(audio){
		if(buffer){
			return ((double)buffer->pos() / ((double)sound_samplerate * (double) sizeof(int16_t)));
		}
	}else if(media){
		//for media files, we ensure that last seek command is finished
		waitLastMediaSeekTakeAction();
		return media->position() / (double)(1000.0);
	}
	return 0.0;
}

bool Sound::seek(double sec) {
	//For QAudio the seeking must be done by using the buffer. Attention: the new position must be divisible
	//with sizeof(int16_t) - the sample size of generated sound is 16 bit
	//For QMediaPlayer we need to wait a bit if the sound is not seekable for the moment, but not loger than 2 seconds
	if(audio){
		return (buffer->seek( (qint64)((double)sound_samplerate * sec)  * (qint64) sizeof(int16_t)));
	}else if(media){
		if(!isReady) waitMediaStatusChanged();
		waitLastMediaSeekTakeAction();
		if(!isStopping){
			if(media->isSeekable()) {
				isPositionChanged = false;
				media->setPosition(sec * 1000L);
				return true;
			} else {
				QTimer *timer = new QTimer();
				timer->setSingleShot(true);
				QEventLoop *loop = new QEventLoop();
				connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));
				timer->start(2000);
				QObject::connect(this, SIGNAL(exitWaitingLoop()), loop, SLOT(quit()));
				QObject::connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), loop, SLOT(quit()));
				QObject::connect(media, SIGNAL(seekableChanged(bool)), loop, SLOT(quit()));
				while(!isStopping && media && !media->isSeekable() && media->state() != QMediaPlayer::StoppedState && timer->isActive()){
					loop->exec(QEventLoop::WaitForMoreEvents);
				}
				delete (loop);
				delete (timer);
				if(media->isSeekable()) {
					isPositionChanged = false;
					media->setPosition(sec * 1000L);
					return true;
				} else {
					return false;
				}
			}
		}
	}
	return true;
}

void Sound::waitLastMediaSeekTakeAction(){
	//If we seek into a media file multiple times, only the first seek command take action
	//We need to wait for SIGNAL  QMediaPlayer::positionChanged(qint64 position)
	//to make sure that previous seek command is processed.
	//Only then we can set another position or read the current position after a seek.
	//A simple solution is to wait after each seek command, but thus we lose speed.

	if(!isPositionChanged && !isStopping){
		//wait for the last seek command to take action
		QTimer *timer = new QTimer();
		timer->setSingleShot(true);
		QEventLoop *loop = new QEventLoop();
		connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));
		timer->start(2000);
		QObject::connect(this, SIGNAL(exitWaitingLoop()), loop, SLOT(quit()));
		QObject::connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), loop, SLOT(quit()));
		QObject::connect(media, SIGNAL(positionChanged(qint64)), loop, SLOT(quit()));
		while(!isStopping && !isPositionChanged && timer->isActive()){
			loop->exec(QEventLoop::WaitForMoreEvents);
		}
		delete (loop);
		delete (timer);
	}
}

void Sound::waitMediaStatusChanged(){
	//If we seek into a media file multiple times, only the first seek command take action
	//We need to wait for SIGNAL  QMediaPlayer::positionChanged(qint64 position)
	//to make sure that previous seek command is processed.
	//Only then we can set another position or read the current position after a seek.
	//A simple solution is to wait after each seek command, but thus we lose speed.

	if(!isReady && !isStopping){
		//wait for the last seek command to take action
		QTimer *timer = new QTimer();
		timer->setSingleShot(true);
		QEventLoop *loop = new QEventLoop();
		connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));
		timer->start(2000);
		QObject::connect(this, SIGNAL(exitWaitingLoop()), loop, SLOT(quit()));
		QObject::connect(media, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), loop, SLOT(quit()));
		if(!isStopping){
			loop->exec(QEventLoop::WaitForMoreEvents);
		}
		delete (loop);
		delete (timer);
	}
}

double Sound::length() {
	//For QMediaPlayer the length of played sound may not be available when initial playback begins (asyncron mode).
	//If we need the length for a media we may wait a bit right after sound starts to play, but not longer than 2 seconds.
	if(audio){
		return ( buffer->size() / (double) sizeof(int16_t) / (double)sound_samplerate );
	}else if(media){
		bool timeisup = false;
		if(media_duration<0.0){
			QTimer *timer = new QTimer();
			timer->setSingleShot(true);
			QEventLoop *loop = new QEventLoop();
			connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));
			timer->start(2000);
			QObject::connect(this, SIGNAL(exitWaitingLoop()), loop, SLOT(quit()));
			QObject::connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), loop, SLOT(quit()));
			QObject::connect(media, SIGNAL(durationChanged(qint64)), loop, SLOT(quit()));
			while(!isStopping && timer->isActive() && media_duration<0.0){
				loop->exec(QEventLoop::WaitForMoreEvents);
			}
			if(!timer->isActive()) timeisup=true;
			delete (loop);
			delete (timer);
		}
		if(timeisup || media_duration<0.0) return (-1.0); //return -1 to display a warning
		return (media_duration/1000.0);
	}
	return 0;
}


//tell that the master volume has been changed
void Sound::updatedMasterVolume(double v) {
	masterVolume=v; //keep value for fade effect
	if(audio){
		audio->setVolume((qreal)(v/10.0*individualVolume));
	}else if(media){
		media->setVolume(v*10.0*individualVolume);
	}
}

int Sound::lastError() {
	if(audio){
		return audio->error();
	}else if(media){
		return media->error();
	}
	return 0;
}

void Sound::prepareConnections() {
	if(audio){
		connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleAudioStateChanged(QAudio::State)));
	}else if(media){
		connect(media, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(handleMediaStatusChanged(QMediaPlayer::MediaStatus)));
		connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(handleMediaStateChanged(QMediaPlayer::State)));
		connect(media, SIGNAL(durationChanged(qint64)), this, SLOT(handleMediaDurationChanged(qint64)));
		connect(media, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(handleMediaError(QMediaPlayer::Error)));
		connect(media, SIGNAL(positionChanged(qint64)), this, SLOT(handlePositionChanged(qint64)));
	}
}

void Sound::handlePositionChanged(qint64 position){
	//Signal the position of the content has changed to position, expressed in milliseconds.
	isPositionChanged = true;
	//This flag is usefull because we need to wait before make another seek
	//or before return the true position after a seek action
}

void Sound::handleMediaStatusChanged(QMediaPlayer::MediaStatus s){
	if(s!=QMediaPlayer::LoadingMedia){
		isReady = true;
		if(needValidation){ //media is nor loaded yet
			isValidated=false;
			needValidation=false;
		}
	}
}

void Sound::handleMediaDurationChanged(qint64 d){
	media_duration=d;
	if(needValidation){
		isValidated=media->duration()>0.0;
		needValidation=false;
	}
}

void Sound::handleMediaError(QMediaPlayer::Error err) {
	if(needValidation){
		isValidated=media->duration()>0.0;
		needValidation=false;
	}
	//qDebug() << err;
}

void Sound::handleMediaStateChanged(QMediaPlayer::State newState){
	soundState = newState;
	if(newState==QMediaPlayer::StoppedState){
		soundStateExpected=0;
		if(loopCountdown==1 || isStopping){ //if this was the last loop or the user request to stop
			if(fadeTimer.isActive()) fadeTimer.stop();
			if(!isPlayer){
				isStopping=true;
				media->disconnect(this);
				emit(exitWaitingLoop());
				media->setMedia(QMediaContent());
				emit(deleteMe(id));
				this->disconnect();
			}else{
				loopCountdown=loopSaved; //Recharge number of loops
				media->setPosition(0);
			}
		}else{
			if(loopCountdown>1) loopCountdown--;
			play();
		}
	}
}

void Sound::handleAudioStateChanged(QAudio::State newState){
	switch (newState) {
	case QAudio::ActiveState:
		soundState = 1;
		break;
	case QAudio::SuspendedState:
		soundState = 2;
		break;
	case QAudio::StoppedState:
		soundState = 0;
		soundStateExpected=0;
		if(loopCountdown==1 || isStopping){ //if this was the last loop or the user request to stop
			if(fadeTimer.isActive()) fadeTimer.stop();
			if(!isPlayer){
				isStopping=true;
				audio->disconnect(this);
				emit(exitWaitingLoop());
				emit(deleteMe(id));
				this->disconnect();
			}else{
				audio->stop();
				loopCountdown=loopSaved; //Recharge number of loops
				if(buffer) buffer->seek(0);
			}
		}else{
			if(loopCountdown>1) loopCountdown--;
			if(buffer) buffer->seek(0);
			play();
		}
		break;
	case QAudio::IdleState:
		soundState = 0;
		soundStateExpected=0;
		if(loopCountdown==1 || isStopping){ //if this was the last loop or the user request to stop
			if(fadeTimer.isActive()) fadeTimer.stop();
			if(!isPlayer){
				isStopping=true;
				audio->disconnect(this);
				audio->stop();
				emit(exitWaitingLoop());
				emit(deleteMe(id));
				this->disconnect();
			}else{
				audio->stop();
				loopCountdown=loopSaved; //Recharge number of loops
				if(buffer) buffer->seek(0);
			}
		}else{
			if(loopCountdown>1) loopCountdown--;
			if(buffer) buffer->seek(0);
			play();
		}
		break;
	}
}

void Sound::systemMassCommand(int i){
	//0 - stop all playing sounds
	//1 - play/resume all sounds paused with previous soundsystem(2) command.
	//2 - pause all playing sounds
	//3 - stop and delete all sounds and players
	//4 - play/resume all paused sounds

	switch (i) {
	case 0:
		if(soundStateExpected == 1) stop();
		break;
	case 1:
		if(isPausedBySystem) play();
		break;
	case 2:
		if(soundStateExpected == 1){
			pause();
			isPausedBySystem = true;
		}
		break;
	case 3:
		stopsSoundsAndWaiting();
		break;
	case 4:
		if(soundStateExpected == 2) play();
		break;
	}
}


void Sound::stopsSoundsAndWaiting(){
	//This slot is connected to main sound system
	//It is time to shutdown any activity
	//We will do the entire job here, disconnecting signals

	isStopping=true;
	loopCountdown=1;
	soundState = 0;
	if(fadeTimer.isActive()) fadeTimer.stop();
	if(audio){
		audio->disconnect(this);
		if(audio->state()==QAudio::SuspendedState || soundStateExpected == 2){
			//Sometimes, if user request Pause, we need to start audio to properly stop and delete it
			audio->start(buffer);
		}
		audio->stop();
		emit(exitWaitingLoop());
		emit(deleteMe(id));
		this->disconnect();
	}else if(media){
		media->disconnect(this);
		media->stop();
		emit(exitWaitingLoop());
		media->setMedia(QMediaContent());
		emit(deleteMe(id));
		this->disconnect();
	}
}

//Fade effect
void Sound::timerEvent(QTimerEvent *event){
	if (event->timerId() == fadeTimer.timerId()) {
		if(soundState==1){//only while it is playing
			if(fadeDelayCountdown <=0 ){
				fadeCountdown--;
				if(fadeCountdown<=0){
					individualVolume=fadeVolume;
				}else{
					individualVolume+=(fadeVolume-individualVolume)/(double)fadeCountdown;
				}
				if(audio){
					audio->setVolume((qreal)(masterVolume/10.0*individualVolume));
				}else if(media){
					media->setVolume(masterVolume*10*individualVolume);
				}
				if(fadeCountdown<=0 || individualVolume==fadeVolume){
					fadeTimer.stop();
				}
			}else{
				fadeDelayCountdown--;
			}
		}
	} else {
		QObject::timerEvent(event);
	}
}


// ##########################################################################################
// ##########################################################################################

SoundSystem::SoundSystem() :
	QObject()
{
	error=NULL;
	soundSystemIsStopping = false;
	lastIdUsed=0;
	soundID=0;
	lastBeepLoaded=0;
	sound_envelope_length=0;
	sound_envelope_release=0;
	sound_envelope_release_bit=0.0;
	sound_harmonics[1] = 1.0; //set the fundamental tone / 1st harmonic to full amplitude


	SETTINGS;
	setMasterVolume(settings.value(SETTINGSSOUNDVOLUME, SETTINGSSOUNDVOLUMEDEFAULT).toInt());
	sound_samplerate = settings.value(SETTINGSSOUNDSAMPLERATE, SETTINGSSOUNDSAMPLERATEDEFAULT).toInt();
	sound_normalize_ms = settings.value(SETTINGSSOUNDNORMALIZE, SETTINGSSOUNDNORMALIZEDEFAULT).toInt();
	sound_fade_ms = settings.value(SETTINGSSOUNDVOLUMERESTORE, SETTINGSSOUNDVOLUMERESTOREDEFAULT).toInt();

	// setup the audio format
	format.setSampleRate(sound_samplerate);
	format.setChannelCount(1);
	format.setSampleSize(16);	// 16 bit audio
	format.setCodec("audio/pcm");
	format.setSampleType(QAudioFormat::SignedInt);

// debugging - list available devices
//QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
//foreach (QAudioDeviceInfo i, devices) {
//    fprintf(stderr, i.deviceName().toUtf8().constData());
//    fprintf(stderr, "\n");
//}

	QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
	if (!info.isFormatSupported(format)) {
		format = info.nearestFormat(format);
#ifdef DEBUG
		fprintf(stderr,"Switching to nearest audio format.\n");
#endif
	}

	int i;
	double bit, pos;

	//Generate sin waveform
	waveformsin = new int16_t[sound_samplerate];
	bit = 2 * 4*atan(1.0) / ((double) sound_samplerate); //pi = 4*atan(1.0);
	for(i = 0; i < sound_samplerate; i++) {
		waveformsin[i]=(int16_t) (SOUND_HALFWAVE * sin(bit * (double)i));
	}

	//Generate square waveform
	//waveformsquare = new int16_t[sound_samplerate];
	//for(i = 0; i < sound_samplerate/2; i++) {
	//   waveformsquare[i]=(int16_t) SOUND_HALFWAVE;
	//}
	//for(; i < sound_samplerate; i++) {
	//    waveformsquare[i]=(int16_t) -SOUND_HALFWAVE;
	//}

	//Generate triangle waveform
	//waveformtriangle = new int16_t[sound_samplerate];
	//bit = SOUND_HALFWAVE / ((double) sound_samplerate) * 4.0;
	//pos=0.0;
	//for(i = 0; i < sound_samplerate/4; i++) {
	//    waveformtriangle[i]=(int16_t) pos;
	//    pos+=bit;
	//}
	//for(; i < sound_samplerate/4*3; i++) {
	//    waveformtriangle[i]=(int16_t) pos;
	//    pos-=bit;
	//}
	//for(; i < sound_samplerate; i++) {
	//    waveformtriangle[i]=(int16_t) pos;
	//    pos+=bit;
	//}

	//Generate saw waveform
	//waveformsaw = new int16_t[sound_samplerate];
	//bit = SOUND_HALFWAVE * 2.0 / ((double) sound_samplerate);
	//pos=0.0;
	//for(i = 0; i < sound_samplerate/2; i++) {
	//    waveformsaw[i]=(int16_t) pos;
	//    pos+=bit;
	//}
	//pos=-SOUND_HALFWAVE;
	//for(; i < sound_samplerate; i++) {
	//    waveformsaw[i]=(int16_t) pos;
	//    pos+=bit;
	//}

	//Generate pulse waveform
	//waveformpulse = new int16_t[sound_samplerate];
	//for(i = 0; i < sound_samplerate/10; i++) {
	//    waveformpulse[i]=(int16_t) SOUND_HALFWAVE;
	//}
	//for(; i < sound_samplerate; i++) {
	//    waveformpulse[i]=(int16_t) -SOUND_HALFWAVE;
	//}

	//Allocate space for waveformcustom (created by user)
	//waveformcustom = new int16_t[sound_samplerate];

	//Allocate space for sound wave mixed with harmonics
	//waveformmixedwithharmonics = new int16_t[sound_samplerate];

	currentwaveform = waveformsin; //default waveform = sin
	mustUpdateWaveformSample = false;
	waveformsample = waveformsin;
}

SoundSystem::~SoundSystem() {
	exit();
	//delete loaded sounds
	QMap<QString, LoadedSound>::const_iterator i = loadedsounds.constBegin();
	while (i != loadedsounds.constEnd()) {
		delete(i.value().byteArray);
		++i;
	}
	loadedsounds.clear();
	delete[] waveformsin;
	//delete[] waveformsquare;
	//delete[] waveformtriangle;
	//delete[] waveformsaw;
	//delete[] waveformpulse;
	//delete[] waveformcustom;
	//delete[] waveformmixedwithharmonics;
}

void SoundSystem::deleteMe(int id){
	if(soundsmap.count(id)){
		disconnect(this, 0, soundsmap[id], 0);
		soundsmap[id]->deleteLater();
		soundsmap.erase(id);
	}
}

void SoundSystem::validateLoadedSound(QString source, bool v){
	//Validate loaded file only once.
	//See waitLoadedMediaValidation() for details.
	if(loadedsounds.count(source)){
		loadedsounds[source].needValidation = false;
		loadedsounds[source].isValidated = v;
	}
}


void SoundSystem::exit(){
	if(soundSystemIsStopping) return;
	soundSystemIsStopping = true; // no more  new sounds
	//stop everything
	emit(stopsSoundsAndWaiting());
}



int SoundSystem::playSound(QString s, bool isPlayer){
	soundID=0;
	if(soundsmap.size() >= SOUND_MAX_INSTANCES){
		if(*error)(*error)->q(ERROR_TOOMANYSOUNDS);
		return 0;
	}
	if(soundSystemIsStopping) return 0;
	QUrl url(s);
	if(s.startsWith("sound:")){
		if(loadedsounds.count(s)){
			lastIdUsed++;
			//Do not play a loaded sound that is not a valid one
			//(play if sound is validated already or try to play for the first time)
			if(loadedsounds[s].needValidation || loadedsounds[s].isValidated){
				//play only if loaded file is a valid one or if need validation
				soundsmap[lastIdUsed] = new Sound(this);
				soundsmap[lastIdUsed]->id = lastIdUsed;
				soundsmap[lastIdUsed]->error = error;
				soundsmap[lastIdUsed]->isPlayer = isPlayer; //set if this is a player that not delete sound at stop
				connect(this, SIGNAL(stopsSoundsAndWaiting()), soundsmap[lastIdUsed], SLOT(stopsSoundsAndWaiting()));
				connect(this, SIGNAL(systemMassCommand(int)), soundsmap[lastIdUsed], SLOT(systemMassCommand(int)));
				connect(soundsmap[lastIdUsed], SIGNAL(deleteMe(int)), this, SLOT(deleteMe(int)));
				connect(soundsmap[lastIdUsed], SIGNAL(validateLoadedSound(QString, bool)), this, SLOT(validateLoadedSound(QString, bool)));
				soundsmap[lastIdUsed]->media = new QMediaPlayer(soundsmap[lastIdUsed]);
				soundsmap[lastIdUsed]->individualVolume = loadedsounds[s].individualVolume;
				soundsmap[lastIdUsed]->updatedMasterVolume(masterVolume);
				soundsmap[lastIdUsed]->type = SOUNDTYPE_MEMORY;
				soundsmap[lastIdUsed]->source = s;
				soundsmap[lastIdUsed]->prepareConnections();
				soundsmap[lastIdUsed]->buffer = new QBuffer(loadedsounds[s].byteArray);
				soundsmap[lastIdUsed]->buffer->open(QIODevice::ReadOnly);
				soundsmap[lastIdUsed]->buffer->seek(0);
				soundsmap[lastIdUsed]->needValidation = loadedsounds[s].needValidation;
				soundsmap[lastIdUsed]->isValidated = loadedsounds[s].isValidated;
				soundsmap[lastIdUsed]->media->setMedia(QMediaContent(), soundsmap[lastIdUsed]->buffer);
				soundsmap[lastIdUsed]->scheduledFade=loadedsounds[s].scheduledFade;
				soundsmap[lastIdUsed]->fadeVolume=loadedsounds[s].fadeVolume;
				soundsmap[lastIdUsed]->fadeCountdown=loadedsounds[s].fadeCountdown;
				soundsmap[lastIdUsed]->fadeDelayCountdown=loadedsounds[s].fadeDelayCountdown;
				soundsmap[lastIdUsed]->loopSaved=loadedsounds[s].loop;
				soundsmap[lastIdUsed]->loopCountdown=loadedsounds[s].loop;
				if(!isPlayer){
					soundsmap[lastIdUsed]->play(); //if is a regular sound then play it, if is a player, then do not play it
				}
				soundID=lastIdUsed;
			}
		}else{
			//there is no resource loaded with that ID
			if(*error)(*error)->q(ERROR_SOUNDRESOURCE);
		}
	}else if(s.startsWith("beep:")){
		if(loadedsounds.count(s)){
			lastIdUsed++;
			soundsmap[lastIdUsed] = new Sound(this);
			soundsmap[lastIdUsed]->id = lastIdUsed;
			soundsmap[lastIdUsed]->error = error;
			soundsmap[lastIdUsed]->isPlayer = isPlayer; //set if this is a player that not delete sound at stop
			connect(this, SIGNAL(stopsSoundsAndWaiting()), soundsmap[lastIdUsed], SLOT(stopsSoundsAndWaiting()));
			connect(this, SIGNAL(systemMassCommand(int)), soundsmap[lastIdUsed], SLOT(systemMassCommand(int)));
			connect(soundsmap[lastIdUsed], SIGNAL(deleteMe(int)), this, SLOT(deleteMe(int)));
			//use this only when we gonna use QMediaPlayer instead of QAudioOutput (QAudioOutput intialization error)
			//connect(soundsmap[lastIdUsed], SIGNAL(validateLoadedSound(QString, bool)), this, SLOT(validateLoadedSound(QString, bool)));
			soundsmap[lastIdUsed]->type = SOUNDTYPE_MEMORY;
			soundsmap[lastIdUsed]->source = s;
			soundsmap[lastIdUsed]->buffer = new QBuffer(loadedsounds[s].byteArray);
			soundsmap[lastIdUsed]->buffer->open(QIODevice::ReadOnly);
			soundsmap[lastIdUsed]->buffer->seek(0);
			soundsmap[lastIdUsed]->audio = new QAudioOutput(format,soundsmap[lastIdUsed]);
			soundsmap[lastIdUsed]->individualVolume = loadedsounds[s].individualVolume;
			soundsmap[lastIdUsed]->updatedMasterVolume(masterVolume);
			soundsmap[lastIdUsed]->sound_samplerate=sound_samplerate;
			soundsmap[lastIdUsed]->prepareConnections();
			soundsmap[lastIdUsed]->scheduledFade=loadedsounds[s].scheduledFade;
			soundsmap[lastIdUsed]->fadeVolume=loadedsounds[s].fadeVolume;
			soundsmap[lastIdUsed]->fadeCountdown=loadedsounds[s].fadeCountdown;
			soundsmap[lastIdUsed]->fadeDelayCountdown=loadedsounds[s].fadeDelayCountdown;
			soundsmap[lastIdUsed]->loopSaved=loadedsounds[s].loop;
			soundsmap[lastIdUsed]->loopCountdown=loadedsounds[s].loop;
			//use this only when we gonna use QMediaPlayer instead of QAudioOutput (QAudioOutput intialization error)
			//soundsmap[lastIdUsed]->needValidation = false;
			//soundsmap[lastIdUsed]->isValidated = true;
			if(!isPlayer){
				soundsmap[lastIdUsed]->play(); //if is a regular sond then play it, if is a player, then do not play it
			}
			soundID=lastIdUsed;
		}else{
			//there is no resource loaded with that ID
			if(*error)(*error)->q(ERROR_SOUNDRESOURCE);
		}
	}else if(QFileInfo(s).exists()){
		lastIdUsed++;
		soundsmap[lastIdUsed] = new Sound(this);
		soundsmap[lastIdUsed]->id = lastIdUsed;
		soundsmap[lastIdUsed]->error = error;
		soundsmap[lastIdUsed]->isPlayer = isPlayer; //set if this is a player that not delete sound at stop
		connect(this, SIGNAL(stopsSoundsAndWaiting()), soundsmap[lastIdUsed], SLOT(stopsSoundsAndWaiting()));
		connect(this, SIGNAL(systemMassCommand(int)), soundsmap[lastIdUsed], SLOT(systemMassCommand(int)));
		connect(soundsmap[lastIdUsed], SIGNAL(deleteMe(int)), this, SLOT(deleteMe(int)));
		soundsmap[lastIdUsed]->media = new QMediaPlayer(soundsmap[lastIdUsed]);
		soundsmap[lastIdUsed]->type = SOUNDTYPE_FILE;
		soundsmap[lastIdUsed]->source = s;
		soundsmap[lastIdUsed]->prepareConnections();
		//there are cases when an invalid file as "text.txt" is not detected as QMediaPlayer::InvalidMedia and signal is not emitted
		//If we stop a program like below, program freezes when we try to delete this kind of sound... we should request validation
		//    i= 100000
		//    x=soundplay("license.txt")
		//    loop:
		//    i=i-1 : soundpause : soundplay
		//    print soundposition ; " " ; (100000-i)
		//    if i > 0 then goto loop
		soundsmap[lastIdUsed]->needValidation = true;
		soundsmap[lastIdUsed]->isValidated = false;
		soundsmap[lastIdUsed]->media->setMedia(QUrl::fromLocalFile(QFileInfo(s).absoluteFilePath()));
		soundsmap[lastIdUsed]->updatedMasterVolume(masterVolume);

		if(!isPlayer) soundsmap[lastIdUsed]->play(); //if it is a regular sound then play it, if it is a player, then do not play it
		soundID=lastIdUsed;
	}else if (url.isValid() && (url.scheme()=="http" || url.scheme()=="https" || url.scheme()=="ftp")){
		lastIdUsed++;
		soundsmap[lastIdUsed] = new Sound(this);
		soundsmap[lastIdUsed]->id = lastIdUsed;
		soundsmap[lastIdUsed]->error = error;
		soundsmap[lastIdUsed]->isPlayer = isPlayer; //set if this is a player that not delete sound at stop
		connect(this, SIGNAL(stopsSoundsAndWaiting()), soundsmap[lastIdUsed], SLOT(stopsSoundsAndWaiting()));
		connect(this, SIGNAL(systemMassCommand(int)), soundsmap[lastIdUsed], SLOT(systemMassCommand(int)));
		connect(soundsmap[lastIdUsed], SIGNAL(deleteMe(int)), this, SLOT(deleteMe(int)));
		soundsmap[lastIdUsed]->media = new QMediaPlayer(soundsmap[lastIdUsed]);
		soundsmap[lastIdUsed]->type = SOUNDTYPE_WEB;
		soundsmap[lastIdUsed]->source = s;
		soundsmap[lastIdUsed]->prepareConnections();
		soundsmap[lastIdUsed]->media->setMedia(QUrl::fromUserInput(s));
		soundsmap[lastIdUsed]->updatedMasterVolume(masterVolume);
		//see above
		soundsmap[lastIdUsed]->needValidation = true;
		soundsmap[lastIdUsed]->isValidated = false;
		if(!isPlayer) soundsmap[lastIdUsed]->play(); //if is a regular sond then play it, if is a player, then do not play it
		soundID=lastIdUsed;
	}else{
		//Unable to load sound file
		if(*error)(*error)->q(ERROR_SOUNDFILE);
	}
	return soundID;
}


int SoundSystem::playSound(std::vector<std::vector<double>> sounddata, bool isPlayer) {
	soundID=0;
	if(soundSystemIsStopping) return 0;
	lastIdUsed++;
	soundsmap[lastIdUsed] = new Sound(this);
	soundsmap[lastIdUsed]->id = lastIdUsed;
	soundsmap[lastIdUsed]->error = error;
	soundsmap[lastIdUsed]->isPlayer = isPlayer; //set if this is a player that not delete sound at stop
	connect(this, SIGNAL(stopsSoundsAndWaiting()), soundsmap[lastIdUsed], SLOT(stopsSoundsAndWaiting()));
	connect(this, SIGNAL(systemMassCommand(int)), soundsmap[lastIdUsed], SLOT(systemMassCommand(int)));
	connect(soundsmap[lastIdUsed], SIGNAL(deleteMe(int)), this, SLOT(deleteMe(int)));
	soundsmap[lastIdUsed]->byteArray  = generateSound(sounddata);
	soundsmap[lastIdUsed]->buffer = new QBuffer(soundsmap[lastIdUsed]->byteArray);
	soundsmap[lastIdUsed]->buffer->open(QIODevice::ReadWrite);
	soundsmap[lastIdUsed]->buffer->seek(0);
	soundsmap[lastIdUsed]->type = SOUNDTYPE_GENERATED;
	soundsmap[lastIdUsed]->audio = new QAudioOutput(format,soundsmap[lastIdUsed]);
	soundsmap[lastIdUsed]->updatedMasterVolume(masterVolume);
	soundsmap[lastIdUsed]->sound_samplerate=sound_samplerate;
	soundsmap[lastIdUsed]->prepareConnections();
	//use this only when we gonna use QMediaPlayer instead of QAudioOutput (QAudioOutput intialization error)
	//soundsmap[lastIdUsed]->needValidation = false;
	//soundsmap[lastIdUsed]->isValidated = true;
	if(!isPlayer) soundsmap[lastIdUsed]->play(); //if is a regular sond then play it, if is a player, then do not play it
	soundID=lastIdUsed;
	return soundID;
}

void SoundSystem::loadSoundFromArray(QString id, QByteArray* arr){
	if (loadedsounds.contains(id)) delete(loadedsounds[id].byteArray);
	loadedsounds[id].byteArray = new QByteArray(*arr);
	//file loaded into memory... need validation at first play
	loadedsounds[id].needValidation = true;
	loadedsounds[id].isValidated = false;
	loadedsounds[id].individualVolume = 1.0;
	loadedsounds[id].scheduledFade = false;
	loadedsounds[id].fadeVolume=1.0;
	loadedsounds[id].fadeCountdown=0;
	loadedsounds[id].fadeDelayCountdown=0;
	loadedsounds[id].loop=1;
}


QString SoundSystem::loadSoundFromVector(std::vector<std::vector<double> > sounddata) {
	//generate a sound, load it into memory and return ID of loaded resource like "beep:23"
	lastBeepLoaded++;
	QString id = QString("beep:") + QString::number(lastBeepLoaded);
	if (loadedsounds.contains(id)) delete(loadedsounds[id].byteArray);
	loadedsounds[id].byteArray = generateSound(sounddata);
	//we don´t need a validation - it is not a media file, but the next step is to use QMediaPlayer instead of QAudioOutput when QAudioOutput fails at initialization
	loadedsounds[id].needValidation = false;
	loadedsounds[id].isValidated = true;
	loadedsounds[id].individualVolume = 1.0;
	loadedsounds[id].scheduledFade = false;
	loadedsounds[id].fadeVolume=1.0;
	loadedsounds[id].fadeCountdown=0;
	loadedsounds[id].fadeDelayCountdown=0;
	loadedsounds[id].loop=1;
	return id;
}

QString SoundSystem::loadRaw(std::vector<double> raw) {
	//build a sound from raw data, load it into memory and return ID of loaded resource like "beep:23"
	const int size = raw.size();
	std::vector<int16_t> sounddata;
	sounddata.resize(size);
	for(int i=0;i<size;i++){
		double d = raw[i];
		if(d>1.0)d=1.0;
		if(d<-1.0)d=-1.0;
		sounddata[i]=d*SOUND_HALFWAVE;
	}
	lastBeepLoaded++;
	QString id = QString("beep:") + QString::number(lastBeepLoaded);
	loadedsounds[id].byteArray = new QByteArray(reinterpret_cast<const char*>(sounddata.data()), size*sizeof(int16_t));
	//we don´t need a validation - it is not a media file, but the next step is to use QMediaPlayer instead of QAudioOutput when QAudioOutput fails at initialization
	loadedsounds[id].needValidation = false;
	loadedsounds[id].isValidated = true;
	loadedsounds[id].individualVolume = 1.0;
	loadedsounds[id].scheduledFade = false;
	loadedsounds[id].fadeVolume=1.0;
	loadedsounds[id].fadeCountdown=0;
	loadedsounds[id].fadeDelayCountdown=0;
	loadedsounds[id].loop=1;
	return id;
}

bool SoundSystem::unloadSound(QString id){
	if (loadedsounds.contains(id)){
		for (auto it = soundsmap.begin(); it != soundsmap.end(); ){
			if(it->second->source == id){
				if(it->second->soundState != 0) it->second->stop();
				delete(it->second);
				soundsmap.erase(it++);
			}else{
				++it;
			}
		}
		delete(loadedsounds[id].byteArray);
		loadedsounds.remove(id);
		return true;
	}
	return false;
}

QByteArray*  SoundSystem::generateSound(std::vector<std::vector<double>> sounddata) {
	// mix lists of sounds in the to be played in the same time (mixed sounds)
	// the vector or vectors soundata contains a vector for each voice
	// a voice contains freq/dir pairs in order to play them

	double wave;
	double frequency;
	short s;
	char *cs = (char *) &s;
	QByteArray* array = new QByteArray();
	QBuffer buffer(array);
	std::vector<double> raw;
	int max_env=sound_envelope_length-1; //last envelope element


	updateWaveformSample();


	buffer.open(QIODevice::ReadWrite|QIODevice::Truncate);

	if(sounddata.size() == 1 && sound_envelope_release==0){
		//one voice - there is no need to mix voices, so we can write directly to buffer
		//sound_envelope_release==0 - no release effect == no overlaping sounds
		wave=0.0;
		double rest=0.0;
		for(unsigned int index=0; index<sounddata[0].size(); index+=2) {
			//keep rest and use next time... for precision less than .02 ms
			// dim a(2000)
			// for i=0 to 999
			// a[i*2]=i+100
			// a[i*2+1]=.01
			// next i
			// sound a
			double length_double = ((double)sound_samplerate * sounddata[0][index+1] / 1000.0) + rest;
			int length = (int) length_double;
			rest=length_double-(double)length;
			frequency = fabs(sounddata[0][index]);
			if(frequency==0.0) { // silance (put out zeros)
				wave=0.0;
				s=0;
				for(int i = 0; i < length; i++) {
					buffer.write(cs,sizeof(int16_t));
				}
			} else {
				//if we use a sound envelope
				if(sound_envelope_length>0){
					for(int i = 0; i < length; i++) {
						s = waveformsample[(int)wave]*sound_envelope[i<max_env?i:max_env];
						buffer.write(cs,sizeof(int16_t));
						wave=fmod(wave+frequency, sound_samplerate);
					}
					//simple, plain sound
				}else{
					for(int i = 0; i < length; i++) {
						s = waveformsample[(int)wave];
						buffer.write(cs,sizeof(int16_t));
						wave=fmod(wave+frequency, sound_samplerate);
					}
				}
			}
		}
	}else{
		//Raw mix multiple voices
		long int rawlength=0, x;
		for(unsigned int voice=0; voice<sounddata.size(); voice++){
			wave=0.0;
			x=0;
			double rest=0.0;
			for(unsigned int index=0; index<sounddata[voice].size(); index+=2) {
				double length_double = ((double)sound_samplerate * sounddata[voice][index+1] / 1000.0) + rest;
				int length = (int) length_double;
				rest=length_double-(double)length;
				if(x+length>rawlength){
					rawlength=x+length;
					raw.resize(rawlength,0.0);
				}
				frequency = fabs(sounddata[voice][index]);
				if(frequency==0.0) { //silance
					x+=length;
				} else {





						//if we use a sound envelope
						if(sound_envelope_length>0){
							double e = 0.0;
							for(int i = 0; i < length; i++) {
								e = sound_envelope[i<max_env?i:max_env]; //envelope amplitude value
								raw[x] += (double)waveformsample[(int)wave]*e;
								x++;
								wave=fmod(wave+frequency, sound_samplerate);
							}
							if(e>0.0 && sound_envelope_release>0 && sound_envelope_release_bit>0.0){
							//we got a valid release effect
								int rlen=e/sound_envelope_release_bit;
								if(rlen>0){
									int xx=x;
									//double wwave=wave;
									if(xx+rlen>rawlength){
										rawlength=xx+rlen;
										raw.resize(xx+rlen,0.0);
									}
									for(int i = 0; i < rlen; i++) {
										e = e-sound_envelope_release_bit; //envelope amplitude value
										//raw[xx] += (double)waveformsample[(int)wwave]*e;
										raw[xx] += (double)waveformsample[(int)wave]*e;
										xx++;
										//wwave=fmod(wwave+frequency, sound_samplerate);
										wave=fmod(wave+frequency, sound_samplerate);
									}
									wave=0.0;

								}

							}
						 }else{
						 //simple, plain sound
							for(int i = 0; i < length; i++) {
								raw[x] += (double)waveformsample[(int)wave];
								x++;
								wave=fmod(wave+frequency, sound_samplerate);
							}
						}






				}
			}
		}

		double max=SOUND_HALFWAVE; // max value of mixed sounds. If is greater, then normalize
		const int normalizestandby = sound_samplerate * sound_normalize_ms / 1000; // the period to keep normalize (1 sec)
		const int normalizefade = sound_samplerate * sound_fade_ms / 1000; //the time period  to fade volume to normal value after a normalize period (eceeded sound)
		double fadebit; // this will keep the value to substract from max (volume) in the fade sequence
		int counter=0; //used to count the normalize standby period
		long int lastexceeded = - 10;

		//the magic of mixing sounds (fit sounds in maximum volume and keep ratio)
		for(x=0;x<rawlength;x++){
			//check if volume is exceeding and change max value to scale sound into range
			if(raw[x]<=-max){
				max = raw[x]*-1;
				counter = normalizestandby;
				fadebit = (max-(double)SOUND_HALFWAVE)/(double)normalizefade;
				lastexceeded=x;
			}else if(raw[x]>=max){
				max = raw[x];
				counter = normalizestandby;
				fadebit = (max-(double)SOUND_HALFWAVE)/(double)normalizefade;
				lastexceeded=x;
			}else if(lastexceeded==x-1){
				//normalize also the previous zone to avoid flat top of sound wave
				long int pos=lastexceeded;
				//rewind to the begining of the exceeding wave (negative/positive)
				if(raw[lastexceeded]>0){
					while(pos>0 && raw[pos]>0) pos--;
				}else{
					while(pos>0 && raw[pos]<0) pos--;
				}
				buffer.seek(pos*sizeof(int16_t));
				// set the new counter to the true begining of normalized zone
				counter = normalizestandby-(lastexceeded-pos);
				//write the new values
				for(;pos<=lastexceeded;pos++){
					s = (int16_t) (raw[pos]* (double) SOUND_HALFWAVE / max);
					buffer.write(cs,sizeof(int16_t));
				}
			}

			//wait a period of time (normalizestandby) before to fade to normal volume again
			//just in case the exceeding volume is still used
			if(counter>0){
				counter--;
			}else{
				//fade to normal volume
				if(max>SOUND_HALFWAVE){
					max-=fadebit;
				}
			}
			s = (int16_t) (raw[x]* (double) SOUND_HALFWAVE / max);
			buffer.write(cs,sizeof(int16_t));
		}
	}
	//for debug
	/*
	buffer.seek(0);
	QByteArray z = buffer.readAll();
	QFile file("some_name.ext");
	file.open(QIODevice::WriteOnly);
	file.write(z);
	//file.write(cs,sizeof(int16_t));
	file.close();
	*/
	buffer.seek(0);
	return array;
}

void SoundSystem::updateWaveformSample(void){
	//update waveformsample to speed up generating sounds that contain harmonics
	//re-create waveformsample only when is needed instead mixing harmonics everytime when generate each sound
	if(!mustUpdateWaveformSample) return;
	mustUpdateWaveformSample = false;

	if(sound_harmonics.size()==1){
		//if there are no harmonics
		if(sound_harmonics[1]==1.0){
			waveformsample = currentwaveform;
		}else{
			const double v = sound_harmonics[1];
			for(int i=0;i<sound_samplerate;i++){
				waveformmixedwithharmonics[i] = (int16_t) ((double)currentwaveform[i]*v);
			}
			waveformsample = waveformmixedwithharmonics;
		}
	}else{
		std::vector<double> raw;
		raw.resize(sound_samplerate, 0.0);
		for (std::map<int,double>::iterator it=sound_harmonics.begin(); it!=sound_harmonics.end(); ++it){
			const double v = it->second;
			const int h = it->first;
			int ii=0;
			for(int i=0;i<sound_samplerate;i++){
				raw[i]+=(double)currentwaveform[ii]*v;
				ii=(ii+h)%sound_samplerate;
			}
		}
		double max = (double)SOUND_HALFWAVE;
		for(int i=0;i<sound_samplerate;i++) if(raw[i]>fabs(max)) max=raw[i];
		const double ratio = (double)SOUND_HALFWAVE / max;
		for(int i=0;i<sound_samplerate;i++){
			waveformmixedwithharmonics[i]=(int16_t) (raw[i]*ratio);
		}
		waveformsample = waveformmixedwithharmonics;
	}
}

void SoundSystem::pause(int sound){
	if(soundSystemIsStopping) return;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)) soundsmap[sound]->pause();
}

double SoundSystem::position(int sound){
	if(soundSystemIsStopping) return 0.0;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)) return soundsmap[sound]->position();
	return 0.0;
}

void SoundSystem::seek(int sound, double pos){
	if(soundSystemIsStopping) return;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)){
		bool s = soundsmap[sound]->seek(pos);
		if(!s && *error)(*error)->q(WARNING_SOUNDNOTSEEKABLE);
	}
}

int SoundSystem::state(int sound){
	if(soundSystemIsStopping) return 0;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)) return soundsmap[sound]->state();
	return 0;
}

void SoundSystem::stop(int sound){
	if(soundSystemIsStopping) return;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)) soundsmap[sound]->stop();
}

void SoundSystem::wait(int sound){
	if(soundSystemIsStopping) return;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)) soundsmap[sound]->wait();
}

double SoundSystem::length(int sound) {
	if(soundSystemIsStopping) return 0.0;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)){
		double l = soundsmap[sound]->length();
		if(l>=0.0) return l;
		if(*error)(*error)->q(WARNING_SOUNDLENGTH);
	}
	return 0.0;
}

void SoundSystem::volume(int sound, double vol){
	//set individual volume for specified sound
	if(soundSystemIsStopping) return;
	vol=vol/10.0;
	if(vol<0.0) vol=0.0;
	if(vol>1.0) vol=1.0;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)){
		if(soundsmap[sound]->fadeTimer.isActive()) soundsmap[sound]->fadeTimer.stop();
		soundsmap[sound]->individualVolume=vol;
		soundsmap[sound]->updatedMasterVolume(masterVolume);
	}
}

void SoundSystem::volume(QString id, double vol){
	//set implicit value for individual volume for a loaded sound
	//every future play of this sound resource will use this value by default
	//There is also a bug https://bugreports.qt.io/browse/QTBUG-43765
	//Adjustments to the volume should change the volume of this specified stream, not the global volume.
	//QT Fix Version/s: 5.6.3, 5.7.2, 5.8.0

	if(soundSystemIsStopping) return;
	vol=vol/10.0;
	if(vol<0.0) vol=0.0;
	if(vol>1.0) vol=1.0;
	if (loadedsounds.contains(id)){
		loadedsounds[id].individualVolume=vol;
	}else{
		//there is no resource loaded with that ID
		if(*error)(*error)->q(ERROR_SOUNDRESOURCE);
	}
}


void SoundSystem::loop(int sound, int loop){
	if(soundSystemIsStopping) return;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)){
		soundsmap[sound]->loopSaved = loop;
		soundsmap[sound]->loopCountdown = loop;
	}
}

void SoundSystem::loop(QString id, int loop){
	if(soundSystemIsStopping) return;
	if (loadedsounds.contains(id)){
		loadedsounds[id].loop=loop;
	}else{
		//there is no resource loaded with that ID
		if(*error)(*error)->q(ERROR_SOUNDRESOURCE);
	}
}

void SoundSystem::fade(int sound, double vol, int ms, int delay){
	//fade volume for specified sound to the new value during ms
	if(soundSystemIsStopping) return;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)){
		if(soundsmap[sound]->fadeTimer.isActive())
			soundsmap[sound]->fadeTimer.stop();
		if(ms<SOUNDFADEMS && delay<SOUNDFADEMS){
			volume(sound, vol);
			return;
		}
		vol=vol/10.0;
		if(vol<0.0) vol=0.0;
		if(vol>1.0) vol=1.0;
		soundsmap[sound]->fadeVolume=vol;
		soundsmap[sound]->fadeCountdown=ms/SOUNDFADEMS;
		soundsmap[sound]->fadeDelayCountdown=delay/SOUNDFADEMS;
		soundsmap[sound]->fadeTimer.start(SOUNDFADEMS, soundsmap[sound]);
	}
}

void SoundSystem::fade(QString id, double vol, int ms, int delay){
	//fade volume for specified sound to the new value during ms
	if(soundSystemIsStopping) return;
	vol=vol/10.0;
	if(vol<0.0) vol=0.0;
	if(vol>1.0) vol=1.0;
	if (loadedsounds.contains(id)){
		if(ms<SOUNDFADEMS && delay<SOUNDFADEMS){
			loadedsounds[id].scheduledFade=false;
			loadedsounds[id].individualVolume=vol;
		}else{
			loadedsounds[id].scheduledFade=true;
			loadedsounds[id].fadeVolume=vol;
			loadedsounds[id].fadeCountdown=ms/SOUNDFADEMS;
			loadedsounds[id].fadeDelayCountdown=delay/SOUNDFADEMS;
		}
	}else{
		//there is no resource loaded with that ID
		if(*error)(*error)->q(ERROR_SOUNDRESOURCE);
	}
}

void SoundSystem::play(int sound){
	if(soundSystemIsStopping) return;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)){
		soundsmap[sound]->play();
	}
}

void SoundSystem::setMasterVolume(double volume) {
	// volume MUST be betwen 0(mute) and 10(all the way)
	if(soundSystemIsStopping) return;
	if (volume < 0.0) volume = 0;
	if (volume > 10.0) volume = 10.0;
	masterVolume = volume;
	for ( auto it = soundsmap.begin(); it != soundsmap.end(); ++it )
		it->second->updatedMasterVolume(masterVolume);
}

double SoundSystem::getMasterVolume() {
	return masterVolume;
}

void SoundSystem::waveform(int wave){
	if(soundSystemIsStopping) return;
	switch(wave){
	case 1:
		currentwaveform = waveformsquare;
		break;
	case 2:
		currentwaveform = waveformtriangle;
		break;
	case 3:
		currentwaveform = waveformsaw;
		break;
	case 4:
		currentwaveform = waveformpulse;
		break;
	default:
		currentwaveform = waveformsin;
	}
	mustUpdateWaveformSample = true; //update waveform of the sound next time we generate one
}

void SoundSystem::envelope(std::vector<double> e){
	if(soundSystemIsStopping) return;
	double vol1, vol2, duration_double, rest=0.0;
	int duration=0, index=0, i=0;
	int const max = sound_samplerate * 20;
	int const size = (int) e.size() - 4;
	sound_envelope_length=0;
	sound_envelope_release=0;
	sound_envelope_release_bit=0.0;
	sound_envelope.clear();

	vol2=e[i];
	if(vol2<0.0)vol2=0.0;
	if(vol2>1.0)vol2=1.0;

	while(i<=size){
		vol1=vol2;
		i++;
		duration_double=(e[i]*(double)sound_samplerate/1000.0)+rest; //add previus rest too
		duration=(int)duration_double;
		rest=duration_double-(double)duration; //do not lose small pieces of length

		i++;
		vol2=e[i];
		if(vol2<0.0)vol2=0.0;
		if(vol2>1.0)vol2=1.0;

		if(duration>=0.0){
			sound_envelope_length=sound_envelope_length+duration;
			if(sound_envelope_length>max){
				sound_envelope_length=0;
				sound_envelope_release=0;
				sound_envelope_release_bit=0.0;
				sound_envelope.clear();
				if(*error)(*error)->q(ERROR_ENVELOPEMAX);
				return;
			}
			sound_envelope.resize(sound_envelope_length,0.0);
			double bit=(vol2-vol1)/(double)duration;
			while(duration>0){
				sound_envelope[index]=vol1;
				vol1+=bit;
				index++;
				duration--;
			}
		}
	}
	sound_envelope_length++;
	sound_envelope.resize(sound_envelope_length,vol2);
	i++;
	sound_envelope_release = (int)(e[i]*(double)sound_samplerate/1000.0);
	if(sound_envelope_release>0){
		if(vol2<=0.0) vol2=1.0; //if sustain amplitude (last amplitude value from list) is 0, then the release is the time in samples to fade from 1.0 ampitude to 0.0
		sound_envelope_release_bit=vol2/(double)sound_envelope_release;
	}else{
		sound_envelope_release_bit=0.0;
	}
}

void SoundSystem::noenvelope(){
	if(soundSystemIsStopping) return;
	sound_envelope_length=0;
	sound_envelope_release=0;
	sound_envelope_release_bit=0.0;
	sound_envelope.clear();

}

void SoundSystem::noharmonics(){
	if(soundSystemIsStopping) return;
	sound_harmonics.clear();
	sound_harmonics[1]=1.0;
	mustUpdateWaveformSample = true; //update waveform of the sound next time we generate one
}

void SoundSystem::harmonics(int h, double a){
	//fundamental tone / 1st harmonic must always exists
	//and yes, it can have 0.0 amplitude
	if(soundSystemIsStopping) return;
	if(a<0.0) a=0.0;
	if(a>1.0) a=1.0;
	if(a==0.0 && h!=1){
		sound_harmonics.erase(h);
	}else{
		sound_harmonics[h]=a;
	}
	mustUpdateWaveformSample = true; //update waveform of the sound next time we generate one
}

void SoundSystem::playerOff(int sound){
	if(soundSystemIsStopping) return;
	if(sound<0) sound=soundID;
	if(soundsmap.count(sound)){
		soundsmap[sound]->isPlayer=false;
		soundsmap[sound]->stopsSoundsAndWaiting();
	}
}

void SoundSystem::system(int i){
	//0 - stop all playing sounds
	//1 - play/resume all sounds paused with previous soundsystem(2) command.
	//2 - pause all playing sounds
	//3 - stop and delete all sounds and players
	//4 - play/resume all paused sounds
	if(soundSystemIsStopping) return;
	emit(systemMassCommand(i));
}

int SoundSystem::samplerate(){
	return sound_samplerate;
}

void SoundSystem::customWaveform(std::vector<double> wave, bool logic){
	//qDebug() << "start";
	double totallen=0.0, position=0.0, target=0.0;
	int size = wave.size();

	//if user create a perfect wave...
	if(size==sound_samplerate && !logic){
		for(int i=0;i<sound_samplerate;i++){
			if(wave[i]<-1.0){
				waveformcustom[i]=-1.0;
			}else if(wave[i]>1.0){
				waveformcustom[i]=1.0;
			}else{
				waveformcustom[i]=wave[i];
			}
		}
		currentwaveform = waveformcustom;
		mustUpdateWaveformSample = true; //update waveform of the sound next time we generate one
		return;
	}


	if(!logic){
		//transform raw data into logical data
		wave.resize(size*2+1,wave[0]);
		while(size>0){
			size--;
			wave[size*2]=wave[size];
			wave[size*2+1]=1.0;
		}
		size = wave.size();
	}

	//prepare input from user and calculate the total length of given waveform
	for(int i=0; i<size; i++){
		//sanitize input
		if(i%2==0){
			if(wave[i] < -1.0){
				wave[i] = -1.0;
			}else if(wave[i] > 1.0){
				wave[i] = 1.0;
			}
		}else{
			//total length
			if(wave[i] < 0.0) wave[i] = 0.0; //set negative duration to 0
			totallen+=wave[i];
		}
	}
	if(size%2==0){
		size++;
		wave.resize(size,wave[0]);//make sure to close the wave with amplitude value
	}

	if(totallen==0.0){
		for(int i = 0; i < sound_samplerate; i++)waveformcustom[i]=0;
	}else{
		int w=0;
		for(int i = 0; i < sound_samplerate; i++) {
			target=totallen/(double)sound_samplerate*(double)i;
			while(wave[w+1]+position<=target && w<size-3){
				position+=wave[w+1];
				w+=2;
			}
			if(wave[w+1]!=0){
				double angle = (wave[w+2]-wave[w])/wave[w+1];
				waveformcustom[i]=SOUND_HALFWAVE *(wave[w]+(target-position)*angle);
			}else{
				waveformcustom[i]=SOUND_HALFWAVE *wave[w];
			}
		}
	}

	currentwaveform = waveformcustom;
	mustUpdateWaveformSample = true; //update waveform of the sound next time we generate one
	//qDebug() << "end";
}
