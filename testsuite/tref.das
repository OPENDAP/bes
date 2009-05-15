HTTP/1.0 200 OK
XDODS-Server: *
XOPeNDAP-Server: *
XDAP: *
Date: *
Last-Modified: *
Content-Type: text/plain
Content-Description: dods_das

Attributes {
    HDF5_ROOT_GROUP {
        Group1 {
            String Dataset "Dataset1", "Dataset2";
        }
        String Dataset "ZZZDataset3";
    }
    /Group1/ {
        String HDF5_OBJ_FULLPATH "/Group1/";
    }
    /Group1/Dataset1 {
        String HDF5_OBJ_FULLPATH "/Group1/Dataset1";
    }
    /Group1/Dataset2 {
        String HDF5_OBJ_FULLPATH "/Group1/Dataset2";
    }
    /ZZZDataset3 {
        String HDF5_OBJ_FULLPATH "/ZZZDataset3";
    }
}
