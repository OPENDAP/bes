
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)

noinst_DATA = bes.conf bes.mds.conf mds

CLEANFILES = bes.conf bes.mds.conf mds_ledger.txt bes.log $(srcdir)/package.m4 \
$(TESTSUITE) 

EXTRA_DIST = dap $(TESTSUITE) $(TESTSUITE).at atlocal.in \
$(srcdir)/handler_tests_macros.m4 bes.conf.in bes.mds.conf.in \
$(srcdir)/package.m4

DISTCLEANFILES = atconfig

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using 
# configure's value for the parameter when running the distcheck target.
%.conf : %.conf.in
	@clean_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

bes.conf: $(top_srcdir)/configure.ac

bes.mds.conf: $(top_srcdir)/configure.ac

mds:
	test -d mds || mkdir -p mds

############## Autotest follows #####################

AUTOM4TE = autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

TESTSUITE = $(srcdir)/testsuite

# MDS_TESTSUITE = $(srcdir)/mds_testsuite

check-local: atconfig atlocal $(TESTSUITE) $(MDS_TESTSUITE) bes.mds.conf bes.conf
	@echo "Test without the MDS"
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)
	@echo "Test using the MDS"
	test -d $(builddir)/mds && rm $(builddir)/mds/*
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS) --jobs=1 --conf=bes.mds.conf 

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -rf $(builddir)/mds

%: %.at 
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at
$(TESTSUITE): $(srcdir)/package.m4 $(srcdir)/handler_tests_macros.m4

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

