//
// This file is part of h5_dap_handler, A C++ implementation of the DAP handler
// for HDF5 data.
/**

*/
   

#include <string>
#include <hdf5.h>

using std::string ;

// #include <H5Ipublic.h>

#include "DAS.h"

bool depth_first( hid_t , char *, DAS &, const char * ) ; 
bool get_softlink( DAS &, hid_t, const string &, int ) ;
void read_objects( DAS &das, const string &varname, hid_t dset, int num_attr ) ;
bool find_gloattr( hid_t file, DAS &das ) ;

