# Automake file for ngap_module
#
# 07/25/18
#
AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=ngap_module
M_VER=1.0.9

AM_CPPFLAGS = -I$(top_srcdir)-I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/xmlcommand \
-I$(top_srcdir)/http $(DAP_CFLAGS)
LIBADD = $(BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes

lib_bes_LTLIBRARIES = libngap_module.la
libngap_module_la_SOURCES = $(NGAP_SRC) $(NGAP_HDR)
libngap_module_la_LDFLAGS = -avoid-version -module 
libngap_module_la_LIBADD = $(LIBADD)

NGAP_SRC =  NgapRequestHandler.cc \
            NgapModule.cc \
            NgapOwnedContainer.cc \
            NgapOwnedContainerStorage.cc \
            NgapApi.cc

# MemoryCache.cc NgapContainer.cc NgapContainerStorage.cc

NGAP_HDR =  NgapNames.h \
            NgapRequestHandler.h \
            NgapModule.h \
            NgapOwnedContainer.h \
            NgapOwnedContainerStorage.h \
            NgapApi.h \
            MemoryCache.h

# NgapContainer.h NgapContainerStorage.h

EXTRA_DIST = ngap.conf.in

CLEANFILES = *~ ngap.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = ngap.conf

ngap.conf: ngap.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > ngap.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(NGAP_SRC) $(NGAP_HDR)


