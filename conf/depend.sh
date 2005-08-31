#!/bin/sh
#
# This file is part of libdap, A C++ implmentation of the OPeNDAP Data
# Access Protocol.

# Copyright (c) 2002,2003 OPeNDAP, Inc.
# Author: James Gallagher <jgallagher@opendap.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
# depend is a script replacement for makedepend. It requires gcc.
# -s causes system includes to be added to the dependencies.
# -b <extension> causes <extension> to be used instead of `bak' when naming
#    the backup copy of the Makefile.
# -m <makefile name> causes depend to use <makefile name> instead of
#    `Makefile'. 
# -n Don't actually make the dependencies, just print out what would have been 
#    done. 
# -t Truncate the `Makefile' after making a backup copy. 
# jhrg 8/17/99
#
# Added -b option. Added `-E' to CFLAGS.
# jhrg 3/5/96
#

# $Id$

usage="depend [-s][-t][-b <ext>][-m <makefile name>] -- <compiler options> -- <files>"
CFLAGS="-E -MM -MG"
tmp=/usr/tmp/depend$$
makefile=Makefile
bak=bak
doit=1
truncate=0

# read the command options

x=`getopt stnb:m: "$@"`		# "$@" preserves quotes in the input

if [ $? != 0 ]			# $? is the exit status of 'getopt ...'
then
   echo "${usage}"
   exit 2
fi

# set -- $x sets the shell positional params $1, $2, ... to the tokens in $x
# (which were put there by `getopt ...`. The eval preserves any quotes in $x.

eval set -- $x

for c in "$@"
do
    case $c in
	-s) CFLAGS="-E -M"; shift 1;;
	-m) makefile=$2; shift 2;;
	-b) bak=$2; shift 2;;
	-n) doit=0; shift 1;;
	-t) truncate=1; shift 1;;
        -\?) echo $usage; exit 2;;
	--) shift 1; break;;
    esac
done

# accumulate the C compiler options into CFLAGS

while [ $1 != '--' ]
do
    CFLAGS="$CFLAGS $1"
    shift 1
done
shift 1				# shift past the second `--' to the files

if test $doit = "1"
then

    if test $truncate = "0"
    then
	# build the new Makefile in a tmp directory using the existing 
	# makefile: first copy everything upto the dependencies using awk,
	# then compoute and append the dependencies of the files (which are
	# the remaining arguments). 
	awk 'BEGIN {found = 0}
	    /DO NOT DELETE/ {found = 1; print $0}
	    found != 1 {print $0}
	    found == 1 {exit}' $makefile > $tmp
    fi
    gcc $CFLAGS $* >> $tmp

    mv $makefile ${makefile}.${bak}	# backup the current Makefile 

    mv $tmp $makefile

else

    echo "gcc $CFLAGS $* >> $tmp"
    echo "mv $makefile ${makefile}.${bak}"
    echo "mv $tmp $makefile"

fi

exit 0
