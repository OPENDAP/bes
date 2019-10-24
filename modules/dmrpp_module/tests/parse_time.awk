#!/usr/bin/awk -f
#
# Read values from 'time -p' and print them out as one line in csv
# form. jhrg 9/23/19
BEGIN {printf("Run, Real, System, User\n", n, r, u, s); all_found = 0}
/^[[0-9]+/ {n = $1}
/^real/ {r = $2}
/^user/ {u = $2}
/^sys/ {s = $2; all_found = 1}
{if (all_found) printf("%d, %f, %f, %f\n", n, r, u, s); all_found = 0}
