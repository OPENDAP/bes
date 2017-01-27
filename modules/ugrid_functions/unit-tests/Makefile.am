
SUBDIRS = 

# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/modules/ugrid_functions $(GF_CFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(GF_LIBS) $(BES_DISPATCH_LIB) $(BES_EXTRA_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = -I$(top_srcdir) $(GF_CFLAGS) $(XML2_CFLAGS)
LIBADD = $(GF_LIBS) 
endif

# AM_CPPFLAGS = -I$(top_srcdir) $(GF_CFLAGS) $(XML2_CFLAGS)
if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LIBADD += $(CPPUNIT_LIBS)
endif

if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --quiet --trace-children=yes --error-exitcode=1  --dsymutil=yes --leak-check=yes
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

noinst_HEADERS = test_config.h

DIRS_EXTRA = 

EXTRA_DIST = 

CLEANFILES = testout .dodsrc  *.gcda *.gcno

DISTCLEANFILES = test_config.h *.strm *.file tmp.txt

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = NDimArrayTest BindTest possibly_lost GFTests
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

NDimArrayTest_SOURCES =  NDimArrayTest.cc
NDimArrayTest_LDADD = ../NDimensionalArray.o $(LIBADD)

BindTest_SOURCES = BindTest.cc
BindTest_LDADD = $(LIBADD)

GFTests_SOURCES = GFTests.cc
GFTests_LDADD = $(LIBADD)

possibly_lost_SOURCES = possibly_lost.cc
possibly_lost_LDADD = $(LIBADD)

