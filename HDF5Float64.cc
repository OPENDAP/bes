
// #define DODS_DEBUG

#include <string>
#include <ctype.h>
#include "config_hdf5.h"
#include "InternalErr.h"
#include "h5dds.h"
#include "HDF5Float64.h"
#include "HDF5Structure.h"
#include "debug.h"

typedef struct s2_t {
    /// Buffer for a 64-bit float in compound data
    dods_float64 a;
} s2_t;



HDF5Float64::HDF5Float64(const string & n, const string &d) : Float64(n, d)
{
    ty_id = -1;
    dset_id = -1;
}

BaseType *HDF5Float64::ptr_duplicate()
{
    return new HDF5Float64(*this);
}

bool HDF5Float64::read()
{
    if (read_p())
        return false;

    if (get_dap_type(ty_id) == "Float64") {
        dods_float64 buf;
	get_data(dset_id, (void *) &buf);
        set_read_p(true);
	set_value(buf);
    }

    if (get_dap_type(ty_id) == "Structure") {
        DBG(cerr << "=read(): Structure" << endl);
        BaseType *q = get_parent();
        if (!q)
        	throw InternalErr(__FILE__, __LINE__, "null pointer");
        HDF5Structure &p = dynamic_cast < HDF5Structure & >(*q);
        DBG(cerr << "=read(): Size = " << p.get_entire_array_size() << endl);
        char Msgi[256];

#if 0
        dods_float64 flt64;
#endif
#ifdef DODS_DEBUG
        int i = H5Tget_nmembers(ty_id);
	if(i < 0){
	   throw InternalErr(__FILE__, __LINE__, "H5Tget_nmembers() failed.");
	}
#endif
        int j = 0;
        int k = 0;

        s2_t *buf = 0;
	try {
	    buf = new s2_t[p.get_entire_array_size()];
	    string myname = name();
	    string parent_name;

	    hid_t s2_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
	    hid_t stemp_tid;

	    if(s2_tid < 0){
		throw InternalErr(__FILE__, __LINE__, "cannot create a new datatype");
	    }

	    DBG(cerr << "=read() ty_id=" << ty_id << " name=" << myname <<
		" size=" << i << endl);
	    while (q != NULL) {
		if (q->is_constructor_type()) {     // Grid, structure or sequence
		    if (k == 0) {
			// Bottom level structure
			if (H5Tinsert(s2_tid, myname.c_str(), HOFFSET(s2_t, a),
				  H5T_NATIVE_DOUBLE) < 0){
			   throw InternalErr(__FILE__, __LINE__, "Unable to add datatype.");
			}
		    } else {
			stemp_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
			if(stemp_tid < 0){
			   throw InternalErr(__FILE__, __LINE__, "cannot create a new datatype");
			}
			if (H5Tinsert(stemp_tid, parent_name.c_str(), 0, s2_tid) < 0){
			   throw InternalErr(__FILE__, __LINE__, "Unable to add datatype.");
			}
			s2_tid = stemp_tid;
		    }
		    parent_name = q->name();
		    p = dynamic_cast < HDF5Structure & >(*q);
		    // Remember the index of array from the last parent.
		    j = p.get_array_index();
		    q = q->get_parent();
		} else {
		    q = NULL;
		}
		k++;
	    }

	    if (H5Dread(dset_id, s2_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf) < 0) {
		// buf is deleted in the catch ... block below and
		// should not be deleted here. pwest Mar 18, 2009
		//delete[] buf;
		throw InternalErr(__FILE__, __LINE__,
				  string
				  ("hdf5_dods server failed when getting int32 data for structure\n")
				  + Msgi);
	    }
	    set_read_p(true);
	    DBG(cerr << "index " << j << endl);
#if 0
	    flt64 = (dods_float64) buf[j].a;
	    val2buf(&flt64);
#endif
	    set_value(buf[j].a);
	    delete[] buf;
	}
	catch(...) {
	    // memory allocation exception could have been thrown in
	    // creating this ptr so check if exists before deleting.
	    // pwest Mar 18, 2009
	    if( buf ) delete[] buf;
	    throw;
	}
    }
    return false;
}

void HDF5Float64::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5Float64::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5Float64::get_did()
{
    return dset_id;
}

hid_t HDF5Float64::get_tid()
{
    return ty_id;
}
