#ifdef __GNUG__
#pragma implementation
#endif

// #define DODS_DEBUG

#include <string>
#include <ctype.h>

#include "config_hdf5.h"
#include "debug.h"
#include "h5dds.h"
#include "InternalErr.h"
#include "HDF5Float32.h"
#include "HDF5Structure.h"


typedef struct s2_t {
  /// Buffer for a 32-bit float in compound data
  dods_float32    a;
} s2_t;


HDF5Float32::HDF5Float32(const string & n):Float32(n)
{
  ty_id = -1;
  dset_id = -1;
}

BaseType *
HDF5Float32::ptr_duplicate()
{
  return new HDF5Float32(*this);
}

bool
HDF5Float32::read(const string & dataset)
{
  DBG(cerr << ">HDFFloat32::read() dataset=" << dataset << endl);
  DBG(cerr << ">HDFFloat32::read() ty_id=" << ty_id << endl);
  DBG(cerr << ">HDFFloat32::read() dset_id=" << dset_id << endl);  
  if (read_p())
    return false;

  if (return_type(ty_id) == "Float32") {
    float buf;
    dods_float32 flt32;
    char Msgi[256];

    if (get_data(dset_id, (void *) &buf, Msgi) < 0) {
      throw InternalErr(__FILE__, __LINE__,
			string("hdf5_dods server failed when getting float32 data\n")
			+ Msgi);
    }

    set_read_p(true);
    flt32 = (dods_float32) buf;
    val2buf(&flt32);
  }

  if (return_type(ty_id) == "Structure") {
    
    BaseType *q = get_parent();
    HDF5Structure *p = dynamic_cast<HDF5Structure*>(q); 
    char Msgi[256];    

    dods_float32 flt32;
#ifdef DODS_DEBUG
    int i =  H5Tget_nmembers(ty_id);
#endif    
    int j = 0;
    int k = 0;
    
    s2_t buf[p->get_entire_array_size()];
    
    string myname = name();
    string parent_name;
      
    hid_t s2_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
    hid_t stemp_tid;
    
    DBG(cerr << "=HDF5Float32::read() ty_id=" << ty_id << " name=" << myname << endl);
    while(q != NULL){
      if(q->is_constructor_type()){ // Grid, structure or sequence
	if(k == 0){
	  // Bottom level structure
	  H5Tinsert(s2_tid, myname.c_str(), HOFFSET(s2_t, a), H5T_NATIVE_FLOAT);
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
    flt32 =  buf[j].a;
    val2buf(&flt32);
  }
  
  return false;
}

void
HDF5Float32::set_did(hid_t dset)
{
  dset_id = dset;
}

void
HDF5Float32::set_tid(hid_t type)
{
  ty_id = type;
}

hid_t
HDF5Float32::get_did()
{
  return dset_id;
}

hid_t
HDF5Float32::get_tid()
{
  return ty_id;
}
