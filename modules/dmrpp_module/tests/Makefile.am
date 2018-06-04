
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)

noinst_DATA = bes.conf bes_mds_tests.conf mds_for_tests

CLEANFILES = bes.conf bes.log $(srcdir)/package.m4 $(TESTSUITE) \
bes_mds_tests.conf $(TESTSUITE_MDS)

EXTRA_DIST = contiguous chunked new_types $(srcdir)/package.m4 atlocal.in \
$(TESTSUITE) $(TESTSUITE).at bes.conf.in $(TESTSUITE_MDS) $(TESTSUITE_MDS).at \
bes_mds_tests.conf.in mds_tests

DISTCLEANFILES = atconfig 

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
%.conf : %.conf.in
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

bes.conf: $(top_srcdir)/configure.ac $(srcdir)/bes.conf.in

bes_mds_tests.conf: $(top_srcdir)/configure.ac $(srcdir)/bes_mds_tests.conf.in

$(builddir)/mds_for_tests:
	cp -r $(top_srcdir)/modules/dmrpp_module/data/mds_for_tests $(builddir)/mds_for_tests
	(cd $(builddir)/mds_for_tests \
	&& chmod -R a+w * \
	&& for d in *.in; do sed -e "s%[@]abs_top_srcdir[@]%${abs_top_srcdir}%" $$d > `basename $$d .in`; done)

############## Autotest follows #####################

AUTOM4TE = autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

TESTSUITE = $(srcdir)/testsuite

TESTSUITE_MDS = $(srcdir)/testsuite_mds_tests

check-local: atconfig atlocal $(TESTSUITE) $(TESTSUITE_MDS) bes.conf bes_mds_tests.conf mds_for_tests
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)
	$(SHELL) '$(TESTSUITE_MDS)' $(TESTSUITEFLAGS) --conf=bes_mds_tests.conf 

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	test ! -f '$(TESTSUITE_MDS)' || $(SHELL) '$(TESTSUITE_MDS)' --clean
	test ! -d $(builddir)/mds_for_tests || rm -rf $(builddir)/mds_for_tests

%: %.at 
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

$(TESTSUITE): $(srcdir)/package.m4 $(top_srcdir)/modules/handler_tests_macros.m4

$(TESTSUITE_MDS): $(srcdir)/package.m4 $(top_srcdir)/modules/handler_tests_macros.m4

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

