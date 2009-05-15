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
        String Dataset "dset1";
    }
    /dset1 {
        String HDF5_OBJ_FULLPATH "/dset1";
        String attr1 "String withsp   ", "String\\withsl   ", "String&withamp  ", "String<withlt   ", "String>withgt   ", "String'withapos ", "String"withquot ";
    }
}
