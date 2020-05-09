
AUTOMAKE_OPTIONS = foreign 

CLEANFILES =

# NB: Make sure to distribute both the source and generated files
# for the test
EXTRA_DIST = $(TESTSUITE) $(TESTSUITE).at $(srcdir)/package.m4 \
atlocal.in ff hdf4 hdf5 nc show ssfunc mds_initial_state

DISTCLEANFILES = atconfig

mds_cachedir = $(datadir)/mds
mds_cache_DATA = $(srcdir)/mds_initial_state/*

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/testsuite

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -f $(TESTSUITE) $(srcdir)/package.m4

check-local: $(TESTSUITE) install-data
	@if bescmdln -x "show version;"; \
		then $(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS); \
		else echo "no bes found, not running cmdln-based tests"; \
	fi

AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4

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
