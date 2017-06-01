Attributes {
    NC_GLOBAL {
        String projection_type "mapped";
        String projection "geographic";
        Int32 gctp_sys 0;
        Int32 gctp_zone 0;
        Float64 gctp_parm 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000000000000000;
        Int32 gctp_datum 12;
        Float32 Southernmost_Northing 37.2686996;
        Float32 Northernmost_Northing 38.0247002;
        Float32 Westernmost_Easting 236.580002;
        Float32 Easternmost_Easting 237.479996;
        Float64 et_affine 0.000000000000000, 0.02700000000000001, 0.03600000000000023, 0.000000000000000, 236.5800000000000, 37.26870000000000;
        Int32 rows 29;
        Int32 cols 26;
        Float64 polygon_latitude 37.26870000000000, 38.02470000000000, 38.02470000000000, 37.26870000000000, 37.26870000000000;
        Float64 polygon_longitude 236.5800000000000, 236.5800000000000, 237.4800000000000, 237.4800000000000, 236.5800000000000;
        String time_coverage_end "2006-06-07T12:00:00Z";
    }
    time {
        String long_name "End Time";
        String standard_name "time";
        String units "seconds since 1970-01-01T00:00:00Z";
        Float64 actual_range 1149681600.000000, 1149681600.000000;
    }
    altitude {
        String long_name "Altitude";
        String standard_name "altitude";
        String units "m";
        Float32 actual_range 0.00000000, 0.00000000;
    }
    lat {
        String long_name "Latitude";
        String standard_name "latitude";
        String units "degrees_north";
        String point_spacing "even";
        Float32 actual_range 37.2686996, 38.0247002;
        String coordsys "geographic";
        String Metadata "This is TEST metadata!";
    }
    lon {
        String long_name "Longitude";
        String standard_name "longitude";
        String units "degrees_east";
        String point_spacing "even";
        Float32 actual_range 236.580002, 237.479996;
        String coordsys "geographic";
    }
    CGusfc {
        CGusfc {
            Float32 _FillValue -1.00000003e+32;
            Float32 missing_value -1.00000003e+32;
            Int32 numberOfObservations 303;
            Float32 actual_range -0.287640005, 0.276320010;
        }
        time {
            String long_name "End Time";
            String standard_name "time";
            String units "seconds since 1970-01-01T00:00:00Z";
            Float64 actual_range 1149681600.000000, 1149681600.000000;
        }
        altitude {
            String long_name "Altitude";
            String standard_name "altitude";
            String units "m";
            Float32 actual_range 0.00000000, 0.00000000;
        }
        lat {
            String long_name "Latitude";
            String standard_name "latitude";
            String units "degrees_north";
            String point_spacing "even";
            Float32 actual_range 37.2686996, 38.0247002;
            String coordsys "geographic";
        }
        lon {
            String long_name "Longitude";
            String standard_name "longitude";
            String units "degrees_east";
            String point_spacing "even";
            Float32 actual_range 236.580002, 237.479996;
            String coordsys "geographic";
        }
    }
}
