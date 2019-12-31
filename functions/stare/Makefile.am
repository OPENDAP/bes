

AUTOMAKE_OPTIONS = foreign 

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = $(DAP_CFLAGS) -I$(top_srcdir)/dispatch -I$(STARE_INC) -I$(prefix)/deps/include
# Added  -Wno-deprecated to suppress warnings about auto_ptr<>. jhrg 11/7/19
AM_CXXFLAGS = -Wno-deprecated -Wno-vla-extension
AM_LDADD =  $(DAP_CLIENT_LIBS) -L$(abs_top_srcdir)/dispatch -lbes_dispatch $(H5_LDFLAGS) $(H5_LIBS) $(STARE_LDFLAGS) $(STARE_LIBS)

SUBDIRS = . unit-tests tests

if USE_VALGRIND
TESTS_ENVIRONMENT = valgrind --quiet --trace-children=yes --error-exitcode=1 --dsymutil=yes --leak-check=yes
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

bin_PROGRAMS = build_sidecar

build_sidecar_SOURCES = build_sidecar.cc
build_sidecar_LDADD = $(AM_LDADD)

EXTRA_DIST = 

CLEANFILES = 

DISTCLEANFILES = 
