
AUTOMAKE_OPTIONS = foreign 

LDADD = $(top_builddir)/dispatch/libbes_dispatch.la $(DAP_LIBS) 
AM_CPPFLAGS = -I$(top_srcdir)  $(XML2_CFLAGS)
AM_CXXFLAGS =

noinst_PROGRAMS = FileLockingCacheTest
noinst_DATA =
CLEANFILES = 
EXTRA_DIST = 

FileLockingCacheTest_SOURCES = FileLockingCacheTest.cc FileLockingCacheTest.h
FileLockingCacheTest_CPPFLAGS = $(AM_CPPFLAGS)
FileLockingCacheTest_LDADD = $(LDADD)

