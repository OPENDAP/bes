
AUTOMAKE_OPTIONS = foreign

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

noinst_PROGRAMS = remote_resource_tester

remote_resource_tester_SOURCES = remote_resource_tester.cc
remote_resource_tester_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch -I$(top_srcdir)/http
remote_resource_tester_LDFLAGS = -L$(top_builddir)/dispatch -L$(top_builddir)/http
# $(BES_DISPATCH_LIB) $(BES_HTTP_LIB)
remote_resource_tester_LDADD = -lbes_dispatch -lbes_http
BES_CONF_IN = bes.conf.in

noinst_DATA = bes.conf

CLEANFILES = bes.conf $(srcdir)/package.m4 $(TESTSUITE)

EXTRA_DIST = $(TESTSUITE).at $(TESTSUITE) atlocal.in $(srcdir)/package.m4 $(BES_CONF_IN)

DISTCLEANFILES =  atconfig

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

# This target starts the mock s3 service, runs the tests, and then stops the service.
# Note that once the service is started, the code waits for it to start to respond.
# jhrg 5/10/23
check-local: atconfig atlocal $(srcdir)/package.m4 $(TESTSUITE) remote_resource_tester bes.conf
	@python3 mock-s3/main.py >/dev/null 2>&1 &
	@while ! curl -s http://localhost:8000/200/1/init >/dev/null; do sleep 1; done
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)
	@curl -s http://localhost:8000/exit >/dev/null

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean

AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4
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

