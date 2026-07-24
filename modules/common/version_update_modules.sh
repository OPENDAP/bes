#!/bin/bash
#
# Update the handler version numbers in the configure.ac, Makefile.am,
# NEWS.md and update the ChangeLog with the git log info for each of the
# bes modules given on the command line.
#
# This script automates rote updates when little has changed in a
# handler. It will update the NEWS.md file, but not README or INSTALL.
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
HR="########################################################################"
HR3="--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---"
HR2="-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
HR1="- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -"

function loggy() {
    echo  "$@" | awk '{ print "# version_update_modules.sh() - "$0;}'  >&2
    return $?
}

function vlog() {
    if [[ -n "$verbose" ]]; then loggy "$@"; fi
    return $?
}

args=$(getopt "nvk" $*)
if test $? != 0
then
    loggy "Usage: version_update_modules.sh [-nvk]"
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



modules=$(cat all_modules.txt)

for module in $modules
do
    vlog "$HR1"
    vlog "BEGIN module: $module"

    (cd ../$module

     # If the sentinel file is here, do nothing.
     if [[ -f version_updated ]]; then
       loggy "Found a 'version_updated' file, exiting."
       exit 1
     fi

     # If the sentinel file was not found, update the module's version information,
     # by first creating the sentinel file... But don't make it in non_destructive mode
     if  [[ -z $non_destructive ]]
     then
        vlog "Updating sentinel file"
        echo "Updated on $(date)"  > version_updated
     fi
     
     # Get the version number and module from the Makefile.am.

     name=$(grep '^M_NAME.*' Makefile.am | sed 's@M_NAME=\(.*\)$@\1@')
     version=$(grep '^M_VER.*' Makefile.am | sed 's@M_VER=\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\)$@\1@')
     
     vlog "In $module/Makefile.am found: name: $name; version: $version"
     
     # gnarly awk code from stack overflow
     new_version=`echo $version | awk -F. -v OFS=. 'NF==1{print ++$NF}; NF>1{if(length($NF+1)>length($NF))$(NF-1)++; $NF=sprintf("%0*d", length($NF), ($NF+1)%(10^length($NF))); print}'`
     vlog "Updating to new_version: '$new_version'"

     # Update Makefile.am
     vlog "Updating Makefile.am"
     new_m_ver_line="M_VER=$new_version"
     vlog "new_m_ver_line: '$new_m_ver_line'"

     sed "s/^M_VER.*/$new_m_ver_line/g" < Makefile.am > Makefile.am.tmp

     if [[ -z "$non_destructive" ]]
     then
         vlog "Updating Makefile.am"
         vlog "$(mv -v Makefile.am Makefile.am.bak)"
         vlog "$(mv -v Makefile.am.tmp Makefile.am)"
     fi
     
     if [[ -n "$clean" ]]
     then
         vlog "Removing backup file."
	       rm -v Makefile.am.bak
     fi 

     # This ends the subshell that processes a given module
    )
    vlog "END module: $module"

done


