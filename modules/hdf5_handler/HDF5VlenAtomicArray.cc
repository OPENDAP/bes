// This file is part of the hdf5_handler implementing for the default option
// Copyright (c) The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file HDF5VlenAtomicArray.cc
/// \brief The implementation of the retrieval of data values for Vlen integer and float array data.
/// Currently this only applies to ACOS level 2 and OCO2 level 1B data.
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) The HDF Group
///
/// All rights reserved.

#include <iostream>
#include <memory>
#include <cassert>
#include <BESDebug.h>
#include <libdap/InternalErr.h>

#include "HDF5RequestHandler.h"
#include "HDF5VlenAtomicArray.h"

using namespace std;
using namespace libdap;

BaseType *HDF5VlenAtomicArray::ptr_duplicate()
{
    auto HDF5VlenAtomicArray_unique = make_unique<HDF5VlenAtomicArray>(*this);
    return HDF5VlenAtomicArray_unique.release();
}

HDF5VlenAtomicArray::HDF5VlenAtomicArray(const string & n, const string &d, BaseType * v, bool vlen_index) :
    HDF5Array(n, d, v),is_vlen_index(vlen_index) {
}

bool HDF5VlenAtomicArray::read()
{
    BESDEBUG("h5","Coming to HDF5VlenAtomicArray read "<<endl);

    read_vlen_internal(is_vlen_index);
    
    return true;
}

void HDF5VlenAtomicArray::read_vlen_internal(bool vlen_index) {

    hid_t file_id = H5Fopen(dataset().c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if (file_id <0) {
        string msg = "Fail to obtain the HDF5 file ID for the file " + dataset() +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    string vlen_var_path = get_VarPath();
    BESDEBUG("h5","variable name is "<<name() <<endl);
    BESDEBUG("h5","variable path is  "<<vlen_var_path <<endl);

    hid_t dset_id = H5Dopen2(file_id,vlen_var_path.c_str(),H5P_DEFAULT);
    if (dset_id < 0) {
        H5Fclose(file_id);
        string msg = "Fail to open the vlen variable " + vlen_var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    hid_t vlen_type = H5Dget_type(dset_id);
    if (vlen_type <0) {
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Fail to obtain the vlen data type for variable " + vlen_var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
    hid_t vlen_basetype = H5Tget_super(vlen_type);
    if (vlen_basetype <0) {
        H5Tclose(vlen_type);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Fail to obtain the vlen base data type for variable " + vlen_var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    if (H5Tget_class(vlen_basetype) != H5T_INTEGER && H5Tget_class(vlen_basetype) != H5T_FLOAT) {
        H5Tclose(vlen_basetype);
        H5Tclose(vlen_type);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Only support float or intger variable-length datatype, the variable name is " + vlen_var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    hid_t vlen_base_memtype = H5Tget_native_type(vlen_basetype, H5T_DIR_ASCEND); 
    if (vlen_base_memtype <0) {
        H5Tclose(vlen_basetype);
        H5Tclose(vlen_type);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Fail to obtain the vlen memory base data type for the variable " + vlen_var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
 
    hid_t vlen_memtype = H5Tvlen_create(vlen_base_memtype);
    if (vlen_memtype <0) {
        H5Tclose(vlen_base_memtype);
        H5Tclose(vlen_basetype);
        H5Tclose(vlen_type);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Fail to create the vlen memory data type for the variable " + vlen_var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
 
    // Will not support the scalar type.
    hid_t vlen_space = H5Dget_space(dset_id);
    if (vlen_space <0) {
        H5Tclose(vlen_base_memtype);
        H5Tclose(vlen_memtype);
        H5Tclose(vlen_basetype);
        H5Tclose(vlen_type);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Fail to obtain the vlen data space ID for the variable " + vlen_var_path +".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    if (H5Sget_simple_extent_type(vlen_space) != H5S_SIMPLE) {
        H5Sclose(vlen_space);
        H5Tclose(vlen_base_memtype);
        H5Tclose(vlen_memtype);
        H5Tclose(vlen_basetype);
        H5Tclose(vlen_type);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Only support array of float or intger variable-length datatype,";
        msg += "the vlen variable name is " + vlen_var_path + ".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Obtain the vlen counts, offsets and steps.

    hssize_t vlen_num_elems = 1;
    unsigned int num_dims = 0;
    vector<hsize_t> vlen_count;
    vector<hsize_t> vlen_step;
    vector<hsize_t> vlen_offset;

    if (vlen_index) {
        num_dims = this->dimensions();
        vector<int64_t> offset(num_dims);
        vector<int64_t> count(num_dims);
        vector<int64_t> step(num_dims);
        vlen_count.resize(num_dims);
        vlen_step.resize(num_dims);
        vlen_offset.resize(num_dims);
        vlen_num_elems = (hssize_t)(format_constraint(offset.data(), step.data(), count.data())); 
        for (int i = 0; i <num_dims; i++) {
            vlen_count[i] = (hsize_t)(count[i]);
            vlen_offset[i] = (hsize_t)(offset[i]);
            vlen_step[i] = (hsize_t)(step[i]);
        }
    }
    else {
        num_dims = this->dimensions()-1;
        if (num_dims ==0) {
            H5Sclose(vlen_space);
            H5Tclose(vlen_base_memtype);
            H5Tclose(vlen_memtype);
            H5Tclose(vlen_basetype);
            H5Tclose(vlen_type);
            H5Dclose(dset_id);
            H5Fclose(file_id);
            string msg = "This variable is a variable-length array, the number of dimensions for DAP4 representation must be greater than 1.";
            msg += "The variable name is " + vlen_var_path + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        vector<int64_t> offset(num_dims+1);
        vector<int64_t> count(num_dims+1);
        vector<int64_t> step(num_dims+1);
        format_constraint(offset.data(), step.data(), count.data()); 
    
        libdap::Array::Dim_iter last_dim_iter = this->dim_end()-1;
        int64_t last_dim_size = this->dimension_size(last_dim_iter);

        // Note: since the last dimension is added by us and 
        // the size is the maximum length of the variable length type elements in this array.
        // we fill zero after the real data in the last dimension. So the last dimension
        // should not be subset like other variables. 
        if (count.back() != last_dim_size) {
            H5Sclose(vlen_space);
            H5Tclose(vlen_base_memtype);
            H5Tclose(vlen_memtype);
            H5Tclose(vlen_basetype);
            H5Tclose(vlen_type);
            H5Dclose(dset_id);
            H5Fclose(file_id);
            string msg = "This variable is a variable-length array, the last dimension in the DAP4 representation cannot be subset.";
            msg += "The variable name is " + vlen_var_path + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        vlen_count.resize(num_dims);
        vlen_step.resize(num_dims);
        vlen_offset.resize(num_dims);
    
        for (int i = 0; i <num_dims; i++) {
            vlen_count[i] = (hsize_t)(count[i]);
            vlen_offset[i] = (hsize_t)(offset[i]);
            vlen_step[i] = (hsize_t)(step[i]);
            vlen_num_elems *=vlen_count[i];
        }
    }

    BESDEBUG("h5","vlen_count[0]: "<<vlen_count[0] <<endl);
    BESDEBUG("h5","vlen_offset[0]: "<<vlen_offset[0] <<endl);
    BESDEBUG("h5","vlen_step[0]: "<<vlen_step[0] <<endl);
    BESDEBUG("h5","num_dims: "<<num_dims <<endl);
    BESDEBUG("h5","vlen_num_elems: "<<vlen_num_elems <<endl);
    
    if (H5Sselect_hyperslab(vlen_space, H5S_SELECT_SET,
                            vlen_offset.data(), vlen_step.data(),
                            vlen_count.data(), nullptr) < 0) {
        H5Tclose(vlen_base_memtype);
        H5Tclose(vlen_memtype);
        H5Sclose(vlen_space);
        H5Tclose(vlen_basetype);
        H5Tclose(vlen_type);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Could not select hyperslab for the vlen variable " + vlen_var_path + ".";
        throw InternalErr(__FILE__, __LINE__, msg);
    } 
 
    hid_t memspace = H5Screate_simple(num_dims,vlen_count.data(),nullptr);
    if (memspace < 0) {
        H5Tclose(vlen_base_memtype);
        H5Tclose(vlen_memtype);
        H5Sclose(vlen_space);
        H5Tclose(vlen_basetype);
        H5Tclose(vlen_type);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Could not create memory space  for the vlen variable " + vlen_var_path + ".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    vector<hvl_t> vlen_data(vlen_num_elems);
    if (H5Dread(dset_id, vlen_memtype, memspace, vlen_space, H5P_DEFAULT, vlen_data.data()) <0) {
        H5Tclose(vlen_base_memtype);
        H5Tclose(vlen_memtype);
        H5Sclose(vlen_space);
        H5Sclose(memspace);
        H5Tclose(vlen_basetype);
        H5Tclose(vlen_type);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Could not read the data for the vlen variable " + vlen_var_path + ".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Handle vlen and vlen_index here.
    if (vlen_index) {
        if (this->var()->type() != dods_int32_c) { 
            H5Tclose(vlen_base_memtype);
            H5Tclose(vlen_memtype);
            H5Sclose(vlen_space);
            H5Sclose(memspace);
            H5Tclose(vlen_basetype);
            H5Tclose(vlen_type);
            H5Dclose(dset_id); 
            H5Fclose(file_id);
            string msg = "vlen_index datatype must be 32-bit integer."; 
            throw InternalErr( __FILE__, __LINE__, msg); 
        } 
        vector<int> vlen_index_data; 
        for (ssize_t i = 0; i<vlen_num_elems; i++)  
            vlen_index_data.push_back(vlen_data[i].len); 
        set_value_ll(vlen_index_data.data(),vlen_num_elems); 
    }
    else {

        switch (this->var()->type()) {
            case dods_byte_c:
            case dods_uint8_c:
            case dods_char_c:
            case dods_int8_c:
            case dods_int16_c:
            case dods_uint16_c:
            case dods_int32_c:
            case dods_uint32_c:
            case dods_int64_c:
            case dods_uint64_c:
            case dods_float32_c:
            case dods_float64_c: {

                // Retrieve the last dimension size.
                libdap::Array::Dim_iter last_dim_iter = this->dim_end()-1;
                int64_t last_dim_size = this->dimension_size(last_dim_iter);
                size_t bytes_per_element = this->var()->width_ll();
                size_t total_data_buf_size = vlen_num_elems*last_dim_size*bytes_per_element;

                vector<char> data_buf(total_data_buf_size,0);
                char *temp_data_buf_ptr = data_buf.data();

                for (ssize_t i = 0; i < vlen_num_elems; i++) {

                    size_t vlen_element_size = vlen_data[i].len * bytes_per_element;

                    // Copy the vlen data to the data buffer.
                    memcpy(temp_data_buf_ptr,vlen_data[i].p,vlen_element_size);

                    // Move the data buffer pointer to the next element.
                    // In this regular array, the rest data will be filled with zero.
                    temp_data_buf_ptr += last_dim_size*bytes_per_element;

                }
                val2buf(data_buf.data());

                break;
            }
            default: {
                H5Tclose(vlen_base_memtype);
                H5Tclose(vlen_memtype);
                H5Sclose(vlen_space);
                H5Sclose(memspace);
                H5Tclose(vlen_basetype);
                H5Tclose(vlen_type);
                H5Dclose(dset_id); 
                H5Fclose(file_id);
                string msg = "Vector::val2buf: bad type.";
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
    }
    if(H5Dvlen_reclaim(vlen_memtype, memspace, H5P_DEFAULT, (void*)(vlen_data.data())) <0) {
        H5Tclose(vlen_base_memtype);
        H5Tclose(vlen_memtype);
        H5Sclose(vlen_space);
        H5Sclose(memspace);
        H5Tclose(vlen_basetype);
        H5Tclose(vlen_type);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "H5Dvlen_reclaim failed.";
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    H5Sclose(vlen_space);
    H5Sclose(memspace);
    H5Tclose(vlen_base_memtype);
    H5Tclose(vlen_basetype);
    H5Tclose(vlen_type);
    H5Tclose(vlen_memtype);
    H5Dclose(dset_id);
    H5Fclose(file_id);

    set_read_p(true);
    
}
