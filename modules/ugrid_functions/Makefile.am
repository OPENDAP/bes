
# Build libugridFunctions, part of libdap.

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I conf

# Set the module version here, in the spec file and in configure.ac
M_NAME=ugrid_functions
M_VER=1.2.1

AM_CPPFLAGS = $(GF_CFLAGS) -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)
LIBADD = $(GF_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -Wcast-align

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS =  . unit-tests tests
# DIST_SUBDIRS = unit-tests tests

lib_besdir=$(libdir)/bes
lib_bes_LTLIBRARIES = libugrid_functions.la

SRCS = UgridFunctions.cc \
	ugrid_utils.cc \
	MeshDataVariable.cc \
	TwoDMeshTopology.cc  \
	ugrid_restrict.cc  \
	NDimensionalArray.cc 

HDRS = UgridFunctions.h\
	LocationType.h \
	ugrid_utils.h \
	MeshDataVariable.h  \
	TwoDMeshTopology.h \
	ugrid_restrict.h \
	NDimensionalArray.h 

libugrid_functions_la_SOURCES = $(SRCS) $(HDRS)
# libugrid_functions_la_CPPFLAGS = $(GF_CFLAGS) $(XML2_CFLAGS)
libugrid_functions_la_LDFLAGS = -avoid-version -module 
libugrid_functions_la_LIBADD = $(LIBADD) 

EXTRA_DIST = data ugrid_functions.conf.in

package_version=$(PACKAGE_MAJOR_VERSION).$(PACKAGE_MINOR_VERSION).$(PACKAGE_SUBMINOR_VERSION)

CLEANFILES = *~ ugrid_functions.conf ugrid_functions-*.tar.gz

sample_datadir = $(datadir)/hyrax/data/ugrids
sample_data_DATA = data/ugrid_test_01.nc data/ugrid_test_02.nc data/ugrid_test_03.nc data/ugrid_test_04.nc 

moduledir = $(sysconfdir)/bes/modules
module_DATA = ugrid_functions.conf

ugrid_functions.conf: ugrid_functions.conf.in $(top_builddir)/config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
                -e "s%[@]sysconfdir[@]%${sysconfdir}%" \
                -e "s%[@]pkglibdir[@]%${pkglibdir}%" \
                -e "s%[@]datarootdir[@]%${datarootdir}%" \
                -e "s%[@]datadir[@]%${datadir}%" \
                -e "s%[@]bindir[@]%${bindir}%" \
                -e "s%[@]bes_modules_dir[@]%${lib_besdir}%" $< > ugrid_functions.conf

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	cccc --outdir=$(C4_DIR) $(libugrid_functions_la_SOURCES)

