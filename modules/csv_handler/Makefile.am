
# Automake file for csv_handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=csv_handler
M_VER=1.1.8

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = $(BES_CPPFLAGS)
LIBADD = $(BES_DAP_LIBS)
endif

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libcsv_module.la

SUBDIRS = . tests

CSV_SRCS = \
		CSVModule.cc CSVRequestHandler.cc			\
		CSV_Data.cc CSV_Header.cc CSV_Obj.cc CSV_Reader.cc	\
		CSVDAS.cc CSVDDS.cc CSV_Utils.cc

CSV_HDRS = \
		CSVModule.h CSVRequestHandler.h				\
		CSVDAS.h CSVDDS.h CSV_Data.h CSV_Field.h		\
		CSV_Header.h CSV_Obj.h CSV_Reader.h CSV_Utils.h

libcsv_module_la_SOURCES = $(CSV_SRCS) $(CSV_HDRS)
libcsv_module_la_LDFLAGS = -avoid-version -module 
libcsv_module_la_LIBADD = $(LIBADD)

EXTRA_DIST = data csv.conf.in 

CLEANFILES = *~ csv.conf

sample_datadir = $(datadir)/hyrax/data/csv
sample_data_DATA = data/temperature.csv

install-data-local: csv.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/csv.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/csv.conf $(DESTDIR)$(sysconfdir)/bes/modules/csv.conf.$$currdate; \
	$(INSTALL_DATA) csv.conf $(DESTDIR)$(sysconfdir)/bes/modules/csv.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/csv.conf

csv.conf: csv.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > csv.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libgateway_module_la_SOURCES)

