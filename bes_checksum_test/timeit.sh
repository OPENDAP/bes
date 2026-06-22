#!/bin/bash
export H1="##############################################################"
export H2="--------------------------------------------------------------"
export H3="-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"


#########################################################################
# loggy()
#
# A handy logging op.
#
function loggy(){
    if test -n "${log_file}"
    then
        echo  "$@"  | awk '{ print "# "$0;}' | tee -a "${log_file}" >&2
    else
        echo  "$@" | awk '{ print "# "$0;}'  >&2
    fi
}

#########################################################################
# timed_retrieval()
#
# Run a bescmd that asks for a dap4 data response.
# Returns the time, as ms, that the transmit_data operation took to 
# complete.
#
function timed_retrieval(){
    local cmd_file="$1"
    local target_file="$2"
 
    echo "" > "$bes_log"
    real_time=$( (time -p besstandalone -c "$bes_conf" -i "$cmd_file" -f "$target_file"; ) 2>&1 | grep real )
    loggy "time(): $real_time"
    
    # Look in the bes_log and use our friend grep to find all the transmit data lines, 
    # Format them as JSON using beslog2json.py, truning on the timing output, nad 
    cmd_timing=$(cat "$bes_log" | grep transmit_data | beslog2json.py -t t | \
            grep -E "transmit_data" | grep "get dap for d1" | jq '."elapsed-us"')
    cmd_timing=$(cat "$bes_log" | grep transmit_data | beslog2json.py -t t | \
                 grep "get dap for d1" | jq '."elapsed-us"')
    loggy "command_timing: $cmd_timing"
    echo "$cmd_timing"
}

function padme(){
    local value="$1"
    
    local retval=$value
    if test $value -lt 10;   then retval="0${retval}"; fi
    if test $value -lt 100;  then retval="0${retval}"; fi
    if test $value -lt 1000; then retval="0${retval}"; fi
    
    echo $retval
}


function fnoc1_test_config() {
    chksums_true_bescmd="fnoc1_chksum_true.bescmd.xml"
    chksums_false_bescmd="fnoc1_chksum_false.bescmd.xml"
    no_chksums_bescmd="fnoc1_no_chksum.bescmd.xml"
}

function coads_test_config() {
    chksums_true_bescmd="coads_chksum_true.bescmd.xml"
    chksums_false_bescmd="coads_chksum_false.bescmd.xml"
    no_chksums_bescmd="coads_no_chksum.bescmd.xml"
}

function daymet_test_config() {
    chksums_true_bescmd="daymet_chksum_true.bescmd.xml"
    chksums_false_bescmd="daymet_chksum_false.bescmd.xml"
    no_chksums_bescmd="daymet_no_chksum.bescmd.xml"
}

#---------------------------------------------------------------------
# The Settings

loggy "$H1"

# BES Things
prefix=${prefix:-"/Users/ndp/OPeNDAP/hyrax/build"}
loggy "                prefix: $prefix"

bes_log="$prefix/var/bes.log"
loggy "               bes_log: $bes_log"

bes_conf="$prefix/etc/bes/bes.conf"
loggy "              bes_conf: $bes_conf"

results_dir="./results"
loggy "           results_dir: $results_dir"
mkdir -p "$results_dir"
loggy ""

export chksums_true_bescmd
export chksums_false_bescmd
export no_chksums_bescmd


#fnoc1_test_config
#daymet_test_config
coads_test_config

loggy "           chksum_true: $chksums_true_bescmd"
loggy "  chksums_false_bescmd: $chksums_false_bescmd"
loggy "     no_chksums_bescmd: $no_chksums_bescmd"


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# Checksum True Files
#
chksums_true_file="checksums_true.csv"
loggy "     chksums_true_file: $chksums_true_file"

chksums_true_dap_file="checksums_true"
loggy " chksums_true_dap_file: $chksums_true_dap_file "

loggy ""

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# Checksum False Files
#
chksums_false_file="checksums_false.csv"
loggy "    chksums_false_file: $chksums_false_file"

chksums_false_dap_file="checksums_false"
loggy "chksums_false_dap_file: $chksums_false_dap_file "

loggy ""

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# No Checksum Files
#
no_chksums_file="no_checksums.csv"
loggy "       no_chksums_file: $no_chksums_file"

no_chksums_dap_file="no_checksums"
loggy "   no_chksums_dap_file: $no_chksums_dap_file "

loggy ""


#---------------------------------------------------------------------
#
# Reset output files and write header.
#
echo "rep, $chksums_true_file" > "$chksums_true_file"
echo "rep, $chksums_false_file" > "$chksums_false_file"
echo "rep, $no_chksums_file" > "$no_chksums_file"

rm -rv "${results_dir}"
mkdir -p "${results_dir}"

#---------------------------------------------------------------------
# Do The Reps
#
loggy "$H2"
for i in {1..10}
do
    loggy "Rep: $i"
    
    lapstr=$(padme $i)
    
    loggy "Request Checksums True"
    target_file="${results_dir}/${chksums_true_dap_file}_${lapstr}.dap"
    # target_file="/dev/null"
    loggy "target_file: $target_file"
    
    echo -n "$i, " >> "$chksums_true_file"
    time_in_ms=$(timed_retrieval "$chksums_true_bescmd" "${target_file}" )
    echo "$time_in_ms" >> "$chksums_true_file"
    loggy $(ls -l "$target_file")
    loggy ""
    
    loggy "Request Checksums False"
    target_file="${results_dir}/${chksums_false_dap_file}_${lapstr}.dap"
    # target_file="/dev/null"
    loggy "target_file: $target_file"
    
    echo -n "$i, " >> "$chksums_false_file"
    time_in_ms=$(timed_retrieval "$chksums_false_bescmd" "${target_file}" )
    echo "$time_in_ms" >> "$chksums_false_file"
    loggy $(ls -l "$target_file")
    loggy ""
    
    loggy "Request No Checksums"
    target_file="${results_dir}/${no_chksums_dap_file}_${lapstr}.dap"
    # target_file="/dev/null"
    loggy "target_file: $target_file"
    
    echo -n "$i, " >> "$no_chksums_file"
    time_in_ms=$(timed_retrieval "$no_chksums_bescmd" "${target_file}" )
    echo "$time_in_ms" >> "$no_chksums_file"
    loggy $(ls -l "$target_file")
    loggy "$H3"
done



