# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/dispatch
AM_LDADD = 
if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

DIRS_EXTRA = 

EXTRA_DIST = $(DIRS_EXTRA) 

CLEANFILES = 

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = pptcapi_extensionsT pptcapi_hexstrtoiT
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

pptcapi_extensionsT_SOURCES = pptcapi_extensionsT.cc
pptcapi_extensionsT_CPPFLAGS = $(AM_CPPFLAGS)
pptcapi_extensionsT_LDADD = $(top_builddir)/ppt/libbes_pptcapi.la $(AM_LDADD)

pptcapi_hexstrtoiT_SOURCES = pptcapi_hexstrtoiT.cc
pptcapi_hexstrtoiT_CPPFLAGS = $(AM_CPPFLAGS)
pptcapi_hexstrtoiT_LDADD = $(top_builddir)/ppt/libbes_pptcapi.la $(AM_LDADD)

