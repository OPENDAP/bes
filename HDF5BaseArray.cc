// This file is part of hdf5_handler an HDF5 file handler for the OPeNDAP
// data server.

// Author: Muqun Yang <myang6@hdfgroup.org> 

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
///////////////////////////////////////////////////////////////////////////////
/// \file HDF5BaseArray.cc
/// \brief Implementation of a helper class that aims to reduce code redundence for different special CF derived array class
/// For example, format_constraint has been called by different CF derived array class,
/// and write_nature_number_buffer has also be used by missing variables on both 
/// HDF-EOS5 and generic HDF5 products. 
/// 
/// This class converts HDF5 array type into DAP array.
/// 
/// \author Kent Yang       (myang6@hdfgroup.org)
/// Copyright (c) 2011-2016 The HDF Group
/// 
/// All rights reserved.
///
/// 
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <cassert>
#include <BESDebug.h>
#include "InternalErr.h"

#include "HDF5BaseArray.h"


#if 0
BaseType *HDF5BaseArray::ptr_duplicate()
{
    return new HDF5BaseArray(*this);
}

// Always return true. 
// Data will be read from the missing coordinate variable class(HDF5GMCFMissNonLLCVArray etc.)
bool HDF5BaseArray::read()
{
    BESDEBUG("h5","Coming to HDF5BaseArray read "<<endl);
    return true;
}

#endif

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDF5BaseArray::format_constraint (int *offset, int *step, int *count)
{
    long nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();

    while (p != dim_end ()) {

        int start = dimension_start (p, true);
        int stride = dimension_stride (p, true);
        int stop = dimension_stop (p, true);

        // Check for illegal  constraint
        if (start > stop) {
            ostringstream oss;
            oss << "Array/Grid hyperslab start point "<< start <<
                   " is greater than stop point " <<  stop <<".";
            throw Error(malformed_expr, oss.str());
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
    }// while (p != dim_end ())

    return nels;
}

void HDF5BaseArray::write_nature_number_buffer(int rank, int tnumelm) {

    if (rank != 1) 
        throw InternalErr(__FILE__, __LINE__, "Currently the rank of the missing field should be 1");
    
    vector<int>offset;
    vector<int>count;
    vector<int>step;
    offset.resize(rank);
    count.resize(rank);
    step.resize(rank);


    int nelms = format_constraint(&offset[0], &step[0], &count[0]);

    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    vector<int>val;
    val.resize(nelms);

    if (nelms == tnumelm) {
        for (int i = 0; i < nelms; i++)
            val[i] = i;
        set_value((dods_int32 *) &val[0], nelms);
    }
    else {
        for (int i = 0; i < count[0]; i++)
            val[i] = offset[0] + step[0] * i;
        set_value((dods_int32 *) &val[0], nelms);
    }
}

//#if 0
void HDF5BaseArray::read_data_from_mem_cache(H5DataType h5type, const vector<size_t> &h5_dimsizes,void* buf) {

    BESDEBUG("h5", "Coming to read_data_from_mem_cache"<<endl);
    vector<int>offset;
    vector<int>count;
    vector<int>step;

    int ndims = h5_dimsizes.size();
    if(ndims == 0)
        throw InternalErr(__FILE__, __LINE__, "Currently we only support array numeric data in the cache, the number of dimension for this file is 0");
    

    offset.resize(ndims);
    count.resize(ndims);
    step.resize(ndims);
    int nelms = format_constraint (&offset[0], &step[0], &count[0]);

    // set the original position to the starting point
    vector<size_t>pos(ndims,0);
    for (int i = 0; i< ndims; i++)
        pos[i] = offset[i];


    switch (h5type) {

        case H5UCHAR:
                
        {
            vector<unsigned char> val;
            subset<unsigned char>(
                                      buf,
                                      ndims,
                                      h5_dimsizes,
                                      &offset[0],
                                      &step[0],
                                      &count[0],
                                      &val,
                                      pos,
                                      0
                                     );

            set_value ((dods_byte *) &val[0], nelms);
        } // case H5UCHAR
            break;

        case H5CHAR:
        {

            vector<char>val;
            subset<char>(
                                      buf,
                                      ndims,
                                      h5_dimsizes,
                                      &offset[0],
                                      &step[0],
                                      &count[0],
                                      &val,
                                      pos,
                                      0
                                     );

            vector<short>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (short) (val[counter]);
            set_value ((dods_int16 *) &val[0], nelms);

        } // case H5CHAR
           break;

        case H5INT16:
        {
            vector<short> val;
            subset<short>(
                                      buf,
                                      ndims,
                                      h5_dimsizes,
                                      &offset[0],
                                      &step[0],
                                      &count[0],
                                      &val,
                                      pos,
                                      0
                                     );


            set_value ((dods_int16 *) &val[0], nelms);
        }// H5INT16
            break;


        case H5UINT16:
        {
    	    vector<unsigned short> val;
	    subset<unsigned short>(
				    buf,
				    ndims,
                                    h5_dimsizes,
                                    &offset[0],
                                    &step[0],
                                    &count[0],
                                    &val,
                                    pos,
                                    0
                                  );

               
            set_value ((dods_uint16 *) &val[0], nelms);
        } // H5UINT16
            break;

        case H5INT32:
        {
            vector<int>val;
            subset<int>(
			buf,
			ndims,
			h5_dimsizes,
			&offset[0],
			&step[0],
			&count[0],
			&val,
			pos,
			0
			);

            set_value ((dods_int32 *) &val[0], nelms);
        } // case H5INT32
            break;

        case H5UINT32:
        {
            vector<unsigned int>val;
            subset<unsigned int>(
				buf,
				ndims,
				h5_dimsizes,
				&offset[0],
				&step[0],
				&count[0],
				&val,
				pos,
				0
				);

            set_value ((dods_uint32 *) &val[0], nelms);
        }
            break;

        case H5FLOAT32:
        {
            vector<float>val;
            subset<float>(
	    		  buf,
			  ndims,
			  h5_dimsizes,
			  &offset[0],
			  &step[0],
			  &count[0],
			  &val,
			  pos,
			  0
			  );
            set_value ((dods_float32 *) &val[0], nelms);
        }
            break;


        case H5FLOAT64:
        {

            vector<double>val;
            subset<double>(
		    	    buf,
	    		    ndims,
    			    h5_dimsizes,
			    &offset[0],
    			    &step[0],
    			    &count[0],
			    &val,
			    pos,
			    0
			    );
            set_value ((dods_float64 *) &val[0], nelms);
        } // case H5FLOAT64
            break;

    }
}

//! Getting a subset of a variable
//
//      \param input Input variable
//       \param dim dimension info of the input
//       \param start start indexes of each dim
//       \param stride stride of each dim
//       \param edge count of each dim
//       \param poutput output variable
// 	\parrm index dimension index
//       \return 0 if successful. -1 otherwise.
//
template<typename T>  
int HDF5BaseArray::subset(
    void* input,
    int rank,
    const vector<size_t> & dim,
    int start[],
    int stride[],
    int edge[],
    vector<T> *poutput,
    vector<size_t>& pos,
    int index)
{
    for(int k=0; k<edge[index]; k++) 
    {	
        pos[index] = start[index] + k*stride[index];
        if(index+1<rank)
            subset(input, rank, dim, start, stride, edge, poutput,pos,index+1);			
        if(index==rank-1)
        {
            size_t cur_pos = INDEX_nD_TO_1D( dim, pos);
            void* tempbuf = (void*)((char*)input+cur_pos*sizeof(T));
            poutput->push_back(*(static_cast<T*>(tempbuf)));
            //poutput->push_back(input[HDF5CFUtil::INDEX_nD_TO_1D( dim, pos)]);
        }
    } // end of for
    return 0;
} // end of template<typename T> static int subset

size_t HDF5BaseArray::INDEX_nD_TO_1D (const std::vector < size_t > &dims,
                                 const std::vector < size_t > &pos){
    //
    //  int a[10][20][30];  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3);
    //  int b[10][2]; // &b[1][1] == b + (2*1 + 1);
    // 
    if(dims.size () != pos.size ())
        throw InternalErr(__FILE__,__LINE__,"dimension error in INDEX_nD_TO_1D routine.");
    size_t sum = 0;
    size_t  start = 1;

    for (size_t p = 0; p < pos.size (); p++) {
        size_t m = 1;

        for (size_t j = start; j < dims.size (); j++)
            m *= dims[j];
        sum += m * pos[p];
        start++;
    }
    return sum;
}



