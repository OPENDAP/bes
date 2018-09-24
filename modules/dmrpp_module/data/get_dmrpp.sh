#!/bin/sh

show_usage() {
    cat <<EOF

 Usage: $0 [options] <hdf5 file>

 Write the DMR++ for hdf5_file to stdout
 
 The CWD is set as the BES Data Root directory. This utility will
 add an entry to the bes.log specified in the configuration file.
 The DMR++ is built using the DMR as returned by the HDF5 handler,
 using options as set in the bes configuration file found here.
 
 -h: Show help
 -v: Verbose: Print the DMR too
 -V: Very Verbose: print the DMR, the command and the configuration
     file used to build the DMR
 -r: Just print the DMR that will be used to build the DMR++
 -u: URL for the DMR++ file

 Limitations: 
 * The pathanme to the hdf5 file must be relative from the
   directory where this command was run; absolute paths won't work. 
 * The build_dmrpp command must be in the CWD. 
 * The bes conf template has to build by hand. jhrg 5/11/18
EOF
}

OPTIND=1        # Reset in case getopts has been used previously in this shell

verbose=
very_verbose=
just_dmr=
dmrpp_url=

while getopts "h?vVru:" opt; do
    case "$opt" in
    h|\?)
        show_usage
        exit 0
        ;;
    v)
        verbose="-v"
        echo "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -";
        echo "${0} - BEGIN (verbose)";
        ;;
    V)
        very_verbose="yes"
        verbose="-v"
         echo "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -";
        echo "${0} - BEGIN (very_verbose)";
       ;;
    r)
        just_dmr="yes"
        ;;
    u)
        dmrpp_url="-u $OPTARG"
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

# build the xml file
hdf5_file=$1
TMP_CMD=$(mktemp -t get_dmr_$$)

cat <<EOF > $TMP_CMD 
<?xml version="1.0" encoding="UTF-8"?>
<bes:request xmlns:bes="http://xml.opendap.org/ns/bes/1.0#" reqID="[http-8080-1:27:bes_request]">
  <bes:setContext name="xdap_accept">4.0</bes:setContext>
  <bes:setContext name="dap_explicit_containers">no</bes:setContext>
  <bes:setContext name="errors">xml</bes:setContext>
  <bes:setContext name="max_response_size">0</bes:setContext>
  
  <bes:setContainer name="c" space="catalog">$hdf5_file</bes:setContainer>
  
  <bes:define name="d" space="default">
    <bes:container name="c">
    </bes:container>
  </bes:define>
  
  <bes:get type="dmr" definition="d" />
  
</bes:request>
EOF

if test -n "$very_verbose"
then
    echo "$TMP_CMD: $TMP_CMD"
    cat $TMP_CMD
fi

TMP_CONF=$(mktemp -t conf_$$)

# Use the cwd as the BES's Data Root directory - this is a trick so that the
# script can get a DMR using the HDF5 handler algorithm, as tweaked by the 
# handler's configuration parameters in the bes.hdf5.cf.template.conf file.
sed -e "s%[@]hdf5_root_directory[@]%`pwd`%" < bes.hdf5.cf.template.conf > $TMP_CONF

if test -n "$very_verbose"
then
    echo "$TMP_CONF"
    cat $TMP_CONF
fi

TMP_DMR_RESP=$(mktemp -t dmr_$$)

# use besstandalone to get the DMR
besstandalone -c $TMP_CONF -i $TMP_CMD > $TMP_DMR_RESP

if test -n "$verbose" || test -n "$just_dmr"
then
    echo "DMR:"
    cat $TMP_DMR_RESP
fi

if test -z "$just_dmr"
then
    ./build_dmrpp $verbose -f $hdf5_file -r $TMP_DMR_RESP $dmrpp_url
fi

# TODO Use trap to ensure these are really removed
rm $DMR_CMD $TMP_DMR_RESP $TMP_CONF
 
