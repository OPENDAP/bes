
AUTOMAKE_OPTIONS = foreign 

if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --trace-children=yes --error-exitcode=1 --dsymutil=yes \
--leak-check=yes  --max-stackframe=3000000  --quiet
endif

AM_CPPFLAGS = -I$(top_srcdir)
AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA = bes.conf

CLEANFILES = *.log *.sum gmon.out

EXTRA_DIST = data bescmd TestServerTest.at $(TESTSUITE) \
        atlocal.in package.m4.in bes.conf.in handler_tests_macros.m4

DISTCLEANFILES = bes.conf

bes.conf: bes.conf.in $(top_builddir)/config.status
	@my_topdir=`echo ${abs_top_srcdir} | sed 's/\(.*\)\/\(.[^\/]*\)\/\.\./\1/g'`; \
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]modulesdir[@]%${libdir}/bes%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]libdir[@]%${libdir}%" \
		-e "s%[@]datadir[@]%${datadir}%" \
		-e "s%[@]abs_top_srcdir[@]%$$my_topdir%" \
		-e "s%[@]top_builddir[@]%${top_builddir}%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > bes.conf

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/TestServerTest

$(TESTSUITE): $(srcdir)/handler_tests_macros.m4 $(srcdir)/package.m4 $(srcdir)/TestServerTest.at

check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean

distclean-local:
	-rm atconfig

AUTOTEST = $(AUTOM4TE) --language=autotest

% : %.at 
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at
