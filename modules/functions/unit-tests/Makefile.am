
# Tests

AUTOMAKE_OPTIONS = foreign 

# Removed jhrg 9.22.17 in an attempt to fix distcheck's build
# subdir-objects

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/modules/functions \
$(DAP_CFLAGS)
# $(GDAL_CFLAGS)
AM_LDADD =  $(DAP_SERVER_LIBS) $(top_builddir)/dispatch/libbes_dispatch.la \
$(top_builddir)/modules/functions/.libs/libfunctions_module.a
# $(GF_LIBS)

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

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

noinst_HEADERS = test_config.h

EXTRA_DIST = test_config.h.in ce-functions-testsuite tabular scale

CLEANFILES = testout .dodsrc *.gcda *.gcno

# I added '*.po' because there are dependencies on ../*.o files and
# that seems to leave *.Po files here that distclean complains about.
DISTCLEANFILES = test_config.h *.strm *.file *.Po tmp.txt

test_config.h: test_config.h.in Makefile
	@my_topdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${my_topdir}%" $< > test_config.h


############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = CEFunctionsTest GridGeoConstraintTest Dap4_CEFunctionsTest \
TabularFunctionTest BBoxFunctionTest RoiFunctionTest BBoxUnionFunctionTest \
OdometerTest MaskArrayFunctionTest MakeMaskFunctionTest RangeFunctionTest \
BBoxCombFunctionTest

# See note about GDAL linking issues in ../Makefile.am. jhrg 6/11/22
# if BUILD_GDAL
# UNIT_TESTS += ScaleUtilTest ScaleUtilTest3D
# endif

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

CEFunctionsTest_SOURCES = CEFunctionsTest.cc  $(TEST_SRC)
CEFunctionsTest_LDADD = -ltest-types $(AM_LDADD)

Dap4_CEFunctionsTest_SOURCES = Dap4_CEFunctionsTest.cc
Dap4_CEFunctionsTest_LDADD = -ltest-types $(AM_LDADD)

GridGeoConstraintTest_SOURCES = GridGeoConstraintTest.cc 
GridGeoConstraintTest_LDADD = -ltest-types $(AM_LDADD)

TabularFunctionTest_SOURCES = TabularFunctionTest.cc 
TabularFunctionTest_LDADD = -ltest-types $(AM_LDADD)

BBoxFunctionTest_SOURCES = BBoxFunctionTest.cc $(TEST_SRC)
BBoxFunctionTest_LDADD = -ltest-types $(AM_LDADD)

BBoxUnionFunctionTest_SOURCES = BBoxUnionFunctionTest.cc $(TEST_SRC)
BBoxUnionFunctionTest_LDADD = -ltest-types $(AM_LDADD)

BBoxCombFunctionTest_SOURCES = BBoxCombFunctionTest.cc $(TEST_SRC)
BBoxCombFunctionTest_LDADD = -ltest-types $(AM_LDADD)

RoiFunctionTest_SOURCES = RoiFunctionTest.cc $(TEST_SRC)
RoiFunctionTest_LDADD = -ltest-types $(AM_LDADD)

OdometerTest_SOURCES = OdometerTest.cc $(TEST_SRC)
OdometerTest_LDADD = $(AM_LDADD) $(DAP_LIBS)

MaskArrayFunctionTest_SOURCES = MaskArrayFunctionTest.cc $(TEST_SRC)
MaskArrayFunctionTest_LDADD = -ltest-types $(AM_LDADD)

MakeMaskFunctionTest_SOURCES = MakeMaskFunctionTest.cc $(TEST_SRC)
MakeMaskFunctionTest_LDADD = -ltest-types $(AM_LDADD)

RangeFunctionTest_SOURCES = RangeFunctionTest.cc $(TEST_SRC)
RangeFunctionTest_CPPFLAGS = $(AM_CPPFLAGS) $(CPPUNIT_CFLAGS)
RangeFunctionTest_LDADD = -ltest-types $(AM_LDADD)

# if BUILD_GDAL
#
# ScaleUtilTest_SOURCES = ScaleUtilTest.cc $(TEST_SRC)
# ScaleUtilTest_CPPFLAGS = $(AM_CPPFLAGS) $(CPPUNIT_CFLAGS) $(GDAL_CFLAGS)
# ScaleUtilTest_LDADD = -ltest-types $(AM_LDADD) $(GDAL_LDFLAGS)
#
# ScaleUtilTest3D_SOURCES = ScaleUtilTest3D.cc $(TEST_SRC)
# ScaleUtilTest3D_CPPFLAGS = $(AM_CPPFLAGS) $(CPPUNIT_CFLAGS) $(GDAL_CFLAGS)
# ScaleUtilTest3D_LDADD = -ltest-types $(AM_LDADD) $(GDAL_LDFLAGS)
#
# endif
