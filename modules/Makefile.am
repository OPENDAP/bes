# Build the DAP Modules
#
# jhrg 11/18/13.

AUTOMAKE_OPTIONS = foreign check-news

SUBDIRS = csv_handler dap-server fileout_json freeform_handler xml_data_handler \
gateway_module

if BUILD_NETCDF
SUBDIRS += netcdf_handler fileout_netcdf
endif
