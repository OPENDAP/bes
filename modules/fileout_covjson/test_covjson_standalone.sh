#!/bin/bash

rm test_output.txt

touch test_output.txt

#sudo ./build/bin/besstandalone -c ./bes/modules/fileout_covjson/tests/bes.conf -i ./bes/modules/fileout_covjson/tests/covjson/coads_climatology_abstract_object_DATA.bescmd > test_output.txt
sudo ./../../../build/bin/besstandalone -c ./tests/bes.conf -i ./tests/covjson/coads_climatology_abstract_object_DATA.bescmd > test_output.txt
