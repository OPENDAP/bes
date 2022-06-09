
# Automake file for the gdal_handler
#
# 1/11/12 jhrg
#

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = $(GDAL_CFLAGS) $(DAP_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch
AM_LDFLAGS = $(GDAL_LDFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CXXFLAGS =

if COMPILER_IS_GCC
AM_CXXFLAGS += -Wall -W -Wcast-align
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -Wall -W -Wcast-align 

include $(top_srcdir)/coverage.mk

noinst_LTLIBRARIES = libgdal_reader.la

libgdal_reader_la_SOURCES = $(GDAL_SRCS)
libgdal_reader_la_LIBADD = $(LIBADD)
libgdal_reader_la_LDFLAGS = $(AM_LDFLAGS)
libgdal_reader_la_LDFLAGS += -no-undefined

GDAL_SRCS = GDALArray.cc GDALGrid.cc gdal_utils.cc

GDAL_HDRS = GDALTypes.h gdal_utils.h
