#!/bin/bash

export log_file=./redir_vs_effctv.log

./CurlUtilsTest -d time_redirect_url_and_effective_url 2>&1 | tee $log_file
 
echo "retrieve_effective_url (µs)" > retrieve_effective_url.csv
# We use tail +2 to skip the first record because it's the WARMUP request which is always wicked slow.
cat $log_file | grep ELAPSED | grep retrieve_effective_url | tail +2 | awk 'BEGIN{FS="\\]\\["}{n=split($6,us," "); print us[1];}' >> retrieve_effective_url.csv

echo "get_redirect_url (µs)" > get_redirect_url.csv
cat $log_file | grep ELAPSED | grep get_redirect_url | awk 'BEGIN{FS="\\]\\["}{n=split($6,us," "); print us[1];}' >> get_redirect_url.csv

paste -d ", " retrieve_effective_url.csv get_redirect_url.csv > reu_vs_gru.csv