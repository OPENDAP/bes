/////////////////////////////////////////////////////////////////////////////
// Helper functions for handling dimension maps,clear memories and separating namelist
//
//  Authors:   MuQun Yang <myang6@hdfgroup.org> Eunsoo Seo
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifndef HDFCFUTIL_H
#define HDFCFUTIL_H

#include <stdlib.h>
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

#include "mfhdf.h"
#include "hdf.h"

#ifdef USE_HDFEOS2_LIB
#include "HdfEosDef.h"
#include "HDFEOS2.h"
#endif
#include "HDFSP.h"

#include <TheBESKeys.h>
#include <BESUtil.h>
#include <DDS.h>
#include <DAS.h>
#include <InternalErr.h>
//#include <dodsutil.h>


#include "escaping.h" // for escattr

#define MAX_NON_SCALE_SPECIAL_VALUE 65535
#define MIN_NON_SCALE_SPECIAL_VALUE 65500

//using namespace std;
//using namespace libdap;

struct dimmap_entry
{
    std::string geodim;
    std::string datadim;
    int32 offset, inc;
};


struct HDFCFUtil
{

    /// Check the BES key. 
    static bool check_beskeys(const std::string key);

    /// From a string separated by a separator to a list of string,
    /// for example, split "ab,c" to {"ab","c"}
    static void Split (const char *s, int len, char sep,
        std::vector < std::string > &names);

    /// Assume sz is Null terminated string.
    static void Split (const char *sz, char sep,
                            std::vector < std::string > &names);

    /// Clear memory, No need anymore. Will remove it. KY 2013-02-13
#if 0
    static void ClearMem (int32 * offset32, int32 * count32, int32 * step32,
						  int *offset, int *count, int *step);
    static void ClearMem2 (int32 * offset32, int32 * count32, int32 * step32);
    static void ClearMem3 (int *offset, int *count, int *step);
#endif

    /// This is a safer way to insert and update map item than map[key]=val style string assignment.
    /// Otherwise, the local testsuite at The HDF Group will fail for HDF-EOS2 data
    ///  under iMac machine platform.
    static bool insert_map(std::map<std::string,std::string>& m, std::string key, std::string val);

    /// Change special characters to "_"
    static string get_CF_string(std::string s);

    /// Obtain the unique name for the clashed names and save it to set namelist.
    static void gen_unique_name(std::string &str,std::set<std::string>& namelist, int&clash_index);

    static void Handle_NameClashing(std::vector<std::string>&newobjnamelist);
    static void Handle_NameClashing(std::vector<std::string>&newobjnamelist,std::set<std::string>&objnameset);

    static std::string print_attr(int32, int, void*);
    static std::string print_type(int32);

    // Subsetting the 2-D fields
    template <typename T> static void LatLon2DSubset (T* outlatlon, int ydim, int xdim, T* latlon, int32 * offset, int32 * count, int32 * step);

    static void correct_fvalue_type(libdap::AttrTable *at,int32 dtype);
#ifdef USE_HDFEOS2_LIB
    // Handle MODIS data products
    static bool change_data_type(libdap::DAS & das, SOType scaletype, std::string new_field_name);
    static bool is_special_value(int32 dtype,float fillvalue, float realvalue);

    static int check_geofile_dimmap(const std::string & geofilename);
    static bool is_modis_dimmap_nonll_field(std::string & fieldname);
    static void obtain_dimmap_info(const std::string& filename, HDFEOS2::Dataset*dataset,std::vector<struct dimmap_entry>& dimmaps, std::string & modis_geofilename,bool &geofile_nas_dimmap);
    static void handle_modis_special_attrs(libdap::AttrTable *at,const std::string newfname, SOType scaletype, bool gridname_change_valid_range, bool changedtype, bool &change_fvtype);
    static void handle_modis_vip_special_attrs(const std::string& valid_range_value,const std::string& scale_factor_value, float& valid_min, float & valid_max);
    static void handle_amsr_attrs(libdap::AttrTable *at);
#endif 

    // Handle HDF4 OBPG products
    static void check_obpg_global_attrs(HDFSP::File*f,std::string & scaling, float & slope,bool &global_slope_flag,float & intercept, bool & global_intercept_flag);
    static void add_obpg_special_attrs(HDFSP::File*f,libdap::DAS &das,  HDFSP::SDField* spsds,std::string & scaling, float&slope,bool &global_slope_flag,float& intercept,bool &global_intercept_flag);

    // Handle HDF4 OTHERHDF products that follow SDS dimension scale model. 
    // The special handling of AVHRR data is also included.
    static void handle_otherhdf_special_attrs(HDFSP::File *f, libdap::DAS &das);

    // Handle Merra and CERES attributes with the BES key EnableCERESMERRAShortName.
    static void handle_merra_ceres_attrs_with_bes_keys(HDFSP::File*f, libdap::DAS &das,std::string filename);

    // Handle the attributes with the BES key EnableVdataDescAttr.
    static void handle_vdata_attrs_with_desc_key(HDFSP::File*f,libdap::DAS &das);
};

inline int32
INDEX_nD_TO_1D (const std::vector < int32 > &dims,
                                const std::vector < int32 > &pos)
{
    /*
     int a[10][20][30];  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3);
     int b[10][2]; // &b[1][2] == b + (20*1 + 2);
    */
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


#endif
