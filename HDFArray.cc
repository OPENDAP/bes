// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
/////////////////////////////////////////////////////////////////////////////
// Copyright 1996, by the California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.

// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: HDFArray.cc,v $ - implmentation of HDFArray class
//
/////////////////////////////////////////////////////////////////////////////

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

#include "escaping.h"
#include "HDFArray.h"
#include "dhdferr.h"

#include "Error.h"

HDFArray::HDFArray(const string &n, BaseType *v) : Array(n, v)
{}

HDFArray::~HDFArray() {}

BaseType *HDFArray::ptr_duplicate() { return new HDFArray(*this); }
void LoadArrayFromSDS(HDFArray *ar, const hdf_sds& sds);
void LoadArrayFromGR(HDFArray *ar, const hdf_gri& gr);

// Read in an Array from either an SDS or a GR in an HDF file.
bool HDFArray::read(const string &dataset)
{
    int err = 0;
    int status = read_tagref(dataset, -1, -1, err);
    if (err)
	throw Error(unknown_error, "Could not read from dataset.");
    return status;
}

bool 
HDFArray::read_tagref(const string &dataset, int32 tag, int32 ref, int &err)
{
    if (read_p())
	return true;

    // get the HDF dataset name, SDS name
    string hdf_file = dataset;
    string hdf_name = this->name();

    // get slab constraint
    vector<int> start, edge, stride;
    bool isslab = GetSlabConstraint(start, edge, stride);

    bool foundsds = false;
    hdf_sds sds;
    if (tag==-1 || tag==DFTAG_NDG) {
	if (SDSExists(hdf_file.c_str(), hdf_name.c_str())) {
	    hdfistream_sds sdsin(hdf_file.c_str());
	    if(ref != -1)
		sdsin.seek_ref(ref);
	    else
		sdsin.seek(hdf_name.c_str());
	    if (isslab)
		sdsin.setslab(start, edge, stride, false);
	    sdsin >> sds;
	    sdsin.close();
	    foundsds = true;
	}
    }

    bool foundgr = false;
    hdf_gri gr;
    if (!foundsds && (tag==-1 || tag==DFTAG_VG))  {
	if (GRExists(hdf_file.c_str(), hdf_name.c_str())) {
	    hdfistream_gri grin(hdf_file.c_str());
	    if(ref != -1)
		grin.seek_ref(ref);
	    else
		grin.seek(hdf_name.c_str());
	    if (isslab)
		grin.setslab(start, edge, stride, false);
	    grin >> gr;
	    grin.close();
	    foundgr = true;
	}
    }

    if (foundsds)
	LoadArrayFromSDS(this, sds);
    else if (foundgr)
	LoadArrayFromGR(this, gr);

    if (foundgr || foundsds) {
	set_read_p(true);	// Moved here; see bug 136
	err = 0;		// no error
	return true;
    }
    else {
	err = 1;
	return false;
    }
}

Array *NewArray(const string &n, BaseType *v)
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

// $Log: HDFArray.cc,v $
// Revision 1.12.4.1  2003/05/21 16:26:51  edavis
// Updated/corrected copyright statements.
//
// Revision 1.12  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.10.4.6  2002/04/12 00:07:04  jimg
// I removed old code that was wrapped in #if 0 ... #endif guards.
//
// Revision 1.10.4.5  2002/04/12 00:03:14  jimg
// Fixed casts that appear throughout the code. I changed most/all of the
// casts to the new-style syntax. I also removed casts that we're not needed.
//
// Revision 1.10.4.4  2002/04/10 18:38:10  jimg
// I modified the server so that it knows about, and uses, all the DODS
// numeric datatypes. Previously the server cast 32 bit floats to 64 bits and
// cast most integer data to 32 bits. Now if an HDF file contains these
// datatypes (32 bit floats, 16 bit ints, et c.) the server returns data
// using those types (which DODS has supported for a while...).
//
// Revision 1.10.4.3  2002/03/26 20:46:06  jimg
// Moved the call to set_read_p() so that the read_p flag is not set until after
// the values are read. This (hopefully) fixes bug 136 where some arrays that
// could not be read are marked as read (because set_read_p is called before the
// functions that read the data fail). Bug 136 is hard to test since I can't
// find a URL for it. This one may resurface later...
//
// Revision 1.10.4.2  2002/03/14 19:15:07  jimg
// Fixed use of int err in read() so that it's always initialized to zero.
// This is a fix for bug 135.
//
// Revision 1.11  2001/08/27 17:21:34  jimg
// Merged with version 3.2.2
//
// Revision 1.10.4.1  2001/07/28 00:25:15  jimg
// I removed the code which escapes names. This function is now handled
// for all the servers by the dap.
//
// Revision 1.10  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.9  2000/03/31 16:56:05  jimg
// Merged with release 3.1.4
//
// Revision 1.8.8.1  2000/03/20 22:26:52  jimg
// Switched to the id2dods, etc. escaping function in the dap.
//
// Revision 1.8  1999/05/06 03:23:34  jimg
// Merged changes from no-gnu branch
//
// Revision 1.7.6.1  1999/05/06 00:27:21  jimg
// Jakes String --> string changes
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
