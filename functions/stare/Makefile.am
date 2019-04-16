

AUTOMAKE_OPTIONS = foreign 

# Headers in 'tests' are used by the arrayT unit tests.

#AM_CPPFLAGS = $(BES_CPPFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/functions $(DAP_CFLAGS) $(GDAL_CFLAGS)
#AM_LDADD =  $(DAP_SERVER_LIBS) $(GF_LIBS) $(top_builddir)/dispatch/libbes_dispatch.la


#Define __macosx for STARE
#Remove hard code for filepath
AM_CPPFLAGS = $(DAP_CFLAGS) -D__macosx -I$(top_srcdir)/../hyrax-dependencies/src/STARE/include -I$(prefix)/deps/include
AM_CXXFLAGS = --std=c++11
AM_LDADD =  $(DAP_CLIENT_LIBS) -L$(top_srcdir)/../hyrax-dependencies/src/STARE/src -lSTARE

#if CPPUNIT
#AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
#AM_LDADD += $(CPPUNIT_LIBS)
#endif

if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --quiet --trace-children=yes --error-exitcode=1 --dsymutil=yes --leak-check=yes
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

bin_PROGRAMS = build_sidecar opendapTest

build_sidecar_SOURCES = stareMapping.cc
build_sidecar_LDADD = $(AM_LDADD) $(H5_LDFLAGS) $(H5_LIBS) -lhdf5_cpp
opendapTest_SOURCES = opendapTest.cpp
opendapTest_LDADD = $(AM_LDADD)


EXTRA_DIST = 

CLEANFILES = 

DISTCLEANFILES = 
