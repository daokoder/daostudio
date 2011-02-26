######################################################################
# qmake (2.00)
######################################################################

TEMPLATE = app
#CONFIG += thread release
CONFIG += thread debug
#QT += webkit
QT += network
DEPENDPATH += . src
INCLUDEPATH += . src

DESTDIR = .

# Input
#HEADERS += src/daoCodeSHL.h src/daoMonitor.h
#SOURCES += src/daoCodeSHL.cpp src/daoMonitor.cpp
#FORMS += src/daoMonitor.ui src/daoDataWidget.ui

HEADERS += src/daoCodeSHL.h src/daoDebugger.h src/daoMonitor.h
SOURCES += src/daoCodeSHL.cpp src/daoDebugger.cpp src/daoMonitor.cpp
FORMS += src/daoMonitor.ui src/daoDataWidget.ui src/daoAbout.ui

#QMAKE_CXXFLAGS += -fPIC

RESOURCES += daostudio.qrc
TRANSLATIONS = langs/daostudio_zh_cn.ts

win32 {
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
	QMAKESPEC = macx-g++
	INCLUDEPATH += ../dao/kernel
	DEFINES += UNIX MAC_OSX
	#LIBS += -ldao
	LIBS += ../dao/dao.a
  #QMAKE_LFLAGS += -L. -lDaoQtCore -lDaoQtGui -lDaoQtNetwork
}
