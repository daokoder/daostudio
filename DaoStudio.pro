######################################################################
# qmake (2.00)
######################################################################

#TEMPLATE = app

#CONFIG += thread release
CONFIG += thread debug

#QT += webkit
QT += network

DEPENDPATH += . src
INCLUDEPATH += . src dao/kernel dao/modules/serializer

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

DEPENDPATH += dao
DEPENDPATH += dao/modules/serializer

LIBS += -Ldao -Ldao/modules/serializer -ldao -ldao_serializer


win32 {
	POST_TARGETDEPS += dao/dao.dll dao/modules/serializer/dao_serializer.dll
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
	POST_TARGETDEPS += dao/libdao.dylib dao/modules/serializer/libdao_serializer.dylib
	ICON = icons/daostudio.icns
	QMAKESPEC = macx-g++
	DEFINES += UNIX MAC_OSX
	#LIBS += -lz -lssl -lcrypto
	QMAKE_LFLAGS += -Wl,-rpath,dao -Wl,-rpath,dao/modules/serializer

	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
} else:unix{
	POST_TARGETDEPS += dao/libdao.so dao/modules/serializer/libdao_serializer.so
}

