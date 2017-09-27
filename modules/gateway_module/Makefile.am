
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

EXTRA_DIST = gateway.conf.in

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


