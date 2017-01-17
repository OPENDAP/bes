#!/bin/sh

function getit()
{
FILENAME=$1
DATASET_NAME=$2
DAP4_CE=$3
CATALOG_URL="http://test.opendap.org:8080/opendap/cloudydap"
DAP4_URL="$CATALOG_URL/$DATASET_NAME.dap?dap4.ce=$DAP4_CE"

echo "Retrieving: $DAP4_URL"
curl -s "$DAP4_URL" | getdap4 -D -M - | sed -e "s|$CATALOG_URL|data|g" > $FILENAME.baseline


cmdDoc=`cat <<EOF 
<?xml version="1.0" encoding="UTF-8"?>
<bes:request xmlns:bes="http://xml.opendap.org/ns/bes/1.0#" reqID="[http-bio-8080-exec-1:20:gateway_request]">
  <bes:setContext name="bes_timeout">300</bes:setContext>
  <bes:setContext name="xdap_accept">2.0</bes:setContext>
  <bes:setContext name="dap_explicit_containers">no</bes:setContext>
  <bes:setContext name="errors">xml</bes:setContext>
  <bes:setContext name="max_response_size">0</bes:setContext>
  <bes:setContainer name="myContainer" space="catalog">data/$DATASET_NAME.dmrpp</bes:setContainer>
  <bes:define name="d1" space="default">
    <bes:container name="myContainer">
        <bes:dap4constraint>$DAP4_CE</bes:dap4constraint>
    </bes:container>
  </bes:define>
  <bes:get type="dap" definition="d1" />
</bes:request>
EOF
`

echo $cmdDoc > $FILENAME

}


###############################################################
# chunked_twoD.h5
#
#     Float32 d_4_chunks[100][100];
#     <h4:chunkDimensionSizes>50 50</h4:chunkDimensionSizes>
function mk2D(){
    echo "MAKING Constrained 2D Array Baselines"
    getit chunked_twoD_CE_entire_array.bescmd chunked_twoD.h5 d_4_chunks[0:1:99][0:1:99]
    getit chunked_twoD_CE_chunk_0.bescmd chunked_twoD.h5 d_4_chunks[13:7:44][5:1:41]
    getit chunked_twoD_CE_chunk_1.bescmd chunked_twoD.h5 d_4_chunks[3:5:32][53:1:66]
    getit chunked_twoD_CE_chunk_2.bescmd chunked_twoD.h5 d_4_chunks[61:8:97][13:1:38]
    getit chunked_twoD_CE_chunk_3.bescmd chunked_twoD.h5 d_4_chunks[63:4:99][57:1:76]
    getit chunked_twoD_CE_chunks_0-1.bescmd chunked_twoD.h5 d_4_chunks[13:7:44][47:1:53]
    getit chunked_twoD_CE_chunks_0-2.bescmd chunked_twoD.h5 d_4_chunks[13:7:64][17:1:41]
    getit chunked_twoD_CE_chunks_1-3.bescmd chunked_twoD.h5 d_4_chunks[13:5:76][62:1:87]
    getit chunked_twoD_CE_chunks_2-3.bescmd chunked_twoD.h5 d_4_chunks[77:3:99][36:1:61]
    getit chunked_twoD_CE_all_chunks.bescmd chunked_twoD.h5 d_4_chunks[13:7:69][34:1:60]
    getit chunked_twoD_CE_all_chunks_00.bescmd chunked_twoD.h5 d_4_chunks[13:7:69][30:10:59]
    getit chunked_twoD_CE_all_chunks_01.bescmd chunked_twoD.h5 d_4_chunks[13:7:69][34:3:60]
    echo "END Constrained 2D Array Baselines"
}
###############################################################
# chunked_twoD_asymmetric.h5
#
#     Float32 d_8_chunks[100][200];
#     <h4:chunkDimensionSizes>50 50</h4:chunkDimensionSizes>
function mk2DAsym(){
    echo "MAKING Constrained 2D Asymmetric Array Baselines"
    getit chunked_twoD_asym_CE_chunk_0.bescmd chunked_twoD_asymmetric.h5 d_8_chunks[13:7:44][5:2:41]
    getit chunked_twoD_asym_CE_chunk_1.bescmd chunked_twoD_asymmetric.h5 d_8_chunks[11:2:43][55:3:91]
    getit chunked_twoD_asym_CE_chunk_2.bescmd chunked_twoD_asymmetric.h5 d_8_chunks[21:2:33][106:2:132]
    getit chunked_twoD_asym_CE_chunk_3.bescmd chunked_twoD_asymmetric.h5 d_8_chunks[3:5:17][156:5:187]
    getit chunked_twoD_asym_CE_chunk_4.bescmd chunked_twoD_asymmetric.h5 d_8_chunks[63:7:94][5:2:41]
    getit chunked_twoD_asym_CE_chunk_5.bescmd chunked_twoD_asymmetric.h5 d_8_chunks[61:2:93][55:3:91]
    getit chunked_twoD_asym_CE_chunk_6.bescmd chunked_twoD_asymmetric.h5 d_8_chunks[71:2:83][106:2:132]
    getit chunked_twoD_asym_CE_chunk_7.bescmd chunked_twoD_asymmetric.h5 d_8_chunks[53:5:67][156:5:187]
    getit chunked_twoD_asym_CE_all_chunks.bescmd chunked_twoD_asymmetric.h5 d_8_chunks[13:9:87][11:15:187]
    
    # This one tests if we can skip chunks that don't fall in stride
    getit chunked_twoD_asym_CE_stride_skip_chunks_2-3.bescmd chunked_twoD_asymmetric.h5 d_8_chunks[13:7:44][0:161:199]

    echo "END Constrained 2D Asymmetric Array Baselines"
}


###############################################################
# chunked_threeD_asymmetric.h5
#
#     Float32 d_8_chunks[100][50][200];
#     <h4:chunkDimensionSizes>50 50 50</h4:chunkDimensionSizes>
function mk3DAsym(){
    echo "MAKING Constrained 3D Asymmetric Array Baselines"
    getit chunked_threeD_asym_CE_chunk_0.bescmd chunked_threeD_asymmetric.h5 d_8_chunks[13:7:44][2:3:10][5:2:41]
    getit chunked_threeD_asym_CE_chunk_1.bescmd chunked_threeD_asymmetric.h5 d_8_chunks[11:2:43][2:4:10][55:3:91]
    getit chunked_threeD_asym_CE_chunk_2.bescmd chunked_threeD_asymmetric.h5 d_8_chunks[21:2:33][2:3:10][106:2:132]
    getit chunked_threeD_asym_CE_chunk_3.bescmd chunked_threeD_asymmetric.h5 d_8_chunks[3:5:17][2:4:10][156:5:187]
    getit chunked_threeD_asym_CE_chunk_4.bescmd chunked_threeD_asymmetric.h5 d_8_chunks[63:7:94][2:3:10][5:2:41]
    getit chunked_threeD_asym_CE_chunk_5.bescmd chunked_threeD_asymmetric.h5 d_8_chunks[61:2:93][2:2:10][55:3:91]
    getit chunked_threeD_asym_CE_chunk_6.bescmd chunked_threeD_asymmetric.h5 d_8_chunks[71:2:83][2:5:10][106:2:132]
    getit chunked_threeD_asym_CE_chunk_7.bescmd chunked_threeD_asymmetric.h5 d_8_chunks[53:5:67][2:4:10][156:5:187]
    
    getit chunked_threeD_asym_CE_all_chunks.bescmd chunked_threeD_asymmetric.h5 d_8_chunks[13:9:87][2:2:4][11:15:187]
 
    echo "END Constrained 3D Asymmetric Array Baselines"
}




###############################################################
# chunked_fourD.h5
#
#     Float32 d_8_chunks[40][40][40];
#     <h4:chunkDimensionSizes>20 20 20 20</h4:chunkDimensionSizes>
function mk4D(){
    echo "MAKING Constrained 4D Array Baselines"
    getit chunked_fourD_CE_chunk_00.bescmd chunked_fourD.h5 d_16_chunks[0:2:7][0:2:7][0:2:7][0:2:7]
    getit chunked_fourD_CE_chunk_01.bescmd chunked_fourD.h5 d_16_chunks[0:2:7][0:2:7][0:2:7][21:2:29]
    getit chunked_fourD_CE_chunk_02.bescmd chunked_fourD.h5 d_16_chunks[0:2:7][0:2:7][21:2:29][0:2:7]
    getit chunked_fourD_CE_chunk_03.bescmd chunked_fourD.h5 d_16_chunks[0:2:7][0:2:7][21:2:29][21:2:29]
    getit chunked_fourD_CE_chunk_04.bescmd chunked_fourD.h5 d_16_chunks[0:2:7][21:2:29][0:2:7][0:2:7]
    getit chunked_fourD_CE_chunk_05.bescmd chunked_fourD.h5 d_16_chunks[0:2:7][21:2:29][0:2:7][21:2:29]
    getit chunked_fourD_CE_chunk_06.bescmd chunked_fourD.h5 d_16_chunks[0:2:7][21:2:29][21:2:29][0:2:7]
    getit chunked_fourD_CE_chunk_07.bescmd chunked_fourD.h5 d_16_chunks[0:2:7][21:2:29][21:2:29][21:2:29]
    getit chunked_fourD_CE_chunk_08.bescmd chunked_fourD.h5 d_16_chunks[21:2:29][0:2:7][0:2:7][0:2:7]
    getit chunked_fourD_CE_chunk_09.bescmd chunked_fourD.h5 d_16_chunks[21:2:29][0:2:7][0:2:7][21:2:29]
    getit chunked_fourD_CE_chunk_10.bescmd chunked_fourD.h5 d_16_chunks[21:2:29][0:2:7][21:2:29][0:2:7]
    getit chunked_fourD_CE_chunk_11.bescmd chunked_fourD.h5 d_16_chunks[21:2:29][0:2:7][21:2:29][21:2:29]    
    getit chunked_fourD_CE_chunk_12.bescmd chunked_fourD.h5 d_16_chunks[21:2:29][21:2:29][0:2:7][0:2:7]
    getit chunked_fourD_CE_chunk_13.bescmd chunked_fourD.h5 d_16_chunks[21:2:29][21:2:29][0:2:7][21:2:29]
    getit chunked_fourD_CE_chunk_14.bescmd chunked_fourD.h5 d_16_chunks[21:2:29][21:2:29][21:2:29][0:2:7]
    getit chunked_fourD_CE_chunk_15.bescmd chunked_fourD.h5 d_16_chunks[21:2:29][21:2:29][21:2:29][21:2:29]
    
    getit chunked_fourD_CE_all_chunks.bescmd chunked_fourD.h5 d_16_chunks[0:9:29][5:7:29][11:5:33][3:11:31]
    echo "END Constrained 4D Array Baselines"
}

function mkBadChunkAlignmentTests(){
    echo "MAKING Bad Chunk Alignment Baselines"
    getit chunked_threeD_asymmetric_uneven_entire_array.h5.bescmd chunked_threeD_asymmetric_uneven.h5 d_odd_chunks
    getit chunked_threeD_asymmetric_uneven_CE_00.h5.bescmd chunked_threeD_asymmetric_uneven.h5 d_odd_chunks[0:1:99][0:1:49][0:1:199]
    getit chunked_threeD_asymmetric_uneven_CE_01.h5.bescmd chunked_threeD_asymmetric_uneven.h5 d_odd_chunks[0:1:99][0:25:49][0:1:199]
    getit chunked_threeD_asymmetric_uneven_CE_02.h5.bescmd chunked_threeD_asymmetric_uneven.h5 d_odd_chunks[0:2:99][0:2:49][0:2:199]
    getit chunked_threeD_asymmetric_uneven_CE_03.h5.bescmd chunked_threeD_asymmetric_uneven.h5 d_odd_chunks[0:3:99][0:3:49][0:3:199]
    getit chunked_threeD_asymmetric_uneven_CE_04.h5.bescmd chunked_threeD_asymmetric_uneven.h5 d_odd_chunks[0:25:99][0:25:49][0:50:199]
    echo "END Bad Chunk Alignment Baselines"
}

function mkStuff(){
    echo "MAKING Stuff"
    getit chunked_fourD_CE_chunk_00.bescmd chunked_threeD_asymmetric_uneven.h5 d_odd_chunks[13:9:87][2:2:4][11:15:187]
    echo "END Stuff"
}



#mk2D;
#mk2DAsym;
#mk3DAsym;
#mk4D;

mkBadChunkAlignmentTests;




