
# Automake file for OPENDAP_TYPE-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

lib_LTLIBRARIES = libOPENDAP_TYPE_module.la

OPENDAP_CLASS_SRCS:=

OPENDAP_CLASS_HDRS:=

BES_SRCS = \
		OPENDAP_CLASSModule.cc OPENDAP_CLASSRequestHandler.cc

BES_HDRS = \
		OPENDAP_CLASSModule.h OPENDAP_CLASSRequestHandler.h \
		OPENDAP_CLASSResponseNames.h \
		config_OPENDAP_TYPE.h

libOPENDAP_TYPE_module_la_SOURCES = $(OPENDAP_CLASS_SRCS) $(BES_SRCS) $(OPENDAP_CLASS_HDRS) $(BES_HDRS)
libOPENDAP_TYPE_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libOPENDAP_TYPE_module_la_LDFLAGS = -avoid-version -module 
libOPENDAP_TYPE_module_la_LIBADD = $(BES_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = COPYRIGHT COPYING OPENDAP_TYPE_handler.spec

CLEANFILES = *~

###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts OPENDAP_TYPE_module-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb OPENDAP_TYPE_module-@PACKAGE_VERSION@.tar.gz
