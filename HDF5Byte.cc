#ifdef __GNUG__
#pragma implementation
#endif

// #define DODS_DEBUG
#include <string>
#include <ctype.h>

#include "config_hdf5.h"
#include "debug.h"
#include "InternalErr.h"
#include "h5dds.h"
#include "HDF5Byte.h"
#include "HDF5Structure.h"

typedef struct s2_t {
  dods_byte    a;
} s2_t;


HDF5Byte::HDF5Byte(const string & n) : Byte(n)
{
  ty_id = -1;
  dset_id = -1;
}

BaseType *
HDF5Byte::ptr_duplicate()
{
  return new HDF5Byte(*this);
}

bool
HDF5Byte::read(const string & dataset)
{
  if (read_p())
    return false;

  if (return_type(ty_id) == "Byte") {
    dods_byte Dbyte;
    char Msgi[256];
    if (get_data(dset_id, (void *) &Dbyte, Msgi) < 0) {
      throw InternalErr(__FILE__, __LINE__,
			string("hdf5_dods server failed when getting one byte data\n"));

    }
    set_read_p(true);
    val2buf(&Dbyte);
  }

  if (return_type(ty_id) == "Structure") {
        
    BaseType *q = get_parent();
    HDF5Structure *p = dynamic_cast<HDF5Structure*>(q);
    
    char Msgi[256];

    int i =  H5Tget_nmembers(ty_id);
    int j;
    int k = 0;

    hid_t s1_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
    hid_t stemp_tid;

    s2_t buf[p->get_entire_array_size()];
    
    string myname = name();
    string parent_name;
    

    DBG(cerr
	<< "=read() ty_id=" << ty_id
	<< " name=" << myname
	<< " no of members =" << i << endl);    
    while(q != NULL){

      if(q->is_constructor_type()){ // Grid, structure or sequence
	if(k == 0){
	  // Bottom level structure
	  DBG(cerr << "my_name " << myname.c_str()  << endl);
	  H5Tinsert(s1_tid, myname.c_str(), HOFFSET(s2_t, a), H5T_NATIVE_INT);
	}
	else{
	  DBG(cerr << k << " parent_name=" <<  parent_name << endl);

	  stemp_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
	  H5Tinsert(stemp_tid, parent_name.c_str(), 0, s1_tid);
	  s1_tid = stemp_tid;

	}
	// Remember the last parent name.
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
    } // while ()


    if (H5Dread(dset_id, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf) < 0) {
      throw InternalErr(__FILE__, __LINE__,
			string("hdf5_dods server failed when getting int32 data for structure\n")
			+ Msgi);      
    }    
    
    set_read_p(true);
    DBG(cerr << "index " <<  j << endl);
    dods_byte dbyte = (dods_byte) buf[j].a;
    val2buf(&dbyte);
    
  } // In case of structure

  return false;
}

void
HDF5Byte::set_did(hid_t dset)
{
  dset_id = dset;
}

void
HDF5Byte::set_tid(hid_t type)
{
  ty_id = type;
}

hid_t
HDF5Byte::get_did()
{
  return dset_id;
}

hid_t
HDF5Byte::get_tid()
{
  return ty_id;
}
