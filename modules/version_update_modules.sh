#!/bin/bash
#
# Update the handler version numbers in the configure.ac, Makefile.am,
# NEWS and update the ChangeLog with the git log info for each of the
# bes modules given on the command line.
#
# This script automates rote updates when little has changed in a
# handler. It will update the NEWS file, but not README or INSTALL.
# When there are substantive changes, don't use it!
#
# This script tests for a 'sentinel' file that, if present, stops the
# script from making any changes. This is because running the script
# several times in the same directories for a single release is
# probably an error. The idea behind the sentinel file is that a
# person has to intentionally remove it to get the script to run and
# edit the configure.ac, etc., files.
#
# Options: -n: Do not modify files like Makefile.am but do make the 
#              the temp files.
#          -v: Verbose
#          -k: clean backup files

args=`getopt "nvk" $*`
if test $? != 0
then
    echo "Usage: version_update_modules.sh [-nvk] <directories>"
    exit 2
fi

non_destructive=
verbose=
clean=

set -- $args

for i
do
    case "$i"
    in
        -n)
            non_destructive=yes
            shift;;
        -v)
            verbose=yes;
            shift;;
        -k)
            clean=yes;
            shift;;
        --)
            shift; break;;
    esac
done

verbose() {
    if test -n $verbose
    then
        echo $1
    fi
}

for module in $@
do
    echo "Entering $module"
    
    (cd $module

     # If the sentinel file is here, do nothing.
     if test -f version_updated
     then
	   echo "Found a 'version_updated' file, exiting."
	   exit 1
     fi

     # If the sentinel file was not found, update the module's version information,
     # by first creating the sentinel file... But don't make it in non_destructive mode
     if test -z $non_destructive
     then
        verbose "Updating sentinel file"
        cat <<EOF >version_updated
Updated on `date`
EOF
     fi
     
     # Get the version number and module from the Makefile.am.

     name=`grep '^M_NAME.*' Makefile.am | sed 's@M_NAME=\(.*\)$@\1@'`
     version=`grep '^M_VER.*' Makefile.am | sed 's@M_VER=\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\)$@\1@'`
     
     verbose "Name: $name; Version: $version"
     
     # gnarly awk code from stack overflow
     new_version=`echo $version | awk -F. -v OFS=. 'NF==1{print ++$NF}; NF>1{if(length($NF+1)>length($NF))$(NF-1)++; $NF=sprintf("%0*d", length($NF), ($NF+1)%(10^length($NF))); print}'`
     verbose "New Version: $new_version"

     # Update Makefile.am
     verbose "Updating Makefile.am"
     new_m_ver_line="M_VER=$new_version"
     sed "s/^M_VER.*/$new_m_ver_line/g" < Makefile.am > Makefile.am.tmp

     if test -z $non_destructive
     then
         mv Makefile.am Makefile.am.bak
         mv Makefile.am.tmp Makefile.am
     fi
     
     if test -n $clean
     then
	     rm Makefile.am.bak
     fi 

     # This ends the subshell that processes a given module
    )

done

