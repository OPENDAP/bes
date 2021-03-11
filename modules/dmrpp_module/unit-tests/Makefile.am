
# Tests

AUTOMAKE_OPTIONS = foreign

# TODO set these with configure. jhrg 11/26/19
OPENSSL_LIBS=-lcrypto
OPENSSL_INC=-I/usr/local/opt/openssl/include
OPENSSL_LDFLAGS=-L/usr/local/opt/openssl/lib

AM_CPPFLAGS = $(H5_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/modules -I$(top_srcdir)/modules/dmrpp_module \
-I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/http $(DAP_CFLAGS) $(OPENSSL_INC)

# FIXME with configure.ac jhrg 9/2/20
BES_DAP_LIB_STATIC = $(abs_top_builddir)/dap/.libs/libdap_module.a

# Added -lz for ubuntu
LIBADD = $(BES_DISPATCH_LIB) $(BES_DAP_LIB_STATIC) $(BES_HTTP_LIB) $(BES_EXTRA_LIBS) \
$(H5_LDFLAGS) $(H5_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) \
$(OPENSSL_LDFLAGS) $(OPENSSL_LIBS) $(XML2_LIBS) -lz

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS) 
LIBADD += $(CPPUNIT_LIBS)
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

CREDS_CONF = $(builddir)/credentials.conf
CREDS_CONF_IN = $(srcdir)/input-files/credentials.conf.in

noinst_HEADERS = test_config.h

noinst_DATA = bes.conf bes_ngap_s3_creds.conf $(CREDS_CONF) input-files/weak.conf

EXTRA_DIST = bes.conf.in bes_ngap_s3_creds.conf.in test_config.h.in curl_handle_pool_keys.conf \
baselines input-files awsv4

CLEANFILES = *.gcda *.gcno *.strm *.file test_config.h tmp.txt

DISTCLEANFILES = bes.conf bes_ngap_s3_creds.conf bes.log $(CREDS_CONF)

BUILT_SOURCES = test_config.h

BES_CONF_IN = bes.conf.in
BES_NGAP_CREDS_CONF_IN = bes_ngap_s3_creds.conf.in

test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`python -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_top_srcdir=`python -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	mod_abs_builddir=`python -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_top_srcdir[@]%$${mod_abs_top_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(srcdir)/$(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`python -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
	    -e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

bes_ngap_s3_creds.conf: $(srcdir)/$(BES_NGAP_CREDS_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`python -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
	    -e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes_ngap_s3_creds.conf

$(CREDS_CONF):
	cp $(CREDS_CONF_IN) $(CREDS_CONF)
	chmod 600 $(CREDS_CONF)

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = DmrppArrayTest NgapCredentialsTest SuperChunkTest ChunkTest DmrppParserTest DmrppCommonTest \
DmrppMetadataStoreTest CredentialsManagerTest awsv4_test CurlHandlePoolTest
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

clean-local:
	-rm -rf mds mds_ledger.txt

CurlHandlePoolTest_SOURCES = CurlHandlePoolTest.cc
CurlHandlePoolTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

ChunkTest_SOURCES = ChunkTest.cc
ChunkTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

SuperChunkTest_SOURCES = SuperChunkTest.cc
SuperChunkTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

CredentialsManagerTest_SOURCES = CredentialsManagerTest.cc
CredentialsManagerTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

NgapCredentialsTest_SOURCES = NgapCredentialsTest.cc
NgapCredentialsTest_LDADD = ../AccessCredentials.o ../NgapS3Credentials.o $(LIBADD)

awsv4_test_SOURCES = awsv4_test.cc
awsv4_test_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

DmrppParserTest_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/modules/hdf5_handler
DmrppParserTest_SOURCES = DmrppParserTest.cc
DmrppParserTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

DmrppCommonTest_SOURCES = DmrppCommonTest.cc $(top_srcdir)/modules/read_test_baseline.cc
DmrppCommonTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

DmrppMetadataStoreTest_SOURCES = DmrppMetadataStoreTest.cc $(top_srcdir)/modules/read_test_baseline.cc
DmrppMetadataStoreTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

DmrppArrayTest_SOURCES = DmrppArrayTest.cc $(top_srcdir)/modules/read_test_baseline.cc
DmrppArrayTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)
