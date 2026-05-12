package org.basic256;

import android.app.Activity;
import android.content.Intent;
import android.speech.tts.TextToSpeech;
import android.speech.tts.TextToSpeech.OnInitListener;
import org.qtproject.qt5.android.bindings.QtActivity;

public class AndroidTTSHelper {

	//Copyright (C) 2010 James M. Reneau
	//
	// This program is free software; you can redistribute it and/or modify it
	// under the terms of the GNU General Public License as published by the
	// Free Software Foundation; either version 2 of the License, or (at your option) any later version.
	//
	// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
	// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
	//
	// You should have received a copy of the GNU General Public License along with this program;
	// if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	// Boston, MA 02111-1307, USA
	//
	// Originally developed for the mmtag assistive technology project
	//
	// Modification History
	// Date...... Programmer... Description...
	// 2010-02-07 j.m.reneau    original coding
	// 2014-01-01 j.m.reneau    moved to basic256 - changed package and name
	
	private boolean ttsInitialized = false;
	private TextToSpeech ttsEngine;
	
	public AndroidTTSHelper() {
		ttsEngine = new TextToSpeech(QtActivity.getQtActivityInstance(), onInitListener);
	}

	private OnInitListener onInitListener = new TextToSpeech.OnInitListener()
	{
	    public void onInit(int stuff)
	    {
	    	ttsInitialized = true;
	    }
	};

	public void say(String text) {
		if (ttsInitialized) {
			ttsEngine.speak(text, TextToSpeech.QUEUE_FLUSH, null);
			// wait for say to finish
			while(ttsEngine.isSpeaking()) {
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					break;
				}
			}
		}
	}
	
}
