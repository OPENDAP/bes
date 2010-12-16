
# Automake file for csv-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libcsv_module.la

SUBDIRS = bes-testsuite

CSV_SRCS = \
		CSVModule.cc CSVRequestHandler.cc \
		CSV_Data.cc CSV_Header.cc CSV_Obj.cc CSV_Reader.cc \
		CSVDAS.cc CSVDDS.cc

CSV_HDRS = \
		CSVModule.h CSVRequestHandler.h \
		CSVResponseNames.h \
		CSVDAS.h CSVDDS.h CSV_Data.h CSV_Field.h \
		CSV_Header.h CSV_Obj.h CSV_Reader.h

libcsv_module_la_SOURCES = $(CSV_SRCS) $(CSV_HDRS)
libcsv_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libcsv_module_la_LDFLAGS = -avoid-version -module 
libcsv_module_la_LIBADD = $(BES_DAP_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = data COPYRIGHT COPYING csv.conf.in

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

csv.conf: csv.conf.in config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > csv.conf

###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts csv_module-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb csv_module-@PACKAGE_VERSION@.tar.gz
