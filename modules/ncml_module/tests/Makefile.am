
AUTOMAKE_OPTIONS = foreign

TESTSUITE = $(srcdir)/testsuite
# Run the tests in parallel, which speeds things up considerably.

TEST_FILES = gateway_tests.at aggregations.at attribute_tests.at parse_error_misc.at	\
variable_misc.at variable_new_arrays.at variable_new_multi_arrays.at	\
variable_new_scalars.at variable_new_structures.at variable_remove.at	\
variable_rename.at gateway_tests.at handler_tests.at dmrpp_tests.at

EXTRA_DIST = $(TESTSUITE).at $(TEST_FILES) $(srcdir)/package.m4 \
$(TESTSUITE) atlocal.in template.bescmd.in bes.conf.in bes.gdal.conf.in \
baselines cache

if BUILD_GDAL
BES_CONF = bes.gdal.conf
else
BES_CONF = bes.conf
endif

noinst_DATA = $(BES_CONF)

CLEANFILES = $(BES_CONF)

DISTCLEANFILES = atconfig cache/* mds_ledger.txt

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
%.conf: %.conf.in $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

check-local: atconfig atlocal $(srcdir)/package.m4 $(TESTSUITE) $(BES_CONF)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS) --conf=$(BES_CONF)

# Technically, we don't need this target now, but it might be useful
# to add it later to be complete.  It would only need to run
# besstandalone on the INSTALLED bes.conf file after it was processed
# with 'make bes-conf' to add the NCML module.  We'd have to figure
# out a way to let the testsuite script know whether to use the local
# bes.conf or the installed one.  
# installcheck-local: atconfig atlocal
# $(TESTSUITE) $(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(bindir)' \
# $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 

# Use the missing script for better error checking on autom4te run.
AUTOM4TE = $(SHELL) $(top_srcdir)/conf/missing --run autom4te

AUTOTEST = $(AUTOM4TE) --language=autotest

% : %.at $(srcdir)/package.m4 $(TEST_FILES)
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

# Generate the m4 defines from top level configure.ac
# The `:;' works around a Bash 3.2 bug when the output is not writable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],' && \
	echo '  [@PACKAGE_NAME@])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],' && \
	echo '  [@PACKAGE_TARNAME@])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],' && \
	echo '  [@PACKAGE_VERSION@])' && \
	echo 'm4_define([AT_PACKAGE_STRING],' && \
	echo '  [@PACKAGE_STRING@])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT],' && \
	echo '  [@PACKAGE_BUGREPORT@])'; \
	} >'$(srcdir)/package.m4'
