HTTP/1.0 200 OK
XDODS-Server: 3.0
XOPeNDAP-Server: 3.0
XDAP: *
Date: *
Last-Modified: *
Content-Type: text/plain
Content-Description: dods_das

Attributes {
    HDF5_ROOT_GROUP {
        String Dataset "Dataset3";
        Group1 {
            String Dataset "Dataset1", "Dataset2";
        }
    }
    /Dataset3 {
        String HDF5_OBJ_FULLPATH "/Dataset3";
    }
    /Group1/ {
        String HDF5_OBJ_FULLPATH "/Group1/";
        String HDF5_COMMENT "Foo!";
    }
    /Group1/Dataset1 {
        String HDF5_OBJ_FULLPATH "/Group1/Dataset1";
    }
    /Group1/Dataset2 {
        String HDF5_OBJ_FULLPATH "/Group1/Dataset2";
    }
}
