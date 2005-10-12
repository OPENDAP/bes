# Automake file for libhdfclass
#
# 9/19/2005 jhrg
#
# $Id: Makefile.am 12175 2005-09-15 20:22:36Z jimg $

AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -I$(top_srcdir)

#CXXFLAGS_DEBUG = -g3 -O0 -Wall -fno-default-inline -fno-defer-pop
#CXXFLAGS = @CXXFLAGS@ -Wall

noinst_LIBRARIES = libhdfclass.a

libhdfclass_a_SOURCES = annot.cc gri.cc hcerr.h hcutil.cc inst.cc vdata.cc\
	genvec.cc hcerr.cc hcstream.h hdfclass.h sds.cc vgroup.cc

EXTRA_DIST = 

CLEANFILES = *~
