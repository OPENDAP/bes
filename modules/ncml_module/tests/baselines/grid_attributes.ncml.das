Attributes {
    NC_GLOBAL {
        String title "COADS 1-degree Equatorial Enhanced";
        String history "";
        String Conventions "COARDS";
    }
    DODS_EXTRA {
        String Unlimited_Dimension "time";
    }
    lat {
        String long_name "Latitude";
        String units "degrees_north";
        Float32 actual_range 10.0000000, -10.0000000;
    }
    lon {
        String long_name "Longitude";
        String units "degrees_east";
        Float32 actual_range 0.500000000, 359.500000;
    }
    time {
        String units "days since 1-1-1 00:00:0.0";
        String long_name "Time";
        String delta_t "0000-01-00 00:00:00";
        String avg_period "0000-01-00 00:00:00";
        Float64 actual_range 715511.0000000000, 729360.0000000000;
    }
    cldc {
        cldc {
            Float32 valid_range 0.00000000, 8.00000000;
            Float32 actual_range 0.00000000, 8.00000000;
            String units "okta";
            Int16 precision 1;
            Int16 missing_value 32766;
            Int16 _FillValue 32766;
            String long_name "Cloudiness Monthly Mean at Surface";
            String dataset "COADS 1-degree Equatorial Enhanced\012AI";
            String var_desc "Cloudiness\012C";
            String level_desc "Surface\0120";
            String statistic "Mean\012M";
            String parent_stat "Individual Obs\012I";
            Float32 add_offset 3276.50000;
            Float32 scale_factor 0.100000001;
        }
        time {
            String units "days since 1-1-1 00:00:0.0";
            String long_name "Time";
            String delta_t "0000-01-00 00:00:00";
            String avg_period "0000-01-00 00:00:00";
            Float64 actual_range 715511.0000000000, 729360.0000000000;
        }
        lat {
            String long_name "Latitude";
            String units "degrees_north";
            Float32 actual_range 10.0000000, -10.0000000;
            String Description "I am a new attribute in the Grid map vector named lat!";
        }
        lon {
            String long_name "Longitude";
            String units "degrees_east";
            Float32 actual_range 0.500000000, 359.500000;
            String Description "I am a new attribute in the Grid map vector named lon!";
        }
    }
}
