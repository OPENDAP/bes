
# Automake file for dap_module
#
# 01/22/2013 Hacked up by ndp
#

AUTOMAKE_OPTIONS = foreign check-news 

ACLOCAL_AMFLAGS = -I conf

AM_CPPFLAGS = $(XML2_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/xmlcommand $(BES_CPPFLAGS) -I$(top_srcdir)/dispatch \
$(DAP_CFLAGS)

#  -I$(top_srcdir)/modules/dmrpp_module

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

AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

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
	BESDapService.cc \
	BESDapResponseBuilder.cc \
	BESDapFunctionResponseCache.cc \
	BESStoredDapResultCache.cc \
	DapFunctionUtils.cc \
	DapUtils.cc \
	CachedSequence.cc \
	CacheTypeFactory.cc \
	TempFile.cc \
	CacheMarshaller.cc \
	CacheUnMarshaller.cc \
	ObjMemCache.cc \
	ShowPathInfoResponseHandler.cc \
	GlobalMetadataStore.cc

#	BESDapNullAggregationServer.cc BESDapErrorInfo.cc

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
	BESDapService.h \
	BESDapResponseBuilder.h \
	BESDapFunctionResponseCache.h \
	BESStoredDapResultCache.h \
	DapFunctionUtils.h \
	DapUtils.h \
	CachedSequence.h \
	CacheTypeFactory.h \
	TempFile.h \
	CacheMarshaller.h \
	CacheUnMarshaller.h \
	ObjMemCache.h \
	GlobalMetadataStore.h \
	ShowPathInfoResponseHandler.h

# 	BESDapNullAggregationServer.h BESDapErrorInfo.h

libdap_module_la_SOURCES = $(BESDAP_SRCS) $(BESDAP_HDRS)
# libdap_module_la_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dispatch $(DAP_CFLAGS)
libdap_module_la_LDFLAGS = -avoid-version -module 
libdap_module_la_LIBADD = $(DAP_LIBS) $(LIBS)

pkginclude_HEADERS = $(BESDAP_HDRS) 

pkgdata_DATA = dap_help.html dap_help.txt dap_help.xml

EXTRA_PROGRAMS = 

EXTRA_DIST = data dap.conf.in dap_help.html dap_help.txt dap_help.xml

CLEANFILES = *~ dap.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = dap.conf

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
