# Automake file for freeform-handler
#
# 9/14/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

SUBDIRS = FFND . tests

# Set the module version here, in the spec file and in configure.ac
M_NAME=freeform_handler
M_VER=3.10.9

# There are some defines which must be set when you build code which uses
# FreeForm.h. Add them here. 4/17/98 jhrg
AM_CPPFLAGS = -DFREEFORM  $(FF_CPPFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap \
-I$(top_srcdir)/modules/freeform_handler/FFND $(DAP_CFLAGS)
LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) FFND/libfreeform.la

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libff_module.la

libff_module_la_SOURCES = $(FFTYPE_SRC) $(HANDLER_SRC) $(FFTYPE_HDR) \
	$(HANDLER_HDR) $(SERVER_SRC) $(SERVER_HDR)

libff_module_la_LDFLAGS = -avoid-version -module 
libff_module_la_LIBADD = $(LIBADD) 

FFTYPE_SRC = FFArray.cc FFFloat64.cc FFInt32.cc FFStructure.cc	\
	FFUrl.cc FFByte.cc FFGrid.cc FFSequence.cc FFUInt16.cc	\
	FFFloat32.cc FFInt16.cc FFStr.cc FFUInt32.cc FFD4Sequence.cc


FFTYPE_HDR = FFArray.h FFFloat32.h FFInt16.h FFStr.h FFUInt32.h	\
	FFByte.h FFFloat64.h FFInt32.h FFStructure.h FFUrl.h	\
	FFGrid.h FFSequence.h FFUInt16.h FFD4Sequence.h

HANDLER_HDR = ff_ce_functions.h date_proc.h DODS_Date_Factory.h DODS_Date.h   \
	DODS_Date_Time_Factory.h DODS_Date_Time.h DODS_Decimal_Year_Factory.h \
	DODS_Decimal_Year.h DODS_EndDate_Time_Factory.h			      \
	DODS_EndDecimal_Year_Factory.h DODS_StartDate_Time_Factory.h	      \
	DODS_StartDecimal_Year_Factory.h DODS_Time_Factory.h DODS_Time.h      \
	FreeFormCPP.h read_ff.h util_ff.h config_ff.h DODS_EndDate_Factory.h  \
	DODS_EndTime_Factory.h DODS_StartDate_Factory.h			      \
	DODS_StartTime_Factory.h

HANDLER_SRC = util_ff.cc ffdds.cc ffdas.cc		\
	ff_ce_functions.cc date_proc.cc DODS_Date.cc		\
	DODS_Time.cc DODS_Time_Factory.cc DODS_Date_Factory.cc	\
	DODS_Date_Time.cc DODS_Date_Time_Factory.cc		\
	DODS_StartDate_Time_Factory.cc				\
	DODS_EndDate_Time_Factory.cc

SERVER_SRC = FFRequestHandler.cc FFModule.cc

SERVER_HDR = FFRequestHandler.h FFModule.h

EXTRA_DIST = data ff.conf.in

CLEANFILES = ff.conf

sample_datadir = $(datadir)/hyrax/data/ff
sample_data_DATA = data/1998-6-avhrr.dat data/avhrr.dat data/gsodock.dat data/gsodock.dat.das \
	data/1998-6-avhrr.fmt data/avhrr.fmt data/gsodock.fmt \
	data/1998-6-avhrr.dat.das data/avhrr.dat.das

moduledir = $(sysconfdir)/bes/modules
module_DATA = ff.conf

ff.conf: ff.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > ff.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libff_module_la_SOURCES)


