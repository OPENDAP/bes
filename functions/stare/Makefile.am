

AUTOMAKE_OPTIONS = foreign 

# Headers in 'tests' are used by the arrayT unit tests.

AM_CPPFLAGS = $(DAP_CFLAGS) -I$(top_srcdir)/dispatch -I$(STARE_INC) -I$(prefix)/deps/include $(NC_CPPFLAGS)
# Added  -Wno-deprecated to suppress warnings about auto_ptr<>. jhrg 11/7/19
AM_CXXFLAGS = -Wno-deprecated -Wno-vla-extension
AM_LDADD =  $(DAP_CLIENT_LIBS) -L$(abs_top_builddir)/dispatch -lbes_dispatch $(H5_LDFLAGS) $(H5_LIBS) \
$(STARE_LDFLAGS) $(STARE_LIBS) $(NC_LDFLAGS) $(NC_LIBS)

SUBDIRS = . unit-tests tests

if USE_VALGRIND
TESTS_ENVIRONMENT = valgrind --quiet --trace-children=yes --error-exitcode=1 --dsymutil=yes --leak-check=yes
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align

AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

# Old programs used at the start of the STARE project. jhrg 8/20/20
# noinst_PROGRAMS = build_sidecar build_test_s_indices

build_sidecar_SOURCES = build_sidecar.cc
build_sidecar_LDADD = $(AM_LDADD)

build_test_s_indices_SOURCES = build_test_s_indices.cc
build_test_s_indices_LDADD = $(AM_LDADD)

sample_datadir = $(datadir)/hyrax/data/stare

sample_data_DATA = data/README data/MYD09.A2019003_hacked.h5 \
data/MYD09.A2019003_hacked_stare.h5 data/MYD09.A2019003_hacked_stare_res.h5 \
data/Target_overlap_nw.h5 data/Target_overlap_nw_stare.h5 data/Target_overlap_nw_stare_res.h5 \
data/t1_sidecar.nc

EXTRA_DIST = data/README data/MYD09.A2019003_hacked.h5 \
data/MYD09.A2019003_hacked_stare.h5 data/MYD09.A2019003_hacked_stare_res.h5 \
data/Target_overlap_nw.h5 data/Target_overlap_nw_stare.h5 data/Target_overlap_nw_stare_res.h5 \
data/t1_sidecar.nc

CLEANFILES = 

DISTCLEANFILES = 
