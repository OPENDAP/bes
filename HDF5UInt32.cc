#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"

#include "h5dds.h"
#include "HDF5UInt32.h"
#include "HDF5Structure.h"
#include "debug.h"

typedef struct s2_t {
    /// Buffer for a 32-bit integer in compound data
    dods_int32 a;
} s2_t;


HDF5UInt32::HDF5UInt32(const string & n):UInt32(n)
{
}

BaseType *HDF5UInt32::ptr_duplicate()
{

    return new HDF5UInt32(*this);
}

bool HDF5UInt32::read(const string & dataset)
{
    if (read_p())
        return false;

    if (return_type(ty_id) == "UInt32") {
	dods_uint32 buf;
	get_data(dset_id, (void *) &buf);
        set_read_p(true);
	set_value(buf);
    }

    if (return_type(ty_id) == "Structure") {

        BaseType *q = get_parent();
        if (!q)
        	throw InternalErr(__FILE__, __LINE__, "null pointer");
        HDF5Structure &p = dynamic_cast < HDF5Structure & >(*q);

        char Msgi[256];
#ifdef DODS_DEBUG
        int i = H5Tget_nmembers(ty_id);
#endif
        int j = 0;
        int k = 0;

        hid_t s1_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
        hid_t stemp_tid;

        s2_t *buf = 0;
	try {
	    buf = new s2_t[p.get_entire_array_size()];
        string myname = name();
        string parent_name;



        DBG(cerr << "=read() ty_id=" << ty_id << " name=" << myname <<
            " size=" << i << endl);
        while (q != NULL) {

            if (q->is_constructor_type()) {     // Grid, structure or sequence
                if (k == 0) {
                    // Bottom level structure
                    DBG(cerr << "my_name " << myname.c_str() << endl);
                    H5Tinsert(s1_tid, myname.c_str(), HOFFSET(s2_t, a),
                              H5T_NATIVE_UINT32);
                } else {
                    DBG(cerr << k << " parent_name=" << parent_name <<
                        endl);

                    stemp_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
                    H5Tinsert(stemp_tid, parent_name.c_str(), 0, s1_tid);
                    s1_tid = stemp_tid;

                }
                // Remember the last parent name.
                parent_name = q->name();
                p = dynamic_cast < HDF5Structure & >(*q);
                // Remember the index of array from the last parent.
                j = p.get_array_index();
                q = q->get_parent();

            } else {
                q = NULL;
            }
            k++;
        }                       // while ()


        if (H5Dread(dset_id, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf) <
            0) {
            throw InternalErr(__FILE__, __LINE__,
                              string
                              ("hdf5_dods server failed when getting int32 data for structure\n")
                              + Msgi);
        }

        set_read_p(true);
        DBG(cerr << "index " << j << endl);
#if 0
        dods_int32 intg32 = (dods_int32) buf[j].a;
        val2buf(&intg32);
#endif
	set_value(buf[j].a);
	delete[] buf;
	}
	catch(...) {
	    delete[] buf;
	    throw;
	}

    }                           // In case of structure

    
    return false;
}

void HDF5UInt32::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5UInt32::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5UInt32::get_did()
{
    return dset_id;
}

hid_t HDF5UInt32::get_tid()
{
    return ty_id;
}
