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
 *   This program provides the storage information of an HDF5 chunking/contiguous dataset.
 *   For a chunked dataset, the number of chunks in an HDF5 dataset and the number of chunk dimensions, 
 *   the starting file address of a chunk, storage size of a chunk and the logical offset of this chunk are provided.
 *   For a contiguous dataset, the offset and the length of this HDF5 dataset are provided.
 */

#include "hdf5.h"
#include <stdlib.h>

int
main (int argc, char*argv[])
{
    hid_t       file;                        /* handles */
    hid_t       dataset;
    hid_t       cparms;
    H5D_layout_t data_layout;

    herr_t      status;

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
        printf("    Addr: %llu\n",cont_addr);
        printf("    Size: %llu\n",cont_size);
   
    }
    else if (H5D_CHUNKED == data_layout)  {

        hid_t filespace;
        unsigned int dataset_rank;
        hsize_t* temp_coords;
        hsize_t* chunk_dims;
        int chunk_rank;
        hsize_t num_chunks;
        hsize_t total_num_chunks;
        int i,j;

        printf("storage: chunked.\n");

        /* Get filespace handle first. */
        filespace = H5Dget_space(dataset);   
        dataset_rank = H5Sget_simple_extent_ndims(filespace);
        chunk_dims = (hsize_t*)calloc(dataset_rank,sizeof(size_t));
        chunk_rank = H5Pget_chunk(cparms,dataset_rank,chunk_dims);
        printf("   Number of dimensions in a chunk is  %i\n",chunk_rank);
        for(i = 0; i <chunk_rank; i++) 
            printf(" Chunk Dim %d is %d\n",i,chunk_dims[i]);  
        temp_coords = (hsize_t*)calloc(chunk_rank,sizeof(size_t));
        H5Sselect_all(filespace);
        H5Dget_num_chunks(dataset,filespace,&total_num_chunks);
printf("total_num_chunks is %llu\n",total_num_chunks);
        H5Dget_num_chunks(dataset,filespace,&num_chunks);
        printf("   Number of chunks is %llu\n",num_chunks);

        for(i = 0; i<num_chunks;i++) {

            haddr_t addr = 0;
            hsize_t size = 0;
 
            H5Dget_chunk_info(dataset,filespace,i,temp_coords,NULL,&addr,&size);
            printf("    Chunk index:  %d\n",i);
            printf("    Number of bytes:  %u\n",size);
            printf("    Logical offset: offset");
            for (j=0; j<chunk_rank; j++) 
                printf("[%llu]",temp_coords[j]);
            printf("\n");
            printf("      Physical offset: %llu\n",addr); 
            printf("\n");
        }
        free(temp_coords);
        free(chunk_dims);
        H5Sclose(filespace);
    }
    else if(H5D_COMPACT == data_layout) {/* Compact storage */
        printf("Storage: compact\n");
        size_t comp_size =0;
    }
    H5Pclose(cparms);
    H5Dclose(dataset);
    H5Fclose(file);

    return 0;
}
