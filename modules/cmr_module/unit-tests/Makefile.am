# Tests

AUTOMAKE_OPTIONS = foreign


AM_CPPFLAGS = -I$(top_srcdir)/modules -I$(top_srcdir)/modules/cmr_module \
-I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)

# Added -lz for ubuntu
LIBADD = $(BES_DISPATCH_LIB) $(BES_DAP_LIB) $(BES_EXTRA_LIBS) \
$(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) -lz


if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LIBADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

DISTCLEANFILES = test_config.h *.Po

CLEANFILES = *.dbg *.log tmp/*

EXTRA_DIST = baselines test_config.h.in

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
	@mod_abs_srcdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	mod_abs_builddir=`echo ${abs_builddir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

tmp:
	test -d tmp || mkdir tmp

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = CmrApiTest
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

OBJS = ../CMRCatalog.o

CmrApiTest_SOURCES = CmrApiTest.cc
CmrApiTest_LDADD = ../RemoteHttpResource.o  ../curl_utils.o ../rjson_utils.o ../CmrCache.o ../CmrUtils.o ../CmrApi.o ../Granule.o $(OBJS) $(LIBADD)
