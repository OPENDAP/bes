#!/bin/sh


#export dap_server="http://ec2-54-242-224-73.compute-1.amazonaws.com:8080"
#export data_set="/opendap/hyrax/ebs/Ike/2D_varied_manning_windstress/test_dir-norename.ncml"

export dap_server="http://localhost:8080"
export data_set="/opendap/hyrax/ugrids/Ike/2D_varied_manning_windstress/test_dir-norename.ncml"


export requestSuffix=".dds"

# Unescaped:     ugr5(0,ua,"29.3<lat&lat<29.8&-95.0>lon&lon>-94.4")
# Shell Escaped: ugr5\(0,ua,\"29.3\<lat\&lat\<29.8\&-95.0\>lon\&lon\>-94.4\"\)
echo "curl $dap_server$data_set$requestSuffix\?ugr5\(0,ua,\\\"29.3\<lat\&lat\<29.8\&-95.0\>lon\&lon\<-94.4\\\"\)"




# Unescaped:     ugr5(0,ua,"28.0<lat&lat<29.0&-89.0<lon&lon<-88.0")
# Shell Escaped: ugr5\(0,ua,\"28.0\<lat\&lat\<29.0\&-89.0\<lon\&lon\<-88.0\"\)
echo "curl $dap_server$data_set$requestSuffix\?ugr5\(0,ua,\\\"28.0\<lat\&lat\<29.0\&-89.0\<lon\&lon\<-88.0\\\"\)"


