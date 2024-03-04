
SUBDIRS = 

# Tests

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/modules/debug_functions -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
AM_LDADD = $(BES_DISPATCH_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(BES_HTTP_LIB)

# jhrg 6/2/23 $(BES_EXTRA_LIBS)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
endif

if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --quiet --trace-children=yes --error-exitcode=1  --dsymutil=yes --leak-check=yes
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -Wcast-align

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

noinst_HEADERS = test_config.h

DIRS_EXTRA = 

EXTRA_DIST = test_config.h.in

CLEANFILES = testout .dodsrc  *.gcda *.gcno test_config.h *.strm *.file tmp.txt

DISTCLEANFILES = 

BUILT_SOURCES = test_config.h

# `${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`

# test_config.h: $(srcdir)/test_config.h.in Makefile
#	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
#	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
#	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
#	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = ErrorFunctionTest AbortFunctionTest SleepFunctionTest
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

OBJS = ../DebugFunctions.o

ErrorFunctionTest_SOURCES =  ErrorFunctionTest.cc 
ErrorFunctionTest_LDADD =  ../DebugFunctions.o $(ErrorFunctionTest_OBJ) $(AM_LDADD) $(DAP_LIBS)

AbortFunctionTest_SOURCES =  AbortFunctionTest.cc 
AbortFunctionTest_LDADD =  ../DebugFunctions.o $(AbortFunctionTest_OBJ) $(AM_LDADD) $(DAP_LIBS)

SleepFunctionTest_SOURCES =  SleepFunctionTest.cc 
SleepFunctionTest_LDADD =  ../DebugFunctions.o $(SleepFunctionTest_OBJ) $(AM_LDADD) $(DAP_LIBS)
