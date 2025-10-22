# Tests

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/modules/common \
    -I$(top_srcdir)/modules/dmrpp_module/ngap_container -I$(top_srcdir)/dispatch \
    -I$(top_srcdir)/dap -I$(top_srcdir)/http -I$(top_srcdir)/rapidjson $(DAP_CFLAGS)

LIBADD = -L$(top_builddir)/modules/common -lmodules_common -L$(builddir)/../ -lngap \
    $(BES_DISPATCH_LIB) $(top_builddir)/dap/.libs/libdap_module.a $(BES_HTTP_LIB) \
    $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

# $(BES_DAP_LIB) Replace with static library explicitly referenced. jhrg 4/29/24

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

CLEANFILES = *.dbg *.log
# bes.conf
EXTRA_DIST = test_config.h.in cmr_json_responses data
# bes.conf.in bes.cache.conf
check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

# noinst_DATA = bes.cache.conf
# bes.conf
# See target below. jhrg
# $(abs_builddir)/static-cache

BUILT_SOURCES = test_config.h

noinst_HEADERS = test_config.h

# This way of building the header ensures it's in the build dir and that there
# are no '../' sequences in the paths. The BES will reject paths with 'dot dot'
# in them in certain circumstances. jhrg 1/21/18
test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	mod_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" \
	    -e "s%[@]abs_top_srcdir[@]%$${mod_abs_top_srcdir}%" $< > test_config.h

# bes.conf: $(top_srcdir)/configure.ac
# bes.cache.conf: $(top_srcdir)/configure.ac

%.conf: %.conf.in
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

# This may be useful at some point, but the NGAP module does not use
# it currently. jhrg 3/11/20
$(abs_builddir)/static-cache:
	@echo "# abs_top_srcdir: $(abs_top_srcdir)"
	@echo "# top_srcdir: $(top_srcdir)"
	@echo "# srcdir: $(srcdir)"
	@echo "# abs_top_builddir: $(abs_top_builddir)"
	@echo "# abs_builddir: $(abs_builddir)"
	@echo "# builddir: $(builddir)"
	@echo "# Copying preloaded NGAP cache from: '$(top_srcdir)/modules/dmrpp_module/ngap_container/data/ngap_cache_for_tests' to '$(builddir)/static-cache'"
	cp -r $(top_srcdir)/modules/ngap_module/data/ngap_cache_for_tests $(builddir)/static-cache
	chmod -R ug+w $(builddir)/static-cache

############################################################################
# Unit Tests
#

# NgapContainerTest NgapRequestHandlerTest

if CPPUNIT
UNIT_TESTS = MemoryCacheTest NgapApiTest NgapOwnedContainerTest
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
	test ! -d $(builddir)/owned-cache || rm -rf $(builddir)/owned-cache

MemoryCacheTest_SOURCES = MemoryCacheTest.cc
MemoryCacheTest_LDADD = $(LIBADD)

NgapApiTest_SOURCES = NgapApiTest.cc
NgapApiTest_LDADD =  $(LIBADD)

NgapRequestHandlerTest_SOURCES = NgapRequestHandlerTest.cc
NgapRequestHandlerTest_LDADD = $(LIBADD)

NgapOwnedContainerTest_SOURCES = NgapOwnedContainerTest.cc
NgapOwnedContainerTest_LDADD = $(LIBADD)

