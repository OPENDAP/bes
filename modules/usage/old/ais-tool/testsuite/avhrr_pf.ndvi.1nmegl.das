# -*- Perl -*-

Attributes {
    NC_GLOBAL {
        String Conventions "COARDS"; # This might be a lie...
	# The following three are not part of COARDS
	String title "NDVI Data from AVHRR Land Pathfinder";
	Url location
    "http://daac.gsfc.nasa.gov/daac-bin/nph-ff/DODS/inter_disc/biosphere/avhrr_ndvi/";
        String documentation "http://daac.gsfc.nasa.gov/CAMPAIGN_DOCS/FTP_SITE/INT_DIS/readmes/pal.html";
    }
    ndvi {
        String long_name "Normalized Difference Vegetation Index";
	# Float64 scale_factor ; # Not used
	# Float64 add_offset ; # Not used
	Float64 _FillValue -99.999;
	Float64 missing_value -9.999;
	Float64 valid_min -0.200; # COARDS?
	Float64 valid_max 0.730;  # COARDS?
	# String units ; # Since ndvi is dimensionless, leave out 'units.'
    }
}
