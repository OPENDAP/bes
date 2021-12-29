
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)

CLEANFILES = $(TESTSUITE_DMRPP)

EXTRA_DIST = $(srcdir)/package.m4 $(TESTSUITE_DMRPP) $(TESTSUITE_DMRPP).at \
atlocal.in build_dmrpp_macros.m4

DISTCLEANFILES = atconfig

############## Autotest follows #####################

AUTOM4TE = autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

TESTSUITE_DMRPP = $(srcdir)/testsuite

check-local: atconfig atlocal $(TESTSUITE_DMRPP)
	$(SHELL) '$(TESTSUITE_DMRPP)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE_DMRPP)' || $(SHELL) '$(TESTSUITE_DMRPP)' --clean

%: %.at 
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

$(TESTSUITE_DMRPP): $(TESTSUITE_DMRPP).at $(srcdir)/package.m4 $(srcdir)/build_dmrpp_macros.m4 $(top_srcdir)/modules/handler_tests_macros.m4

#$(top_srcdir)/modules/dmrpp_module/tests_build_dmrpp/build_dmrpp_macros.m4

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

