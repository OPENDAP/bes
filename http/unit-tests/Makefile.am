# Tests

AUTOMAKE_OPTIONS = foreign

# FIXME Remove this hack. Set these with configure. jhrg 11/25/19
OPENSSL_LIBS=-lcrypto
OPENSSL_INC=-I/usr/local/opt/openssl/include
OPENSSL_LDFLAGS=-L/usr/local/opt/openssl/lib

AM_CPPFLAGS = $(OPENSSL_INC) -I$(top_srcdir) -I$(top_srcdir)/http \
    -I$(top_srcdir)/dispatch $(DAP_CFLAGS)
LIBADD = $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS) $(BES_DISPATCH_LIB) $(BES_HTTP_LIB)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS) -Wno-deprecated-declarations
LIBADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

DISTCLEANFILES = bes.conf bes.log bes_ngap_s3_creds.conf credentials.conf test_config.h

CLEANFILES = bes.conf *.dbg *.log

EXTRA_DIST = test_config.h.in bes.conf.in allowed_hosts_test.ini bes_ngap_s3_creds.conf.in \
awsv4 weak.conf credentials.conf.src

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

noinst_DATA = bes.conf bes_ngap_s3_creds.conf

BUILT_SOURCES = test_config.h credentials.conf bes.conf bes_ngap_s3_creds.conf

noinst_HEADERS = test_config.h

# This way of building the header ensures it's in the build dir and that there
# are no '../' sequences in the paths. The BES will reject paths with 'dot dot'
# in them in certain circumstances. jhrg 1/21/18
test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_top_srcdir[@]%$${mod_abs_top_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

credentials.conf: credentials.conf.src
	cp $< $@
	chmod 600 $@

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using the
# configure value for the parameter when running the distcheck target.

bes.conf: bes.conf.in $(top_srcdir)/configure.ac
bes_ngap_s3_creds.conf: bes_ngap_s3_creds.conf.in $(top_srcdir)/configure.ac

%.conf: %.conf.in
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

############################################################################
# Unit Tests
#

if CPPUNIT

UNIT_TESTS =  HttpUtilsTest HttpErrorTest RemoteResourceTest EffectiveUrlCacheTest HttpUrlTest SignedUrlCacheTest \
AllowedHostsTest awsv4_test CurlUtilsTest CurlSListTest

# CredentialsManagerTest was removed because it was testing our S3 signing code and
# that code is broken and being replaced by the AWS C++ SDK. jhrg 10/17/25
# CredentialsManagerTest HttpCacheTest

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
	test ! -d $(builddir)/static-cache || rm -rf $(builddir)/static-cache
	test ! -d $(builddir)/cache || rm -rf $(builddir)/cache

HttpUtilsTest_SOURCES = HttpUtilsTest.cc
HttpUtilsTest_LDADD = $(LIBADD)

HttpErrorTest_SOURCES = HttpErrorTest.cc
HttpErrorTest_LDADD =  $(LIBADD)


# ResponseTest_SOURCES = ResponseTest.cc
# ResponseTest_LDADD = $(LIBADD)

RemoteResourceTest_SOURCES = RemoteResourceTest.cc
RemoteResourceTest_CXXFLAGS = -static
RemoteResourceTest_LDADD = $(LIBADD)

CurlUtilsTest_SOURCES = CurlUtilsTest.cc
CurlUtilsTest_LDADD = $(LIBADD)

# HttpCacheTest_SOURCES = HttpCacheTest.cc
# HttpCacheTest_LDADD = $(LIBADD)

EffectiveUrlCacheTest_SOURCES = EffectiveUrlCacheTest.cc
EffectiveUrlCacheTest_LDADD = $(LIBADD)

SignedUrlCacheTest_SOURCES = SignedUrlCacheTest.cc
SignedUrlCacheTest_LDADD = $(LIBADD)

HttpUrlTest_SOURCES = HttpUrlTest.cc
HttpUrlTest_LDADD = $(LIBADD)

AllowedHostsTest_SOURCES = AllowedHostsTest.cc
AllowedHostsTest_LDADD = $(LIBADD)

# CredentialsManagerTest_SOURCES = CredentialsManagerTest.cc
# CredentialsManagerTest_LDADD = ../.libs/libbes_http.a $(LIBADD)

awsv4_test_SOURCES = awsv4_test.cc
awsv4_test_LDADD = ../.libs/libbes_http.a $(LIBADD)

CurlSListTest_SOURCES = CurlSListTest.cc
CurlSListTest_LDADD = $(LIBADD)