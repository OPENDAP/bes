
# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch $(DAP_CFLAGS) 
AM_LDADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) -ltest-types \
$(top_builddir)/dispatch/libbes_dispatch.la $(top_builddir)/dap/libdap_module.la

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
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

DIRS_EXTRA = response_cache input-files

EXTRA_DIST = $(DIRS_EXTRA) test_utils.cc test_utils.h test_config.h.in

CLEANFILES = testout .dodsrc  *.gcda *.gcno

DISTCLEANFILES = test_config.h *.strm *.file tmp.txt

# test_config.h: test_config.h.in Makefile
#	sed -e "s%[@]abs_srcdir[@]%${abs_srcdir}%" $< > test_config.h

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = ResponseBuilderTest ResponseCacheTest StoredDap2ResultTest \
StoredDap4ResultTest SequenceAggregationServerTest
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

TEST_SRC = test_utils.cc test_utils.h

ResponseBuilderTest_SOURCES = ResponseBuilderTest.cc $(TEST_SRC)
ResponseBuilderTest_LDADD = $(AM_LDADD) 

ResponseCacheTest_SOURCES = ResponseCacheTest.cc  $(TEST_SRC)
ResponseCacheTest_LDADD = $(AM_LDADD)

StoredDap2ResultTest_SOURCES = StoredDap2ResultTest.cc  $(TEST_SRC)
StoredDap2ResultTest_LDADD = $(AM_LDADD)

StoredDap4ResultTest_SOURCES = StoredDap4ResultTest.cc  $(TEST_SRC)
StoredDap4ResultTest_LDADD = $(AM_LDADD)

SequenceAggregationServerTest_SOURCES = SequenceAggregationServerTest.cc
SequenceAggregationServerTest_LDADD = $(AM_LDADD)

# StoredResultTest_SOURCES = StoredResultTest.cc  $(TEST_SRC)
# StoredResultTest_LDADD = $(AM_LDADD)
