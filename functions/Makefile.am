
# Automake file for functions
#
# 01/28/2013 Hacked up by jhrg
#

AM_CXXFLAGS = 

AUTOMAKE_OPTIONS = foreign check-news

if DAP_MODULES
AM_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(BES_CPPFLAGS)
LIBADD = $(BES_DAP_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
# ./configure --disable-shared CXXFLAGS=...
# or ./configure --enable-developer --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

if BES_DEVELOPER
AM_CXXFLAGS += $(CXXFLAGS_DEBUG)
endif

# Set the module version here, in the spec file and in configure.ac
M_NAME=functions
M_VER=1.1.0

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . unit-tests tests

# We use DIST_SUBDIRS because we want swath2grid to be part of the dist.
# However, since we use it we must list all of the subdirs we want; without
# it, automake will automatically distribute all the stuff in SUBDIRS.
DIST_SUBDIRS = unit-tests tests swath2grid

if BUILD_GDAL
# Even if GDAL is found when building the BES, hold off on this code
# until it has tests and supports DAP4
# SUBDIRS +=  swath2grid 
endif

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libfunctions_module.la

BUILT_SOURCES = gse.tab.cc gse.tab.hh lex.gse.cc

SRCS = grid_utils.cc DapFunctions.cc GeoConstraint.cc GridGeoConstraint.cc \
gse.tab.cc lex.gse.cc GSEClause.cc GeoGridFunction.cc GridFunction.cc \
LinearScaleFunction.cc VersionFunction.cc MakeArrayFunction.cc \
MakeMaskFunction.cc DilateArrayFunction.cc \
BindNameFunction.cc BindShapeFunction.cc TabularFunction.cc \
TabularSequence.cc BBoxFunction.cc RoiFunction.cc roi_util.cc \
BBoxUnionFunction.cc Odometer.cc MaskArrayFunction.cc functions_util.cc \
DapFunctionsRequestHandler.cc

HDRS = grid_utils.h DapFunctions.h GeoConstraint.h GridGeoConstraint.h \
gse.tab.hh gse_parser.h GSEClause.h GeoGridFunction.h \
GridFunction.h LinearScaleFunction.h VersionFunction.h MakeArrayFunction.h \
MakeMaskFunction.h DilateArrayFunction.h \
BindNameFunction.h BindShapeFunction.h TabularFunction.h \
TabularSequence.h BBoxFunction.h RoiFunction.h roi_util.h \
BBoxUnionFunction.h Odometer.h MaskArrayFunction.h functions_util.h \
DapFunctionsRequestHandler.h

libfunctions_module_la_SOURCES = $(SRCS) $(HDRS)
# libfunctions_module_la_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
libfunctions_module_la_LDFLAGS = -avoid-version -module 
# libfunctions_module_la_LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
libfunctions_module_la_LIBADD = $(LIBADD)

# Don't install the headers for this kind of module.
pkginclude_HEADERS = $(HDRS) 

# Build the grid selection sub_expression scanner and parser

lex.gse.cc: gse.lex gse.tab.cc gse.tab.hh
	$(LEX) $(LFLAGS) $<

# $(AM_LFLAGS) 

gse.tab.cc gse.tab.hh: gse.yy
	$(YACC) $(YFLAGS) $<

# gse.tab.hh
#	if test -e gse.tab.cc.h; then mv gse.tab.cc.h gse.tab.hh; fi
# $(AM_YFLAGS) -p gse_ -o gse.tab.cc 

# ce_functions.o: ce_functions.cc ce_functions

EXTRA_PROGRAMS = 

EXTRA_DIST = functions.conf.in gse.yy gse.lex

CLEANFILES = *~ functions.conf

install-data-local: functions.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/functions.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/functions.conf $(DESTDIR)$(sysconfdir)/bes/modules/functions.conf.$$currdate; \
	$(INSTALL_DATA) functions.conf $(DESTDIR)$(sysconfdir)/bes/modules/functions.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/functions.conf

functions.conf: functions.conf.in $(top_builddir)/config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
                -e "s%[@]datarootdir[@]%${datarootdir}%" \
                -e "s%[@]datadir[@]%${datadir}%" \
                -e "s%[@]bindir[@]%${bindir}%" \
                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > functions.conf

distclean-local:
	-rm gse.tab.cc gse.tab.hh lex.gse.cc

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libfunctions_module_la_SOURCES)

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
#
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
	./configure --prefix=$(prefix) --disable-dependency-tracking $(PKG_CONF_FLAGS)
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
	cp -R @PACKAGE@-@PACKAGE_VERSION@.pkg @PACKAGE@-@PACKAGE_VERSION@
	cp README NEWS @PACKAGE@-@PACKAGE_VERSION@
	dropdmg -i --sanitize-for-servers --format zlib @PACKAGE@-@PACKAGE_VERSION@
	-rm -rf @PACKAGE@-@PACKAGE_VERSION@

pkg: pkg-main

