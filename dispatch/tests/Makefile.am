
AUTOMAKE_OPTIONS = foreign 


LDADD = $(top_builddir)/dispatch/libbes_dispatch.la $(DAP_LIBS) 
AM_CPPFLAGS = -I$(top_srcdir)  $(XML2_CFLAGS)
AM_CXXFLAGS =

noinst_PROGRAMS = FileLockingCacheTest
noinst_DATA =
CLEANFILES = 
EXTRA_DIST = 
DISTCLEANFILES = test_config.h

test_config.h: test_config.h.in Makefile
	@my_topdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_srcdir[@]%$${my_topdir}%" $< > test_config.h

FileLockingCacheTest_SOURCES = FileLockingCacheTest.cc FileLockingCacheTest.h test_config.h
FileLockingCacheTest_CPPFLAGS = $(AM_CPPFLAGS)
FileLockingCacheTest_LDADD = $(LDADD)

