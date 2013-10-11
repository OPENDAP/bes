// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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

/// \file HDF5UInt16.cc
/// 
/// \brief The implementation of mapping unsigned HDF5 16 bit integer to DAP UInt16 for the default option.
///
/// \author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// \author Kent Yang       (myang6@hdfgroup.org)
/// \author James Gallagher (jgallagher@opendap.org)


#include "config_hdf5.h"
#include "debug.h"
#include <string>
#include <ctype.h>

#include "InternalErr.h"

#include "h5dds.h"
#include "HDF5UInt16.h"
#include "HDF5Structure.h"

typedef struct s2_uint16_t {
    /// Buffer for a 16 bit integer in compound data
    dods_int16 a;
} s2_uint16_t;


HDF5UInt16::HDF5UInt16(const string & n, const string &d) : UInt16(n, d)
{
    ty_id = -1;
    dset_id = -1;

}

BaseType *HDF5UInt16::ptr_duplicate()
{

    return new HDF5UInt16(*this);
}

bool HDF5UInt16::read()
{
    if (read_p())
	return true;

    if (get_dap_type(ty_id) == "UInt16") {
	dods_uint16 buf;
	get_data(dset_id, (void *) &buf);
	set_read_p(true);
	set_value(buf);

        // Release the handles.
        if (H5Tclose(ty_id) < 0) {
            throw InternalErr(__FILE__, __LINE__, "Unable to close the datatype.");
        }
        if (H5Dclose(dset_id) < 0) {
            throw InternalErr(__FILE__, __LINE__, "Unable to close the dset.");
        }

    }

    if (get_dap_type(ty_id) == "Structure") {

	BaseType *q = get_parent();
	if (!q)
	    throw InternalErr(__FILE__, __LINE__, "null pointer");
	HDF5Structure &p = static_cast<HDF5Structure &> (*q);

#ifdef DODS_DEBUG
	int i = H5Tget_nmembers(ty_id);
	if (i < 0) {
	    throw InternalErr(__FILE__, __LINE__, "H5Tget_nmembers() failed.");
	}
#endif
	int j = 0;
	int k = 0;

	hid_t s1_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_uint16_t));
	hid_t stemp_tid;

	if (s1_tid < 0) {
	    throw InternalErr(__FILE__, __LINE__, "cannot create a new datatype");
	}

	vector<s2_uint16_t> buf(p.get_entire_array_size());
	string myname = name();
	string parent_name;

	DBG(cerr
		<< "=read() ty_id=" << ty_id
		<< " name=" << myname << " no of members =" << i << endl);
	while (q != NULL) {

	    if (q->is_constructor_type()) { // Grid, structure or sequence
		if (k == 0) {
		    // Bottom level structure
		    DBG(cerr << "=read() my_name " << myname.
			    c_str() << endl);
		    if (H5Tinsert(s1_tid, myname.c_str(), HOFFSET(s2_uint16_t, a), H5T_NATIVE_UINT16) < 0) {
			throw InternalErr(__FILE__, __LINE__, "Unable to add datatype.");
		    }
		}
		else {
		    DBG(cerr << k << "=read() parent_name=" << parent_name
			    << endl);

		    stemp_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_uint16_t));
		    if (stemp_tid < 0) {
			throw InternalErr(__FILE__, __LINE__, "cannot create a new datatype");
		    }
		    if (H5Tinsert(stemp_tid, parent_name.c_str(), 0, s1_tid) < 0) {
			throw InternalErr(__FILE__, __LINE__, "Unable to add datatype.");
		    }
		    s1_tid = stemp_tid;

		}
		// Remember the last parent name.
		parent_name = q->name();
		p = static_cast<HDF5Structure &> (*q);
		// Remember the index of array from the last parent.
		j = p.get_array_index();
		q = q->get_parent();

	    }
	    else {
		q = NULL;
	    }
	    k++;
	} // while ()


	if (H5Dread(dset_id, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buf[0]) < 0) {
	    // this should not be called here. The exception on
	    // the next line is thrown and caught below. In the
	    // catch block the buf is deleted. pcw Mar 18, 2009
	    //delete[] buf;
	    throw InternalErr(__FILE__, __LINE__, "hdf5_dods server failed when getting int32 data for structure");
	    // string
	    // (
	    // + Msgi);
	}

	set_read_p(true);
	DBG(cerr << "index " << j << endl);
	set_value(buf[j].a);
    } // In case of structure
 
    return true;
}

void HDF5UInt16::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5UInt16::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5UInt16::get_did()
{
    return dset_id;
}

hid_t HDF5UInt16::get_tid()
{
    return ty_id;
}
