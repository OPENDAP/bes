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
/// \file HDF5GMSPCFArray.cc
/// \brief The implementation of the retrieval of data values for special  HDF5 products
/// Currently this only applies to ACOS level 2 data.
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

#include "HDF5GMSPCFArray.h"

BaseType *HDF5GMSPCFArray::ptr_duplicate()
{
    return new HDF5GMSPCFArray(*this);
}

bool HDF5GMSPCFArray::read()
{

    vector<int>offset;
    vector<int>count;
    vector<int>step;
    vector<hsize_t>hoffset;
    vector<hsize_t>hcount;
    vector<hsize_t>hstep;

    int nelms = 0;

    if((otype != H5INT64 && otype !=H5UINT64) 
       || (dtype !=H5INT32)) 
        throw InternalErr (__FILE__, __LINE__,
                          "The datatype of the special product is not right.");

    if (rank < 0) 
        throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension of the variable is negative.");

    else if (rank == 0) 
        nelms = 1;
    else {

        offset.resize(rank);
        count.resize(rank);
        step.resize(rank);
        hoffset.resize(rank);
        hcount.resize(rank);
        hstep.resize(rank);
 

        nelms = format_constraint (&offset[0], &step[0], &count[0]);

        for (int i = 0; i <rank; i++) {
            hoffset[i] = (hsize_t) offset[i];
            hcount[i] = (hsize_t) count[i];
            hstep[i] = (hsize_t) step[i];
        }
    }

//    hid_t fileid = -1;
    hid_t dsetid = -1;
    hid_t dspace = -1;
    hid_t mspace = -1;
    hid_t dtypeid = -1;
    hid_t memtype = -1;

#if 0
    if ((fileid = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {

        ostringstream eherr;
        eherr << "HDF5 File " << filename 
              << " cannot be opened. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }
#endif

    if ((dsetid = H5Dopen(fileid,varname.c_str(),H5P_DEFAULT))<0) {

        //H5Fclose(fileid);
        ostringstream eherr;
        eherr << "HDF5 dataset " << varname
              << " cannot be opened. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if ((dspace = H5Dget_space(dsetid))<0) {

        H5Dclose(dsetid);
        //H5Fclose(fileid);
        ostringstream eherr;
        eherr << "Space id of the HDF5 dataset " << varname
              << " cannot be obtained. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    if (rank > 0) {
        if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET,
                           &hoffset[0], &hstep[0],
                           &hcount[0], NULL) < 0) {

            H5Sclose(dspace);
            H5Dclose(dsetid);
            //H5Fclose(fileid);
            ostringstream eherr;
            eherr << "The selection of hyperslab of the HDF5 dataset " << varname
                  << " fails. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        mspace = H5Screate_simple(rank, (const hsize_t*)&hcount[0],NULL);
        if (mspace < 0) {
            H5Sclose(dspace);
            H5Dclose(dsetid);
            //H5Fclose(fileid);
            ostringstream eherr;
            eherr << "The creation of the memory space of the  HDF5 dataset " << varname
                  << " fails. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

    }


    if ((dtypeid = H5Dget_type(dsetid)) < 0) {
        if (rank >0) 
            H5Sclose(mspace);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        //H5Fclose(fileid);
        ostringstream eherr;
        eherr << "Obtaining the datatype of the HDF5 dataset " << varname
              << " fails. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if ((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND))<0) {

        if (rank >0) 
            H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        //H5Fclose(fileid);
        ostringstream eherr;
        eherr << "Obtaining the memory type of the HDF5 dataset " << varname
              << " fails. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());

    }

    H5T_class_t ty_class = H5Tget_class(dtypeid);
    if (ty_class < 0) {

        if (rank >0) 
            H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Tclose(memtype);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        //H5Fclose(fileid);
        ostringstream eherr;
        eherr << "Obtaining the type class of the HDF5 dataset " << varname
              << " fails. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());

    }

    if (ty_class !=H5T_INTEGER) {
        if (rank >0) 
            H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Tclose(memtype);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        //H5Fclose(fileid);
        ostringstream eherr;
        eherr << "The type class of the HDF5 dataset " << varname
              << " is not H5T_INTEGER. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    size_t ty_size = H5Tget_size(dtypeid);
    if (ty_size != H5Tget_size(H5T_STD_I64LE)) {
        if (rank >0) 
            H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Tclose(memtype);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        //H5Fclose(fileid);
        ostringstream eherr;
        eherr << "The type size of the HDF5 dataset " << varname
              << " is not right. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    hid_t read_ret = -1;


    vector<long long>orig_val;
    orig_val.resize(nelms);

    vector<int> val;
    val.resize(nelms);
 
    if (0 == rank) 
        read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&orig_val[0]);
    else 
        read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&orig_val[0]);

    if (read_ret < 0) {
        if (rank >0) 
            H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Tclose(memtype);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        //H5Fclose(fileid);
        ostringstream eherr;
        eherr << "Cannot read the HDF5 dataset " << varname
              <<" with type of 64-bit integer"<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Create "Time" or "Date" part of the original array. 
    // First get 10 power number of bits right
    int max_num = 1;
    for (int i = 0; i <numofdbits; i++) 
        max_num = 10 * max_num;

    int num_cut = 1;
    for (int i = 0; i<(sdbit-1) ; i++) 
        num_cut = 10 *num_cut;
    
    // Second generate the number based on the starting bit and number of bits
    // For example, number 1234, starting bit is 1, num of bits is 2

    // The final number is 34. The formula is
    //  (orig_val/pow(sbit-1)))%(pow(10,nbits))
    // In this example, 34 = (1234/1)%(100) = 34

    for (int i = 0; i <nelms; i ++) 
        val[i] = (orig_val[i]/num_cut)%max_num;


    set_value ((dods_int32 *)&val[0],nelms);
       
    if (rank >0) 
        H5Sclose(mspace);
    H5Tclose(dtypeid);
    H5Tclose(memtype);
    H5Sclose(dspace);
    H5Dclose(dsetid);
    //H5Fclose(fileid);
    
    return true;
}

        
// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDF5GMSPCFArray::format_constraint (int *offset, int *step, int *count)
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
