
# Automake file for www-interface
#
# 5/17/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign

# Set the module version here, in the spec file and in configure.ac
M_NAME=www-interface
M_VER=4.2.4

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap \
-I$(top_srcdir)/xmlcommand -I$(top_srcdir) $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

# These are not used by automak but are often useful for certain types of
# debugging.
CXXFLAGS_DEBUG = -g3 -O0 -Wall 

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libwww_module.la

libwww_module_la_SOURCES = $(WWW_SOURCES) $(BES_SOURCES)
libwww_module_la_LDFLAGS = -avoid-version -module 
libwww_module_la_LIBADD = $(LIBADD)

BUILT_SOURCES = javascript.h

WWW_SOURCES = WWWArray.cc WWWFloat64.cc WWWInt32.cc WWWSequence.cc	 \
	WWWUInt16.cc WWWByte.cc WWWGrid.cc WWWStr.cc WWWUInt32.cc	 \
	WWWFloat32.cc WWWInt16.cc WWWOutput.cc WWWStructure.cc WWWUrl.cc \
	WWWFloat32.h WWWInt16.h WWWSequence.h WWWUInt16.h WWWArray.h	 \
	WWWFloat64.h WWWInt32.h WWWStr.h WWWUInt32.h WWWByte.h WWWGrid.h \
	WWWOutput.h WWWStructure.h WWWUrl.h javascript.h		 \
	WWWOutputFactory.cc WWWOutputFactory.h get_html_form.cc		 \
	get_html_form.h  

BES_SOURCES = BESWWWModule.cc BESWWWResponseHandler.cc BESWWWTransmit.cc \
	BESXMLWWWGetCommand.cc BESWWWRequestHandler.cc			\
	BESWWW.h BESWWWModule.h BESWWWNames.h BESWWWRequestHandler.h	\
	BESWWWResponseHandler.h BESWWWTransmit.h BESXMLWWWGetCommand.h

EXTRA_DIST = js2h.pl www.js

javascript.h: www.js js2h.pl js2h.pl.in
	perl js2h.pl $< > $@

CLEANFILES = *~ javascript.h
