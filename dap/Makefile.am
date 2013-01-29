
# Automake file for dap_module
#
# 01/22/2013 Hacked up by ndp
#

AM_CPPFLAGS = -I$(top_srcdir)/dispatch 

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

SUBDIRS = . bes-testsuite

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdap_module.la

BESDAP_SRCS = 	BESDASResponseHandler.cc \
	BESDDSResponseHandler.cc \
	BESDDXResponseHandler.cc \
	BESDataResponseHandler.cc \
	BESDataDDXResponseHandler.cc \
	BESDMRResponseHandler.cc \
	BESDapModule.cc \
	BESDapRequestHandler.cc \
	BESDapTransmit.cc \
	BESDapResponse.cc \
	BESDASResponse.cc \
	BESDDSResponse.cc \
	BESDataDDSResponse.cc \
	BESDMRResponse.cc \
	BESDapError.cc \
	BESDapErrorInfo.cc \
	BESDapService.cc \
	BESServerFunctionsList.cc

BESDAP_HDRS = BESDASResponseHandler.h \
	BESDDSResponseHandler.h \
	BESDDXResponseHandler.h \
	BESDataResponseHandler.h \
	BESDataDDXResponseHandler.h \
	BESDMRResponseHandler.h \
	BESDapModule.h \
	BESDapRequestHandler.h \
	BESDapTransmit.h \
	BESDapNames.h \
	BESDapResponse.h \
	BESDASResponse.h \
	BESDDSResponse.h \
	BESDMRResponse.h \
	BESDataDDSResponse.h \
	BESDapError.h \
	BESDapErrorInfo.h \
	BESDapService.h \
	BESServerFunctionsList.h

libdap_module_la_SOURCES = $(BESDAP_SRCS) $(BESDAP_HDRS)
libdap_module_la_CPPFLAGS = $(BES_CPPFLAGS) -I../dispatch $(DAP_CFLAGS)
libdap_module_la_LDFLAGS = -avoid-version -module 
libdap_module_la_LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

pkginclude_HEADERS = $(BESDAP_HDRS) 

# jhrg 1/25/13 $(BES_DAP_LIBS) $(DAP_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = COPYRIGHT COPYING besdsap_module.spec \
    besdap.conf.in OSX_Resources

CLEANFILES = *~ besdap.conf

install-data-local: dap.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/dap.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/dap.conf $(DESTDIR)$(sysconfdir)/bes/modules/dap.conf.$$currdate; \
	$(INSTALL_DATA) dap.conf $(DESTDIR)$(sysconfdir)/bes/modules/dap.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/dap.conf

# dap.conf: dap.conf.in ../config.status
#	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > dap.conf

dap.conf: dap.conf.in ../config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
                -e "s%[@]datarootdir[@]%${datarootdir}%" \
                -e "s%[@]datadir[@]%${datadir}%" \
                -e "s%[@]bindir[@]%${bindir}%" \
                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > dap.conf


# Not nearly as clean as it could be, but this removes .svn directories
# in subdirs.
dist-hook:
	rm -rf `find $(distdir) -name .svn`

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libdap_module_la_SOURCES)

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

