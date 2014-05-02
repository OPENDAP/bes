
# Automake file for dap-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

AM_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch
ACLOCAL_AMFLAGS = -I conf
AM_CXXFLAGS = 

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

SUBDIRS = . tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdapreader_module.la

BES_SRCS = DapModule.cc DapRequestHandler.cc

BES_HDRS = DapModule.h DapRequestHandler.h 

libdapreader_module_la_SOURCES = $(BES_SRCS) $(BES_HDRS)
libdapreader_module_la_CPPFLAGS = $(DAP_CFLAGS) $(AM_CPPFLAGS)
libdapreader_module_la_LDFLAGS = -avoid-version -module 
libdapreader_module_la_LIBADD = $(DAP_CLIENT_LIBS) -ltest-types

EXTRA_PROGRAMS = 

EXTRA_DIST = data dapreader.conf.in

CLEANFILES = *~ dapreader.conf

sample_datadir = $(datadir)/hyrax/data/dapreader
sample_data_DATA = data/fnoc1.das data/fnoc1.dds data/fnoc1.data

dap4_datadir = $(datadir)/hyrax/data/dapreader/dap4
dap4_data_DATA = data/dap4/dap4.html data/dap4/test_simple_1.dmr 

D4_xml_datadir = $(datadir)/hyrax/data/dapreader/dap4/D4-xml
D4_xml_data_DATA = data/dap4/D4-xml/*.xml

dmr_testsuite_datadir = $(datadir)/hyrax/data/dapreader/dap4/dmr-testsuite
dmr_testsuite_data_DATA = data/dap4/dmr-testsuite/*.xml	 

install-data-local: dapreader.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/dapreader.conf \
	    && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/dapreader.conf $(DESTDIR)$(sysconfdir)/bes/modules/dapreader.conf.$$currdate; \
	$(INSTALL_DATA) dapreader.conf $(DESTDIR)$(sysconfdir)/bes/modules/dapreader.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/dapreader.conf

dapreader.conf: dapreader.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > dapreader.conf

#reader.conf: reader.conf.in $(top_builddir)/config.status
#	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
#                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
#                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
#                -e "s%[@]datarootdir[@]%${datarootdir}%" \
#                -e "s%[@]datadir[@]%${datadir}%" \
#                -e "s%[@]bindir[@]%${bindir}%" \
#                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > reader.conf

distclean-local:

# Not nearly as clean as it could be, but this removes .svn directories
# in subdirs.
dist-hook:
	rm -rf `find $(distdir) -name .svn`

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libdapreader_module_la_SOURCES)

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


