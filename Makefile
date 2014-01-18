
PLATS = linux freebsd

MODE ?= release
SUFFIX ?=

DAOMAKE_DIR = ./dao/tools/daomake/bootstrap
DAOMAKE = $(DAOMAKE_DIR)/daomake

WIN_INSTALL_DIR = DaoStudio
WIN_INSTALL = --option-INSTALL-PATH ../$(WIN_INSTALL_DIR)
MAC_INSTALL = --option-INSTALL-PATH ../DaoStudio.app/Contents/Frameworks

$(PLATS) :
	cd $(DAOMAKE_DIR) && $(MAKE) -f Makefile $@
	$(DAOMAKE) mkdir2 build
	cd build && ../$(DAOMAKE) --mode $(MODE) --platform $@ --suffix .daomake  ../dao
	cd build && $(MAKE) -f Makefile.daomake
	qmake -o Makefile.qmake DaoStudio.pro
	$(MAKE) -f Makefile.qmake

macosx :
	cd $(DAOMAKE_DIR) && $(MAKE) -f Makefile $@
	$(DAOMAKE) mkdir2 build
	cd build && ../$(DAOMAKE) --mode $(MODE) --platform $@ --suffix .daomake $(MAC_INSTALL) ../dao
	cd build && $(MAKE) -f Makefile.daomake
	qmake -spec macx-g++ -o Makefile.qmake DaoStudio.pro
	$(MAKE) -f Makefile.qmake
	cd build && $(MAKE) -f Makefile.daomake install
	./update_bundle.sh

mingw :
	cd $(DAOMAKE_DIR) && $(MAKE) -f Makefile $@
	cd build && ..\dao\tools\daomake\bootstrap\daomake --mode $(MODE) --platform $@ --suffix .daomake $(WIN_INSTALL) ..\dao
	cd build && $(MAKE) -f Makefile.daomake
	qmake -o Makefile.qmake DaoStudio.pro
	$(MAKE) -f Makefile.qmake
	cd build && $(MAKE) -f Makefile.daomake install
	$(DAOMAKE) copy $(MODE)/DaoStudio.exe $(WIN_INSTALL_DIR)
	$(DAOMAKE) copy build/dao.dll $(WIN_INSTALL_DIR)
	$(DAOMAKE) copy build/modules/serializer/dao_serializer.dll $(WIN_INSTALL_DIR)
	$(DAOMAKE) copy build/modules/debugger/dao_debugger.dll $(WIN_INSTALL_DIR)
	$(DAOMAKE) copy build/modules/profiler/dao_profiler.dll $(WIN_INSTALL_DIR)
	$(DAOMAKE) copy $(QT_DIR)/bin/mingw*.dll $(WIN_INSTALL_DIR)
	$(DAOMAKE) copy $(QT_DIR)/bin/QtCore4.dll $(WIN_INSTALL_DIR)
	$(DAOMAKE) copy $(QT_DIR)/bin/QtGui4.dll $(WIN_INSTALL_DIR)
	$(DAOMAKE) copy $(QT_DIR)/bin/QtNetwork4.dll $(WIN_INSTALL_DIR)


clean :
	cd build && make -f Makefile.daomake clean
	make -f Makefile.qmake clean
