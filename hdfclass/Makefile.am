# Automake file for libhdfclass
#
# 9/19/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -I$(top_srcdir)
AM_CXXFLAGS = 
#CXXFLAGS_DEBUG = -g3 -O0 -Wall -fno-default-inline 
#CXXFLAGS = @CXXFLAGS@ -Wall

noinst_LTLIBRARIES = libhdfclass.la

libhdfclass_la_SOURCES = annot.cc gri.cc hcerr.h hcutil.cc inst.cc vdata.cc\
	genvec.cc hcerr.cc hcstream.h hdfclass.h sds.cc vgroup.cc
libhdfclass_la_CPPFLAGS = $(AM_CPPFLAGS) $(BES_CPPFLAGS)
libhdfclass_la_LDFLAGS = $(LDFLAGS) $(HDF4_LDFLAGS)
libhdfclass_la_LIBADD =  $(BES_DAP_LIBS)
# $(HDF4_LIBS) If HDF4 is built with --with-picm then adding the HDF4_LIBS
# breaks the link here, and vice versa, without pic, you need the libs (I 
# think) jhrg
EXTRA_DIST = 

CLEANFILES = *~
