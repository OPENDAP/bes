
SUBDIRS = 

# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir) $(GF_CFLAGS) $(XML2_CFLAGS) $(DAP_CFLAGS)
AM_LDADD =  $(GF_LIBS) ../libfunctions_module.la  $(top_srcdir)/dispatch/libbes_dispatch.la
if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
endif


if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --quiet --trace-children=yes --error-exitcode=1  --dsymutil=yes --leak-check=yes
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
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

test_config.h: test_config.h.in Makefile
	sed -e "s%[@]abs_srcdir[@]%${abs_srcdir}%" $< > test_config.h

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = ArrayGeoConstraintTest CEFunctionsTest GridGeoConstraintTest
	
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

TEST_SRC = testFile.cc testFile.h


ArrayGeoConstraintTest_SOURCES = ArrayGeoConstraintTest.cc
ArrayGeoConstraintTest_LDADD =  $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) -ltest-types

CEFunctionsTest_SOURCES = CEFunctionsTest.cc
CEFunctionsTest_LDADD = $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

GridGeoConstraintTest_SOURCES = GridGeoConstraintTest.cc
GridGeoConstraintTest_LDADD = $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

