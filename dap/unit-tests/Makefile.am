
# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch $(DAP_CFLAGS) 
AM_LDADD =  $(top_builddir)/dispatch/libbes_dispatch.la -ltest-types \
$(DAP_LIBS) $(LIBS)

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

noinst_HEADERS = test_config.h

noinst_DATA = bes.conf

DIRS_EXTRA = response_cache input-files

EXTRA_DIST = $(DIRS_EXTRA) test_utils.cc test_utils.h TestFunction.h test_config.h.in bes.conf.in

CLEANFILES = testout .dodsrc  *.gcda *.gcno

DISTCLEANFILES = test_config.h  *.strm *.file *.Po tmp.txt bes.conf

BES_CONF_IN = bes.conf.in

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = ResponseBuilderTest ObjMemCacheTest FunctionResponseCacheTest \
TemporaryFileTest

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

TEST_SRC = test_utils.cc test_utils.h

ResponseBuilderTest_SOURCES = ResponseBuilderTest.cc $(TEST_SRC)
ResponseBuilderTest_OBJS = ../BESDapResponseBuilder.o ../BESDataDDSResponse.o \
../BESDDSResponse.o ../BESDapResponse.o ../BESDapFunctionResponseCache.o \
../BESStoredDapResultCache.o ../DapFunctionUtils.o ../CachedSequence.o ../CacheTypeFactory.o \
../CacheMarshaller.o ../CacheUnMarshaller.o ../../dispatch/BESFileLockingCache.o
ResponseBuilderTest_LDADD = $(ResponseBuilderTest_OBJS) $(AM_LDADD) 

FunctionResponseCacheTest_SOURCES = FunctionResponseCacheTest.cc $(TEST_SRC)
FunctionResponseCacheTest_OBJS = ../BESDapFunctionResponseCache.o ../DapFunctionUtils.o \
../CachedSequence.o ../CacheTypeFactory.o ../CacheMarshaller.o ../CacheUnMarshaller.o 
FunctionResponseCacheTest_LDADD = $(FunctionResponseCacheTest_OBJS) $(AM_LDADD)

ObjMemCacheTest_SOURCES = ObjMemCacheTest.cc
ObjMemCacheTest_OBJS = ../ObjMemCache.o
ObjMemCacheTest_LDADD = $(ObjMemCacheTest_OBJS) $(AM_LDADD)

TemporaryFileTest_SOURCES = TemporaryFileTest.cc
TemporaryFileTest_OBJS = ../TempFile.o
TemporaryFileTest_LDADD = $(TemporaryFileTest_OBJS) $(AM_LDADD)

# StoredDap2ResultTest_SOURCES = StoredDap2ResultTest.cc  $(TEST_SRC)
# StoredDap2ResultTest_LDADD = $(AM_LDADD)

# StoredDap4ResultTest_SOURCES = StoredDap4ResultTest.cc  $(TEST_SRC)
# StoredDap4ResultTest_LDADD = $(AM_LDADD)

# SequenceAggregationServerTest_SOURCES = SequenceAggregationServerTest.cc
# SequenceAggregationServerTest_LDADD = $(AM_LDADD)
