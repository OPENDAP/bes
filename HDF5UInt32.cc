

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
#include "HDF5UInt32.h"

UInt32 *
NewUInt32(const string &n)
{
    return new HDF5UInt32(n);
}

HDF5UInt32::HDF5UInt32(const string &n) : UInt32(n)
{
}

BaseType *
HDF5UInt32::ptr_duplicate(){

    return new HDF5UInt32(*this);
}

// ask experts about this interface.
bool
HDF5UInt32::read(const string &dataset, int &error)
{
    dods_uint32 intu32;
    hid_t type,dset;
    long buf;
    // char * cint32="UInt32";
    char * Msgi;
    string cint32 ="UInt32";
    Msgi = new char[255*sizeof(char)];


    if (read_p()) {
      delete [] Msgi;
      return false;
    }
 

    if (return_type(ty_id)==cint32) {
    
       if(get_data(dset_id,(void *)&buf,Msgi)<0) {
	 delete [] Msgi;
	 throw InternalErr(
	   string("hdf5_dods server failed when getting unsigned int32 data\n")
	   +Msgi,__FILE__,__LINE__);
       }

       set_read_p(true);
       intu32 = (dods_uint32) buf;
       val2buf( &intu32 );
       return true;

    }

    delete [] Msgi;
    error = 1;
    return false;
}

void 
HDF5UInt32::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5UInt32::set_tid(hid_t type) {ty_id = type;}
hid_t 
HDF5UInt32::get_did() {return dset_id;}
hid_t
HDF5UInt32::get_tid(){return ty_id;}














