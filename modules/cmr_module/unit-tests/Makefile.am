# Tests

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/modules -I$(top_srcdir)/modules/cmr_module \
-I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/http $(DAP_CFLAGS)

# Added -lz for ubuntu
STATIC_DAP_MODULE = $(top_builddir)/dap/.libs/libdap_module.a
LIBADD = $(BES_DISPATCH_LIB) $(STATIC_DAP_MODULE) $(BES_HTTP_LIB) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) -lz

# jhrg 6/2/23 $(BES_EXTRA_LIBS)

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

EXTRA_DIST = test_config.h.in bes.conf.in 

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

noinst_DATA = bes.conf $(abs_builddir)/static-cache 

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

BES_CONF_IN = bes.conf.in

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@echo "# abs_top_srcdir: $(abs_top_srcdir)" 
	@echo "# top_srcdir: $(top_srcdir)" 
	@echo "# srcdir: $(srcdir)" 
	@echo "# abs_top_builddir: $(abs_top_builddir)" 
	@echo "# abs_builddir: $(abs_builddir)" 
	@echo "# builddir: $(builddir)" 
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

$(abs_builddir)/static-cache:
	@echo "# abs_top_srcdir: $(abs_top_srcdir)" 
	@echo "# top_srcdir: $(top_srcdir)" 
	@echo "# srcdir: $(srcdir)" 
	@echo "# abs_top_builddir: $(abs_top_builddir)" 
	@echo "# abs_builddir: $(abs_builddir)" 
	@echo "# builddir: $(builddir)" 
	@echo "# Coping preloaded CMR cache from: '$(top_srcdir)/modules/cmr_module/data/cmr_cache_for_tests' to '$(builddir)/static-cache'" 
	cp -r $(top_srcdir)/modules/cmr_module/data/cmr_cache_for_tests $(builddir)/static-cache
	chmod -R ug+w $(builddir)/static-cache

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = # CmrCatalogTest GranuleTest # FIXME disabled 5/16/24 because target collection was deleted.
# This fails often: CmrApiTest jhrg 6/29/23
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

OBJS = ../JsonUtils.o ../CmrApi.o \
       ../Provider.o ../Collection.o ../Granule.o ../GranuleUMM.o \
       ../CmrCatalog.o

CmrApiTest_SOURCES = CmrApiTest.cc
CmrApiTest_LDADD = $(OBJS) $(LIBADD)

CmrCatalogTest_SOURCES = CmrCatalogTest.cc
CmrCatalogTest_LDADD = $(OBJS) $(LIBADD)

GranuleTest_SOURCES = GranuleTest.cc
GranuleTest_LDADD = $(OBJS) $(LIBADD)
