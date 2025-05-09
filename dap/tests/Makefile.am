
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch 
AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA = bes.conf bes.annotation.conf

# Using this will result in a summary of tests while the check-local
# target will show the individual tests. Defining both targets will
# produce both outputs. jhrg 2/10/15
#
# TESTS = testsuite

CLEANFILES = bes.conf bes.annotation.conf

# NB: Make sure to distribute both the source and generated files
# for the test
EXTRA_DIST = $(TESTSUITE) $(TESTSUITE).at $(TESTSUITE2) $(TESTSUITE2).at \
atlocal.in bes.conf.in bes.annotation.conf.in handler_tests_macros.m4 \
$(srcdir)/package.m4 bescmd

DISTCLEANFILES = atconfig

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.

%.conf: %.conf.in
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

bes.conf: $(top_srcdir)/configure.ac
bes.annotation.conf: $(top_srcdir)/configure.ac

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/testsuite

TESTSUITE2 = $(srcdir)/annotation_testsuite

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	test ! -f '$(TESTSUITE2)' || $(SHELL) '$(TESTSUITE2)' --clean
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 

check-local: $(TESTSUITE) $(TESTSUITE2) bes.conf bes.annotation.conf
	$(SHELL) $(TESTSUITE)
	$(SHELL) $(TESTSUITE2) --conf bes.annotation.conf

AUTOTEST = $(AUTOM4TE) --language=autotest

# Switch back to building the package.m4 here in the makefile
# since that gives better control over things like PACKAGE_NAME,
# which will be handy in the handlers to keep their package names
# from all being 'bes'.

$(TESTSUITE): $(srcdir)/package.m4 $(srcdir)/handler_tests_macros.m4
$(TESTSUITE2): $(srcdir)/package.m4 $(srcdir)/handler_tests_macros.m4

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
