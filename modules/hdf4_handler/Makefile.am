# Automake file for hdf4-handler
#
# 9/19/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=hdf4_handler
M_VER=1.16.9

AM_CXXFLAGS = -I$(top_srcdir)/hdfclass
AM_YFLAGS = -d -v
AM_LFLAGS = -8

if DAP_BUILTIN_MODULES
AM_CPPFLAGS = $(HDF4_CFLAGS) $(HDFEOS2_CPPFLAGS) -I$(top_srcdir)/modules/hdf4_handler/hdfclass \
-I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS) $(HDF4_CPPFLAGS)
LIBADD = hdfclass/libhdfclass.la $(HDFEOS2_LDFLAGS) $(HDFEOS2_LIBS) $(HDF4_LDFLAG) $(HDF4_LIBS) \
$(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
# Maybe we don't need this anymore. jhrg 11/30/14
# CC = $(HDF4_CC)
else
AM_CPPFLAGS = $(HDF4_CFLAGS) $(HDFEOS2_CPPFLAGS) -I$(top_srcdir)/hdfclass $(BES_CPPFLAGS)
LIBADD = hdfclass/libhdfclass.la $(HDFEOS2_LDFLAGS) $(HDFEOS2_LIBS) $(HDF4_LDFLAGS) $(HDF4_LIBS) \
$(BES_DAP_LIBS)
endif

AM_CPPFLAGS += -DUSE_DAP4 -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

# jhrg 5/12/21 Note that this is defined above. AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = hdfclass . bes-testsuite
# DIST_SUBDIRS = hdfclass bes-testsuite

# noinst_PROGRAMS = eosdas_test

# WA_HDF4_LIBS="-whole-archive $(HDF4_LIBS) -no-whole-archive"

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libhdf4_module.la

libhdf4_module_la_SOURCES = $(HDFTYPE_SRC) $(HANDLER_SRC)  $(SERVER_SRC) $(HDFTYPE_HDR) $(SERVER_HDR)
# libhdf4_module_la_CPPFLAGS = $(CPPFLAGS) $(BES_CPPFLAGS)
libhdf4_module_la_LDFLAGS = -avoid-version -module
libhdf4_module_la_LIBADD = $(LIBADD) 

# Listing all of the sources list this and above for the module, causes automake
# 1.12 to have a fit because the same code is compiled twice, once for libtool and
# once for ld. I'm not sure if this is used at all anymore. I tried building using
# the module 'library' but that won't link on OSX (It might on linux). jhrg 11/30/14.  
# eosdas_test_SOURCES = eosdas-test.cc $(HDFTYPE_SRC) $(HANDLER_SRC) $(HDFTYPE_HDR)
# eosdas_test_LDADD = $(builddir)/hdfclass/libhdfclass.la $(HDFEOS2_LIBS) $(HDF4_LIBS)
# eosdas_test_LDFLAGS=$(LDFLAGS) $(HDF4_LDFLAGS)
# eosdas_test_CPPFLAGS = $(CPPFLAGS) $(BES_CPPFLAGS)

HDFTYPE_SRC = 	$(GRAM_SRC) \
        HDFDMRArray_SPLL.cc HDFDMRArray_EOS2LL.cc HDFDMRArray_SDS.cc HDFDMRArray_VD.cc HDFArray.cc HDFGrid.cc HDFSequence.cc HDFUInt32.cc \
	HDFInt8.cc HDFByte.cc HDFInt16.cc HDFStr.cc HDFUrl.cc HDFFloat32.cc \
	HDFInt32.cc HDFStructure.cc HDFFloat64.cc HDFUInt16.cc \
	HDFTypeFactory.cc 

HDFCF_SRC = HDFCFUtil.cc BESH4MCache.cc HDFCFStr.cc HDFEOS2.cc HDFEOS2CFStr.cc HDFSP.cc \
                HDFSPArrayGeoField.cc HDFSPArrayAddCVField.cc\
		HDFSPArrayMissField.cc HDFSPArray_RealField.cc HDFSPArray_VDField.cc\
	          HDFCFStrField.cc HDFEOS2CFStrField.cc HDFEOS2Array_RealField.cc \
	     HDFEOS2ArrayGridGeoField.cc HDFEOS2ArraySwathGeoField.cc HDFEOS2ArrayMissField.cc\
              HDFEOS2ArraySwathDimMapField.cc HDFEOS2ArraySwathGeoDimMapExtraField.cc \
	      HDFEOS2ArraySwathGeoMultiDimMapField.cc HDFEOS2GeoCFProj.cc HDFEOS2GeoCF1D.cc \
		      HE2CF.cc misr_init.cc misrinv.cc

HDFTYPE_SRC += $(HDFCF_SRC)

HDFTYPE_HDR = HDFDMRArray_SPLL.h HDFDMRArray_EOS2LL.h HDFDMRArray_SDS.h HDFDMRArray_VD.h HDFArray.h HDFFloat32.h HDFInt16.h HDFSequence.h HDFUInt16.h \
	HDFByte.h HDFInt8.h HDFFloat64.h HDFInt32.h HDFStr.h HDFUInt32.h \
	HDFGrid.h HDFStructure.h HDFUrl.h HDFTypeFactory.h 


HDFCF_HDR = HDFEOS2.h HDFSP.h HDFCFUtil.h \
	HDFSPEnumType.h HDFCFStr.h \
	HDFSPArrayGeoField.h HDFSPArrayAddCVField.h HDFSPArrayMissField.h HDFSPArray_RealField.h HDFSPArray_VDField.h \
	HDFEOS2EnumType.h HDFEOS2HandleType.h BESH4MCache.h \
	HDFEOS2Array_RealField.h \
	HDFEOS2ArrayGridGeoField.h HDFEOS2ArraySwathGeoField.h HDFEOS2ArrayMissField.h\
	HDFEOS2ArraySwathDimMapField.h\
	HDFEOS2ArraySwathGeoDimMapExtraField.h \
	HDFEOS2ArraySwathGeoMultiDimMapField.h \
        HDFCFStrField.h \
	HDFEOS2CFStr.h \
	HDFEOS2CFStrField.h \
        HDFEOS2GeoCFProj.h \
        HDFEOS2GeoCF1D.h \
	HE2CF.h misrproj.h errormacros.h

HDFTYPE_HDR += $(HDFCF_HDR)

GRAM_SRC = hdfeos.tab.cc hdfeos.tab.hh lex.hdfeos.cc 


HANDLER_SRC = dhdferr.cc hdfdesc.cc hc2dap.cc hdfutil.cc \
	dhdferr.h dodsutil.h hdfutil.h ReadTagRef.h \
	hdf-maps.h config_hdf.h 


SERVER_SRC = HDF4RequestHandler.cc HDF4Module.cc

SERVER_HDR = HDF4RequestHandler.h HDF4Module.h HDF4_DDS.h HDF4_DMR.h

EXTRA_DIST = data data.nasa hdf-testfiles \
		hdfeos.yy hdfeos.lex \
		COPYRIGHT_URI  \
		OSX_Resources h4.conf.in test.nasa.with_hdfeos2.sh USAGE

if !DAP_BUILTIN_MODULES
EXTRA_DIST += hdf4_handler.spec hdf4_handler.spec.cf
endif

CLEANFILES = *~ *.log *.sum $(GRAM_SRC) h4.conf

sample_datadir = $(datadir)/hyrax/data/hdf4
sample_data_DATA = data/S2000415.HDF.gz data/foo2.hdf.gz data/grid_1_2d.hdf

moduledir = $(sysconfdir)/bes/modules
module_DATA = h4.conf

h4.conf: h4.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > h4.conf

.PHONY: grammarfiles
grammarfiles: $(GRAM_SRC)
	for f in $(GRAM_SRC); do cp $$f grammarfiles; done

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libhdf4_module_la_SOURCES)

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
	rpmbuild -ts --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb --clean $(RPM_OPTIONS) @PACKAGE@-@PACKAGE_VERSION@.tar.gz

###########################################################################
# Build the HDF-EOS parser

hdfeos.tab.cc hdfeos.tab.hh: hdfeos.yy
	$(YACC) $(YFLAGS) $<

# Build the HDF-EOS scanner

lex.hdfeos.cc lex.hdfeos.hh: hdfeos.lex hdfeos.tab.cc hdfeos.tab.hh
	$(LEX) $(LFLAGS) $<

###########################################################################


# Build OS/X Packages. The strange operations with configure and make
# are there so that the values built into bes-config will match the mac
# osx install dirs and not the temp directory used to build the packages


.PHONY: clean-pkg pkg-build pkg-main pkg-dmg pkg showEnv
PACKAGEMAKER=/Developer/usr/bin/packagemaker
PKG_CONF_FLAGS=

clean-pkg:
	-rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@

pkg-build: clean-pkg
	./configure --prefix=$(prefix) --with-hdf4=$(HDF4_PATH) --with-hdfeos2=$(HDFEOS2_DIR) --disable-dependency-tracking $(PKG_CONF_FLAGS)
	make clean all
	DESTDIR=`pwd`/mac_osx make install

pkg-main: pkg-build
	./OSX_Resources/update_mac_package_contents.pl README
	cat ./OSX_Resources/InstallationCheck.proto | sed -e "s^_PREFIX_^$(prefix)^g" > foo
	mv foo ./OSX_Resources/InstallationCheck
	chmod 755 ./OSX_Resources/InstallationCheck
	cat ./OSX_Resources/Info.plist.proto | \
	   sed -e "s^_PREFIX_^$(prefix)^g" \
	       -e "s^_FULL_VERSION_^@PACKAGE_VERSION@^g" \
	       -e "s^_MAJOR_VERSION_^@PACKAGE_MAJOR_VERSION@^g" \
	       -e "s^_MINOR_VERSION_^@PACKAGE_MINOR_VERSION@^g" \
	       > foo
	mv foo ./OSX_Resources/Info.plist
	${PACKAGEMAKER} --root mac_osx --id org.opendap.@PACKAGE@ \
		--title "@PACKAGE@ @PACKAGE_VERSION@" --version @PACKAGE_VERSION@ \
		--out @PACKAGE@-@PACKAGE_VERSION@.pkg --resources OSX_Resources

pkg-dmg: pkg-main
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@.dmg
	mkdir @PACKAGE@-@PACKAGE_VERSION@
	cp -r @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@
	cp README NEWS @PACKAGE@-@PACKAGE_VERSION@
	dropdmg -i --sanitize-for-servers --format zlib @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@

pkg: pkg-main

showEnv: 
	@echo "PACKAGE:         " @PACKAGE@
	@echo "PACKAGE-VERSION: " @PACKAGE_VERSION@
	@echo "HDF4_PATH:       " $(HDF4_PATH)
	@echo "HDFEOS2_DIR:     " $(HDFEOS2_DIR)




