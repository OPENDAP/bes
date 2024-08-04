
AUTOMAKE_OPTIONS = foreign 
AM_CPPFLAGS = -I$(top_srcdir)/dispatch  $(XML2_CFLAGS)  $(DAP_CFLAGS)
AM_CXXFLAGS =

LDADD = $(top_builddir)/dispatch/libbes_dispatch.la $(DAP_LIBS)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LDADD += $(CPPUNIT_LIBS)
endif

noinst_PROGRAMS =

EXTRA_DIST = $(TESTSUITE) $(TESTSUITE).at atlocal.in $(srcdir)/package.m4 \
test_config.h.in

DISTCLEANFILES = atconfig

CLEANFILES = test_config.h

# These are all created by the FileLockingCacheTest program Run by clean-local.
# jhrg 1/23/18
.PHONY: cache-clean
cache-clean:
	-rm -rf blocking cache create_and_lock create_and_lock_2 create_and_lock_3 read_hooks

noinst_HEADERS = test_config.h

BUILT_SOURCES = test_config.h

test_config.h: $(srcdir)/test_config.h.in Makefile
	mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

FileLockingCacheTest_SOURCES = FileLockingCacheTest.cc
FileLockingCacheTest_LDADD = $(LDADD)

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/testsuite

clean-local: cache-clean
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 

# Force the tests here to run; use -v until we sort out the intermittent issues
# with these tests. 
check-local: $(TESTSUITE) atconfig atlocal 
	$(SHELL) '$(TESTSUITE)'

AUTOTEST = $(AUTOM4TE) --language=autotest

# Switch back to building the package.m4 here in the makefile
# since that gives better control over things like PACKAGE_NAME,
# which will be handy in the handlers to keep their package names
# from all being 'bes'.
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
