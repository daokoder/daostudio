######################################################################
# qmake (2.00)
######################################################################

TEMPLATE = app
CONFIG += thread release
#CONFIG += thread debug
#QT += webkit
QT += network
DEPENDPATH += . src
INCLUDEPATH += . src dao/kernel libssh2/include libssh2/src

DEFINES += DAO_WITH_MACRO
DEFINES += DAO_WITH_NUMARRAY
DEFINES += DAO_WITH_THREAD
DEFINES += DAO_WITH_SYNCLASS

DESTDIR = .

# Input
HEADERS += src/daoConsole.h \
		   src/daoEditor.h \
		   src/daoCodeSHL.h \
		   src/daoDebugger.h \
		   src/daoMonitor.h \
		   src/daoStudio.h \
		   src/daoStudioMain.h \
		   dao/kernel/dao.h \
		   dao/kernel/daolib.h \
		   dao/kernel/daoBase.h \
		   dao/kernel/daoType.h \
		   dao/kernel/daoStdtype.h \
		   dao/kernel/daoNamespace.h \
		   dao/kernel/daoGC.h \
		   dao/kernel/daoNumtype.h \
		   dao/kernel/daoClass.h \
		   dao/kernel/daoLexer.h \
		   dao/kernel/daoParser.h \
		   dao/kernel/daoMacro.h \
		   dao/kernel/daoAsmbc.h \
		   dao/kernel/daoRegex.h \
		   dao/kernel/daoValue.h \
		   dao/kernel/daoContext.h \
		   dao/kernel/daoProcess.h \
		   dao/kernel/daoStdlib.h \
		   dao/kernel/daoArray.h \
		   dao/kernel/daoMap.h \
		   dao/kernel/daoConst.h \
		   dao/kernel/daoRoutine.h \
		   dao/kernel/daoObject.h \
		   dao/kernel/daoThread.h \
		   dao/kernel/daoSched.h \
		   dao/kernel/daoStream.h \
		   dao/kernel/daoString.h \
		   dao/kernel/daoVmspace.h \
		   libssh2/include/libssh2.h \
           libssh2/include/libssh2_publickey.h \
           libssh2/include/libssh2_sftp.h \
		   libssh2/src/libssh2_config.h\
           libssh2/src/channel.h \
           libssh2/src/comp.h \
           libssh2/src/crypto.h \
           libssh2/src/libgcrypt.h \
           libssh2/src/libssh2_priv.h \
           libssh2/src/mac.h \
           libssh2/src/misc.h \
           libssh2/src/openssl.h \
           libssh2/src/packet.h \
           libssh2/src/session.h \
           libssh2/src/sftp.h \
           libssh2/src/transport.h \
           libssh2/src/userauth.h

SOURCES += src/daoConsole.cpp \
		   src/daoEditor.cpp \
		   src/daoCodeSHL.cpp \
		   src/daoDebugger.cpp \
		   src/daoMonitor.cpp \
		   src/daoStudio.cpp \
		   src/daoStudioMain.cpp \
		   dao/kernel/daoType.c \
		   dao/kernel/daoStdtype.c \
		   dao/kernel/daoNamespace.c \
		   dao/kernel/daoGC.c \
		   dao/kernel/daoNumtype.c \
		   dao/kernel/daoClass.c \
		   dao/kernel/daoLexer.c \
		   dao/kernel/daoParser.c \
		   dao/kernel/daoMacro.c \
		   dao/kernel/daoAsmbc.c \
		   dao/kernel/daoRegex.c \
		   dao/kernel/daoValue.c \
		   dao/kernel/daoContext.c \
		   dao/kernel/daoProcess.c \
		   dao/kernel/daoStdlib.c \
		   dao/kernel/daoArray.c \
		   dao/kernel/daoMap.c \
		   dao/kernel/daoConst.c \
		   dao/kernel/daoRoutine.c \
		   dao/kernel/daoObject.c \
		   dao/kernel/daoThread.c \
		   dao/kernel/daoSched.c \
		   dao/kernel/daoStream.c \
		   dao/kernel/daoString.c \
		   dao/kernel/daoVmspace.c \
		   libssh2/src/agent.c \
           libssh2/src/channel.c \
           libssh2/src/comp.c \
           libssh2/src/crypt.c \
           libssh2/src/global.c \
           libssh2/src/hostkey.c \
           libssh2/src/keepalive.c \
           libssh2/src/kex.c \
           libssh2/src/knownhost.c \
           libssh2/src/libgcrypt.c \
           libssh2/src/mac.c \
           libssh2/src/misc.c \
           libssh2/src/openssl.c \
           libssh2/src/packet.c \
           libssh2/src/pem.c \
           libssh2/src/publickey.c \
           libssh2/src/scp.c \
           libssh2/src/session.c \
           libssh2/src/sftp.c \
           libssh2/src/transport.c \
           libssh2/src/userauth.c \
           libssh2/src/version.c 

FORMS += src/daoStudio.ui \
		 src/daoMonitor.ui \
		 src/daoDataWidget.ui \
		 src/daoAbout.ui \
		 src/daoHelpVIM.ui

#QMAKE_CXXFLAGS += -fPIC

RESOURCES += DaoStudio.qrc
TRANSLATIONS = langs/daostudio_zh_cn.ts

win32 {
	RC_FILE = DaoStudio.rc
	DEFINES += WIN32
	LIBS += -lwinmm -lwsock32
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
	LIBS += -lz -lssl -lcrypto
}
