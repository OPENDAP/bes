
# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/functions -I$(top_srcdir)/functions/geo-functions $(GF_CFLAGS) $(XML2_CFLAGS) $(DAP_CFLAGS)
LDADD = -ltest-types $(CPPUNIT_LIBS) $(DAP_SERVER_LIBS) $(TEST_OBJ) $(DAP_CLIENT_LIBS) $(GDAL_LDFLAGS) 
#-L.. -lfunctions_module  
# ../libfunctions_module.la

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LDADD += $(CPPUNIT_LIBS)
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

EXTRA_DIST = test_config.h.in

CLEANFILES = testout .dodsrc  *.gcda *.gcno

# I added '*.po' because there are dependencies on ../*.o files and
# that seems to leave *.Po files here that distclean complains about.
DISTCLEANFILES = test_config.h *.strm *.file *.Po tmp.txt

# TODO Check if this sed command to filter out the ../ from the value of 
# ${abs_srcdir} is really needed for the distcheck target. jhrg 5/16/13
test_config.h: test_config.h.in Makefile
	@my_topdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${my_topdir}%" $< > test_config.h


############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = ScaleUtilTest
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
TEST_SRC = 

# Many of the tests use these - build once
TEST_OBJ =

ScaleUtilTest_SOURCES = ScaleUtilTest.cc ../scale_util.cc $(TEST_SRC)
ScaleUtilTest_CPPFLAGS = $(AM_CPPFLAGS) $(CPPUNIT_CFLAGS)
ScaleUtilTest_LDADD = $(LDADD)

# 
