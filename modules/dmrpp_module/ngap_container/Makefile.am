# Automake file for ngap_module
#
# 07/25/18
#
AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/xmlcommand \
-I$(top_srcdir)/http $(DAP_CFLAGS)

LIBADD =
# BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests

noinst_LTLIBRARIES = libngap.la
libngap_la_SOURCES = $(NGAP_SRC) $(NGAP_HDR)
libngap_la_LDFLAGS =
libngap_la_LIBADD = $(LIBADD)

NGAP_SRC = NgapOwnedContainer.cc NgapOwnedContainerStorage.cc  NgapApi.cc

# NgapModule.cc NgapRequestHandler.cc
# MemoryCache.cc NgapContainer.cc NgapContainerStorage.cc

NGAP_HDR = NgapNames.h NgapOwnedContainer.h NgapOwnedContainerStorage.h \
    NgapApi.h MemoryCache.h

# NgapContainer.h NgapContainerStorage.h NgapRequestHandler.h NgapModule.h

# EXTRA_DIST = ngap.conf.in

CLEANFILES = *~

# ngap.conf

# moduledir = $(sysconfdir)/bes/modules
# module_DATA = ngap.conf

# ngap.conf: ngap.conf.in $(top_builddir)/config.status
# 	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > ngap.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(NGAP_SRC) $(NGAP_HDR)


