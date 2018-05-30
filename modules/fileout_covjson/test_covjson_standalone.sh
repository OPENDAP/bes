#!/bin/bash

rm coads_climatology.covjson
#rm fnoc1.covjson
#rm cyg-ddmi-grid-wind-a10-d20.covjson
#rm windsat_remss_ovw_l3_20180105_rt.nc.gz.nc.covjson

touch coads_climatology.covjson
#touch fnoc1.covjson
#touch cyg-ddmi-grid-wind-a10-d20.covjson
#touch windsat_remss_ovw_l3_20180105_rt.nc.gz.nc.covjson

./../../../build/bin/besstandalone -c ./tests/bes.conf -i ./tests/covjson/coads_climatology_abstract_object_DATA.bescmd > coads_climatology.covjson
#./../../../build/bin/besstandalone -c ./tests/bes.conf -i ./tests/covjson/fnoc1_abstract_object_DATA.bescmd > fnoc1.covjson
#./../../../build/bin/besstandalone -c ./tests/bes.conf -i ./tests/covjson/cyg-ddmi-grid-wind-a10-d20_abstract_object_DATA.bescmd > cyg-ddmi-grid-wind-a10-d20.covjson
#./../../../build/bin/besstandalone -c ./tests/bes.conf -i ./tests/covjson/windsat_remss_ovw_l3_20180105_rt.nc.gz.nc_abstract_object_DATA.bescmd > windsat_remss_ovw_l3_20180105_rt.nc.gz.nc.covjson
