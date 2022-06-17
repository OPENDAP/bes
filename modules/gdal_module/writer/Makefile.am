
# Automake file for fileout_gdal
#
# 11/1/12 jhrg
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = $(GDAL_CFLAGS) $(DAP_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch
AM_CXXFLAGS =
AM_LDFLAGS = $(GDAL_LDFLAGS)
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

noinst_LTLIBRARIES = libgdal_writer.la

libgdal_writer_la_SOURCES = $(FONG_SRC) $(FONG_HDR)
libgdal_writer_la_LIBADD = $(LIBADD)
libgdal_writer_la_LDFLAGS = $(AM_LDFLAGS)
libgdal_writer_la_LDFLAGS += -no-undefined

FONG_SRC = GeoTiffTransmitter.cc JPEG2000Transmitter.cc FONgTransform.cc FONgGrid.cc

FONG_HDR = GeoTiffTransmitter.h JPEG2000Transmitter.h FONgTransform.h FONgGrid.h
