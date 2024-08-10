
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)

AM_CXXFLAGS =

BES_CONF_IN = bes.conf.in
BES_DAAC_CONF_IN = bes-daac.conf.in

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA = bes.conf bes-daac.conf

CLEANFILES = bes.conf bes-daac.conf

EXTRA_DIST = ngap $(TESTSUITE).at $(TESTSUITE) $(TESTSUITE_DAAC).at $(TESTSUITE_DAAC) atlocal.in \
CM.config.template package.m4 $(BES_CONF_IN) $(BES_DAAC_CONF_IN)

DISTCLEANFILES = atconfig

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

bes-daac.conf: $(BES_DAAC_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes-daac.conf

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/testsuite
TESTSUITE_DAAC = $(srcdir)/testsuite-daac

check-local: atconfig atlocal $(srcdir)/package.m4 $(TESTSUITE) $(TESTSUITE_DAAC) bes.conf bes-daac.conf
	@if [ -f $(srcdir)/CM.config ]; then \
	    $(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS); \
	    $(SHELL) '$(TESTSUITE_DAAC)' $(TESTSUITEFLAGS) --conf bes-daac.conf; \
	else \
	    echo "No access credentials available, skipping tests that use S3"; \
	    true;\
	fi

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	test ! -f '$(TESTSUITE_DAAC)' || $(SHELL) '$(TESTSUITE_DAAC)' --clean
	-rm -f $(TESTSUITE) $(TESTSUITE_DAAC) $(srcdir)/package.m4

AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4 $(top_srcdir)/modules/common/handler_tests_macros.m4
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

$(TESTSUITE_DAAC): $(TESTSUITE_DAAC).at $(srcdir)/package.m4 $(top_srcdir)/modules/common/handler_tests_macros.m4
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

