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

#include <assert.h>
#include <ctype.h>
#define HAVE_CONFIG_H
#include "config_dap.h"

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
    int start, stride, stop;
    int id = 0;
    long nels = 1;
  
    for (Pix p = first_dim(); p ; next_dim(p), id++) {
    
      start = dimension_start(p, true); 
      stride = dimension_stride(p, true);
      stop = dimension_stop(p, true);
#if 0
      cout << "start " << start << endl;
      cout << "stride " << stride << endl;
      cout << "stop "<<stop << endl;
#endif

      // Check for empty constraint

      if (stride == 0) return -1;
      if(start == 0 && stop == 0 )
	return -1;
      
      offset[id] = start;
      step[id] = stride;
      count[id] = ((stop - start)/stride) + 1; // count of elements
      nels *= count[id];      // total number of values for variable

    }
    return nels;
}

bool
HDF5Array::read(const string &dataset, int &error){

  int i, nelms;
  int strindex;
  size_t data_size;
  int *ip;
  int  *offset;
  int  *count;
  int  *step;

   offset = new int [num_dim];
   count  = new int [num_dim];
   step   = new int [num_dim];
  
   //if ((offset==NULL) || (count==NULL) || (step==NULL)) *error = 1;
 
   nelms =-1;
   nelms = format_constraint(offset,step,count);

#if 0
  cout << "coming into array reading " << endl;fflush(stdout);
  cout << "dset id "<< dset_id << endl;fflush(stdout);
  cout << "nelms " << nelms << endl;fflush(stdout);
#endif

  if (nelms == -1 || nelms == num_elm) {

    data_size = memneed;// from the field.

    char *convbuf = new char[data_size];

    if (get_data(dset_id,(void *)convbuf,Msga)<0) {
      cerr<< "hdf5_dods server failed on getting data" << endl<< Msga << endl;
      error = 1;
      return false;
    }

    
    if(check_h5str(ty_id)){

      set_read_p(true);
      size_t elesize = H5Tget_size(ty_id);


      for (strindex = 0;strindex < num_elm; strindex++){
	char *strbuf = new char[elesize+1];
	if(get_strdata(dset_id,strindex,convbuf,strbuf,Msga)<0){
	  error =1;
	  return false;
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

    if ((data_size = nelms *H5Tget_size(ty_id))<0) {
       error = 1;
       return false;
    }

#if 0

    for (i=0;i<num_dim;i++) {
      cout <<i<< "offset"<<offset[i]<<endl;
      cout <<i<<"count"<<count[i]<<endl;
      cout <<i<<"step"<<step[i]<<endl;
    }

#endif

    char *convbuf = new char[data_size];
    if (!get_slabdata(dset_id,offset,step,count,num_dim,data_size,(void *)convbuf,Msga)) {

       cerr<< "hdf5_dods server failed on getting data" << endl<< Msga << endl;
       error =1;
       return false;
    }
    set_read_p(true);
    val2buf((void*)convbuf); 
    delete[]convbuf;

  }
   delete  [] offset;
   delete  []step;
   delete []count;

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





