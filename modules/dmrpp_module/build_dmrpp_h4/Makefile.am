# Automake file for cmr_module
#
# 07/25/18
#
AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=builddmrpp_h4
M_VER=0.1.0

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/modules/dmrpp_module \
    -I$(top_srcdir)/pugixml/src -I$(top_srcdir)/http -I$(top_srcdir)/modules/ngap_module -I$(top_srcdir)/dap \
    $(DAP_CFLAGS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

# SUBDIRS = . unit-tests tests

# These tests will fail if the DmrApiTest also fails: tests jhrg 6/29/23

bin_PROGRAMS = build_dmrpp_h4

build_dmrpp_h4_SOURCES = $(SRC) $(HDR)

SRC = build_dmrpp_h4.cc build_dmrpp_util_h4.cc ../Chunk.cc ../DMRpp.cc ../DMZ.cc ../DmrppArray.cc ../DmrppByte.cc ../DmrppCommon.cc \
   ../DmrppD4Enum.cc ../DmrppD4Group.cc ../DmrppD4Opaque.cc ../DmrppD4Sequence.cc ../DmrppFloat32.cc ../DmrppFloat64.cc \
   ../DmrppInt16.cc ../DmrppInt32.cc ../DmrppInt64.cc ../DmrppInt8.cc ../DmrppStr.cc ../DmrppStructure.cc \
   ../DmrppTypeFactory.cc ../DmrppUInt16.cc ../DmrppUInt32.cc ../DmrppUInt64.cc ../DmrppUrl.cc ../SuperChunk.cc \
   ../DmrppRequestHandler.cc ../CurlHandlePool.cc

HDR = build_dmrpp_util_h4.h ../Chunk.h ../DMRpp.h ../DMZ.h ../DmrppArray.h ../DmrppByte.h ../DmrppCommon.h \
    ../DmrppD4Enum.h ../DmrppD4Group.h ../DmrppD4Opaque.h ../DmrppD4Sequence.h ../DmrppFloat32.h ../DmrppFloat64.h \
    ../DmrppInt16.h ../DmrppInt32.h ../DmrppInt64.h ../DmrppInt8.h ../DmrppStr.h ../DmrppStructure.h \
    ../DmrppTypeFactory.h ../DmrppUInt16.h ../DmrppUInt32.h ../DmrppUInt64.h ../DmrppUrl.h ../SuperChunk.h \
    ../DmrppRequestHandler.h ../CurlHandlePool.h

build_dmrpp_h4_CPPFLAGS = $(AM_CPPFLAGS)

build_dmrpp_h4_LDFLAGS = $(HDF4_LDFLAGS)

build_dmrpp_h4_LDADD = $(BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(BES_DAP_LIB) $(DAP_LIBS) $(OPENSSL_LDFLAGS) \
	$(OPENSSL_LIBS) $(XML2_LIBS) $(BYTESWAP_LIBS) $(HDF4_LIBS)

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


