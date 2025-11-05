# Automake file for cmr_module
#
# 07/25/18
#
AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=cmr_module
M_VER=1.1.0

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS) -I$(top_srcdir)/http
LIBADD = $(BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
#LIBADD = $(BES_DISPATCH_LIB) $(BES_DAP_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = .
# unit-tests tests; turned off on 10/28/25 because of CMR issues. jhrg

# These tests will fail if the DmrApiTest also fails: tests jhrg 6/29/23

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libcmr_module.la

libcmr_module_la_SOURCES = $(CMR_SRC) $(CMR_HDR)
libcmr_module_la_LDFLAGS = -avoid-version -module 
libcmr_module_la_LIBADD = $(LIBADD)

CMR_SRC = CmrApi.cc \
	Provider.cc Collection.cc  Granule.cc  GranuleUMM.cc \
	CmrCatalog.cc CmrModule.cc CmrContainer.cc CmrContainerStorage.cc \
	JsonUtils.cc

CMR_HDR = CmrApi.h CmrNames.h   \
	Provider.h Collection.h Granule.h GranuleUMM.h  \
	CmrCatalog.h CmrModule.h CmrContainer.h CmrContainerStorage.h \
	CmrInternalError.h CmrNotFoundError.h \
	JsonUtils.h


EXTRA_DIST = cmr.conf.in data

CLEANFILES = *~ cmr.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = cmr.conf

cmr.conf: cmr.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > cmr.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(CMR_SRC) $(CMR_HDR)


