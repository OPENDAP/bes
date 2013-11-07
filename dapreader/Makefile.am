
# Automake file for dap-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch
ACLOCAL_AMFLAGS = -I conf
AM_CXXFLAGS = 

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
# ./configure --disable-shared CXXFLAGS=...
# or ./configure --enable-developer --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

if BES_DEVELOPER
AM_CXXFLAGS += $(CXXFLAGS_DEBUG)
endif

SUBDIRS = tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdapreader_module.la

BES_SRCS = DapModule.cc DapRequestHandler.cc

BES_HDRS = DapModule.h DapRequestHandler.h 

libdapreader_module_la_SOURCES = $(BES_SRCS) $(BES_HDRS)
libdapreader_module_la_CPPFLAGS = $(DAP_CFLAGS) $(AM_CPPFLAGS)
libdapreader_module_la_LDFLAGS = -avoid-version -module 
libdapreader_module_la_LIBADD = $(DAP_CLIENT_LIBS) -ltest-types

# I'm not sure why these were added, but they are not needed to 
# pass the tests. jhrg 11/6/13
#		\
	# $(abs_top_builddir)/dispatch/libbes_dispatch.la	\
	# $(abs_top_builddir)/xmlcommand/libbes_xml_command.la	\
	# $(abs_top_builddir)/ppt/libbes_ppt.la			\
	# $(OPENSSL_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = data

CLEANFILES = *~

