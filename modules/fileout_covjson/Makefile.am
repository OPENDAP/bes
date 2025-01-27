
# Automake file for fileout_json
#
# 06/23/14 ndp
#

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=fileout_covjson
M_VER=1.1.0

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"
AM_CPPFLAGS += -Wno-vla-extension

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libfocovjson_module.la

libfocovjson_module_la_SOURCES = $(FOCOVJSON_SRC) $(FOCOVJSON_HDR)
libfocovjson_module_la_LDFLAGS = -avoid-version -module
libfocovjson_module_la_LIBADD = $(LIBADD)

FOCOVJSON_SRC = FoCovJsonRequestHandler.cc FoCovJsonModule.cc FoDapCovJsonTransmitter.cc FoDapCovJsonTransform.cc \
	StreamString.cc focovjson_utils.cc

FOCOVJSON_HDR = FoCovJsonRequestHandler.h FoCovJsonModule.h FoDapCovJsonTransmitter.h FoDapCovJsonTransform.h \
	StreamString.h focovjson_utils.h

EXTRA_DIST = data focovjson.conf.in

CLEANFILES = *~ focovjson.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = focovjson.conf

focovjson.conf: focovjson.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > focovjson.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:
	cccc --outdir=$(C4_DIR) $(FONC_SRC) $(FONC_HDR)
