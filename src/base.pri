##### Do NOT modify this!###############
CONFIG -= release debug
################################


###### START USER CONFIGURATION ########

#
# Choose debug or release build
#

CONFIG += debug
#CONFIG += release

#
# Add support for Jack / ALSA audio driver. If you have a 
# Mac OS X, ALSA support will be disabled automatically.
#

DEFINES += JACK_SUPPORT
DEFINES += ALSA_SUPPORT
DEFINES += PORTAUDIO_SUPPORT
#DEFINES += LV2_SUPPORT

DEFINES += STATIC_BUILD
DEFINES += PRECOMPILED_HEADER
#DEFINES += USE_MEM_CHECKER

#
# Uncomment the line which notes your CPU type
#

#QMAKE_CXXFLAGS_RELEASE += -mtune=pentium3
#QMAKE_CXXFLAGS_RELEASE += -mtune=pentium4
#QMAKE_CXXFLAGS_RELEASE += -mtune=pentium-m
# nocona seems to work better for core2duo
# QMAKE_CXXFLAGS_RELEASE += -march=nocona
# gcc 4.3 has a core2 flag, though it's not released yet.
#QMAKE_CXXFLAGS_RELEASE += -march=core2

#
# Use Memory Locking 
#

DEFINES += USE_MLOCK

####### END USER CONFIGURATION ########




CONFIG += warn_on \
#	  link_prl \
	  qt

QT += xml


MOC_DIR = build 
UI_DIR = build 
OBJECTS_DIR = build 

QMAKE_CXXFLAGS += $$system(pkg-config --cflags glib-2.0) -fstack-protector-all 
#QMAKE_CXXFLAGS += -fstack-protector-all 

debug {
#	DEFINES += RT_THREAD_CHECK
	DEFINES += USE_DEBUGGER
#	DEFINES += USE_MEM_CHECKER
}


unix {
	DEFINES += LINUX_BUILD

	release {
		
#		DEFINES += USE_DEBUGGER
		
#		QMAKE_CXXFLAGS_RELEASE += -g
#		QMAKE_CXXFLAGS_RELEASE += -finline-functions
		
		MACHINETYPE = $$system(arch)
		
		contains( MACHINETYPE, x86_64 )	{
			QMAKE_CXXFLAGS_RELEASE += -mtune=athlon64
		}
		
		contains( MACHINETYPE, i[456]86) {
			DEFINES += SSE_OPTIMIZATIONS
			QMAKE_CXXFLAGS_RELEASE += -msse -mfpmath=sse
		}
		
	}
	
	GCCVERSION = $$system(gcc -dumpversion)
	
	contains(DEFINES, PRECOMPILED_HEADER):CONFIG += precompile_header
	
	contains(GCCVERSION, [45].[012345].[012345] ) {
#		Makes only sense if there are loops to vectorize, which isn't the case (yet)
#		Maybe gcc 4.2 will do a better job on certain loops?
#		specially those in memops. would be nice if they get some optimization too...
#		and of course the ones in gdither etc.
#		QMAKE_CXXFLAGS_RELEASE += -ftree-vectorizer-verbose=2 -ftree-vectorize
	}
	
}

macx {
	DEFINES -= ALSA_SUPPORT
	DEFINES += MAC_OS_BUILD
	
	LIBS += -ljack
		
	QMAKE_LFLAGS_SONAME  = -Wl,-install_name,@executable_path/../Frameworks/
}

win32 { 
	DEFINES += WIN_BUILD
	DEFINES -= ALSA_SUPPORT
	DEFINES -= JACK_SUPPORT
	DEFINES -= USE_MLOCK
	DEFINES -= LV2_SUPPORT
}
