# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir)/cmdln -I$(top_srcdir)/dispatch $(DAP_CFLAGS)
AM_LDADD =  $(top_builddir)/dispatch/libbes_dispatch.la -ltest-types $(DAP_LIBS) $(LIBS)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This header file is used for configuration location
noinst_HEADERS = test_config.h

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

DIRS_EXTRA = 

EXTRA_DIST = $(DIRS_EXTRA) test_config.h.in

CLEANFILES = 

DISTCLEANFILES = test_config.h

############################################################################
# Unit Tests


test_config.h: test_config.h.in Makefile
	sed -e "s%[@]srcdir[@]%${srcdir}%" $< > test_config.h

if CPPUNIT
UNIT_TESTS = translateT
else
UNIT_TESTS =

check-local:
	@echo ""
	@echo "**********************************************************"
	@echo "You must have cppunit 1.12.x or greater installed to run *"
	@echo "check target in cmdln unit-tests directory               *"
	@echo "**********************************************************"
	@echo ""
endif

translateT_SOURCES = translateT.cc
translateT_CPPFLAGS = $(XML2_CFLAGS) $(AM_CPPFLAGS)
translateT_LDADD = $(top_builddir)/cmdln/CmdTranslation.o $(top_builddir)/dispatch/libbes_dispatch.la $(XML2_LIBS) $(AM_LDADD)

