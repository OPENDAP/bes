// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5GMCFMissLLArray.cc
/// \brief Implementation of the retrieval of the missing lat/lon values for general HDF5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2013 The HDF Group
///
/// All rights reserved.

#include "config_hdf5.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "InternalErr.h"
#include <BESDebug.h>

#include "HDF5GMCFMissLLArray.h"



BaseType *HDF5GMCFMissLLArray::ptr_duplicate()
{
    return new HDF5GMCFMissLLArray(*this);
}

bool HDF5GMCFMissLLArray::read()
{

    BESDEBUG("h5","Coming to HDF5GMCFMissLLArray read "<<endl);
    // Here we still use vector just in case we need to tackle "rank>1" in the future.
    // Also we would like to keep it consistent with other similar handlings.

    vector<int>offset;
    vector<int>count;
    vector<int>step;

    offset.resize(rank);
    count.resize(rank);
    step.resize(rank);
        
    int nelms = format_constraint (&offset[0], &step[0], &count[0]);

    if (GPMM_L3 == product_type || GPMS_L3 == product_type) 
       obtain_gpm_l3_ll(&offset[0],&step[0],nelms);
    else if (Aqu_L3 == product_type || OBPG_L3 == product_type) // Aquarious level 3 
       obtain_aqu_obpg_l3_ll(&offset[0],&step[0],nelms);

    return true;
}

void HDF5GMCFMissLLArray::obtain_aqu_obpg_l3_ll(int* offset,int* step,int nelms) {

        // Read File attributes
        // Latitude Step, SW Point Latitude, Number of Lines
        // Longitude Step, SW Point Longitude, Number of Columns
        
    if (1 != rank ) 
          throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension for Aquarius Level 3 map data must be 1");

    string check_pass_fileid_key_str="H5.EnablePassFileID";
    bool check_pass_fileid_key = false;
    check_pass_fileid_key = HDF5CFDAPUtil::check_beskeys(check_pass_fileid_key_str);
    if(false == check_pass_fileid_key) {
    if ((fileid = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {
        ostringstream eherr;
        eherr << "HDF5 File " << filename 
              << " cannot be opened. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }
    }

    hid_t rootid = -1;
    if ((rootid = H5Gopen(fileid,"/",H5P_DEFAULT)) < 0) {
     //       H5Fclose(fileid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "HDF5 dataset " << varname
                  << " cannot be opened. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    float LL_first_point = 0.0;
    float LL_step = 0.0;
    int   LL_total_num = 0;

    if (CV_LAT_MISS == cvartype) {
       string Lat_SWP_name =(Aqu_L3 == product_type)?"SW Point Latitude":"sw_point_latitude";
       string Lat_step_name =(Aqu_L3== product_type)?"Latitude Step":"latitude_step";
       string Num_lines_name =(Aqu_L3== product_type)?"Number of Lines":"number_of_lines";
       float  Lat_SWP = 0.0;
       float  Lat_step = 0.0;
       int    Num_lines = 0;
       vector<char> dummy_str;

       obtain_ll_attr_value(fileid,rootid,Lat_SWP_name,Lat_SWP,dummy_str);
       obtain_ll_attr_value(fileid,rootid,Lat_step_name,Lat_step,dummy_str);
       obtain_ll_attr_value(fileid,rootid,Num_lines_name,Num_lines,dummy_str);
       // STOP 
       if (Num_lines <= 0) {
                H5Gclose(rootid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
                //H5Fclose(fileid);
                throw InternalErr(__FILE__,__LINE__,"The number of line must be >0");
            }

            // The first number of the latitude is at the north west corner
            LL_first_point = Lat_SWP + (Num_lines -1)*Lat_step;
            LL_step = Lat_step *(-1.0);
            LL_total_num = Num_lines;
        }

        if (CV_LON_MISS == cvartype) {
            string Lon_SWP_name =(Aqu_L3==product_type)?"SW Point Longitude":"sw_point_longitude";
            string Lon_step_name =(Aqu_L3==product_type)? "Longitude Step":"longitude_step";
            string Num_columns_name =(Aqu_L3==product_type)? "Number of Columns":"number_of_columns";
            float  Lon_SWP = 0.0;
            float  Lon_step = 0.0;
            int    Num_cols =0;

            vector<char>dummy_str_value;

            obtain_ll_attr_value(fileid,rootid,Lon_SWP_name,Lon_SWP,dummy_str_value);
            obtain_ll_attr_value(fileid,rootid,Lon_step_name,Lon_step,dummy_str_value);
            obtain_ll_attr_value(fileid,rootid,Num_columns_name,Num_cols,dummy_str_value);
            if (Num_cols <= 0) {
                H5Gclose(rootid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
                //H5Fclose(fileid);
                throw InternalErr(__FILE__,__LINE__,"The number of line must be >0");
            }

            // The first number of the latitude is at the north west corner
            LL_first_point = Lon_SWP;
            LL_step = Lon_step;
            LL_total_num = Num_cols;
        }

        vector<float>val;
        val.resize(nelms);

        if (nelms > LL_total_num) {
            H5Gclose(rootid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
            //H5Fclose(fileid);
            throw InternalErr (__FILE__, __LINE__,
                          "The number of elements exceeds the total number of  Latitude or Longitude");
        }

        for (int i = 0; i < nelms; ++i)
            val[i] = LL_first_point + (offset[0] + i*step[0])*LL_step;

        set_value ((dods_float32 *) &val[0], nelms);
        H5Gclose(rootid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        //H5Fclose(fileid);
 
}

void HDF5GMCFMissLLArray::obtain_gpm_l3_ll(int* offset,int* step,int nelms) {

        if (1 != rank ) 
            throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension for Aquarius Level 3 map data must be 1");

     string check_pass_fileid_key_str="H5.EnablePassFileID";
    bool check_pass_fileid_key = false;
    check_pass_fileid_key = HDF5CFDAPUtil::check_beskeys(check_pass_fileid_key_str);

    if(false == check_pass_fileid_key) {
    if ((fileid = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {
        ostringstream eherr;
        eherr << "HDF5 File " << filename 
              << " cannot be opened. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }
    }

       hid_t grid_grp_id = -1;
        
        string grid_grp_name;
        //string sub_grp1_name, sub_grp2_name;
        // GPMDPR: update grid_group_name. 
//        cerr<<"name is "<<name <<endl;
        if((name() == "nlat") || (name() == "nlon")) {
            string temp_grid_grp_name(GPM_GRID_GROUP_NAME1,strlen(GPM_GRID_GROUP_NAME1));
            temp_grid_grp_name = "/" + temp_grid_grp_name;
            if(H5Lexists(fileid,temp_grid_grp_name.c_str(),H5P_DEFAULT) >0)
               grid_grp_name = temp_grid_grp_name;
            else {
                string temp_grid_grp_name2(GPM_GRID_GROUP_NAME2,strlen(GPM_GRID_GROUP_NAME2));
                temp_grid_grp_name2 = "/"+temp_grid_grp_name2;
                if(H5Lexists(fileid,temp_grid_grp_name2.c_str(),H5P_DEFAULT) >0)
                   grid_grp_name = temp_grid_grp_name2;
                else 
                   throw InternalErr (__FILE__, __LINE__,"Unknown GPM grid group name ");
 
            }
        }

        else {
            string temp_grids_group_name(GPM_GRID_MULTI_GROUP_NAME,strlen(GPM_GRID_MULTI_GROUP_NAME));
            if(name() == "lnH" || name() == "ltH") 
                grid_grp_name = temp_grids_group_name + "/G2" ;
            else if(name() == "lnL" || name() == "ltL") 
                grid_grp_name = temp_grids_group_name + "/G1" ;
        }
// varname is supposed to include the full path. However, it takes too much effort to obtain the full path 
// for a created coordiate variable based on the dimension name only. Since GPM has a fixed group G1 
// for lnL and ltL and another fixed group G2 for lnH and ltH. We just use these names. These information
// is from GPM file specification.
#if 0
            if(name() == "lnH" || name() == "ltH" || 
                name() == "lnL" || name() == "ltL") {
                string temp_grids_group_name(GPM_GRID_MULTI_GROUP_NAME,strlen(GPM_GRID_MULTI_GROUP_NAME));
               
cerr<<"varname is "<<varname <<endl;
                size_t grids_group_pos = varname.find(temp_grids_group_name);
                if(string::npos == grids_group_pos) {
                    throw InternalErr (__FILE__, __LINE__,
                              "Cannot find group Grids.");
                }

                string grids_cgroup_path = varname.substr(grids_group_pos+1);
                size_t grids_cgroup_pos = varname.find_first_of("/");
                if(string::npos == grids_cgroup_pos) {
                     throw InternalErr (__FILE__, __LINE__,
                          "Cannot find child group of group Grids.");
                }

                string temp_sub_grp_name = grids_cgroup_path.substr(0,grids_cgroup_pos);
                if(name() == "lnH" || name() == "ltH")
                    sub_grp1_name = temp_sub_grp_name;
                else if(name() == "lnL" || name() == "ltL") 
                    sub_grp2_name = temp_sub_grp_name;

                grid_grp_name = temp_grids_group_name + "/" + temp_sub_grp_name;

            }
#endif

        if ((grid_grp_id = H5Gopen(fileid,grid_grp_name.c_str(),H5P_DEFAULT)) < 0) {
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "HDF5 dataset " << varname
                  << " cannot be opened. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        // GPMDPR: update grid_info_name. 
        string grid_info_name(GPM_ATTR2_NAME,strlen(GPM_ATTR2_NAME));
        if(name() == "lnL" || name() == "ltL") 
            grid_info_name = "G1_" + grid_info_name;
        else if(name() == "lnH" || name() == "ltH")
            grid_info_name = "G2_" + grid_info_name;

        vector<char> grid_info_value;
        float dummy_value = 0.0;
        try {
        obtain_ll_attr_value(fileid,grid_grp_id,grid_info_name,dummy_value,grid_info_value);
        }
        catch(...) {
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
            throw;

        }
        
        float lat_start = 0;;
        float lon_start = 0.;
        float lat_res = 0.;
        float lon_res = 0.;

        int   latsize = 0;
        int   lonsize = 0;

        //vector<char> info_value(grid_info_value.begin(),grid_info_value.end());
        HDF5CFUtil::parser_gpm_l3_gridheader(grid_info_value,latsize,lonsize,
                                         lat_start,lon_start,lat_res,lon_res,false);

        if(0 == latsize || 0 == lonsize) {
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        throw InternalErr (__FILE__, __LINE__, "Either latitude or longitude size is 0. ");
        }

        vector<float>val;
        val.resize(nelms);

        if(CV_LAT_MISS == cvartype) {

            if(nelms > latsize) {
                H5Gclose(grid_grp_id);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
            //H5Fclose(fileid);
                throw InternalErr (__FILE__, __LINE__,
                          "The number of elements exceeds the total number of  Latitude ");

            }
            for (int i = 0; i < nelms; ++i) 
                val[i] = lat_start+offset[0]*lat_res+lat_res/2 + i*lat_res*step[0];
        }
        else if(CV_LON_MISS == cvartype) {

            if(nelms > lonsize) {
                H5Gclose(grid_grp_id);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
            //H5Fclose(fileid);
                throw InternalErr (__FILE__, __LINE__,
                          "The number of elements exceeds the total number of  Longitude");

            }

            for (int i = 0; i < nelms; ++i) 
                val[i] = lon_start+offset[0]*lon_res+lon_res/2 + i*lon_res*step[0];
        }
    
    set_value ((dods_float32 *) &val[0], nelms);

    H5Gclose(grid_grp_id);
    HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);

#if 0
        

        vector<float>val;
        val.resize(nelms);

        if (nelms > LL_total_num) {
            H5Gclose(rootid);
            //H5Fclose(fileid);
            throw InternalErr (__FILE__, __LINE__,
                          "The number of elements exceeds the total number of  Latitude or Longitude");
        }

        for (int i = 0; i < nelms; ++i)
            val[i] = LL_first_point + (offset[0] + i*step[0])*LL_step;

        set_value ((dods_float32 *) &val[0], nelms);
        H5Gclose(rootid);
        //H5Fclose(fileid);
#endif
 
}
//template<class T>
template<typename T>
void HDF5GMCFMissLLArray::obtain_ll_attr_value(hid_t file_id, hid_t s_root_id,
                                               const string & s_attr_name, T& attr_value,
                                               vector<char> & str_attr_value) {

    hid_t s_attr_id = -1;
    if ((s_attr_id = H5Aopen_by_name(s_root_id,".",s_attr_name.c_str(),
                     H5P_DEFAULT, H5P_DEFAULT)) <0) {
        string msg = "Cannot open the HDF5 attribute  ";
        msg += s_attr_name;
        H5Gclose(s_root_id);
        //H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    hid_t attr_type = -1;
    if ((attr_type = H5Aget_type(s_attr_id)) < 0) {
        string msg = "cannot get the attribute datatype for the attribute  ";
        msg += s_attr_name;
        H5Aclose(s_attr_id);
        H5Gclose(s_root_id);
        //H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    hid_t attr_space = -1;
    if ((attr_space = H5Aget_space(s_attr_id)) < 0) {
        string msg = "cannot get the hdf5 dataspace id for the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Gclose(s_root_id);
        //H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    int num_elm = H5Sget_simple_extent_npoints(attr_space);
     
    if (0 == num_elm ) {
        string msg = "cannot get the number for the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        //H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    if (1 != num_elm) {
        string msg = "The number of attribute must be 1 for Aquarius level 3 data ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        //H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }


    size_t atype_size = H5Tget_size(attr_type);
    if (atype_size <= 0) {
        string msg = "cannot obtain the datatype size of the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        //H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    if(H5T_STRING == H5Tget_class(attr_type)) {
        if(H5Tis_variable_str(attr_type)) { 
            H5Tclose(attr_type);
            H5Aclose(s_attr_id);
            H5Sclose(attr_space);
            H5Gclose(s_root_id);
            throw InternalErr(__FILE__,__LINE__,
            "Currently we assume the attributes we use to retrieve lat and lon are NOT variable length string.");
        }
        else {
            str_attr_value.resize(atype_size);
            if (H5Aread(s_attr_id,attr_type, &str_attr_value[0])<0){
                string msg = "cannot retrieve the value of  the attribute ";
                msg += s_attr_name;
                H5Tclose(attr_type);
                H5Aclose(s_attr_id);
                H5Sclose(attr_space);
                H5Gclose(s_root_id);
                //H5Fclose(file_id);
                throw InternalErr(__FILE__, __LINE__, msg);

            }
        }
    }

    else if (H5Aread(s_attr_id,attr_type, &attr_value)<0){
        string msg = "cannot retrieve the value of  the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        //H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);

    }

    H5Tclose(attr_type);
    H5Sclose(attr_space);
    H5Aclose(s_attr_id);
}



// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDF5GMCFMissLLArray::format_constraint (int *offset, int *step, int *count)
{
        long nels = 1;
        int id = 0;

        Dim_iter p = dim_begin ();

        while (p != dim_end ()) {

                int start = dimension_start (p, true);
                int stride = dimension_stride (p, true);
                int stop = dimension_stop (p, true);


                // Check for illegical  constraint
                if (stride < 0 || start < 0 || stop < 0 || start > stop) {
                        ostringstream oss;

                        oss << "Array/Grid hyperslab indices are bad: [" << start <<
                                ":" << stride << ":" << stop << "]";
                        throw Error (malformed_expr, oss.str ());
                }

                // Check for an empty constraint and use the whole dimension if so.
                if (start == 0 && stop == 0 && stride == 0) {
                        start = dimension_start (p, false);
                        stride = dimension_stride (p, false);
                        stop = dimension_stop (p, false);
                }

                offset[id] = start;
                step[id] = stride;
                count[id] = ((stop - start) / stride) + 1;      // count of elements
                nels *= count[id];              // total number of values for variable

                BESDEBUG ("h5",
                         "=format_constraint():"
                         << "id=" << id << " offset=" << offset[id]
                         << " step=" << step[id]
                         << " count=" << count[id]
                         << endl);

                id++;
                p++;
        }

        return nels;
}

