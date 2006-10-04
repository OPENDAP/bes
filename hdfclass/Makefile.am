# Automake file for libhdfclass
#
# 9/19/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -I$(top_srcdir)

#CXXFLAGS_DEBUG = -g3 -O0 -Wall -fno-default-inline -fno-defer-pop
#CXXFLAGS = @CXXFLAGS@ -Wall

noinst_LTLIBRARIES = libhdfclass.la

libhdfclass_la_SOURCES = annot.cc gri.cc hcerr.h hcutil.cc inst.cc vdata.cc\
	genvec.cc hcerr.cc hcstream.h hdfclass.h sds.cc vgroup.cc
libhdfclass_la_LDFLAGS = $(LDFLAGS) $(HDF4_LDFLAGS)
libhdfclass_la_LIBADD = $(HDF4_LIBS)

EXTRA_DIST = 

CLEANFILES = *~
