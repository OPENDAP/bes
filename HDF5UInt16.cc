

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
#include "HDF5UInt16.h"

UInt16 *
NewUInt16(const string &n)
{
    return new HDF5UInt16(n);
}

HDF5UInt16::HDF5UInt16(const string &n) : UInt16(n)
{
  ty_id = -1;
  dset_id = -1;

}

BaseType *
HDF5UInt16::ptr_duplicate(){

    return new HDF5UInt16(*this);
}

// ask experts about this interface.
bool
HDF5UInt16::read(const string &dataset, int &error)
{
    dods_uint16 intu16;
    hid_t type,dset;
    short buf;
    //  char * cuint16="UInt16";
    char * Msgi;
    string cuint16 ="UInt16";
    Msgi = new char[255*sizeof(char)];

    if (read_p()) {
      delete [] Msgi;
        return false;
    }

    if (return_type(ty_id)==cuint16) {
       if (get_data(dset_id,(void *)&buf,Msgi)<0){
	 delete [] Msgi;
	 throw InternalErr(
	   string("hdf5_dods server failed when getting unsigned int16 data\n")
	   +Msgi,__FILE__,__LINE__);
       }

       set_read_p(true);
       intu16 = (dods_int16) buf;
       val2buf( &intu16 );
       return true;

    }

    delete [] Msgi;
    error = 1;
    return false;
}

void 
HDF5UInt16::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5UInt16::set_tid(hid_t type) {ty_id = type;}
hid_t 
HDF5UInt16::get_did() {return dset_id;}
hid_t
HDF5UInt16::get_tid(){return ty_id;}














