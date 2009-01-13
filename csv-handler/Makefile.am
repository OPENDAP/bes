
# Automake file for csv-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libcsv_module.la
dist_bin_SCRIPTS = bes-csv-data.sh

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
libcsv_module_la_LIBADD = $(BES_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = data COPYRIGHT COPYING csv_handler.spec bes-csv-data.sh.in

CLEANFILES = *~ bes-csv-data.sh

sample_datadir = $(datadir)/hyrax/data/csv
sample_data_DATA = data/temperature.csv

do_subst = sed -e 's,[@]sysconfdir[@],$(sysconfdir),g' \
		   	   -e 's,[@]libdir[@],$(libdir),g' \
		   	   -e 's,[@]datarootdir[@],$(datarootdir),g' \
		   	   -e 's,[@]datadir[@],$(datadir),g'

bes-csv-data.sh: bes-csv-data.sh.in config.status
	$(do_subst) < $(srcdir)/bes-csv-data.sh.in > bes-csv-data.sh
	chmod +x bes-csv-data.sh

# This makes sure that the handler modifies the bes which configure found
# even when distcheck is run.
bes-conf: bes-csv-data.sh
	(bes_prefix=`bes-config --prefix` ; \
	if test "$$bes_prefix" = "/usr" ; \
	 then \
	    ./bes-csv-data.sh /etc/bes/bes.conf /usr/lib/bes ;\
	else \
	    ./bes-csv-data.sh $$bes_prefix/etc/bes/bes.conf $$bes_prefix/lib/bes ; \
	fi)

###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts csv_module-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb csv_module-@PACKAGE_VERSION@.tar.gz
