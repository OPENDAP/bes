
# Automake file for httpd_catalog_module
#
# 10/16/18 - ndp
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=httpd_catalog_module
M_VER=0.9.0

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . 
# Move these back once there are tests. jhrg 10/15/18
# unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libhttpd_catalog_module.la

libhttpd_catalog_module_la_SOURCES = $(HTTP_CAT_SRC) $(HTTP_CAT_HDR)
libhttpd_catalog_module_la_LDFLAGS = -avoid-version -module 
libhttpd_catalog_module_la_LIBADD = $(LIBADD)

HTTP_CAT_SRC = HttpdCatalogModule.cc HttpdCatalogUtils.cc HttpdCatalog.cc \
	curl_utils.cc RemoteHttpResourceCache.cc RemoteHttpResource.cc \
	HttpdDirScraper.cc
	
HTTP_CAT_HDR = HttpdCatalogError.h  HttpdCatalog.h \
	HttpdCatalogModule.h HttpdCatalogUtils.h HttpdCatalogNames.h \
	curl_utils.h RemoteHttpResourceCache.h RemoteHttpResource.h \
	HttpdDirScraper.h 

EXTRA_DIST = data httpd_catalog.conf.in 

CLEANFILES = *~ httpd_catalog.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = httpd_catalog.conf

httpd_catalog.conf: httpd_catalog.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > $@

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(HTTP_CAT_SRC) $(HTTP_CAT_HDR)
