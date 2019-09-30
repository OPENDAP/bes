
# Here's how the data were collected. I renamed the time_data to
# time_data_{serial,parallel}.txt after running the tests.

for t in `seq 1 100`; do  echo $t >> time_data; time -p (besstandalone -c bes_serial.conf -i s3/airs_3.dap > tmp) 2>> time_data; done

for t in `seq 1 100`; do  echo $t >> time_data; time -p (besstandalone -c bes.conf -i s3/airs_3.dap > tmp) 2>> time_data; done
