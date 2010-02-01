// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  

#include "config_hdf5.h"


#include <string>
#include <ctype.h>

#include "InternalErr.h"

#include "h5dds.h"
#include "HDF5Str.h"
#include "HDF5Structure.h"
// #define DODS_DEBUG
#include "debug.h"

/// A temporary structure for retrieving data from HDF5 compound data type.
typedef struct s2_t {
    /// Buffer for string in compound data
    char a[max_str_len];
} s2_t;


HDF5Str::HDF5Str(const string & n, const string &d) 
 : ty_id(-1), dset_id(-1), array_flag(0), Str(n, d)
{
}

BaseType *HDF5Str::ptr_duplicate()
{
    return new HDF5Str(*this);
}

bool HDF5Str::read()
{
    size_t size = H5Tget_size(ty_id);
    DBG(cerr << ">read() size=" << size << endl);
    if (read_p())
        return false;

#if 0
    if (array_flag == 1) {
        DBG(cerr << "=read(): array is dected." << endl);
        return true;
    }
#endif

    if (size == 0){
	throw InternalErr(__FILE__, __LINE__, "cannot return the size of datatype");
    }
    if (get_dap_type(ty_id) == "String") {
        char *chr = new char[size + 1];
	get_data(dset_id, (void *)chr);
        set_read_p(true);
        string str = chr;
	set_value(str);

        delete[]chr;
    }


    if (get_dap_type(ty_id) == "Structure") {
        BaseType *q = get_parent();

        char Msgi[max_str_len];

        int i = H5Tget_nmembers(ty_id);
        int j = 0;
        int k = 0;

        hid_t s2_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
        hid_t stemp_tid;
#if 0
        s2_t *buf = new s2_t[i];
#endif
	    vector<s2_t> buf(i);
        string myname = name();
        string parent_name;

	if (i < 0){
	   throw InternalErr(__FILE__, __LINE__, "H5Tget_nmembers() failed.");
	}
	if (s2_tid < 0){
	   throw InternalErr(__FILE__, __LINE__, "cannot create a new datatype");
	}
	
#if 0
	try {
#endif
	    DBG(cerr << "=read() ty_id=" << ty_id << " name=" << myname <<
		" size=" << i << endl);
	    while (q != NULL) {
		if (q->is_constructor_type()) {     // Grid, structure or sequence
		    if (k == 0) {
			hid_t type = H5Tcopy(H5T_C_S1);
			if (type < 0){
			   throw InternalErr(__FILE__, __LINE__, "cannot copy");
			}
			if (H5Tset_size(type, (size_t) size) < 0){
			   throw InternalErr(__FILE__, __LINE__, "Unable to set size of datatype.");
			}
			if (H5Tset_strpad(type, H5T_STR_NULLTERM) < 0){
			   throw InternalErr(__FILE__, __LINE__, "H5Tset_strpad() failed.");
			}
			if (H5Tinsert(s2_tid, myname.c_str(), 0, type) < 0){
			   throw InternalErr(__FILE__, __LINE__, "Unable to add to datatype.");
			}
		    } else {
			stemp_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
			if (stemp_tid < 0){
           		    throw InternalErr(__FILE__, __LINE__, "cannot create a new datatype");
		        }
			if (H5Tinsert(stemp_tid, parent_name.c_str(), 0, s2_tid) < 0){
			    throw InternalErr(__FILE__, __LINE__, "Unable to add datatype.");
                        }
			s2_tid = stemp_tid;
		    }
		    // Remember the last parent name.
		    parent_name = q->name();
		    HDF5Structure &p = dynamic_cast < HDF5Structure & >(*q);
		    // Remember the index of array from the last parent.
		    j = p.get_array_index();
		    q = q->get_parent();
		} else {
		    q = NULL;
		}
		k++;
	    }

	    if (H5Dread(dset_id, s2_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buf[0]) < 0) {
		// buf is deleted in the catch ... block below so
		// shouldn't be deleted here. pwest Mar 18, 2009
		//delete[] buf;
		throw InternalErr(__FILE__, __LINE__,
				  string("hdf5_dods server failed when getting int32 data for structure\n")
				  + Msgi);
	    }
	    set_read_p(true);
	    string str = buf[j].a;
	    val2buf(&str);
#if 0	    
	    delete[] buf; buf = 0;
	}
	catch (...) {
	    delete[] buf;
	    throw;
	}
#endif
    }

    return false;
}

void HDF5Str::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5Str::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5Str::get_did()
{
    return dset_id;
}

hid_t HDF5Str::get_tid()
{
    return ty_id;
}
