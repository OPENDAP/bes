# Automake file for builddmrpp_module
#
# 03/22/23
#
AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=ngap_module
M_VER=1.0.8

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/xmlcommand -I$(top_srcdir)/http -I$(top_srcdir)/modules/ngap_module $(DAP_CFLAGS)
LIBADD = $(BES_DISPATCH_LIB) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes

lib_bes_LTLIBRARIES = libbuilddmrpp_module.la
libbuilddmrpp_module_la_SOURCES = $(BULIDDMRPP_SRC) $(BUILDDMRPP_HDR)
libbuilddmrpp_module_la_LDFLAGS = -avoid-version -module 
libbuilddmrpp_module_la_LIBADD = $(LIBADD)

BUILDDMRPP_SRC =  BuildDmrppRequestHandler.cc \
            BuildDmrppModule.cc \
            NgapBuildDmrppContainer.cc \
            NgapBuildDmrppContainerStorage.cc

BUILDDMRPP_HDR =  BuildDmrppRequestHandler.h \
            BuildDmrppModule.h \
            NgapBuildDmrppContainer.h \
            NgapBuildDmrppContainerStorage.h

# Not used. jhrg 3/30/22 NgapError.h

EXTRA_DIST = builddmrpp.conf.in data

CLEANFILES = *~ builddmrpp.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = builddmrpp.conf

builddmrpp.conf: builddmrpp.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > builddmrpp.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(BUILDDMRPP_SRC) $(BUILDDMRPP_HDR)


