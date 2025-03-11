
# Automake file for fileout_gdal
#
# 11/1/12 jhrg
AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = $(DMRPP_CFLAGS) $(DAP_CFLAGS) $(H5_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/dap \
    -I$(top_srcdir)/dispatch -I$(top_srcdir)/http -I$(top_srcdir)/pugixml/src \
    -I$(top_srcdir)/modules/dmrpp_module

AM_CXXFLAGS =
AM_LDFLAGS = $(DMRPP_LDFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

if COMPILER_IS_GCC
AM_CXXFLAGS += -Wall -W -Wcast-align
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -Wall -W -Wcast-align

include $(top_srcdir)/coverage.mk

noinst_LTLIBRARIES = libdmrpp_writer.la

libdmrpp_writer_la_SOURCES = $(FODMRPP_SRC) $(FODMRPP_HDR)
libdmrpp_writer_la_LIBADD = $(LIBADD)
libdmrpp_writer_la_LDFLAGS = $(AM_LDFLAGS)
libdmrpp_writer_la_LDFLAGS += -no-undefinedpwd

BUILT_SOURCES = h5common.cc h5common.h
DISTCLEANFILES = h5common.cc h5common.h

h5common.cc: Makefile
	cp $(top_srcdir)/modules/hdf5_handler/h5common.cc ./h5common.cc

h5common.h: Makefile
	cp $(top_srcdir)/modules/hdf5_handler/h5common.h ./h5common.h

FODMRPP_SRC = FODmrppTransmitter.cc $(top_srcdir)/modules/dmrpp_module/build_dmrpp_util.cc h5common.cc

FODMRPP_HDR = FODmrppTransmitter.h $(top_srcdir)/modules/dmrpp_module/build_dmrpp_util.h h5common.h
