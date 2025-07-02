/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Copyright (c) 2005 OPeNDAP, Inc.
// Copyright (c) 2007-2023 The HDF Group, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//         Hyo-Kyung Lee <hyoklee@hdfgroup.org>
//          Kent Yang <myang6@hdfgroup.org>
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

/// \file HDF5Structure.cc
/// \brief The implementation of converting HDF5 compound type into DAP structure for the default option.
/// \author Kent Yang

#include <memory>
#include "hdf5.h"
#include "h5dds.h"
#include "HDF5Structure.h"
#include <libdap/InternalErr.h>
#include <BESInternalError.h>
#include "BESDebug.h"

using namespace std;
using namespace libdap;

BaseType *HDF5Structure::ptr_duplicate()
{
    auto HDF5Structure_unique = make_unique<HDF5Structure>(*this);
    return HDF5Structure_unique.release();
}

HDF5Structure::HDF5Structure(const string & n, const string &vpath, const string &d)
    : Structure(n, d),var_path(vpath)
{
}

HDF5Structure::HDF5Structure(const HDF5Structure &rhs) : Structure(rhs)
{
    m_duplicate(rhs);
}

HDF5Structure & HDF5Structure::operator=(const HDF5Structure & rhs)
{
    if (this == &rhs)
        return *this;

    libdap::Structure::operator=(rhs);
    m_duplicate(rhs);

    return *this;
}

bool HDF5Structure::read()
{

    BESDEBUG("h5",
        ">read() dataset=" << dataset()<<endl);

    if (read_p())
        return true;

    hid_t file_id = H5Fopen(dataset().c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if (file_id < 0) {
        string msg = "Fail to obtain the HDF5 file ID for the file " + dataset() +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
   
    hid_t dset_id = -1;
    if (true == is_dap4())
        dset_id = H5Dopen2(file_id,var_path.c_str(),H5P_DEFAULT);
    else
        dset_id = H5Dopen2(file_id,name().c_str(),H5P_DEFAULT);

    if(dset_id < 0) {
        H5Fclose(file_id);
        string msg = "Fail to obtain the HDF5 dataset ID for the variable " + var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    vector<char> values;
    hid_t dtypeid = H5Dget_type(dset_id);
    if(dtypeid < 0) {
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Fail to obtain the HDF5 data type for the variable " + var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg);
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

void HDF5Structure::do_structure_read(hid_t dsetid, hid_t dtypeid,vector <char> &values,bool has_values, size_t values_offset) {

    hid_t memtype = -1;
    hid_t mspace = -1;

    if ((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND)) < 0) {
        string msg = "Fail to obtain memory datatype for the compound datatype variable " + var_path + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if ((mspace = H5Dget_space(dsetid)) < 0) {
        H5Tclose(memtype);
        string msg = "Fail to obtain the data space for the compound datatype variable " + var_path + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    if (false == has_values) {

        size_t ty_size = H5Tget_size(memtype);
        values.resize(ty_size);

        hid_t read_ret = H5Dread(dsetid, memtype, mspace, mspace, H5P_DEFAULT, (void *) values.data());
        if (read_ret < 0) {
            H5Tclose(memtype);
            H5Sclose(mspace);
            string msg = "Fail to read the HDF5 compound datatype variable " + var_path + ".";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        has_values = true;
    }

    hid_t memb_id = -1;
    H5T_class_t memb_cls = H5T_NO_CLASS;
    int nmembs = 0;
    size_t memb_offset = 0;
    char *memb_name = nullptr;

    try {
        if ((nmembs = H5Tget_nmembers(memtype)) < 0) {
            string msg = "Fail to obtain the number of HDF5 compound datatype members for variable " + var_path + ".";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        for (unsigned int u = 0; u < (unsigned) nmembs; u++) {

            if ((memb_id = H5Tget_member_type(memtype, u)) < 0) {
                string msg = "Fail to obtain the datatype of an HDF5 compound datatype member for variable " + var_path + ".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            // Get member type class 
            memb_cls = H5Tget_member_class(memtype, u);

            // Get member offset
            memb_offset = H5Tget_member_offset(memtype, u);

            // Get member name
            memb_name = H5Tget_member_name(memtype, u);
            if (memb_name == nullptr) {
                string msg = "Fail to obtain the name of an HDF5 compound datatype member for variable " + var_path + ".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            if (memb_cls == H5T_COMPOUND) {
                HDF5Structure &memb_h5s = dynamic_cast<HDF5Structure &>(*var(memb_name));
                memb_h5s.do_structure_read(dsetid, memb_id, values, has_values, memb_offset + values_offset);
            } else if (memb_cls == H5T_ARRAY) {

                // memb_id, obtain the number of dimensions
                int at_ndims = H5Tget_array_ndims(memb_id);
                if (at_ndims <= 0) {
                    string msg = "Fail to obtain number of dimensions of the array datatype for variable " + var_path + ".";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                HDF5Array &h5_array_type = dynamic_cast<HDF5Array &>(*var(memb_name));
                vector<int64_t> at_offset(at_ndims, 0);
                vector<int64_t> at_count(at_ndims, 0);
                vector<int64_t> at_step(at_ndims, 0);

                int64_t at_nelms = h5_array_type.format_constraint(at_offset.data(), at_step.data(), at_count.data());

                // Read the array data
                h5_array_type.do_h5_array_type_read(dsetid, memb_id, values, has_values, memb_offset + values_offset,
                                                    at_nelms, at_offset.data(), at_count.data(), at_step.data());

            } else if (memb_cls == H5T_INTEGER || memb_cls == H5T_FLOAT || memb_cls == H5T_STRING) {
                do_structure_read_atomic(memb_id, memb_name, memb_cls, values, values_offset, memb_offset);
            } else {
                free(memb_name);
                H5Tclose(memb_id);
                string msg = "Only support the field of compound datatype when the field type class is integer, float, string, array or compound.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            // Close member type ID 
            H5Tclose(memb_id);
            var(memb_name)->set_read_p(true);
            free(memb_name);
        } // end for 

    }

    catch (...) {
         catch_free(memb_name, memb_id, memtype, mspace, values);
        throw;
    }

    // H5Dvlen_reclaim  only applies to vlen data. Maybe a compound datatype may include variable length string.
    // Since it doesn't cause any error. So leave it here and observe. KY 2025-06-26
    if (H5Dvlen_reclaim(memtype, mspace, H5P_DEFAULT, (void *) values.data()) < 0) {
        string msg = "Unable to reclaim the memory of storing  compound datatype array. ";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    H5Tclose(memtype);
    H5Sclose(mspace);
}

void HDF5Structure::do_structure_read_atomic(hid_t memb_id, char *memb_name, H5T_class_t memb_cls,
                                             const vector<char> &values, size_t values_offset,
                                             size_t memb_offset) {


    if (memb_cls == H5T_INTEGER || memb_cls == H5T_FLOAT) {
        void *src = (void *) (values.data() + values_offset + memb_offset);

        if (true == promote_char_to_short(memb_cls, memb_id) && is_dap4() == false) {
            char val_int8;
            memcpy(&val_int8, src, 1);
            auto val_short = (short) val_int8;
            var(memb_name)->val2buf(&val_short);
        } else {
            var(memb_name)->val2buf(src);
        }
    } else if (memb_cls == H5T_STRING) {
        do_structure_read_string(memb_id, memb_name, values, values_offset, memb_offset);

    }
}

void HDF5Structure::do_structure_read_string(hid_t memb_id, char *memb_name,
                                             const vector<char> &values, size_t values_offset,
                                             size_t memb_offset) {

    void *src = (void *) (values.data() + values_offset + memb_offset);

    // distinguish between variable length and fixed length
    if (true == H5Tis_variable_str(memb_id)) {

        auto temp_bp = (char *) src;
        string final_str;
        get_vlen_str_data(temp_bp, final_str);
        var(memb_name)->val2buf((void *) &final_str);

    } else {// Obtain string

        vector<char> str_val;
        size_t memb_size = H5Tget_size(memb_id);
        if (memb_size == 0) {
            H5Tclose(memb_id);
            free(memb_name);
            string msg = "Fail to obtain the size of HDF5 compound datatype.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
        str_val.resize(memb_size);
        memcpy(str_val.data(), src, memb_size);
        string temp_string(str_val.begin(), str_val.end());
        var(memb_name)->val2buf(&temp_string);

    }
}

void HDF5Structure::catch_free(char *memb_name, hid_t memb_id, hid_t memtype, hid_t mspace, vector<char>&values) const {

    if ((memtype != -1) && (mspace != -1) && (H5Dvlen_reclaim(memtype, mspace,
                                                              H5P_DEFAULT, (void *) values.data()) < 0)) {
        string msg = "Unable to reclaim the compound datatype array.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    if (memtype != -1)
        H5Tclose(memtype);
    if (mspace != -1)
        H5Sclose(mspace);

    if (memb_id != -1)
        H5Tclose(memb_id);

    if (memb_name != nullptr)
        free(memb_name);
}
