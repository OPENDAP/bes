


#ifdef __GNUG__
#pragma implementation
#endif

#include <assert.h>

#include <string>
#include <assert.h>
#include <ctype.h>

#define HAVE_CONFIG_H
#include "config_dap.h"
#include "HDF5Str.h"


Str *
NewStr(const string &n)
{
    return new HDF5Str(n);
}

HDF5Str::HDF5Str(const string &n) : Str(n)
{
  ty_id = -1;
  dset_id = -1;
  //  cerr <<"coming to the string" << endl;
}

BaseType *
HDF5Str::ptr_duplicate()
{
    return new HDF5Str(*this);
}

bool
HDF5Str::read(const string &dataset, int &error)
{

    hid_t type,dset;
    char *chr;
    char * Msgi;
    size_t size;
    string cstring ="String";
    Msgi = new char[255*sizeof(char)];

    fflush(stdout);
    if (read_p()) {
      delete Msgi;
        return false;
    }

    if(array_flag == 1) {
      delete Msgi;
      return true;
    }
    dset = get_did();
    type = get_tid();

    if (return_type(type) == cstring) {

      size = H5Tget_size(type);
      chr = new char[size +1];
      //  cout << "before getting data \n" << endl;
        if(get_data(dset,(void *)chr,Msgi)<0) {
	  // cerr << Msgi << endl;
	  // cerr << "failed at get_data "<< endl;
	  cout <<Msgi <<endl;
          cout <<"error getting data"<<endl;
	 delete Msgi;
	 error = 1;
	 return false;
       }
	set_read_p(true);
	string str = chr;
	val2buf(&str);
  	delete [] chr;
	return true;
    }

    delete Msgi;
    error = 0;
    return false;

}

void 
HDF5Str::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5Str::set_tid(hid_t type) {ty_id = type;}
void
HDF5Str::set_arrayflag(int flag) {array_flag = flag;}
int
HDF5Str::get_arrayflag() {return array_flag;}
hid_t 
HDF5Str::get_did() {return dset_id;}
hid_t
HDF5Str::get_tid(){return ty_id;}
