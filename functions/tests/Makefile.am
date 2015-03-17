
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
atlocal.in bes.conf.in

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

# The `:;' works around a Bash 3.2 bug when the output is not writeable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])' && \
	echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >'$(srcdir)/package.m4'
	