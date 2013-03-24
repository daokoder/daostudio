
PLATS = linux freebsd mingw

MODE ?= release
SUFFIX ?=

$(PLATS) :
	cd dao/tools/daomake && $(MAKE) -f Makefile.bootstrap $@
	cd dao && ./tools/daomake/daomake --mode $(MODE) --platform $@
	cd dao && $(MAKE) -f Makefile$(SUFFIX)
	qmake -o Makefile.qmake DaoStudio.pro
	make -f Makefile.qmake

macosx :
	cd dao/tools/daomake && $(MAKE) -f Makefile.bootstrap $@
	cd dao && ./tools/daomake/daomake --mode $(MODE) --platform $@
	cd dao && $(MAKE) -f Makefile$(SUFFIX)
	qmake -spec macx-g++ -o Makefile.qmake DaoStudio.pro
	make -f Makefile.qmake
	./update_bundle.sh

