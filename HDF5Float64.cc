
#ifdef __GNUG__
#pragma implementation
#endif

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



HDF5Float64::HDF5Float64(const string & n):Float64(n)
{
  ty_id = -1;
  dset_id = -1;
}

BaseType *
HDF5Float64::ptr_duplicate()
{
  return new HDF5Float64(*this);
}

bool
HDF5Float64::read(const string & dataset)
{
  if (read_p())
    return false;

  if (return_type(ty_id) == "Float64") {
    dods_float64 buf;
    dods_float64 flt64;
    char Msgi[256];

    if (get_data(dset_id, (void *) &buf, Msgi) < 0) {
      throw InternalErr(__FILE__, __LINE__,
			string("hdf5_dods server failed when getting float64 data\n")
			+ Msgi);
    }

    set_read_p(true);
    flt64 = (dods_float64) buf;
    val2buf(&flt64);
  }

  if (return_type(ty_id) == "Structure") {
    DBG(cerr << "=read(): Structure" << endl);
    BaseType *q = get_parent();
    HDF5Structure *p = dynamic_cast<HDF5Structure*>(q); 
    DBG(cerr << "=read(): Size = " << p->get_entire_array_size() <<  endl);
    char Msgi[256];
    
    dods_float64 flt64;
#ifdef DODS_DEBUG
    int i =  H5Tget_nmembers(ty_id);
#endif    
    int j = 0;
    int k = 0;
    
    s2_t buf[p->get_entire_array_size()]; // <hyokyung 2007.06.18. 10:06:47>
    
    string myname = name();
    string parent_name;
    
    hid_t s2_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
    hid_t stemp_tid;

    DBG(cerr << "=read() ty_id=" << ty_id << " name=" << myname  << " size=" << i << endl);        
    while(q != NULL){
      if(q->is_constructor_type()){ // Grid, structure or sequence
	if(k == 0){
	  // Bottom level structure
	  H5Tinsert(s2_tid, myname.c_str(), HOFFSET(s2_t, a), H5T_NATIVE_DOUBLE);
	}
	else{
	  stemp_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
	  H5Tinsert(stemp_tid, parent_name.c_str(), 0, s2_tid);
	  s2_tid = stemp_tid;
	}
	parent_name = q->name();
	p = dynamic_cast<HDF5Structure*>(q);
	// Remember the index of array from the last parent.
	j = p->get_array_index();	
	q = q->get_parent();
      }
      else{
	q = NULL;
      }
      k++;
    }
    
    if (H5Dread(dset_id, s2_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf)
	< 0) {
      throw InternalErr(__FILE__, __LINE__,
			string("hdf5_dods server failed when getting int32 data for structure\n")
			+ Msgi);      
    }
    set_read_p(true);
    DBG(cerr << "index " <<  j << endl);
    flt64 = (dods_float64) buf[j].a;
    val2buf(&flt64);      
  }
  return false;
}

void
HDF5Float64::set_did(hid_t dset)
{
  dset_id = dset;
}

void
HDF5Float64::set_tid(hid_t type)
{
  ty_id = type;
}

hid_t
HDF5Float64::get_did()
{
  return dset_id;
}

hid_t
HDF5Float64::get_tid()
{
  return ty_id;
}
