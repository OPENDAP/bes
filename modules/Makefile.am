# Build the DAP Modules
#
# jhrg 11/18/13.

AUTOMAKE_OPTIONS = foreign check-news

SUBDIRS = csv_handler dap-server fileout_json freeform_handler xml_data_handler \
gateway_module

if BUILD_NETCDF
SUBDIRS += netcdf_handler fileout_netcdf
endif

if BUILD_GDAL
SUBDIRS += fileout_gdal gdal_handler
endif

if BUILD_FITS
SUBDIRS += fits_handler
endif

if BUILD_HDF4
SUBDIRS += hdf4_handler
endif

if BUILD_HDF5
SUBDIRS += hdf5_handler
endif

if BUILD_NCML
SUBDIRS += ncml_module
endif

if USING_GRIDFIELDS
SUBDIRS += ugrid_functions
endif
