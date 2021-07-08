
AUTOMAKE_OPTIONS = foreign

noinst_DATA = bes.conf

CLEANFILES = bes.conf

EXTRA_DIST = bescmd $(TESTSUITE).at $(TESTSUITE) \
$(srcdir)/package.m4 atlocal.in bes.conf.in

DISTCLEANFILES = atconfig

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: bes.conf.in $(top_srcdir)/configure.ac

%.conf: %.conf.in
	@clean_abs_top_srcdir=`python -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/testsuite

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4 $(top_srcdir)/modules/handler_tests_macros.m4

# Note These tests do not work when run in parallel.
check-local: atconfig atlocal $(TESTSUITE)
	@echo "Run tests with an empty cache"
	-rm -rf dap_cache/*
	test `ls -1 dap_cache/ | wc -l` -eq 0
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)
	@echo "Cached responses: `ls -1 dap_cache/ | wc -l`"

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -rf dap_cache/*
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 

AUTOTEST = $(AUTOM4TE) --language=autotest

% : %.at 
	$(AUTOTEST) -I '$(srcdir)' -o $@ $<

# The `:;' works around a Bash 3.2 bug when the output is not writeable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])' && \
	echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	echo 'm4_define([AT_TOP_SRCDIR], 		[@top_srcdir@])'; \
	} >'$(srcdir)/package.m4'
