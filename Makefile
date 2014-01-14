
PLATS = linux freebsd mingw

MODE ?= release
SUFFIX ?=

DAOMAKE_DIR = ./dao/tools/daomake/bootstrap
DAOMAKE = $(DAOMAKE_DIR)/daomake

MAC_INSTALL = --option-INSTALL-PATH ../DaoStudio.app/Contents/Frameworks

$(PLATS) :
	cd $(DAOMAKE_DIR) && $(MAKE) -f Makefile $@
	$(DAOMAKE) mkdir2 build
	cd build && ../$(DAOMAKE) --mode $(MODE) --platform $@ --suffix .daomake  ../dao
	cd build && $(MAKE) -f Makefile.daomake
	qmake -o Makefile.qmake DaoStudio.pro
	$(MAKE) -f Makefile.qmake
	cd build && $(MAKE) -f Makefile.daomake install

macosx :
	cd $(DAOMAKE_DIR) && $(MAKE) -f Makefile $@
	$(DAOMAKE) mkdir2 build
	cd build && ../$(DAOMAKE) --mode $(MODE) --platform $@ --suffix .daomake $(MAC_INSTALL) ../dao
	cd build && $(MAKE) -f Makefile.daomake
	qmake -spec macx-g++ -o Makefile.qmake DaoStudio.pro
	$(MAKE) -f Makefile.qmake
	cd build && $(MAKE) -f Makefile.daomake install
	./update_bundle.sh

clean :
	make -f Makefile.daomake clean
	make -f Makefile.qmake clean
