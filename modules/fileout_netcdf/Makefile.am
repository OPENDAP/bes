
# Automake file for fileout_netcdf
#
# 11/11/2008 pcw
#
# $Id: Makefile.am 19627 2008-09-18 17:18:05Z jimg $

# AUTOMAKE_OPTIONS = ; Moved to configure.ac

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=fileout_netcdf
M_VER=1.5.5

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/rapidjson $(NC_CPPFLAGS) $(DAP_CFLAGS)
LIBADD = $(NC_LDFLAGS) $(NC_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS = -Wno-inconsistent-missing-override
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . data/build_test_data unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libfonc_module.la

libfonc_module_la_SOURCES = $(FONC_SRC) $(FONC_HDR)
# libfonc_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libfonc_module_la_LDFLAGS = -avoid-version -module 
libfonc_module_la_LIBADD = $(LIBADD)

FONC_SRC = FONcTransform.cc FONcTransmitter.cc FONcRequestHandler.cc	\
	FONcModule.cc FONcUtils.cc FONcStr.cc FONcShort.cc FONcInt.cc	\
	FONcFloat.cc FONcDouble.cc FONcStructure.cc FONcArray.cc	\
	FONcGrid.cc FONcSequence.cc FONcByte.cc FONcBaseType.cc		\
	FONcDim.cc FONcMap.cc FONcAttributes.cc FONcUShort.cc FONcUInt.cc	\
	FONcUByte.cc FONcInt64.cc FONcUInt64.cc FONcInt8.cc history_utils.cc \
	d4_tools.cc

FONC_HDR = FONcTransform.h FONcTransmitter.h FONcRequestHandler.h	\
	FONcModule.h FONcUtils.h FONcStr.h FONcShort.h FONcInt.h	\
	FONcFloat.h FONcDouble.h FONcStructure.h FONcArray.h		\
	FONcGrid.h FONcSequence.h FONcByte.h FONcBaseType.h		\
	FONcDim.h FONcMap.h FONcAttributes.h FONcUShort.h FONcUInt.h	\
	FONcUByte.h FONcInt64.h FONcUInt64.h FONcInt8.h history_utils.h \
	FONcNames.h d4_tools.h

EXTRA_DIST = data fonc.conf.in

CLEANFILES = *~ fonc.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = fonc.conf

fonc.conf: fonc.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > fonc.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(FONC_SRC) $(FONC_HDR)
