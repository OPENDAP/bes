# Automake file for ngap-specific code
#
# 07/25/18
#
AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/xmlcommand \
-I$(top_srcdir)/http $(DAP_CFLAGS)

LIBADD =

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests

noinst_LTLIBRARIES = libngap.la
libngap_la_SOURCES = $(NGAP_SRC) $(NGAP_HDR)
libngap_la_LDFLAGS =
libngap_la_LIBADD = $(LIBADD)

NGAP_SRC = NgapOwnedContainer.cc NgapOwnedContainerStorage.cc  NgapApi.cc

NGAP_HDR = NgapNames.h NgapOwnedContainer.h NgapOwnedContainerStorage.h \
    NgapApi.h MemoryCache.h

# EXTRA_DIST = ngap.conf.in

CLEANFILES = *~

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(NGAP_SRC) $(NGAP_HDR)


