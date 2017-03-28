
# AUTOMAKE_OPTIONS = foreign

TESTSUITE = $(srcdir)/testsuite

CLEANFILES = bes.conf

EXTRA_DIST = baselines $(TESTSUITE).at $(TESTSUITE)		\
	$(srcdir)/package.m4 atlocal.in template.bescmd.in	\
	bes.conf.in bes.conf.modules.in

noinst_DATA = bes.conf

DISTCLEANFILES = atconfig

dist_check_SCRIPTS = $(TESTSUITE) atconfig atlocal

check-local: atconfig atlocal $(TESTSUITE) bes.conf
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

if DAP_MODULES
BES_CONF_IN = bes.conf.modules.in
else
BES_CONF_IN = bes.conf.in
endif

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
bes.conf: $(BES_CONF_IN) $(top_srcdir)/configure.ac
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > bes.conf

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

$(TESTSUITE): $(srcdir)/package.m4  $(srcdir)/testsuite.at bes.conf $(TEST_FILES)
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


