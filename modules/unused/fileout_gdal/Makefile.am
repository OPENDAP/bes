
# Automake file for fileout_gdal
#
# 11/1/12 jhrg

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

SUBDIRS = . tests
# DIST_SUBDIRS = tests

# Set the module version here, in the spec file and in configure.ac
M_NAME=fileout_gdal
M_VER=0.11.0

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(GDAL_CFLAGS) $(DAP_CFLAGS)
LIBADD = $(GDAL_LDFLAGS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libfong_module.la

libfong_module_la_SOURCES = $(FONG_SRC) $(FONG_HDR)
# libfong_module_la_CPPFLAGS = $(BES_CPPFLAGS) $(GDAL_CFLAGS)
libfong_module_la_LDFLAGS = -avoid-version -module 
libfong_module_la_LIBADD = $(LIBADD)

FONG_SRC = GeoTiffTransmitter.cc JPEG2000Transmitter.cc FONgRequestHandler.cc	\
	FONgModule.cc FONgTransform.cc FONgGrid.cc

FONG_HDR = GeoTiffTransmitter.h JPEG2000Transmitter.h FONgRequestHandler.h	\
	FONgModule.h FONgTransform.h FONgGrid.h

EXTRA_DIST = data fong.conf.in

CLEANFILES = *~ fong.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = fong.conf

fong.conf: fong.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > fong.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(FONG_SRC) $(FONG_HDR)
