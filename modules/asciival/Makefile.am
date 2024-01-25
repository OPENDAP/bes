
# Automake file for asciival
#
# 5/17/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign

# Set the module version here, in the spec file and in configure.ac
M_NAME=asciival
M_VER=4.2.5

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir) $(DAP_CFLAGS)
LIBADD = $(BES_DAP_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

# These are not used by automake but are often useful for certain types of
# debugging.
CXXFLAGS_DEBUG = -g3 -O0 -Wall 

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libascii_module.la

libascii_module_la_SOURCES = $(ASCII_SOURCES) $(BES_SOURCES)
libascii_module_la_LDFLAGS = -avoid-version -module 
libascii_module_la_LIBADD = $(LIBADD)

CLEANFILES = *~ *.log *.sum

EXTRA_DIST = ascii.conf.in

DISTCLEANFILES = ascii.conf

moduledir = $(sysconfdir)/bes/modules
module_DATA = ascii.conf

ascii.conf: ascii.conf.in $(top_builddir)/config.status

# Build the module's bes.conf file
%.conf : %.conf.in
	sed -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > $@


ASCII_SOURCES = AsciiArray.cc AsciiInt16.cc AsciiStructure.cc		   \
		AsciiInt32.cc AsciiUInt16.cc AsciiByte.cc AsciiOutput.cc   \
		AsciiUInt32.cc AsciiFloat32.cc AsciiUrl.cc AsciiFloat64.cc \
		AsciiSequence.cc  AsciiGrid.cc AsciiStr.cc AsciiArray.h	   \
		AsciiInt32.h AsciiUInt32.h AsciiByte.h AsciiOutput.h	   \
		AsciiUrl.h AsciiFloat32.h AsciiSequence.h AsciiFloat64.h   \
		AsciiStr.h AsciiGrid.h AsciiStructure.h AsciiInt16.h	   \
		AsciiUInt16.h AsciiOutputFactory.cc AsciiOutputFactory.h   \
		get_ascii.cc get_ascii.h get_ascii_dap4.cc get_ascii_dap4.h

BES_SOURCES = BESAsciiModule.cc BESAsciiTransmit.cc BESAsciiRequestHandler.cc \
	    BESAsciiModule.h BESAsciiTransmit.h BESAsciiRequestHandler.h BESAsciiNames.h
