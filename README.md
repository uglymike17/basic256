BASIC-256 is an easy to use version of BASIC designed to teach anybody how to program. It was/is aimed at teaching the beginnings of programming to youngsters. 
In fact the original name of Basic256 was Kidbasic, but it is already quite capable for everyday hobby use in its current state.
A Qt5-based program, it has a 3-pane IDE with edit-, output- and graphics-windows. 
The original code and current downloadable version resides on SourceForge (https://sourceforge.net/projects/kidbasic/) and is at version 2.0.0.11

Unfortunately, development of Basic256 has stopped apparently after a failed attempt to port it to Qt6.
Some have tried to get involved with development (RiOn and comick) and a most recent try has even moved sourceforge to github ( GitHub - comick/basic256)

This new  GitHub repository is my attempt to restart Basic256 and takes the r946 branch of the original code (the branch from which resulted the 2.0.0.11 version) with the aim of trying to modernize the codebase.

The aim for this branch is
 - make it compile on Windows, Linux-Intel and Linux-ARM (RPi) from a single Actions pipeline. (Qt being cross-platform, MacOS port may also be looked at).
 - port from qmake to CMake
 - make clean-ups & modernisations where possible. (currently synchronious ESpeak has already been replaced by asynchronious Qt texttoSpeech)

The far-off dream is to refactor the project files into
	core/        ← interpreter (no Qt)
	runtime/     ← execution engine
	ui/          ← Qt frontend
	cli/         ← optional

then to extract the interpreter so a WebAssembly would become possible to run Basic256 in a browser. 

All his might make it possible to relaunch Basic256 on a new, modern website like this A.I.-generated one:

<img width="1024" height="1536" alt="image" src="https://github.com/user-attachments/assets/32f4c601-2637-4f97-927e-7afc5226a163" />
