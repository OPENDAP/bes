#!/bin/sh

# Exit is a pipeline or sub-shell returns non-zero
set -e

# Basic idea:
# after_failure
#  - sudo tar -czf /tmp/build-${TRAVIS_BUILD_NUMBER}-logs.tgz your-application-logs/
#  - scp /tmp/build-${TRAVIS_BUILD_NUMBER}-logs.tgz travis@your-server.com:~/logs

TRAVIS_BUILD_NUMBER=0

cat <<EOF | xargs $1
./cmdln/testsuite/besTest.log
./dap/tests/besDapModuleTest.log
./dapreader/tests/DapReaderTest.log
./functions/tests/broken_tests.log
./functions/tests/functionsTest.log
./hello_world/bes-testsuite/hello_handlerTest.log
./modules/csv_handler/bes-testsuite/csv_handlerTest.log
./modules/dap-server/asciival/tests/ASCII_HandlerTest.log
./modules/fileout_gdal/tests/testsuite_jp2.log
./modules/fileout_gdal/tests/testsuite_tif.log
./modules/fileout_json/bes-testsuite/fileout_jsonTest.log
./modules/fileout_netcdf/unit-tests/fonc_handlerTest.log
./modules/fits_handler/bes-testsuite/fits_handlerTest.log
./modules/freeform_handler/bes-testsuite/freeform_handlerTest.log
./modules/gateway_module/bes-testsuite/gateway_handlerTest.log
./modules/gdal_handler/tests/testsuite.log
./modules/hdf4_handler/bes-testsuite/hdf4_handlerTest.log
./modules/hdf4_handler/bes-testsuite/hdf4_handlerTest.cf.log
./modules/hdf4_handler/bes-testsuite/hdf4_handlerTest.nasa.with_hdfeos2.log
./modules/hdf4_handler/bes-testsuite/hdf4_handlerTest.with_hdfeos2.log
./modules/hdf5_handler/bes-testsuite/hdf5_handlerTest.log
./modules/hdf5_handler/bes-testsuite/hdf5_handlerTest.nasa.log
./modules/ncml_module/tests/aggregations.log
./modules/ncml_module/tests/attribute_tests.log
./modules/ncml_module/tests/parse_error_misc.log
./modules/ncml_module/tests/testsuite.log
./modules/ncml_module/tests/variable_misc.log
./modules/ncml_module/tests/variable_new_arrays.log
./modules/ncml_module/tests/variable_new_multi_arrays.log
./modules/ncml_module/tests/variable_new_scalars.log
./modules/ncml_module/tests/variable_new_structures.log
./modules/ncml_module/tests/variable_remove.log
./modules/ncml_module/tests/variable_rename.log
./modules/netcdf_handler/bes-testsuite/nc4_netcdf_handler_tests.log
./modules/netcdf_handler/bes-testsuite/netcdf_handlerTest.log
./modules/ugrid_functions/tests/ugrid_functionsTest.log
./modules/w10n_handler/bes-testsuite/w10n_handlerTest.log
./modules/xml_data_handler/tests/testsuite.log
EOF
