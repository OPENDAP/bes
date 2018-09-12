
# Automake file for sample-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/xmlcommand $(XML2_CFLAGS)
AM_CXXFLAGS =

CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

if BES_DEVELOPER
AM_CXXFLAGS += $(CXXFLAGS_DEBUG)
endif

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libsample_module.la

SUBDIRS = . tests

BES_SRCS = SampleSayResponseHandler.cc SampleSayXMLCommand.cc SampleModule.cc SampleRequestHandler.cc

BES_HDRS = SampleSayResponseHandler.h SampleSayXMLCommand.h SampleModule.h SampleRequestHandler.h \
SampleResponseNames.h

libsample_module_la_SOURCES = $(BES_SRCS) $(BES_HDRS)
libsample_module_la_LDFLAGS = -avoid-version -module 
libsample_module_la_LIBADD = $(BES_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = sample.conf.in

CLEANFILES = *~ sample.conf

moduledir = $(DESTDIR)$(sysconfdir)/bes/modules
module_DATA =  sample.conf

sample.conf: sample.conf.in $(top_builddir)/config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
                -e "s%[@]datarootdir[@]%${datarootdir}%" \
                -e "s%[@]datadir[@]%${datadir}%" \
                -e "s%[@]bindir[@]%${bindir}%" \
                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > sample.conf

