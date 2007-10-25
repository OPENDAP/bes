///
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// @author Muqun Yang <ymuqun@hdfgroup.org>
///
/// Copyright (c) 2007   HDF Group
/// Copyright (C) 1997   National Center for Supercomputing Applications.
///                      All rights reserved.
 
#ifndef _H5Git_H
#define _H5Git_H

#include <hdf5.h>
#include "common.h"
int          check_h5str(hid_t);
char*        correct_name(char *);
hid_t        get_attr_info(hid_t dset, int index,
			   DSattr_t * attr_inst, int *, char *);
int          get_data(hid_t dset, void *buf, char *);
hid_t        get_dataset(hid_t pid, char *dname, DS_t * dt_inst_ptr, char *);
hid_t        get_diminfo(hid_t, int, int *, size_t *, hid_t *);
char*        get_dimname(hid_t, int);
hid_t        get_fileid(const char *filename);
hid_t        get_memtype(hid_t);
int          get_slabdata(hid_t dset, int *, int *, int *,
			  int num_dim, void *, char *);
int          get_strdata(int, char *, char *, int, char *);
bool         has_matching_grid_dimscale(hid_t dataset, int ndim, int* size);
H5GridFlag_t maptogrid(hid_t,int);
int          map_to_grid(hid_t,int,int);

#endif //_H5Git_H 

// $Log$ //
