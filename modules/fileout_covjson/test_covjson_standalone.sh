#!/bin/bash

rm coads_climatology.covjson
#rm cyg-ddmi-grid-wind-a10-d20.covjson

touch coads_climatology.covjson
#touch cyg-ddmi-grid-wind-a10-d20.covjson

sudo ./../../../build/bin/besstandalone -c ./tests/bes.conf -i ./tests/covjson/coads_climatology_abstract_object_DATA.bescmd > coads_climatology.covjson
#sudo ./../../../build/bin/besstandalone -c ./tests/bes.conf -i ./tests/covjson/cyg-ddmi-grid-wind-a10-d20_abstract_object_DATA.bescmd > cyg-ddmi-grid-wind-a10-d20.covjson
