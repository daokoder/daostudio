
PLATS = linux freebsd

MODE ?= release
SUFFIX ?=

DAOMAKE_DIR = ./dao/tools/daomake/bootstrap
DAOMAKE = $(DAOMAKE_DIR)/daomake

INSTALL     = DaoStudio-0.5-Beta
INSTALL_BIN = $(INSTALL)/bin

WIN_INSTALL     = $(INSTALL)
WIN_INSTALL_BIN = $(INSTALL)/bin

OPT_INSTALL     = --option-INSTALL-PATH ../$(INSTALL)
OPT_INSTALL_MAC = --option-INSTALL-PATH ../DaoStudio.app/Contents/Frameworks
OPT_INSTALL_WIN = --option-INSTALL-PATH ../$(WIN_INSTALL)

QT_LIB_DIR = /usr/lib/i386-linux-gnu

all:
	@echo "Please choose a platform!"

$(PLATS) :
	cd $(DAOMAKE_DIR) && $(MAKE) -f Makefile $@
	$(DAOMAKE) mkdir2 build
	cd build && ../$(DAOMAKE) --mode $(MODE) --suffix .daomake --option-CODE-STATE ON --option-HELP-PATH ../Dao/modules/help --option-HELP-FONT "WenQuanYi Micro Hei Mono" $(OPT_INSTALL) ../dao
	cd build && $(MAKE) -f Makefile.daomake
	qmake -o Makefile.qmake DaoStudio.pro
	$(MAKE) -f Makefile.qmake

linux-local:
	cd $(DAOMAKE_DIR) && $(MAKE) -f Makefile linux
	$(DAOMAKE) mkdir2 build
	cd build && ../$(DAOMAKE) --mode $(MODE) --platform linux --suffix .daomake --option-CODE-STATE ON --option-HELP-PATH ../Dao/modules/help --option-HELP-FONT "WenQuanYi Micro Hei Mono" $(OPT_INSTALL) ../dao
	cd build && $(MAKE) -f Makefile.daomake
	qmake -o Makefile.qmake DaoStudio.pro
	$(MAKE) -f Makefile.qmake
	cd build && $(MAKE) -f Makefile.daomake install
	$(DAOMAKE) mkdir2 $(INSTALL)/langs
	$(DAOMAKE) copy langs/daostudio_zh_cn.qm $(INSTALL)/langs/
	$(DAOMAKE) copy DaoStudio $(INSTALL_BIN)
	$(DAOMAKE) copy build/dao $(INSTALL_BIN)
	$(DAOMAKE) copy build/libdao.so $(INSTALL_BIN)
	$(DAOMAKE) copy build/modules/serializer/libdao_serializer.so $(INSTALL_BIN)
	$(DAOMAKE) copy build/modules/debugger/libdao_debugger.so $(INSTALL_BIN)
	$(DAOMAKE) copy build/modules/profiler/libdao_profiler.so $(INSTALL_BIN)
	$(DAOMAKE) copy $(QT_LIB_DIR)/libQtCore.so $(INSTALL_BIN)
	$(DAOMAKE) copy $(QT_LIB_DIR)/libQtGui.so $(INSTALL_BIN)
	$(DAOMAKE) copy $(QT_LIB_DIR)/libQtNetwork.so $(INSTALL_BIN)
	$(DAOMAKE) echo "LD_LIBRARY_PATH=$(INSTALL_BIN) $(INSTALL_BIN)/DaoStudio" | $(DAOMAKE) cat2 $(INSTALL)/DaoStudio
	chmod +x $(INSTALL)/DaoStudio

macosx :
	cd $(DAOMAKE_DIR) && $(MAKE) -f Makefile $@
	$(DAOMAKE) mkdir2 build
	cd build && ../$(DAOMAKE) --mode $(MODE) --suffix .daomake --option-CODE-STATE ON --option-HELP-PATH ../Dao/modules/help --option-HELP-FONT "WenQuanYi Micro Hei Mono" $(OPT_INSTALL_MAC) ../dao
	cd build && $(MAKE) -f Makefile.daomake
	qmake -spec macx-g++ -o Makefile.qmake DaoStudio.pro
	$(MAKE) -f Makefile.qmake
	cd build && $(MAKE) -f Makefile.daomake install
	./update_bundle.sh

mingw :
	cd $(DAOMAKE_DIR) && $(MAKE) -f Makefile $@
	$(DAOMAKE) mkdir2 build
	cd build && ../$(DAOMAKE) --mode $(MODE) --platform $@ --suffix .daomake --option-CODE-STATE ON --option-HELP-PATH ../Dao/modules/help --option-HELP-FONT "WenQuanYi Micro Hei Mono" $(OPT_INSTALL_WIN) ../dao
	cd build && $(MAKE) -f Makefile.daomake
	$(QT_DIR)/bin/qmake -o Makefile.qmake DaoStudio.pro
	$(MAKE) -j64 -f Makefile.qmake
	cd build && $(MAKE) -f Makefile.daomake install
	$(DAOMAKE) mkdir2 $(WIN_INSTALL)/langs
	$(DAOMAKE) copy langs/daostudio_zh_cn.qm $(WIN_INSTALL)/langs/
	$(DAOMAKE) copy $(MODE)/DaoStudio.exe $(WIN_INSTALL_BIN)
	$(DAOMAKE) copy build/dao.exe $(WIN_INSTALL_BIN)
	$(DAOMAKE) copy build/dao.dll $(WIN_INSTALL_BIN)
	$(DAOMAKE) copy build/modules/serializer/dao_serializer.dll $(WIN_INSTALL_BIN)
	$(DAOMAKE) copy build/modules/debugger/dao_debugger.dll $(WIN_INSTALL_BIN)
	$(DAOMAKE) copy build/modules/profiler/dao_profiler.dll $(WIN_INSTALL_BIN)
	$(DAOMAKE) copy $(QT_DIR)/bin/QtCore4.dll $(WIN_INSTALL_BIN)
	$(DAOMAKE) copy $(QT_DIR)/bin/QtGui4.dll $(WIN_INSTALL_BIN)
	$(DAOMAKE) copy $(QT_DIR)/bin/QtNetwork4.dll $(WIN_INSTALL_BIN)

# -j64, to fix:
# QWinEventNotifier: Cannot have more than 62 enabled at one time
# https://bugreports.qt-project.org/browse/QTBUG-8819

clean :
	cd build && make -f Makefile.daomake clean
	make -f Makefile.qmake clean
