
# Build libswath2grid, part of the bes/functions module when GDAL is present.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch $(XML2_CFLAGS) $(CURL_CFLAGS) \
$(GDAL_CFLAGS) $(DAP_SERVER_CFLAGS) $(DAP_CLIENT_CFLAGS)

SUBDIRS = . unit-tests # tests

# We use DIST_SUBDIRS because we want geo-functions to be part of the dist.
# However, since we use it we must list all of the subdirs we want; without
# it, automake will automatically distribute all the stuff in SUBDIRS.
DIST_SUBDIRS = unit-tests # tests 

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

noinst_LTLIBRARIES = libswath2grid.la

libswath2grid_la_SOURCES = $(SRCS) $(HDRS)
libswath2grid_la_LDFLAGS = 
libswath2grid_la_LIBADD = $(GDAL_LDFLAGS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

SRCS = ScaleGrid.cc scale_util.cc

# AbstractDataset.cpp BoundingBox.cpp wcs_error.cpp wcsUtil.cpp 
# DAP_Dataset.cpp reproj_functions.cc

# Geo2DArray.cc

HDRS = scale_util.h

# AbstractDataset.h BoundingBox.h wcs_error.h wcsUtil.h 
# DAP_Dataset.h reproj_functions.h

# Geo2DArray.h

if  LIBDAP
check-dap:
	@echo ""
	@echo "**********************************************************"
	@echo "USING DAP "
	@echo "DAP_CLIENT_CFLAGS:  " $(DAP_CLIENT_CFLAGS)
	@echo "DAP_SERVER_CFLAGS:  " $(DAP_SERVER_CFLAGS)
	@echo "DAP_CLIENT_LIBS:    " $(DAP_CLIENT_LIBS)
	@echo "DAP_SERVER_LIBS:    " $(DAP_SERVER_LIBS)
	@echo "**********************************************************"
	@echo ""
else
check-dap:
	@echo ""
	@echo "**********************************************************"
	@echo " Unable to locate DAP libraries!"
	@echo "**********************************************************"
	@echo ""
endif


if  BUILD_GDAL
check-gdal:
	@echo ""
	@echo "**********************************************************"
	@echo "Using gdal. "
	@echo "GDAL_CFLAGS:  " $(GDAL_CFLAGS)
	@echo "GDAL_LDFLAGS: " $(GDAL_LDFLAGS)
	@echo "**********************************************************"
	@echo ""
else
check-gdal:
	@echo ""
	@echo "**********************************************************"
	@echo "You must have gdal 12.15.12 or greater installed to run"
	@echo "check target in unit-tests directory                   "
	@echo "GDAL_VERSION: '$(GDAL_VERSION)'"
	@echo "prefix: '$(prefix)'"
	@echo "**********************************************************"
	@echo ""
endif
