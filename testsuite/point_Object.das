HTTP/1.0 200 OK
XDODS-Server: *
XOPeNDAP-Server: *
XDAP: *
Date: *
Last-Modified: *
Content-Type: text/plain
Content-Description: dods_das

Attributes {
    HDFEOS {
        String HDF5_OBJ_FULLPATH "/HDFEOS/";
    }
    ADDITIONAL {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ADDITIONAL/";
    }
    FILE_ATTRIBUTES {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES/";
    }
    POINTS {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/";
    }
    OMI%20L2G%20Test {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/OMI L2G Test/";
        UInt32 _LevelWritten 1, 1, 0, 0, 0, 0, 0, 0;
    }
    Data {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/OMI L2G Test/Data/";
    }
    Measurements {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/OMI L2G Test/Data/Measurements";
    }
    Stations {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/OMI L2G Test/Data/Stations";
    }
    Linkage {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/OMI L2G Test/Linkage/";
    }
    BCKPOINTER%3a1-%3e0 {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/OMI L2G Test/Linkage/BCKPOINTER:1->0";
    }
    FWDPOINTER%3a0-%3e1 {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/OMI L2G Test/Linkage/FWDPOINTER:0->1";
    }
    HDFEOS%20INFORMATION {
        String HDF5_OBJ_FULLPATH "/HDFEOS INFORMATION/";
        String HDFEOSVersion "HDFEOS_5.1.10";
    }
    /HDFEOS%20INFORMATION/StructMetadata.0 {
        SwathStructure {
        }
        GridStructure {
        }
        PointStructure {
            POINT_1 {
                String PointName "OMI L2G Test";
                Level {
                    Level_0 {
                        String LevelName "Stations";
                        PointField_1 {
                            String PointFieldName "Name";
                            Int32 Order 0;
                        }
                        PointField_2 {
                            String PointFieldName "Latitude";
                            Int32 Order 0;
                        }
                        PointField_3 {
                            String PointFieldName "Longitude";
                            Int32 Order 0;
                        }
                        PointField_4 {
                            String PointFieldName "Altitude";
                            Int32 Order 0;
                        }
                        PointField_5 {
                            String PointFieldName "ID";
                            Int32 Order 0;
                        }
                    }
                    Level_1 {
                        String LevelName "Measurements";
                        PointField_1 {
                            String PointFieldName "Name";
                            Int32 Order 0;
                        }
                        PointField_2 {
                            String PointFieldName "Latitude";
                            Int32 Order 0;
                        }
                        PointField_3 {
                            String PointFieldName "Longitude";
                            Int32 Order 0;
                        }
                        PointField_4 {
                            String PointFieldName "ColumnAmountO3";
                            Int32 Order 0;
                        }
                    }
                }
                LevelLink {
                    LevelLink_1 {
                        String Parent "Stations";
                        String Child "Measurements";
                        String LinkField "Name";
                    }
                }
            }
            Alias OMI_L2G_Test POINT_1;
        }
        ZaStructure {
        }
    }
}
