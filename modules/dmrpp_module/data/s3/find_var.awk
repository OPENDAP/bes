#
# Little awk program to read a DMR++ and count variables and their chunks
# also prints out the names of the dimensions which can be used to compute
# the total size of asking for the first N variables.
#
BEGIN { count = 0; chunks = 0 }
/<Float../ { ++count; print $1 }
/<Int../ { ++count; print $1 }
/<Dim / { print $2 }
/<Dimension/ { print $2, ", ", $3 }
/<dmrpp:chunk / { ++chunks } 
{ if (count > 35) { exit} }
END { print "count: ", count, " chunks: ", chunks }
