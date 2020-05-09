
AUTOMAKE_OPTIONS = foreign

noinst_DATA = bes.conf

CLEANFILES = bes.conf

EXTRA_DIST = $(TESTSUITE_TIF).at $(TESTSUITE_TIF) atlocal.in \
$(TESTSUITE_JP2).at $(TESTSUITE_JP2) $(BES_CONF_IN) package.m4 gdal

DISTCLEANFILES = atconfig

BES_CONF_IN = bes.conf.modules.in

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`python -c "import os; print os.path.abspath('${abs_top_srcdir}')"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE_TIF = $(srcdir)/testsuite_tif
TESTSUITE_JP2 = $(srcdir)/testsuite_jp2

check-local: atconfig atlocal bes.conf $(TESTSUITE_TIF) $(TESTSUITE_JP2)
	$(SHELL) '$(TESTSUITE_TIF)' $(TESTSUITE_TIFFLAGS)
	test ! -f '$(TESTSUITE_JP2)' || $(SHELL) '$(TESTSUITE_JP2)' $(TESTSUITE_JP2FLAGS)

clean-local:
	test ! -f '$(TESTSUITE_TIF)' || $(SHELL) '$(TESTSUITE_TIF)' --clean
	test ! -f '$(TESTSUITE_JP2)' || $(SHELL) '$(TESTSUITE_JP2)' --clean
	-rm -f $(TESTSUITE_TIF) $(TESTSUITE_JP2) $(srcdir)/package.m4

installcheck-local: $(TESTSUITE_TIF) $(TESTSUITE_JP2) atconfig atlocal cache
	$(SHELL) '$(TESTSUITE_TIF)' AUTOTEST_PATH='$(bindir)' $(TESTSUITE_TIFFFLAGS)
	$(SHELL) '$(TESTSUITE_JP2)' AUTOTEST_PATH='$(bindir)' $(TESTSUITE_JP2FLAGS)

AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE_TIF): $(TESTSUITE_TIF).at $(srcdir)/package.m4 $(srcdir)/handler_tests_macros.m4
$(TESTSUITE_JP2): $(TESTSUITE_JP2).at $(srcdir)/package.m4 $(srcdir)/handler_tests_macros.m4

%: %.at
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

