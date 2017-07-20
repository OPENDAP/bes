
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS = -I$(top_srcdir)/dispatch  $(XML2_CFLAGS)  $(DAP_CFLAGS)
AM_CXXFLAGS =
# I put DAP_LIBS here because the AM docs say that LDADD should not have linker flags
# and DAP_LIBS has both libs and flags :-(
AM_LDFLAGS = $(DAP_LIBS) -L$(top_builddir)/dispatch
LDADD = -lbes_dispatch 

noinst_PROGRAMS = FileLockingCacheTest

# You need this because AM assumes that .h files don't need to be built
BUILT_SOURCES = test_config.h

noinst_DATA =
CLEANFILES = 
EXTRA_DIST = 
# you don't need this because you/I have declared this as built 
DISTCLEANFILES = test_config.h

test_config.h: test_config.h.in
	@my_topdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_srcdir[@]%$${my_topdir}%" $< > test_config.h

FileLockingCacheTest_SOURCES = FileLockingCacheTest.cc FileLockingCacheTest.h test_config.h
# You don't need this because AM_CPPFLAGS is used by default
# FileLockingCacheTest_CPPFLAGS = $(AM_CPPFLAGS)
# and LDADD
# FileLockingCacheTest_LDADD = $(LDADD)
