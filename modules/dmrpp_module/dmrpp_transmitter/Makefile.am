
# Automake file for fileout_gdal
#
# 11/1/12 jhrg
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch -I$(top_srcdir)/modules/ngap_module

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

noinst_LTLIBRARIES = libdmrpp_return_as.la

libdmrpp_return_as_la_SOURCES = $(SRC) $(HDR)
libdmrpp_return_as_la_LIBADD = $(LIBADD)
libdmrpp_return_as_la_LDFLAGS = $(AM_LDFLAGS)
libdmrpp_return_as_la_LDFLAGS += -no-undefinedpwd

SRC = ReturnAsDmrppTransmitter.cc

HDR = ReturnAsDmrppTransmitter.h
