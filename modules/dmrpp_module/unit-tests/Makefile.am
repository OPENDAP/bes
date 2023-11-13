
# Tests

AUTOMAKE_OPTIONS = foreign subdir-objects
# subdir-objects
# using .cc files above this dir seems to break 'subdir-objects' jhrg 5/5/22

# TODO set these with configure. jhrg 11/26/19
# OPENSSL_LIBS=-lcrypto

AM_CPPFLAGS = $(H5_CPPFLAGS) -I$(top_srcdir)  -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap \
    -I$(top_srcdir)/http -I$(top_srcdir)/pugixml/src -I$(top_srcdir)/modules/common \
    -I$(top_srcdir)/modules/dmrpp_module $(DAP_CFLAGS)

# Added -lz for ubuntu
LIBADD = $(BES_DISPATCH_LIB) $(top_builddir)/dap/.libs/libdap_module.a $(BES_HTTP_LIB) \
    -L$(top_builddir)/modules/common -lmodules_common $(H5_LDFLAGS) \
    $(H5_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) $(OPENSSL_LIBS) $(XML2_LIBS) -lz

# jhrg 6/2/23 $(BES_EXTRA_LIBS)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS) 
LIBADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -Wcast-align

AM_CXXFLAGS = -Wno-vla-extension -Wno-inconsistent-missing-override -Wno-unused-variable
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

noinst_PROGRAMS = pugi_xml_test

pugi_xml_test_SOURCES = pugi_xml_test.cc

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

noinst_HEADERS = test_config.h

noinst_DATA = bes.conf

EXTRA_DIST = bes.conf.in test_config.h.in curl_handle_pool_keys.conf baselines input-files

CLEANFILES = *.gcda *.gcno *.strm *.file test_config.h tmp.txt

DISTCLEANFILES = bes.conf bes.log

BUILT_SOURCES = test_config.h

BES_CONF_IN = bes.conf.in

test_config.h: $(srcdir)/test_config.h.in Makefile.am
	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_top_srcdir[@]%$${mod_abs_top_srcdir}%" \
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

UNIT_TESTS = DmrppArrayTest SuperChunkTest ChunkTest DmrppParserTest DmrppCommonTest CurlHandlePoolTest \
DMZTest build_dmrpp_util_test DmrppChunkOdometerTest vlsa_util_test

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

DmrppParserTest_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/modules/hdf5_handler
DmrppParserTest_SOURCES = DmrppParserTest.cc
DmrppParserTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

DmrppCommonTest_SOURCES = DmrppCommonTest.cc
DmrppCommonTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

# DmrppMetadataStoreTest_SOURCES = DmrppMetadataStoreTest.cc
# DmrppMetadataStoreTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

DmrppArrayTest_SOURCES = DmrppArrayTest.cc
DmrppArrayTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

DMZTest_SOURCES = DMZTest.cc
DMZTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

DmrppChunkOdometerTest_SOURCES = DmrppChunkOdometerTest.cc
DmrppChunkOdometerTest_LDADD = ../.libs/libdmrpp_module.a $(LIBADD)

build_dmrpp_util_test_CPPFLAGS = $(AM_CPPFLAGS) $(H5_CPPFLAGS) -I$(top_srcdir)/modules/hdf5_handler
build_dmrpp_util_test_SOURCES = build_dmrpp_util_test.cc ../build_dmrpp_util.cc ../h5common.cc
build_dmrpp_util_test_LDADD = $(H5_LDFLAGS) $(H5_LIBS) ../.libs/libdmrpp_module.a $(LIBADD)

vlsa_util_test_SOURCES = vlsa_util_test.cc ../vlsa_util.cc
vlsa_util_test_LDADD   = $(LIBADD)


