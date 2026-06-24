BASIC-256 is modern retro BASIC programming environment for learning coding and having fun. 
It was originally called Kidbasic and was started in 2007, but today in its current state, it is already quite capable for everyday hobby use.
A Qt5-based program, it has a 3-pane IDE with edit-, output- and graphics-windows. 
![Interface](Basic256.png)

It can also be called from the command-line/Terminal with the following options:

| Short | Long  | #2    |
| :---: | :---: | :---: |
|-? -h|--help|Display command-line help.|  
||--help-all|Display command-line help including Qt-specific options.|  
|-v|--version|Display the BASIC-256 version.|
|-r|--run|Load and immediately run the specified .kbs program. Must precede the filename.|
|-a|--app|Run specified .kbs without Edit window | 
|-g|--graph|Run specified .kbs with only Graphics window | 
|-t|--text|Run specified .kbs with only Text window | 
|-l|--lang --languageset|Start BASIC-256 using the specified language.|

One can even make shortcut on the desktop with a .bat file like:
      @echo off
      C:\PATH_TO_BASIC256\basic256.exe -g Mandelbrot-256.kbs
to have a file run as if it was an application.

The original code and current downloadable version resides on SourceForge (https://sourceforge.net/projects/kidbasic/) and is at version 2.0.0.11, which launched in 2020. It comes with an Example directory but most programs there need to be updated. There is also a Testsuite directory but this does not come with the Installer.

Unfortunately, development of Basic256 has apparently been stopped  after a failed attempt to port it to Qt6.

This new  GitHub repository (Github - uglymike17/basic256) is my attempt to restart Basic256 and takes the v2.0.99.10.2 the branch with the aim of trying to modernize the codebase into a v2.1.

The initial aim for this branch is to provide a modern toolchain and stabilisation
 
 - port from qmake to CMake and from minGW to MSVC
   ==> Seems to have gone ok, although some issues cropped up. (eg: A DIM statement should now always add 'fill 0' to prevent issues)
 - make clean-ups & modernisations where possible. 
   ==> fix some leaks
   ==> removed depreciated Qt operands.
 - make it compile on Windows, Linux-x86 and Linux-ARM (RPi) and MacOS Silicon.
   ==> All architecture build ok but need more testing.

 Current status as of 20/06/2026
 ********************************
 There is a single build.yml file that builds and packages all architectures via scripts. At the moment I produce an install .exe for Windows (nsi based) and a standalone Windows .zip file, a tarball for Linux-x86 and one for Linux-ARM and a zip file for MacOS, no deb yet
 1. Windows .zip file seems to be working. (more testing required)
 2. Windows .exe file seems to be working. (more testing required)
 2. Linux x86 seems to be working. (more testing required)
 3. Raspberry Pi is problematic on Trixie. Github runner Ubuntu-24.4-ARM has a different Qt5 than Trixie. These do not mix and as Trixie does not include several required Qt5 libraries, I have to bundle all the runner's Qt5 libs. Even with this, speech does not work out-of-the-box since Trixie does not come with speech-dispatcher so this MUST be installed. So RPi build works but the tar ball is very big due to all the bundled libraries.
 4. MacOS Silicon (M1,M2,M3) resulted in an Homebrew basic256 app, but having no developer license, I added an ad-hoc signing. This should prevent the message: "basic256.app" is damaged and can't be opened. This should show the messge "unidentified developer" instead which one can hopefully click through. Most recent MacOS' do not allow this, I'm told. There is however a possibility to add your own Developer ID in the script and so to open a path to notarisation which would allow installation on modern MacOS versions.
 
Future actions
**************************
First thing on the agenda is to get the word out so anybody could give their feedback on it. 
A debian file would also be nice but is of lower priority (Linux Distro's provide their own Basic256 package, so this would only become required after major improvements to the code). 
Help with the debian packaging would be very much appreciated

Once this main branch is stable enough, I (we?) can start adding small features/bugfixes. A big whishlist item would be to switch to the qscintilla editor, but this is not urgent.
Ideally there should later be an uncoupling of interpreter code, GUI code, CLI code (?) etc in order to be able to compile it into WebAssembly. (one can dream...). Finally, a Qt6 migration is long overdue

Also, example files should be updated to reflect more modern machines, the wiki-based help at doc.basic256.org should be updated and a renewed website would be required. Neither of these is under my control...

Remark
**********************
I'm mainly a Basic256 fan (see https://uglymike.static.domains/) and have practically no knowledge of github, c++ or 'real' programming and project managment.
I'm using free accounts on chatGPT, Claude, Google's Gemini and Perplexity to get where I am now.