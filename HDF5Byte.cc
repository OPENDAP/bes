
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
#include "HDF5Byte.h"



Byte *
NewByte(const string &n)
{
    return new HDF5Byte(n);
}

HDF5Byte::HDF5Byte(const string &n) : Byte(n)
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
HDF5Byte::read(const string &dataset, int &error)
{

    dods_byte Dbyte;
    hid_t type,dset;
    int buf;
    char *Msgi;
    string cbyte ="Byte";
    Msgi = new char[255*sizeof(char)];

    if (read_p()) {
      delete [] Msgi;
        return false;
    }

    if (return_type(ty_id)==cbyte) {
       if(get_data(dset_id,(void *)&Dbyte,Msgi)<0){
	 delete [] Msgi;
	 throw InternalErr(string("hdf5_dods server failed when getting one byte data\n")
				  +Msgi,__FILE__,__LINE__);
       }
       set_read_p(true);
       val2buf( &Dbyte);
       return true;

    }

    delete [] Msgi;
    error = 1;
    return false;
}
 
void 
HDF5Byte::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5Byte::set_tid(hid_t type) {ty_id = type;}
hid_t 
HDF5Byte::get_did() {return dset_id;}
hid_t
HDF5Byte::get_tid(){return ty_id;}




  
