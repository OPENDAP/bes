
# Automake file for functions
#
# 01/28/2013 Hacked up by jhrg
#

# AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap
AM_YFLAGS = -d -v
AM_LFLAGS = -8

AUTOMAKE_OPTIONS = foreign check-news

SUBDIRS =  . unit-tests tests
DIST_SUBDIRS = unit-tests tests

if GDAL_FOUND
SUBDIRS +=  swath2grid 
DIST_SUBDIRS += swath2grid 
endif

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libfunctions_module.la

BUILT_SOURCES = gse.tab.cc gse.tab.hh lex.gse_.cc

SRCS = grid_utils.cc DapFunctions.cc GeoConstraint.cc GridGeoConstraint.cc \
gse.tab.cc lex.gse_.cc GSEClause.cc GeoGridFunction.cc GridFunction.cc \
LinearScaleFunction.cc VersionFunction.cc MakeArrayFunction.cc \
BindNameFunction.cc

HDRS = grid_utils.h DapFunctions.h GeoConstraint.h GridGeoConstraint.h \
gse.tab.hh gse_parser.h GSEClause.h GeoGridFunction.h \
GridFunction.h LinearScaleFunction.h VersionFunction.h MakeArrayFunction.h \
BindNameFunction.h

libfunctions_module_la_SOURCES = $(SRCS) $(HDRS)
libfunctions_module_la_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
libfunctions_module_la_LDFLAGS = -avoid-version -module 
libfunctions_module_la_LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

# Don't install the headers for this kind of module.
pkginclude_HEADERS = $(HDRS) 

# Build the grid selection sub_expression scanner and parser

lex.gse_.cc: gse.lex gse.tab.cc gse.tab.hh
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<

gse.tab.cc gse.tab.hh: gse.y
	echo "YACC: "$(YACC)
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p gse_ -o gse.tab.cc $<
	if test -e gse.tab.cc.h; then mv gse.tab.cc.h gse.tab.hh; fi

# ce_functions.o: ce_functions.cc ce_functions

EXTRA_PROGRAMS = 

EXTRA_DIST = functions.conf.in gse.y gse.lex

CLEANFILES = *~ functions.conf

install-data-local: functions.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/functions.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/functions.conf $(DESTDIR)$(sysconfdir)/bes/modules/functions.conf.$$currdate; \
	$(INSTALL_DATA) functions.conf $(DESTDIR)$(sysconfdir)/bes/modules/functions.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/functions.conf

functions.conf: functions.conf.in ../config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
                -e "s%[@]datarootdir[@]%${datarootdir}%" \
                -e "s%[@]datadir[@]%${datadir}%" \
                -e "s%[@]bindir[@]%${bindir}%" \
                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > functions.conf

distclean-local:
	-rm gse.tab.cc gse.tab.hh lex.gse_.cc

# Not nearly as clean as it could be, but this removes .svn directories
# in subdirs.
dist-hook:
	rm -rf `find $(distdir) -name .svn`

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

