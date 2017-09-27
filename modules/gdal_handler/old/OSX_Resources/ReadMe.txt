

Updated for version 3.10.0

This is the OPeNDAP netCDF Data Handler. It is used along with Hyrax.

For information about building the OPeNDAP netCDF Data Handler, see theINSTALL file.

This data handler is a component of the OPeNDAP DAP Server; the server basesoftware is designed to allow any number of handlers to be configured easily.See the DAP Server README and INSTALL files for information aboutconfiguration, including how to use this handler.

This version of the handler has been updated to read netcdf 4 filesand supports most of the netcdf 4 data types, not just those definedas part of the netcdf 'classic' data model.

See http://docs.opendap.org/index.php/BES_-_Modules_-_The_NetCDF_Handlerfor the full story on the different data models and how they aremapped into DAP2 along with the (expanded) set of configurationparameters.

Test data are also installed, so after installing this handler, Hyrax will have data to serve providing an easy way to test your new installationand to see how a working bes.conf should look. To use this, make surethat you first install the bes, and that dap-server gets installed too.Finally, every time you install or reinstall handlers, make sure to restartthe BES and OLFS.

