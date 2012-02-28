######################################################################
# qmake (2.00)
######################################################################

#TEMPLATE = app

#CONFIG += thread release
CONFIG += thread debug

#QT += webkit
QT += network

DEPENDPATH += . src
INCLUDEPATH += . src dao/kernel dao/modules/auxlib

DEFINES += DAO_WITH_MACRO
DEFINES += DAO_WITH_THREAD
DEFINES += DAO_WITH_NUMARRAY
DEFINES += DAO_WITH_CONCURRENT
DEFINES += DAO_WITH_DYNCLASS
DEFINES += DAO_WITH_DECORATOR

DESTDIR = .

# Input
HEADERS += src/daoConsole.h \
		   src/daoEditor.h \
		   src/daoCodeSHL.h \
		   src/daoInterpreter.h \
		   src/daoDebugger.h \
		   src/daoMonitor.h \
		   src/daoStudio.h \
		   src/daoStudioMain.h

SOURCES += src/daoConsole.cpp \
		   src/daoEditor.cpp \
		   src/daoCodeSHL.cpp \
		   src/daoInterpreter.cpp \
		   src/daoDebugger.cpp \
		   src/daoMonitor.cpp \
		   src/daoStudio.cpp \
		   src/daoStudioMain.cpp

FORMS += src/daoStudio.ui \
		 src/daoMonitor.ui \
		 src/daoAbout.ui \
		 src/daoHelpVIM.ui

#QMAKE_CXXFLAGS += -fPIC

RESOURCES += DaoStudio.qrc
TRANSLATIONS = langs/daostudio_zh_cn.ts

PREPROCESS_FILES = dummy.cpp

makedao.name = Dao
makedao.input = PREPROCESS_FILES
makedao.output = dummy.o
makedao.commands = make -f Makefile.dummy && cd dao && make lib
makedao.clean_commands = cd dao && make clean
#makedao.variable_out = SOURCES
#makedao.depends = Makefile

DEPENDPATH += dao
DEPENDPATH += dao/modules/auxlib

LIBS += -Ldao -Ldao/modules/auxlib -ldao -ldao_aux

QMAKE_EXTRA_COMPILERS += makedao

win32 {
	RC_FILE = DaoStudio.rc
	DEFINES += WIN32
	#LIBS += -lwinmm -lwsock32
}
unix {
	DEFINES += UNIX
	LIBS += -lz -lssl -lcrypto
	#QMAKE_LFLAGS += -Xlinker -rpath -Xlinker .
}
mac {
	ICON = icons/daostudio.icns
	QMAKESPEC = macx-g++
	DEFINES += UNIX MAC_OSX
	#LIBS += -lz -lssl -lcrypto
	QMAKE_LFLAGS += -Wl,-rpath,dao -Wl,-rpath,dao/modules/auxlib

	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
}

