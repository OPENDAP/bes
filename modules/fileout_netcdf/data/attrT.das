Attributes {
    String title "Attribute Test DDS";
    String contact "Patrick West";
    String contact_email "opendap-tech@opendap.org";
    Byte byteattr 65, 66, 67, 68, 69, 70, 71, 72, 73, 74;
    Int16 int16attr -50, -40, -30, -20, -10, 0, 10, 20, 30, 40, 50;
    UInt16 ui16attr 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000;
    Int32 int32attr -100000, -80000, -60000, -40000, -20000, 0, 20000, 40000, 60000, 80000, 100000;
    UInt32 uint32attr 0, 100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000, 1000000;
    Float32 floatattr 0.15, 5.81, 11.47, 17.13, 22.79, 28.45, 34.11, 39.77, 45.43;
    Float64 doubleattr 100.155, 976.423, 1852.69, 2728.96, 3605.23, 4481.49, 5357.76, 6234.03, 7110.3, 7986.57, 8862.83, 9739.1;
    Byte singlebyteattr 77;
    String singlestringattr "String Value";
    container {
        String containerattr "Container Attribute Value";
        subcontainer {
            String subcontainerattr "Sub Container Attribute Value";
        }
    }
    String mystructattr1 "mystructattr1val";
    String mystructattr2 "mystructattr2val";
    String mystructattr3 "mystructattr3val";
}
