/////////////////////////////////////////////////////////////////////////////
// Helper functions for handling dimension maps,clear memories and separating namelist
//
//  Authors:   Kent Yang <myang6@hdfgroup.org> Eunsoo Seo
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifndef HDFCFUTIL_H
#define HDFCFUTIL_H

#include <stdlib.h>
#include <sys/file.h>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <assert.h>
#include <iomanip>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>
#include <cerrno>

#include "mfhdf.h"
#include "hdf.h"

#ifdef USE_HDFEOS2_LIB
#include "HdfEosDef.h"
#include "HDFEOS2.h"
#endif
#include "HDFSP.h"

#include <TheBESKeys.h>
#include <BESUtil.h>
#include <libdap/DDS.h>
#include <libdap/DAS.h>
#include <libdap/InternalErr.h>
#include <BESInternalError.h>

#include <libdap/escaping.h> // for escattr

// This is the maximum number of MODIS special values.
const int MAX_NON_SCALE_SPECIAL_VALUE=65535;

// This is the minimum number of MODIS special values.
const int MIN_NON_SCALE_SPECIAL_VALUE=65500;



// This is used to retrieve dimension map info. when retrieving the data values of HDF-EOS swath that 
// has swath dimension maps.
struct dimmap_entry
{
    // geo dimension name
    std::string geodim;

    // data dimension name
    std::string datadim;
    
    // offset and increment of a dimension map
    int32 offset;
    int32 inc;
};


struct HDFCFUtil
{

    /// Close HDF4 and HDF-EOS2 file IDs. For performance reasons, we want to keep HDF-EOS2/HDF4 IDs open
    /// for one operation(DDS,DAS or DATA). In case of exceptions, these IDs need to be closed.
    static void close_fileid(int32 sdfd,int32 file_id,int32 gridfd,int32 swathfd,bool pass_fileid_key);

    static std::string escattr_fvalue(std::string  s);

    /// From a string separated by a separator to a list of string,
    /// for example, split "ab,c" to {"ab","c"}
    static void Split (const char *s, int len, char sep,
        std::vector < std::string > &names);

    /// Assume sz is Null terminated string.
    static void Split (const char *sz, char sep,
                            std::vector < std::string > &names);

    /// This is a safer way to insert and update a c++ map value.
    /// Otherwise, the local testsuite at The HDF Group will fail for HDF-EOS2 data
    ///  under iMac machine platform.
    static bool insert_map(std::map<std::string,std::string>& m, std::string key, std::string val);

    /// Change special characters to "_"
    static std::string get_CF_string(std::string s);

    /// Obtain the unique name for the clashed names and save it to set namelist.
    static void gen_unique_name(std::string &str,std::set<std::string>& namelist, int&clash_index);

    /// General routines to handle name clashings
    static void Handle_NameClashing(std::vector<std::string>&newobjnamelist);
    static void Handle_NameClashing(std::vector<std::string>&newobjnamelist,std::set<std::string>&objnameset);

    /// Print attribute values in string
    static std::string print_attr(int32, int, void*);

    /// Print datatype in string
    static std::string print_type(int32);

    /// Obtain datatype size
    static short obtain_type_size(int32);


    // Subsetting the 2-D fields
    template <typename T> static void LatLon2DSubset (T* outlatlon, int ydim, int xdim, T* latlon, const int32 * offset, const int32 * count, const int32 * step);

    /// CF requires the _FillValue attribute datatype is the same as the corresponding field datatype. For some NASA files, this is not true.
    /// So we need to check if the _FillValue's datatype is the same as the attribute's. If not, we need to correct them.
    static void correct_fvalue_type(libdap::AttrTable *at,int32 dtype);


    /// CF requires the scale_factor and add_offset attribute datatypes hold the same  datatype.
    /// So far we haven't found that scale_factor and add_offset attributes hold different datatypes in NASA files.
    /// But just in case, we implement a BES key to give users a chance to check this. By default, the key is always off. 
    static void correct_scale_offset_type(libdap::AttrTable *at);


#ifdef USE_HDFEOS2_LIB

    /// The following routines change_data_type,is_special_value, check_geofile_dimmap, is_modis_dimmap_nonll_field, obtain_dimmap_info,
    /// handle_modis_special_attrs,handle_modis_vip_special_attrs are all routines for handling MODIS data products. 

    /// Check if we need to change the datatype for MODIS fields. The datatype needs to be changed
    /// mainly because of non-CF scale and offset rules. To avoid violating CF conventions, we apply
    /// the non-CF MODIS scale and offset rule to MODIS data. So the final data type may be different
    /// than the original one due to this operation. For example, the original datatype may be int16.
    /// After applying the scale/offset rule, the datatype may become float32.
    /// Currently when disabling scale and offset key is true,(H4.DisableScaleOffsetComp=true),
    /// this function should only apply to MODIS level 1B,VIP CMG Grid and some snow and ice products
    /// that use the Key attribute. KY 2014-02-12
    static bool change_data_type(libdap::DAS & das, SOType scaletype, const std::string & new_field_name);

    /// For MODIS (confirmed by level 1B) products, values between 65500(MIN_NON_SCALE_SPECIAL_VALUE)  
    /// and 65535(MAX_NON_SCALE_SPECIAL_VALUE) are treated as
    /// special values. These values represent non-physical data values caused by various failures.
    /// For example, 65533 represents "when Detector is saturated".
    static bool is_special_value(int32 dtype,float fillvalue, float realvalue);

    /// Check if the MODIS file has dimension map and return the number of dimension maps
    static int check_geofile_dimmap(const std::string & geofilename);

    /// This is for the case that the separate MODIS geo-location file is used.
    /// Some geolocation names at the MODIS data file are not consistent with
    /// the names in the MODIS geo-location file. So need to correct them.
    static bool is_modis_dimmap_nonll_field(std::string & fieldname);

    /// Obtain the MODIS swath dimension map info.
    static void obtain_dimmap_info(const std::string& filename, HDFEOS2::Dataset*dataset,std::vector<struct dimmap_entry>& dimmaps, std::string & modis_geofilename,bool &geofile_nas_dimmap);

    ///Helper function to provide new scale and offset attributes given the values and types. The output types will only be float and double. 
    //Scale and offset values with other types will be converted to float and their input values must also be in float.
    //If the add_offset_found is false, the add_offset attribute will not be added. add_offset information will not be used.
    static void add_scale_offset_attrs(libdap::AttrTable* at, const std::string& s_type, float svalue_f, double svalue_d, bool add_offset_found, 
                                       const std::string& o_type, float ovalue_f, double ovalue_d);

    static void add_scale_str_offset_attrs(libdap::AttrTable* at, const std::string& s_type, const std::string& s_value_str, bool add_offset_found, 
                                       const std::string& o_type, float ovalue_f, double ovalue_d);



    /// Temp adding the handling of MODIS special attribute routines when disabling the scale computation
    static void handle_modis_special_attrs_disable_scale_comp(libdap::AttrTable *at,const string &filename, bool is_grid, const std::string &newfname, SOType scaletype);

    /// These routines will handle scale_factor,add_offset,valid_min,valid_max and other attributes such as Number_Type to make sure the CF is followed.
    /// For example, For the case that the scale and offset rule doesn't follow CF, the scale_factor and add_offset attributes are renamed
    /// to orig_scale_factor and orig_add_offset to keep the original field info.
    static void handle_modis_special_attrs(libdap::AttrTable *at,const std::string &filename, bool is_grid, const std::string & newfname, SOType scaletype, bool gridname_change_valid_range, bool changedtype, bool &change_fvtype);

    /// This routine makes the MeaSUREs VIP attributes follow CF.
    static void handle_modis_vip_special_attrs(const std::string& valid_range_value,const std::string& scale_factor_value, float& valid_min, float & valid_max);

    /// Make AMSR-E attributes follow CF.
    static void handle_amsr_attrs(libdap::AttrTable *at);

    // The following four functions is to handle 1-D CF CV variables required by CF projection conventions for grid. 
    // Obtains the latitude and longitude dimension info. of an HDF-EOS2 grid.
    static void obtain_grid_latlon_dim_info(const HDFEOS2::GridDataset*,string &,int32 &,string &,int32 &);

    // Adds the 1-D cf grid projection mapping attribute to data variables
    // It is called by the function add_cf_grid_attrs. 
    static void add_cf_grid_mapping_attr(libdap::DAS &das, const HDFEOS2::GridDataset*gdset,const string& cf_projection,
                                         const string & dim0name,int32 dim0size,const string &dim1name,int32 dim1size);

    // This function adds the 1-D horizontal coordinate variables as well as the dummy projection variable to the grid.
    static void add_cf_grid_cvs(libdap::DDS & dds, const HDFEOS2::GridDataset *gdset);

    //Adds 1D grid mapping CF attributes to CV and data variables.
    static void add_cf_grid_cv_attrs(libdap::DAS &das, const HDFEOS2::GridDataset *gdset);

#endif 

    // Check OBPG attributes. Specifically, check if slope and intercept can be obtained from the file level. 
    // If having global slope and intercept,  obtain OBPG scaling, slope and intercept values.
    static void check_obpg_global_attrs(HDFSP::File*f,std::string & scaling, float & slope,bool &global_slope_flag,float & intercept, bool & global_intercept_flag);

    // For some OBPG files that only provide slope and intercept at the file level, 
    // global slope and intercept are needed to add to all fields and their names are needed to be changed to scale_factor and add_offset.
    // For OBPG files that provide slope and intercept at the field level,  slope and intercept are needed to rename to scale_factor and add_offset.
    static void add_obpg_special_attrs(const HDFSP::File*f,libdap::DAS &das,  const HDFSP::SDField* spsds,const std::string & scaling, float&slope,const bool &global_slope_flag,float& intercept,const bool &global_intercept_flag);

    // Handle HDF4 OTHERHDF products that follow SDS dimension scale model. 
    // The special handling of AVHRR data is also included.
    static void handle_otherhdf_special_attrs(const HDFSP::File *f, libdap::DAS &das);

    // Add  missing CF attributes for non-CV varibles
    static void add_missing_cf_attrs(const HDFSP::File*f,libdap::DAS &das);

    // Handle Merra and CERES attributes with the BES key EnableCERESMERRAShortName.
    static void handle_merra_ceres_attrs_with_bes_keys(const HDFSP::File*f, libdap::DAS &das,const std::string& filename);

    // Handle the attributes with the BES key EnableVdataDescAttr.
    static void handle_vdata_attrs_with_desc_key(const HDFSP::File*f,libdap::DAS &das);

    // Map AIRS version 6 or MOD08 HDF-EOS object attributes to DAP2, here we simply call Hopen and just retrieve the vgroup info. for the performance reason.
    static void map_eos2_objects_attrs(libdap::DAS &das,const string &filename);
    static void map_eos2_one_object_attrs_wrapper(libdap::DAS &das,int32 file_id,int32 vgroup_id, const string &vgroup_name, bool is_grid);
    static void map_eos2_one_object_attrs(libdap::DAS &das,int32 file_id,int32 obj_attr_group_id, const string &vgroup_name);

    // Parse TRMM V7 GridHeaders
    static void parser_trmm_v7_gridheader(const std:: vector<char>&value, int& latsize, int&lonsize, float& lat_start, float& lon_start, float& lat_res, float& lon_res, bool check_reg_orig);

    // Use to generate cache file name.
    // Reverse the char array order
    static void rev_str(char *str, int len);

    // Return the index of the char array for the integer part 
    static int int_to_str(int,char str[],int);

    // Convert a double number to char array
    static void dtoa(double,char *,int);

    // Obtain the double number in the string format
    static std::string get_double_str(double,int,int);

    // Obtain the integer in the string format
    static std::string get_int_str(int);
     
   
    // Read double-type data from a file to a vector of double type
    static ssize_t read_vector_from_file(int fd,vector<double> &,size_t);
     
    // wrap function of Unix read to a buffer. Memory for the buffer should be allocated.
    static ssize_t read_buffer_from_file(int fd,void*buf,size_t);
    static std::string obtain_cache_fname(const std::string & fprefix, const std::string & fname, const std::string &vname); 
    static size_t obtain_dds_cache_size(const HDFSP::File*);
    static void write_sp_sds_dds_cache(const HDFSP::File*,FILE*,size_t,const std::string & fname);
    static void read_sp_sds_dds_cache(FILE*,libdap::DDS * dds_ptr,const std::string &filename, const std::string &hdf_filename);
     
};

/// This inline routine will translate N dimensions into 1 dimension.
inline int32
INDEX_nD_TO_1D (const std::vector < int32 > &dims,
                                const std::vector < int32 > &pos)
{
// Leave this #if 0 #endif since this illustrates how this function works. KY 2023-10-12
#if 0
    /*
     int a[10][20][30];  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3);
     int b[10][2]; // &b[1][2] == b + (20*1 + 2);
    */
#endif
    assert (dims.size () == pos.size ());
    int32 sum = 0;
    int32 start = 1;

    for (unsigned int p = 0; p < pos.size (); p++) {
        int32 m = 1;

        for (unsigned int j = start; j < dims.size (); j++)
            m *= dims[j];
        sum += m * pos[p];
        start++;
    }
    return sum;
}

// Build a lock of a certain type.
static inline struct flock *lock(int type) {
    static struct flock lock;
    lock.l_type = (short)type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    return &lock;
}

static inline string get_errno() {
        const char *s_err = strerror(errno);
        if (s_err)
                return s_err;
        else
                return "Unknown error.";
}

#endif
