
# Build libdebug_functions, part of libdap.

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=debug_functions
M_VER=1.0.3

if DAP_MODULES
AM_CPPFLAGS = $(GF_CFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(GF_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(GF_CFLAGS) $(XML2_CFLAGS)
LIBADD = $(GF_LIBS) 
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs


SUBDIRS =  . unit-tests tests
# DIST_SUBDIRS = unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdebug_functions.la

SRCS =  \
	DebugFunctions.cc 

HDRS =  \
	DebugFunctions.h 
	
libdebug_functions_la_SOURCES = $(SRCS) $(HDRS)
# libdebug_functions_la_CPPFLAGS = $(GF_CFLAGS) $(XML2_CFLAGS)
libdebug_functions_la_LDFLAGS = -avoid-version -module 
libdebug_functions_la_LIBADD = $(LIBADD) 

EXTRA_DIST = data COPYING debug_functions.conf.in

if !DAP_MODULES
EXTRA_DIST += debug_functions.spec
endif

package_version=$(PACKAGE_MAJOR_VERSION).$(PACKAGE_MINOR_VERSION).$(PACKAGE_SUBMINOR_VERSION)

CLEANFILES = *~ debug_functions.conf debug_functions-*.tar.gz

install-data-local: debug_functions.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/debug_functions.conf \
	      && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/debug_functions.conf $(DESTDIR)$(sysconfdir)/bes/modules/debug_functions.conf.$$currdate; \
	$(INSTALL_DATA) debug_functions.conf $(DESTDIR)$(sysconfdir)/bes/modules/debug_functions.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/debug_functions.conf

debug_functions.conf: debug_functions.conf.in $(top_builddir)/config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
                -e "s%[@]datarootdir[@]%${datarootdir}%" \
                -e "s%[@]datadir[@]%${datadir}%" \
                -e "s%[@]bindir[@]%${bindir}%" \
                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > debug_functions.conf

                # Not nearly as clean as it could be, but this removes .svn directories
C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libdebug_functions_la_SOURCES)

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
