
# Automake file for dap-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

AM_CPPFLAGS = -I$(top_srcdir)/dispatch
ACLOCAL_AMFLAGS = -I conf

SUBDIRS = bes-testsuite

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdapreader_module.la

BES_SRCS = \
		DapModule.cc DapRequestHandler.cc

BES_HDRS = \
		DapModule.h DapRequestHandler.h DapResponseNames.h

libdapreader_module_la_SOURCES = $(BES_SRCS) $(BES_HDRS)
libdapreader_module_la_CPPFLAGS = $(DAP_CFLAGS) $(AM_CPPFLAGS)
libdapreader_module_la_LDFLAGS = -avoid-version -module 
libdapreader_module_la_LIBADD = $(BES_DAP_LIBS) $(DAP_CLIENT_LIBS) -L$(libdir) -lbes_dispatch -lbes_xml_command -lbes_ppt -lbes_dap $(OPENSSL_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = data COPYRIGHT COPYING

CLEANFILES = *~

