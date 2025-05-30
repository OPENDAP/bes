
# Tests

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap \
    -I$(top_srcdir)/modules -I$(top_srcdir)/modules/asciival $(DAP_CFLAGS)
LIBADD = $(BES_DISPATCH_LIB) $(DAP_CLIENT_LIBS)

# jhrg 6/2/23 $(BES_EXTRA_LIBS)

AM_LDADD = $(LIBADD)
AM_CXXFLAGS = 

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror

AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

# This header file is used for parse files
noinst_HEADERS = test_config.h

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

EXTRA_DIST = testsuite test_config.h.in

CLEANFILES = test_config.h

DISTCLEANFILES = 

BUILT_SOURCES = test_config.h

test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

############################################################################
# Unit Tests
#

# The object files from asciival are needed to link the unit tests
ASCIIOBJS = ../AsciiByte.o ../AsciiInt32.o ../AsciiFloat64.o ../AsciiStr.o   \
	../AsciiUrl.o ../AsciiArray.o ../AsciiStructure.o ../AsciiSequence.o \
	../AsciiGrid.o ../AsciiUInt32.o ../AsciiInt16.o ../AsciiUInt16.o     \
	../AsciiFloat32.o ../AsciiOutput.o ../AsciiOutputFactory.o	     \
	../get_ascii.o

if CPPUNIT
UNIT_TESTS = AsciiArrayTest AsciiOutputTest
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

AsciiArrayTest_SOURCES = AsciiArrayTest.cc
AsciiOutputTest_SOURCES = AsciiOutputTest.cc

AsciiArrayTest_LDADD = $(ASCIIOBJS) $(AM_LDADD)
AsciiOutputTest_LDADD = $(ASCIIOBJS) $(AM_LDADD)

