
# Automake file for usage
#
# 5/17/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news
ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=usage
M_VER=4.2.7

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(BES_CPPFLAGS) $(DAP_CFLAGS)
LIBADD = $(BES_DAP_LIBS)
endif

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libusage_module.la
besdir=$(datadir)/bes

dist_bes_DATA = usage_help.html usage_help.txt

USAGE_SOURCES = usage.cc usage.h 

BES_SOURCES = BESUsageModule.h BESUsageNames.h BESUsageResponseHandler.h \
	  BESUsageTransmit.h BESUsageModule.cc BESUsageResponseHandler.cc \
	  BESUsageTransmit.cc BESUsage.h BESUsageRequestHandler.cc \
	  BESUsageRequestHandler.h

libusage_module_la_SOURCES = $(USAGE_SOURCES) $(BES_SOURCES)
# libusage_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libusage_module_la_LDFLAGS = -avoid-version -module 
libusage_module_la_LIBADD = $(LIBADD) # $(BES_DAP_LIBS)

EXTRA_DIST = usage.conf.in 

DISTCLEANFILES = usage.conf

install-data-local: usage.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/usage.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/usage.conf $(DESTDIR)$(sysconfdir)/bes/modules/usage.conf.$$currdate; \
	$(INSTALL_DATA) usage.conf $(DESTDIR)$(sysconfdir)/bes/modules/usage.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/usage.conf

usage.conf: usage.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > usage.conf

C4_DIR=./cccc
C4_USAGE=--html_outfile=cccc-usage.html --xml_outfile=cccc-usage.xml --db_outfile=cccc-usage.db

.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(C4_USAGE) $(libusage_module_la_SOURCES)

