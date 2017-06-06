# Tests

AUTOMAKE_OPTIONS = foreign

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap \
-I$(top_srcdir)/modules/w10n_handler $(DAP_CFLAGS)
LIBADD = $(BES_DISPATCH_LIB) $(BES_EXTRA_LIBS) $(DAP_SERVER_LIBS)
else
AM_CPPFLAGS = -I$(top_srcdir) $(BES_CPPFLAGS)
LIBADD = $(BES_DAP_LIBS)
endif

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LIBADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

DISTCLEANFILES = test_config.h

CLEANFILES = *.dbg *.log tmp/*

EXTRA_DIST = test_config.h.in baselines tmp testdir

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = W10nTest
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

OBJS = ../w10n_utils.o

W10nTest_SOURCES = W10nTest.cc
W10nTest_LDADD = $(OBJS) $(LIBADD)

noinst_HEADERS = test_config.h
