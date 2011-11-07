GameOfLife WFSCollider - Wave Field Synthesis spatialization for SuperCollider.
===============================================================================

GameOfLife WFSCollider is an adapted version of SuperCollider, the audio synthesis engine and programming language, for Wave Field Synthesis spatialization.

It's currently being used in the 192 speakers system of the [Game Of Life Foundation](http://gameoflife.nl/en), based in Leiden, the Netherlands.

WFSCollider consists of an audio spatialization engine that places individual sound sources in space according to the principles of [Wave Field Synthesis](http://en.wikipedia.org/wiki/Wave_field_synthesis).

The system is currently allows to import mono sound files and place them in a score editor where start times, and durations can be set and trajectories or positions assigned to each event.

Each score can be saved as an xml file.

## System Requirements ##

Mac OS X 10.5 or greater

## Download ##

A prepackaged version is available from the download section at [github](https://github.com/GameOfLife/WFSCollider).

## Installation ##

To install, just drag the application to your applications folder.

## Building ##

Get the source:

	git clone --recursive git://github.com/GameOfLife/WFSCollider.git

Go to the folder created and switch to the wfscurrent_cmake_sample_sched branch

	git checkout wfscurrent_cmake_sample_sched

then
    mkdir build
    cd build
    cmake -D standalone:string=WFSCollider -DSC_QT=OFF -DCMAKE_BUILD_TYPE=Release ..
    make install -j8
for dual cores use -j4 for quad cores -j8, etc. The application will then be in ./build/WFSCollider/

For development work do

    cmake -D standalone:string=WFSCollider -DSC_QT=OFF -DWFS=DEV -DCMAKE_BUILD_TYPE=Release ..

For deployment to the GOL system:

    cmake -D standalone:string=WFSCollider -DSC_QT=OFF -DWFS=GAMEOFLIFE -DCMAKE_BUILD_TYPE=Release ..

For release

With supernova:

    cmake -D standalone:string=WFSCollider -DSC_QT=OFF -DWFS=OFFLINE -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES='i386;x86_64' -DSUPERNOVA=ON -DCMAKE_OSX_SYSROOT=Developer/SDKs/MacOSX10.6.sdk ..

without supernova:

    cmake -D standalone:string=WFSCollider -DSC_QT=OFF -DWFS=OFFLINE -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES='i386;x86_64' -DCMAKE_OSX_SYSROOT="/Developer/SDKs/MacOSX10.6.sdk" ..

And until 3.5 lib bundling is fixed:
    dylibbundler -of -b -x WFSCollider.app/Contents/MacOS/WFSCollider -p @executable_path/../Resources/ -d WFSCollider.app/Contents/Resources/

## Acknowledgments ##
WFSCollider was conceived by the Game Of Life Foundation, and developed by W. Snoei, R. Ganchrow and J. Truetzschler and M. Negrão.

## License ##
Both SuperCollider and the WFSCollider library are licensed under the GNU GENERAL PUBLIC LICENSE Version 3.  
