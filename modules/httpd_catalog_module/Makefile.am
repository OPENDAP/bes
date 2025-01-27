
# Automake file for httpd_catalog_module
#
# 10/16/18 - ndp
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=httpd_catalog_module
M_VER=0.9.9

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/http -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libhttpd_catalog_module.la

libhttpd_catalog_module_la_SOURCES = $(HTTP_CAT_SRC) $(HTTP_CAT_HDR)
libhttpd_catalog_module_la_LDFLAGS = -avoid-version -module 
libhttpd_catalog_module_la_LIBADD = $(LIBADD)

HTTP_CAT_SRC = HttpdCatalogModule.cc HttpdCatalog.cc \
	HttpdCatalogContainer.cc HttpdCatalogContainerStorage.cc \
	HttpdDirScraper.cc

HTTP_CAT_HDR = HttpdCatalog.h HttpdCatalogNames.h \
	HttpdCatalogModule.h \
	HttpdCatalogContainer.h HttpdCatalogContainerStorage.h \
	HttpdDirScraper.h 

# HttpdCatalogError.h  

EXTRA_DIST = data httpd_catalog.conf.in 

CLEANFILES = *~ httpd_catalog.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = httpd_catalog.conf

httpd_catalog.conf: httpd_catalog.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > $@

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(HTTP_CAT_SRC) $(HTTP_CAT_HDR)
