# Makefile to build class 'rubberband' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.
#
# use : make pdincludepath=/path/to/pure-data/src/
#
# The following command will build the external and install the distributable
# files into a subdirectory called build/rubberband :
#
# make install pdincludepath=../pure-data/src/ objectsdir=./build

RUBBERBAND_VERSION = 3.1.2

RUBBERBAND = rubberband-$(RUBBERBAND_VERSION)

lib.name = rubbershifter~

$(lib.name).class.sources = rubbershifter.cpp $(RUBBERBAND)/single/RubberBandSingle.cpp

# all extra files to be included in binary distribution of the library
datafiles = rubbershifter~-help.pd LICENSE README.md

cflags = -Wno-unused -Wno-unused-parameter -Wno-cast-function-type -std=c++11 -I$(RUBBERBAND)/rubberband

PDLIBBUILDERDIR ?= .
include $(PDLIBBUILDERDIR)/Makefile.pdlibbuilder

VERSION = 0.0.0
#$(shell git describe --abbrev=4)

update-pdlibbuilder:
	curl https://raw.githubusercontent.com/pure-data/pd-lib-builder/master/Makefile.pdlibbuilder > ./Makefile.pdlibbuilder

download-rubberband:
	curl https://breakfastquay.com/files/releases/$(RUBBERBAND).tar.bz2 > $(RUBBERBAND).tar.bz2
	tar -xjf $(RUBBERBAND).tar.bz2 $(RUBBERBAND)
	rm $(RUBBERBAND).tar.bz2

deken-source:
	@rm -rf build_src
	@mkdir -p build_src/rubbershifter
	@cp rubbershifter.cpp  $(datafiles) Makefile.pdlibbuilder Makefile build_src/rubbershifter
	cd build_src/ ; deken upload -v $(VERSION) rubbershifter

deken-binary:
	@rm -rf build
	@make install objectsdir=./build
	cd build/ ; deken upload -v $(VERSION) rubbershifter


