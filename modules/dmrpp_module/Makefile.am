
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)

# removed $(H5_CPPFLAGS) 

ACLOCAL_AMFLAGS = -I conf
AM_CXXFLAGS = 

# Set the module version here, in the spec file and in configure.ac
M_NAME=dmrpp_module
M_VER=1.0.1

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
# ./configure --disable-shared CXXFLAGS=...
# or ./configure --enable-developer --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = . unit-tests tests data

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdmrpp_module.la

BES_SRCS = DMRpp.cc DmrppCommon.cc Chunk.cc CurlHandlePool.cc DmrppByte.cc DmrppArray.cc \
DmrppFloat32.cc DmrppFloat64.cc DmrppInt16.cc DmrppInt32.cc DmrppInt64.cc \
DmrppInt8.cc DmrppUInt16.cc DmrppUInt32.cc DmrppUInt64.cc DmrppStr.cc  \
DmrppStructure.cc DmrppUrl.cc DmrppD4Enum.cc DmrppD4Group.cc DmrppD4Opaque.cc \
DmrppD4Sequence.cc  DmrppTypeFactory.cc DmrppParserSax2.cc DmrppMetadataStore.cc

BES_HDRS = DMRpp.h DmrppCommon.h Chunk.h  CurlHandlePool.h DmrppByte.h \
DmrppArray.h DmrppFloat32.h DmrppFloat64.h DmrppInt16.h DmrppInt32.h \
DmrppInt64.h DmrppInt8.h DmrppUInt16.h DmrppUInt32.h DmrppUInt64.h \
DmrppStr.h DmrppStructure.h DmrppUrl.h DmrppD4Enum.h DmrppD4Group.h \
DmrppD4Opaque.h DmrppD4Sequence.h DmrppTypeFactory.h DmrppParserSax2.h \
DmrppMetadataStore.h

DMRPP_MODULE = DmrppModule.cc DmrppRequestHandler.cc DmrppModule.h DmrppRequestHandler.h

BUILD_DMRPP = DmrppRequestHandler.cc DmrppRequestHandler.h

libdmrpp_module_la_SOURCES = $(BES_HDRS) $(BES_SRCS) $(DMRPP_MODULE)
libdmrpp_module_la_LDFLAGS = -avoid-version -module 
libdmrpp_module_la_LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(H5_LDFLAGS) $(H5_LIBS) -ltest-types 

bin_PROGRAMS = build_dmrpp

build_dmrpp_CPPFLAGS = $(AM_CPPFLAGS) $(H5_CPPFLAGS)

build_dmrpp_SOURCES = $(BES_SRCS) $(BES_HDRS) $(BUILD_DMRPP) build_dmrpp.cc

build_dmrpp_LDADD = $(BES_DISPATCH_LIB) $(DAP_MODULE_OBJS) $(BES_EXTRA_LIBS) \
$(H5_LDFLAGS) $(H5_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) -lz

EXTRA_PROGRAMS = 

EXTRA_DIST = dmrpp.conf.in

CLEANFILES = *~ dmrpp.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = dmrpp.conf

dmrpp.conf: dmrpp.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > dmrpp.conf

distclean-local:

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libdmrpp_module_la_SOURCES)

DAP_MODULE_OBJS = $(top_builddir)/dap/BESDASResponseHandler.o \
$(top_builddir)/dap/BESDDSResponseHandler.o \
$(top_builddir)/dap/BESDDXResponseHandler.o \
$(top_builddir)/dap/BESDataResponseHandler.o \
$(top_builddir)/dap/BESDataDDXResponseHandler.o \
$(top_builddir)/dap/BESDMRResponseHandler.o \
$(top_builddir)/dap/BESDap4ResponseHandler.o \
$(top_builddir)/dap/BESDapRequestHandler.o \
$(top_builddir)/dap/BESDapTransmit.o \
$(top_builddir)/dap/BESDapResponse.o \
$(top_builddir)/dap/BESDASResponse.o \
$(top_builddir)/dap/BESDDSResponse.o \
$(top_builddir)/dap/BESDataDDSResponse.o \
$(top_builddir)/dap/BESDMRResponse.o \
$(top_builddir)/dap/BESDapError.o \
$(top_builddir)/dap/BESDapErrorInfo.o \
$(top_builddir)/dap/BESDapService.o \
$(top_builddir)/dap/BESDapResponseBuilder.o \
$(top_builddir)/dap/BESDapFunctionResponseCache.o \
$(top_builddir)/dap/BESStoredDapResultCache.o \
$(top_builddir)/dap/BESDapNullAggregationServer.o \
$(top_builddir)/dap/DapFunctionUtils.o \
$(top_builddir)/dap/CachedSequence.o \
$(top_builddir)/dap/CacheTypeFactory.o \
$(top_builddir)/dap/TempFile.o \
$(top_builddir)/dap/CacheMarshaller.o \
$(top_builddir)/dap/CacheUnMarshaller.o \
$(top_builddir)/dap/ObjMemCache.o \
$(top_builddir)/dap/ShowPathInfoResponseHandler.o \
$(top_builddir)/dap/GlobalMetadataStore.o
