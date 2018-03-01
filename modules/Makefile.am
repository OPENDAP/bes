# Build the DAP Modules
#
# jhrg 11/18/13.

AUTOMAKE_OPTIONS = foreign

SUBDIRS = csv_handler usage asciival www-interface freeform_handler

if BES_DEVELOPER
SUBDIRS += debug_functions dmrpp_module
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
SUBDIRS += netcdf_handler fileout_netcdf
endif

# Why are these here? Because they have tests that depend on
# netcdf.
# FIXME Rewrite the tests to use the dapread handler. jhrg 12/12/14
SUBDIRS += fileout_json w10n_handler

SUBDIRS += gateway_module

if BUILD_GDAL
SUBDIRS += fileout_gdal gdal_handler
endif

if BUILD_FITS
SUBDIRS += fits_handler
endif

# Same deal as above, but this uses HDF4/5 too... jhrg 12/12/14
SUBDIRS += xml_data_handler

if BUILD_NCML
SUBDIRS += ncml_module
endif

if USING_GRIDFIELDS
SUBDIRS += ugrid_functions
endif
