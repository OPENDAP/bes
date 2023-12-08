## Makefile.am -- Process this file with automake to produce Makefile.in
##
##   Copyright (C) 2010-2016 The HDF Group
##
##   Author: Hyo-Kyung Lee
##
##   Date: July 26, 2016
##   Note: added flex/bison cals.
## 
##   Date: April 24, 2011
##   Note: added LDFLAGS for ssl.
##
##   Date: March 18, 2011
##   Note: renamed hmap to h4mapwriter.
##
##  This file is part of h4mapwriter tool.
AUTOMAKE_OPTIONS = foreign check-news
AM_CC = h4cc
# Remove -Wall flag for Solaris cc compiler.
AM_CFLAGS =  -Wall
AM_LDFLAGS = -L/opt/homebrew/Cellar/openssl@3/3.1.0/lib -lssl -lcrypto \
-L/Users/jimg/src/opendap/hyrax_git/build/deps/lib -ldf -lmfhdf
# get rid of -luuid for OSX
# -luuid
bin_PROGRAMS = h4mapwriter
GRAM_SRC = odl.tab.c odl.tab.h lex.odl.c
YACC = bison
LEX = flex
h4mapwriter_SOURCES = $(GRAM_SRC) \
	       hmap.c \
               xml_hmap.c \
               xml_sds.c xml_vdata.c xml_ris.c xml_an.c xml_vg.c xml_pal.c \
               h4.c h4_ris.c h4_sds.c  h4_vdata.c h4_pal.c \
               h4.h hmap.h  xml.h xml_sds.h xml_vg.h 
SUBDIRS = . 
# testsuite
DIST_SUBDIRS = . 
# testsuite
man_MANS = h4mapwriter.1
dist_man_MANS = h4mapwriter.1
EXTRA_DIST = h4mapwriter.spec odl.y odl.lex


# Build linux RPMs
srpm: dist
	rpmbuild -ts --clean @PACKAGE@-@PACKAGE_VERSION@.tar.gz

rpm: dist
	rpmbuild -tb --clean @PACKAGE@-@PACKAGE_VERSION@.tar.gz


###########################################################################
# Build the ODL parser

odl.tab.c odl.tab.h: odl.y
	$(YACC) $(YFLAGS) $<

# Build the ODL scanner

lex.odl.c lex.odl.h: odl.lex odl.tab.c odl.tab.h
	$(LEX) $(LFLAGS) $<
