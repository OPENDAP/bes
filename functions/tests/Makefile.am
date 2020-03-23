
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

CLEANFILES = bes.conf

EXTRA_DIST = data bescmd functions $(TESTSUITE).at $(TESTSUITE) \
$(srcdir)/package.m4 atlocal.in bes.conf.in

DISTCLEANFILES = atconfig generate_data_baselines.sh generate_metadata_baselines.sh

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using
# configure's value for the parameter when running the distcheck target.
%.conf : %.conf.in
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

bes.conf: bes.conf.in $(top_srcdir)/configure.ac

data/build_responses.conf: data/build_responses.conf.in $(top_srcdir)/configure.ac

# This is a more general tool for managing cache contents for tests.
# This code only needs to clean out the cache.
# cleanup.sh: cleanup.sh.in ../../config.status
#	sed -e "s%[@]srcdir[@]%${srcdir}%" $< > cleanup.sh
#	chmod +x cleanup.sh

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/testsuite
TESTSUITEFLAGS =

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4 $(srcdir)/../../modules/handler_tests_macros.m4

# Note These tests do not work when run in parallel.
check-local: atconfig atlocal $(TESTSUITE)
	@echo "Run tests with an empty cache"
	-rm -rf dap_cache/*
	test `ls -1 dap_cache/ | wc -l` -eq 0
	$(SHELL) '$(TESTSUITE)' # $(TESTSUITEFLAGS)
	-test `ls -1 dap_cache/ | wc -l` -eq 52
	@echo "These tests will mysteriously fail if function caching is turned off"
	@echo "Run tests with a full cache"
	$(SHELL) '$(TESTSUITE)' # $(TESTSUITEFLAGS)
	-test `ls -1 dap_cache/ | wc -l` -eq 52
	
clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -rf dap_cache/*
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 

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
