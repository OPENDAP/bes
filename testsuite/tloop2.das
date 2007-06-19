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
    HDF5_softlink1 {
        String g2.1 "/g1";
    }
    g2 {
        String HDF5_OBJ_FULLPATH "/g2/";
    }
    /g2/ {
        String HDF5_HARDLINK /g1/g1.1/;
    }
}
