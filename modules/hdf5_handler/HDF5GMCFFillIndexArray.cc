// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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

////////////////////////////////////////////////////////////////////////////////
/// \file HDF5GMCFFillIndexArray.cc
/// \brief This class includes the methods to read data array into DAP buffer from an HDF5 dataset for the CF option.
/// \brief It is implemented to address the netCDF dimension without the dimensional scale. Since the HDF5 default fill value
/// \brief (0 for most platforms) fills in this dimension dataset, this will cause confusions for users.
/// \brief So the handler will fill in index number(0,1,2,...) just as the handling of missing coordinate variables.
///

/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#include "config_hdf5.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <BESDebug.h>
#include <libdap/InternalErr.h>

#include <libdap/Str.h>
#include "HDF5GMCFFillIndexArray.h"

using namespace std;
using namespace libdap;

BaseType *HDF5GMCFFillIndexArray::ptr_duplicate()
{
    return new HDF5GMCFFillIndexArray(*this);
}

// Read in an HDF5 Array 
bool HDF5GMCFFillIndexArray::read()
{

    BESDEBUG("h5","Coming to HDF5GMCFFillIndexArray read "<<endl);

    read_data_NOT_from_mem_cache(false,nullptr);

    return true;
}


void HDF5GMCFFillIndexArray::read_data_NOT_from_mem_cache(bool /*add_cache*/,void*/*buf*/) {


    BESDEBUG("h5","Coming to HDF5GMCFFillIndexArray: read_data_NOT_from_mem_cache"<<endl);


    int nelms = 0;

#if 0
cerr<<"coming to read function"<<endl;
cerr<<"file name " <<filename <<endl;
"h5","var name "<<varname <<endl;
#endif

    if (rank != 1) 
        throw InternalErr (__FILE__, __LINE__,
                          "Currently the rank of the dimension scale must be 1.");

    vector<int> offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    // Obtain the number of the subsetted elements
    nelms = format_constraint (offset.data(), step.data(), count.data());


    switch (dtype) {

        case H5UCHAR:
                
        {
            vector<unsigned char> val;
            val.resize(nelms);

            for (int i = 0; i < count[0]; i++)
                val[i] = (unsigned char)(offset[0] + step[0] * i);

            set_value ((dods_byte *) val.data(), nelms);
        } // case H5UCHAR
            break;


        // signed char maps to 16-bit integer in DAP2(HDF5 to DAP2 mapping document.)
        case H5CHAR:
        {
            if(is_dap4 == false) {

                vector<short>val;
                val.resize(nelms);

                for (int i = 0; i < count[0]; i++)
                    val[i] = (short)(offset[0] + step[0] * i);
               
                set_value ((dods_int16 *) val.data(), nelms);
            }
            else {

                vector<char> val;
                val.resize(nelms);

                for (int i = 0; i < count[0]; i++)
                    val[i] = (char)(offset[0] + step[0] * i);

                set_value ((dods_int8 *) val.data(), nelms);

            }
        }// H5CHAR and H5INT16
            break;

    

        case H5INT16:
        {

            vector<short>val;
            val.resize(nelms);

            for (int i = 0; i < count[0]; i++)
                val[i] = (short)(offset[0] + step[0] * i);
               
            set_value ((dods_int16 *) val.data(), nelms);
        }// H5CHAR and H5INT16
            break;


        case H5UINT16:
        {
            vector<unsigned short> val;
            val.resize(nelms);

            for (int i = 0; i < count[0]; i++)
                val[i] = (unsigned short)(offset[0] + step[0] * i);
                
            set_value ((dods_uint16 *) val.data(), nelms);
        } // H5UINT16
            break;


        case H5INT32:
        {
            vector<int>val;
            val.resize(nelms);

            for (int i = 0; i < count[0]; i++)
                val[i] = offset[0] + step[0] * i;

            set_value ((dods_int32 *) val.data(), nelms);
        } // case H5INT32
            break;

        case H5UINT32:
        {
            vector<unsigned int>val;
            val.resize(nelms);

            for (int i = 0; i < count[0]; i++)
                val[i] = offset[0] + step[0] * i;

            set_value ((dods_uint32 *) val.data(), nelms);
        }
            break;

        case H5INT64:
        {
            vector<long long>val;
            val.resize(nelms);

            for (int i = 0; i < count[0]; i++)
                val[i] = offset[0] + step[0] * i;

            set_value ((dods_int64 *) val.data(), nelms);
        } // case H5INT64
            break;

        case H5UINT64:
        {
            vector<unsigned long long>val;
            val.resize(nelms);

            for (int i = 0; i < count[0]; i++)
                val[i] = offset[0] + step[0] * i;

            set_value ((dods_uint64 *) val.data(), nelms);
        }
            break;

        case H5FLOAT32:
        {

            vector<float>val;
            val.resize(nelms);

            for (int i = 0; i < count[0]; i++)
                val[i] = (float)(offset[0] + step[0] * i);

            set_value ((dods_float32 *) val.data(), nelms);
        }
            break;


        case H5FLOAT64:
        {

            vector<double>val;
            val.resize(nelms);

            for (int i = 0; i < count[0]; i++)
                val[i] = offset[0] + step[0] * i;

            set_value ((dods_float64 *) val.data(), nelms);
        } // case H5FLOAT64
            break;


        case H5FSTRING:
        case H5VSTRING:
        default:
        {
            ostringstream eherr;
            eherr << "Currently the dimension scale datatype cannot be string"<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

    }


    return;
}
