AUTOMAKE_OPTIONS = foreign

TESTSUITE_TIF = $(srcdir)/testsuite_tif

# if OPENJPEG_FOUND
# TESTSUITE_JP2 = $(srcdir)/testsuite_jp2
# else
TESTSUITE_JP2 = 
# endif

EXTRA_DIST = $(TESTSUITE_TIF).at $(TESTSUITE_TIF) $(srcdir)/package.m4 \
atlocal.in handler_tests_macros.m4 $(BES_CONF_IN) gdal

# if OPENJPEG_FOUND
# EXTRA_DIST += $(TESTSUITE_JP2).at $(TESTSUITE_JP2)
# endif

noinst_DATA = bes.conf

CLEANFILES = bes.conf

DISTCLEANFILES = atconfig

check-local: atconfig atlocal $(TESTSUITE_TIF) $(TESTSUITE_JP2)
	$(SHELL) '$(TESTSUITE_TIF)' $(TESTSUITE_TIFFLAGS)
	test ! -f '$(TESTSUITE_JP2)' || $(SHELL) '$(TESTSUITE_JP2)' $(TESTSUITE_TIFFLAGS)

clean-local:
	test ! -f '$(TESTSUITE_TIF)' || $(SHELL) '$(TESTSUITE_TIF)' --clean
	test ! -f '$(TESTSUITE_JP2)' || $(SHELL) '$(TESTSUITE_JP2)' --clean
	-rm -f $(TESTSUITE_TIF) $(TESTSUITE_JP2) $(srcdir)/package.m4 

BES_CONF_IN = bes.conf.modules.in

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf


# Use the missing script for better error checking on autom4te run.
AUTOM4TE = $(SHELL) $(top_srcdir)/conf/missing --run autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE_TIF): $(TESTSUITE_TIF).at bes.conf $(srcdir)/package.m4 $(srcdir)/handler_tests_macros.m4 
	$(AUTOTEST) -I '$(srcdir)' -o ${srcdir}/$@.tmp $@.at
	mv ${srcdir}/$@.tmp ${srcdir}/$@

$(TESTSUITE_JP2): $(TESTSUITE_JP2).at bes.conf $(srcdir)/package.m4 $(srcdir)/handler_tests_macros.m4
	$(AUTOTEST) -I '$(srcdir)' -o ${srcdir}/$@.tmp $@.at
	mv ${srcdir}/$@.tmp ${srcdir}/$@

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

