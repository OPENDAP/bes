# Automake file for ngap_module
#
# 07/25/18
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=ngap_module
M_VER=1.0.3

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libngap_module.la

libngap_module_la_SOURCES = $(NGAP_SRC) $(NGAP_HDR)
libngap_module_la_LDFLAGS = -avoid-version -module 
libngap_module_la_LIBADD = $(LIBADD)

NGAP_SRC = NgapApi.cc Granule.cc \
	NgapCache.cc rjson_utils.cc RemoteHttpResource.cc \
	NgapModule.cc NgapUtils.cc NgapContainer.cc 

NGAP_HDR = NgapApi.h NgapError.h Granule.h \
	NgapCache.h rjson_utils.h RemoteHttpResource.h \
	NgapModule.h NgapUtils.h NgapContainer.h NgapNames.h

EXTRA_DIST = rapidjson ngap.conf.in data

CLEANFILES = *~ ngap.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = ngap.conf

ngap.conf: ngap.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > ngap.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(NGAP_SRC) $(NGAP_HDR)


