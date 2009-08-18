# Automake file for hdf4-handler
#
# 9/19/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf
AM_CXXFLAGS = -I$(top_srcdir)/hdfclass
AM_YFLAGS = -d -v
AM_LFLAGS = -8

SUBDIRS = hdfclass

bin_PROGRAMS = dap_hdf4_handler
noinst_PROGRAMS = eosdas_test

WA_HDF4_LIBS="-whole-archive $(HDF4_LIBS) -no-whole-archive"

if OPENDAPSERVER

SUBDIRS += bes-testsuite

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libhdf4_module.la

libhdf4_module_la_SOURCES = $(HDFTYPE_SRC) $(HANDLER_SRC)  $(SERVER_SRC) \
                            $(HDFTYPE_HDR) $(SERVER_HDR)
libhdf4_module_la_CPPFLAGS = $(CPPFLAGS) $(BES_CPPFLAGS)
libhdf4_module_la_LDFLAGS = -avoid-version -module
libhdf4_module_la_LIBADD = hdfclass/libhdfclass.la $(HDFEOS2_LIBS) $(HDF4_LIBS) $(BES_DAP_LIBS)
dist_bin_SCRIPTS = bes-hdf4-data.sh

endif

dap_hdf4_handler_SOURCES = hdf_handler.cc $(HDFTYPE_SRC) $(HANDLER_SRC) \
                           $(HDFTYPE_HDR)
dap_hdf4_handler_LDFLAGS=$(LDFLAGS) $(HDF4_LDFLAGS)
dap_hdf4_handler_LDADD = hdfclass/libhdfclass.la $(HDFEOS2_LIBS) $(HDF4_LIBS)
dap_hdf4_handler_CPPFLAGS = $(CPPFLAGS) $(BES_CPPFLAGS)

eosdas_test_SOURCES = eosdas-test.cc $(HDFTYPE_SRC) $(HANDLER_SRC) \
		      $(HDFTYPE_HDR)
eosdas_test_LDADD = hdfclass/libhdfclass.la $(HDFEOS2_LIBS) $(HDF4_LIBS)
eosdas_test_LDFLAGS=$(LDFLAGS) $(HDF4_LDFLAGS)
eosdas_test_CPPFLAGS = $(CPPFLAGS) $(BES_CPPFLAGS)

HDFTYPE_SRC = 	$(GRAM_SRC) \
        HDFArray.cc HDFGrid.cc HDFSequence.cc HDFUInt32.cc \
	HDFByte.cc HDFInt16.cc HDFStr.cc HDFUrl.cc HDFFloat32.cc \
	HDFInt32.cc HDFStructure.cc HDFFloat64.cc HDFUInt16.cc \
	HDFTypeFactory.cc 

if CF
CF_SRC = HDFEOS.cc HDFEOSGrid.cc HDFEOSArray.cc HDFCFSwath.cc
HDFTYPE_SRC += $(CF_SRC)
endif

if HDFEOS2
HDFEOS2_SRC = HDFEOS2.cc HDFEOS2Array.cc HDFEOS2Array2D.cc 
HDFTYPE_SRC += $(HDFEOS2_SRC)
endif

HDFTYPE_HDR = HDFArray.h HDFFloat32.h HDFInt16.h HDFSequence.h HDFUInt16.h \
	HDFByte.h HDFFloat64.h HDFInt32.h HDFStr.h HDFUInt32.h \
	HDFGrid.h HDFStructure.h HDFUrl.h HDFTypeFactory.h 

if CF
CF_HDR = HDFEOSGrid.h HDFEOSArray.h HDFCFSwath.h
HDFTYPE_HDR += $(CF_HDR)
endif

if HDFEOS2
HDFEOS2_HDR = HDFEOS2.h
HDFTYPE_HDR += $(HDFEOS2_HDR)
endif

GRAM_SRC = hdfeos.tab.cc hdfeos.tab.h lex.hdfeos.cc 

if CF
CF_GRAM_SRC = hdfeos2.tab.cc hdfeos2.tab.hh lex.hdfeos2.cc
endif

if CF
# Parser for EOS Grid generation is added. <hyokyung 2008.11.11. 10:21:13>
GRAM_SRC += $(CF_GRAM_SRC)
endif

HANDLER_SRC = dhdferr.cc hdfdesc.cc hc2dap.cc hdfutil.cc \
	dhdferr.h dodsutil.h hdfutil.h ReadTagRef.h \
	hdf-dods.h hdf-maps.h config_hdf.h 


SERVER_SRC = HDF4RequestHandler.cc HDF4Module.cc

SERVER_HDR = HDF4RequestHandler.h HDF4Module.h

EXTRA_DIST = data hdf-testsuite hdf-testfiles hdfeos.y hdfeos.lex \
	COPYRIGHT_URI hdf4_handler.spec OSX_Resources

# DEJATOOL = dap_hdf4_handler
# RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir hdf-testsuite

CLEANFILES = *~ *.log *.sum $(GRAM_SRC)

if OPENDAPSERVER

EXTRA_DIST += bes-hdf4-data.sh.in
CLEANFILES += bes-hdf4-data.sh

sample_datadir = $(datadir)/hyrax/data/hdf4
sample_data_DATA = data/1990104h09da-gdm.hdf data/S2000415.HDF

do_subst = sed -e 's,[@]sysconfdir[@],$(sysconfdir),g' \
		   	   -e 's,[@]libdir[@],$(libdir),g' \
		   	   -e 's,[@]datadir[@],$(datadir),g'

bes-hdf4-data.sh: bes-hdf4-data.sh.in Makefile
	$(do_subst) < $(srcdir)/bes-hdf4-data.sh.in > bes-hdf4-data.sh
	chmod +x bes-hdf4-data.sh

# This makes sure that the handler modifies the bes which confiure found
# even when distcheck is run.
bes-conf: bes-hdf4-data.sh
	(bes_prefix=`bes-config --prefix` ; \
	if test "$$bes_prefix" = "/usr" ; \
	then \
	    ./bes-hdf4-data.sh /etc/bes/bes.conf /usr/lib/bes ;\
	else \
	    ./bes-hdf4-data.sh $$bes_prefix/etc/bes/bes.conf $$bes_prefix/lib/bes ; \
	fi)

endif

# Copy the generated grammar files to the 'grammarfiles' directory.
.PHONY: grammarfiles
grammarfiles: $(GRAM_SRC)
	for f in $(GRAM_SRC); do cp $$f grammarfiles; done

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
	rpmbuild -ts hdf4_handler-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb hdf4_handler-@PACKAGE_VERSION@.tar.gz

###########################################################################

# Build the HDF-EOS parser

hdfeos.tab.cc hdfeos.tab.h: hdfeos.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p hdfeos  -o hdfeos.tab.cc $<
	if test -e hdfeos.tab.cc.h; then mv hdfeos.tab.cc.h hdfeos.tab.hh; fi

hdfeos2.tab.cc hdfeos2.tab.hh: hdfeos2.y HDFEOS.h
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p hdfeos2 -o hdfeos2.tab.cc $<
	if test -e hdfeos2.tab.cc.h; then mv hdfeos2.tab.cc.h hdfeos2.tab.hh; fi

# Build the HDF-EOS scanner

lex.hdfeos.cc: hdfeos.lex hdfeos.tab.cc hdfeos.tab.hh
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -Phdfeos $<
	mv lex.hdfeos.c lex.hdfeos.cc

lex.hdfeos2.cc: hdfeos2.lex hdfeos2.tab.cc hdfeos2.tab.hh
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<

###########################################################################

# Build OS/X Packages.

PACKAGEMAKER=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker*

UNIV_ARCH=-arch ppc -arch i386
MAC_SYSROOT=Developer/SDKs/MacOSX10.4u.sdk

PKG_CONF_FLAGS=

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

