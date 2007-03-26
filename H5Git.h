//
//
// Copyright (C) 1997   National Center for Supercomputing Applications.
//                      All rights reserved.
 
#ifndef _H5Git_H
#define _H5Git_H

#include <hdf5.h>
#include "common.h"

int get_strdata(hid_t, int, char *, char *, char *);
int check_h5str(hid_t);
hid_t get_attr_info(hid_t dset, int index, DSattr_t * attr_inst, int *,
		    char *);
hid_t get_dataset(hid_t pid, char *dname, DS_t * dt_inst_ptr, char *);
int get_data(hid_t dset, void *buf, char *);
int get_slabdata(hid_t dset, int *, int *, int *, int num_dim, hsize_t,
		 void *, char *);
hid_t get_fileid(const char *filename);
char *get_dimname(hid_t, int);
H5GridFlag_t maptogrid(hid_t,int);
int map_to_grid(hid_t,int,int);
hid_t get_diminfo(hid_t, int, int *, size_t *, hid_t *);
hid_t get_memtype(hid_t);
char *correct_name(char *);
#endif /*_H5Git_H*/

// $Log$ //
