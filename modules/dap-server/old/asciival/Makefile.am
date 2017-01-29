
# Automake file for asciival
#
# 5/17/2005 jhrg
#
# $Id$

AUTOMAKE_OPTIONS = foreign

# Set the module version here, in the spec file and in configure.ac
M_NAME=dap-server/ascii
M_VER=4.1.5

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir) $(DAP_CFLAGS)
LIBADD = $(BES_DAP_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = -I$(top_srcdir) $(BES_CPPFLAGS)
LIBADD = $(BES_DAP_LIBS)
endif

AM_CPPFLAGS += -DMODULE_NAME=\"$(M_NAME)\" -DMODULE_VERSION=\"$(M_VER)\"

# These are not used by automake but are often useful for certain types of
# debugging.
CXXFLAGS_DEBUG = -g3 -O0 -Wall 
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = . unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libascii_module.la

libascii_module_la_SOURCES = $(ASCII_SOURCES) $(BES_SOURCES)
libascii_module_la_LDFLAGS = -avoid-version -module 
libascii_module_la_LIBADD = $(LIBADD)

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

# BESAsciiResponseHandler.cc BESAsciiResponseHandler.h
EXTRA_DIST = 

CLEANFILES = *~ *.log *.sum

###########################################################################

# Fortify targets.

.PHONY: fortify
fortify:
	sourceanalyzer -b @PACKAGE@ $(MAKE) libascii_module.la
	sourceanalyzer -b @PACKAGE@ -scan -f ../@PACKAGE@-@PACKAGE_VERSION@.fpr

# Use this to clean the fortify project.
.PHONY: fortifyclean
fortifyclean:
	sourceanalyzer -b @PACKAGE@ -clean

