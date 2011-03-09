######################################################################
# qmake (2.00)
######################################################################

TEMPLATE = app
CONFIG += thread release
#CONFIG += thread debug
#QT += webkit
QT += network
DEPENDPATH += . src
INCLUDEPATH += . src

DESTDIR = .

# Input
HEADERS += src/daoConsole.h src/daoEditor.h src/daoCodeSHL.h src/daoDebugger.h src/daoMonitor.h src/daoStudio.h src/daoStudioMain.h
SOURCES += src/daoConsole.cpp src/daoEditor.cpp src/daoCodeSHL.cpp src/daoDebugger.cpp src/daoMonitor.cpp src/daoStudio.cpp src/daoStudioMain.cpp
FORMS += src/daoStudio.ui src/daoMonitor.ui src/daoAbout.ui src/daoHelpVIM.ui
#QMAKE_CXXFLAGS += -fPIC

RESOURCES += DaoStudio.qrc
TRANSLATIONS = langs/daostudio_zh_cn.ts

win32 {
	RC_FILE = DaoStudio.rc
	INCLUDEPATH += ..\dao\kernel
	DEFINES += WIN32
	LIBS += ..\dao\dao.a -lwinmm -lwsock32 -lmsvcp60
}
unix {
	INCLUDEPATH += ../dao/kernel
	DEFINES += UNIX
	#LIBS += -ldao
	LIBS += ../dao/dao.a
  #QMAKE_LFLAGS += -Xlinker -rpath -Xlinker .
}
mac {
	ICON = icons/daostudio.icns
	QMAKESPEC = macx-g++
	INCLUDEPATH += ../dao/kernel
	DEFINES += UNIX MAC_OSX
	#LIBS += -ldao
	LIBS += ../dao/dao.a
}
