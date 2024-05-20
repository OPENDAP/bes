
# Tests

AUTOMAKE_OPTIONS = foreign

# Headers in 'tests' are used by the arrayT unit tests.

# FIXME Remove this hack. Set these with configure. jhrg 11/25/19
OPENSSL_LIBS=-lcrypto
OPENSSL_INC=-I/usr/local/opt/openssl/include
#OPENSSL_LDFLAGS=-L/usr/local/opt/openssl/lib

AM_CPPFLAGS = $(OPENSSL_INC) -I$(top_srcdir) -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch \
-I$(top_srcdir)/modules/dmrpp_module $(DAP_CFLAGS)

# Removed linking with shared objects. jhrg 1/21/11
# LDADD = $(top_builddir)/dispatch/libbes_dispatch.la $(top_builddir)/dap/libdap_module.la \
# $(top_builddir)/modules/dmrpp_module/libdmrpp_module.la -ltest-types $(DAP_LIBS) $(LIBS)

LDADD = $(top_builddir)/dispatch/libbes_dispatch.la $(top_builddir)/dap/.libs/libdap_module.a \
$(top_builddir)/modules/dmrpp_module/.libs/libdmrpp_module.a -ltest-types $(DAP_LIBS) $(LIBS)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LDADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

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

# If 'pathinfo_files' does not exist, replicate it here. tar (used to make
# the source dist) dereferences symbolic links, so we make them this 
# way to get distcheck to work.
pathinfo_files:
	test -d pathinfo_files || mkdir -p pathinfo_files/nc
	(cd pathinfo_files/nc && touch testfile.txt && ln -s testfile.txt link_to_testfile.txt)
	(cd pathinfo_files && ln -s nc link_to_nc)

clean-local:
	-rm -rf mds pathinfo_files response_cache tmp temp_file_test

EXTRA_DIST = input-files mds_baselines test_utils.cc test_utils.h TestFunction.h \
test_config.h.in bes.conf.in

CLEANFILES = testout test_config.h bes.conf .dodsrc 

DISTCLEANFILES = *.strm *.file *.Po tmp.txt  tmp_* bes.log mds_ledger.txt

BES_CONF_IN = bes.conf.in

test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(srcdir)/$(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
	    -e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = ResponseBuilderTest ObjMemCacheTest FunctionResponseCacheTest \
ShowPathInfoTest TemporaryFileTest GlobalMetadataStoreTest DapUtilsTest

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
../CacheMarshaller.o ../CacheUnMarshaller.o
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

DapUtilsTest_SOURCES = DapUtilsTest.cc
DapUtilsTest_OBJS = ../DapUtils.o
DapUtilsTest_LDADD = $(DapUtilsTest_OBJS) $(LDADD)

TemporaryFileTest_SOURCES = TemporaryFileTest.cc
TemporaryFileTest_OBJS = ../TempFile.o
TemporaryFileTest_LDADD = $(TemporaryFileTest_OBJS) $(LDADD)

GlobalMetadataStoreTest_SOURCES = GlobalMetadataStoreTest.cc $(TEST_SRC)
GlobalMetadataStoreTest_OBJS = ../GlobalMetadataStore.o ../TempFile.o
GlobalMetadataStoreTest_LDADD = $(GlobalMetadataStoreTest_OBJS) $(LDADD)

# StoredDap2ResultTest_SOURCES = StoredDap2ResultTest.cc  $(TEST_SRC)
# StoredDap2ResultTest_LDADD = $(LDADD)

# StoredDap4ResultTest_SOURCES = StoredDap4ResultTest.cc  $(TEST_SRC)
# StoredDap4ResultTest_LDADD = $(LDADD)

# SequenceAggregationServerTest_SOURCES = SequenceAggregationServerTest.cc
# SequenceAggregationServerTest_LDADD = $(LDADD)
