

#ifdef __GNUG__
#pragma implementation
#endif

#include <assert.h>
#include <string>
#include <assert.h>
#include <ctype.h>

#define HAVE_CONFIG_H
#include "config_dap.h"
#include "InternalErr.h"
#include "HDF5Float32.h"

Float32 *
NewFloat32(const string &n)
{
    return new HDF5Float32(n);
}

HDF5Float32::HDF5Float32(const string &n) : Float32(n)
{
  ty_id = -1;
  dset_id = -1;
}

BaseType *
HDF5Float32::ptr_duplicate(){

    return new HDF5Float32(*this);
}

// ask experts about this interface.
bool
HDF5Float32::read(const string &dataset, int &error)
{
  
    dods_float32 flt32;
    hid_t type,dset;
    float buf;
    char * Msgi;
    string cfloat32="Float32";
    Msgi = new char[255*sizeof(char)];

    if (read_p()) {
      delete []Msgi;
      return false;
    }
  
    if (return_type(ty_id)==cfloat32) {

       if(get_data(dset,(void *)&buf,Msgi)<0) {
	 delete [] Msgi;
	 throw InternalErr(
	   string("hdf5_dods server failed when getting float32 data\n")
	   +Msgi,__FILE__,__LINE__);
       }

       set_read_p(true);
       flt32 = (dods_float32) buf;
       val2buf( &flt32 );
       return true;

    }
    delete []Msgi;
    error = 1;
    return false;
}
 
void 
HDF5Float32::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5Float32::set_tid(hid_t type) {ty_id = type;}
hid_t 
HDF5Float32::get_did() {return dset_id;}
hid_t
HDF5Float32::get_tid(){return ty_id;}














