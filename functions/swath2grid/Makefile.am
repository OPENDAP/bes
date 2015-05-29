
# Build libswath2grid, part of the bes/functions module when GDAL is present.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch $(XML2_CFLAGS) $(CURL_CFLAGS) \
$(GDAL_CFLAGS) $(DAP_SERVER_CFLAGS) $(DAP_CLIENT_CFLAGS)

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
# Now (12/20/12) this fails; don't run until it works.
# TESTS = $(UNIT_TESTS)

noinst_LTLIBRARIES = libswath2grid.la

libswath2grid_la_SOURCES = $(SRCS) $(HDRS)
libswath2grid_la_LDFLAGS = 
libswath2grid_la_LIBADD = $(GDAL_LDFLAGS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

SRCS = AbstractDataset.cpp wcs_error.cpp    \
BoundingBox.cpp wcsUtil.cpp DAP_Dataset.cpp  reproj_functions.cc

# NC_GOES_Dataset.cpp NC_GOES_Dataset.h

HDRS = AbstractDataset.h wcs_error.h    \
BoundingBox.h wcsUtil.h DAP_Dataset.h reproj_functions.h

if CPPUNIT
UNIT_TESTS = s2gTest

else
UNIT_TESTS =

check-local:
	@echo ""
	@echo "**********************************************************"
	@echo "You must have cppunit 1.12.x or greater installed to run *"
	@echo "check target in unit-tests directory                     *"
	@echo "**********************************************************"
	@echo ""
endif

s2gTest_SOURCES = s2gTest.cc
s2gTest_CPPFLAGS = $(AM_CPPFLAGS) $(CPPUNIT_CFLAGS)
s2gTest_LDADD = -ltest-types libswath2grid.la $(CPPUNIT_LIBS) $(DAP_SERVER_LIBS) \
$(DAP_CLIENT_LIBS) $(GDAL_LDFLAGS) 

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
