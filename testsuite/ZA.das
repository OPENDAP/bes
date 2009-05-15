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
        HDFEOS {
            ADDITIONAL {
                FILE_ATTRIBUTES {
                }
            }
            ZAS {
                ZA1 {
                    Data%20Fields {
                        String Dataset "Count", "Density", "Presure", "Spectra", "Temperature", "Test_string";
                    }
                }
            }
        }
        HDFEOS%20INFORMATION {
            String Dataset "StructMetadata.0";
        }
    }
    /HDFEOS/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/";
    }
    /HDFEOS/ADDITIONAL/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ADDITIONAL/";
    }
    /HDFEOS/ADDITIONAL/FILE_ATTRIBUTES/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES/";
    }
    /HDFEOS/ZAS/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ZAS/";
    }
    /HDFEOS/ZAS/ZA1/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ZAS/ZA1/";
        Int32 GlobalAttribute 1, 2, 3, 4;
        String GLOBAL_CHAR_ATTR "ABCDEF";
        Int32 GLOBAL_LONG_ATTR 1111111, 2222222, 3333333, 4444444;
        Float64 GLOBAL_DOUBLE_ATTR 1.111111, 2.2222219999999999, 3.3333330000000001, 4.4444439999999998;
    }
    /HDFEOS/ZAS/ZA1/Data%20Fields/ {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ZAS/ZA1/Data Fields/";
        Int32 GroupAttribute 10, 20, 30, 40;
    }
    /HDFEOS/ZAS/ZA1/Data%20Fields/Count {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ZAS/ZA1/Data Fields/Count";
        Int32 _FillValue 0;
    }
    /HDFEOS/ZAS/ZA1/Data%20Fields/Density {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ZAS/ZA1/Data Fields/Density";
        Float32 _FillValue 0.;
        Int32 LocalAttribute 100, 200, 300, 400;
    }
    /HDFEOS/ZAS/ZA1/Data%20Fields/Presure {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ZAS/ZA1/Data Fields/Presure";
        Float64 _FillValue 0.;
    }
    /HDFEOS/ZAS/ZA1/Data%20Fields/Spectra {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ZAS/ZA1/Data Fields/Spectra";
        Float32 _FillValue 0.;
    }
    /HDFEOS/ZAS/ZA1/Data%20Fields/Temperature {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ZAS/ZA1/Data Fields/Temperature";
        Float32 _FillValue 0.;
    }
    /HDFEOS/ZAS/ZA1/Data%20Fields/Test_string {
        String HDF5_OBJ_FULLPATH "/HDFEOS/ZAS/ZA1/Data Fields/Test_string";
        Int32 ARRAYOFSTRINGS 57;
        Int32 StringLengthAttribute 72;
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
        }
        ZaStructure {
            ZA_1 {
                String ZaName "ZA1";
                Dimension {
                    Dimension_1 {
                        String DimensionName "MyTrack1";
                        Int32 Size 20;
                    }
                    Dimension_2 {
                        String DimensionName "MyTrack2";
                        Int32 Size 10;
                    }
                    Dimension_3 {
                        String DimensionName "Res2tr";
                        Int32 Size 40;
                    }
                    Dimension_4 {
                        String DimensionName "Res2xtr";
                        Int32 Size 20;
                    }
                    Dimension_5 {
                        String DimensionName "Bands";
                        Int32 Size 15;
                    }
                    Dimension_6 {
                        String DimensionName "IndxTrack";
                        Int32 Size 12;
                    }
                    Dimension_7 {
                        String DimensionName "ExtDim";
                        Int32 Size 60;
                    }
                    Dimension_8 {
                        String DimensionName "Unlim";
                        Int32 Size -1;
                    }
                }
                DimensionMap {
                }
                IndexDimensionMap {
                }
                DataField {
                    DataField_1 {
                        String DataFieldName "Density";
                        String DimList "MyTrack1";
                        String MaxdimList "MyTrack1";
                    }
                    DataField_2 {
                        String DataFieldName "Temperature";
                        String DimList "MyTrack1", "MyTrack2";
                        String MaxdimList "MyTrack1", "MyTrack2";
                    }
                    DataField_3 {
                        String DataFieldName "Presure";
                        String DimList "Res2tr", "Res2xtr";
                        String MaxdimList "Res2tr", "Res2xtr";
                    }
                    DataField_4 {
                        String DataFieldName "Test_string";
                        String DimList "MyTrack2";
                        String MaxdimList "MyTrack2";
                    }
                    DataField_5 {
                        String DataFieldName "Spectra";
                        String DimList "Bands", "Res2tr", "Res2xtr";
                        String MaxdimList "Bands", "Res2tr", "Res2xtr";
                    }
                    DataField_6 {
                        String DataFieldName "Count";
                        String DimList "MyTrack1";
                        String MaxdimList "Unlim";
                    }
                }
            }
        }
    }
}
