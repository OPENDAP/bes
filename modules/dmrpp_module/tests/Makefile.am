
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)

noinst_DATA = bes.conf bes_mds_tests.conf mds_for_tests

CLEANFILES = bes.conf bes.log $(srcdir)/package.m4 $(TESTSUITE) \
bes_mds_tests.conf bes_serial.conf $(TESTSUITE_MDS) $(TESTSUITE_S3) $(TESTSUITE_DMRPP)

EXTRA_DIST = contiguous chunked new_types mds_tests s3 $(srcdir)/package.m4 \
$(TESTSUITE) $(TESTSUITE).at $(TESTSUITE_MDS) $(TESTSUITE_MDS).at \
$(TESTSUITE_S3)  $(TESTSUITE_S3).at \
$(TESTSUITE_DMRPP) $(TESTSUITE_DMRPP).at \
atlocal.in bes.conf.in bes_mds_tests.conf.in  bes_serial.conf.in

DISTCLEANFILES = atconfig mds_for_tests_ledger.txt

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
%.conf : %.conf.in
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

bes.conf: $(top_srcdir)/configure.ac $(srcdir)/bes.conf.in

bes_serial.conf: $(top_srcdir)/configure.ac $(srcdir)/bes_serial.conf.in

bes_mds_tests.conf: $(top_srcdir)/configure.ac $(srcdir)/bes_mds_tests.conf.in

$(builddir)/mds_for_tests:
	cp -r $(top_srcdir)/modules/dmrpp_module/data/mds_for_tests $(builddir)/mds_for_tests
	chmod -R a+w $(builddir)/mds_for_tests
	(cd $(builddir)/mds_for_tests && \
	 clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	 for d in *.in; \
		do sed -e "s%[@]abs_top_srcdir[@]%$${clean_abs_top_srcdir}%" $$d > `basename $$d .in`; \
	 done)

############## Autotest follows #####################

AUTOM4TE = autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

TESTSUITE = $(srcdir)/testsuite

TESTSUITE_MDS = $(srcdir)/testsuite_mds_tests

TESTSUITE_S3 = $(srcdir)/testsuite_s3_tests

TESTSUITE_DMRPP = $(srcdir)/testsuite_dmrpp_tests

s3-tests: $(TESTSUITE_S3) bes.conf bes_serial.conf
	@echo "# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "
	@if [ -n "$${CMAC_ID}" ] \
		&& [ -n "$${CMAC_ACCESS_KEY}" ] \
		&& [ -n "$${CMAC_REGION}" ]  \
		&& [ -n "$${CMAC_BUCKET}" ]   \
		&& [ -n "$${CMAC_URL}" ] ; then \
		echo "--- S3 Auth Tests ---"; \
		$(SHELL) '$(TESTSUITE_S3)' $(TESTSUITEFLAGS); \
	else \
		echo "No access credentials available, skipping S3 authentication tests"; \
		true;\
	fi
	@echo "# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "

check-local: atconfig atlocal $(TESTSUITE) $(TESTSUITE_MDS) $(TESTSUITE_S3) $(TESTSUITE_DMRPP) bes.conf bes_serial.conf bes_mds_tests.conf mds_for_tests
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS) --conf=bes_serial.conf
	$(SHELL) '$(TESTSUITE_MDS)' $(TESTSUITEFLAGS) --conf=bes_mds_tests.conf
	$(SHELL) '$(TESTSUITE_DMRPP)' $(TESTSUITEFLAGS)
	@echo "# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "
	@if [ -n "$${CMAC_ID}" ] \
		&& [ -n "$${CMAC_ACCESS_KEY}" ] \
		&& [ -n "$${CMAC_REGION}" ]  \
		&& [ -n "$${CMAC_BUCKET}" ]   \
		&& [ -n "$${CMAC_URL}" ] ; then \
		echo "--------- S3 Auth Tests ---------"; \
		echo "        CMAC_ID: <Has Been Set>"; \
		echo "CMAC_ACCESS_KEY: <Has Been Set>"; \
		echo "    CMAC_REGION: ${CMAC_REGION}"; \
		echo "    CMAC_BUCKET: ${CMAC_BUCKET}"; \
		echo "       CMAC_URL: ${CMAC_URL}"; \
		$(SHELL) '$(TESTSUITE_S3)' $(TESTSUITEFLAGS); \
	else \
		echo "No access credentials available, skipping S3 authentication tests"; \
		true;\
	fi
	@echo "# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	test ! -f '$(TESTSUITE_MDS)' || $(SHELL) '$(TESTSUITE_MDS)' --clean
	test ! -f '$(TESTSUITE_S3)' || $(SHELL) '$(TESTSUITE_S3)' --clean
	test ! -f '$(TESTSUITE_DMRPP)' || $(SHELL) '$(TESTSUITE_DMRPP)' --clean
	test ! -d $(builddir)/mds_for_tests || rm -rf $(builddir)/mds_for_tests

%: %.at 
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4 $(top_srcdir)/modules/handler_tests_macros.m4

$(TESTSUITE_MDS): $(TESTSUITE_MDS).at $(srcdir)/package.m4 $(top_srcdir)/modules/handler_tests_macros.m4

$(TESTSUITE_S3): $(TESTSUITE_S3).at $(srcdir)/package.m4 $(top_srcdir)/modules/handler_tests_macros.m4

$(TESTSUITE_DMRPP): $(TESTSUITE_DMRPP).at $(srcdir)/package.m4 $(top_srcdir)/modules/handler_tests_macros.m4

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

