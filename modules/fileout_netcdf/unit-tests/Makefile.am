# Tests

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap \
    -I$(top_srcdir)/modules -I$(top_srcdir)/rapidjson -I$(top_srcdir)/modules/fileout_netcdf $(DAP_CFLAGS)

LIBADD =  $(NC_LDFLAGS) $(NC_LIBS)  $(BES_DISPATCH_LIB) $(BES_DAP_LIB) $(DAP_SERVER_LIBS)

# TODO set these with configure. jhrg 11/26/19
OPENSSL_LIBS=-lcrypto
OPENSSL_INC=-I/usr/local/opt/openssl/include
OPENSSL_LD=-L/usr/local/opt/openssl/lib

LIBADD += $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)

# jhrg 6/2/23 $(BES_EXTRA_LIBS)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LIBADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror

AM_CXXFLAGS = -Wno-inconsistent-missing-override
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

DISTCLEANFILES = test_config.h *.Po

CLEANFILES = *.dbg *.log tmp/*

EXTRA_DIST = test_config.h.in

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

# Adding the tmp dir here causes make to put it in the build dir
noinst_DATA = tmp

BUILT_SOURCES = test_config.h

noinst_HEADERS = test_config.h

# This way of building the header ensures it's in the build dir and that there
# are no '../' seqeunces in the paths. The BES will reject paths with 'dot dot'
# in them in certain circumstances. jhrg 1/21/18
test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

tmp:
	test -d tmp || mkdir tmp

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = HistoryUtilsTest FONcArrayTest D4ToolsTest

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

OBJS = ../history_utils.o
OBJS2 = ../d4_tools.o
LDFLAGS = -static

HistoryUtilsTest_SOURCES = HistoryUtilsTest.cc
HistoryUtilsTest_LDADD = $(OBJS) $(LIBADD)

FONcArrayTest_SOURCES = FONcArrayTest.cc
FONcArrayTest_LDADD = ../.libs/libfonc_module.a $(LIBADD)

D4ToolsTest_SOURCES = D4ToolsTest.cc
D4ToolsTest_LDADD = $(OBJS2) $(LIBADD)