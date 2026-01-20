# Tests

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/aws -I$(top_srcdir)/dispatch

# Help the link step find libs at build time too (not just run time).
# NB: We could use the default value of AM_LDFLAGS for the AWS libs if we wanted
# to, but before we had configure.ac set to include the the deps lib dir
# we used special code just for the AWS library. I'll keep that for now.
# jhrg 1/7/26
AM_LDFLAGS = -L$(aws_libdir)

# This should be 'AM_LDADD' but for some reason the check_PROGRAMS variable doesn't work
# like bin_PROGRAMS.
# NB: aws_libs is set in configure.ac. jhrg 10/16/25

LDADD = $(top_builddir)/dispatch/libbes_dispatch.la $(top_builddir)/aws/libbes_aws.la $(aws_libs)

# aws_libdir is set in configure.ac. jhrg 10/16/25
# aws_libdir64 is also set in configure.ac sbl 11/14/25

if DARWIN
AM_LDFLAGS += -Wl,-rpath,$(aws_libdir) -Wl,-rpath,@loader_path
else
AM_LDFLAGS += -Wl,-rpath,$(aws_libdir) -Wl,-rpath,'$$ORIGIN'
    ifeq ($(strip $(aws_libdir64)),)
        $(info "aws_libdir64 not set, skipping")
    else
        AM_LDFLAGS += -L$(aws_libdir64)
        AM_LDFLAGS += -Wl,-rpath,$(aws_libdir64)
    endif
endif

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS) -Wno-deprecated-declarations
LDADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror

AM_CXXFLAGS =
# See above. jhrg 10/10/25 AM_LDFLAGS =
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

UNIT_TESTS = AWS_SDK_Test

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
AWS_SDK_Test_LDADD = $(LDADD)
