
SUBDIRS = 

# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/functions $(GF_CFLAGS) $(XML2_CFLAGS) $(DAP_CFLAGS)
AM_LDADD =  $(GF_LIBS) -L.. -lfunctions_module  $(top_builddir)/dispatch/libbes_dispatch.la
# ../libfunctions_module.la

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
endif

if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --quiet --trace-children=yes --error-exitcode=1  --dsymutil=yes --leak-check=yes
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

noinst_HEADERS = test_config.h

DIRS_EXTRA = 

EXTRA_DIST = test_config.h.in ce-functions-testsuite

CLEANFILES = testout .dodsrc  *.gcda *.gcno

DISTCLEANFILES = test_config.h *.strm *.file tmp.txt

# TODO Check if this sed command to filter out the ../ from the value of 
# ${abs_srcdir} is really needed for the distcheck target. jhrg 5/16/13
test_config.h: test_config.h.in Makefile
	@my_topdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_srcdir[@]%$${my_topdir}%" $< > test_config.h


############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = CEFunctionsTest GridGeoConstraintTest Dap4_CEFunctionsTest
	
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

CEFunctionsTest_SOURCES = CEFunctionsTest.cc
CEFunctionsTest_LDADD = $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

Dap4_CEFunctionsTest_SOURCES = Dap4_CEFunctionsTest.cc
Dap4_CEFunctionsTest_LDADD = $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

GridGeoConstraintTest_SOURCES = GridGeoConstraintTest.cc
GridGeoConstraintTest_LDADD = $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

