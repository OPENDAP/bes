# Build the DAP Modules
#
# jhrg 11/18/13.

AUTOMAKE_OPTIONS = foreign

# Might move these to a child directory for testing things. jhrg 5/16/18
EXTRA_DIST = read_test_baseline.cc read_test_baseline.h handler_tests_macros.m4

SUBDIRS = csv_handler usage asciival www-interface freeform_handler dmrpp_module

if WITH_CMR
SUBDIRS += cmr_module 
endif

if BES_DEVELOPER
SUBDIRS += debug_functions
endif

if BUILD_HDF4
SUBDIRS += hdf4_handler
endif

if BUILD_HDF5
SUBDIRS += hdf5_handler
endif

# fileout_netcdf needs to come after HDF4 (and maybe HDF5
# in the future) because of the tests it runs.
if BUILD_NETCDF
SUBDIRS += netcdf_handler fileout_netcdf fileout_covjson
endif

# Why are these here? Because they have tests that depend on
# netcdf.
# FIXME Rewrite the tests to use the dapread handler. jhrg 12/12/14
SUBDIRS += ngap_module fileout_json w10n_handler gateway_module httpd_catalog_module xml_data_handler

if BUILD_GDAL
SUBDIRS += fileout_gdal gdal_handler
endif

if BUILD_FITS
SUBDIRS += fits_handler
endif

if BUILD_NCML
SUBDIRS += ncml_module
endif

if USING_GRIDFIELDS
SUBDIRS += ugrid_functions
endif
