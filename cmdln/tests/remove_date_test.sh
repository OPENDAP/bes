#!/bin/bash 

#test_value="$(cat $1)"
test_value=" <Value>build_dmrpp -f /tmp/tmpznmtnt8g/daymet_v4_daily_na_tmax_2010.nc -r daymet_v4_daily_na_tmax_2010.nc.dmr -u OPeNDAP_DMRpp_DATA_ACCESS_URL -M</Value>"

echo "$test_value" | sed -e "s@/tmp/[^/]*/@/tmp/temp-dir-name/@g" 
    
    