#!/bin/bash
#
# debug2csv <input besdebug file> <outptu csv file>

sed -e 's@\[@@g' -e 's@\]@,@g' $1 > $2
