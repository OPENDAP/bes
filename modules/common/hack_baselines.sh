#!/bin/sh
#

DAP2_DATASET='<Dataset name="\(.*\)" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns="http://xml.opendap.org/ns/DAP2" xsi:schemaLocation="http://xml.opendap.org/ns/DAP2 http://xml.opendap.org/dap/dap2.xsd">'

DAP3_2_DATASET='<Dataset name="\1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://xml.opendap.org/ns/DAP/3.2# http://xml.opendap.org/dap/dap3.2.xsd" xmlns:grddl="http://www.w3.org/2003/g/data-view#" grddl:transformation="http://xml.opendap.org/transforms/ddxToRdfTriples.xsl" xmlns="http://xml.opendap.org/ns/DAP/3.2#" xmlns:dap="http://xml.opendap.org/ns/DAP/3.2#" dapVersion="3.2">'

while read -r file
do

    echo $file
    
    sed 's@<dataBLOB href=""/>@<blob href="cid:"/>@g' < $file > $file.tmp

    sed "s@$DAP2_DATASET@$DAP3_2_DATASET@g" < $file.tmp > $file.dap32

    mv $file $file.dap2

    cp $file.dap32 $file
done


