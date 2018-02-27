
# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch $(DAP_CFLAGS) 
LDADD =  $(top_builddir)/dispatch/libbes_dispatch.la -ltest-types $(DAP_LIBS) $(LIBS)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LDADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

# Don't install test_config.h
noinst_HEADERS = test_config.h
# Build it first for the all, check and install targets. jhrg 1/20/18
BUILT_SOURCES = test_config.h

# Adding this here ensures the directory is made at runtime and in the
# build directory (I think - check this). jhrg 1/19/18
noinst_DATA = response_cache pathinfo_files bes.conf

response_cache:
	test -d response_cache  || mkdir response_cache 
	
# If 'input-files' does not exist, replicate it here. tar (used to make
# the source dist) dereferences symbolic links, so we make them this 
# way to get distcheck to work.
pathinfo_files:
	test -d pathinfo_files || mkdir -p pathinfo_files/nc
	(cd pathinfo_files/nc && touch testfile.txt && ln -s testfile.txt link_to_testfile.txt)
	(cd pathinfo_files && ln -s nc link_to_nc)
	
clean-local:
	-rm -rf response_cache pathinfo_files
	
EXTRA_DIST = test_utils.cc test_utils.h TestFunction.h test_config.h.in bes.conf.in input-files

CLEANFILES = testout test_config.h bes.conf .dodsrc 

DISTCLEANFILES = *.strm *.file *.Po tmp.txt  bes.log

BES_CONF_IN = bes.conf.in

test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	mod_abs_builddir=`echo ${abs_builddir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(srcdir)/$(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
	    -e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = ResponseBuilderTest ObjMemCacheTest FunctionResponseCacheTest \
ShowPathInfoTest TemporaryFileTest

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
ResponseBuilderTest_LDADD = $(ResponseBuilderTest_OBJS) $(LDADD) 

FunctionResponseCacheTest_SOURCES = FunctionResponseCacheTest.cc $(TEST_SRC)
FunctionResponseCacheTest_OBJS = ../BESDapFunctionResponseCache.o ../DapFunctionUtils.o \
../CachedSequence.o ../CacheTypeFactory.o ../CacheMarshaller.o ../CacheUnMarshaller.o 
FunctionResponseCacheTest_LDADD = $(FunctionResponseCacheTest_OBJS) $(LDADD)

ObjMemCacheTest_SOURCES = ObjMemCacheTest.cc
ObjMemCacheTest_OBJS = ../ObjMemCache.o
ObjMemCacheTest_LDADD = $(ObjMemCacheTest_OBJS) $(LDADD)

ShowPathInfoTest_SOURCES = ShowPathInfoTest.cc
ShowPathInfoTest_OBJS = ../ShowPathInfoResponseHandler.o 
ShowPathInfoTest_LDADD = $(ShowPathInfoTest_OBJS) $(LDADD)

TemporaryFileTest_SOURCES = TemporaryFileTest.cc
TemporaryFileTest_OBJS = ../TempFile.o
TemporaryFileTest_LDADD = $(TemporaryFileTest_OBJS) $(LDADD)

# StoredDap2ResultTest_SOURCES = StoredDap2ResultTest.cc  $(TEST_SRC)
# StoredDap2ResultTest_LDADD = $(LDADD)

# StoredDap4ResultTest_SOURCES = StoredDap4ResultTest.cc  $(TEST_SRC)
# StoredDap4ResultTest_LDADD = $(LDADD)

# SequenceAggregationServerTest_SOURCES = SequenceAggregationServerTest.cc
# SequenceAggregationServerTest_LDADD = $(LDADD)
