
# Automake file for OPENDAP_TYPE-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

bin_PROGRAMS = opendapserver OPENDAP_TYPE_handler

OPENDAP_CLASS_SRCS:=

OPENDAP_CLASS_HDRS:=

SRVR_HANDLER_SRC = \
		OPENDAP_TYPE_module.cc \
		OPENDAP_CLASSRequestHandler.cc OPENDAP_CLASSRequestHandler.h \
		OPENDAP_CLASSResponseNames.h config_OPENDAP_TYPE.h

MODULE_DIR = $(BES_MODULE_DIR)

opendapserver_MODULES = $(MODULE_DIR)/OPeNDAPServerHandler.o \
			$(MODULE_DIR)/ServerApp.o

opendapserver_SOURCES = $(OPENDAP_CLASS_SRCS) $(SRVR_HANDLER_SRC) $(OPENDAP_CLASS_HDRS)
opendapserver_CPPFLAGS = $(BES_CPPFLAGS)
opendapserver_LDFLAGS = $(BES_LDFLAGS) 
opendapserver_LDADD = $(BES_LDADD) $(opendapserver_MODULES)

BES_HANDLER_SRC = \
		OPENDAP_TYPE_handler.cc \
		OPENDAP_TYPE_module.cc \
		OPENDAP_CLASSRequestHandler.cc OPENDAP_CLASSRequestHandler.h \
		OPENDAP_CLASSHandlerApp.cc OPENDAP_CLASSHandlerApp.h \
		OPENDAP_CLASSResponseNames.h config_OPENDAP_TYPE.h

OPENDAP_TYPE_handler_SOURCES = $(OPENDAP_CLASS_SRCS) $(BES_HANDLER_SRC) $(OPENDAP_CLASS_HDRS)
OPENDAP_TYPE_handler_CPPFLAGS = $(BES_CPPFLAGS)
OPENDAP_TYPE_handler_LDFLAGS = $(BES_LDFLAGS) 
OPENDAP_TYPE_handler_LDADD = $(BES_LDADD)

EXTRA_PROGRAMS = 

EXTRA_DIST = COPYRIGHT COPYING OPENDAP_TYPE_handler.spec

CLEANFILES = *~

###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts cdf_handler-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb cdf_handler-@PACKAGE_VERSION@.tar.gz
