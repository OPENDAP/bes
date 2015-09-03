#!/bin/sh

# Exit is a pipeline or sub-shell returns non-zero
set -e

# Basic idea: Put this in the travis.yml file
# after_failure
#  - sudo tar -czf /tmp/build-${TRAVIS_BUILD_NUMBER}-logs.tgz your-application-logs/
#  - scp /tmp/build-${TRAVIS_BUILD_NUMBER}-logs.tgz travis@your-server.com:~/logs

cat <<EOF | xargs tar -czf $1 
./dapreader/tests/
./functions/tests/
./modules/csv_handler/bes-testsuite/
./modules/dap-server/asciival/tests/
./modules/fileout_gdal/tests/
./modules/fileout_json/bes-testsuite/
./modules/fileout_netcdf/unit-tests/
./modules/fits_handler/bes-testsuite/
./modules/gateway_module/bes-testsuite/
./modules/gdal_handler/tests/
./modules/hdf4_handler/bes-testsuite/
./modules/hdf5_handler/bes-testsuite/
./modules/ncml_module/tests/
./modules/netcdf_handler/bes-testsuite/
./modules/netcdf_handler/bes-testsuite/
./modules/ugrid_functions/tests/
./modules/w10n_handler/bes-testsuite/
./modules/xml_data_handler/tests/
EOF

# ./modules/freeform_handler/bes-testsuite/
