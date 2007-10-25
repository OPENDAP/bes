#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "HDF5GridEOS.h"
#include "H5EOS.h"

extern H5EOS eos;

BaseType *
HDF5GridEOS::ptr_duplicate()
{
    return new HDF5GridEOS(*this);
}


HDF5GridEOS::HDF5GridEOS(const string & n):Grid(n)
{
    ty_id = -1;
    dset_id = -1;
}

HDF5GridEOS::~HDF5GridEOS()
{
}

bool
HDF5GridEOS::read(const string & dataset)
{
  if (read_p())		// nothing to do
    return false;

  // Read data array elements.
  array_var()->read(dataset);
  // Read map array elements.
  Map_iter p = map_begin();

  while (p != map_end()) {
    Array *a = dynamic_cast < Array * >(*p);
    read_dimension(a);
    ++p;
  }    
  set_read_p(true);

  return false;
}

void
HDF5GridEOS::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5GridEOS::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5GridEOS::get_did()
{
    return dset_id;
}

hid_t
HDF5GridEOS::get_tid()
{
    return ty_id;
}

void
HDF5GridEOS::read_dimension(Array* a)
{
    Array::Dim_iter d = a->dim_begin();
    int start = a->dimension_start(d, true);
    int stride = a->dimension_stride(d, true);
    int stop = a->dimension_stop(d, true);
    int count = ((stop - start) / stride) + 1;
    int loc = eos.get_dimension_data_location(a->name());
    
    if(loc >= 0){
      a->set_read_p(true);
      a->val2buf((void *)
		 get_dimension_data(eos.dimension_data[loc], start, stride, stop, count));      
    }
    else{
      cerr << "Could not retrieve map data" << endl;
    }
}

dods_float32* HDF5GridEOS::get_dimension_data(dods_float32* buf,
					      int start, int stride, int stop, int count)
{
  int i=0;
  int j=0;
  dods_float32* dim_buf = NULL;
  dim_buf = new dods_float32[count];
  for(i=start; i <= stop; i = i+stride){
    dim_buf[j] = buf[i];
    j++;
  }
  if(count != j){
    cerr << "HDF5GridEOS::get_dimension_data(): index mismatch" << endl;
  }
  return dim_buf;
}
