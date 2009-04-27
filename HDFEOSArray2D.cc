/*-------------------------------------------------------------------------
 * Copyright (C) 2009	The HDF Group
 *			All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

// #define DODS_DEBUG
// #define CF

#include <iostream>
#include <memory>
#include <sstream>
#include <ctype.h>

#include "config_hdf.h"

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
#include "HDFEOSArray2D.h"
#include "HDFEOS.h"


// using namespace std;
extern HDFEOS eos;

BaseType *HDFEOSArray2D::ptr_duplicate()
{
    return new HDFEOSArray2D(*this);
}

HDFEOSArray2D::HDFEOSArray2D(const string & n, BaseType * v)
    : Array(n, v)
{
  d_num_dim = 2;  
}

HDFEOSArray2D::~HDFEOSArray2D()
{
}


bool HDFEOSArray2D::read(const string &dataset)
{
  
  DBG(cerr << ">HDFEOSArray2D::read(): " << name() << endl);

  if(!eos.is_orthogonal()){   // It must be a 2-D case.
    
    string dim_name = name();
    dim_name = eos.get_EOS_name(dim_name);
    int loc = eos.get_dimension_data_location(dim_name);
    int* offset = new int[d_num_dim];
    int* count = new int[d_num_dim];
    int* step = new int[d_num_dim];
    int nelms = format_constraint(offset, step, count);
    int count2 = eos.get_xdim_size() * eos.get_ydim_size(); // <hyokyung 2009.02.10. 12:55:00>
    
    if(nelms ==  count2){    
      set_value(eos.dimension_data[loc], count2);
    }
    else{
      int *picks = new int[nelms];
      int total_elems = linearize_multi_dimensions(offset, step, count, picks);
      dods_float32* slab = new dods_float32[nelms];
      for(int i=0; i < nelms; i++){
	dods_float32* f = (dods_float32 *)eos.dimension_data[loc];
	DBG(cerr << i << ":" << f[picks[i]] << endl);
	slab[i] = f[picks[i]];
      }
      set_value(slab,nelms);
      delete[] slab;
      delete[] picks;      
    }
    delete[] offset;
    delete[] count;
    delete[] step;
    
    return false;
  }
  else{
    cerr << "1-D Grid map array should have been used." << endl;
    return true;
  }
}


int HDFEOSArray2D::format_constraint(int *offset, int *step, int *count) {
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

int HDFEOSArray2D::linearize_multi_dimensions(int *start, int *stride, int *count,
					       int *picks)
{
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

    // Kent's contribution.
    temp_count = new int[d_num_dim];
    int temp_index;
    int i;
    int array_index = 0;
    int temp_count_dim = 0; /* this variable changes when dim. is added */
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
		      + (temp_count[d_num_dim - 1 - temp_count_dim] - 1)
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
#if 0
      temp_index = 0;
#endif
      array_index = 0;

      for (i = 0; i < d_num_dim; i++) {
	if (temp_count[i] < count[i]) {
	  temp_count[i]++;
	  break;
	} 
	else { // We reach the end of the dimension, set it to 1 and
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
