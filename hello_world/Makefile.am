
# Automake file for sample-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libsample_module.la
dist_bin_SCRIPTS = bes-sample-data.sh

Sample_SRCS:=

Sample_HDRS:=

BES_SRCS = \
	SampleSayResponseHandler.cc SampleSayXMLCommand.cc \
		SampleModule.cc SampleRequestHandler.cc \
		SayReporter.cc

BES_HDRS = \
	SampleSayResponseHandler.h SampleSayXMLCommand.h \
		SampleModule.h SampleRequestHandler.h \
		SampleResponseNames.h \
		SayReporter.h \
		config_sample.h

libsample_module_la_SOURCES = $(Sample_SRCS) $(BES_SRCS) $(Sample_HDRS) $(BES_HDRS)
libsample_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libsample_module_la_LDFLAGS = -avoid-version -module 
libsample_module_la_LIBADD = $(BES_LIBS)

EXTRA_PROGRAMS = 

EXTRA_DIST = COPYRIGHT COPYING sample_handler.spec bes-sample-data.sh.in

CLEANFILES = *~ bes-sample-data.sh

do_subst = sed -e 's,[@]sysconfdir[@],$(sysconfdir),g' \
		   	   -e 's,[@]libdir[@],$(libdir),g' \
		   	   -e 's,[@]datarootdir[@],$(datarootdir),g' \
		   	   -e 's,[@]datadir[@],$(datadir),g'

bes-sample-data.sh: bes-sample-data.sh.in config.status
	$(do_subst) < $(srcdir)/bes-sample-data.sh.in > bes-sample-data.sh
	chmod +x bes-sample-data.sh

# This makes sure that the handler modifies the bes which configure found
# even when distcheck is run.
bes-conf: bes-sample-data.sh
	(bes_prefix=`bes-config --prefix` ; \
	if test "$$bes_prefix" = "/usr" ; \
	 then \
	    ./bes-sample-data.sh /etc/bes/bes.conf /usr/lib/bes ;\
	else \
	    ./bes-sample-data.sh $$bes_prefix/etc/bes/bes.conf $$bes_prefix/lib/bes ; \
	fi)

