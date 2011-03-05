
# Automake file for dap-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

AM_CPPFLAGS = -I$(top_srcdir)/dispatch
ACLOCAL_AMFLAGS = -I conf

#SUBDIRS = bes-testsuite
SUBDIRS = 

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdapreader_module.la

BES_SRCS = \
		DapModule.cc DapRequestHandler.cc

BES_HDRS = \
		DapModule.h DapRequestHandler.h DapResponseNames.h

libdapreader_module_la_SOURCES = $(BES_SRCS) $(BES_HDRS)
libdapreader_module_la_CPPFLAGS = $(DAP_CFLAGS) $(AM_CPPFLAGS)
libdapreader_module_la_LDFLAGS = -avoid-version -module 
libdapreader_module_la_LIBADD = $(DAP_CLIENT_LIBS)		\
	$(abs_top_builddir)/dispatch/libbes_dispatch.la	\
	$(abs_top_builddir)/xmlcommand/libbes_xml_command.la	\
	$(abs_top_builddir)/ppt/libbes_ppt.la			\
	$(abs_top_builddir)/dispatch/libbes_dap.la		\
	$(OPENSSL_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = data

CLEANFILES = *~

