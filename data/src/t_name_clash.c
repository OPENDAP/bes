/*
  Test group/variable/attribute that may produce duplicate entries 
  when their names are replaced with CF characters.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_name_clash t_name_clash.c

  To generate the test file, run
  %./t_name_clash

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_name_clash.h5

  Copyright (C) 2012 The HDF Group
 */

#include "hdf5.h"

#define H5FILE_NAME "t_name_clash.h5"

int
main(void)
{
     hid_t file; 			/* file handle */
     hid_t space, space2, space3;	/* handles */
     hid_t attr, attr2;	 		/* handles */
     hid_t datatype, datatype2; 	/* handles */
     hid_t grp, grp2, grp3;		/* handles */
     hid_t dataset, dataset2, dataset3;	/* handles */
     hsize_t dims[1]={10}, dims2[1]={24}, dims3[2]={1, 2}, dims4[2]={2, 2};
     herr_t status;  
 
     /*
      * Data initialization.
      */
     int    data[]  = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};  
     int8_t data2[] = {97, 116, 116, 114, 105, 98, 117, 116, 101, 32, 111, 102, 32, 114, 111, 111, 116, 32, 103, 114, 111, 117, 112, 0}; 
     int    data3[1][2] = {{0, 1}};  
     double data4[2][2] = {{0, 0.0001}, {1, 1.0001}};
 
     /*
      * Create a new file using H5F_ACC_TRUNC access,
      * default file creation properties, and default file
      * access properties.
      */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create dataspace.
     */
    space  = H5Screate_simple (1, dims,  NULL);
    space2 = H5Screate_simple (1, dims2, NULL);

    /*
     * Define datatype for the data in the file.
     */
    datatype  = H5Tcopy(H5T_NATIVE_INT);

    datatype2 = H5Tcopy(H5T_NATIVE_SCHAR);

    /*
     * Test attribute name clash.
     */

    /*
     * Create the attribute and write the string data to it.
     */
    attr   = H5Acreate2 (file, "attr 1", datatype, space, H5P_DEFAULT, H5P_DEFAULT);
    attr2  = H5Acreate2 (file, "attr_1", datatype2, space2, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr,  datatype, data);
    status = H5Awrite (attr2, datatype2, data2);

    /*
     * Close and release resources.
     */
    status = H5Aclose (attr);
    status = H5Aclose (attr2);

    /*
     * Test group name clash.
     */

    grp  = H5Gcreate2(file, "/d1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    grp2 = H5Gcreate2(file, "/g 1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    grp3 = H5Gcreate2(file, "/g_1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    space3 = H5Screate_simple (2, dims3, NULL);

    dataset = H5Dcreate2(file, "d1_", datatype, space3, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);    
    status = H5Dwrite(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data3);

    /*
     * Create the attribute and write the string data to it.
     */
    attr   = H5Acreate2 (dataset, "attr 1", datatype, space, H5P_DEFAULT, H5P_DEFAULT);
    attr2  = H5Acreate2 (dataset, "attr_1", datatype2, space2, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr,  datatype, data);
    status = H5Awrite (attr2, datatype2, data2);

    /*
     * Close and release resources.
     */
    status = H5Aclose (attr);
    status = H5Aclose (attr2);
    status = H5Sclose (space);
    status = H5Sclose (space2);

    status = H5Sclose(space3);
    status = H5Dclose(dataset);
    status = H5Gclose(grp);
    status = H5Gclose(grp2);
    status = H5Gclose(grp3);

    /*
     * Test dataset name clash.
     */

    space = H5Screate_simple (2, dims3, NULL);

    grp  = H5Gcreate2(file, "/g", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dataset  = H5Dcreate2(grp, "dset1",   datatype, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);    
    dataset2 = H5Dcreate2(grp, "dset1_1", datatype, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dataset3 = H5Dcreate2(grp, "dset1_2", datatype, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  
    status = H5Dwrite(dataset,  datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data3);
    status = H5Dwrite(dataset2, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data3);
    status = H5Dwrite(dataset3, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data3);

    status = H5Sclose(space);
    status = H5Dclose(dataset);
    status = H5Dclose(dataset2);
    status = H5Dclose(dataset3);
    status = H5Gclose(grp);
    status = H5Tclose(datatype);
    status = H5Tclose(datatype2);

    space =  H5Screate_simple (2, dims4, NULL);
   
    datatype = H5Tcopy(H5T_NATIVE_DOUBLE);
  
    dataset = H5Dcreate2(file, "g_dset1", datatype, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);   
    status = H5Dwrite(dataset,  datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data4);
    status = H5Dclose(dataset);

    dataset = H5Dcreate2(file, "g_dset1_1", datatype, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite(dataset,  datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data4);
    status = H5Dclose(dataset);

    status = H5Tclose(datatype);
    status = H5Sclose(space);
    status = H5Fclose (file);

    return 0;
}
