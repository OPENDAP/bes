/*-------------------------------------------------------------------------
 * Copyright (C) 1999	National Center for Supercomputing Applications.
 *			All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

#ifdef __GNUG__
#pragma implementation
#endif

#define MAX_HDF5_DIMS 20

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <iostream.h>
#include <strstream.h>
#include <assert.h>
#include <ctype.h>
#include <memory>

#define HAVE_CONFIG_H
#include "config_dap.h"
#include "Error.h"
#include "InternalErr.h"

#include "HDF5Array.h"

static char Msga[255];

Array *
NewArray(const string &n, BaseType *v)
{
    return new HDF5Array(n, v);
}

BaseType *
HDF5Array::ptr_duplicate()
{
    return new HDF5Array(*this);
}

HDF5Array::HDF5Array(const string &n, BaseType *v) : Array(n, v)
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
  
    for (Pix p = first_dim(); p ; next_dim(p), id++) {
      int start = dimension_start(p, true); 
      int stride = dimension_stride(p, true);
      int stop = dimension_stop(p, true);

      // Check for empty constraint
      if (stride <= 0 || start < 0 || stop < 0 || start > stop 
	  || start == 0 && stop == 0) {
	ostrstream oss;
	oss << "Array/Grid hyperslab indices are bad: [" << start << ":"
	    << stride << ":" << stop << "]" << ends;
	throw Error(malformed_expr, oss.str()); 
      }
      
      offset[id] = start;
      step[id] = stride;
      count[id] = ((stop - start)/stride) + 1; // count of elements
      nels *= count[id];      // total number of values for variable
    }

    return nels;
}

bool
HDF5Array::read(const string &dataset, int &error)
{
  size_t data_size;

  int *offset = new int [num_dim];
  int *count = new int [num_dim];
  int *step   = new int [num_dim];

  // Throws Error.
  int nelms = format_constraint(offset, step, count);

#if 0
  cout << "coming into array reading " << endl;fflush(stdout);
  cout << "dset id "<< dset_id << endl;fflush(stdout);
  cout << "nelms " << nelms << endl;fflush(stdout);
#endif

  if (nelms == num_elm) {
    data_size = memneed;	// memneed is a member; from the field.

    char *convbuf = new char[data_size];
    if (get_data(dset_id,(void *)convbuf,Msga)<0) {
      delete [] offset;
      delete [] count;
      delete [] step;
      delete [] convbuf;
      throw InternalErr(string("hdf5_dods server failed on getting data.\n")
			+ Msga, __FILE__, __LINE__);
    }
    
    if(check_h5str(ty_id)) {

      set_read_p(true);
      size_t elesize = H5Tget_size(ty_id);

      for (int strindex = 0;strindex < num_elm; strindex++){
	char *strbuf = new char[elesize+1];
	if(get_strdata(dset_id,strindex,convbuf,strbuf,Msga)<0) {
	  delete [] offset;
	  delete [] count;
	  delete [] step;
	  delete [] convbuf;
	  throw InternalErr(string("hdf5_dods server failed on getting data.\n") 
			    + Msga, __FILE__, __LINE__);
	}

	HDF5Str *tempstr = new HDF5Str;
	//should set data type. 
	string str = strbuf;

	tempstr->set_arrayflag(STR_FLAG);
	tempstr->val2buf(&str);

	set_vec(strindex,tempstr);
	delete[]strbuf;
      }
      H5Dclose(dset_id);

    }
    else {
      set_read_p(true);
      val2buf((void*)convbuf);
    }
    delete[]convbuf;  
  }

  else {
    if ((data_size = nelms *H5Tget_size(ty_id)) < 0) {
      delete [] offset;
      delete [] count;
      delete [] step;
      throw InternalErr(string("hdf5_dods server failed on getting data size."),
			__FILE__, __LINE__);
    }

#if 0
    for (i=0;i<num_dim;i++) {
      cout <<i<< "offset"<<offset[i]<<endl;
      cout <<i<<"count"<<count[i]<<endl;
      cout <<i<<"step"<<step[i]<<endl;
    }
#endif

    char *convbuf = new char[data_size];
    if (!get_slabdata(dset_id, offset, step, count, num_dim, data_size,
		      (void *)convbuf,Msga)) {
      delete [] offset;
      delete [] count;
      delete [] step;
      delete [] convbuf;
      throw InternalErr(string("hdf5_dods server failed on getting hyperslab data.\n") 
			+ Msga, __FILE__, __LINE__);
    }

    set_read_p(true);
    val2buf((void*)convbuf); 
    delete [] convbuf;
  }

  delete [] offset;
  delete [] step;
  delete [] count;
}

// public functions to set all parameters needed in read function.

void 
HDF5Array::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5Array::set_tid(hid_t type) {ty_id = type;}
void
HDF5Array::set_memneed(size_t need) {memneed = need;}
void
HDF5Array::set_numdim(int ndims) {num_dim = ndims;}
void
HDF5Array::set_numelm(int nelms) {num_elm = nelms;}
hid_t 
HDF5Array::get_did() {return dset_id;}
hid_t
HDF5Array::get_tid(){return ty_id;}





