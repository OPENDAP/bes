Attributes {
    Longitude {
        String units "degrees_east";
        String long_name "Longitude";
    }
    Latitude {
        String units "degrees_north";
        String long_name "Latitude";
    }
    Solution_4_Land {
        String units "level";
        String long_name "Solution_4_Land(fake)";
    }
    QA_Byte_Land {
        String units "level";
        String long_name "QA_Byte_Land(fake)";
    }
    Num_By_Products {
        String units "level";
        String long_name "Num_By_Products(fake)";
    }
    QA_Byte_Ocean {
        String units "level";
        String long_name "QA_Byte_Ocean(fake)";
    }
    MODIS_Band_Land {
        String long_name "Center Wavelengths of MODIS Bands Used in Land Retrieval Algorithms";
        String units "Nanometers";
    }
    MODIS_Band_Ocean {
        String long_name "Center Wavelengths of MODIS Bands Used in Ocean Retrieval Algorithms";
        String units "Nanometers";
    }
    Solution_1_Land {
        String long_name "Central Wavelength of MODIS Bands Used in Continental Model Retrieval";
        String units "Nanometers";
    }
    Solution_2_Land {
        String long_name "Central Wavelength of MODIS Bands Used in Corrected Model Retrieval";
        String units "Nanometers";
    }
    Solution_3_Land {
        String long_name "Central Wavelength of MODIS Bands Used in Scatter Plot Solution";
        String units "Nanometers";
    }
    Solution_Ocean {
        String long_name "Index of Ocean Solution Types  1 - Best 2 - Average";
        String units "None";
    }
    Solution_Index {
        String long_name "Solution Index for Ocean Best Small (1-4) and Large (5-9) Modes";
        String units "None";
    }
    Num_DeepBlue_Wavelengths {
        String long_name "Center Wavelengths of MODIS Bands Used in Deep Blue Algorithm";
        String units "Nanometers";
    }
    Scan_Start_Time {
        String coordinates "Latitude Longitude";
        Float64 valid_range 0.0000000000000000, 3155800000.0000000;
        Float64 _FillValue -999.00000000000000;
        String long_name "TAI Time at Start of Scan replicated across the swath";
        String units "Seconds since 1993-1-1 00:00:00.0 0";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "MODIS Input";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Solar_Zenith {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 18000;
        Int16 _FillValue -9999;
        String long_name "Solar Zenith Angle, Cell to Sun";
        String units "Degrees";
        Float64 scale_factor 0.0099999997764825821;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "MODIS Input";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Solar_Azimuth {
        String coordinates "Latitude Longitude";
        Int16 valid_range -18000, 18000;
        Int16 _FillValue -9999;
        String long_name "Solar_Azimuth Angle, Cell to Sun";
        String units "Degrees";
        Float64 scale_factor 0.0099999997764825821;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "MODIS Input";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Sensor_Zenith {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 18000;
        Int16 _FillValue -9999;
        String long_name "Sensor_Zenith Angle, Cell to Sensor";
        String units "Degrees";
        Float64 scale_factor 0.0099999997764825821;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "MODIS Input";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Sensor_Azimuth {
        String coordinates "Latitude Longitude";
        Int16 valid_range -18000, 18000;
        Int16 _FillValue -9999;
        String long_name "Sensor_Azimuth Angle, Cell to Sun";
        String units "Degrees";
        Float64 scale_factor 0.0099999997764825821;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "MODIS Input";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Cloud_Mask_QA {
        String coordinates "Latitude Longitude";
        Int32 valid_range 0, -1;
        Int32 _FillValue 0;
        String long_name "Cloud Mask info on 10x10 km resolution";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "MODIS Input";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
        String description "
                                                                                   
Cloud_mask_QA flags:                                                                
                                                                                    
                                                                                    
QA Flag Name     Number of    Bit Value   Description                               
                 Bits                                                               
--------------------------------------------------------------                      
Cloud Mask           1            0       Undetermined                              
                                  1       Determined                                
                                                                                    
Cloud Mask           2            0       0-25% Cloudy pixels                       
Quality Flag                      1       25-50% cloudy pixels                      
                                  2       50-75% cloudy pixels                      
                                  3       75-100%cloudy pixels                      
                                                                                    
Day/Night            1            0       Night                                     
flag                              1       Day                                       
                                                                                    
Sun glint            1            0       Yes                                       
flag                              1       No                                        
                                                                                    
Snow/Ice flag        1            0       Yes                                       
                                  1       No                                        
                                                                                    
Land/Water           2            0       Water (ocean)                             
flag                              1       Coastal                                   
                                  2       Desert                                    
                                  3       Land                                      
---------------------- 1 byte total --------------------------                      
                                                                                    
";
    }
    Scattering_Angle {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 18000;
        Int16 _FillValue -9999;
        String long_name "Scattering Angle";
        String units "Degrees";
        Float64 scale_factor 0.0099999997764825821;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Optical_Depth_Land_And_Ocean {
        String coordinates "Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "AOT at 0.55 micron for both ocean (best) and land (corrected) with best quality data (Quality flag=3)";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Image_Optical_Depth_Land_And_Ocean {
        String coordinates "Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "AOT at 0.55 micron for both ocean (best) and land (corrected) with all quality data (Quality flag=1,2,3)";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Optical_Depth_Ratio_Small_Land_And_Ocean {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "Ratio of small mode optical depth at 0.55 micron";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Mean_Reflectance_Land_All {
        String coordinates "Solution_3_Land Latitude Longitude";
        Int16 valid_range 0, 10000;
        Int16 _FillValue -9999;
        String long_name "Average Reflectance at 0.47, 0.66, and 2.1 micron after cloud screening";
        String units "None";
        Float64 scale_factor 9.9999997473787516e-05;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Standard_Deviation_Reflectance_Land_All {
        String coordinates "Solution_3_Land Latitude Longitude";
        Int16 valid_range 0, 10000;
        Int16 _FillValue -9999;
        String long_name "Standard Deviation of Reflectance at 0.47, 0.66, and 2.1 micron after cloud screening";
        String units "None";
        Float64 scale_factor 9.9999997473787516e-05;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Path_Radiance_Land {
        String coordinates "Solution_1_Land Latitude Longitude";
        Int16 valid_range 0, 10000;
        Int16 _FillValue -9999;
        String long_name "Path Radiance at 0.47 and 0.66 micron";
        String units "None";
        Float64 scale_factor 9.9999997473787516e-05;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Error_Path_Radiance_Land {
        String coordinates "Solution_1_Land Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "Uncertainty of Path Radiance at 0.47 and 0.66 micron";
        String units "None";
        Float64 scale_factor 0.0000000000000000;
        Float64 add_offset 9.9999997473787516e-05;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Critical_Reflectance_Land {
        String coordinates "Solution_1_Land Latitude Longitude";
        Int16 valid_range 0, 10000;
        Int16 _FillValue -9999;
        String long_name "Critical_Reflectance at 0.47 and 0.66 micron";
        String units "None";
        Float64 scale_factor 9.9999997473787516e-05;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Error_Critical_Reflectance_Land {
        String coordinates "Solution_1_Land Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "Uncertainty of Critical_Reflectance at 0.47 and 0.66 micron";
        String units "None";
        Float64 scale_factor 9.9999997473787516e-05;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    QualityWeight_Path_Radiance_Land {
        String coordinates "Solution_1_Land Latitude Longitude";
        Int16 valid_range -20, 20;
        Int16 _FillValue -9999;
        String long_name "Quality Flag for Path Radiance based on the Test for fits";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    QualityWeight_Critical_Reflectance_Land {
        String coordinates "Solution_1_Land Latitude Longitude";
        Int16 valid_range -20, 20;
        Int16 _FillValue -9999;
        String long_name "Quality Flag forCritical_Reflectance based on the Test for fits";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Aerosol_Type_Land {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 4;
        Int16 _FillValue -9999;
        String long_name "Aerosol Type";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Fitting_Error_Land {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "Fitting error for Land";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Surface_Reflectance_Land {
        String coordinates "Solution_2_Land Latitude Longitude";
        Int16 valid_range 0, 5000;
        Int16 _FillValue -9999;
        String long_name "Surface Reflectance at 0.47, 0.66, and 2.13 micron";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Corrected_Optical_Depth_Land {
        String coordinates "Solution_3_Land Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "Corrected Optical Thickness at 0.47, 0.55, and 0.66 micron";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Corrected_Optical_Depth_Land_wav2p1 {
        String coordinates "Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "Corrected Optical Thickness at 2.13 micron";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Optical_Depth_Small_Land {
        String coordinates "Solution_4_Land Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "Optical Thickness for small mode at 0.47, 0.55, 0.66 and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Optical_Depth_Ratio_Small_Land {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "Small mode aerosol fraction";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Number_Pixels_Used_Land {
        String coordinates "Solution_1_Land Latitude Longitude";
        Int16 valid_range 1, 400;
        Int16 _FillValue -9999;
        String long_name "Number of good pixels used for retrieval at 470 and 659 nm";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Mean_Reflectance_Land {
        String coordinates "MODIS_Band_Land Latitude Longitude";
        Int16 valid_range 0, 10000;
        Int16 _FillValue -9999;
        String long_name "Mean Reflectance at 0.47, 0.55, 0.66, 0.86, 1.24, 1.64, and 2.13 um";
        String units "None";
        Float64 scale_factor 9.9999997473787516e-05;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    STD_Reflectance_Land {
        String coordinates "MODIS_Band_Land Latitude Longitude";
        Int16 valid_range 0, 20000;
        Int16 _FillValue -9999;
        String long_name "Standard Deviation of Reflectance at 0.47, 0.55, 0.66, 0.86, 1.24, 1.64, and 2.13 um";
        String units "None";
        Float64 scale_factor 9.9999997473787516e-05;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Mass_Concentration_Land {
        String coordinates "Latitude Longitude";
        Float32 valid_range 0.000000000, 1000.000000;
        Float32 _FillValue -999.0000000;
        String long_name "Mass concentration";
        String units "1.0e-6g/cm^2";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Angstrom_Exponent_Land {
        String coordinates "Latitude Longitude";
        Int16 valid_range -1000, 5000;
        Int16 _FillValue -9999;
        String long_name "Angstrom exponent for 0.47 and 0.67 micron";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Cloud_Fraction_Land {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "Cloud Fraction from Land aerosol Cloud Mask from retrieved and overcast pixels not including cirrus mask";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Quality_Assurance_Land {
        String coordinates "Latitude Longitude QA_Byte_Land";
        Int32 valid_range 0, -1;
        Int32 _FillValue 0;
        String long_name "Runtime QA flags";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
        String description "see MODIS atmosphere QA plan for details                                            
";
    }
    Quality_Assurance_Crit_Ref_Land {
        String coordinates "Latitude Longitude QA_Byte_Land";
        Int32 valid_range 0, -1;
        Int32 _FillValue 0;
        String long_name "Runtime QA flags";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Aerosol_Cldmask_Byproducts_Land {
        String coordinates "Num_By_Products Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "3X3 pixels Standard Deviation at 470,1380 nm, Reflectance at 555,1380,2130 nm, Radiance at 11000 nm, Cirrus Fraction";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Solution_Index_Ocean_Small {
        String coordinates "Solution_Ocean Latitude Longitude";
        Int16 valid_range 1, 4;
        Int16 _FillValue -9999;
        String long_name "Solution Number index small particles";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Solution_Index_Ocean_Large {
        String coordinates "Solution_Ocean Latitude Longitude";
        Int16 valid_range 5, 9;
        Int16 _FillValue -9999;
        String long_name "Solution Number index large particles";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Effective_Optical_Depth_Best_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "AOT at 7 bands for best solution for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Effective_Optical_Depth_Average_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "AOT at 7 bands for average solution for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Optical_Depth_Small_Best_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "AOT at 7 bands for small mode of best solution for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Optical_Depth_Small_Average_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "AOT at 7 bands for small mode of average solution for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Optical_Depth_Large_Best_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "AOT at 7 bands for large mode of best solution for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Optical_Depth_Large_Average_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "AOT at 7 bands for large mode of average solution for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Mass_Concentration_Ocean {
        String coordinates "Solution_Ocean Latitude Longitude";
        Float32 valid_range 0.000000000, 1000.000000;
        Float32 _FillValue -999.0000000;
        String long_name "Mass_Concentration for best and average solutions";
        String units "1.0e-6g/cm^2";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Effective_Radius_Ocean {
        String coordinates "Solution_Ocean Latitude Longitude";
        Int16 valid_range 0, 5000;
        Int16 _FillValue -9999;
        String long_name "Effective_Radius at 0.55 micron of both solutions";
        String units "micron";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Cloud_Condensation_Nuclei_Ocean {
        String coordinates "Solution_Ocean Latitude Longitude";
        Float32 valid_range 0.000000000, 9.999999795e+10;
        Float32 _FillValue -999.0000000;
        String long_name "Column number of CCN at 0.55 micron of both solutions";
        String units "CCN/cm^2";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Asymmetry_Factor_Best_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range 0, 3000;
        Int16 _FillValue -9999;
        String long_name "Asymmetry Factor at 7 bands for best solution for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Asymmetry_Factor_Average_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range 0, 3000;
        Int16 _FillValue -9999;
        String long_name "Asymmetry_Factor at 7 bands for average solution for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Backscattering_Ratio_Best_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range 0, 3000;
        Int16 _FillValue -9999;
        String long_name "Backscattering ratio at 7 bands for best solution for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Backscattering_Ratio_Average_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range 0, 3000;
        Int16 _FillValue -9999;
        String long_name "Backscattering ratio at 7 bands for average solution for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Angstrom_Exponent_1_Ocean {
        String coordinates "Solution_Ocean Latitude Longitude";
        Int16 valid_range -1000, 5000;
        Int16 _FillValue -9999;
        String long_name "Angstrom Exponent for 0.55 and 0.86 micron";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Angstrom_Exponent_2_Ocean {
        String coordinates "Solution_Ocean Latitude Longitude";
        Int16 valid_range -1000, 5000;
        Int16 _FillValue -9999;
        String long_name "Angstrom Exponent for 0.865 and 2.13 micron";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Least_Squares_Error_Ocean {
        String coordinates "Solution_Ocean Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "Least square error estimated";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Optical_Depth_Ratio_Small_Ocean_0_55micron {
        String coordinates "Solution_Ocean Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "Ratio of small mode optical depth at 0.55 micron for best (1) and average (2) solutions";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Optical_Depth_by_models_ocean {
        String coordinates "Solution_Index Latitude Longitude";
        Int16 valid_range -100, 5000;
        Int16 _FillValue -9999;
        String long_name "Optical Depth for small and large modes placed in model index";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Cloud_Fraction_Ocean {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "Cloud Fraction from Ocean aerosol Cloud Mask from retrieved  and overcast pixels not including cirrus mask";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Number_Pixels_Used_Ocean {
        String coordinates "Latitude Longitude";
        Int16 valid_range 1, 400;
        Int16 _FillValue -9999;
        String long_name "Number of good pixels used for retrieval at at 865 nm";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Mean_Reflectance_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range 0, 10000;
        Int16 _FillValue -9999;
        String long_name "Mean Reflectance at 7 bands for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 9.9999997473787516e-05;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    STD_Reflectance_Ocean {
        String coordinates "MODIS_Band_Ocean Latitude Longitude";
        Int16 valid_range 0, 20000;
        Int16 _FillValue -9999;
        String long_name "Standard Deviation of Reflectance at 7 bands for 0.47, 0.55, 0.66, 0.86, 1.24, 1.63, and 2.13 um";
        String units "None";
        Float64 scale_factor 9.9999997473787516e-05;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Quality_Assurance_Ocean {
        String coordinates "Latitude Longitude QA_Byte_Ocean";
        Int32 valid_range 0, -1;
        Int32 _FillValue 0;
        String long_name "Run time QA flags";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
        String description "(see MODIS atmosphere QA plan for details)                                          
";
    }
    Aerosol_Cldmask_Byproducts_Ocean {
        String coordinates "Num_By_Products Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "3X3 pixels Standard Deviation at 550, 1380 nm, Reflectance at 550,1380,2130 nm, Radiance at 11000 nm, Cirrus Fraction";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Deep_Blue_Aerosol_Optical_Depth_550_Land {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 5000;
        Int16 _FillValue -9999;
        String long_name "AOT at 0.55 micron for land with all quality data (Quality flag=1,2,3)";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Deep_Blue_Aerosol_Optical_Depth_Land {
        String coordinates "Num_DeepBlue_Wavelengths Latitude Longitude";
        Int16 valid_range 0, 5000;
        Int16 _FillValue -9999;
        String long_name "AOT at 0.412, 0.47, and 0.66 micron for land with all quality data (Quality flag=1,2,3)";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Deep_Blue_Angstrom_Exponent_Land {
        String coordinates "Latitude Longitude";
        Int16 valid_range -500, 5000;
        Int16 _FillValue -9999;
        String long_name "Deep Blue Angstrom Exponent for land (0.412-0.47 micron) with all quality data (Quality flag=1,2,3)";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Deep_Blue_Single_Scattering_Albedo_Land {
        String coordinates "Num_DeepBlue_Wavelengths Latitude Longitude";
        Int16 valid_range 700, 1000;
        Int16 _FillValue -9999;
        String long_name "Deep Blue Single Scattering Albedo at 0.412, 0.47, and 0.66 micron  for land with all quality data (Quality flag=1,2,3)";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Deep_Blue_Surface_Reflectance_Land {
        String coordinates "Num_DeepBlue_Wavelengths Latitude Longitude";
        Int16 valid_range 0, 1000;
        Int16 _FillValue -9999;
        String long_name "Deep Blue Surface Reflectance at 0.412, 0.47, and 0.66 micron  for land with all quality data (Quality flag=1,2,3)";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Deep_Blue_Mean_Reflectance_Land {
        String coordinates "Num_DeepBlue_Wavelengths Latitude Longitude";
        Int16 valid_range 0, 10000;
        Int16 _FillValue -9999;
        String long_name "Average measured TOA Reflectance after cloud screening at 0.412, 0.47, and 0.66 micron for land";
        String units "None";
        Float64 scale_factor 9.9999997473787516e-05;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Deep_Blue_Number_Pixels_Used_Land {
        String coordinates "Num_DeepBlue_Wavelengths Latitude Longitude";
        Int16 valid_range 0, 100;
        Int16 _FillValue -9999;
        String long_name "Number of pixels used for AOT retrieval at 0.412, 0.47, and 0.66 micron for land";
        String units "None";
        Float64 scale_factor 1.0000000000000000;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 1345, 10;
        String Geolocation_Pointer "none";
    }
    Deep_Blue_Aerosol_Optical_Depth_550_Land_STD {
        String coordinates "Latitude Longitude";
        Int16 valid_range 0, 10000;
        Int16 _FillValue -9999;
        String long_name "Standard Deviation of AOT at 0.55 micron for land with all quality data (Quality flag=1,2,3)";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 2025, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    Deep_Blue_Aerosol_Optical_Depth_Land_STD {
        String coordinates "Num_DeepBlue_Wavelengths Latitude Longitude";
        Int16 valid_range 0, 10000;
        Int16 _FillValue -9999;
        String long_name "Standard Deviation of AOT at 0.412, 0.47, and 0.66 micron for land with all quality data (Quality flag=1,2,3)";
        String units "None";
        Float64 scale_factor 0.0010000000474974513;
        Float64 add_offset 0.0000000000000000;
        String Parameter_Type "Output";
        Int32 Cell_Along_Swath_Sampling 5, 2025, 10;
        Int32 Cell_Across_Swath_Sampling 5, 2025, 10;
        String Geolocation_Pointer "Internal geolocation arrays";
    }
    CoreMetadata {
        INVENTORYMETADATA {
            String GROUPTYPE "MASTERGROUP";
            ECSDATAGRANULE {
                LOCALGRANULEID {
                    Int32 NUM_VAL 1;
                    String VALUE "MOD04_L2.A2015021.0020.051.2015021011703.NRT.hdf";
                }
                PRODUCTIONDATETIME {
                    Int32 NUM_VAL 1;
                    String VALUE "2015-01-21T01:17:03.000Z";
                }
                DAYNIGHTFLAG {
                    Int32 NUM_VAL 1;
                    String VALUE "Day";
                }
                REPROCESSINGACTUAL {
                    Int32 NUM_VAL 1;
                    String VALUE "Near Real Time";
                }
                LOCALVERSIONID {
                    Int32 NUM_VAL 1;
                    String VALUE "051";
                }
                REPROCESSINGPLANNED {
                    Int32 NUM_VAL 1;
                    String VALUE "further update is anticipated";
                }
            }
            MEASUREDPARAMETER {
                MEASUREDPARAMETERCONTAINER {
                    String CLASS "1", "2";
                    QAFLAGS {
                        String CLASS "1", "2";
                        SCIENCEQUALITYFLAG {
                            Int32 NUM_VAL 1, 1;
                            String VALUE "Not Investigated", "Not Investigated";
                            String CLASS "1", "2";
                        }
                        AUTOMATICQUALITYFLAGEXPLANATION {
                            Int32 NUM_VAL 1, 1;
                            String CLASS "1", "2";
                            String VALUE "FurtherInvestigationNeeded", "FurtherInvestigationNeeded";
                        }
                        AUTOMATICQUALITYFLAG {
                            Int32 NUM_VAL 1, 1;
                            String CLASS "1", "2";
                            String VALUE "Suspect", "Suspect";
                        }
                        SCIENCEQUALITYFLAGEXPLANATION {
                            Int32 NUM_VAL 1, 1;
                            String VALUE "See http://modis-atmos.gsfc.nasa.gov/validation.html for more details on MODIS Atmosphere data quality.", "See http://modis-atmos.gsfc.nasa.gov/validation.html for more details on MODIS Atmosphere data quality.";
                            String CLASS "1", "2";
                        }
                    }
                    QASTATS {
                        String CLASS "1", "2";
                        QAPERCENTMISSINGDATA {
                            Int32 NUM_VAL 1, 1;
                            String CLASS "1", "2";
                            Int32 VALUE 100, 82;
                        }
                    }
                    PARAMETERNAME {
                        String CLASS "1", "2";
                        Int32 NUM_VAL 1, 1;
                        String VALUE "Corrected_Optical_Depth_Land", "Effective_Optical_Depth_Average_Ocean";
                    }
                }
            }
            ORBITCALCULATEDSPATIALDOMAIN {
                ORBITCALCULATEDSPATIALDOMAINCONTAINER {
                    String CLASS "1";
                    EQUATORCROSSINGDATE {
                        String CLASS "1";
                        Int32 NUM_VAL 1;
                        String VALUE "2015-01-21";
                    }
                    EQUATORCROSSINGTIME {
                        String CLASS "1";
                        Int32 NUM_VAL 1;
                        String VALUE "00:35:37.220788";
                    }
                    ORBITNUMBER {
                        String CLASS "1";
                        Int32 NUM_VAL 1;
                        Int32 VALUE 80278;
                    }
                    EQUATORCROSSINGLONGITUDE {
                        String CLASS "1";
                        Int32 NUM_VAL 1;
                        Float64 VALUE 148.643162037209;
                    }
                }
            }
            COLLECTIONDESCRIPTIONCLASS {
                VERSIONID {
                    Int32 NUM_VAL 1;
                    Int32 VALUE 51;
                }
                SHORTNAME {
                    Int32 NUM_VAL 1;
                    String VALUE "MOD04_L2";
                }
            }
            INPUTGRANULE {
                INPUTPOINTER {
                    Int32 NUM_VAL 30;
                    String VALUE "MOD03.A2015021.0020.005.2015021010406.NRT.hdf", "MOD02QKM.A2015021.0020.005.2015021010727.NRT.hdf", "MOD02HKM.A2015021.0020.005.2015021010727.NRT.hdf", "MOD021KM.A2015021.0020.005.2015021010727.NRT.hdf", "MOD35_L2.A2015021.0020.005.2015021011147.NRT.hdf", "
          small_v2c1.dat.v3", "small_v1c1.dat.v2", "small_v3c1.dat.v2", "big_v2c1.dat.v5", "big_v1c1.dat.v2", "big_v3c1.dat.v2", "lookup_land_w0466.v1", "lookup_land_w0553.v1", "lookup_land_w0644.v1", "lookup_land_w2119.v1", "aerosol_land_map.v2", "gdas1.PGrbF00.150120.18z", "
          TOAST_150119.GRB";
                }
            }
            SPATIALDOMAINCONTAINER {
                HORIZONTALSPATIALDOMAINCONTAINER {
                    BOUNDINGRECTANGLE {
                        EASTBOUNDINGCOORDINATE {
                            Int32 NUM_VAL 1;
                            Float64 VALUE -178.300341777391;
                        }
                        WESTBOUNDINGCOORDINATE {
                            Int32 NUM_VAL 1;
                            Float64 VALUE 144.407889869471;
                        }
                        SOUTHBOUNDINGCOORDINATE {
                            Int32 NUM_VAL 1;
                            Float64 VALUE 35.8348560988731;
                        }
                        NORTHBOUNDINGCOORDINATE {
                            Int32 NUM_VAL 1;
                            Float64 VALUE 57.5981144436849;
                        }
                    }
                }
            }
            RANGEDATETIME {
                RANGEENDINGDATE {
                    Int32 NUM_VAL 1;
                    String VALUE "2015-01-21";
                }
                RANGEENDINGTIME {
                    Int32 NUM_VAL 1;
                    String VALUE "00:25:00.000000";
                }
                RANGEBEGINNINGDATE {
                    Int32 NUM_VAL 1;
                    String VALUE "2015-01-21";
                }
                RANGEBEGINNINGTIME {
                    Int32 NUM_VAL 1;
                    String VALUE "00:20:00.000000";
                }
            }
            PGEVERSIONCLASS {
                PGEVERSION {
                    Int32 NUM_VAL 1;
                    String VALUE "PGE04:51.0.11";
                }
            }
            ASSOCIATEDPLATFORMINSTRUMENTSENSOR {
                ASSOCIATEDPLATFORMINSTRUMENTSENSORCONTAINER {
                    String CLASS "1";
                    ASSOCIATEDSENSORSHORTNAME {
                        String CLASS "1";
                        Int32 NUM_VAL 1;
                        String VALUE "MODIS";
                    }
                    ASSOCIATEDPLATFORMSHORTNAME {
                        String CLASS "1";
                        Int32 NUM_VAL 1;
                        String VALUE "Terra";
                    }
                    ASSOCIATEDINSTRUMENTSHORTNAME {
                        String CLASS "1";
                        Int32 NUM_VAL 1;
                        String VALUE "MODIS";
                    }
                }
            }
            ADDITIONALATTRIBUTES {
                ADDITIONALATTRIBUTESCONTAINER {
                    String CLASS "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15";
                    ADDITIONALATTRIBUTENAME {
                        String CLASS "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15";
                        Int32 NUM_VAL 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1;
                        String VALUE "SuccessfulRetrievalPct_Land", "SuccessfulRetrievalPct_Ocean", "LowConfidentClearPct", "DayProcessedPct", "NightProcessedPct", "SunglintProcessedPct", "Snow_IceSurfaceProcessedPct", "LandProcessedPct", "WaterProcessedPct", "ShadowFoundPct", "ThinCirrusSolarFoundPct", "ThinCirrusIR_FoundPct", "NonCloudObstructionFoundPct", "MaxSolarZenithAngle", "MinSolarZenithAngle";
                    }
                    INFORMATIONCONTENT {
                        String CLASS "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15";
                        PARAMETERVALUE {
                            Int32 NUM_VAL 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1;
                            String CLASS "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15";
                            String VALUE "    0.00", "   18.02", "   85.12", "  100.00", "    0.00", "    0.00", "    3.47", "    5.53", "   94.47", "    0.00", "   35.91", "   10.47", "    0.46", "   82.08", "   56.14";
                        }
                    }
                }
            }
        }
    }
    ArchiveMetadata {
        ARCHIVEDMETADATA {
            String GROUPTYPE "MASTERGROUP";
            PROCESSINGENVIRONMENT {
                Int32 NUM_VAL 1;
                String VALUE "Linux nrt1min107 2.6.18-308.20.1.el5PAE #1 SMP Tue Nov 13 11:03:25 EST 2012 i686 i686 i386 GNU/Linux";
            }
            ALGORITHMPACKAGE {
                ALGORITHMPACKAGEACCEPTANCEDATE {
                    Int32 NUM_VAL 1;
                    String VALUE "June 1997";
                }
                ALGORITHMPACKAGEMATURITYCODE {
                    Int32 NUM_VAL 1;
                    String VALUE "at-launch";
                }
                ALGORITHMPACKAGENAME {
                    Int32 NUM_VAL 1;
                    String VALUE "ATBD-MOD-02";
                }
                ALGORITHMPACKAGEVERSION {
                    Int32 NUM_VAL 1;
                    String VALUE "2";
                }
                LOCALINPUTGRANULEID {
                    Int32 NUM_VAL 10;
                    String VALUE "MOD03.A2015021.0020.005.2015021010406.NRT.hdf", "MOD02QKM.A2015021.0020.005.2015021010727.NRT.hdf", "MOD02HKM.A2015021.0020.005.2015021010727.NRT.hdf", "MOD021KM.A2015021.0020.005.2015021010727.NRT.hdf", "MOD35_L2.A2015021.0020.005.2015021011147.NRT.hdf";
                }
                INSTRUMENTNAME {
                    Int32 NUM_VAL 1;
                    String VALUE "Moderate Resolution Imaging Spectroradiometer";
                }
            }
            GPOLYGON {
                GPOLYGONCONTAINER {
                    String CLASS "1";
                    GRING {
                        String CLASS "1";
                        EXCLUSIONGRINGFLAG {
                            Int32 NUM_VAL 1;
                            String CLASS "1";
                            String VALUE "N";
                        }
                    }
                    GRINGPOINT {
                        String CLASS "1";
                        GRINGPOINTLONGITUDE {
                            Int32 NUM_VAL 4;
                            String CLASS "1";
                            Float64 VALUE 145.254353621684, -178.298647878, 170.96401007598, 144.396422633589;
                        }
                        GRINGPOINTLATITUDE {
                            Int32 NUM_VAL 4;
                            String CLASS "1";
                            Float64 VALUE 57.6028620096364, 52.38984590252, 35.7198608754103, 39.304225709955;
                        }
                        GRINGPOINTSEQUENCENO {
                            Int32 NUM_VAL 4;
                            String CLASS "1";
                            Float64 VALUE 1, 2, 3, 4;
                        }
                    }
                }
            }
            VERYGOODQUALITYDATAPCT_LAND {
                Int32 NUM_VAL 1;
                String VALUE "    0.00";
            }
            GOODQUALITYDATAPCT_LAND {
                Int32 NUM_VAL 1;
                String VALUE "  100.00";
            }
            MARGINALQUALITYDATAPCT_LAND {
                Int32 NUM_VAL 1;
                String VALUE "    0.00";
            }
            BADQUALITYDATAPCT_LAND {
                Int32 NUM_VAL 1;
                String VALUE "    0.00";
            }
            VERYGOODQUALITYDATAPCT_OCEAN {
                Int32 NUM_VAL 1;
                String VALUE "   64.65";
            }
            GOODQUALITYDATAPCT_OCEAN {
                Int32 NUM_VAL 1;
                String VALUE "    3.80";
            }
            MARGINALQUALITYDATAPCT_OCEAN {
                Int32 NUM_VAL 1;
                String VALUE "   15.96";
            }
            BADQUALITYDATAPCT_OCEAN {
                Int32 NUM_VAL 1;
                String VALUE "   17.00";
            }
            ALGORITHMSOFTWAREVERSIONLAND {
                Int32 NUM_VAL 1;
                String VALUE "2";
            }
            ALGORITHMSOFTWAREVERSIONOCEAN {
                Int32 NUM_VAL 1;
                String VALUE "2";
            }
            DESCRREVISION {
                Int32 NUM_VAL 1;
                String VALUE "5.0";
            }
            PRODUCTIONHISTORY {
                Int32 NUM_VAL 1;
                String VALUE "PGE04:51.0.11";
            }
            LONGNAME {
                Int32 NUM_VAL 1;
                String VALUE "MODIS/Terra Aerosol 5-Min L2 Swath 10km";
            }
        }
    }
    HDF_GLOBAL {
        String HDFEOSVersion "HDFEOS_V2.9";
        Int32 Number_of_Instrument_Scans 203;
        Int32 Maximum_Number_of_1km_Frames 135;
        String title "
 MODIS HDF File Specification MOD04_L2: MODIS Level 2 Aerosol 
 Land and Ocean Product                                       
";
        String Slope_and_Offset_Usage "
 The local SDS scale_factor and add_offset attributes are used for the      
 conversion of stored integer data to geophysical floating point numbers.   
 The implementation follows conventional HDF usage (See HDF Users Guide):   
                                                                            
       float value = scale_factor*(stored integer - add_offset)             
                                                                            
 The unit of the derived floating point value is indicated in the 'units' 
 local attribute which is also provided.                                    
";
    }
}
