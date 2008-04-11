# Automake file for hdf5_handler
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news dejagnu

ACLOCAL_AMFLAGS = -I conf

AM_YFLAGS = -d -v
AM_LFLAGS = -8

bin_PROGRAMS = dap_h5_handler

lib_LTLIBRARIES = libhdf5_handler.la

BUILT_SOURCES = $(GRAM_SRCS)

if OPENDAPSERVER

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libhdf5_module.la
dist_bin_SCRIPTS = bes-hdf5-data.sh

endif

GRAM_SRCS = hdfeos.tab.cc hdfeos.tab.h lex.hdfeos.cc \
	hdfeos_das.tab.cc hdfeos_das.tab.h lex.hdfeos_das.cc

HDF5SRC = HDF5Array.cc HDF5Byte.cc HDF5Float64.cc HDF5Grid.cc HDF5UInt32.cc \
	HDF5Int32.cc HDF5Sequence.cc HDF5Str.cc HDF5Structure.cc HDF5Url.cc \
	HDF5UInt16.cc HDF5Int16.cc HDF5Float32.cc HDF5TypeFactory.cc \
	HDF5GridEOS.cc HDF5ArrayEOS.cc 

HDF5HDR = HDF5Array.h HDF5Float64.h HDF5Int32.h HDF5Structure.h HDF5Url.h \
	HDF5Byte.h HDF5Grid.h HDF5Sequence.h HDF5UInt16.h HDF5Float32.h \
	HDF5Int16.h HDF5Str.h HDF5UInt32.h HDF5TypeFactory.h HDF5GridEOS.h HDF5ArrayEOS.h

SRCS = h5dds.cc h5das.cc H5Git.cc H5EOS.cc H5PathFinder.cc $(GRAM_SRCS)

HDRS = common.h h5dds.h h5das.h H5Git.h H5EOS.h H5PathFinder.h

SERVER_SRC = HDF5RequestHandler.cc HDF5Module.cc

SERVER_HDR = HDF5RequestHandler.h HDF5Module.h

libhdf5_handler_la_SOURCES = $(HDF5SRC) $(SRCS) $(HDF5HDR)  $(HDRS)

libhdf5_module_la_SOURCES = $(HDF5SRC) $(SRCS) $(HDF5HDR)  $(HDRS) $(SERVER_SRC) $(SERVER_HDR)
libhdf5_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libhdf5_module_la_LDFLAGS = -avoid-version -module 
libhdf5_module_la_LIBADD = $(BES_DAP_LIBS) -L${hdf5_path}/lib -lhdf5 -lz

dap_h5_handler_SOURCES = h5_handler.cc h5_handler.h
dap_h5_handler_LDADD = libhdf5_handler.la -L${hdf5_path}/lib -lhdf5 -lz

DEJATOOL = dap_h5_handler

RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir testsuite  # testsuite_internal 

EXTRA_DIST = hdfeos_das.y hdfeos.y hdfeos_das.lex  hdfeos.lex OSX_Resources \
	data COPYING hdf5_handler.spec bes-hdf5-data.sh.in

CLEANFILES = *~ *.sum *.log

if OPENDAPSERVER

CLEANFILES += bes-hdf5-data.sh

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

endif

###########################################################################

# Build the HDF-EOS scanner

lex.hdfeos.cc: hdfeos.lex hdfeos.tab.cc hdfeos.tab.hh
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<


lex.hdfeos_das.cc: hdfeos_das.lex hdfeos_das.tab.cc hdfeos_das.tab.h
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<

# Build the HDF-EOS parser

hdfeos.tab.cc hdfeos.tab.hh: hdfeos.y H5EOS.h
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p hdfeos -o hdfeos.tab.cc $<
	if test -e hdfeos.tab.cc.h; then mv hdfeos.tab.cc.h hdfeos.tab.hh; fi

hdfeos_das.tab.cc hdfeos_das.tab.h: hdfeos_das.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p hdfeos_das -o hdfeos_das.tab.cc $<
	if test -e hdfeos_das.tab.cc.h; then mv hdfeos_das.tab.cc.h hdfeos_das.tab.hh; fi

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

