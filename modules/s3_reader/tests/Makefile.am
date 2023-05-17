
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)
AM_CXXFLAGS =

BES_CONF_IN = bes.conf.in

noinst_DATA = bes.conf

CLEANFILES = bes.conf

EXTRA_DIST = s3 $(TESTSUITE).at $(TESTSUITE) atlocal.in \
$(srcdir)/package.m4 $(BES_CONF_IN)

DISTCLEANFILES = atconfig

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/testsuite

s3_reader_region = us-west-2
s3_reader_url = https://s3-module-test-bucket.s3.us-west-2.amazonaws.com

check-local: atconfig atlocal $(srcdir)/package.m4 $(TESTSUITE)
	@echo "# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "
	@if [ -n "$${CMAC_ID}" ] \
		&& [ -n "$${CMAC_ACCESS_KEY}" ] \
		&& [ -n "$${CMAC_REGION}" -a $${CMAC_REGION} = $(s3_reader_region) ]  \
		&& [ -n "$${CMAC_URL}" -a $${CMAC_URL} = $(s3_reader_url) ] ; then \
		echo "--------- S3 Auth Tests ---------"; \
		echo "        CMAC_ID: <Has Been Set>"; \
		echo "CMAC_ACCESS_KEY: <Has Been Set>"; \
		echo "    CMAC_REGION: ${CMAC_REGION}"; \
		echo "       CMAC_URL: ${CMAC_URL}"; \
		$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS); \
	else \
		echo "No access credentials available, skipping S3 authentication tests"; \
		true;\
	fi
	@echo "# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - "

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 

AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4 $(top_srcdir)/modules/common/handler_tests_macros.m4

% : %.at
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
	echo 'm4_define([AT_TOP_SRCDIR],        [@top_srcdir@])'; \
	} >'$(srcdir)/package.m4'

