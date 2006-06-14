
# Automake file for hdf4-handler
#
# 9/19/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign check-news dejagnu

ACLOCAL_AMFLAGS = -I conf
AM_CXXFLAGS = -I$(top_srcdir)/hdfclass
AM_YFLAGS = -d -v
AM_LFLAGS = -8

SUBDIRS = hdfclass

bin_PROGRAMS = dap_hdf4_handler
noinst_PROGRAMS = eosdas_test

lib_LTLIBRARIES = libhdf4_handler.la
if OPENDAPSERVER
    lib_besdir=$(prefix)/lib/bes
    lib_bes_LTLIBRARIES = libhdf4_module.la
endif

HDFTYPE_SRC = HDFArray.cc HDFGrid.cc HDFSequence.cc   HDFUInt32.cc \
	HDFByte.cc HDFInt16.cc HDFStr.cc HDFUrl.cc HDFFloat32.cc \
	HDFInt32.cc HDFStructure.cc HDFFloat64.cc HDFUInt16.cc \
	HDFTypeFactory.cc

HDFTYPE_HDR = HDFArray.h HDFFloat32.h HDFInt16.h HDFSequence.h HDFUInt16.h \
	HDFByte.h HDFFloat64.h HDFInt32.h HDFStr.h HDFUInt32.h \
	HDFGrid.h HDFStructure.h HDFUrl.h HDFTypeFactory.h

HANDLER_SRC = dhdferr.cc hdfdesc.cc funcs.cc hc2dap.cc hdfutil.cc \
	dhdferr.h dodsutil.h hdfutil.h ReadTagRef.h \
	hdf-dods.h hdf-maps.h config_hdf.h \
	hdfeos.tab.cc hdfeos.tab.h lex.hdfeos.cc

SERVER_SRC = HDF4RequestHandler.cc HDF4Module.cc

SERVER_HDR = HDF4RequestHandler.h HDF4Module.h

libhdf4_handler_la_SOURCES = $(HDFTYPE_SRC) $(HANDLER_SRC) $(HDFTYPE_HDR) 

libhdf4_module_la_SOURCES = $(HDFTYPE_SRC) $(HANDLER_SRC) $(SERVER_SRC) $(HDFTYPE_HDR) $(SERVER_HDR)
libhdf4_module_la_CPPFLAGS = $(CPPFLAGS) $(BES_CPPFLAGS)
libhdf4_module_la_LDFLAGS = $(BES_LDFLAGS)

dap_hdf4_handler_SOURCES = hdf_handler.cc
dap_hdf4_handler_LDADD = libhdf4_handler.la hdfclass/libhdfclass.a

eosdas_test_SOURCES = eosdas-test.cc
eosdas_test_LDADD = libhdf4_handler.la hdfclass/libhdfclass.a

EXTRA_DIST = hdf-testsuite hdf-testfiles hdfeos.y hdfeos.lex COPYRIGHT_URI \
	hdf4_handler.spec

DEJATOOL = dap_hdf4_handler
RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir hdf-testsuite

CLEANFILES = *~ *.log *.sum

###########################################################################

# Build linux RPMs

srpm:
	rpmbuild -ts hdf4_handler-@PACKAGE_VERSION@.tar.gz

rpm:
	rpmbuild -tb hdf4_handler-@PACKAGE_VERSION@.tar.gz
	
###########################################################################

# Build the HDF-EOS parser

hdfeos.tab.cc hdfeos.tab.h: hdfeos.y
	$(YACC) $(YFLAGS) $(AM_YFLAGS) -p hdfeos $<
	mv hdfeos.tab.c hdfeos.tab.cc

# Build the HDF-EOS scanner

lex.hdfeos.cc: hdfeos.lex hdfeos.tab.cc hdfeos.tab.h
	$(LEX) $(LFLAGS) $(AM_LFLAGS) -Phdfeos $<
	mv lex.hdfeos.c lex.hdfeos.cc


