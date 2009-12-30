# Automake file for hdf5_handler
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

AM_YFLAGS = -d -v
AM_LFLAGS = -8

BUILT_SOURCES = $(GRAM_SRCS)

SUBDIRS = bes-testsuite

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libhdf5_module.la
dist_bin_SCRIPTS = bes-hdf5-data.sh

GRAM_SRCS = he5dds.tab.cc he5dds.tab.hh lex.he5dds.cc \
	he5das.tab.cc he5das.tab.hh lex.he5das.cc

HDF5SRC = HDF5Array.cc HDF5Byte.cc HDF5Float64.cc HDF5Grid.cc HDF5UInt32.cc \
	HDF5Int32.cc HDF5Sequence.cc HDF5Str.cc HDF5Structure.cc HDF5Url.cc \
	HDF5UInt16.cc HDF5Int16.cc HDF5Float32.cc \
	HDF5GridEOS.cc HDF5ArrayEOS.cc HE5ShortName.cc HE5CF.cc

HDF5HDR = HDF5Array.h HDF5Float64.h HDF5Int32.h HDF5Structure.h HDF5Url.h \
	HDF5Byte.h HDF5Grid.h HDF5Sequence.h HDF5UInt16.h HDF5Float32.h \
	HDF5Int16.h HDF5Str.h HDF5UInt32.h HDF5GridEOS.h HDF5ArrayEOS.h HE5ShortName.h HE5CF.h

SRCS = h5dds.cc h5das.cc h5get.cc HE5Parser.cc HDF5PathFinder.cc $(GRAM_SRCS)

HDRS = common.h h5dds.h h5das.h h5get.h HE5Parser.h HDF5PathFinder.h

SERVER_SRC = HDF5RequestHandler.cc HDF5Module.cc

SERVER_HDR = HDF5RequestHandler.h HDF5Module.h

libhdf5_module_la_SOURCES = $(HDF5SRC) $(SRCS) $(HDF5HDR)  $(HDRS) $(SERVER_SRC) $(SERVER_HDR)
libhdf5_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libhdf5_module_la_LDFLAGS = -avoid-version -module 
libhdf5_module_la_LIBADD = $(BES_DAP_LIBS) -L${hdf5_path}/lib -lhdf5 -lz

EXTRA_DIST = he5das.y he5dds.y he5das.lex  he5dds.lex OSX_Resources \
	data COPYING hdf5_handler.spec bes-hdf5-data.sh.in

CLEANFILES = *~ *.sum *.log *.output he5das.tab.cc he5das.tab.hh \
	lex.he5das.cc lex.he5dds.cc he5dds.tab.cc he5dds.tab.hh \
	bes-hdf5-data.sh

sample_datadir = $(datadir)/hyrax/data/hdf5
sample_data_DATA = data/tcompound.h5

do_subst = sed -e 's,[@]sysconfdir[@],$(sysconfdir),g' \
		   	   -e 's,[@]libdir[@],$(libdir),g' \
		   	   -e 's,[@]datadir[@],$(datadir),g'

bes-hdf5-data.sh: bes-hdf5-data.sh.in Makefile
	$(do_subst) < $(srcdir)/bes-hdf5-data.sh.in > bes-hdf5-data.sh
	chmod +x bes-hdf5-data.sh

# This makes sure that the handler modifies the bes which configure found
# even when distcheck is run.
bes-conf: bes-hdf5-data.sh
	(bes_prefix=`bes-config --prefix` ; \
	if test "$$bes_prefix" = "/usr" ; \
	 then \
	    ./bes-hdf5-data.sh /etc/bes/bes.conf /usr/lib/bes ;\
	else \
	    ./bes-hdf5-data.sh $$bes_prefix/etc/bes/bes.conf $$bes_prefix/lib/bes ; \
	fi)

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libhdf5_module_la_SOURCES)

###########################################################################

# Build the HDF-EOS scanner

lex.he5dds.cc: he5dds.lex he5dds.tab.cc he5dds.tab.hh
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<


lex.he5das.cc: he5das.lex he5das.tab.cc he5das.tab.hh
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<

# Build the HDF-EOS parser

he5dds.tab.cc he5dds.tab.hh: he5dds.y HE5Parser.h
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p he5dds -o he5dds.tab.cc $<
	if test -e he5dds.tab.cc.h; then mv he5dds.tab.cc.h he5dds.tab.hh; fi

he5das.tab.cc he5das.tab.hh: he5das.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p he5das -o he5das.tab.cc $<
	if test -e he5das.tab.cc.h; then mv he5das.tab.cc.h he5das.tab.hh; fi

###########################################################################

# Fortify targets.

.PHONY: fortify
fortify:
	sourceanalyzer -b @PACKAGE@ $(MAKE)
	sourceanalyzer -b @PACKAGE@ -scan -f @PACKAGE@-@PACKAGE_VERSION@.fpr

# Use this to clean the fortify project.
.PHONY: fortifyclean
fortifyclean:
	sourceanalyzer -b @PACKAGE@ -clean

###########################################################################

# Build linux RPMs

srpm: dist
	rpmbuild -ts hdf5_handler-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb hdf5_handler-@PACKAGE_VERSION@.tar.gz

###########################################################################

# Build OS/X Packages.

PACKAGEMAKER=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker*

UNIV_ARCH=-arch ppc -arch i386
MAC_SYSROOT=Developer/SDKs/MacOSX10.4u.sdk

PKG_CONF_FLAGS="--enable-cf --enable-short-path"

clean-pkg:
	-rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@

pkg-build:
	./configure --prefix=/usr --disable-dependency-tracking $(PKG_CONF_FLAGS)
	make clean all
	DESTDIR=${PWD}/mac_osx make install

pkg-main:
	./OSX_Resources/update_mac_package_contents.pl README "configure.ac" "mac_osx/usr"
	${PACKAGEMAKER} -build -p @PACKAGE@-@PACKAGE_VERSION@.pkg \
		-f mac_osx/usr \
		-ds -r OSX_Resources \
		-i OSX_Resources/Info.plist \
		-d OSX_Resources/Description.plist
	-rm -rf mac_osx

pkg-dmg:
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@.dmg
	mkdir @PACKAGE@-@PACKAGE_VERSION@
	mv @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@
	cp README NEWS @PACKAGE@-@PACKAGE_VERSION@
	dropdmg -i --sanitize-for-servers --format zlib @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@

pkg: clean-pkg pkg-build pkg-main pkg-dmg

