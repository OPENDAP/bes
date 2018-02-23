
# Automake file for dap_module
#
# 01/22/2013 Hacked up by ndp
#

AUTOMAKE_OPTIONS = foreign check-news 

ACLOCAL_AMFLAGS = -I conf

AM_CPPFLAGS =  $(XML2_CFLAGS) -I$(top_srcdir)/xmlcommand
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

SUBDIRS = . unit-tests tests

# These tests depend on dapreader, which is built after this code.
# jhrg 3/17/15. 
# tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdap_module.la

BESDAP_SRCS = 	BESDASResponseHandler.cc \
	BESDDSResponseHandler.cc \
	BESDDXResponseHandler.cc \
	BESDataResponseHandler.cc \
	BESDataDDXResponseHandler.cc \
	BESDMRResponseHandler.cc \
	BESDap4ResponseHandler.cc \
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
	BESDapResponseBuilder.cc \
	BESDapFunctionResponseCache.cc \
	BESStoredDapResultCache.cc \
	BESDapNullAggregationServer.cc \
	DapFunctionUtils.cc \
	CachedSequence.cc \
	CacheTypeFactory.cc \
	BESHandlerUtil.cc \
	TempFile.cc \
	CacheMarshaller.cc \
	CacheUnMarshaller.cc \
	ObjMemCache.cc \
	ShowPathInfoResponseHandler.cc \
	GlobalMetadataStore.cc

BESDAP_HDRS = BESDASResponseHandler.h \
	BESDDSResponseHandler.h \
	BESDDXResponseHandler.h \
	BESDataResponseHandler.h \
	BESDataDDXResponseHandler.h \
	BESDMRResponseHandler.h \
	BESDap4ResponseHandler.h \
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
	BESDapResponseBuilder.h \
	BESDapFunctionResponseCache.h \
	BESStoredDapResultCache.h \
	BESDapNullAggregationServer.h \
	DapFunctionUtils.h \
	CachedSequence.h \
	CacheTypeFactory.h \
	BESHandlerUtil.h \
	TempFile.h \
	CacheMarshaller.h \
	CacheUnMarshaller.h \
	ObjMemCache.h \
	GlobalMetadataStore.h \
	ShowPathInfoResponseHandler.h

libdap_module_la_SOURCES = $(BESDAP_SRCS) $(BESDAP_HDRS)
libdap_module_la_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dispatch $(DAP_CFLAGS)
libdap_module_la_LDFLAGS = -avoid-version -module 
libdap_module_la_LIBADD = $(DAP_LIBS) $(LIBS)

pkginclude_HEADERS = $(BESDAP_HDRS) 

pkgdata_DATA = dap_help.html dap_help.txt dap_help.xml

EXTRA_PROGRAMS = 

EXTRA_DIST = data dap.conf.in dap_help.html dap_help.txt dap_help.xml

CLEANFILES = *~ dap.conf

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

dap.conf: dap.conf.in $(top_builddir)/config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
                -e "s%[@]datarootdir[@]%${datarootdir}%" \
                -e "s%[@]datadir[@]%${datadir}%" \
                -e "s%[@]bindir[@]%${bindir}%" \
                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > dap.conf

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

