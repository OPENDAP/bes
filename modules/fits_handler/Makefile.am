
# Automake file for fits_handler
#
# 5/17/2005 jhrg
#
# $Id: Makefile.am 12972 2006-01-05 15:41:07Z pwest $

AUTOMAKE_OPTIONS = foreign check-news

ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=fits_handler
M_VER=1.0.30

AM_CPPFLAGS = $(CFITS_CPPFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(CFITS_LDFLAGS) $(CFITS_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libfits_module.la

FITSSRCS:=fits_read_attributes.cc fits_read_descriptors.cc
# BESAutoPtr.cc

FITSHDRS:=fits_read_attributes.h fits_read_descriptors.h BESAutoPtr.h 
# config.h

BES_HANDLER_SRC = FitsRequestHandler.cc FitsModule.cc \
		  FitsRequestHandler.h FitsModule.h

libfits_module_la_SOURCES = $(FITSSRCS) $(FITSHDRS) $(BES_HANDLER_SRC)
# libfits_module_la_CPPFLAGS = $(BES_CPPFLAGS)
libfits_module_la_LDFLAGS = -avoid-version -module 
libfits_module_la_LIBADD = $(LIBADD)

EXTRA_DIST = data fits.conf.in

CLEANFILES = *~ fits.conf

sample_datadir = $(datadir)/hyrax/data/fits
sample_data_DATA = data/20060308.172859.mk4.cpb.fts data/20060308.172859.mk4.rpb.fts data/20060328.185927.chp.bsh.fts data/20060328.190602.dpm.alr.fts

moduledir = $(sysconfdir)/bes/modules
module_DATA = fits.conf

fits.conf: fits.conf.in $(top_builddir)/config.status
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > fits.conf


