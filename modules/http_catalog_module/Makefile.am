
# Automake file for cmr_module
#
# 07/25/18
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=http_catalog_module
M_VER=0.9.0

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . 
# Move these back once there are tests. jhrg 10/15/18
# unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libhttp_catalog_module.la

libhttp_catalog_module_la_SOURCES = $(HTTP_CAT_SRC) $(HTTP_CAT_HDR)
libhttp_catalog_module_la_LDFLAGS = -avoid-version -module 
libhttp_catalog_module_la_LIBADD = $(LIBADD)

HTTP_CAT_SRC = HttpCatalogCache.cc HttpCatalogModule.cc HttpCatalogUtils.cc \
	curl_utils.cc RemoteHttpResource.cc
	
HTTP_CAT_HDR = HttpCatalogError.h HttpCatalogCache.h  \
	HttpCatalogModule.h HttpCatalogUtils.h HttpCatalogNames.h \
	curl_utils.h RemoteHttpResource.h

EXTRA_DIST = data http_catalog.conf.in 

CLEANFILES = *~ http_catalog.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = http_catalog.conf

http_catalog.conf: http_catalog.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > $@

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(HTTP_CAT_SRC) $(HTTP_CAT_HDR)
