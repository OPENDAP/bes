
# Automake file for usage
#
# 5/17/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=usage
M_VER=4.2.6

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(BES_CPPFLAGS) $(DAP_CFLAGS)
LIBADD = $(BES_DAP_LIBS)
endif

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libusage_module.la
besdir=$(datadir)/bes

dist_bes_DATA = usage_help.html usage_help.txt

USAGE_SOURCES = usage.cc usage.h 

BES_SOURCES = BESUsageModule.h BESUsageNames.h BESUsageResponseHandler.h \
	  BESUsageTransmit.h BESUsageModule.cc BESUsageResponseHandler.cc \
	  BESUsageTransmit.cc BESUsage.h BESUsageRequestHandler.cc \
	  BESUsageRequestHandler.h

libusage_module_la_SOURCES = $(USAGE_SOURCES) $(BES_SOURCES)
# libusage_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libusage_module_la_LDFLAGS = -avoid-version -module 
libusage_module_la_LIBADD = $(LIBADD) # $(BES_DAP_LIBS)

EXTRA_DIST = COPYING COPYRIGHT_URI README \
	usage.conf.in OSX_Resources

if !DAP_MODULES
EXTRA_DIST += usage.spec
endif

DISTCLEANFILES = usage.conf

install-data-local: usage.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/usage.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/usage.conf $(DESTDIR)$(sysconfdir)/bes/modules/usage.conf.$$currdate; \
	$(INSTALL_DATA) usage.conf $(DESTDIR)$(sysconfdir)/bes/modules/usage.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/usage.conf

usage.conf: usage.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > usage.conf

C4_DIR=./cccc
C4_USAGE=--html_outfile=cccc-usage.html --xml_outfile=cccc-usage.xml --db_outfile=cccc-usage.db

.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(C4_USAGE) $(libusage_module_la_SOURCES)

###########################################################################

# Fortify targets.

.PHONY: fortify
fortify:
	sourceanalyzer -b @PACKAGE@ $(MAKE) libusage_module.la
	sourceanalyzer -b @PACKAGE@ -scan -f @PACKAGE@-@PACKAGE_VERSION@.fpr
	(cd asciival && $(MAKE) fortify)
	(cd www-interface && $(MAKE) fortify)

# Use this to clean the fortify project.
.PHONY: fortifyclean
fortifyclean:
	sourceanalyzer -b @PACKAGE@ -clean
	(cd asciival && $(MAKE) fortifyclean)
	(cd www-interface && $(MAKE) fortifyclean)

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
	./configure --prefix=$(prefix) --disable-dependency-tracking $(PKG_CONF_FLAGS)
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

