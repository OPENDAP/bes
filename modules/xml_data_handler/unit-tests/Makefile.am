
# Tests

# Arrange to build with the backward compatibility mode enabled.
AM_CPPFLAGS = -I$(top_srcdir)/modules/xml_data_handler -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(BES_DISPATCH_LIB) $(BES_EXTRA_LIBS) $(DAP_SERVER_LIBS) $(XML2_LIBS)

AM_LDADD = $(LIBADD)
AM_CXXFLAGS = 

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This header file is used for parse files
noinst_HEADERS = test_config.h

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

EXTRA_DIST = testsuite test_config.h.in

CLEANFILES = test_config.h

DISTCLEANFILES = *.Po

BUILT_SOURCES = test_config.h

test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	mod_abs_builddir=`echo ${abs_builddir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = XDArrayTest XDOutputTest
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

XD_OBJS = ../XDArray.o ../XDInt16.o ../XDStructure.o ../XDInt32.o	\
        ../XDUInt16.o ../XDByte.o ../XDOutput.o ../XDUInt32.o		\
        ../XDFloat32.o ../XDUrl.o ../XDFloat64.o ../XDSequence.o	\
        ../XDGrid.o ../XDStr.o ../XDOutputFactory.o ../get_xml_data.o

XDArrayTest_SOURCES = XDArrayTest.cc 
XDOutputTest_SOURCES = XDOutputTest.cc 

# if not linking with the module, use $(XDOBJS) below
XDArrayTest_LDADD = $(XD_OBJS) $(AM_LDADD)
XDOutputTest_LDADD = $(XD_OBJS) $(AM_LDADD)

