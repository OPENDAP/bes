
AUTOMAKE_OPTIONS = foreign 
AM_CPPFLAGS = -I$(top_srcdir)/dispatch  $(XML2_CFLAGS)  $(DAP_CFLAGS)
AM_CXXFLAGS =
# I put DAP_LIBS here because the AM docs say that LDADD should not have linker flags
# and DAP_LIBS has both libs and flags :-(
AM_LDFLAGS = $(DAP_LIBS) -L$(top_builddir)/dispatch
LDADD = $(top_builddir)/dispatch/libbes_dispatch.la $(DAP_LIBS) 

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LDADD += $(CPPUNIT_LIBS)
endif

noinst_PROGRAMS = FileLockingCacheTest

TESTS = testsuite

# You need this because AM assumes that .h files don't need to be built
BUILT_SOURCES = test_config.h

noinst_DATA =
CLEANFILES = 
EXTRA_DIST = $(TESTSUITE) $(TESTSUITE).at atlocal.in $(srcdir)/package.m4 \
test_config.h.in

DISTCLEANFILES = atconfig

test_config.h: test_config.h.in
	@my_topdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_srcdir[@]%$${my_topdir}%" $< > test_config.h

FileLockingCacheTest_SOURCES = FileLockingCacheTest.cc FileLockingCacheTest.h test_config.h

############## Autotest follows #####################

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/testsuite

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 

AUTOTEST = $(AUTOM4TE) --language=autotest

# Switch back to building the package.m4 here in the makefile
# since that gives better control over things like PACKAGE_NAME,
# which will be handy in the handlers to keep their package names
# from all being 'bes'.
$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4
	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
	mv $@.tmp $@

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
