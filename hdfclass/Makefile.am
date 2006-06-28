# Automake file for libhdfclass
#
# 9/19/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -I$(top_srcdir)

#CXXFLAGS_DEBUG = -g3 -O0 -Wall -fno-default-inline -fno-defer-pop
#CXXFLAGS = @CXXFLAGS@ -Wall

lib_LTLIBRARIES = libhdfclass.la

libhdfclass_la_SOURCES = annot.cc gri.cc hcerr.h hcutil.cc inst.cc vdata.cc\
	genvec.cc hcerr.cc hcstream.h hdfclass.h sds.cc vgroup.cc

EXTRA_DIST = 

CLEANFILES = *~
