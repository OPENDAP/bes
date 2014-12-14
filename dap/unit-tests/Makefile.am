
# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch $(DAP_CFLAGS) 
AM_LDADD = $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) -ltest-types \
-L$(top_builddir)/dispatch -lbes_dispatch -L$(top_builddir)/dap -ldap_module

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

noinst_HEADERS = test_config.h

DIRS_EXTRA = response_cache input-files

EXTRA_DIST = $(DIRS_EXTRA) testFile.cc testFile.h test_config.h.in

CLEANFILES = testout .dodsrc  *.gcda *.gcno

DISTCLEANFILES = test_config.h *.strm *.file tmp.txt

# test_config.h: test_config.h.in Makefile
#	sed -e "s%[@]abs_srcdir[@]%${abs_srcdir}%" $< > test_config.h

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = ResponseBuilderTest ResponseCacheTest
# Removed from the h 1.9 branch and the trunk StoredResultTest
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

ResponseBuilderTest_SOURCES = ResponseBuilderTest.cc $(TEST_SRC)
ResponseBuilderTest_LDADD = $(AM_LDADD) 

ResponseCacheTest_SOURCES = ResponseCacheTest.cc  $(TEST_SRC)
ResponseCacheTest_LDADD = $(AM_LDADD)

# StoredResultTest_SOURCES = StoredResultTest.cc  $(TEST_SRC)
# StoredResultTest_LDADD = $(AM_LDADD)
