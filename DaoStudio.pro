######################################################################
# qmake (2.00)
######################################################################

#TEMPLATE = app

CONFIG += thread release
#CONFIG += thread debug

#QT += webkit
QT += network

DEPENDPATH += . src
INCLUDEPATH += . src Dao/kernel Dao/modules/serializer
INCLUDEPATH += Dao/modules/debugger Dao/modules/profiler

DEFINES += DAO_WITH_THREAD
DEFINES += DAO_WITH_NUMARRAY
DEFINES += DAO_WITH_CONCURRENT
DEFINES += DAO_WITH_DECORATOR
DEFINES += DAO_USE_CODE_STATE

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

DEPENDPATH += build
DEPENDPATH += build/lib/dao/modules

LIBS += -Lbuild -Lbuild/lib/dao/modules
LIBS += -ldao -ldao_serializer -ldao_debugger -ldao_profiler


win32 {
	POST_TARGETDEPS += build/bin/dao.dll build/lib/dao/modules/dao_serializer.dll
	POST_TARGETDEPS += build/lib/dao/modules/dao_debugger.dll
	POST_TARGETDEPS += build/lib/dao/modules/dao_profiler.dll
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
	POST_TARGETDEPS += build/lib/libdao.dylib build/lib/dao/modules/libdao_serializer.dylib
	POST_TARGETDEPS += build/lib/dao/modules/libdao_debugger.dylib
	POST_TARGETDEPS += build/lib/dao/modules/libdao_profiler.dylib
	ICON = icons/daostudio.icns
	QMAKESPEC = macx-g++
	DEFINES += UNIX MAC_OSX
	#LIBS += -lz -lssl -lcrypto
	QMAKE_LFLAGS += -Wl,-rpath,build/lib -Wl,-rpath,build/lib/dao/modules

	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
} else:unix{
	POST_TARGETDEPS += build/lib/libdao.so build/lib/dao/modules/libdao_serializer.so
	POST_TARGETDEPS += build/lib/dao/modules/libdao_debugger.so
	POST_TARGETDEPS += build/lib/dao/modules/libdao_profiler.so
	QMAKE_LFLAGS += -Wl,-rpath,build/lib -Wl,-rpath,build/lib/dao/modules
}

