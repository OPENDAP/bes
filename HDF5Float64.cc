


#ifdef __GNUG__
#pragma implementation
#endif

#include <assert.h>
#include <string>
#include <assert.h>
#include <ctype.h>

#define HAVE_CONFIG_H
#include "config_dap.h"
#include "HDF5Float64.h"

Float64 *
NewFloat64(const string &n)
{
    return new HDF5Float64(n);
}

HDF5Float64::HDF5Float64(const string &n) : Float64(n)
{
  ty_id = -1;
  dset_id = -1;
}

BaseType *
HDF5Float64::ptr_duplicate()
{
    return new HDF5Float64(*this); // Copy ctor calls duplicate to do the work
}
 
bool
HDF5Float64::read(const string &dataset, int &error)
{
    dods_float64 flt64;
    hid_t type,dset;
    double buf;
    char *Msgi;
    string cfloat64 ="Float64";
    Msgi = new char[255*sizeof(char)];
    if (read_p()) {
      delete [] Msgi;
        return false;
    }
 
    if (return_type(ty_id)==cfloat64) {
       if(get_data(dset,(void *)&buf,Msgi)<0) {
	 cerr << Msgi << endl;
	 delete [] Msgi;
	 error = 1;
	 return false;
       }
       set_read_p(true);
       flt64 = (dods_float64) buf;
       val2buf( &flt64 );
       return true;

    }

    delete [] Msgi;
    error = 1;
    return false;
}
  
void 
HDF5Float64::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5Float64::set_tid(hid_t type) {ty_id = type;}
hid_t 
HDF5Float64::get_did() {return dset_id;}
hid_t
HDF5Float64::get_tid(){return ty_id;}




