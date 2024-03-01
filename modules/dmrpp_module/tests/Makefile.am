
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)

noinst_DATA = bes.conf

CLEANFILES = bes.conf bes.log bes_serial.conf

# Parallel make clean fail. jhrg 4/13/22 $(TESTSUITE) $(TESTSUITE_S3) $(srcdir)/package.m4

EXTRA_DIST = contiguous chunked compact new_types s3 fill_value deflate_twice string_arrays hdf4\
$(TESTSUITE) $(TESTSUITE).at $(TESTSUITE_S3) $(TESTSUITE_S3).at $(TESTSUITE_DMRPP_BUILDER) \
$(TESTSUITE_DMRPP_BUILDER).at atlocal.in bes.conf.in bes_serial.conf.in $(srcdir)/package.m4 \
multi_input_tests.m4

DISTCLEANFILES = atconfig

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using the
# configure script's value for the parameter when running the distcheck target.
%.conf : %.conf.in
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

bes.conf: $(top_srcdir)/configure.ac $(srcdir)/bes.conf.in

bes_serial.conf: $(top_srcdir)/configure.ac $(srcdir)/bes_serial.conf.in

############## Autotest follows #####################

AUTOM4TE = autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

TESTSUITE = $(srcdir)/testsuite

TESTSUITE_S3 = $(srcdir)/testsuite_s3_tests

TESTSUITE_DMRPP_BUILDER = $(srcdir)/testsuite_dmrpp_builder

check-local: atconfig atlocal $(TESTSUITE) $(TESTSUITE_S3) $(TESTSUITE_DMRPP_BUILDER) bes.conf bes_serial.conf
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS) --conf=bes_serial.conf
	@echo "# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "
	@if [ -n "$${CMAC_ID}" ] \
		&& [ -n "$${CMAC_ACCESS_KEY}" ] \
		&& [ -n "$${CMAC_REGION}" ]  \
		&& [ -n "$${CMAC_URL}" ] ; then \
		echo "--------- S3 Auth Tests ---------"; \
		echo "        CMAC_ID: <Has Been Set>"; \
		echo "CMAC_ACCESS_KEY: <Has Been Set>"; \
		echo "    CMAC_REGION: ${CMAC_REGION}"; \
		echo "       CMAC_URL: ${CMAC_URL}"; \
		$(SHELL) '$(TESTSUITE_S3)' $(TESTSUITEFLAGS); \
	else \
		echo "No access credentials available, skipping S3 authentication tests"; \
		true;\
	fi
	@echo "# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	test ! -f '$(TESTSUITE_S3)' || $(SHELL) '$(TESTSUITE_S3)' --clean

%: %.at 
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4 $(top_srcdir)/modules/common/handler_tests_macros.m4 multi_input_tests.m4

$(TESTSUITE_S3): $(TESTSUITE_S3).at $(srcdir)/package.m4 $(top_srcdir)/modules/common/handler_tests_macros.m4

$(TESTSUITE_DMRPP_BUILDER): $(TESTSUITE_DMRPP_BUILDER).at $(srcdir)/package.m4 $(top_srcdir)/modules/common/handler_tests_macros.m4

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

