
# Automake file for fileout_netcdf
#
# 11/11/2008 pcw
#
# $Id: Makefile.am 19627 2008-09-18 17:18:05Z jimg $

# AUTOMAKE_OPTIONS = ; Moved to configure.ac

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=fileout_netcdf
M_VER=1.4.3

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(NC_CPPFLAGS) $(DAP_CFLAGS)
LIBADD = $(NC_LDFLAGS) $(NC_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(BES_CPPFLAGS) # or wahtever was set here or in ..._CPPFLAGS
LIBADD = $(BES_DAP_LIBS)      # and ..._LIBADD
endif

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

# I'm switching from the older tests in 'unit-tests' to the newer ones in 'tests'
SUBDIRS = . data/build_test_data tests 
DIST_SUBDIRS = data/build_test_data tests unit-tests 

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libfonc_module.la

libfonc_module_la_SOURCES = $(FONC_SRC) $(FONC_HDR)
# libfonc_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libfonc_module_la_LDFLAGS = -avoid-version -module 
libfonc_module_la_LIBADD = $(LIBADD)

FONC_SRC = FONcTransform.cc FONcTransmitter.cc FONcRequestHandler.cc	\
	FONcModule.cc FONcUtils.cc FONcStr.cc FONcShort.cc FONcInt.cc	\
	FONcFloat.cc FONcDouble.cc FONcStructure.cc FONcArray.cc	\
	FONcGrid.cc FONcSequence.cc FONcByte.cc FONcBaseType.cc		\
	FONcDim.cc FONcMap.cc FONcAttributes.cc

FONC_HDR = FONcTransform.h FONcTransmitter.h FONcRequestHandler.h	\
	FONcModule.h FONcUtils.h FONcStr.h FONcShort.h FONcInt.h	\
	FONcFloat.h FONcDouble.h FONcStructure.h FONcArray.h		\
	FONcGrid.h FONcSequence.h FONcByte.h FONcBaseType.h		\
	FONcDim.h FONcMap.h FONcAttributes.h

EXTRA_DIST = data COPYRIGHT COPYING fonc.conf.in doxy.conf

if !DAP_MODULES
EXTRA_DIST += fileout_netcdf.spec
endif

CLEANFILES = *~ fonc.conf

install-data-local: fonc.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/fonc.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/fonc.conf $(DESTDIR)$(sysconfdir)/bes/modules/fonc.conf.$$currdate; \
	$(INSTALL_DATA) fonc.conf $(DESTDIR)$(sysconfdir)/bes/modules/fonc.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/fonc.conf

fonc.conf: fonc.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > fonc.conf

.PHONY: docs
docs:
	srcdir=$(srcdir) doxygen $(srcdir)/doxy.conf
	(cd docs && tar -czf html.tar.gz html)

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(FONC_SRC) $(FONC_HDR)

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
# are there so that the values built into dap-config will match the mac
# osx install dirs and not the temp directory used to build the packages

# PACKAGEMAKER=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker*

PACKAGEMAKER=/Developer/usr/bin/packagemaker
PKG_CONF_FLAGS=

clean-pkg:
	-rm -rf mac_osx @PACKAGE@-@PACKAGE_VERSION@.pkg

pkg-build: clean-pkg
	./configure --prefix=$(prefix) --with-netcdf=$(NC_PATH) --disable-dependency-tracking $(PKG_CONF_FLAGS)
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






