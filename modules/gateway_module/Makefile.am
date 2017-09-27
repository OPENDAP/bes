
# Automake file for gateway-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=gateway_module
M_VER=1.2.1

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(BES_CPPFLAGS) # or wahtever was set here or in ..._CPPFLAGS
LIBADD = $(BES_DAP_LIBS)      # and ..._LIBADD
endif

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libgateway_module.la

GATEWAY_SRCS = \
		GatewayModule.cc GatewayRequestHandler.cc		\
		GatewayContainer.cc GatewayContainerStorage.cc		\
		GatewayError.cc GatewayRequest.cc GatewayUtils.cc \
		GatewayCache.cc RemoteHttpResource.cc curl_utils.cc

GATEWAY_HDRS = \
		GatewayModule.h GatewayRequestHandler.h			\
		GatewayResponseNames.h					\
		GatewayContainer.h GatewayContainerStorage.h		\
		GatewayError.h GatewayRequest.h GatewayUtils.h  \
		GatewayCache.h RemoteHttpResource.h curl_utils.h

libgateway_module_la_SOURCES = $(GATEWAY_SRCS) $(GATEWAY_HDRS)
# libgateway_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libgateway_module_la_LDFLAGS = -avoid-version -module 
libgateway_module_la_LIBADD = $(LIBADD)

EXTRA_PROGRAMS = 

EXTRA_DIST = COPYRIGHT COPYING \
    gateway.conf.in OSX_Resources

if !DAP_MODULES
EXTRA_DIST += gateway_module.spec
endif

CLEANFILES = *~ gateway.conf

install-data-local: gateway.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/gateway.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/gateway.conf $(DESTDIR)$(sysconfdir)/bes/modules/gateway.conf.$$currdate; \
	$(INSTALL_DATA) gateway.conf $(DESTDIR)$(sysconfdir)/bes/modules/gateway.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/gateway.conf

gateway.conf: gateway.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > gateway.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libgateway_module_la_SOURCES)

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

