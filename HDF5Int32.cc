

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
#include "HDF5Int32.h"

Int32 *
NewInt32(const string &n)
{
    return new HDF5Int32(n);
}

HDF5Int32::HDF5Int32(const string &n) : Int32(n)
{
  ty_id = -1;
  dset_id = -1;
}

BaseType *
HDF5Int32::ptr_duplicate(){

    return new HDF5Int32(*this);
}

// ask experts about this interface.
bool
HDF5Int32::read(const string &dataset, int &error)
{
    dods_int32 intg32;
    hid_t type,dset;
    int buf;
    char *Msgi;
    Msgi = new char[255*sizeof(char)];
    string cint32="Int32";

    if (read_p())  {
      delete [] Msgi;
      return false;
    }
       dset = get_did();
       type = get_tid();
    if (return_type(type)==cint32){

       if(get_data(dset,(void *)&buf,Msgi)<0) {
	 delete [] Msgi;
	 throw InternalErr(
	   string("hdf5_dods server failed when getting int32 data\n")
	   +Msgi,__FILE__,__LINE__);
       }

       set_read_p(true);
       intg32 = (dods_int32) buf;
       val2buf( &intg32 );
       return true;

    }
    delete [] Msgi;
    error = 1;
    return false;
}

void 
HDF5Int32::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5Int32::set_tid(hid_t type) {ty_id = type;}
hid_t 
HDF5Int32::get_did() {return dset_id;}
hid_t
HDF5Int32::get_tid(){return ty_id;}














