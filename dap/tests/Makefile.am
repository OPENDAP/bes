
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch 
AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA = bes.conf

# Using this will result in a summary of tests while the check-local
# target will show the individual tests. Defining both targets will
# produce both outputs. jhrg 2/10/15
#
TESTS = besDapModuleTest

CLEANFILES = *.log *.trs

EXTRA_DIST = besDapModuleTest.at atlocal.in package.m4.in bes.conf.in

DISTCLEANFILES = bes.conf

############## Autotest follows #####################

AUTOM4TE = autom4te
 
# TESTSUITE = $(srcdir)/besdap_moduleTest
TESTSUITE = $(srcdir)/besDapModuleTest

# check-local: atconfig atlocal $(TESTSUITE)
#	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean

distclean-local:
	-rm atconfig

AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE): $(srcdir)/besDapModuleTest.at
	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
	mv $@.tmp $@
