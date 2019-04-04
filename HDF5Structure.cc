/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Copyright (c) 2005 OPeNDAP, Inc.
// Copyright (c) 2007-2016 The HDF Group, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//         Hyo-Kyung Lee <hyoklee@hdfgroup.org>
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
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

// #define DODS_DEBUG
/// \file HDF5Structure.cc
/// \brief The implementation of converting HDF5 compound type into DAP structure for the default option.
/// \author Kent Yang


#include <string>
#include <ctype.h>
#include "config_hdf5.h"
#include "hdf5.h"
#include "h5dds.h"
#include "HDF5Structure.h"
#include "InternalErr.h"
#include "BESDebug.h"

using namespace std;
using namespace libdap;

BaseType *HDF5Structure::ptr_duplicate()
{
    return new HDF5Structure(*this);
}

HDF5Structure::HDF5Structure(const string & n, const string &vpath, const string &d)
    : Structure(n, d)
{
    var_path = vpath;
}

HDF5Structure::~HDF5Structure()
{
}
HDF5Structure::HDF5Structure(const HDF5Structure &rhs) : Structure(rhs)
{
}

HDF5Structure & HDF5Structure::operator=(const HDF5Structure & rhs)
{
    if (this == &rhs)
        return *this;

    //static_cast < Structure & >(*this) = rhs;  // run Structure assignment
    dynamic_cast < Structure & >(*this) = rhs;  // run Structure assignment


    return *this;
}

bool HDF5Structure::read()
{

    BESDEBUG("h5",
        ">read() dataset=" << dataset()<<endl);

    if (read_p())
        return true;

    hid_t file_id = H5Fopen(dataset().c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if(file_id < 0) {
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the HDF5 file ID .");
    }
   
    hid_t dset_id = -1;
    if(true == is_dap4())
        dset_id = H5Dopen2(file_id,var_path.c_str(),H5P_DEFAULT);
    else
        dset_id = H5Dopen2(file_id,name().c_str(),H5P_DEFAULT);

    if(dset_id < 0) {
        H5Fclose(file_id);
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the datatype .");
    }
    vector<char> values;
    hid_t dtypeid = H5Dget_type(dset_id);
    if(dtypeid < 0) {
        H5Dclose(dset_id);
        H5Fclose(file_id);
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the datatype .");
    }
    try {
        do_structure_read(dset_id,dtypeid,values,false,0);
    }
    catch(...) {
        H5Tclose(dtypeid);
        H5Dclose(dset_id);
        H5Fclose(file_id);
         throw;
    }
    set_read_p(true);

    H5Tclose(dtypeid);
    H5Dclose(dset_id);
    H5Fclose(file_id);

    return true;
}

void HDF5Structure::do_structure_read(hid_t dsetid, hid_t dtypeid,vector <char> &values,bool has_values, int values_offset) {

    hid_t memtype = -1;
    hid_t mspace  = -1;

    if ((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND))<0) {
        throw InternalErr (__FILE__, __LINE__, "Fail to obtain memory datatype.");
    }

    if(false == has_values) {

        if((mspace = H5Dget_space(dsetid))<0) {
            throw InternalErr (__FILE__, __LINE__, "Fail to obtain memory datatype.");
        }

        size_t ty_size = H5Tget_size(memtype);
        if (ty_size == 0) {
            H5Tclose(memtype);
            throw InternalErr (__FILE__, __LINE__,"Fail to obtain the size of HDF5 compound datatype.");
        }

        values.resize(ty_size);
        hid_t read_ret = -1;
        read_ret = H5Dread(dsetid,memtype,mspace,mspace,H5P_DEFAULT,(void*)&values[0]);
        if (read_ret < 0) {
            H5Tclose(memtype);
            throw InternalErr (__FILE__, __LINE__, "Fail to read the HDF5 compound datatype dataset.");
        }

        has_values = true;
    }

    hid_t               memb_id = -1;      
    H5T_class_t         memb_cls = H5T_NO_CLASS;
    int                 nmembs = 0;
    size_t              memb_offset = 0;
    unsigned int        u = 0;
    char*               memb_name = NULL;

    try {
        if((nmembs = H5Tget_nmembers(memtype)) < 0) {
            throw InternalErr (__FILE__, __LINE__, "Fail to obtain number of HDF5 compound datatype.");
        }

        for(u = 0; u < (unsigned)nmembs; u++) {

            if((memb_id = H5Tget_member_type(memtype, u)) < 0) 
                throw InternalErr (__FILE__, __LINE__, "Fail to obtain the datatype of an HDF5 compound datatype member.");

            // Get member type class 
            if((memb_cls = H5Tget_member_class (memtype, u)) < 0) 
                throw InternalErr (__FILE__, __LINE__, "Fail to obtain the datatype class of an HDF5 compound datatype member.");

            // Get member offset
            memb_offset= H5Tget_member_offset(memtype,u);

            // Get member name
            memb_name = H5Tget_member_name(memtype,u);
            if(memb_name == NULL) 
                throw InternalErr (__FILE__, __LINE__, "Fail to obtain the name of an HDF5 compound datatype member.");

            if (memb_cls == H5T_COMPOUND) {  
                HDF5Structure &memb_h5s = dynamic_cast<HDF5Structure&>(*var(memb_name));
                memb_h5s.do_structure_read(dsetid,memb_id,values,has_values,memb_offset+values_offset);
            }
            else if(memb_cls == H5T_ARRAY) {

                // memb_id, obtain the number of dimensions
                int at_ndims = H5Tget_array_ndims(memb_id);
                if(at_ndims <= 0)  
                    throw InternalErr (__FILE__, __LINE__, "Fail to obtain number of dimensions of the array datatype.");

                HDF5Array &h5_array_type = dynamic_cast<HDF5Array&>(*var(memb_name));
                vector<int> at_offset(at_ndims,0);
                vector<int> at_count(at_ndims,0);
                vector<int> at_step(at_ndims,0);

                int at_nelms = h5_array_type.format_constraint(&at_offset[0],&at_step[0],&at_count[0]);

                // Read the array data
                h5_array_type.do_h5_array_type_read(dsetid, memb_id,values,has_values,memb_offset+values_offset,
                                                    at_nelms,&at_offset[0],&at_count[0],&at_step[0]);

            }
            else if(memb_cls == H5T_INTEGER || memb_cls == H5T_FLOAT) {
                if(true == promote_char_to_short(memb_cls,memb_id)) {
                    void *src = (void*)(&values[0] + values_offset +memb_offset);
                    char val_int8;
                    memcpy(&val_int8,src,1);
                    short val_short=(short)val_int8;
                    var(memb_name)->val2buf(&val_short);
                }
                else {
                    var(memb_name)->val2buf(&values[0] +  values_offset +memb_offset);

                }
            }
            else if(memb_cls == H5T_STRING) {

                // distinguish between variable length and fixed length
                if(true == H5Tis_variable_str(memb_id)) {

                    void *src = (void*)(&values[0]+values_offset + memb_offset);
                    char*temp_bp = (char*)src;
                    string final_str ="";
                    get_vlen_str_data(temp_bp,final_str);
                    var(memb_name)->val2buf((void*)&final_str);

                }
                else {// Obtain string

                    void *src = (void*)(&values[0]+values_offset + memb_offset);
                    vector<char> str_val;
                    size_t memb_size = H5Tget_size(memb_id);
                    if (memb_size == 0) {
                        H5Tclose(memb_id);
                        free(memb_name);
                        throw InternalErr (__FILE__, __LINE__,"Fail to obtain the size of HDF5 compound datatype.");
                    }
                    str_val.resize(memb_size);
                    memcpy(&str_val[0],src,memb_size);
                    string temp_string(str_val.begin(),str_val.end());
                    var(memb_name)->val2buf(&temp_string);
// This doesn't work either.                var(memb_name)->val2buf(&str_val[0]);
                    
                // We may just pass the string, (maybe string pad is preserved.)
                // Probably not, DAP string may not keep the size. This doesn't work.
                //var(memb_name)->val2buf(&values[0]+value_offset + memb_offset);
                }
            }
            else {
                free(memb_name);
                H5Tclose(memb_id);
                throw InternalErr (__FILE__, __LINE__, 
                         "Only support the field of compound datatype when the field type class is integer, float, string, array or compound..");

            }
            // Close member type ID 
            H5Tclose(memb_id);
            var(memb_name)->set_read_p(true);
            free(memb_name);
        } // end for 

    }

    catch(...) {
        if((memtype != -1) && (mspace !=-1)) {
         if(H5Dvlen_reclaim(memtype,mspace,H5P_DEFAULT,(void*)&values[0])<0)
            throw InternalErr(__FILE__, __LINE__, "Unable to reclaim the compound datatype array.");
        }
        if(memtype != -1)
            H5Tclose(memtype);
        if(mspace != -1)
            H5Sclose(mspace);

        if(memb_id != -1)
            H5Tclose(memb_id);

        if(memb_name != NULL)
            free(memb_name);
        throw;
    }

    if((memtype != -1) && (mspace !=-1)) {
         if(H5Dvlen_reclaim(memtype,mspace,H5P_DEFAULT,(void*)&values[0])<0)
            throw InternalErr(__FILE__, __LINE__, "Unable to reclaim the compound datatype array.");
    }
    if(memtype != -1)
        H5Tclose(memtype);
    if(mspace != -1)
        H5Sclose(mspace);
}

