
# Automake file for hdf4-handler
#
# 9/19/2005 jhrg
#
# $Id: Makefile.am 12175 2005-09-15 20:22:36Z jimg $

AUTOMAKE_OPTIONS = foreign check-news dejagnu

ACLOCAL_AMFLAGS = -I conf
AM_CPPFLAGS = -I$(top_srcdir)/hdfclass
AM_YFLAGS = -d -v
AM_LFLAGS = -8

SUBDIRS = hdfclass

bin_PROGRAMS = dap_hdf4_handler
noinst_PROGRAMS = eosdas_test

HDFTYPE_SRC = HDFArray.cc HDFGrid.cc HDFSequence.cc   HDFUInt32.cc \
	HDFByte.cc HDFInt16.cc HDFStr.cc HDFUrl.cc HDFFloat32.cc \
	HDFInt32.cc HDFStructure.cc HDFFloat64.cc HDFUInt16.cc \
	HDFTypeFactory.cc

HDFTYPE_HDR = HDFArray.h HDFFloat32.h HDFInt16.h HDFSequence.h HDFUInt16.h \
	HDFByte.h HDFFloat64.h HDFInt32.h HDFStr.h HDFUInt32.h \
	HDFGrid.h HDFStructure.h HDFUrl.h HDFTypeFactory.h


HANDLER_SRC = dhdferr.cc hdfdesc.cc funcs.cc hc2dap.cc hdfutil.cc \
	hdf_handler.cc dhdferr.h dodsutil.h hdfutil.h ReadTagRef.h \
	hdf-dods.h hdf-maps.h hdfeos.tab.cc hdfeos.tab.h \
	lex.hdfeos.cc config_hdf.h

dap_hdf4_handler_SOURCES = $(HDFTYPE_SRC) $(HANDLER_SRC) $(HDFTYPE_HDR)
dap_hdf4_handler_LDADD = hdfclass/libhdfclass.a

eosdas_test_SOURCES = eosdas-test.cc hdfeos.tab.cc hdfeos.tab.h \
	lex.hdfeos.cc
eosdas_test_LDADD = hdfclass/libhdfclass.a

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


