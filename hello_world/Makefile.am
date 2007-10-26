
# Automake file for sample-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

lib_LTLIBRARIES = libsample_module.la

Sample_SRCS:=

Sample_HDRS:=

BES_SRCS = \
	SampleSayResponseHandler.cc SampleSayCommand.cc \
		SampleModule.cc SampleRequestHandler.cc \
		SayReporter.cc

BES_HDRS = \
	SampleSayResponseHandler.h SampleSayCommand.h \
		SampleModule.h SampleRequestHandler.h \
		SampleResponseNames.h \
		SayReporter.h \
		config_sample.h

libsample_module_la_SOURCES = $(Sample_SRCS) $(BES_SRCS) $(Sample_HDRS) $(BES_HDRS)
libsample_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libsample_module_la_LDFLAGS = -avoid-version -module 
libsample_module_la_LIBADD = $(BES_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = COPYRIGHT COPYING sample_handler.spec

CLEANFILES = *~

###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts sample_module-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb sample_module-@PACKAGE_VERSION@.tar.gz
