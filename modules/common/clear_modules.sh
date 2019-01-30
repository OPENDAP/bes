#!bin/sh


#for i in `cat all_modules.txt | grep -v "hdf"`
#do
#    rm -rvf $i
#done

rm -rf \
    csv_handler \
    dap-server \
    debug_functions \
    fileout_* \
    fits_handler \
    freeform_handler \
    gateway_module \
    gdal_handler \
    ncml_module \
    netcdf_handler \
    ugrid_functions \
    w10n_handler \
    xml_data_handler
#    hdf4_handler \
#    hdf5_handler \

