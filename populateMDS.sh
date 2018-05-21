#!/bin/sh
#
# -*- mode: bash; -*-
#
# This file is part of the Back End Server component of the
# Hyrax Data Server.
#
# Copyright (c) 2018 OPeNDAP, Inc.
# Author: Nathan David Potter <ndp@opendap.org>, James Gallagher
# <jgallagher@opendap.org>
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

# Use this with besstandalone; can be set with the -c option
BES_CONF="${prefix}/etc/bes/bes.conf"

show_usage () { 
    echo ""
    echo "OPeNDAP              MAY 20, 2018"

}

OPTIND=1         # Reset in case getopts has been used previously in the shell.

verbose=""

while getopts "h?vc:" opt; do
    case "$opt" in
    h|\?)
        show_usage
        exit 0
        ;;
    v)  verbose="-v"
        ;;
    c)  BES_CONF=$OPTARG
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

[ -n "$1" ] || { echi "Expected one or more pathnames." ; exit 1; }

# Read a pathname from the next argument
pathanme=$1

# Ask the BES to get the DDS and DMR for that file

./localBesGetDap.sh $verbose -i $pathname -d dds > /dev/null

./localBesGetDap.sh $verbose -i $pathname -d dmr > /dev/null

