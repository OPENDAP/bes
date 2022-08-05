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
#include <cstdlib>
#include <iostream>

using namespace std;

int
main (int argc, char*argv[])
{
    hid_t       file;                        /* handles */
    hid_t       dataset;
    hid_t       cparms;
    H5D_layout_t data_layout;

    herr_t      status;


    cout << "#################################################################################################" << endl;
    cout << "# " << argv[0] << endl;
    cout << "#" << endl;

    if(argc !=3) {
        cerr << "Please provide the HDF5 file name and the HDF5 dataset name as the following:" << endl;
        cerr << "    ./h5_obj_addr_info h5_file_name h5_dataset_path " << endl ;
        return 1;
    }
    string filename(argv[1]);
    string vname(argv[2]);
    /*
     * Open the file and the dataset.
     */
    file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if(file < 0) {
        cerr << "HDF5 file " << filename << " cannot be opened successfully,check the file name and try again." << endl;
        return 1;
    }
    cout << "# FILE: " << filename << endl;
    cout << "#" << endl;

    dataset = H5Dopen2(file, vname.c_str(), H5P_DEFAULT);
    if(dataset < 0) {
        H5Fclose(file);
        cerr << "HDF5 dataset (variable):  " << vname << " was not opened successfully, ";
        cerr << "check the dataset path and try again." << endl;
        return 1;
    }
    cout << "#        DATASET: " << vname << endl;

    /*
     * Get creation properties list.
     */
    cparms = H5Dget_create_plist(dataset); /* Get properties handle first. */

    data_layout = H5Pget_layout(cparms);

    if (H5D_CONTIGUOUS == data_layout)  {

        haddr_t cont_addr = 0;
        hsize_t cont_size = 0;
        cout << "#        storage: contiguous" << endl;
        // Using H5Dget_offset(dset_id) for offset and H5Dget_storage_size for size.
        cont_addr = H5Dget_offset(dataset);
        if(cont_addr == HADDR_UNDEF) {
            H5Dclose(dataset);
            H5Fclose(file);
            cerr << "Cannot obtain the contiguous storage address." << endl;
            return 1;
        }
        cont_size = H5Dget_storage_size(dataset);
        // Need to check if fill value if cont_size.
        cout << "#           Addr: " << cont_addr << endl;
        cout << "#           Size: " << cont_size << endl;
    }
    else if (H5D_CHUNKED == data_layout)  {

        hid_t filespace;
        int dataset_rank;
        int chunk_rank;

        cout << "#        storage: chunked" << endl;

        /* Get filespace handle first. */
        filespace = H5Dget_space(dataset);   
        dataset_rank = H5Sget_simple_extent_ndims(filespace);
        cout << "#   dataset_rank: " << dataset_rank << endl;

        // What is happening in this next line?
        // chunk_dims = (hsize_t*)calloc(dataset_rank,sizeof(size_t));

        hsize_t c_dims[dataset_rank];

        chunk_rank = H5Pget_chunk(cparms,dataset_rank,c_dims);
        if(chunk_rank<0){
            cerr << "Call to H5Pget_chunk() returned an error. retval: " << chunk_rank << endl;
            return 1;
        }
        cout << "#     chunk_rank: " << chunk_rank << endl;
        for(int i = 0; i <chunk_rank; i++)
            cout << "#    ChunkDim[" << i << "]: " << c_dims[i] << endl;
        H5Sselect_all(filespace);

        hsize_t chunk_count;
        H5Dget_num_chunks(dataset,filespace,&chunk_count);
        cout << "#    chunk_count: " << chunk_count << endl;


        cout << "#" << endl;
        for(int chunk_index = 0; chunk_index<chunk_count; chunk_index++) {

            haddr_t file_offset = 0;
            hsize_t size = 0;
            hsize_t chunk_coords[chunk_rank];

            H5Dget_chunk_info(dataset,filespace,chunk_index,chunk_coords,NULL,&file_offset,&size);
            cout << "#      chunk_index: " << chunk_index << endl;
            cout << "#             size: " << size << " bytes" << endl;
            cout << "#     chunk_coords: ";
            for (int j=0; j<chunk_rank; j++) {
                cout << "[" << chunk_coords[j] << "]";
            }
            cout << endl;
            cout << "#      file_offset: " << file_offset << endl;
            cout << "#     -- -- -- -- -- -- --" << endl;
        }
        H5Sclose(filespace);
    }
    else if(H5D_COMPACT == data_layout) {/* Compact storage */
        cout << "#        storage: compact" << endl;
        size_t comp_size =0;
    }
    H5Pclose(cparms);
    H5Dclose(dataset);
    H5Fclose(file);

    return 0;
}
