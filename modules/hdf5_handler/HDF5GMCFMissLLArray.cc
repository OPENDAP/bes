// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2023 The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5GMCFMissLLArray.cc
/// \brief Implementation of the retrieval of the missing lat/lon values for general HDF5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2023 The HDF Group
///
/// All rights reserved.

#include <iostream>
#include <memory>
#include <cassert>
#include <BESDebug.h>
#include <libdap/InternalErr.h>
#include "HDF5RequestHandler.h"
#include "HDF5GMCFMissLLArray.h"
#include "h5apicompatible.h"

using namespace std;
using namespace libdap;

static int visit_obj_cb(hid_t  group_id, const char *name, const H5O_info_t *oinfo, void *_op_data);
static herr_t attr_info(hid_t loc_id, const char *name, const H5A_info_t *ainfo, void *_op_data);

BaseType *HDF5GMCFMissLLArray::ptr_duplicate()
{
    auto HDF5GMCFMissLLArray_unique = make_unique<HDF5GMCFMissLLArray>(*this);
    return HDF5GMCFMissLLArray_unique.release();
}

bool HDF5GMCFMissLLArray::read()
{

    BESDEBUG("h5", "Coming to HDF5GMCFMissLLArray read "<<endl);

    if (nullptr == HDF5RequestHandler::get_lrdata_mem_cache())
        read_data_NOT_from_mem_cache(false, nullptr);

    else {

        vector<string> cur_lrd_non_cache_dir_list;
        HDF5RequestHandler::get_lrd_non_cache_dir_list(cur_lrd_non_cache_dir_list);

        string cache_key;

        // Check if this file is included in the non-cache directory                                
        if ((cur_lrd_non_cache_dir_list.empty())
            || ("" == check_str_sect_in_list(cur_lrd_non_cache_dir_list, filename, '/'))) {
            short cache_flag = 2;
            vector<string> cur_cache_dlist;
            HDF5RequestHandler::get_lrd_cache_dir_list(cur_cache_dlist);
            string cache_dir = check_str_sect_in_list(cur_cache_dlist, filename, '/');
            if (cache_dir != "") {
                cache_flag = 3;
                cache_key = cache_dir + varname;
            }
            else
                cache_key = filename + varname;

            // Need to obtain the total number of elements.
            // Obtain dimension size info.
            vector<size_t> dim_sizes;
            Dim_iter i_dim = dim_begin();
            Dim_iter i_enddim = dim_end();
            while (i_dim != i_enddim) {
                dim_sizes.push_back(dimension_size_ll(i_dim));
                ++i_dim;
            }

            size_t total_elems = 1;
            for (const auto &dim_size:dim_sizes)
                total_elems = total_elems * dim_size;

            handle_data_with_mem_cache(dtype, total_elems, cache_flag, cache_key,false);
        }
        else
            read_data_NOT_from_mem_cache(false, nullptr);
    }
    return true;
}

// Obtain latitude and longitude for Aquarius and OBPG level 3 products
void HDF5GMCFMissLLArray::obtain_aqu_obpg_l3_ll(const int64_t* offset, const int64_t* step, int64_t nelms, bool add_cache, void* buf)
{

    BESDEBUG("h5", "Coming to obtain_aqu_obpg_l3_ll read "<<endl);

    // Read File attributes
    // Latitude Step, SW Point Latitude, Number of Lines
    // Longitude Step, SW Point Longitude, Number of Columns
    if (1 != rank) {
        string msg = "The number of dimension for Aquarius Level 3 map data must be 1.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    bool check_pass_fileid_key = HDF5RequestHandler::get_pass_fileid();
    if (false == check_pass_fileid_key) {
        if ((fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT)) < 0) {
            string msg = "HDF5 File " + filename + " cannot be opened. ";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }

    hid_t rootid = -1;
    if ((rootid = H5Gopen(fileid, "/", H5P_DEFAULT)) < 0) {
        HDF5CFUtil::close_fileid(fileid, check_pass_fileid_key);
        string msg = "HDF5 dataset " + varname + " cannot be opened. ";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    float LL_first_point = 0.0;
    float LL_step = 0.0;
    int LL_total_num = 0;

    if (CV_LAT_MISS == cvartype) {

        string Lat_SWP_name = (Aqu_L3 == product_type) ? "SW Point Latitude" : "sw_point_latitude";
        if ((Aqu_L3 == product_type) && (H5Aexists(rootid,Lat_SWP_name.c_str())==0))
            Lat_SWP_name ="sw_point_latitude";

        string Lat_step_name = (Aqu_L3 == product_type) ? "Latitude Step" : "latitude_step";
        if ((Aqu_L3 == product_type) && (H5Aexists(rootid,Lat_step_name.c_str())==0))
            Lat_step_name ="latitude_step";

        string Num_lines_name = (Aqu_L3 == product_type) ? "Number of Lines" : "number_of_lines";
        if ((Aqu_L3 == product_type) && (H5Aexists(rootid,Num_lines_name.c_str())==0))
            Num_lines_name ="number_of_lines";

        float Lat_SWP = 0.0;
        float Lat_step = 0.0;
        int Num_lines = 0;
        vector<char> dummy_str;

        obtain_ll_attr_value(fileid, rootid, Lat_SWP_name, Lat_SWP, dummy_str);
        obtain_ll_attr_value(fileid, rootid, Lat_step_name, Lat_step, dummy_str);
        obtain_ll_attr_value(fileid, rootid, Num_lines_name, Num_lines, dummy_str);
        if (Num_lines <= 0) {
            H5Gclose(rootid);
            HDF5CFUtil::close_fileid(fileid, check_pass_fileid_key);
            string msg = "The number of line must be >0.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        // The first number of the latitude is at the north west corner
        LL_first_point = Lat_SWP + (Num_lines - 1) * Lat_step;
        LL_step = (float)(Lat_step * (-1.0));
        LL_total_num = Num_lines;
    }

    if (CV_LON_MISS == cvartype) {

        string Lon_SWP_name = (Aqu_L3 == product_type) ? "SW Point Longitude" : "sw_point_longitude";
        if ((Aqu_L3 == product_type) && (H5Aexists(rootid,Lon_SWP_name.c_str())==0))
            Lon_SWP_name ="sw_point_longitude";

        string Lon_step_name = (Aqu_L3 == product_type) ? "Longitude Step" : "longitude_step";
        if ((Aqu_L3 == product_type) && (H5Aexists(rootid,Lon_step_name.c_str())==0))
            Lon_step_name ="longitude_step";

        string Num_columns_name = (Aqu_L3 == product_type) ? "Number of Columns" : "number_of_columns";
        if ((Aqu_L3 == product_type) && (H5Aexists(rootid,Num_columns_name.c_str())==0))
            Num_columns_name ="number_of_columns";

        float Lon_SWP = 0.0;
        float Lon_step = 0.0;
        int Num_cols = 0;

        vector<char> dummy_str_value;

        obtain_ll_attr_value(fileid, rootid, Lon_SWP_name, Lon_SWP, dummy_str_value);
        obtain_ll_attr_value(fileid, rootid, Lon_step_name, Lon_step, dummy_str_value);
        obtain_ll_attr_value(fileid, rootid, Num_columns_name, Num_cols, dummy_str_value);
        if (Num_cols <= 0) {
            H5Gclose(rootid);
            HDF5CFUtil::close_fileid(fileid, check_pass_fileid_key);
            string msg = "The number of line must be >0.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        // The first number of the latitude is at the north west corner
        LL_first_point = Lon_SWP;
        LL_step = Lon_step;
        LL_total_num = Num_cols;
    }

    vector<float> val;
    val.resize(nelms);

    if (nelms > LL_total_num) {
        H5Gclose(rootid);
        HDF5CFUtil::close_fileid(fileid, check_pass_fileid_key);
        string msg = "The number of elements exceeds the total number of  Latitude or Longitude.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    for (int64_t i = 0; i < nelms; ++i)
        val[i] = LL_first_point + (offset[0] + i * step[0]) * LL_step;

    if (true == add_cache) {
        vector<float> total_val;
        total_val.resize(LL_total_num);
        for (int64_t total_i = 0; total_i < LL_total_num; total_i++)
            total_val[total_i] = LL_first_point + total_i * LL_step;
        memcpy(buf, total_val.data(), 4 * LL_total_num);
    }

    set_value_ll(val.data(), nelms);
    H5Gclose(rootid);
    HDF5CFUtil::close_fileid(fileid, check_pass_fileid_key);
}

// Obtain lat/lon for GPM level 3 products
void HDF5GMCFMissLLArray::obtain_gpm_l3_ll(const int64_t* offset, const int64_t* step, int64_t nelms, bool add_cache, void*buf)
{

    if (1 != rank) {
        string msg = "The number of dimension for GPM Level 3 map data must be 1.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    bool check_pass_fileid_key = HDF5RequestHandler::get_pass_fileid();

    if (false == check_pass_fileid_key) {
        if ((fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT)) < 0) {
            string msg = "HDF5 File " + filename + " cannot be opened. ";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }

    vector<char> grid_info_value;
    float lat_start = 0;
    float lon_start = 0.;
    float lat_res = 0.;
    float lon_res = 0.;

    int latsize = 0;
    int lonsize = 0;


    if(GPMM_L3 == product_type || GPMS_L3 == product_type) {
        hid_t grid_grp_id = -1;
        string grid_grp_name;

        if ((name() == "nlat") || (name() == "nlon")) {
    
            string temp_grid_grp_name(GPM_GRID_GROUP_NAME1, strlen(GPM_GRID_GROUP_NAME1));
            temp_grid_grp_name = "/" + temp_grid_grp_name;
            if (H5Lexists(fileid, temp_grid_grp_name.c_str(), H5P_DEFAULT) > 0)
                grid_grp_name = temp_grid_grp_name;
            else {
                string temp_grid_grp_name2(GPM_GRID_GROUP_NAME2, strlen(GPM_GRID_GROUP_NAME2));
                temp_grid_grp_name2 = "/" + temp_grid_grp_name2;
                if (H5Lexists(fileid, temp_grid_grp_name2.c_str(), H5P_DEFAULT) > 0)
                    grid_grp_name = temp_grid_grp_name2;
                else {
                    string msg = "Unknown GPM grid group name. ";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
            }
        }
    
        else {
            string temp_grids_group_name(GPM_GRID_MULTI_GROUP_NAME, strlen(GPM_GRID_MULTI_GROUP_NAME));
            if (name() == "lnH" || name() == "ltH")
                grid_grp_name = temp_grids_group_name + "/G2";
            else if (name() == "lnL" || name() == "ltL") grid_grp_name = temp_grids_group_name + "/G1";
        }
    
        // varname is supposed to include the full path. However, it takes too much effort to obtain the full path 
        // for a created coordiate variable based on the dimension name only. Since GPM has a fixed group G1 
        // for lnL and ltL and another fixed group G2 for lnH and ltH. We just use these names. These information
        // is from GPM file specification.
       
        if ((grid_grp_id = H5Gopen(fileid, grid_grp_name.c_str(), H5P_DEFAULT)) < 0) {
            HDF5CFUtil::close_fileid(fileid, check_pass_fileid_key);
            string msg = "HDF5 dataset " + varname + " cannot be opened. ";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    
        // GPMDPR: update grid_info_name. 
        string grid_info_name(GPM_ATTR2_NAME, strlen(GPM_ATTR2_NAME));
        if (name() == "lnL" || name() == "ltL")
            grid_info_name = "G1_" + grid_info_name;
        else if (name() == "lnH" || name() == "ltH") grid_info_name = "G2_" + grid_info_name;
    
        float dummy_value = 0.0;
        try {
            obtain_ll_attr_value(fileid, grid_grp_id, grid_info_name, dummy_value, grid_info_value);
            HDF5CFUtil::parser_gpm_l3_gridheader(grid_info_value, latsize, lonsize, lat_start, lon_start, lat_res, lon_res,
            false);
    
           H5Gclose(grid_grp_id);
        }
        catch (...) {
            H5Gclose(grid_grp_id);
            H5Fclose(fileid);
            throw;
        }
    
    }
    else {
        vector<char> grid_info_value1;
        vector<char> grid_info_value2;
        obtain_gpm_l3_new_grid_info(fileid,grid_info_value1,grid_info_value2);
        obtain_lat_lon_info(grid_info_value1,grid_info_value2,latsize,lonsize,lat_start,lon_start,lat_res,lon_res);
    }

    HDF5CFUtil::close_fileid(fileid, check_pass_fileid_key);

    try {
        send_gpm_l3_ll_to_dap(latsize,lonsize,lat_start,lon_start,lat_res,lon_res,offset,step,nelms,add_cache, buf);
    }
    catch (...) {
        throw;
    }
}


// Obtain latitude/longitude attribute values
//template<class T>
template<typename T>
void HDF5GMCFMissLLArray::obtain_ll_attr_value(hid_t /*file_id*/, hid_t s_root_id, const string & s_attr_name,
    T& attr_value, vector<char> & str_attr_value) const
{

    BESDEBUG("h5", "Coming to obtain_ll_attr_value"<<endl);
    hid_t s_attr_id = -1;
    if ((s_attr_id = H5Aopen_by_name(s_root_id, ".", s_attr_name.c_str(),
    H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        string msg = "Cannot open the HDF5 attribute  ";
        msg += s_attr_name;
        H5Gclose(s_root_id);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    hid_t attr_type = -1;
    if ((attr_type = H5Aget_type(s_attr_id)) < 0) {
        string msg = "cannot get the attribute datatype for the attribute  ";
        msg += s_attr_name;
        H5Aclose(s_attr_id);
        H5Gclose(s_root_id);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    hid_t attr_space = -1;
    if ((attr_space = H5Aget_space(s_attr_id)) < 0) {
        string msg = "cannot get the hdf5 dataspace id for the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Gclose(s_root_id);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    hssize_t num_elm = H5Sget_simple_extent_npoints(attr_space);

    if (0 == num_elm) {
        string msg = "cannot get the number for the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    if (1 != num_elm) {
        string msg = "The number of attribute must be 1 for Aquarius level 3 data ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    size_t atype_size = H5Tget_size(attr_type);
    if (atype_size <= 0) {
        string msg = "cannot obtain the datatype size of the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    if (H5T_STRING == H5Tget_class(attr_type)) {
        if (H5Tis_variable_str(attr_type)) {
            H5Tclose(attr_type);
            H5Aclose(s_attr_id);
            H5Sclose(attr_space);
            H5Gclose(s_root_id);
            string msg = "Currently we assume the attributes we use to retrieve lat and lon are NOT variable length string.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
        else {
            str_attr_value.resize(atype_size);
            if (H5Aread(s_attr_id, attr_type, str_attr_value.data()) < 0) {
                string msg = "cannot retrieve the value of  the attribute ";
                msg += s_attr_name;
                H5Tclose(attr_type);
                H5Aclose(s_attr_id);
                H5Sclose(attr_space);
                H5Gclose(s_root_id);
                throw BESInternalError(msg,__FILE__, __LINE__);

            }
        }
    }

    else if (H5Aread(s_attr_id, attr_type, &attr_value) < 0) {
        string msg = "cannot retrieve the value of  the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        throw BESInternalError(msg,__FILE__, __LINE__);

    }

    H5Tclose(attr_type);
    H5Sclose(attr_space);
    H5Aclose(s_attr_id);
}

void HDF5GMCFMissLLArray::obtain_gpm_l3_new_grid_info(hid_t file,
                                                      vector<char>& grid_info_value1, 
                                                      vector<char>& grid_info_value2) const{

   typedef struct {
       char* name;
       char* value;
   } attr_info_t;

   attr_info_t attr_na;
   attr_na.name = nullptr;
   attr_na.value = nullptr;

   herr_t ret_o= H5OVISIT(file, H5_INDEX_NAME, H5_ITER_INC, visit_obj_cb, (void*)&attr_na);
   if(ret_o < 0){
        H5Fclose(file);
        string msg = "H5OVISIT failed. ";
        throw BESInternalError(msg,__FILE__,__LINE__);
   }
   else if(ret_o >0) {
        BESDEBUG("h5","Found the GPM level 3 Grid_info attribute."<<endl);
        grid_info_value1.resize(strlen(attr_na.value));
        memcpy(grid_info_value1.data(),attr_na.value,strlen(attr_na.value));
#if 0
        string tv(grid_info_value1.begin(),grid_info_value1.end());
        cerr<<"grid_info_value1 is "<<tv <<endl;
        printf("attr_name 1st is %s\n",attr_na.name);
        printf("attr_value 1st is %s\n",attr_na.value);
#endif
        // Find the grid_info_value of the second grid.
        // Note: the memory allocated for the first grid info is released 
        // by the attribute callback function.
        // In this round, we need to release the memory allocated for the second grid info.
        herr_t ret_o2= H5OVISIT(file, H5_INDEX_NAME, H5_ITER_INC, visit_obj_cb, (void*)&attr_na);
        if(ret_o2 < 0) {
            H5Fclose(file);
            string msg = "H5OVISIT failed again. ";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
        else if(ret_o2>0) {
            if(attr_na.name) {
#if 0
                //printf("attr_name second is %s\n",attr_na.name);
#endif
                free(attr_na.name);
            }
            if(attr_na.value) {
#if 0
                //printf("attr_value second is %s\n",attr_na.value);
                //grid_info_value2(attr_na.value,attr_na.value+strlen(attr_na.value));
#endif
                grid_info_value2.resize(strlen(attr_na.value));
                memcpy(grid_info_value2.data(),attr_na.value,strlen(attr_na.value));
#if 0
            string tv(grid_info_value2.begin(),grid_info_value2.end());
            //cerr<<"grid_info_value2 is "<<tv <<endl;
#endif 
                free(attr_na.value);
            }
        }
    }
}

void HDF5GMCFMissLLArray::obtain_lat_lon_info(const vector<char>& grid_info_value1,
                                              const vector<char>& grid_info_value2,
                                              int& latsize,int& lonsize,
                                              float& lat_start,float& lon_start,
                                              float& lat_res,float& lon_res) const{

    float lat1_start = 0;
    float lon1_start = 0.;
    float lat1_res = 0.;
    float lon1_res = 0.;
    int lat1size = 0;
    int lon1size = 0;

    float lat2_start = 0;
    float lon2_start = 0.;
    float lat2_res = 0.;
    float lon2_res = 0.;
    int lat2size = 0;
    int lon2size = 0;

    HDF5CFUtil::parser_gpm_l3_gridheader(grid_info_value1, lat1size, lon1size, lat1_start, lon1_start, 
                                         lat1_res, lon1_res,false);

    HDF5CFUtil::parser_gpm_l3_gridheader(grid_info_value2, lat2size, lon2size, lat2_start, lon2_start, 
                                         lat2_res, lon2_res,false);

    bool pick_gv1 = true;

    // We use the resolution (the smaller value is high resolution) to distinguish the two lat/lons.
    if (name() == "lnL" || name() == "ltL") {
        if(lat1_res <lat2_res) 
            pick_gv1 = false;
    }
    else if (name() == "lnH" || name() == "ltH") {
        if(lat1_res >lat2_res) 
            pick_gv1 = false;
    }

    if(true == pick_gv1) {
        latsize = lat1size;
        lonsize = lon1size;
        lat_start = lat1_start;
        lon_start = lon1_start;
        lat_res = lat1_res;
        lon_res = lon1_res;
    }
    else {
        latsize = lat2size;
        lonsize = lon2size;
        lat_start = lat2_start;
        lon_start = lon2_start;
        lat_res = lat2_res;
        lon_res = lon2_res;
    }
}

//Callback function to retrieve the grid information.
// We delibereately use malloc() and free() in this callback according to the HDF5 documentation. 
static herr_t
attr_info(hid_t loc_id, const char *name, const H5A_info_t *ainfo, void *_op_data)
{

   typedef struct {
       char* name;
       char* value;
   } attr_info_t;

    herr_t ret = 0;
    attr_info_t *op_data = (attr_info_t *)_op_data;

    // Attribute name is GridHeader
    if(strstr(name,GPM_ATTR2_NAME)!=nullptr)  {
        hid_t attr;
        hid_t atype;  
        attr = H5Aopen(loc_id, name, H5P_DEFAULT);
        if(attr<0) 
            return -1;
        atype  = H5Aget_type(attr);
        if(atype <0) {
            H5Aclose(attr);
            return -1;
        }
        if(H5T_STRING == H5Tget_class(atype)){
            // Note here: we find that the HDF5 API H5Tis_variable_str() causes seg. fault
            // when checking if this is a variable length string. A ticket has been submitted
            // to the HDF group. For GPM, only the fixed-size string is used. So it won't affect
            // here. When the bug is fixed. We should add a check here to avoid the crash of the prog.
            if(op_data->name) {
                if(strncmp(name,op_data->name,strlen(name))!=0) {
                    hid_t aspace = H5Aget_space(attr);
                    if(aspace <0) {
                        H5Aclose(attr);
                        H5Tclose(atype);
                        return -1;
                    }
                    hsize_t num_elms = H5Tget_size(atype)*H5Sget_simple_extent_npoints(aspace);
#if 0
                char *attr_value = op_data->value;
                attr_value = malloc(num_elms+1);
                H5Aread(attr,atype,attr_value);
                printf("attr_value is %s\n",attr_value);
#endif
                    char *cur_attr_value = (char*)malloc(num_elms+1);
                    if(H5Aread(attr,atype,(void*)cur_attr_value)<0) {
                        H5Aclose(attr);
                        H5Sclose(aspace);
                        H5Tclose(atype);
                        free(cur_attr_value);
                        return -1;
                    }

                    // There are two grids in the file. This "if clause" is for the second one.
                    if(strncmp(cur_attr_value,op_data->value,strlen(op_data->value))!=0) {
                        free(op_data->name);
                        op_data->name = nullptr;
                        op_data->name = (char*)malloc(strlen(name)+1);
                        strncpy(op_data->name,name,strlen(name));
                        if(op_data->value)
                            free(op_data->value);
                        op_data->value = nullptr;
                        op_data->value=(char*)malloc(num_elms+1);
                        strncpy(op_data->value,cur_attr_value,strlen(cur_attr_value));
                        ret = 1;
                    }
                    free(cur_attr_value);
                    H5Sclose(aspace);
                }
            }
            else {
                hid_t aspace = H5Aget_space(attr);
                if(aspace <0) {
                    H5Aclose(attr);
                    H5Tclose(atype);
                    return -1;
                }
 
                hsize_t num_elms = H5Tget_size(atype)*H5Sget_simple_extent_npoints(aspace);
                op_data->name = (char*)malloc(strlen(name)+1);
                strncpy(op_data->name,name,strlen(name));
               
#if 0
                char *attr_value = op_data->value;
                attr_value = malloc(num_elms+1);
                H5Aread(attr,atype,attr_value);
                printf("attr_value is %s\n",attr_value);
#endif
                op_data->value = (char*)malloc(num_elms+1);
                if(H5Aread(attr,atype,(void*)op_data->value)<0) {
                        H5Aclose(attr);
                        H5Sclose(aspace);
                        H5Tclose(atype);
                        free(op_data->value);
                }
                H5Sclose(aspace);
                ret =1;
            }
        }
        H5Tclose(atype);
        H5Aclose(attr);
    }
    return ret;

}

// The callback function to iterate every HDF5 object(including groups and datasets)
// Checked the internal HDF5 functions. The object type is used to obtain different
// objects in the internal function. So performance-wise, this routine should be
// the same as the routine that uses the H5Literate.
//
static int 
visit_obj_cb(hid_t  group_id, const char *name, const H5O_info_t *oinfo,
    void *_op_data)
{
   typedef struct {
       char* name;
       char* value;
   } attr_info_t;


#if 0
    //lvisit_ud_t *op_data = (lvisit_ud_t *)_op_data;
#endif

    attr_info_t *op_data = (attr_info_t *)_op_data;
    herr_t ret = 0;

    if(oinfo->type == H5O_TYPE_GROUP) {

        hid_t grp = -1;
        grp = H5Gopen2(group_id,name,H5P_DEFAULT);
        if(grp < 0) 
            return -1;
        ret = H5Aiterate2(grp, H5_INDEX_NAME, H5_ITER_INC, nullptr, attr_info, op_data);
#if 0
    if(ret > 0) {
    printf("object: attr name is %s\n",op_data->name);
    printf("object: attr value is %s\n",op_data->value);
    }
#endif
        if(ret <0){ 
            H5Gclose(grp);
            return -1;
        }
        H5Gclose(grp);
    }
    return ret;
 
}

void HDF5GMCFMissLLArray::send_gpm_l3_ll_to_dap(const int latsize,const int lonsize,const float lat_start,const float lon_start,
                                                const float lat_res,const float lon_res, const int64_t* offset,const int64_t* step,
                                                const int64_t nelms,const bool add_cache, void*buf) {

    if (0 == latsize || 0 == lonsize) {
        string msg = "Either latitude or longitude size is 0. ";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    vector<float> val;
    val.resize(nelms);

    if (CV_LAT_MISS == cvartype) {

        if (nelms > latsize) {
            string msg = "The number of elements exceeds the total number of  Latitude. ";
            throw BESInternalError(msg,__FILE__,__LINE__);

        }
        for (int64_t i = 0; i < nelms; ++i)
            val[i] = lat_start + offset[0] * lat_res + lat_res / 2 + i * lat_res * step[0];

        if (add_cache == true) {
            vector<float> total_val;
            total_val.resize(latsize);
            for (int64_t total_i = 0; total_i < latsize; total_i++)
                total_val[total_i] = lat_start + lat_res / 2 + total_i * lat_res;
            memcpy(buf, total_val.data(), 4 * latsize);
        }
    }
    else if (CV_LON_MISS == cvartype) {

        if (nelms > lonsize) {
            string msg = "The number of elements exceeds the total number of  Longitude.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        for (int64_t i = 0; i < nelms; ++i)
            val[i] = lon_start + offset[0] * lon_res + lon_res / 2 + i * lon_res * step[0];

        if (add_cache == true) {
            vector<float> total_val;
            total_val.resize(lonsize);
            for (int total_i = 0; total_i < lonsize; total_i++)
                total_val[total_i] = lon_start + lon_res / 2 + total_i * lon_res;
            memcpy(buf, total_val.data(), 4 * lonsize);
        }

    }

    set_value_ll((dods_float32 *) val.data(), nelms);

}

void HDF5GMCFMissLLArray::read_data_NOT_from_mem_cache(bool add_cache, void*buf)
{

    BESDEBUG("h5", "Coming to HDF5GMCFMissLLArray: read_data_NOT_from_mem_cache  "<<endl);

    // Here we still use vector just in case we need to tackle "rank>1" in the future.
    // Also we would like to keep it consistent with other similar handlings.
    vector<int64_t> offset;
    vector<int64_t> count;
    vector<int64_t> step;

    offset.resize(rank);
    count.resize(rank);
    step.resize(rank);

    int64_t nelms = format_constraint(offset.data(), step.data(), count.data());

    if (GPMM_L3 == product_type || GPMS_L3 == product_type || GPM_L3_New == product_type)
        obtain_gpm_l3_ll(offset.data(), step.data(), nelms, add_cache, buf);
    else if (Aqu_L3 == product_type || OBPG_L3 == product_type) // Aquarious level 3 
        obtain_aqu_obpg_l3_ll(offset.data(), step.data(), nelms, add_cache, buf);

    return;

}

