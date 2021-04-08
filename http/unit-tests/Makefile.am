# Tests

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) \
                -I$(top_srcdir)/http \
                -I$(top_srcdir)/dispatch \
                $(DAP_CFLAGS)

LIBADD = $(BES_DISPATCH_LIB) $(BES_HTTP_LIB)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LIBADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

DISTCLEANFILES = test_config.h *.Po

CLEANFILES = bes.conf *.dbg *.log

EXTRA_DIST = test_config.h.in bes.conf.in allowed_hosts_test.ini

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

noinst_DATA = bes.conf

BUILT_SOURCES = test_config.h

noinst_HEADERS = test_config.h

# This way of building the header ensures it's in the build dir and that there
# are no '../' seqeunces in the paths. The BES will reject paths with 'dot dot'
# in them in certain circumstances. jhrg 1/21/18
test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`python -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_top_srcdir=`python -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	mod_abs_builddir=`python -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_top_srcdir[@]%$${mod_abs_top_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

BES_CONF_IN = bes.conf.in

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`python -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf; \
	cat bes.conf; # TODO Remove this!


############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS =  HttpUtilsTest RemoteResourceTest CurlUtilsTest EffectiveUrlCacheTest HttpUrlTest AllowedHostsTest
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

HttpUtilsTest_SOURCES = HttpUtilsTest.cc
HttpUtilsTest_LDADD = $(LIBADD)

RemoteResourceTest_SOURCES = RemoteResourceTest.cc
RemoteResourceTest_LDADD = $(LIBADD)

CurlUtilsTest_SOURCES = CurlUtilsTest.cc
CurlUtilsTest_LDADD = $(LIBADD)

EffectiveUrlCacheTest_SOURCES = EffectiveUrlCacheTest.cc
EffectiveUrlCacheTest_LDADD = $(LIBADD)

HttpUrlTest_SOURCES = HttpUrlTest.cc
HttpUrlTest_LDADD = $(LIBADD)

AllowedHostsTest_SOURCES = AllowedHostsTest.cc
AllowedHostsTest_LDADD = $(LIBADD)
