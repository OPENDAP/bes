
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch 
AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA = bes.conf $(builddir)/data

TESTS = $(TESTSUITE)

CLEANFILES = bes.conf

# NB: Make sure to distribute both the source and generated files
# for the test
EXTRA_DIST = $(TESTSUITE) $(TESTSUITE).at atlocal.in \
bes.conf.in handler_tests_macros.m4 bescmd $(srcdir)/package.m4

DISTCLEANFILES = atconfig

BES_CONF_IN = bes.conf.in

$(builddir)/data:
	cp -r $(top_srcdir)/dapreader/data $(builddir)/data
	chmod -R a+w $(builddir)/data

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = testsuite

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 
	-rm -rf $(builddir)/data

AUTOTEST = $(AUTOM4TE) --language=autotest

# Switch back to building the package.m4 here in the makefile
# since that gives better control over things like PACKAGE_NAME,
# which will be handy in the handlers to keep their package names
# from all being 'bes'.
$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4 $(srcdir)/handler_tests_macros.m4
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
