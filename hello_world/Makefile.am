
# Automake file for say-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

lib_LTLIBRARIES = libsay_module.la

Say_SRCS:=

Say_HDRS:=

BES_SRCS = \
	SayResponseHandler.cc \
	SayModule.cc SayRequestHandler.cc	\
	SayCommand.cc SayReporter.cc

BES_HDRS = \
	SayResponseHandler.h \
	SayModule.h SayRequestHandler.h \
	SayCommand.h \
	SayResponseNames.h \
	SayReporter.h \
	config_say.h

libsay_module_la_SOURCES = $(Say_SRCS) $(BES_SRCS) $(Say_HDRS) $(BES_HDRS)
libsay_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libsay_module_la_LDFLAGS = -avoid-version -module 
libsay_module_la_LIBADD = $(BES_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = COPYRIGHT COPYING say_handler.spec

CLEANFILES = *~

###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts say_module-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb say_module-@PACKAGE_VERSION@.tar.gz
