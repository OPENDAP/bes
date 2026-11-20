# CppUnit tests for the GeotiffToDmr tool (../GeotiffToDmr.cc).
#
# The tests link the tool's object file, ../GeotiffToDmr-GeotiffToDmr.o
# (automake names it after the program because the program has its own
# CPPFLAGS), so they test exactly the code that is installed. That object is
# built with -DGEOTIFFTODMR_WITH_GDAL, so the tests link GDAL too.

AUTOMAKE_OPTIONS = foreign

# Same value as in ../Makefile.am. lib_besdir is not an autoconf substitution,
# so it must be defined here too: the tests compare against the default that
# was compiled into ../GeotiffToDmr-GeotiffToDmr.o.
lib_besdir=$(libdir)/bes

AM_CPPFLAGS = -I$(top_srcdir)/modules/gdal_module -I$(top_srcdir)/modules/common \
-I$(top_srcdir)/dispatch $(DAP_CFLAGS) \
-DGEOTIFFTODMR_DEFAULT_BES_LIB_DIR=\"$(lib_besdir)\"

LIBADD = $(BES_DISPATCH_LIB) $(GDAL_PROG_LDFLAGS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LIBADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror

AM_CXXFLAGS=

include $(top_srcdir)/coverage.mk

DISTCLEANFILES = test_config.h *.Po

CLEANFILES = *.dbg *.log

EXTRA_DIST = test_config.h.in

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

BUILT_SOURCES = test_config.h

noinst_HEADERS = test_config.h

# This way of building the header ensures it's in the build dir and that there
# are no '../' sequences in the paths.
test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = GeotiffToDmrTest
else
UNIT_TESTS =

check-local:
	@echo ""
	@echo "**********************************************************"
	@echo "You must have cppunit 1.12.x or greater installed to run *"
	@echo "check target in unit-tests directory                     *"
	@echo "**********************************************************"
	@echo ""
endif

OBJS = ../GeotiffToDmr-GeotiffToDmr.o

GeotiffToDmrTest_SOURCES = GeotiffToDmrTest.cc
GeotiffToDmrTest_LDADD = $(OBJS) $(LIBADD)
