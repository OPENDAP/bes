/*-------------------------------------------------------------------------
 * Copyright (C) 1999	National Center for Supercomputing Applications.
 *			All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

#ifdef __GNUG__
#pragma implementation
#endif


#include "config_hdf5.h"

// #define DODS_DEBUG
#include "debug.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <ctype.h>

#include "Error.h"
#include "InternalErr.h"

#include "HDF5Array.h"
#include "HDF5Structure.h"
#include "HDF5Str.h"

using namespace std;

BaseType *
HDF5Array::ptr_duplicate()
{
  return new HDF5Array(*this);
}

HDF5Array::HDF5Array(const string & n, BaseType * v) : Array(n, v)
{
}

HDF5Array::~HDF5Array()
{
}

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDF5Array::format_constraint(int *offset, int *step, int *count)
{
  long nels = 1;
  int id = 0;

  Dim_iter p = dim_begin();
  while (p != dim_end()) {
      
    int start = dimension_start(p, true);
    int stride = dimension_stride(p, true);
    int stop = dimension_stop(p, true);

    // Check for empty constraint
    if (stride <= 0 || start < 0 || stop < 0 || start > stop){
	
      ostringstream oss;

      oss << "Array/Grid hyperslab indices are bad: [" << start <<
	":" << stride << ":" << stop << "]";
      throw Error(malformed_expr, oss.str());
    }

    offset[id] = start;
    step[id] = stride;
    count[id] = ((stop - start) / stride) + 1;	// count of elements
    nels *= count[id];	// total number of values for variable
	
    DBG(cerr <<id
	<< ": offset=" << offset[id]
	<< " step="    << step[id]
	<< " count="   << count[id]
	<< endl);
	
    id++;
    ++p;
  }
    
  return nels;
}

bool
HDF5Array::read(const string & dataset)
{
  char Msga[255];
  size_t data_size = 0;

  int *offset = new int[d_num_dim];
  int *count = new int[d_num_dim];
  int *step = new int[d_num_dim];
  
  int i,j,k;
  
  char *convbuf = 0;

  DBG(cerr
      << ">read() dataset=" << dataset
      << " data_type_id = " << d_ty_id
      << " name=" << name()
      << " return_type=" << return_type(d_ty_id)
      << " dimension=" << d_num_dim
      << " data_size=" << d_memneed
      << " length = " << length()
      << endl);

  if(return_type(d_ty_id) == "Structure"){
    
    DBG(cerr << "=read() Array of Structure length=" << length() <<  endl);
    

    int picks[d_num_dim];
    int nelms = format_constraint(offset, step, count);

    // Honor constraint evaluation here.
    linearize_multi_dimensions(offset, step, count, picks);
    
    HDF5Structure *p = dynamic_cast<HDF5Structure*>(var());

    // Set the vector.
    for(i=0; i < nelms ; i++){
      p->set_array_index(picks[i]);
      set_vec(i, p);      
    }

    set_read_p(true);
    return false;
  } // if (Structure)

  if(return_type(d_ty_id) == "Array"){

    // Construct an array read from the structure.
    data_size = d_memneed;
    convbuf = new char[data_size];

    hsize_t size2[d_num_dim];
    int perm[d_num_dim];
    H5Tget_array_dims(d_ty_id, size2, perm);
    
    hid_t s1_tid = H5Tcreate(H5T_COMPOUND, data_size);
    hid_t s1_array2;
    float* buf_float;
    int* buf_int;
    
    BaseType *q = get_parent();
    HDF5Structure *p = dynamic_cast<HDF5Structure*>(q);
    // Remember the index of array from the last parent.
    j = p->get_array_index();

    
    if(d_type == H5T_INTEGER){
      s1_array2 = H5Tarray_create(H5T_NATIVE_INT, d_num_dim, size2, NULL);
      // Know the size of structure in advance.
      // Allocate enough buffer for entire array read.
      buf_int = (int*) malloc(18*4*sizeof(int));
      H5Tinsert(s1_tid, name().c_str(), 0, s1_array2);
      H5Dread(d_dset_id, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf_int);
      // How to convert integer array to character array?
      for(i=0; i < data_size; i++){
	char* a = (char*)buf_int;
	convbuf[i] = a[j*sizeof(int)*4+i];
      }

      
    }
    if(d_type == H5T_FLOAT){
      // Know the size of structure in advance.
      // Allocate enough buffer for entire array read.
      buf_float = (float*) malloc(18*30*sizeof(float));    
      s1_array2 = H5Tarray_create(H5T_NATIVE_FLOAT, d_num_dim, size2, NULL);
      H5Tinsert(s1_tid, name().c_str(), 0, s1_array2);
      H5Dread(d_dset_id, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf_float);
      for(i=0; i < data_size; i++){
	char* a = (char*)buf_float;
	convbuf[i] = a[j*sizeof(float)*30+i];
      }
      
    }
    set_read_p(true);
    val2buf((void *) convbuf);
    return false;

  } // if (Array)
  
  try {
    int nelms = format_constraint(offset, step, count); // Throws Error.
    
    if (nelms == d_num_elm) {
      
      data_size = d_memneed;
      
      convbuf = new char[data_size];

      if (get_data(d_dset_id, (void *) convbuf, Msga) < 0) {
	throw InternalErr(__FILE__, __LINE__,
			  string("hdf5_dods server failed on getting data.\n")
			  + Msga);
      }


      if (check_h5str(d_ty_id)) {
	string v_str[d_num_elm];
	size_t elesize = H5Tget_size(d_ty_id);
	DBG(cerr
	    << "element size = " << elesize 
	    << " d_num_elm = " << d_num_elm
	    << endl);

	for (int strindex = 0; strindex < d_num_elm; strindex++) {
	  char *strbuf = new char[elesize + 1];
	  if (get_strdata(strindex, convbuf, strbuf, elesize, Msga) < 0) {
	    throw InternalErr(__FILE__, __LINE__,
			      string("hdf5_dods server failed on getting data.\n")
			      + Msga);
	  }
	  DBG(cerr << "strbuf=" << strbuf << endl);
	  v_str[strindex] = strbuf;
	  delete[]strbuf;
	}

	H5Dclose(d_dset_id);
	set_read_p(true);
	val2buf((void*)&v_str);

      } else {
	set_read_p(true);
	val2buf((void *) convbuf);
      }

    }

    else {
      if ((data_size = nelms * H5Tget_size(d_ty_id)) < 0) {
	throw InternalErr(__FILE__, __LINE__,
			  string("hdf5_dods server failed on getting data size."));
      }

      char *convbuf = new char[data_size];

      if (!get_slabdata(d_dset_id, offset, step, count, d_num_dim, 
			data_size, (void *)convbuf, Msga)) {
	throw InternalErr(__FILE__, __LINE__,
			  string("hdf5_dods server failed on getting hyperslab data.\n")
			  + Msga);
      }
      
      if (check_h5str(d_ty_id)) {
	string v_str[nelms];
	size_t elesize = H5Tget_size(d_ty_id);
	DBG(cerr
	    << "element size = " << elesize 
	    << " nelms = " << nelms
	    << endl);
	for (int strindex = 0; strindex < nelms; strindex++) {
	  char *strbuf = new char[elesize + 1];
	  if (get_strdata(strindex, convbuf, strbuf, elesize, Msga) < 0) {
	    throw InternalErr(__FILE__, __LINE__,
			      string("hdf5_dods server failed on getting data.\n")
			      + Msga);
	  }
	  DBG(cerr << "strbuf=" << strbuf << endl);
	  v_str[strindex] = strbuf;
	  delete[]strbuf;	
	}
	set_read_p(true);
	val2buf((void*)&v_str);
      }
      else{
	set_read_p(true);
	val2buf((void *) convbuf);
      }

    }
  }
  catch (...) {
    delete[]offset;
    delete[]step;
    delete[]count;
    delete[]convbuf;
    throw;
  }

  delete[]offset;
  delete[]step;
  delete[]count;
  delete[]convbuf;

  return false;
}

// public functions to set all parameters needed in read function.

void 
HDF5Array::set_did(hid_t dset) {d_dset_id = dset;}

void 
HDF5Array::set_tid(hid_t type) {d_ty_id = type;}

void
HDF5Array::set_memneed(size_t need) {d_memneed = need;}

void
HDF5Array::set_numdim(int ndims) {d_num_dim = ndims;}

void
HDF5Array::set_numelm(int nelms) {d_num_elm = nelms;}

hid_t 
HDF5Array::get_did() {return d_dset_id;}

hid_t
HDF5Array::get_tid() {return d_ty_id;}
  
void
HDF5Array::linearize_multi_dimensions(int* start, int* stride, int* count, int* picks)
{
    int id = 0;
    int *dim = new int[d_num_dim];
    
    Dim_iter p2 = dim_begin();
  
    while (p2 != dim_end()) {
      int a_size = dimension_size(p2, false); // unconstrained size
      DBG(cerr << "dimension[" <<  id << "] = " << a_size << endl);
      dim[id] = a_size;
      ++id;
      ++p2;
    } // while()

    // Kent's contribution.
    int temp_count[d_num_dim];
    int temp_index;
    int i;
    int array_index = 0;
    int temp_count_dim = 0; /* this variable changes when dim. is added */
    int temp_dim = 1;

    
    for (i=0;i<d_num_dim;i++)
      temp_count[i] = 1;

  
    int num_ele_so_far = 0;
    int total_ele = 1;
    for (i=0;i<d_num_dim;i++)
      total_ele=total_ele*count[i];


    while(num_ele_so_far <total_ele) {
      // loop through the index 

      while(temp_count_dim < d_num_dim) {
#ifdef KENT_ORIG
	// Obtain the index 	
	//  starting from the fastest changing dimension,
	//   calculate the index of each dimension and add it up	
	temp_index = (start[temp_count_dim]+
		      (temp_count[temp_count_dim]-1)*stride[temp_count_dim])*temp_dim;
#endif
	temp_index = (start[d_num_dim - 1 - temp_count_dim]+
		      (temp_count[d_num_dim - 1 - temp_count_dim]-1)*stride[d_num_dim - 1 - temp_count_dim])*temp_dim;	
	array_index =array_index+temp_index;
	temp_dim = temp_dim*dim[d_num_dim - 1 - temp_count_dim];
	temp_count_dim++;
      }
      
      picks[num_ele_so_far] = array_index;
      
      num_ele_so_far++;
      // index can be added 
      DBG(cerr << "number of element looped so far = " << num_ele_so_far << endl);
      for (i =0; i<d_num_dim;i++) {
	DBG(cerr << "temp_count[" << i << "]=" << temp_count[i] << endl);
      }
      DBG(cerr << "index so far " << array_index << endl);

 
      temp_dim = 1;
      temp_count_dim = 0;
      temp_index = 0;
      array_index = 0;

      for (i=0;i<d_num_dim;i++) {
	if(temp_count[i] < count[i]) {
	  temp_count[i]++;
	  break;
	}
	else {// We reach the end of the dimension, set it to 1 and increase the next level dimension. 
	  temp_count[i] = 1;
	}
      }
    }
}
