
# Automake file for OPENDAP_TYPE-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

bin_PROGRAMS = dap_OPENDAP_TYPE_handler
lib_LTLIBRARIES = libOPENDAP_TYPE_handler.la

OPENDAP_CLASS_SRCS:=

OPENDAP_CLASS_HDRS:=

BES_SRCS = \
		OPENDAP_CLASSModule.cc OPENDAP_CLASSRequestHandler.cc \
		OPENDAP_CLASSHandlerApp.cc

BES_HDRS = \
		OPENDAP_CLASSModule.h OPENDAP_CLASSRequestHandler.h \
		OPENDAP_CLASSHandlerApp.h OPENDAP_CLASSResponseNames.h \
		config_OPENDAP_TYPE.h

libOPENDAP_TYPE_handler_la_SOURCES = $(OPENDAP_CLASS_SRCS) $(BES_SRCS) $(OPENDAP_CLASS_HDRS) $(BES_HDRS)

dap_OPENDAP_TYPE_handler_SOURCES = OPENDAP_TYPE_handler.cc
dap_OPENDAP_TYPE_handler_LDADD = libOPENDAP_TYPE_handler.la

EXTRA_PROGRAMS = 

EXTRA_DIST = COPYRIGHT COPYING OPENDAP_TYPE_handler.spec

CLEANFILES = *~

###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts cdf_handler-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb cdf_handler-@PACKAGE_VERSION@.tar.gz
