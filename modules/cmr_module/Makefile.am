
# Automake file for cmr_module
#
# 07/25/18
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=cmr_module
M_VER=1.0.4

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libcmr_module.la

libcmr_module_la_SOURCES = $(CMR_SRC) $(CMR_HDR)
libcmr_module_la_LDFLAGS = -avoid-version -module 
libcmr_module_la_LIBADD = $(LIBADD)

CMR_SRC = CmrApi.cc CmrCatalog.cc Granule.cc \
	CmrCache.cc curl_utils.cc rjson_utils.cc RemoteHttpResource.cc \
	CmrModule.cc CmrUtils.cc CmrContainer.cc CmrContainerStorage.cc

CMR_HDR = CmrApi.h CmrError.h CmrCatalog.h Granule.h \
	CmrCache.h curl_utils.h rjson_utils.h RemoteHttpResource.h \
	CmrModule.h CmrUtils.h CmrContainer.h CmrContainerStorage.h CmrNames.h

EXTRA_DIST = rapidjson cmr.conf.in data

CLEANFILES = *~ cmr.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = cmr.conf

cmr.conf: cmr.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > cmr.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(CMR_SRC) $(CMR_HDR)


