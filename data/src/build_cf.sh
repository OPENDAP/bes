#!/bin/sh
rm *.h5 *.o

/hdfdap/local/bin/h5cc  t_flatten.c  -o t_flatten
./t_flatten

/hdfdap/local/bin/h5cc  t_flatten_name_clash.c  -o t_flatten_name_clash
./t_flatten_name_clash

/hdfdap/local/bin/h5cc  t_name_clash.c -o t_name_clash
./t_name_clash

/hdfdap/local/bin/h5cc  t_non_cf_char.c -o t_non_cf_char
./t_non_cf_char


