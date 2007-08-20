
# Automake file for csv-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libcsv_module.la

nobase_sysconf_DATA = bes.conf

CSV_SRCS:=

CSV_HDRS:=

BES_SRCS = \
		CSVModule.cc CSVRequestHandler.cc \
		CSV_Data.cc CSV_Header.cc CSV_Obj.cc CSV_Reader.cc \
		CSVDAS.cc CSVDDS.cc

BES_HDRS = \
		CSVModule.h CSVRequestHandler.h \
		CSVResponseNames.h \
		config_csv.h \
		CSVDAS.h CSVDDS.h CSV_Data.h CSV_Field.h \
		CSV_Header.h CSV_Obj.h CSV_Reader.h

libcsv_module_la_SOURCES = $(CSV_SRCS) $(BES_SRCS) $(CSV_HDRS) $(BES_HDRS)
libcsv_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libcsv_module_la_LDFLAGS = -avoid-version -module 
libcsv_module_la_LIBADD = $(BES_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = COPYRIGHT COPYING csv_handler.spec

CLEANFILES = *~

bes.conf: bes.conf.in ../config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]pkglibdir[@]%${pkglibdir}%" \
		-e "s%[@]datadir[@]%${datadir}%" \
		-e "s%[@]BES_MODULE_DIR[@]%${BES_MODULE_DIR}%" \
		-e "s%[@]BES_HELP_DIR[@]%${BES_HELP_DIR}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > bes.conf
###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts csv_module-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb csv_module-@PACKAGE_VERSION@.tar.gz
