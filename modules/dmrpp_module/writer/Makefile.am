
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = $(DAP_CFLAGS) $(H5_CPPFLAGS) \
    -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/http -I$(top_srcdir)/pugixml/src \
    -I$(top_srcdir)/modules/dmrpp_module -I$(top_srcdir)/modules/dmrpp_module/types

AM_CXXFLAGS =
AM_LDFLAGS =

if COMPILER_IS_GCC
AM_CXXFLAGS += -Wall -W
endif

include $(top_srcdir)/coverage.mk

noinst_LTLIBRARIES = libdmrpp_writer.la

libdmrpp_writer_la_SOURCES = $(FODMRPP_SRC) $(FODMRPP_HDR)
libdmrpp_writer_la_LDFLAGS = $(AM_LDFLAGS) -no-undefinedpwd

BUILT_SOURCES = h5common.cc h5common.h
DISTCLEANFILES = h5common.cc h5common.h

h5common.cc: Makefile
	cp $(top_srcdir)/modules/hdf5_handler/h5common.cc ./h5common.cc

h5common.h: Makefile
	cp $(top_srcdir)/modules/hdf5_handler/h5common.h ./h5common.h

FODMRPP_SRC = FODmrppTransmitter.cc $(top_srcdir)/modules/dmrpp_module/build_dmrpp_util.cc h5common.cc

FODMRPP_HDR = FODmrppTransmitter.h $(top_srcdir)/modules/dmrpp_module/build_dmrpp_util.h h5common.h
