#!/bin/bash
#
# Update the handler version numbers in the configure.ac, Makefile.am, NEWS.
# Update the ChangeLog with the git log info.

(cd modules

 for module in "$*"
 do

     (cd $module

      # Test the sentinel file. If this is here, do nothing.
      if test -f version_updated
      then
	  echo "Found a 'version_updated' file, exiting."
	  exit 1
      fi

      # If the sentinel file was not found, update the module's version information,
      # by first creating teh sentinel file...
      cat <<EOF >version_updated
Updated on `date`
EOF
      
      # Get the version number and module from the configure.ac.

      name=`grep AC_INIT configure.ac | sed 's@AC_INIT(\(.*\),.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\),.*@\1@'`
      version=`grep AC_INIT configure.ac | sed 's@AC_INIT(\(.*\),.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\),.*@\2@'`

      # gnarly awk code from stack overflow
      new_version=`echo $version | awk -F. -v OFS=. 'NF==1{print ++$NF}; NF>1{if(length($NF+1)>length($NF))$(NF-1)++; $NF=sprintf("%0*d", length($NF), ($NF+1)%(10^length($NF))); print}'`
      echo "New Version: $new_version"

      # Update configure.ac
      new_ac_init_line="AC_INIT($name, $new_version, opendap-tech@opendap.org)"
      sed "s/AC_INIT.*/$new_ac_init_line/g" < configure.ac > configure.ac.tmp

      mv configure.ac configure.ac.bak
      mv configure.ac.tmp configure.ac

      # Update Makefile.am

      new_m_ver_line="M_VER=$new_version"
      sed "s/M_VER.*/$new_m_ver_line/g" < Makefile.am > Makefile.am.tmp

      mv Makefile.am Makefile.am.bak
      mv Makefile.am.tmp Makefile.am

      # Update ChangeLog
      start_date=`awk '/....-..-../ {print $1 ; exit}' ChangeLog`
      if test -z "$start_date"; then start_date="1970-01-01"; fi

      gitlog-to-changelog --since=$start_date > ChangeLog.tmp.top
      cat ChangeLog.tmp.top ChangeLog > ChangeLog.tmp
      rm ChangeLog.tmp.top

      mv ChangeLog ChangeLog.bak
      mv ChangeLog.tmp ChangeLog

      # Update NEWS

      cat <<EOF >NEWS.tmp.top
News for version $new_version

Minor updates, see the ChangeLog

EOF

      cat NEWS.tmp.top NEWS > NEWS.tmp
      rm NEWS.tmp.top

      mv NEWS NEWS.bak
      mv NEWS.tmp NEWS

      # This ends the subshell that processes a given module
     )

 done
 
 # This ends the subshell the CDs to bes/modules
)
