/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1996, California Institute of Technology.  
// ALL RIGHTS RESERVED.   U.S. Government Sponsorship acknowledged. 
//
// Please read the full copyright notice in the file COPYRIGH 
// in this directory.
//
// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: HDFArray.cc,v $ - implmentation of HDFArray class
//
// $Log: HDFArray.cc,v $
// Revision 1.1  1996/10/31 18:43:17  jimg
// Added.
//
// Revision 1.4  1996/09/24 20:23:08  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#include <vector.h>
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>
#include "config_dap.h"
#include "HDFArray.h"
#include "dhdferr.h"
#include "hdfutil.h"

HDFArray::HDFArray(const String &n = (char *)0, BaseType *v = 0) : Array(n, v)
{}

HDFArray::~HDFArray() {}

BaseType *HDFArray::ptr_duplicate() { return new HDFArray(*this); }
void LoadArrayFromSDS(HDFArray *ar, const hdf_sds& sds);
void LoadArrayFromGR(HDFArray *ar, const hdf_gri& gr);

// Read in an Array from either an SDS or a GR in an HDF file.
bool HDFArray::read(const String &dataset, int &err)
{
    if (read_p())
	return true;

    // get the HDF dataset name, SDS name
    String hdf_file = dods2id(dataset);
    String hdf_name = dods2id(this->name());

    bool foundsds = false;
    hdf_sds sds;

    // get slab constraint
    vector<int> start, edge, stride;
    bool isslab = GetSlabConstraint(start, edge, stride);

#ifdef NO_EXCEPTIONS
    if (SDSExists(hdf_file.chars(), hdf_name.chars())) {
#else
    try {
#endif
	hdfistream_sds sdsin(hdf_file.chars());
	sdsin.seek(hdf_name.chars());
	if (isslab)
	    sdsin.setslab(start, edge, stride, false);
	sdsin >> sds;
	sdsin.close();
	foundsds = true;
    }
#ifndef NO_EXCEPTIONS
    catch(...) {}
#endif

    bool foundgr = false;
    hdf_gri gr;
    if (!foundsds)  {
#ifdef NO_EXCEPTIONS
	if (GRExists(hdf_file.chars(), hdf_name.chars())) {
#else
        try {
#endif
	    hdfistream_gri grin(hdf_file.chars());
	    grin.seek(hdf_name.chars());
	    grin >> gr;
	    grin.close();
	    foundgr = true;
	}
#ifndef NO_EXCEPTIONS
	catch(...) { }
#endif
    }

    set_read_p(true);

    if (foundsds)
	LoadArrayFromSDS(this, sds);
    else if (foundgr)
	LoadArrayFromGR(this, gr);

    if (foundgr || foundsds)
	return true;
    else {
	err = 0;
	return false;
    }
}

Array *NewArray(const String &n, BaseType *v)
{ 
    return new HDFArray(n, v); 
} 

// Read the slab constraint parameters; the arrays start_array, edge_array,
// stride_array.  Returns true if there is a slab constraint, false otherwise.
bool HDFArray::GetSlabConstraint(vector<int>& start_array, 
				  vector<int>& edge_array, 
				  vector<int>& stride_array) {
    int start = 0, stop = 0, stride = 0;
    int edge = 0;

    start_array = vector<int>(0);
    edge_array = vector<int>(0);
    stride_array = vector<int>(0);

    for (Pix p=first_dim(); p; next_dim(p)) {
	start = dimension_start(p,true);
	stride = dimension_stride(p,true);
	stop = dimension_stop(p,true);
	if (start == 0 && stop == 0 && stride == 0)
	    return false;	// no slab constraint
	if (start > stop)
	    THROW(dhdferr_arrcons);
	edge = (int)((stop - start)/stride) + 1;
	if (start + edge > dimension_size(p))
	    THROW(dhdferr_arrcons);

	start_array.push_back(start);
	edge_array.push_back(edge);
	stride_array.push_back(stride);
    }
    return true;
}
