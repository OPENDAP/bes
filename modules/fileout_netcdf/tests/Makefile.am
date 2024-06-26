
AUTOMAKE_OPTIONS = foreign

noinst_DATA = bes.conf bes.nc4.conf bes.nc4.grp.conf bes.nc4.cfdmr.conf bes.functions.conf \
bes.size600.conf bes.size50.conf bes.nc4.grp.disable_dio.conf

CLEANFILES = bes.conf bes.nc4.conf bes.nc4.grp.conf bes.nc4.cfdmr.conf bes.functions.conf \
bes.size600.conf bes.size50.conf bes.nc4.grp.disable_dio.conf

EXTRA_DIST = bescmd $(TESTSUITE).at $(TESTSUITE) $(TESTSUITE_FUNC).at $(TESTSUITE_FUNC) \
atlocal.in bes.conf.in bes.nc4.conf.in bes.nc4.grp.conf.in bes.nc4.cfdmr.conf.in bes.functions.conf.in \
package.m4 local_handler_tests_macros.m4 bes.size600.conf.in bes.size50.conf.in bes.nc4.grp.disable_dio.conf.in

DISTCLEANFILES = atconfig

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using
# configure's value for the parameter when running the distcheck target.
%.conf : %.conf.in
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

bes.conf: $(srcdir)/bes.conf.in $(top_srcdir)/configure.ac

bes.nc4.conf: $(srcdir)/bes.nc4.conf.in $(top_srcdir)/configure.ac

bes.nc4.grp.conf: $(srcdir)/bes.nc4.grp.conf.in $(top_srcdir)/configure.ac

bes.nc4.cfdmr.conf: $(srcdir)/bes.nc4.cfdmr.conf.in $(top_srcdir)/configure.ac

bes.functions.conf: $(srcdir)/bes.functions.conf.in $(top_srcdir)/configure.ac

bes.size600.conf: $(srcdir)/bes.size600.conf.in $(top_srcdir)/configure.ac

bes.size50.conf: $(srcdir)/bes.size50.conf.in $(top_srcdir)/configure.ac

bes.nc4.grp.disable_dio.conf: $(srcdir)/bes.nc4.grp.disable_dio.conf.in $(top_srcdir)/configure.ac

############## Autotest follows #####################

AUTOM4TE = autom4te

# The $(srcdir) prefix means that when distcheck is run, the script
# is put in the source directory along with the *.at file used to
# build it, which is important since these scripts are distributed
# in the tar.gz distribution.
TESTSUITE = $(srcdir)/testsuite
# TESTSUITE_NC4 = $(srcdir)/testsuite_nc4
TESTSUITE_FUNC = $(srcdir)/testsuite-functions

# NB: The bes conf files don't appear here because they are built as 'data' above.
check-local: $(TESTSUITE) $(TESTSUITE_FUNC) atconfig atlocal
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)
	$(SHELL) '$(TESTSUITE_FUNC)' --conf=bes.functions.conf $(TESTSUITEFLAGS)

installcheck-local: $(TESTSUITE) $(TESTSUITE_FUNC) atconfig atlocal
	$(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(bindir)' $(TESTSUITEFLAGS)
	$(SHELL) '$(TESTSUITE_FUNC)' AUTOTEST_PATH='$(bindir)' --conf=bes.functions.conf $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	test ! -f '$(TESTSUITE_FUNC)' || $(SHELL) '$(TESTSUITE_FUNC)' --clean
	-rm -f $(TESTSUITE) $(TESTSUITE_FUNC)

AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4 $(top_srcdir)/modules/common/handler_tests_macros.m4 \
$(srcdir)/local_handler_tests_macros.m4

$(TESTSUITE_FUNC): $(TESTSUITE_FUNC).at $(srcdir)/package.m4 $(top_srcdir)/modules/common/handler_tests_macros.m4

%: %.at
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

# The `:;' works around a Bash 3.2 bug when the output is not writable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac $(srcdir)/Makefile.am
	:;{ \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])' && \
	echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])' && \
	echo 'm4_define([AT_TOP_SRCDIR],        [@top_srcdir@])' && \
    echo 'm4_define([AT_PACKAGE_HOST],      [@host@])'; \
	} >'$(srcdir)/package.m4'


