HTTP/1.0 200 OK
XDODS-Server: *
XOPeNDAP-Server:*
XDAP: *
Date: *
Last-Modified: *
Content-Type: *
Content-Description: *

Attributes {
    /HDFEOS/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/";
    }
    /HDFEOS/ADDITIONAL/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ADDITIONAL/";
    }
    /HDFEOS/ADDITIONAL/FILE_ATTRIBUTES/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES/";
    }
    /HDFEOS/POINTS/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/";
    }
    /HDFEOS/POINTS/FixedBuoy%20Point/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FixedBuoy Point/";
        UInt32 _LevelWritten 1, 1, 0, 0, 0, 0, 0, 0;
        Int32 GlobalAttr_Integer 9999;
        Int32 GlobalAttribute_int 9999;
    }
    /HDFEOS/POINTS/FixedBuoy%20Point/Data/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FixedBuoy Point/Data/";
        Int32 GroupAttribute 1, 2, 3, 4, 5;
    }
    /HDFEOS/POINTS/FixedBuoy%20Point/Data/Desc-Loc {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FixedBuoy Point/Data/Desc-Loc";
    }
    /HDFEOS/POINTS/FixedBuoy%20Point/Data/Observations {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FixedBuoy Point/Data/Observations";
        Int32 LocalAttribute 10, 20, 30, 40, 50;
    }
    /HDFEOS/POINTS/FixedBuoy%20Point/Linkage/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FixedBuoy Point/Linkage/";
    }
    /HDFEOS/POINTS/FixedBuoy%20Point/Linkage/BCKPOINTER%3a1-%3e0 {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FixedBuoy Point/Linkage/BCKPOINTER:1->0";
    }
    /HDFEOS/POINTS/FixedBuoy%20Point/Linkage/FWDPOINTER%3a0-%3e1 {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FixedBuoy Point/Linkage/FWDPOINTER:0->1";
    }
    /HDFEOS/POINTS/FloatBuoy%20Point/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FloatBuoy Point/";
        UInt32 _LevelWritten 1, 1, 0, 0, 0, 0, 0, 0;
        Float32 GlobalAttr -7.5;
        Float32 GlobalAttribute_float -7.5;
    }
    /HDFEOS/POINTS/FloatBuoy%20Point/Data/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FloatBuoy Point/Data/";
    }
    /HDFEOS/POINTS/FloatBuoy%20Point/Data/Description {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FloatBuoy Point/Data/Description";
    }
    /HDFEOS/POINTS/FloatBuoy%20Point/Data/Measurements {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FloatBuoy Point/Data/Measurements";
    }
    /HDFEOS/POINTS/FloatBuoy%20Point/Linkage/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FloatBuoy Point/Linkage/";
    }
    /HDFEOS/POINTS/FloatBuoy%20Point/Linkage/BCKPOINTER%3a1-%3e0 {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FloatBuoy Point/Linkage/BCKPOINTER:1->0";
    }
    /HDFEOS/POINTS/FloatBuoy%20Point/Linkage/FWDPOINTER%3a0-%3e1 {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/FloatBuoy Point/Linkage/FWDPOINTER:0->1";
    }
    /HDFEOS/POINTS/Simple%20Point/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/Simple Point/";
        UInt32 _LevelWritten 1, 0, 0, 0, 0, 0, 0, 0;
    }
    /HDFEOS/POINTS/Simple%20Point/Data/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/Simple Point/Data/";
    }
    /HDFEOS/POINTS/Simple%20Point/Data/Sensor {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/Simple Point/Data/Sensor";
    }
    /HDFEOS/POINTS/Simple%20Point/Linkage/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/POINTS/Simple Point/Linkage/";
    }
    /HDFEOS%20INFORMATION/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS INFORMATION/";
        String HDFEOSVersion "HDFEOS_5.1.10.1";
    }
    /HDFEOS%20INFORMATION/StructMetadata.0 {
        SwathStructure {
        }
        GridStructure {
        }
        PointStructure {
            POINT_1 {
                String PointName "Simple Point";
                Level {
                    Level_0 {
                        String LevelName "Sensor";
                        PointField_1 {
                            String PointFieldName "Time";
                            Int32 Order 0;
                        }
                        PointField_2 {
                            String PointFieldName "Concentration";
                            Int32 Order 0;
                        }
                        PointField_3 {
                            String PointFieldName "Species";
                            Int32 Order 0;
                        }
                    }
                }
                LevelLink {
                }
            }
            Alias Simple_Point POINT_1;
            POINT_2 {
                String PointName "FixedBuoy Point";
                Level {
                    Level_0 {
                        String LevelName "Desc-Loc";
                        PointField_1 {
                            String PointFieldName "Label";
                            Int32 Order 0;
                        }
                        PointField_2 {
                            String PointFieldName "Longitude";
                            Int32 Order 0;
                        }
                        PointField_3 {
                            String PointFieldName "Latitude";
                            Int32 Order 0;
                        }
                        PointField_4 {
                            String PointFieldName "DeployDate";
                            Int32 Order 0;
                        }
                        PointField_5 {
                            String PointFieldName "ID";
                            Int32 Order 0;
                        }
                    }
                    Level_1 {
                        String LevelName "Observations";
                        PointField_1 {
                            String PointFieldName "Time";
                            Int32 Order 0;
                        }
                        PointField_2 {
                            String PointFieldName "Rainfall";
                            Int32 Order 0;
                        }
                        PointField_3 {
                            String PointFieldName "Temperature";
                            Int32 Order 0;
                        }
                        PointField_4 {
                            String PointFieldName "ID";
                            Int32 Order 0;
                        }
                    }
                }
                LevelLink {
                    LevelLink_1 {
                        String Parent "Desc-Loc";
                        String Child "Observations";
                        String LinkField "ID";
                    }
                }
            }
            Alias FixedBuoy_Point POINT_2;
            POINT_3 {
                String PointName "FloatBuoy Point";
                Level {
                    Level_0 {
                        String LevelName "Description";
                        PointField_1 {
                            String PointFieldName "Label";
                            Int32 Order 0;
                        }
                        PointField_2 {
                            String PointFieldName "DeployDate";
                            Int32 Order 0;
                        }
                        PointField_3 {
                            String PointFieldName "Weight";
                            Int32 Order 0;
                        }
                        PointField_4 {
                            String PointFieldName "ID";
                            Int32 Order 0;
                        }
                    }
                    Level_1 {
                        String LevelName "Measurements";
                        PointField_1 {
                            String PointFieldName "Time";
                            Int32 Order 0;
                        }
                        PointField_2 {
                            String PointFieldName "Longitude";
                            Int32 Order 0;
                        }
                        PointField_3 {
                            String PointFieldName "Latitude";
                            Int32 Order 0;
                        }
                        PointField_4 {
                            String PointFieldName "Rainfall";
                            Int32 Order 0;
                        }
                        PointField_5 {
                            String PointFieldName "Temperature";
                            Int32 Order 0;
                        }
                        PointField_6 {
                            String PointFieldName "ID";
                            Int32 Order 0;
                        }
                    }
                }
                LevelLink {
                    LevelLink_1 {
                        String Parent "Description";
                        String Child "Measurements";
                        String LinkField "ID";
                    }
                }
            }
            Alias FloatBuoy_Point POINT_3;
        }
        ZaStructure {
        }
    }
}
