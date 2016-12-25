# Automake file for dap-handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

AM_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch
ACLOCAL_AMFLAGS = -I conf
AM_CXXFLAGS = 

# Set the module version here, in the spec file and in configure.ac
M_NAME=dmrpp_module
M_VER=0.4.0

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
# ./configure --disable-shared CXXFLAGS=...
# or ./configure --enable-developer --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = . unit-tests bes-testsuite

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libdmrpp_module.la

BES_SRCS = H4ByteStream.cc \
DmrppModule.cc DmrppRequestHandler.cc DmrppByte.cc DmrppArray.cc \
DmrppFloat32.cc DmrppFloat64.cc DmrppInt16.cc DmrppInt32.cc DmrppInt64.cc \
DmrppInt8.cc DmrppUInt16.cc DmrppUInt32.cc DmrppUInt64.cc DmrppStr.cc  \
DmrppStructure.cc DmrppUrl.cc DmrppD4Enum.cc DmrppD4Group.cc DmrppD4Opaque.cc \
DmrppD4Sequence.cc  DmrppTypeFactory.cc DmrppParserSax2.cc DmrppUtil.cc \
Odometer.cc 

BES_HDRS = DmrppCommon.h H4ByteStream.h \
DmrppModule.h DmrppRequestHandler.h DmrppByte.h \
DmrppArray.h DmrppFloat32.h DmrppFloat64.h DmrppInt16.h DmrppInt32.h \
DmrppInt64.h DmrppInt8.h DmrppUInt16.h DmrppUInt32.h DmrppUInt64.h \
DmrppStr.h DmrppStructure.h DmrppUrl.h DmrppD4Enum.h DmrppD4Group.h \
DmrppD4Opaque.h DmrppD4Sequence.h DmrppTypeFactory.h DmrppParserSax2.h \
DmrppUtil.h Odometer.h 

# hack. This is probably not needed and is currently not used, but
# I'm leaving it in for now. If there are run-time issues with this
# module, add '$(BES_LIBS) $(OPENSSL_LIBS)' to LIBADD below.
# jhrg 10/13/14 
BES_LIBS = $(top_builddir)/xmlcommand/libbes_xml_command.la \
$(top_builddir)/ppt/libbes_ppt.la \
$(top_builddir)/dispatch/libbes_dispatch.la

libdmrpp_module_la_SOURCES = $(BES_HDRS) $(BES_SRCS)
libdmrpp_module_la_CPPFLAGS = $(DAP_CFLAGS) $(AM_CPPFLAGS)
libdmrpp_module_la_LDFLAGS = -avoid-version -module 
libdmrpp_module_la_LIBADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) -ltest-types 

EXTRA_PROGRAMS = 

EXTRA_DIST = data dmrpp.conf.in

CLEANFILES = *~ dmrpp.conf

sample_datadir = $(datadir)/hyrax/data/dmrpp
sample_data_DATA = data/*

install-data-local: dmrpp.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/dmrpp.conf \
	    && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/dmrpp.conf $(DESTDIR)$(sysconfdir)/bes/modules/dmrpp.conf.$$currdate; \
	$(INSTALL_DATA) dmrpp.conf $(DESTDIR)$(sysconfdir)/bes/modules/dmrpp.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/modules/dmrpp.conf

dmrpp.conf: dmrpp.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > dmrpp.conf

# endian_convert: endian_convert.o
#	gcc -o endian_convert endian_convert.cc

distclean-local:

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libdmrpp_module_la_SOURCES)

