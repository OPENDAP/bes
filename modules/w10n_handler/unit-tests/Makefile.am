# Tests

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap \
-I$(top_srcdir)/modules/w10n_handler $(DAP_CFLAGS)
LIBADD = $(BES_DISPATCH_LIB) $(BES_EXTRA_LIBS) $(DAP_SERVER_LIBS)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LIBADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

DISTCLEANFILES =

CLEANFILES = *.dbg *.log test_config.h

EXTRA_DIST = test_config.h.in baselines

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

# Adding the tmp dir here causes make to put it in the buld dir
noinst_DATA = tmp testdir

BUILT_SOURCES = test_config.h

noinst_HEADERS = test_config.h

# This way of building the header ensures it's in the build dir and that there
# are no '../' seqeunces in the paths. The BES will reject paths with 'dot dot'
# in them in certain circumstances. jhrg 1/21/18
test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	mod_abs_builddir=`echo ${abs_builddir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

tmp:
	test -d tmp || mkdir tmp
	
# If 'testdir' does not exist, replicate it here.
# tar (used to make the source dist) dereferences symbolic links, so
# we make them this way to get distcheck to work.
testdir:
	test -d testdir || mkdir -p testdir/nc
	(cd testdir/nc && touch testfile.txt && ln -s testfile.txt link_to_testfile.txt && ln -s nothing bad_link)
	(cd testdir && ln -s nc link_to_nc)
	
clean-local:
	-rm -rf tmp
	-rm -rf testdir

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = W10nTest
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

OBJS = ../w10n_utils.o

W10nTest_SOURCES = W10nTest.cc
W10nTest_LDADD = $(OBJS) $(LIBADD)
