
AUTOMAKE_OPTIONS = foreign 

if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --trace-children=yes --error-exitcode=1 --dsymutil=yes \
--leak-check=yes  --max-stackframe=3000000  --quiet
endif

AM_CPPFLAGS = -I$(top_srcdir)
AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA = bes.conf

CLEANFILES = *.log *.sum gmon.out

EXTRA_DIST = data bescmd functionsTest.at $(TESTSUITE) \
atlocal.in package.m4.in bes.conf.in bes.conf.modules.in handler_tests_macros.m4

DISTCLEANFILES = bes.conf

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/functionsTest
TESTSUITEFLAGS = -j7

$(TESTSUITE): $(srcdir)/handler_tests_macros.m4 $(srcdir)/package.m4 $(srcdir)/functionsTest.at

check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean

distclean-local:
	-rm atconfig

AUTOTEST = $(AUTOM4TE) --language=autotest

% : %.at 
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at
