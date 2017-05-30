
# Automake file for the gdal_handler
#
# 1/11/12 jhrg
#

AUTOMAKE_OPTIONS = foreign check-news 

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=gdal_handler
M_VER=1.0.8

if DAP_MODULES
AM_CPPFLAGS = $(GDAL_CFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(BES_CPPFLAGS) $(DAP_CFLAGS) $(GDAL_CFLAGS)
LIBADD = $(BES_DAP_LIBS) $(DAP_LIBS)
endif

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS = 
if COMPILER_IS_GCC
AM_CXXFLAGS += -Wall -W -Wcast-align
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -Wall -W -Wcast-align 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

if BUILD_HYRAX_DEBUG
AM_CXXFLAGS += $(CXXFLAGS_DEBUG)
endif

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libgdal_module.la

SUBDIRS = . tests

GDAL_SRCS = GDALModule.cc GDALRequestHandler.cc GDALArray.cc GDALGrid.cc \
GDALTypes.cc gdal_das.cc gdal_dds.cc

GDAL_HDRS = GDALModule.h GDALRequestHandler.h GDALTypes.h GDAL_DDS.h GDAL_DMR.h

libgdal_module_la_SOURCES = $(GDAL_SRCS) $(GDAL_HDRS)
libgdal_module_la_LDFLAGS = -avoid-version -module $(GDAL_LDFLAGS)
libgdal_module_la_LIBADD = $(LIBADD)

EXTRA_PROGRAMS = 

EXTRA_DIST = COPYING gdal.conf.in data OSX_Resources

if !DAP_MODULES
EXTRA_DIST += gdal_handler.spec
endif

CLEANFILES = *~ gdal.conf

# Sample data primaries for install
sample_datadir = $(datadir)/hyrax/data/gdal
sample_data_DATA = data/README \
    data/Atlantic.wind.grb.bz2 \
    data/Caribbean.wind.grb.bz2 \
    data/EastAtlantic.wind.grb.bz2 \
    data/CentralAtlantic.wind.grb.bz2 \
    data/cea.tif \
    data/cea.txt \
    data/DIGEST_Example_2.20120926112517.tif \
    data/SP27GTIF.TIF \
    data/SP27GTIF.lgo \
    data/DIGEST_Example_2.20120926112501.jp2 \
    data/goldenears_gray.jpg \
    data/goldenears.jpg

install-data-local: gdal.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/gdal.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/gdal.conf $(DESTDIR)$(sysconfdir)/bes/modules/gdal.conf.$$currdate; \
	$(INSTALL_DATA) gdal.conf $(DESTDIR)$(sysconfdir)/bes/modules/gdal.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/gdal.conf

gdal.conf: gdal.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > gdal.conf

.PHONY: docs
docs:
	doxygen $(srcdir)/doxy.conf
	(cd docs && tar -czf html.tar.gz html)

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libgdal_module_la_SOURCES)

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
# are there so that the values built into dap-config will match the mac
# osx install dirs and not the temp directory used to build the packages

# PACKAGEMAKER=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker*

PACKAGEMAKER=/Developer/usr/bin/packagemaker
PKG_CONF_FLAGS=

clean-pkg: 
	-rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg

pkg-build: clean-pkg 
	./configure --prefix=$(prefix) --with-icu-prefix=$(GDAL_PATH) --disable-dependency-tracking $(PKG_CONF_FLAGS)
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




