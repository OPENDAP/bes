
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)

# AM_CPPFLAGS =
# AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

CLEANFILES = *.log *.sum $(srcdir)/package.m4

EXTRA_DIST = $(TESTSUITE).at $(TESTSUITE) atlocal.in ff hdf4 hdf5 nc show ssfunc

DISTCLEANFILES = besTest

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/besTest

check-local: atconfig atlocal $(TESTSUITE)
	@if bescmdln -x "show version;"; \
		then $(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS); \
		else echo "no bes found, not running cmdln-based tests"; \
	fi

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean

distclean-local:
	-rm atconfig

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4
	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
	mv $@.tmp $@

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
