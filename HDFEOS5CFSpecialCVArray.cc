// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDFEOS5CFSpecialCVArray.cc
/// \brief This class specifies the retrieval of special coordinate variable  values for HDF-EOS5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011 The HDF Group
///
/// All rights reserved.

#include "config_hdf5.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "InternalErr.h"
#include <BESDebug.h>

#include "HDFEOS5CFSpecialCVArray.h"

BaseType *HDFEOS5CFSpecialCVArray::ptr_duplicate()
{
    return new HDFEOS5CFSpecialCVArray(*this);
}

bool HDFEOS5CFSpecialCVArray::read(){

    int* offset = NULL;
    int* count = NULL ;
    int* step = NULL;
    int nelms;

#if 0
cerr<<"coming to read function"<<endl;
cerr<<"file name " <<filename <<endl;
cerr <<"var name "<<varname <<endl;
#endif

    if (rank < 0) 
        throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension of the variable is negative.");

    else if (rank == 0) 
        nelms = 1;
    else {
        try {
            offset = new int[rank];
            count = new int[rank];
            step = new int[rank];
        }

        catch (...) {
            HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
            throw InternalErr (__FILE__, __LINE__,
                              "Cannot allocate the memory for offset,count and setp.");
        }

        nelms = format_constraint (offset, step, count);

    }

    hid_t file_id = -1;

    if ((file_id = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {

        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        ostringstream eherr;
        eherr << "HDF5 File " << filename 
              << " cannot be opened. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    string cv_name = HDF5CFUtil::obtain_string_after_lastslash(varname);
    if ("" == cv_name) {
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr (__FILE__, __LINE__, "Cannot obtain TES CV attribute");
    }

    string group_name = varname.substr(0,varname.size()-cv_name.size());
#if 0
cerr<<"cv_name "<<cv_name <<endl;
cerr<<"group_name "<<group_name <<endl;
#endif
    
    size_t cv_name_sep_pos = cv_name.find_first_of('_',0);  

    if (string::npos == cv_name_sep_pos) {
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr (__FILE__, __LINE__, "Cannot obtain TES CV attribute");
    }
    string cv_attr_name = cv_name.substr(0,cv_name_sep_pos);
#if 0
cerr<<"attr_name "<<cv_attr_name <<endl;
#endif

    htri_t swath_link_exist = H5Lexists(file_id,group_name.c_str(),H5P_DEFAULT);

    if (swath_link_exist <= 0) {
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr (__FILE__, __LINE__, "The TES swath link doesn't exist");
    }
    
    htri_t swath_exist = H5Oexists_by_name(file_id,group_name.c_str(),H5P_DEFAULT); 
    if (swath_exist <= 0) {
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr (__FILE__, __LINE__, "The TES swath doesn't exist");
    }

    htri_t cv_attr_exist = H5Aexists_by_name(file_id,group_name.c_str(),cv_attr_name.c_str(),H5P_DEFAULT);
    if (cv_attr_exist <= 0) {
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr (__FILE__, __LINE__, "The TES swath CV attribute doesn't exist");
    }

    hid_t cv_attr_id = H5Aopen_by_name(file_id,group_name.c_str(),cv_attr_name.c_str(),H5P_DEFAULT,H5P_DEFAULT);
    if (cv_attr_id <0) {
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr (__FILE__, __LINE__, "Cannot obtain the TES CV attribute id");
    }

    hid_t attr_type = -1;
    if ((attr_type = H5Aget_type(cv_attr_id)) < 0) {
        string msg = "cannot get the attribute datatype for the attribute  ";
        msg += cv_attr_name;
        H5Aclose(cv_attr_id);
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    hid_t attr_space = -1;
    if ((attr_space = H5Aget_space(cv_attr_id)) < 0) {
        string msg = "cannot get the hdf5 dataspace id for the attribute ";
        msg += cv_attr_name;
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    int attr_num_elm = 0;
    if (((attr_num_elm = H5Sget_simple_extent_npoints(attr_space)) == 0)) {
        string msg = "cannot get the number for the attribute ";
        msg += cv_attr_name;
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Sclose(attr_space);
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    if (attr_num_elm != (total_num_elm -1)) {
        string msg = "cannot get the number for the attribute ";
        msg += cv_attr_name;
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Sclose(attr_space);
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    if (dtype != H5FLOAT32 || dtype != HDF5CFUtil::H5type_to_H5DAPtype(attr_type)) {
        string msg = "cannot get the number for the attribute ";
        msg += cv_attr_name;
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Sclose(attr_space);
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    hid_t attr_mem_type = -1;
    if ((attr_mem_type = H5Tget_native_type(attr_type,H5T_DIR_ASCEND)) < 0) {
        string msg = "cannot get the attribute datatype for the attribute  ";
        msg += cv_attr_name;
        H5Aclose(cv_attr_id);
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    float *val = NULL;
    float *total_val = NULL;
    float *orig_val = NULL;
    try {
        val = new float[nelms];
        orig_val = new float[total_num_elm-1];
        total_val = new float[total_num_elm];
    }
    catch (...) {
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr (__FILE__, __LINE__,
                          "Cannot allocate the memory for offset,count and setp.");
    }

    if (H5Aread(cv_attr_id,attr_mem_type, (void*)orig_val)<0){
        string msg = "cannot retrieve the value of  the attribute ";
        msg += cv_attr_name;
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Sclose(attr_space);
        H5Fclose(file_id);
        HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
 
#if 0
    float *orig_val_float =(float*)orig_val; 
for (int i =0; i<attr_num_elm; i++)
cerr<<"orig_val "<<orig_val_float[i] <<endl;
#endif
    
    // Panoply cannot accept the same value for the coordinate variable.
    // So I have to create a unique value for the added value. 
    // Since the first data value is 1211.53 hpa, which is below the main sea level
    // already(1013.25 hpa), so I will just use more weight(0.9) for the first data
    // value and less weight(0.1) for the second data value to calculate. 
    // As far as I know, no real data at the added level(fill value -999.0 for data value),
    // it will not affect any visualization results.
    // The position is position of added_value, position of the first_value, position of the second_value.
    // So the equal weight equation is first_value = 0.5*(added_value+second_value);
    // Add the weight, first_value = 0.95*added_value + 0.05*second_value;
    // The approximate formula for the added_value is 
    // add_value = first_value*1.1 -  0.1*second_value;
    // KY 2012-2-20
   
    total_val[0] = 1.1*orig_val[0] - 0.1*orig_val[1];
    for (int i = 1; i < total_num_elm; i++)
        total_val[i] = orig_val[i-1];

#if 0
for (int i = 1; i < total_num_elm; i++)
cerr<<"total_val "<< total_val[i] <<endl;
#endif
 
    for (int i = 0; i <nelms; i++)
        val[i] = total_val[offset[0]+i*step[0]];
#if 0
for (int i = 0; i <nelms; i++)
cerr <<"val "<< val[i] <<endl;
#endif

    set_value((dods_float32*)val, nelms);
    delete []val;
    delete []total_val;
    delete [](float*)orig_val;
    HDF5CFUtil::ClearMem(offset,count,step,NULL,NULL,NULL);
    H5Tclose(attr_type);
    H5Tclose(attr_mem_type);
    H5Aclose(cv_attr_id);
    H5Sclose(attr_space);
    H5Fclose(file_id);

    return false;
}

