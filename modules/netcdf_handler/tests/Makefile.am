
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS =
AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA = bes.conf

CLEANFILES = bes.conf

EXTRA_DIST = nc netcdf_handlerTest netcdf_handlerTest.at \
nc4_netcdf_handler_tests nc4_netcdf_handler_tests.at \
atlocal.in package.m4 bes.conf.in bes.conf.modules.in \
handler_tests_macros.m4

DISTCLEANFILES = atconfig

if DAP_MODULES
BES_CONF_IN = bes.conf.modules.in
else
BES_CONF_IN = bes.conf.in
endif

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE_3 = $(srcdir)/netcdf_handlerTest

if NETCDF4_TESTS
TESTSUITE_4 = $(srcdir)/nc4_netcdf_handler_tests
else
TESTSUITE_4 =
endif

# TESTSUITE_3 is always built, but not TESTSUITE_4

check-local: atconfig atlocal $(srcdir)/package.m4 $(TESTSUITE_3) $(TESTSUITE_4) \
$(srcdir)/handler_tests_macros.m4
	$(SHELL) '$(TESTSUITE_3)' $(TESTSUITEFLAGS)
	test ! -f '$(TESTSUITE_4)' || $(SHELL) '$(TESTSUITE_4)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE_3)' || $(SHELL) '$(TESTSUITE_3)' --clean
	test ! -f '$(TESTSUITE_4)' || $(SHELL) '$(TESTSUITE_4)' --clean
	
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

