
SUBDIRS = 

# Tests

AUTOMAKE_OPTIONS = foreign

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/modules/dmrpp_module -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LDADD = $(BES_DISPATCH_LIB) $(BES_EXTRA_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
else
AM_CPPFLAGS = -I$(top_srcdir) $(XML2_CFLAGS) $(DAP_CFLAGS)
LDADD =  $(DAP_SERVER_LIBS) 
endif

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LDADD += $(CPPUNIT_LIBS)
endif

if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --quiet --trace-children=yes --error-exitcode=1  --dsymutil=yes --leak-check=yes
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

noinst_HEADERS = test_config.h

DIRS_EXTRA = 

EXTRA_DIST = 

CLEANFILES = testout .dodsrc  *.gcda *.gcno

DISTCLEANFILES = test_config.h *.strm *.file tmp.txt

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = DmrppParserTest DmrppTypeReadTest DmrppHttpReadTest
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

# The VPATH and SRCS are not used. We link with the library even though
# that results in warnings that are annoying. jhrg 12/28/16
VPATH = src:../

SRCS = DmrppArray.cc DmrppByte.cc DmrppD4Enum.cc DmrppD4Group.cc DmrppD4Opaque.cc\
DmrppD4Sequence.cc DmrppFloat32.cc DmrppFloat64.cc DmrppInt16.cc DmrppInt32.cc \
DmrppInt64.cc DmrppInt8.cc DmrppParserSax2.cc DmrppStr.cc DmrppStructure.cc \
DmrppUInt16.cc DmrppUInt32.cc DmrppUInt64.cc DmrppUrl.cc DmrppUtil.cc \
DmrppTypeFactory.cc DrmppCommon.h

# Linking with the .so modules produces a warning, but lets ignore it.
# The alternative is to link with objects in the parent dir _but_, we
# still will get a warning rom linking with the bes dap module which
# we have to link against since we're using BESDapError. jhrg 11/21/16

DmrppParserTest_SOURCES = DmrppParserTest.cc
DmrppParserTest_LDFLAGS =  $(LDADD) -L\$(abs_top_builddir)/modules/dmrpp_module -ldmrpp_module

DmrppTypeReadTest_SOURCES = DmrppTypeReadTest.cc
DmrppTypeReadTest_LDFLAGS =  $(LDADD) -L\$(abs_top_builddir)/modules/dmrpp_module -ldmrpp_module

DmrppHttpReadTest_SOURCES = DmrppHttpReadTest.cc
DmrppHttpReadTest_LDFLAGS =  $(LDADD) -L\$(abs_top_builddir)/modules/dmrpp_module -ldmrpp_module
