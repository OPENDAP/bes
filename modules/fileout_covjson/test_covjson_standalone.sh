#!/bin/bash

rm coads_climatology.covjson
#rm fnoc1.covjson
#rm cyg-ddmi-grid-wind-a10-d20.covjson

touch coads_climatology.covjson
#touch fnoc1.covjson
#touch cyg-ddmi-grid-wind-a10-d20.covjson

./../../../build/bin/besstandalone -c ./tests/bes.conf -i ./tests/covjson/coads_climatology_abstract_object_DATA.bescmd > coads_climatology.covjson
#./../../../build/bin/besstandalone -c ./tests/bes.conf -i ./tests/covjson/fnoc1_abstract_object_DATA.bescmd > fnoc1.covjson
#./../../../build/bin/besstandalone -c ./tests/bes.conf -i ./tests/covjson/cyg-ddmi-grid-wind-a10-d20_abstract_object_DATA.bescmd > cyg-ddmi-grid-wind-a10-d20.covjson
