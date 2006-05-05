/*-------------------------------------------------------------------------
 * Copyright (C) 1999	National Center for Supercomputing Applications.
 *			All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

/* This is the HDF5-DAS which extracts DAS class descriptors converted from
   HDF5 attribute of an hdf5 data file. */

#include <string>

using std::string ;

#include <H5Ipublic.h>

#include "DAS.h"

bool depth_first( hid_t , char *, DAS &, const char * ) ; 
bool get_softlink( DAS &, hid_t, const string &, int ) ;
void read_objects( DAS &das, const string &varname, hid_t dset, int num_attr ) ;
bool find_gloattr( hid_t file, DAS &das ) ;

