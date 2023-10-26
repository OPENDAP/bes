
AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = -I$(top_srcdir)/modules/dmrpp_module -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap \
    -I$(top_srcdir)/xmlcommand -I$(top_srcdir)/http -I$(top_srcdir)/pugixml/src $(DAP_CFLAGS)
AM_CXXFLAGS = -Wno-vla-extension -Wno-inconsistent-missing-override -fpic

# FIXME Remove this hack. Set these with configure. jhrg 11/25/19
OPENSSL_LIBS = -lcrypto
OPENSSL_INC = -I/usr/local/opt/openssl/include
OPENSSL_LDFLAGS = -L/usr/local/opt/openssl/lib

AM_CPPFLAGS += $(OPENSSL_INC)

AM_LDFLAGS = 
include $(top_srcdir)/coverage.mk

noinst_LTLIBRARIES = libdmrpp_types.la

libdmrpp_types_la_SOURCES = $(BES_SRCS) $(BES_HDRS)
libdmrpp_types_la_LDFLAGS =
libdmrpp_types_la_LIBADD =

BES_SRCS = DMRpp.cc DmrppCommon.cc Chunk.cc CurlHandlePool.cc DmrppByte.cc DmrppArray.cc \
DmrppFloat32.cc DmrppFloat64.cc DmrppInt16.cc DmrppInt32.cc DmrppInt64.cc \
DmrppInt8.cc DmrppUInt16.cc DmrppUInt32.cc DmrppUInt64.cc DmrppStr.cc  \
DmrppStructure.cc DmrppUrl.cc DmrppD4Enum.cc DmrppD4Group.cc DmrppD4Opaque.cc \
DmrppD4Sequence.cc  DmrppTypeFactory.cc DmrppParserSax2.cc DmrppMetadataStore.cc \
SuperChunk.cc DMZ.cc

BES_HDRS = DMRpp.h DmrppCommon.h Chunk.h  CurlHandlePool.h DmrppByte.h \
DmrppArray.h DmrppFloat32.h DmrppFloat64.h DmrppInt16.h DmrppInt32.h \
DmrppInt64.h DmrppInt8.h DmrppUInt16.h DmrppUInt32.h DmrppUInt64.h \
DmrppStr.h DmrppStructure.h DmrppUrl.h DmrppD4Enum.h DmrppD4Group.h \
DmrppD4Opaque.h DmrppD4Sequence.h DmrppTypeFactory.h DmrppParserSax2.h \
DmrppMetadataStore.h DmrppNames.h byteswap_compat.h  \
SuperChunk.h Base64.h DMZ.h DmrppChunkOdometer.h UnsupportedTypeException.h

EXTRA_PROGRAMS =

CLEANFILES = *~ *.gcda *.gcno *.gcov

distclean-local:

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libdmrpp_module_la_SOURCES)
