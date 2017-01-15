#!/bin/sh

function getit()
{
FILENAME=$1
DATASET_NAME=$2
CE=$3
CATALOG_URL="http://test.opendap.org:8080/opendap/cloudydap"

curl -s "$CATALOG_URL/$DATASET_NAME.dap?dap4.ce=$CE" | getdap4 -D -M - | sed -e "s|$CATALOG_URL|data|g" > $FILENAME
}


#getit chunked_twoD_CE_entire_array.bescmd.baseline chunked_twoD.h5 d_4_chunks[0:1:99][0:1:99]
#getit chunked_twoD_CE_quad_1.bescmd.baseline chunked_twoD.h5 d_4_chunks[13:7:44][5:1:41]
#getit chunked_twoD_CE_quad_2.bescmd.baseline chunked_twoD.h5 d_4_chunks[3:5:32][53:1:66]
#getit chunked_twoD_CE_quad_3.bescmd.baseline chunked_twoD.h5 d_4_chunks[61:8:97][13:1:38]
#getit chunked_twoD_CE_quad_4.bescmd.baseline chunked_twoD.h5 d_4_chunks[63:4:99][57:1:76]
#getit chunked_twoD_CE_quads_1-2.bescmd.baseline chunked_twoD.h5 d_4_chunks[13:7:44][47:1:53]
#getit chunked_twoD_CE_quads_1-3.bescmd.baseline chunked_twoD.h5 d_4_chunks[13:7:64][17:1:41]
#getit chunked_twoD_CE_quads_2-4.bescmd.baseline chunked_twoD.h5 d_4_chunks[13:5:76][62:1:87]
#getit chunked_twoD_CE_quads_3-4.bescmd.baseline chunked_twoD.h5 d_4_chunks[77:3:99][36:1:61]
#getit chunked_twoD_CE_all_quads.bescmd.baseline chunked_twoD.h5 d_4_chunks[13:7:69][34:1:60]


#getit chunked_twoD_CE_all_quads_01.bescmd.baseline chunked_twoD.h5 d_4_chunks[13:7:69][30:10:59]
getit chunked_twoD_CE_all_quads_02.bescmd.baseline chunked_twoD.h5 d_4_chunks[13:7:69][34:3:60]


