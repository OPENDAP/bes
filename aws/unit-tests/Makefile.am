# Tests

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/http -I$(top_srcdir)/aws -I$(top_srcdir)/dispatch
LIBADD = ../.libs/libbes_aws.a $(AWS_SDK_LIBS) $(BES_HTTP_LIB)

# $(BES_DISPATCH_LIB)

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

DISTCLEANFILES = test_config.h

CLEANFILES = *.dbg *.log

EXTRA_DIST = test_config.h.in

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

noinst_DATA =

BUILT_SOURCES = test_config.h

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

%.conf: %.conf.in
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

############################################################################
# Unit Tests
#

if CPPUNIT

UNIT_TESTS =  AWS_SDK_Test

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

AWS_SDK_Test_SOURCES = AWS_SDK_Test.cc
AWS_SDK_Test_LDADD = $(LIBADD)
