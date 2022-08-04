/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 This program actually demostrates the VL buffer from H5Dread 
 DOESNOT contain the data pointer info. HDF5 library makes this buffer pointing to
 the real data instead of the file address.
 */

#include "hdf5.h"
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string.h>
#include <vector>

using namespace std;
#  define UINT32DECODE(p, i) {                              \
   (i)    =  (uint32_t)(*(p) & 0xff);       (p)++;                  \
   (i) |= ((uint32_t)(*(p) & 0xff) <<  8); (p)++;                  \
   (i) |= ((uint32_t)(*(p) & 0xff) << 16); (p)++;                  \
   (i) |= ((uint32_t)(*(p) & 0xff) << 24); (p)++;                  \
}

typedef struct vstr_info_t {
    haddr_t heap_addr;
    size_t  heap_size;
    size_t  heap_obj_index;
    size_t  heap_int_obj_index;
    size_t  heap_obj_size;
}vstr_info_t;

typedef struct vstr_data_info_t {
    haddr_t obj_data_addr;
    size_t  obj_data_size;
}vstr_data_info_t;

void heap_addr_decode(size_t addr_len, const uint8_t **pp/*in,out*/, haddr_t *addr_p/*out*/);

int
main (int argc, char*argv[])
{
    hid_t       file;                        /* handles */
    hid_t       dataset;
    hid_t       cparms;
    H5D_layout_t data_layout;
    int num_elements = 0;
    bool is_vlen_str = false;
    size_t ty_size;

    uint8_t *ubuf;
    vector<char>strval;

    if(argc !=3) {
        printf("Please provide the HDF5 file name and the HDF5 dataset name as the following:\n");
        printf(" ./h5dstoreinfo h5_file_name h5_dset_path .\n");
        return 0;
    }
    /*
     * Open the file and the dataset.
     */
    file = H5Fopen(argv[1], H5F_ACC_RDONLY, H5P_DEFAULT);
    if(file < 0) {
        printf("HDF5 file %s cannot be opened successfully,check the file name and try again.\n",argv[1]);
        return -1;
    }

    dataset = H5Dopen2(file, argv[2], H5P_DEFAULT);
    if(dataset < 0) {
        H5Fclose(file);
        printf("HDF5 dataset %s cannot be opened successfully,check the dataset path and try again.\n",argv[2]);
        return -1;
    }

    /*
     * Get creation properties list.
     */
    cparms = H5Dget_create_plist(dataset); /* Get properties handle first. */

    data_layout = H5Pget_layout(cparms);

    if (H5D_CONTIGUOUS == data_layout)  {

        haddr_t cont_addr = 0;
        hsize_t cont_size = 0;
        printf("Storage: contiguous\n");
        // Using H5Dget_offset(dset_id) for offset and H5Dget_storage_size for size.
        cont_addr = H5Dget_offset(dataset);
        if(cont_addr == HADDR_UNDEF) {
            H5Dclose(dataset);
            H5Fclose(file);
            printf("Cannot obtain the contiguous storage address.\n");
            return -1;
        }
        cont_size = H5Dget_storage_size(dataset);
        // Need to check if fill value if cont_size.
        printf("    Addr: %lu\n",cont_addr);
        printf("    Size: %llu\n",cont_size);

        hid_t dtype = H5Dget_type(dataset);
        if (H5Tis_variable_str(dtype) >0) {
            is_vlen_str = true;
            hid_t space = H5Dget_space(dataset);
            if (H5S_SCALAR == H5Sget_simple_extent_type(space))
                num_elements = 1;
            else
                num_elements = H5Sget_simple_extent_npoints(space);
            ty_size = H5Tget_size(dtype);
            strval.resize(num_elements *ty_size);
            H5Dread(dataset,dtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)&strval[0]);
            H5Sclose(space);
            H5Tclose(dtype);
        }

    }
    H5Pclose(cparms);
    H5Dclose(dataset);
    H5Fclose(file);


    if(is_vlen_str == true) {

        // 1. Obtain the address length(We need to use the HDF5 function) 
        // TODO: H5F_SIZEOF_ADDR (f) 
    
        // 2. Obtain the number of collections, 
        // For each collection, the heap address, the object index and the collection size
        
        vector<vstr_info_t> vstr_info;   
        vstr_info.resize(num_elements);   

        ubuf =(uint8_t*)&strval[0];
        size_t seq_len,seq_idx;
        haddr_t temp_addr;

        UINT32DECODE(ubuf,seq_len);
cout<<"seq_len is " <<seq_len <<endl;
        size_t addr_len = 8;
        heap_addr_decode(addr_len,(const uint8_t **)&ubuf, &temp_addr);
cout <<"temp_addr is " <<temp_addr <<endl;
        UINT32DECODE(ubuf,seq_idx);
        
    }

    
    
    return 0;
}

void heap_addr_decode(size_t addr_len, const uint8_t **pp/*in,out*/, haddr_t *addr_p/*out*/) {

    bool        all_zero = true;    /* True if address was all zeroes */

    /* Reset value in destination */
    *addr_p = 0;

    /* Decode bytes from address */
    for(unsigned u = 0; u < addr_len; u++) {
        uint8_t        c;          /* Local decoded byte */

        /* Get decoded byte (and advance pointer) */
        c = *(*pp)++;

        /* Check for non-undefined address byte value */
        if(c != 0xff)
            all_zero = false;

        if(u < sizeof(*addr_p)) {

            haddr_t        tmp = c;    /* Local copy of address, for casting */

            /* Shift decoded byte to correct position */
            tmp <<= (u * 8);    /*use tmp to get casting right */

            /* Merge into already decoded bytes */
            *addr_p |= tmp;
        } /* end if */
    } /* end for */

    if(all_zero)
        *addr_p = HADDR_UNDEF;

}
