/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 **/

// Sound opcodes.
//
// Interpreter::execByteCode() in Interpreter.cpp decodes each opcode and
// hands these ones to execSoundOp().  The cases are exactly as they were
// there: a break still ends the opcode, and the checks execByteCode()
// makes after every opcode still run when this returns.

#include "InterpreterPrivate.h"

void Interpreter::execSoundOp(int opcode) {
	switch(opcode) {

		case OP_SOUND:
		case OP_SOUNDPLAY:
		case OP_SOUNDPLAYER:
		case OP_SOUNDLOAD:
		{

			// sound - load and play - wait to finish
			// sound localfile
			// sound url
			// sound player# (int expression)
			// sound loadresource

			// soundplay - load and play - dont wait to finish 
			// soundplay localfile
			// soundplay url
			// soundplay player#  (int expression)
			// soundplay loadresource
			
			// soundplayer - create a player but do not start playing
			// returns int "player#" used in soundplay, soundstop...
			// player# = soundplayer( localfile )
			// player# = soundplayer( url )
			// player# = soundplayer( loadresource )
			
			// soundload - create a sound resource (used in sound, soundplay, soundplayer)
			// returns string "loadresource"
			// loadresource = soundload( localfile )
			// loadresource = soundload( url )
			
			DataElement *e = stack->popDE();			// RELEASE
			
			if (DataElement::getType(e) == T_STRING) {
				// a single string
				QString playsource = e->stringval;
#ifdef Q_OS_WASM
				// A file or URL handed straight to SOUND/SOUNDPLAY/SOUNDPLAYER is
				// registered as a "sound:" resource first - see MediaLoader.h.
				if(opcode!=OP_SOUNDLOAD && !playsource.startsWith("sound:") && !playsource.startsWith("beep:")){
					if(!media.registerPlaySource(playsource)){
						if(opcode==OP_SOUNDPLAYER) stack->pushInt(0);
						delete e;
						break;
					}
					playsource = QString("sound:") + playsource;
				}
#endif
				if(opcode==OP_SOUND || opcode==OP_SOUNDPLAY){
					mymutex->lock();
					emit(playSound(playsource, false));
					waitCond->wait(mymutex);
					int id = sound->soundID;
					mymutex->unlock();
					if(opcode==OP_SOUND) sound->wait(id);
				}else if(opcode==OP_SOUNDPLAYER){
					mymutex->lock();
					emit(playSound(playsource, true));
					waitCond->wait(mymutex);
					int id = sound->soundID;
					mymutex->unlock();
					stack->pushInt(id);
				}else{
					// OP_SOUNDLOAD
					stack->pushQString(media.loadSound(e->stringval));
				}
				delete e;
				break;
			 } else if(DataElement::getType(e) == T_INT){
				// a single int
				if(opcode==OP_SOUND||opcode==OP_SOUNDPLAY){
					mymutex->lock();
					emit(soundPlay(e->intval));
					waitCond->wait(mymutex);
					mymutex->unlock();
					if(opcode==OP_SOUND) sound->wait(e->intval);
					delete e;
					break;
				} else {
					error->q(ERROR_EXPECTEDSOUND);
				}
			} else if (DataElement::getType(e)==T_ARRAY) {
				// an array
				int columns = e->arrayCols();
				if(columns%2!=0){
					error->q(ERROR_ARRAYEVEN);
					delete e;
					break;
				}

				double i;
				int rows = e->arrayRows();

				std::vector < std::vector<double> > sounddata;

				for(int row = 0; row < rows; row++) {
					std::vector < double > v;	// vector containing duration
				   for (int col = 0; col < columns; col++) {
						DataElement *av = e->arrayGetData(row, col);			// DONT RELEASE
						if(col%2==0)
							i = convert->getMusicalNote(av);
						else
							i = convert->getFloat(av);
						//printf(">>%i\n",i);
						v.push_back(i);
					}
					sounddata.push_back( v );
				}

				if (!error->pending()){
					if(opcode==OP_SOUND || opcode==OP_SOUNDPLAY){
						mymutex->lock();
						emit(playSound(sounddata, false));
						waitCond->wait(mymutex);
						int id = sound->soundID;
						mymutex->unlock();
						if(opcode==OP_SOUND) sound->wait(id);
					}else if(opcode==OP_SOUNDPLAYER){
						mymutex->lock();
						emit(playSound(sounddata, true));
						waitCond->wait(mymutex);
						int id = sound->soundID;
						mymutex->unlock();
						stack->pushInt(id);
					}else{
						stack->pushQString(sound->loadSoundFromVector(sounddata));
					}
				}
			} else {
				//error invalid SOUND syntax
				error->q(ERROR_EXPECTEDSOUND);
				if(opcode==OP_SOUNDPLAYER) stack->pushInt(0);
				if(opcode==OP_SOUNDLOAD) stack->pushQString("");
			}
			//
			delete e;
		}
		break;

		case OP_SOUNDLOADRAW: {
			std::vector<double> sounddata;
			DataElement *d = stack->popDE();			// RELEASE
			if (DataElement::getType(d)==T_ARRAY) {
				if(d->arrayRows()==1){
					sounddata.resize(d->arrayCols());
					for(int col = 0; col < d->arrayCols(); col++) {
						sounddata[col]=convert->getFloat(d->arrayGetData(0,col));			// DONT RELEASE
					}
					stack->pushQString(sound->loadRaw(sounddata));
				} else {
					error->q(ERROR_ONEDIMENSIONAL);//error 1 dimensional!
				}
			} else {
				error->q(ERROR_ARRAYEXPR);
			}
			delete d;
		}
		break;

		case OP_SOUNDPAUSE: {
			int i = stack->popInt();
			// Must run on the GUI thread: pausing a QMediaPlayer stops
			// its internal timers, and "Timers cannot be stopped from
			// another thread". Route through the main thread like
			// OP_SOUNDSTOP rather than calling sound->pause() here.
			mymutex->lock();
			emit(soundPause(i));
			waitCond->wait(mymutex);
			mymutex->unlock();
		}
		break;

		case OP_SOUNDSTOP: {
			int i = stack->popInt();
			mymutex->lock();
			emit(soundStop(i));
			waitCond->wait(mymutex);
			mymutex->unlock();
		}
		break;

		case OP_SOUNDPLAYEROFF: {
			int i = stack->popInt();
			mymutex->lock();
			emit(soundPlayerOff(i));
			waitCond->wait(mymutex);
			mymutex->unlock();
		}
		break;

		case OP_SOUNDSYSTEM: {
			int i = stack->popInt();
			mymutex->lock();
			emit(soundSystem(i));
			waitCond->wait(mymutex);
			mymutex->unlock();
		}
		break;

		case OP_SOUNDSAMPLERATE: {
			stack->pushInt(sound->samplerate());
		}
		break;

		case OP_SOUNDWAIT: {
			int i = stack->popInt();
			sound->wait(i);
		}
		break;

		case OP_SOUNDNOHARMONICS: {
			sound->noharmonics();
		}
		break;

		case OP_SOUNDHARMONICS: {
			DataElement *de = stack->popDE();			// RELEASE
			if (DataElement::getType(de) == T_ARRAY) {
				if ((de->arrayRows()==1&&de->arrayCols()%2==0) || (de->arrayCols()==2)) {
					if (de->arrayRows()==1) {
						// data in a single row
						for(int col = 0; col < de->arrayCols(); col+=2) {
							sound->harmonics(convert->getInt(de->arrayGetData(0,col)), convert->getFloat(de->arrayGetData(0,col+1)));			// DONT RELEASE
						}
					} else {
						// data in mutiple columns
						for(int row = 0; row < de->arrayRows(); row++) {
							sound->harmonics(convert->getInt(de->arrayGetData(row,0)), convert->getFloat(de->arrayGetData(row,1)));			// DONT RELEASE
						}
					}
				} else {
					error->q(ERROR_HARMONICLIST);
				}
			} else {
				int h = stack->popInt();
				if(h<1){
					error->q(ERROR_HARMONICNUMBER);
				}else{
					sound->harmonics(h, convert->getFloat(de));
				}
			}
			delete de;
		}
		break;

		case OP_SOUNDNOENVELOPE: {
			sound->noenvelope();
		}
		break;

		case OP_SOUNDENVELOPE: {
			std::vector<double> envelope;
			DataElement *de = stack->popDE();			// RELEASE
			if (DataElement::getType(de) == T_ARRAY) {
				// get array of envelope data
				if(de->arrayRows()==1) {
					// level, duration, level, ... level, release - always an even count
					if(de->arrayCols()%2==0 && de->arrayCols()>=4){
						envelope.resize(de->arrayCols());
						for(int col =0; col < de->arrayCols(); col++) {
							envelope[col]=convert->getFloat(de->arrayGetData(0,col));			// DONT RELEASE
						}
					} else {
						error->q(ERROR_ENVELOPEODD);
					}
				} else {
					error->q(ERROR_ONEDIMENSIONAL);
				}
			} else {
				// get the 4 values and build own
				// (fill the outer 'envelope' declared above -- do NOT
				// redeclare it here, or sound->envelope() below receives
				// an empty vector and reads e[0] out of bounds)
				double r = convert->getFloat(de); //release
				double s = stack->popDouble(); //sustain
				double d = stack->popDouble(); //decrease
				double a = stack->popDouble(); //attack
				envelope.resize(6);
				envelope[0] = 0.0;
				envelope[1] = a;
				envelope[2] = 1.0;
				envelope[3] = d;
				envelope[4] = s;
				envelope[5] = r;
			}
			if(!error->pending()) sound->envelope(envelope);
			delete de;
		}
		break;

		case OP_SOUNDWAVEFORM: {
			bool logic = stack->popBool(); //if data is logical, not raw
			DataElement *e = stack->popDE();			// RELEASE
			if (DataElement::getType(e)==T_ARRAY){
				int columns = e->arrayCols();
				int rows = e->arrayRows();
				std::vector<double> wave;

				if(rows!=1){
					//clear stack from the rest of data
					stack->drop(columns);
					for(int i=1;i<rows;i++) stack->drop(stack->popInt());
					error->q(ERROR_ONEDIMENSIONAL); //Creating custom waveform request one dimensional array data
					break;
				}else{
					wave.resize(columns,0);
					while(columns>0){
						columns--;
						DataElement *av = e->arrayGetData(0, columns);
						wave[columns]=convert->getFloat(av);	// amplitudes are -1..1, durations fractional
					}
					if(!error->pending())
						sound->customWaveform(wave, logic);
				}
			}else{
				sound->waveform(convert->getInt(e));
			}
			delete e;
		}
		break;

		case OP_SOUNDSEEK: {
			double p = stack->popDouble();
			int i = stack->popInt();
			// Must run on the GUI thread: seeking a QMediaPlayer
			// starts/stops its internal timers (same reason as
			// OP_SOUNDPAUSE) -- route through the main thread.
			mymutex->lock();
			emit(soundSeek(i, p));
			waitCond->wait(mymutex);
			mymutex->unlock();
		}
		break;

		case OP_SOUNDVOLUME: {
			double v = stack->popDouble();
			DataElement *e = stack->popDE();			// RELEASE
			if (DataElement::getType(e) == T_STRING) {
				//because it do not stop any timer, it is safe to do it from current thread
				sound->volume(e->stringval, v);
			}else{
				mymutex->lock();
				emit(soundVolume(convert->getInt(e), v));
				waitCond->wait(mymutex);
				mymutex->unlock();
			}
			delete e;
		}
		break;

		case OP_SOUNDLOOP: {
			int l = stack->popInt();
			DataElement *e = stack->popDE();
			if (DataElement::getType(e) == T_STRING) {
				sound->loop(e->stringval, l);
			}else{
				sound->loop(convert->getInt(e), l);
			}
			delete e;
		}
		break;

		case OP_SOUNDID: {
			stack->pushInt(sound->soundID);
		}
		break;

		case OP_SOUNDPOSITION: {
			int i = stack->popInt();
			stack->pushDouble(sound->position(i));
		}
		break;

		case OP_SOUNDFADE: {
			double delay = stack->popDouble();
			double ms = stack->popDouble();
			double v = stack->popDouble();
			DataElement *e = stack->popDE();			// RELEASE
			if (DataElement::getType(e) == T_STRING) {
				//because it do not stop any timer, it is safe to do it from current thread
				sound->fade(e->stringval, v, int(ms*1000), int(delay*1000));
			}else{
			mymutex->lock();
			emit(soundFade(convert->getInt(e), v, int(ms*1000), int(delay*1000)));
			waitCond->wait(mymutex);
			mymutex->unlock();
			}
			delete e;			// both branches own e (RELEASE)
		}
		break;

		case OP_SOUNDLENGTH: {
			int i = stack->popInt();
			stack->pushDouble(sound->length(i));
		}
		break;

		case OP_SOUNDSTATE: {
			int i = stack->popInt();
			stack->pushInt(sound->state(i));
		}
		break;

		case OP_VOLUME: {
			// set the wave output height (volume 0-10)
			double volume = stack->popDouble();
			sound->setMasterVolume(volume);
		}
		break;

		case OP_SAY: {
			QString text = stack->popQString();
			// Bound the wait. speakWords() normally wakes us -- via the
			// desktop TTS loop, or via basic256SayFinished() from the
			// browser's onend/onerror -- but a speech engine can accept an
			// utterance and then report absolutely nothing: iOS/iPadOS
			// WebKit does exactly that whenever the utterance was queued
			// without a live user activation, and this was the only wait in
			// the interpreter with no safety net, so the program hung for
			// good. Budget from the text length -- speech runs at roughly
			// 10 characters/second, so 150ms each leaves 50% headroom and
			// a genuinely long sentence is never cut short.
			qint64 sayBudgetMs = 2000 + 150LL * (qint64)text.length();
			if(sayBudgetMs < 5000) sayBudgetMs = 5000;
			if(sayBudgetMs > 300000) sayBudgetMs = 300000;
			mymutex->lock();
			emit(speakWords(text));
			waitCond->wait(mymutex, QDeadlineTimer(sayBudgetMs));
			mymutex->unlock();
		}
		break;

		case OP_WAVPLAY: {
		//obsolete
			QString file = stack->popQString();
			if(file.compare("")!=0) {
				//warning
				error->q(WARNING_WAVOBSOLETE);
				//stop previous mediaplayer
				mymutex->lock();
				emit(soundStop(mediaplayer_id_legacy));
				waitCond->wait(mymutex);
				mymutex->unlock();
				//play new file as a mediaplayer
				mymutex->lock();
				emit(playSound(file, true));
				waitCond->wait(mymutex);
				mediaplayer_id_legacy = sound->soundID;
				mymutex->unlock();
			}
			// start playing mediaplayer
			mymutex->lock();
			emit(soundPlay(mediaplayer_id_legacy));
			waitCond->wait(mymutex);
			mymutex->unlock();
		}
		break;

		case OP_WAVSTOP: {
		//obsolete
			mymutex->lock();
			emit(soundStop(mediaplayer_id_legacy));
			waitCond->wait(mymutex);
			mymutex->unlock();
		}
		break;

		case OP_WAVWAIT: {
		//obsolete
			sound->wait(mediaplayer_id_legacy);
		}
		break;

		case OP_WAVLENGTH: {
		//obsolete
			stack->pushDouble(sound->length(mediaplayer_id_legacy));
		}
		break;

		case OP_WAVPAUSE: {
		//obsolete
			// GUI thread (see OP_SOUNDPAUSE) -- pausing touches media timers.
			mymutex->lock();
			emit(soundPause(mediaplayer_id_legacy));
			waitCond->wait(mymutex);
			mymutex->unlock();
		}
		break;

		case OP_WAVPOS: {
		//obsolete
			stack->pushDouble(sound->position(mediaplayer_id_legacy));
		}
		break;

		case OP_WAVSEEK: {
		//obsolete
			double pos = stack->popDouble();
			// GUI thread (see OP_SOUNDSEEK) -- seeking touches media timers.
			mymutex->lock();
			emit(soundSeek(mediaplayer_id_legacy, pos));
			waitCond->wait(mymutex);
			mymutex->unlock();
		}
		break;

		case OP_WAVSTATE: {
		//obsolete
			stack->pushInt(sound->state(mediaplayer_id_legacy));
		}
		break;

	}
}
