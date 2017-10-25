
# Automake file for netcdf-handler
#
# 5/17/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=netcdf_handler
M_VER=3.11.7

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(NC_CPPFLAGS) $(DAP_CFLAGS)
LIBADD = $(NC_LDFLAGS) $(NC_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

SUBDIRS = . tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libnc_module.la

libnc_module_la_SOURCES = $(NCTYPE_SRC) $(NCTYPE_HDR) $(SERVER_SRC) $(SERVER_HDR)
# libnc_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libnc_module_la_LDFLAGS = -avoid-version -module 
libnc_module_la_LIBADD = $(LIBADD)

NCTYPE_SRC = ncdas.cc ncdds.cc nc_util.cc \
	NCArray.cc NCByte.cc NCFloat64.cc NCGrid.cc NCUInt32.cc		\
	NCInt32.cc NCSequence.cc NCStr.cc NCStructure.cc NCUrl.cc	\
	NCUInt16.cc NCInt16.cc NCFloat32.cc

NCTYPE_HDR = NCFloat64.h NCArray.h NCGrid.h NCSequence.h NCUInt16.h 	\
	NCByte.h NCInt16.h NCStr.h NCUInt32.h NCFloat32.h NCInt32.h	\
	NCStructure.h NCUrl.h nc_util.h config_nc.h

SERVER_SRC = NCRequestHandler.cc NCModule.cc

SERVER_HDR = NCRequestHandler.h NCModule.h

EXTRA_DIST = data nc.conf.in

CLEANFILES = *~ nc.conf

sample_datadir = $(datadir)/hyrax/data/nc
sample_data_DATA = data/bears.nc data/bears.nc.das data/coads_climatology.nc \
data/fnoc1.nc data/fnoc1.das data/fnoc1.nc.html data/zero_length_array.nc

install-data-local: nc.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/nc.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/nc.conf $(DESTDIR)$(sysconfdir)/bes/modules/nc.conf.$$currdate; \
	$(INSTALL_DATA) nc.conf $(DESTDIR)$(sysconfdir)/bes/modules/nc.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/nc.conf

nc.conf: nc.conf.in $(top_srcdir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > nc.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libnc_module_la_SOURCES)


