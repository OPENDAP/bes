
# Automake file for gateway-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=gateway_module
M_VER=1.3.2

AM_CPPFLAGS = -I$(top_srcdir)  -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/http -I$(top_srcdir)/xmlcommand $(DAP_CFLAGS)
# LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
LIBADD = $(BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libgateway_module.la

GATEWAY_SRCS = GatewayModule.cc \
		GatewayRequestHandler.cc \
		GatewayContainer.cc \
		GatewayContainerStorage.cc \
		GatewayError.cc \
		GatewayPathInfoCommand.cc \
		GatewayPathInfoResponseHandler.cc

GATEWAY_HDRS = GatewayModule.h \
		GatewayRequestHandler.h \
		GatewayNames.h \
		GatewayContainer.h \
		GatewayContainerStorage.h \
		GatewayError.h \
		GatewayPathInfoCommand.h \
		GatewayPathInfoResponseHandler.h

libgateway_module_la_SOURCES = $(GATEWAY_SRCS) $(GATEWAY_HDRS)
libgateway_module_la_LDFLAGS = -avoid-version -module 
libgateway_module_la_LIBADD = $(LIBADD)

EXTRA_PROGRAMS = 

EXTRA_DIST = gateway.conf.in

CLEANFILES = *~ gateway.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = gateway.conf

gateway.conf: gateway.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > gateway.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libgateway_module_la_SOURCES)


