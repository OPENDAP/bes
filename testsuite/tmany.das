HTTP/1.0 200 OK
XDODS-Server: *
XOPeNDAP-Server: *
XDAP: *
Date: *
Last-Modified: *
Content-Type: text/plain
Content-Description: dods_das

Attributes {
    g1 {
        String HDF5_OBJ_FULLPATH "/g1/";
    }
    g1.1 {
        String HDF5_OBJ_FULLPATH "/g1/g1.1/";
    }
    dset1 {
        String HDF5_OBJ_FULLPATH "/g1/g1.1/dset1";
        Byte attr1 97, 98, 99, 100, 101, 102, 103, 104, 105, 0;
        Int32 attr2 0, 1, 2, 3;
        Float64 attr3 0., 0.10000000000000001, 0.20000000000000001, 0.30000000000000004, 0.40000000000000002, 0.5, 0.60000000000000009, 0.70000000000000007, 0.80000000000000004, 0.90000000000000002;
    }
    g1.2 {
        String HDF5_OBJ_FULLPATH "/g1/g1.2/";
    }
    link1 {
        String HDF5_OBJ_FULLPATH "/g1/g1.2/link1";
        Byte attr1 97, 98, 99, 100, 101, 102, 103, 104, 105, 0;
        Int32 attr2 0, 1, 2, 3;
        Float64 attr3 0., 0.10000000000000001, 0.20000000000000001, 0.30000000000000004, 0.40000000000000002, 0.5, 0.60000000000000009, 0.70000000000000007, 0.80000000000000004, 0.90000000000000002;
    }
    g2 {
        String HDF5_OBJ_FULLPATH "/g2/";
    }
    HDF5_softlink1 {
        String slink2 "/g1";
    }
    g3 {
        String HDF5_OBJ_FULLPATH "/g3/";
    }
    link3 {
        String HDF5_OBJ_FULLPATH "/g3/link3";
    }
    g4 {
        String HDF5_OBJ_FULLPATH "/g4/";
    }
    dset2 {
        String HDF5_OBJ_FULLPATH "/g4/dset2";
    }
    g5 {
        String HDF5_OBJ_FULLPATH "/g5/";
    }
    HDF5_softlink2 {
        String slink4 "/g6/dset3";
    }
    g6 {
        String HDF5_OBJ_FULLPATH "/g6/";
    }
    dset3 {
        String HDF5_OBJ_FULLPATH "/g6/dset3";
    }
    g7 {
        String HDF5_OBJ_FULLPATH "/g7/";
    }
    HDF5_softlink3 {
        String slink5 "/g8/elink";
    }
    HDF5_softlink4 {
        String slink6 "/g8/udlink";
    }
    g8 {
        String HDF5_OBJ_FULLPATH "/g8/";
    }
}
