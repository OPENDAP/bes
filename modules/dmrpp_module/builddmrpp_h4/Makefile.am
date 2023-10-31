# Automake file for cmr_module
#
# 07/25/18
#
AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=builddmrpp_h4
M_VER=0.1.0

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS) -I$(top_srcdir)/http
LIBADD = $(BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
#LIBADD = $(BES_DISPATCH_LIB) $(BES_DAP_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

# SUBDIRS = . unit-tests tests

# These tests will fail if the DmrApiTest also fails: tests jhrg 6/29/23

bin_PROGRAMS = build_dmrpp_h4

build_dmrpp_h4_SOURCES = $(SRC) $(HDR)

SRC = build_dmrpp_h4.cc

HDR =

build_dmrpp_h4_CPPFLAGS = $(AM_CPPFLAGS)

build_dmrpp_h4_LDFLAGS =

build_dmrpp_h4_LDADD = $(BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(OPENSSL_LDFLAGS)
	$(OPENSSL_LIBS) $(XML2_LIBS) $(BYTESWAP_LIBS) -lz

lib_besdir=$(libdir)/bes

# lib_bes_LTLIBRARIES = libcmr_module.la

# libcmr_module_la_SOURCES = $(CMR_SRC) $(CMR_HDR)
# libcmr_module_la_LDFLAGS = -avoid-version -module
# libcmr_module_la_LIBADD = $(LIBADD)


EXTRA_DIST =

CLEANFILES = *~

moduledir = $(sysconfdir)/bes/modules
module_DATA =

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(CMR_SRC) $(CMR_HDR)


