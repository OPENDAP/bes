
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)
AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA = bes.conf bes.nc4.conf bes.nc4.grp.conf

CLEANFILES = bes.conf bes.nc4.conf bes.nc4.grp.conf

EXTRA_DIST = bescmd $(TESTSUITE).at $(TESTSUITE)  \
atlocal.in bes.conf.in bes.nc4.conf.in bes.nc4.grp.conf.in package.m4 \
local_handler_tests_macros.m4

# I moved the tests back to one file and dropped the 'pass the alternate
# conf file in using --conf=...' because that was not working with ncdump
# for an unknown reason. jhrg 3/19/20

# $(TESTSUITE_NC4).at $(TESTSUITE_NC4)

DISTCLEANFILES = atconfig

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using
# configure's value for the parameter when running the distcheck target.
%.conf : %.conf.in
	@clean_abs_top_srcdir=`python -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

bes.conf: $(srcdir)/bes.conf.in $(top_srcdir)/configure.ac

bes.nc4.conf: $(srcdir)/bes.nc4.conf.in $(top_srcdir)/configure.ac

bes.nc4.grp.conf: $(srcdir)/bes.nc4.grp.conf.in $(top_srcdir)/configure.ac

bes.functions.conf: $(srcdir)/bes.functions.conf.in $(top_srcdir)/configure.ac

############## Autotest follows #####################

AUTOM4TE = autom4te

# The $(srcdir) prefix means that when distcheck is run, the script
# is put in the source directory along with the *.at file used to
# build it, which is important since these scripts are distributed
# in the tar.gz distribution.
TESTSUITE = $(srcdir)/testsuite
# TESTSUITE_NC4 = $(srcdir)/testsuite_nc4
TESTSUITE_FUNC = $(srcdir)/testsuite-functions

check-local: $(TESTSUITE) $(TESTSUITE_FUNC) atconfig atlocal
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)
	$(SHELL) '$(TESTSUITE_FUNC) --conf=bes.functions.conf' $(TESTSUITEFLAGS)

installcheck-local: $(TESTSUITE) atconfig atlocal 
	$(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(bindir)' $(TESTSUITEFLAGS)
	$(SHELL) '$(TESTSUITE_FUNC)' AUTOTEST_PATH='$(bindir)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -f $(TESTSUITE)
	test ! -f '$(TESTSUITE_FUNC)' || $(SHELL) '$(TESTSUITE_FUNC)' --clean
	-rm -f $(TESTSUITE_FUNC)

AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4 $(top_srcdir)/modules/handler_tests_macros.m4 \
$(srcdir)/local_handler_tests_macros.m4

$(TESTSUITE_FUNC): $(TESTSUITE_FUNC).at $(srcdir)/package.m4 $(top_srcdir)/modules/handler_tests_macros.m4

%: %.at
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

# The `:;' works around a Bash 3.2 bug when the output is not writable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])' && \
	echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >'$(srcdir)/package.m4'


