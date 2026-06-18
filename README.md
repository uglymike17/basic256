BASIC-256 is an easy to use version of BASIC designed to teach anybody how to program. It was/is aimed at teaching the beginnings of programming to youngsters. 
In fact the original name of Basic256 was Kidbasic and it started in 2007, but today in its current state, it is already quite capable for everyday hobby use.
A Qt5-based program, it has a 3-pane IDE with edit-, output- and graphics-windows. 
The original code and current downloadable version resides on SourceForge (https://sourceforge.net/projects/kidbasic/) and is at version 2.0.0.11, which launched in 2020. It has an example directory but all programs there need to be updated. There is also a Testsuite.

Unfortunately, development of Basic256 has stopped apparently after a failed attempt to port it to Qt6.
Some have tried to get involved with development (RiOn and comick) and a comick has even moved sourceforge to github ( GitHub - comick/basic256)

This new  GitHub repository (Github - uglymike17/basic256) is my attempt to restart Basic256 and takes the 2.0.99.10.2 the branch with the aim of trying to modernize the codebase.

The aim for this branch is
 - make it compile on Windows, Linux-Intel and Linux-ARM (RPi) and MacOS Silicon.
   ==> All architecture build ok but need more testing.
 - port from qmake to CMake and from minGW to MSVC
   ==> Seems to have gone ok, although some issues cropped up. (eg: A DIM statement should now always add 'fill 0' to prevent issues)
 - make clean-ups & modernisations where possible. 
   ==> fix some leaks
   ==> removed depreciated Qt operands.

 Current status as of 13/06/2026
 ********************************
 There is a single build.yml file that builds and packages all architectures via scripts. At the moment I only produce Tar balls and zip files, no deb or nsi yet
 1. Windows seems to be working. (more testing required)
 2. Linux x86 seems to be working. (more testing required)
 3. Raspberry Pi is problematic on Trixie. Github runner Ubuntu-24.4-ARM has Qt5.15.15 while Trixie has Qt5.15.13. These do not mix and as Trixie does not include several required Qt5 libraries, I have to bundle all the Qt5.15.15 libs. Even with this, speech does not work out-of-the-box since Trixie does not come with speech-dispatcher so this MUST be installed. So RPi build works but the tar ball is very big due to all the bundled libraries.
 4. MacOS Silicon (M1,M2,M3) resulted in an Homebrew basic256 app, but having no developer license, I added an ad-hoc signing. This should prevent the message: "basic256.app" is damaged and can't be opened. This should show the messge "unidentified developer" instead which one can click through. There is however a possibility to add your own Developer ID in the script.
 
Future actions
**************************
Next, I would like to get the word out so anybody could give their feedback on it. 
I updated the Windows installer .nsi file which was last updated in 2020. I have not yhet added it to the windws scripts. 
A debian file would also be nice but is of lower priority (Linux Distro's provide their own Basic256 package, so this would only become required after major improvements to the code). 
Help with the debian packaging would be very much appreciated

Once there (dreaming), then I (we?) can start at trying the move to Qt6 for the next major release.
Ideally there should be an uncoupling of interpreter code, GUI code, CLI code (?) etc in order to be able to compile it into WebAssembly. (one can dream...)

Also, example files should be updated to reflect more modern machines, the wiki-based help at doc.basic256.org should be updated and a renewed website would be required.

Remark
**********************
I'm just a Basic256 fan (see https://uglymike.static.domains/) and have no knowledge whatsoever of github, c++ or 'real' programming and project managment.
I'm making all this stuff up by using free accounts on chatGPT, Claude, Google's Gemini and Perplexity