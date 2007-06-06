
#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"

#include "h5dds.h"
#include "HDF5Str.h"
#include "HDF5Structure.h"
// #define DODS_DEBUG
#include "debug.h"

typedef struct s2_t {
  char a[11];
} s2_t;


HDF5Str::HDF5Str(const string & n):Str(n)
{
    ty_id = -1;
    dset_id = -1;
}

BaseType *
HDF5Str::ptr_duplicate()
{
    return new HDF5Str(*this);
}

bool
HDF5Str::read(const string & dataset)
{
    if (read_p())
	return false;

    if (array_flag == 1){
        DBG(cerr << "=read(): array is dected." << endl);
	return true;
    }

    if (return_type(ty_id) == "String") {
	char Msgi[256];
	size_t size = H5Tget_size(ty_id);
	char *chr = new char[size + 1];

	if (get_data(dset_id, (void *) chr, Msgi) < 0) {
	    delete [] chr;
	    throw InternalErr(__FILE__, __LINE__,
	      string("hdf5_dods server failed when getting string data\n")
			      + Msgi);
	}

	set_read_p(true);
	string str = chr;

	val2buf(&str);
	delete[]chr;
    }


    if (return_type(ty_id) == "Structure") {
      
      BaseType *q = get_parent();
      
      char Msgi[256];
      
      int i =  H5Tget_nmembers(ty_id);
      int j;
      int k = 0;

      hid_t s2_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
      hid_t stemp_tid;
      
      s2_t buf[i];
      string myname = name();
      string parent_name;
    
      
      DBG(cerr << "=HDF5Str::read() ty_id=" << ty_id << " name=" << myname  << " size=" << i << endl);
      while(q != NULL){
	if(q->is_constructor_type()){ // Grid, structure or sequence
	  if(k == 0){	
	    hid_t type = H5Tcopy(H5T_C_S1);
	    H5Tset_size(type, (size_t)10);
	    H5Tset_strpad(type, H5T_STR_NULLTERM);
	    H5Tinsert(s2_tid, myname.c_str(), 0, type);
	  }
	  else{
	    stemp_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
	    H5Tinsert(stemp_tid, parent_name.c_str(), 0, s2_tid);
	    s2_tid = stemp_tid;
	  }
	  // Remember the last parent name.
	  parent_name = q->name();
	  HDF5Structure *p = dynamic_cast<HDF5Structure*>(q);
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
      string str = buf[j].a;
      val2buf(&str);
    }

    
    return false;
}

void
HDF5Str::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Str::set_tid(hid_t type)
{
    ty_id = type;
}

void
HDF5Str::set_arrayflag(int flag)
{
    array_flag = flag;
}

int
HDF5Str::get_arrayflag()
{
    return array_flag;
}

hid_t
HDF5Str::get_did()
{
    return dset_id;
}

hid_t
HDF5Str::get_tid()
{
    return ty_id;
}



