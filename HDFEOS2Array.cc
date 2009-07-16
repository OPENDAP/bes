// This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Copyright (c) 2008-2009 The HDF Group
// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org>
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
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config_hdf.h"

#include <vector>

// Include this on linux to suppres an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>

#include <hdfclass.h>
#include <hcstream.h>

#include <escaping.h>
#include <Error.h>
#include <debug.h>
#include "dhdferr.h"

#include "HDFEOS.h"
#include "HDFEOS2Array.h"

extern HDFEOS eos;


HDFEOS2Array::HDFEOS2Array(const string & n, BaseType * v):
    Array(n, v)
{
}

HDFEOS2Array::~HDFEOS2Array()
{
}

BaseType *HDFEOS2Array::ptr_duplicate()
{
    return new HDFEOS2Array(*this);
}


bool HDFEOS2Array::read()
{
    DBG(cerr << ">HDFEOS2Array::read()"  << endl);
  
    int* offset = new int[d_num_dim];
    int* count = new int[d_num_dim];
    int* step = new int[d_num_dim];
    int nelms = format_constraint(offset, step, count);
  
    int *picks = new int[nelms];
    int total_elems = linearize_multi_dimensions(offset, step, count, picks);
    int count2 = 0;

    string varname = name();
    char* val = NULL;
    int i;
    if(eos.is_grid()){
        DBG(cerr << "=HDFEOS2Array::read() grid" << endl);
        count2 = eos.get_data_grid(varname, &val);
    }
    if(eos.is_swath()){
        DBG(cerr << "=HDFEOS2Array::read() swath" << endl);
        varname = eos.get_EOS_name(varname);
        count2 = eos.get_data_swath(varname, &val);
    }

    DBG(cerr << "=HDFEOS2Array::read() count=" << count2  << endl); 
    if(val == NULL){
        cerr << "=HDFEOS2Array::read() val pointer is NULL." << endl;
        return true;
    }
    // This can be optimized by remembering the total size.

    if(nelms ==  total_elems){
        DBG(cerr
            << "nelms = " << nelms
            << " total_elems = " << total_elems
            << endl);
        switch(var()->type()){
        case dods_byte_c:
            {
                set_value((dods_byte*)val,nelms);
                break;
            }
        case dods_int16_c:
            {
                set_value((dods_int16*)val,nelms);
                break;
            }
            
        case dods_uint16_c:
            {
                set_value((dods_uint16*)val,nelms);                
                break;
            }
        case dods_int32_c:
            {
                set_value((dods_int32*)val,nelms);                
                break;
            }
            
        case dods_uint32_c:
            {
                set_value((dods_uint32*)val,nelms);                
                break;
            }
        case dods_float32_c:
            {
                set_value((dods_float32*)val,nelms);                
                break;                
            }
        case dods_float64_c:
            {
                set_value((dods_float64*)val,nelms);                
                break;                
            }            

        default:
            {
                cerr << "HDFEOS2Array::read() Unknown type = "
                     << type() << endl;
                break;
            }            
        } // switch

    }
    else{
        // Subset the array.
        switch(var()->type()){

        case dods_byte_c:
            {
                dods_byte* slab = new dods_byte[nelms];
                for(i=0; i < nelms; i++){
                    dods_byte* f = (dods_byte *)val;
                    DBG(cerr << i << ":" << f[picks[i]] << endl);
                    slab[i] = f[picks[i]];
                }
                set_value(slab,nelms);
                delete[] slab;	    
                break;
            }

      
        case dods_int16_c:
            {
                dods_int16* slab = new dods_int16[nelms];
                for(i=0; i < nelms; i++){
                    dods_int16* f = (dods_int16 *)val;
                    DBG(cerr << i << ":" << f[picks[i]] << endl);
                    slab[i] = f[picks[i]];
                }
                set_value(slab,nelms);
                delete[] slab;	    
                break;
            }
      
        case dods_uint16_c:
            {
                dods_uint16* slab = new dods_uint16[nelms];
                for(i=0; i < nelms; i++){
                    dods_uint16* f = (dods_uint16 *)val;
                    DBG(cerr << i << ":" << f[picks[i]] << endl);
                    slab[i] = f[picks[i]];
                }
                set_value(slab,nelms);
                delete[] slab;	    
                break;
            }

        case dods_int32_c:
            {
                dods_int32* slab = new dods_int32[nelms];
                for(i=0; i < nelms; i++){
                    dods_int32* f = (dods_int32 *)val;
                    DBG(cerr << i << ":" << f[picks[i]] << endl);
                    slab[i] = f[picks[i]];
                }
                set_value(slab,nelms);
                delete[] slab;	    
                break;
            }
      
        case dods_uint32_c:
            {
                dods_uint32* slab = new dods_uint32[nelms];
                for(i=0; i < nelms; i++){
                    dods_uint32* f = (dods_uint32 *)val;
                    DBG(cerr << i << ":" << f[picks[i]] << endl);
                    slab[i] = f[picks[i]];
                }
                set_value(slab,nelms);
                delete[] slab;	    
                break;
            }
      
      
        case dods_float32_c:
            {
                dods_float32* slab = new dods_float32[nelms];
                for(i=0; i < nelms; i++){
                    dods_float32* f = (dods_float32 *)val;
                    DBG(cerr << i << ":" << f[picks[i]] << endl);
                    slab[i] = f[picks[i]];
                }
                set_value(slab,nelms);
                delete[] slab;	    
                break;
            }

        case dods_float64_c:
            {
                dods_float64* slab = new dods_float64[nelms];
                for(i=0; i < nelms; i++){
                    dods_float64* f = (dods_float64 *)val;
                    DBG(cerr << i << ":" << f[picks[i]] << endl);
                    slab[i] = f[picks[i]];
                }
                set_value(slab,nelms);
                delete[] slab;	    
                break;
            }
      
        default:
            {
                cerr << "HDFEOS2Array::read() Unknown type = "
                     << type() << endl;
                break;
            }
        }
    } // else
    delete[] offset;
    delete[] count;
    delete[] step;
    delete[] picks;

    return false;
}


// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int HDFEOS2Array::format_constraint(int *offset, int *step, int *count) {
    long nels = 1;
    int id = 0;

    Dim_iter p = dim_begin();

    while (p != dim_end()) {

	int start = dimension_start(p, true);
	int stride = dimension_stride(p, true);
	int stop = dimension_stop(p, true);

	// Check for empty constraint
	if (stride <= 0 || start < 0 || stop < 0 || start > stop) {
            ostringstream oss;

            oss << "Array/Grid hyperslab indices are bad: [" << start <<
                ":" << stride << ":" << stop << "]";
            throw Error(malformed_expr, oss.str());
	}

	offset[id] = start;
	step[id] = stride;
	count[id] = ((stop - start) / stride) + 1; // count of elements
	nels *= count[id]; // total number of values for variable

	DBG(cerr
	    << "=format_constraint():"
	    << "id=" << id << " offset=" << offset[id]
	    << " step=" << step[id]
	    << " count=" << count[id]
	    << endl);

	id++;
	p++;
    }

    return nels;
}

int HDFEOS2Array::linearize_multi_dimensions(int *start, int *stride,
                                             int *count, int *picks) {
    DBG(cerr << ">linearize_multi_dimensions()" << endl);
    int total = 1;
    int *dim = 0;
    int *temp_count = 0;
    try {
	int id = 0;
	dim = new int[d_num_dim];
	Dim_iter p2 = dim_begin();

	while (p2 != dim_end()) {
	    int a_size = dimension_size(p2, false); // unconstrained size
	    DBG(cerr << "dimension[" << id << "] = " << a_size << endl);
	    dim[id] = a_size;
	    total = total * a_size;
	    ++id;
	    ++p2;
	} // while()

	temp_count = new int[d_num_dim];
	int temp_index;
	int i;
	int array_index = 0;
	int temp_count_dim = 0; // This variable changes when dim. is added.
	int temp_dim = 1;

	for (i = 0; i < d_num_dim; i++)
	    temp_count[i] = 1;

	int num_ele_so_far = 0;
	int total_ele = 1;
	for (i = 0; i < d_num_dim; i++)
	    total_ele = total_ele * count[i];

	while (num_ele_so_far < total_ele) {
	    // loop through the index 
	    while (temp_count_dim < d_num_dim) {
		temp_index = (start[d_num_dim - 1 - temp_count_dim]
			      + (temp_count[d_num_dim - 1 - temp_count_dim]
                                 - 1)
			      * stride[d_num_dim - 1 -
				       temp_count_dim]) * temp_dim;
		array_index = array_index + temp_index;
		temp_dim = temp_dim * dim[d_num_dim - 1 - temp_count_dim];
		temp_count_dim++;
	    }

	    picks[num_ele_so_far] = array_index;

	    num_ele_so_far++;
	    // index can be added 
	    DBG(cerr << "number of element looped so far = " <<
            	num_ele_so_far << endl);
	    for (i = 0; i < d_num_dim; i++) {
		DBG(cerr << "temp_count[" << i << "]=" << temp_count[i] <<
		    endl);
	    }
	    DBG(cerr << "index so far " << array_index << endl);

	    temp_dim = 1;
	    temp_count_dim = 0;
	    array_index = 0;

	    for (i = 0; i < d_num_dim; i++) {
		if (temp_count[i] < count[i]) {
		    temp_count[i]++;
		    break;
		} 
		else {
                    // We reach the end of the dimension, set it to 1 and
		    // increase the next level dimension.  
		    temp_count[i] = 1;
		}
	    }
	}

	delete[] dim;
	delete[] temp_count;
    }
    catch(...) {
	delete[] dim;
	delete[] temp_count;
	throw;
    }

    DBG(cerr << "<linearize_multi_dimensions()" << endl);
    return total;
}

void HDFEOS2Array::set_numdim(int ndims)
{
    d_num_dim = ndims;
}
