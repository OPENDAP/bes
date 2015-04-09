
# Tests

AUTOMAKE_OPTIONS = foreign subdir-objects

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/functions $(GF_CFLAGS) $(XML2_CFLAGS) $(DAP_CFLAGS)
AM_LDADD =  $(GF_LIBS) -L.. -lfunctions_module  $(top_builddir)/dispatch/libbes_dispatch.la
# ../libfunctions_module.la

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
endif

if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --quiet --trace-children=yes --error-exitcode=1 --dsymutil=yes --leak-check=yes
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

EXTRA_DIST = test_config.h.in ce-functions-testsuite tabular

CLEANFILES = testout .dodsrc  *.gcda *.gcno

# I added '*.po' because there are dependencies on ../*.o files and
# that seems to leave *.Po files here that distclean complains about.
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
UNIT_TESTS = CEFunctionsTest GridGeoConstraintTest Dap4_CEFunctionsTest \
TabularFunctionTest BBoxFunctionTest RoiFunctionTest BBoxUnionFunctionTest \
OdometerTest

# Dap4_TabularFunctionTest Removed since the DAP2 code has moved so far 
# in front of the DAP4 version, which has had virtually no testing.
# jhrg 3/12/15
	
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

# These are used to read in baselines from files
TEST_SRC = test_utils.cc test_utils.h 
TEST_OBJ = ../roi_utils.o

CEFunctionsTest_SOURCES = CEFunctionsTest.cc
CEFunctionsTest_LDADD = $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

Dap4_CEFunctionsTest_SOURCES = Dap4_CEFunctionsTest.cc
Dap4_CEFunctionsTest_LDADD = $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

GridGeoConstraintTest_SOURCES = GridGeoConstraintTest.cc
GridGeoConstraintTest_LDADD = $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

# If dist/distclean fails here, look at the ../ SOURCES and the value of subdir-objects.
# It may be that subdir-objects must be set up higher in the chain of Makefiles.
TabularFunctionTest_OBJ = ../TabularFunction.o
TabularFunctionTest_SOURCES = TabularFunctionTest.cc
TabularFunctionTest_LDADD = $(TabularFunctionTest_OBJ) $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

# Dap4_TabularFunctionTest_SOURCES = ../TabularFunction.cc ../TabularFunction.h Dap4_TabularFunctionTest.cc
# Dap4_TabularFunctionTest_LDADD = $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types
BBoxFunctionTest__OBJ = ../BBoxFunction.o
BBoxFunctionTest_SOURCES = BBoxFunctionTest.cc $(TEST_SRC)
BBoxFunctionTest_LDADD = $(BBoxFunctionTest__OBJ) $(TEST_OBJ) $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

BBoxUnionFunctionTest_OBJ = ../BBoxUnionFunction.o
BBoxUnionFunctionTest_SOURCES = BBoxUnionFunctionTest.cc $(TEST_SRC)
BBoxUnionFunctionTest_LDADD = $(BBoxUnionFunctionTest_OBJ) $(TEST_OBJ) $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

RoiFunctionTest_OBJ = ../RoiFunction.o
RoiFunctionTest_SOURCES = RoiFunctionTest.cc $(TEST_SRC)
RoiFunctionTest_LDADD = $(RoiFunctionTest_OBJ) $(TEST_OBJ) $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)  -ltest-types

OdometerTest_OBJ = ../Odometer.o
OdometerTest_SOURCES = OdometerTest.cc $(TEST_SRC)
OdometerTest_LDADD = $(OdometerTest_OBJ) $(TEST_OBJ) $(AM_LDADD) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
