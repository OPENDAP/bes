
# Tests

AUTOMAKE_OPTIONS = foreign 

# Removed jhrg 9.22.17 in an attempt to fix distcheck's build
# subdir-objects

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap \
	-I$(top_srcdir)/functions -I$(top_srcdir)/functions/stare $(DAP_CFLAGS)
AM_LDADD =  $(DAP_LIBS) $(top_builddir)/dispatch/libbes_dispatch.la  \
	-ltest-types $(H5_LDFLAGS) $(H5_LIBS) $(STARE_LDFLAGS) $(STARE_LIBS)

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

BUILT_SOURCES = test_config.h bes.conf

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

noinst_HEADERS = 

EXTRA_DIST = MYD09.A2019003.2040.006.2019005020913_sidecar.h5 test_config.h.in bes.conf.in

CLEANFILES = testout .dodsrc *.gcda *.gcno

# I added '*.po' because there are dependencies on ../*.o files and
# that seems to leave *.Po files here that distclean complains about.
DISTCLEANFILES = *.strm *.file *.Po tmp.txt bes.conf test_config.h

# TODO Check if this sed command to filter out the ../ from the value of 
# ${abs_srcdir} is really needed for the distcheck target. jhrg 5/16/13
test_config.h: $(srcdir)/test_config.h.in Makefile
	@my_topdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_srcdir[@]%$${my_topdir}%" $< > test_config.h

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using
# configure's value for the parameter when running the distcheck target.
bes.conf: $(srcdir)/bes.conf.in Makefile
	@my_topdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	mod_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$${mod_abs_top_srcdir}%" \
    -e "s%[@]abs_srcdir[@]%$${my_topdir}%" $< > bes.conf

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = StareFunctionsTest

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
#TEST_SRC = test_utils.cc test_utils.h 

# Many of the tests use these - build once
#TEST_OBJ = ../roi_util.o ../functions_util.o

# Listing the objects here keeps from having to link with the module - not a portable
# solution - and listing these as source breaks distcheck jhrg 9/24/15
StareFunctionsTest_SOURCES = StareFunctionsTest.cc  $(TEST_SRC)
StareFunctionsTest_OBJ = ../StareIterateFunction.o
StareFunctionsTest_LDADD = $(StareFunctionsTest_OBJ) $(TEST_OBJ) $(AM_LDADD)
