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
    // For Aquarius level 3, we need to calculate the latitude and longitude
    if (product_type == Aqu_L3) {

        // Read File attributes
        // Latitude Step, SW Point Latitude, Number of Lines
        // Longitude Step, SW Point Longitude, Number of Columns
        

        if (1 != rank ) 
            throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension for Aquarius Level 3 map data must be 1");

        // Here we still use vector just in case we need to tackle "rank>1" in the future.
        // Also we would like to keep it consistent with other similar handlings.

        vector<int>offset;
        vector<int>count;
        vector<int>step;
        vector<hsize_t>hoffset;
        vector<hsize_t>hcount;
        vector<hsize_t>hstep;

        offset.resize(rank);
        count.resize(rank);
        step.resize(rank);
        hoffset.resize(rank);
        hcount.resize(rank);
        hstep.resize(rank);
        
        int nelms = format_constraint (&offset[0], &step[0], &count[0]);

        for (int i = 0; i <rank; i++) {
            hoffset[i] = (hsize_t) offset[i];
            hcount[i] = (hsize_t) count[i];
            hstep[i] = (hsize_t) step[i];
        }
 
        hid_t fileid = -1;
        if ((fileid = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {

            ostringstream eherr;
            eherr << "HDF5 File " << filename
              << " cannot be opened. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        hid_t rootid = -1;
        if ((rootid = H5Gopen(fileid,"/",H5P_DEFAULT)) < 0) {
            H5Fclose(fileid);
            ostringstream eherr;
            eherr << "HDF5 dataset " << varname
                  << " cannot be opened. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        float LL_first_point = 0.0;
        float LL_step = 0.0;
        int   LL_total_num = 0;

        if (CV_LAT_MISS == cvartype) {

            string Lat_SWP_name ="SW Point Latitude";
            string Lat_step_name = "Latitude Step";
            string Num_lines_name = "Number of Lines";
            float  Lat_SWP = 0.0;
            float  Lat_step = 0.0;
            int    Num_lines = 0;

            obtain_ll_attr_value(fileid,rootid,Lat_SWP_name,Lat_SWP);
            obtain_ll_attr_value(fileid,rootid,Lat_step_name,Lat_step);
            obtain_ll_attr_value(fileid,rootid,Num_lines_name,Num_lines);
            if (Num_lines <= 0) {
                H5Gclose(rootid);
                H5Fclose(fileid);
                throw InternalErr(__FILE__,__LINE__,"The number of line must be >0");
            }

            // The first number of the latitude is at the north west corner
            LL_first_point = Lat_SWP + (Num_lines -1)*Lat_step;
            LL_step = Lat_step *(-1.0);
            LL_total_num = Num_lines;
        }

        if (CV_LON_MISS == cvartype) {
            string Lon_SWP_name ="SW Point Longitude";
            string Lon_step_name = "Longitude Step";
            string Num_columns_name = "Number of Columns";
            float  Lon_SWP = 0.0;
            float  Lon_step = 0.0;
            int    Num_cols =0;

            obtain_ll_attr_value(fileid,rootid,Lon_SWP_name,Lon_SWP);
            obtain_ll_attr_value(fileid,rootid,Lon_step_name,Lon_step);
            obtain_ll_attr_value(fileid,rootid,Num_columns_name,Num_cols);
            if (Num_cols <= 0) {
                H5Gclose(rootid);
                H5Fclose(fileid);
                throw InternalErr(__FILE__,__LINE__,"The number of line must be >0");
            }

            // The first number of the latitude is at the north west corner
            LL_first_point = Lon_SWP;
            LL_step = Lon_step;
            LL_total_num = Num_cols;
        }

        vector<float>val;
        val.resize(nelms);

#if 0
        float *val = NULL;
         
        try {
            val       = new float[nelms];
        }
        catch (...) {
            H5Gclose(rootid);
            H5Fclose(fileid);
            throw InternalErr (__FILE__, __LINE__,
                          "Cannot allocate the memory for total_val and val for Latitude or Longitude");
        }
#endif

        if (nelms > LL_total_num) {
            H5Gclose(rootid);
            H5Fclose(fileid);
            throw InternalErr (__FILE__, __LINE__,
                          "The number of elements exceeds the total number of  Latitude or Longitude");
        }

        for (int i = 0; i < nelms; ++i)
            val[i] = LL_first_point + (offset[0] + i*step[0])*LL_step;

        set_value ((dods_float32 *) &val[0], nelms);
        H5Gclose(rootid);
        H5Fclose(fileid);
    }
    return false;
}

template<class T>
void HDF5GMCFMissLLArray::obtain_ll_attr_value(hid_t file_id, hid_t s_root_id,string s_attr_name, T& attr_value) {

    hid_t s_attr_id = -1;
    if ((s_attr_id = H5Aopen_by_name(s_root_id,".",s_attr_name.c_str(),
                     H5P_DEFAULT, H5P_DEFAULT)) <0) {
        string msg = "Cannot open the HDF5 attribute  ";
        msg += s_attr_name;
        H5Gclose(s_root_id);
        H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    hid_t attr_type = -1;
    if ((attr_type = H5Aget_type(s_attr_id)) < 0) {
        string msg = "cannot get the attribute datatype for the attribute  ";
        msg += s_attr_name;
        H5Aclose(s_attr_id);
        H5Gclose(s_root_id);
        H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    hid_t attr_space = -1;
    if ((attr_space = H5Aget_space(s_attr_id)) < 0) {
        string msg = "cannot get the hdf5 dataspace id for the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Gclose(s_root_id);
        H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    int num_elm = 0;
    if (((num_elm = H5Sget_simple_extent_npoints(attr_space)) == 0)) {
        string msg = "cannot get the number for the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    if (1 != num_elm) {
        string msg = "The number of attribute must be 1 for Aquarius level 3 data ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        H5Fclose(file_id);
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
        H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    if (H5Aread(s_attr_id,attr_type, &attr_value)<0){
        string msg = "cannot retrieve the value of  the attribute ";
        msg += s_attr_name;
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        H5Fclose(file_id);
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

