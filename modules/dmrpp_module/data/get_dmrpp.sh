#!/bin/bash
set -e;

CONF_FILE_TEMPLATE="bes.hdf5.cf.conf"
data_root=`pwd`;

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

while getopts "h?vVru:d:o:" opt; do
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
        dmrpp_url="$OPTARG"
        ;;
    d)
        data_root="$OPTARG"
        ;;
    o)
        output_file="$OPTARG"
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

if test -n "$very_verbose"
then
    set -x;
fi

input_data_file="${1}";

if test -z "${output_file}"
then
    output_file="${input_data_file}.dmrpp"
fi

if test -n "${verbose}"
then
	echo "OUTPUT_FILE: '${output_file}'";
	echo "just_dmr: '${just_dmr}'";
	echo "dmrpp_url: '${dmrpp_url}'";
fi

###############################################################################
#
# Build the (temporary) DMR file for the target dataset.
#
function get_dmr() {
	# build the xml file
	DATAFILE="${1}";
	
cmdDoc=`cat <<EOF 
<?xml version="1.0" encoding="UTF-8"?>
<bes:request xmlns:bes="http://xml.opendap.org/ns/bes/1.0#" reqID="get_dmrpp.sh">
  <bes:setContext name="dap_explicit_containers">no</bes:setContext>
  <bes:setContext name="errors">xml</bes:setContext>
  <bes:setContext name="max_response_size">0</bes:setContext>
  
  <bes:setContainer name="c">$DATAFILE</bes:setContainer>
  
  <bes:define name="d" space="default">
    <bes:container name="c">
    </bes:container>
  </bes:define>
  
  <bes:get type="dmr" definition="d" />
  
</bes:request>
EOF
`
	TMP_CMD=$(mktemp -t get_dmr_XXXX)	
    echo "${cmdDoc}" > $TMP_CMD
	
	if test -n "$very_verbose"
	then
	    echo "TMP_CMD: $TMP_CMD"
	    cat $TMP_CMD
	fi
	
	TMP_CONF=$(mktemp -t conf_XXXX)
	
	# Use the cwd as the BES's Data Root directory - this is a trick so that the
	# script can get a DMR using the HDF5 handler algorithm, as tweaked by the 
	# handler's configuration parameters in the bes.hdf5.cf.template.conf file.
	echo "Checking for ${CONF_FILE_TEMPLATE} (pwd: "`pwd`")";
	if  [ ! -f "${CONF_FILE_TEMPLATE}" ]
	then
		echo "ERROR: Missing BES configuration file template \"${CONF_FILE_TEMPLATE}\"";
		echo "       Try running \"make check\" first.";
		exit 1;
	fi

	cat  ${CONF_FILE_TEMPLATE} | sed -e "s%[@]hdf5_root_directory[@]%${data_root}%" > ${TMP_CONF};
	
	if test -n "$very_verbose"
	then
	    echo "TMP_CONF: ${TMP_CONF}"
	    cat ${TMP_CONF}
	fi
	
	TMP_DMR_RESP=$(mktemp -t dmr_XXXX)
	
	# use besstandalone to get the DMR
	besstandalone -c ${TMP_CONF} -i ${TMP_CMD} > ${TMP_DMR_RESP}
	
	if test -n "$verbose" || test -n "$just_dmr"
	then
	    echo "DMR:"
	    cat ${TMP_DMR_RESP}
	fi

}
###############################################################################

###############################################################################
function mk_dmrpp() {
    datafile="${1}";
	if test -z "${just_dmr}"
	then
	    ./build_dmrpp ${verbose} -c "${TMP_CONF}" -f "${data_root}/${datafile}" -r "${TMP_DMR_RESP}" -u "${dmrpp_url}" > "${output_file}";
	else
	    echo "The just_dmr flag is set, skipping dmr++ construction."
	fi
	
	# TODO Use trap to ensure these are really removed
	rm $DMR_CMD $TMP_DMR_RESP $TMP_CONF
}
 ###############################################################################
 
get_dmr  "${input_data_file}";
mk_dmrpp "${input_data_file}";
