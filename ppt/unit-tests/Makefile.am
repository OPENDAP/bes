# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/dispatch $(DAP_CFLAGS)
AM_LDADD =  $(top_builddir)/dispatch/libbes_dispatch.la -ltest-types $(DAP_LIBS) $(LIBS)

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

DIRS_EXTRA = 

EXTRA_DIST = $(DIRS_EXTRA) 

CLEANFILES = sbT.out

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = connT sbT extT
else
UNIT_TESTS =

check-local:
	@echo ""
	@echo "**********************************************************"
	@echo "You must have cppunit 1.12.x or greater installed to run *"
	@echo "check target in ppt unit-tests directory                 *"
	@echo "**********************************************************"
	@echo ""
endif


connT_SOURCES = connT.cc ConnSocket.cc ConnTest.cc ConnSocket.h ConnTest.h \
 ConnTestStrs.h
connT_CPPFLAGS = $(AM_CPPFLAGS)
connT_LDADD = $(top_builddir)/ppt/libbes_ppt.la $(top_builddir)/dispatch/libbes_dispatch.la $(openssl_libs) $(AM_LDADD)

sbT_SOURCES = sbT.cc
sbT_CPPFLAGS = $(AM_CPPFLAGS)
sbT_LDADD = $(top_builddir)/ppt/libbes_ppt.la $(top_builddir)/dispatch/libbes_dispatch.la $(openssl_libs) $(AM_LDADD)

extT_SOURCES = extT.cc ExtConn.cc ExtConn.h
extT_CPPFLAGS = $(AM_CPPFLAGS)
extT_LDADD = $(top_builddir)/ppt/libbes_ppt.la $(top_builddir)/dispatch/libbes_dispatch.la $(openssl_libs) $(AM_LDADD)

