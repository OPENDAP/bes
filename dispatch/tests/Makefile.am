
AUTOMAKE_OPTIONS = foreign 


LDADD = $(top_builddir)/dispatch/libbes_dispatch.la $(DAP_LIBS) 
AM_CPPFLAGS = -I$(top_srcdir)/dispatch  $(XML2_CFLAGS)
AM_CXXFLAGS =

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
FileLockingCacheTest_LDADD = $(LDADD)

