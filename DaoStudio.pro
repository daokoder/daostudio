######################################################################
# qmake (2.00)
######################################################################

#TEMPLATE = app

CONFIG += thread release
#CONFIG += thread debug

#QT += webkit
QT += network

DEPENDPATH += . src
INCLUDEPATH += . src dao/kernel dao/modules/serializer
INCLUDEPATH += dao/modules/debugger dao/modules/profiler

DEFINES += DAO_WITH_MACRO
DEFINES += DAO_WITH_THREAD
DEFINES += DAO_WITH_NUMARRAY
DEFINES += DAO_WITH_CONCURRENT
DEFINES += DAO_WITH_DYNCLASS
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
DEPENDPATH += build/modules/serializer
DEPENDPATH += build/modules/debugger
DEPENDPATH += build/modules/profiler

LIBS += -Lbuild -Lbuild/modules/serializer
LIBS += -Lbuild/modules/debugger -Lbuild/modules/profiler
LIBS += -ldao -ldao_serializer -ldao_debugger -ldao_profiler


win32 {
	POST_TARGETDEPS += build/dao.dll build/modules/serializer/dao_serializer.dll
	POST_TARGETDEPS += build/modules/debugger/dao_debugger.dll
	POST_TARGETDEPS += build/modules/profiler/dao_profiler.dll
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
	POST_TARGETDEPS += build/libdao.dylib build/modules/serializer/libdao_serializer.dylib
	POST_TARGETDEPS += build/modules/debugger/libdao_debugger.dylib
	POST_TARGETDEPS += build/modules/profiler/libdao_profiler.dylib
	ICON = icons/daostudio.icns
	QMAKESPEC = macx-g++
	DEFINES += UNIX MAC_OSX
	#LIBS += -lz -lssl -lcrypto
	QMAKE_LFLAGS += -Wl,-rpath,build -Wl,-rpath,build/modules/serializer
	QMAKE_LFLAGS += -Wl,-rpath,build/modules/debugger
	QMAKE_LFLAGS += -Wl,-rpath,build/modules/profiler

	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
} else:unix{
	POST_TARGETDEPS += build/libdao.so build/modules/serializer/libdao_serializer.so
	POST_TARGETDEPS += build/modules/debugger/libdao_debugger.so
	POST_TARGETDEPS += build/modules/profiler/libdao_profiler.so
}

