HTTP/1.0 200 OK
XDODS-Server: 3.0
XOPeNDAP-Server: 3.0
XDAP: 3.1
Date: *
Last-Modified: *
Content-Type: text/plain
Content-Description: dods_das

Attributes {
    HDF5_ROOT_GROUP {
        String Dataset "Dataset1";
        Group1 {
            Group2 {
                String Dataset "Dataset2", "Dataset3", "Dataset4";
                Group3 {
                    String Dataset "Dataset4";
                    Group4 {
                    }
                }
            }
        }
    }
    /Dataset1 {
        String HDF5_OBJ_FULLPATH "/Dataset1";
    }
    /Group1/ {
        String HDF5_OBJ_FULLPATH "/Group1/";
    }
    /Group1/Group2/ {
        String HDF5_OBJ_FULLPATH "/Group1/Group2/";
    }
    /Group1/Group2/Dataset2 {
        String HDF5_OBJ_FULLPATH "/Group1/Group2/Dataset2";
    }
    /Group1/Group2/Dataset3 {
        String HDF5_OBJ_FULLPATH "/Group1/Group2/Dataset3";
    }
    /Group1/Group2/Dataset4 {
        String HDF5_OBJ_FULLPATH "/Group1/Group2/Dataset4";
    }
    /Group1/Group2/Group3/ {
        String HDF5_OBJ_FULLPATH "/Group1/Group2/Group3/";
    }
    /Group1/Group2/Group3/Dataset4 {
        String HDF5_OBJ_FULLPATH "/Group1/Group2/Group3/Dataset4";
    }
    /Group1/Group2/Group3/Group4/ {
        String HDF5_OBJ_FULLPATH "/Group1/Group2/Group3/Group4/";
    }
}
