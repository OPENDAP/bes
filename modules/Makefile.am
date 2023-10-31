# Build the DAP Modules
#
# jhrg 11/18/13.

AUTOMAKE_OPTIONS = foreign

# Might move these to a child directory for testing things. jhrg 5/16/18
EXTRA_DIST = data/README

toplevel_datadir = $(datadir)/hyrax/data
toplevel_data_DATA = data/README

SUBDIRS = common csv_handler freeform_handler functions usage asciival

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

# Add after hdf5_handler; Does not need the hdf5 handler to serve data, but
# the build_dmrpp binary uses the hdf5 handler to build DMR++ metadata files.
# jhrg 3/21/22
SUBDIRS += dmrpp_module

# fileout_netcdf needs to come after HDF4 (and maybe HDF5
# in the future) because of the tests it runs.
if BUILD_NETCDF
SUBDIRS += netcdf_handler fileout_netcdf fileout_covjson
endif

# Why are these here? Because they have tests that depend on netcdf.
SUBDIRS += httpd_catalog_module ngap_module s3_reader fileout_json w10n_handler gateway_module xml_data_handler builddmrpp_module

if BUILD_GDAL
SUBDIRS += gdal_module
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
