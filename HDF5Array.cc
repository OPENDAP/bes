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

#include <iostream>
#include <memory>
#include <ctype.h>
#include <strstream.h>

#include "Error.h"
#include "InternalErr.h"

#include "HDF5Array.h"
#include "HDF5Str.h"

Array *
NewArray(const string & n, BaseType * v)
{
    return new HDF5Array(n, v);
}

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

    for (Pix p = first_dim(); p; next_dim(p), id++) {
	int start = dimension_start(p, true);
	int stride = dimension_stride(p, true);
	int stop = dimension_stop(p, true);

	// Check for empty constraint
	if (stride <= 0 || start < 0 || stop < 0 || start > stop
	    || start == 0 && stop == 0) {
	    ostrstream oss;

	    oss << "Array/Grid hyperslab indices are bad: [" << start <<
		":" << stride << ":" << stop << "]" << ends;
	    throw Error(malformed_expr, oss.str());
	}

	offset[id] = start;
	step[id] = stride;
	count[id] = ((stop - start) / stride) + 1;	// count of elements
	nels *= count[id];	// total number of values for variable
    }

    return nels;
}

bool
HDF5Array::read(const string & dataset)
{
    char Msga[255];
    size_t data_size;

    int *offset = new int[d_num_dim];
    int *count = new int[d_num_dim];
    int *step = new int[d_num_dim];
    char *convbuf = 0;

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

		set_read_p(true);
		size_t elesize = H5Tget_size(d_ty_id);

		for (int strindex = 0; strindex < d_num_elm; strindex++) {
		    char *strbuf = new char[elesize + 1];

		    if (get_strdata(d_dset_id, strindex, convbuf, strbuf, 
				    Msga) < 0) {
			throw InternalErr(__FILE__, __LINE__,
			  string("hdf5_dods server failed on getting data.\n")
					  + Msga);
		    }

		    HDF5Str *tempstr = new HDF5Str;

		    //should set data type. 
		    string str = strbuf;

		    tempstr->set_arrayflag(STR_FLAG);
		    tempstr->val2buf(&str);

		    set_vec(strindex, tempstr);
		    delete[]strbuf;
		}
		H5Dclose(d_dset_id);

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

	    set_read_p(true);
	    val2buf((void *) convbuf);
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
  
