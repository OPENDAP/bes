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
// Revision 1.6  1998/07/13 20:26:36  jimg
// Fixes from the final test of the new build process
//
// Revision 1.5.4.1  1998/05/22 19:50:53  jimg
// Patch from Jake Hamby to support subsetting raster images
//
// Revision 1.5  1998/04/03 18:34:21  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.4  1998/02/05 20:14:30  jimg
// DODS now compiles with gcc 2.8.x
//
// Revision 1.3  1997/03/10 22:45:14  jimg
// Update for 2.12
//
// Revision 1.5  1996/11/21 23:20:27  todd
// Added error return value to read() mfunc.
//
// Revision 1.4  1996/09/24 20:23:08  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#include <vector>

#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>
#include "config_dap.h"
#include "HDFArray.h"
#include "dhdferr.h"
#include "dodsutil.h"

HDFArray::HDFArray(const String &n = (char *)0, BaseType *v = 0) : Array(n, v)
{}

HDFArray::~HDFArray() {}

BaseType *HDFArray::ptr_duplicate() { return new HDFArray(*this); }
void LoadArrayFromSDS(HDFArray *ar, const hdf_sds& sds);
void LoadArrayFromGR(HDFArray *ar, const hdf_gri& gr);

// Read in an Array from either an SDS or a GR in an HDF file.
bool HDFArray::read(const String &dataset, int &error)
{
    return read_tagref(dataset, -1, -1, error);
}

bool HDFArray::read_tagref(const String &dataset, int32 tag, int32 ref, int &err)
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

    if (tag==-1 || tag==DFTAG_NDG) {
#ifdef NO_EXCEPTIONS
      if (SDSExists(hdf_file.chars(), hdf_name.chars())) {
#else
      try {
#endif
	hdfistream_sds sdsin(hdf_file.chars());
	if(ref != -1)
	  sdsin.seek_ref(ref);
	else
	  sdsin.seek(hdf_name.chars());
	if (isslab)
	  sdsin.setslab(start, edge, stride, false);
	sdsin >> sds;
	sdsin.close();
	foundsds = true;
      }
#ifndef NO_EXCEPTIONS
      catch(...) {}
#else
      }
#endif
    }

    bool foundgr = false;
    hdf_gri gr;
    if (!foundsds && (tag==-1 || tag==DFTAG_VG))  {
#ifdef NO_EXCEPTIONS
	if (GRExists(hdf_file.chars(), hdf_name.chars())) {
#else
        try {
#endif
	    hdfistream_gri grin(hdf_file.chars());
	    if(ref != -1)
	      grin.seek_ref(ref);
	    else
	      grin.seek(hdf_name.chars());
	    if (isslab)
	      grin.setslab(start, edge, stride, false);
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

    if (foundgr || foundsds) {
	err = -1;		// no error
	return true;
    }
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
