# Automake file for hdf5_handler
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=hdf5_handler
M_VER=1.16.8

GCTP_LIBS=gctp/src/libGctp.a
GCTP_CPPFLAGS=-I gctp/include

AM_YFLAGS = -d -v
AM_LFLAGS = -8

if DAP_BUILTIN_MODULES
AM_CPPFLAGS = $(GCTP_CPPFLAGS) $(H5_CPPFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(H5_LDFLAGS) $(H5_LIBS) $(GCTP_LIBS)
else
AM_CPPFLAGS = $(GCTP_CPPFLAGS) $(H5_CPPFLAGS) $(BES_CPPFLAGS)
LIBADD = $(BES_DAP_LIBS) $(H5_LDFLAGS) $(H5_LIBS) $(GCTP_LIBS)
# -L${hdf5_path}/lib -lhdf5 -lz
endif

#AM_CPPFLAGS += -DUSE_DAP4 -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"
AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

BUILT_SOURCES = $(GRAM_SRCS)

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = gctp . bes-testsuite 
# DIST_SUBDIRS = . bes-testsuite

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libhdf5_module.la

GRAM_SRCS = he5das.tab.cc he5das.tab.hh lex.he5das.cc he5dds.tab.cc he5dds.tab.hh lex.he5dds.cc

HDF5DTYPE_SRCS = HDF5CFProj.cc HDF5CFProj1D.cc HDF5MissLLArray.cc HDF5Array.cc HDF5Byte.cc HDF5Int8.cc HDF5Float64.cc  HDF5UInt32.cc \
	HDF5Int32.cc HDF5Sequence.cc HDF5Str.cc HDF5Structure.cc HDF5Url.cc \
	HDF5UInt16.cc HDF5Int16.cc HDF5Float32.cc HDF5Int64.cc HDF5UInt64.cc

HDF5DTYPE_HDRS = HDF5CFProj.h HDF5CFProj1D.h HDF5MissLLArray.h HDF5Array.h HDF5Float64.h HDF5Int32.h HDF5Structure.h HDF5Url.h \
	HDF5Byte.h HDF5Int8.h HDF5Sequence.h HDF5UInt16.h HDF5Float32.h \
	HDF5Int16.h HDF5Str.h HDF5UInt32.h HDF5Int64.h HDF5UInt64.h 

HDF5_SRCS = h5dds.cc h5das.cc h5dmr.cc h5common.cc h5get.cc HDF5PathFinder.cc 
HDF5_HDRS = hdf5_handler.h h5dds.h h5das.h h5dmr.h h5common.h h5get.h  HDF5PathFinder.h config_hdf5.h h5apicompatible.h

HDF5CFDTYPE_SRCS = HDF5GMSPCFArray.cc HDF5GMCFFillIndexArray.cc HDF5GMCFMissLLArray.cc HDF5GMCFMissNonLLCVArray.cc HDF5GMCFSpecialCVArray.cc \
                   HDFEOS5CFMissLLArray.cc HDFEOS5CFMissNonLLCVArray.cc  HDFEOS5CFSpecialCVArray.cc \
                   HDF5CFArray.cc HDF5CFGeoCF1D.cc HDF5BaseArray.cc \
                   HDF5CFGeoCFProj.cc HDF5CFByte.cc HDF5CFInt8.cc HDF5CFUInt16.cc HDF5CFInt16.cc HDF5CFInt32.cc HDF5CFUInt32.cc \
	           HDF5CFFloat32.cc HDF5CFFloat64.cc HDF5CFInt64.cc HDF5CFUInt64.cc HDF5CFStr.cc   

HDF5CFDTYPE_HDRS =  HDF5GMSPCFArray.h HDF5GMCFFillIndexArray.h HDF5GMCFMissLLArray.h HDF5GMCFMissNonLLCVArray.h HDF5GMCFSpecialCVArray.h \
	            HDFEOS5CFMissLLArray.h HDFEOS5CFMissNonLLCVArray.h  HDFEOS5CFSpecialCVArray.h HDF5CFArray.h HDF5CFGeoCF1D.h HDF5BaseArray.h \
                    HDF5CFGeoCFProj.h HDF5CFByte.h HDF5CFInt8.h HDF5CFUInt16.h HDF5CFInt16.h HDF5CFInt32.h HDF5CFUInt32.h \
	            HDF5CFFloat32.h HDF5CFFloat64.h HDF5CFInt64.h HDF5CFUInt64.h HDF5CFStr.h 

HDF5CF_SRCS = h5cfdap.cc HDF5CFModule.cc heos5cfdap.cc h5gmcfdap.cc h5commoncfdap.cc HDFEOS5CF.cc HDF5GMCF.cc HDF5GCFProduct.cc HDF5CF.cc h5cfdaputil.cc HDF5CFUtil.cc HDF5DiskCache.cc\
	he5das.tab.cc  lex.he5das.cc he5dds.tab.cc  lex.he5dds.cc \
	HE5Parser.cc HE5Checker.cc

HDF5CF_HDRS = h5cfdap.h HDF5CFModule.h  heos5cfdap.h h5gmcfdap.h h5commoncfdap.h HDF5GCFProduct.h HDF5CF.h h5cfdaputil.h HDF5CFUtil.h HDF5DiskCache.h HE5Parser.h HE5GridPara.h\
 HE5Dim.h HE5Var.h HE5Grid.h HE5Swath.h HE5Za.h HE5Checker.h he5das.tab.hh  he5dds.tab.hh 

SERVER_SRC = HDF5RequestHandler.cc HDF5Module.cc HDF5_DataMemCache.cc

SERVER_HDR = HDF5RequestHandler.h HDF5Module.h HDF5_DDS.h HDF5_DMR.h HDF5_DataMemCache.h

libhdf5_module_la_SOURCES = $(HDF5DTYPE_SRCS) $(HDF5_SRCS) $(HDF5CFDTYPE_SRCS) $(HDF5CF_SRCS) $(SERVER_SRC) $(HDF5CFDTYPE_HDRS) $(HDF5CF_HDRS) $(HDF5DTYPE_HDRS) $(HDF5_HDRS) $(SERVER_HDR)
# libhdf5_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libhdf5_module_la_LDFLAGS = -avoid-version -module 
libhdf5_module_la_LIBADD = $(LIBADD)

EXTRA_DIST = he5das.yy he5dds.yy he5das.lex he5dds.lex OSX_Resources \
	data data.nasa COPYING h5.conf.in test.nasa.sh

if !DAP_BUILTIN_MODULES
EXTRA_DIST += hdf5_handler.spec
endif

CLEANFILES = *~ *.sum *.log *.output he5das.tab.cc he5das.tab.hh \
	lex.he5das.cc lex.he5dds.cc he5dds.tab.cc he5dds.tab.hh \
	h5.conf

sample_datadir = $(datadir)/hyrax/data/hdf5
sample_data_DATA = data/grid_1_2d.h5

moduledir = $(sysconfdir)/bes/modules
module_DATA = h5.conf

h5.conf: h5.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > h5.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libhdf5_module_la_SOURCES)

###########################################################################

# Build the HDF-EOS DAS scanner
lex.he5das.cc: he5das.lex he5das.tab.cc he5das.tab.hh
	$(LEX) $(LFLAGS) $<

# Build the HDF-EOS DAS parser
he5das.tab.cc he5das.tab.hh: he5das.yy h5cfdap.cc
	$(YACC) $(YFLAGS) $<

# Build the HDF-EOS DDS scanner
lex.he5dds.cc: he5dds.lex
	$(LEX) $(LFLAGS) $<

# Build the HDF-EOS DDS parser
he5dds.tab.cc he5dds.tab.hh: he5dds.yy he5dds.lex
	$(YACC) $(YFLAGS) $<

# Build the HDF-EOS DAS scanner
#lex.he5das.cc: he5das.lex he5das.tab.cc he5das.tab.hh
#	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<

# Build the HDF-EOS DAS parser
#he5das.tab.cc he5das.tab.hh: he5das.y h5cfdap.cc
#	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p he5das -o he5das.tab.cc $<
#	if test -e he5das.tab.cc.h; then mv he5das.tab.cc.h he5das.tab.hh; fi

# Build the HDF-EOS DDS scanner
#lex.he5dds.cc: he5dds.lex #he5dds.tab.cc he5dds.tab.hh 
#	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<

# Build the HDF-EOS DDS parser
#he5dds.tab.cc he5dds.tab.hh: he5dds.y he5dds.lex
#	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p he5dds -o he5dds.tab.cc $<
#	if test -e he5dds.tab.cc.h; then mv he5dds.tab.cc.h he5dds.tab.hh; fi




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

# Build OS/X Packages. The strange operations with configure and make
# are there so that the values built into bes-config will match the mac
# osx install dirs and not the temp directory used to build the packages

PACKAGEMAKER=/Developer/usr/bin/packagemaker
PKG_CONF_FLAGS=

clean-pkg:
	-rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@

pkg-build: clean-pkg
	echo "HDF5_PATH: " $(H5_PATH)
	./configure --prefix=$(prefix) --with-hdf5=$(H5_PATH) --disable-dependency-tracking $(PKG_CONF_FLAGS)
	make clean all
	DESTDIR=`pwd`/mac_osx make install

pkg-main: pkg-build
	./OSX_Resources/update_mac_package_contents.pl README
	cat ./OSX_Resources/InstallationCheck.proto | sed -e "s^_PREFIX_^$(prefix)^g" > foo
	mv foo ./OSX_Resources/InstallationCheck
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

