/*
 *  * This example shows how to create a complex compound data type,
 *   * write an array which has the compound data type to the file,
 *     */
/*
 * Test compound data type for HDF5 handler with default option.
 *
 * Compilation instruction:
 *
 * %/path/to/hdf5/bin/h5cc -o h5_comp_complex h5_comp_complex.c 
 *
 * To generate the test file, run
 * %./h5_comp_complex
 *
 * To view the test file, run
 * %/path/to/hdf5/bin/h5dump compound_more_types.h5
 *
 * Copyright (C) 2012-2015 The HDF Group
 * */

#include "hdf5.h"
#include <assert.h>

#define H5FILE_NAME          "compound_more_types.h5"
/* " macros */
/* Name of dataset to create in datafile                              */
#define F41_DATASETNAME   "Compound_more_types"
/* Dataset dimensions                                                 */
#define F41_LENGTH         6
#define F41_RANK           1
#define F41_ARRAY_RANK     1
#define F41_ARRAY_RANKd    2
#define F41_DIMb           4
#define F41_ARRAY_DIMc     6
#define F41_ARRAY_DIMd1    5
#define F41_ARRAY_DIMd2    6
#define F41_ARRAY_DIMf     10



int
main(void)
{
    /* Structure and array for compound types                             */
    typedef struct Array1Struct {
            int                a;
            const char         *b[F41_DIMb];
            char               c[F41_ARRAY_DIMc];
            char               s[6][3];
            short              d[F41_ARRAY_DIMd1][F41_ARRAY_DIMd2];
            float              e;
            double             f[F41_ARRAY_DIMf];
            char               g;
    } Array1Struct;
    Array1Struct       Array1[F41_LENGTH];

    /* Define the value of the string array                           */
    const char *quote [F41_DIMb] = {
            "A fight is a contract that takes two people to honor.",
            "A combative stance means that you've accepted the contract.",
            "In which case, you deserve what you get.",
            "  --  Professor Cheng Man-ch'ing"
    };

    /* Define the value of the character array                        */
    char chararray [F41_ARRAY_DIMc] = {'H', 'e', 'l', 'l', 'o', '!'};
    char s[6][6][3] = {"a1","a2","a3","a4","a5","a6","b1","b2","b3",
            "b4","b5","b6","c1","c2","c3","c4","c5","c6","d1","d2","d3","d4","d5","d6","e1","e2","e3","e4","e5","e6",
            "f1","f2","f3","f4","f5","f6"};


    hid_t      Array1Structid;            /* File datatype identifier */
    hid_t      array_tid;                 /* Array datatype handle    */
    hid_t      array1_tid;                /* Array datatype handle    */
    hid_t      array2_tid;                /* Array datatype handle    */
    hid_t      array4_tid;                /* Array datatype handle    */
    hid_t      datafile, dataset;         /* Datafile/dataset handles */
    hid_t      dataspace;                 /* Dataspace handle         */
    herr_t     status;                    /* Error checking variable */
    hsize_t    dim[] = {F41_LENGTH};          /* Dataspace dimensions     */
    hsize_t    array_dimb[] = {F41_DIMb};     /* Array dimensions         */
    hsize_t    array_dims[] = {6};
    hsize_t    array_dimd[]={F41_ARRAY_DIMd1,F41_ARRAY_DIMd2}; /* Array dimensions         */
    hsize_t    array_dimf[]={F41_ARRAY_DIMf}; /* Array dimensions         */
    hid_t      str_array_id,str1_array_id;
    hid_t      arrays_tid,str_tid;

    int        m, n, o;                   /* Array init loop vars     */

    /* Initialize the data in the arrays/datastructure                */
    for(m = 0; m< F41_LENGTH; m++) {
        Array1[m].a = m;

        for(n = 0; n < F41_DIMb; n++) {
            Array1[m].b[n] = quote[n];
        }

        for(n = 0; n < F41_ARRAY_DIMc; n++) {
            Array1[m].c[n] = chararray[n];
        }

        for(n = 0; n < 24; n++) {
            for(o = 0; o < 3; o++){
                Array1[m].s[n][o] = s[m][n][o];
            }
        }

        for(n = 0; n < F41_ARRAY_DIMd1; n++) {
            for(o = 0; o < F41_ARRAY_DIMd2; o++){
                Array1[m].d[n][o] = m + n + o;
            }
        }

        Array1[m].e = (float)( m * .96 );

        for(n = 0; n < F41_ARRAY_DIMf; n++) {
            Array1[m].f[n] = ( m * 1024.9637 );
        }

        Array1[m].g = 'm';
    }

    /* Create the dataspace                                           */
    dataspace = H5Screate_simple(F41_RANK, dim, NULL);
    assert(dataspace >= 0);

    /* Create the file                                                */
    datafile = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT,
            H5P_DEFAULT);
    assert(datafile >= 0);

    /* Copy the array data type for the string array                  */
    array_tid = H5Tcopy (H5T_C_S1);
    assert(array_tid >= 0);

    /* Set the string array size to Variable                          */
    status = H5Tset_size (array_tid,H5T_VARIABLE);
    assert(status >= 0);

    /* Create the array data type for the string array                */
    str_array_id = H5Tarray_create2(array_tid, F41_ARRAY_RANK, array_dimb);
    assert(str_array_id >= 0);

   /* Copy the array data type for the character array               */ 
    str_tid = H5Tcopy (H5T_C_S1); 
    assert(array1_tid >= 0); 
 
    /* Set the character array size                                   */ 
    status = H5Tset_size (str_tid, F41_ARRAY_DIMc); 
    assert(status >= 0); 


    /* Copy the array data type for the character array               */
    array1_tid = H5Tcopy (H5T_C_S1);
    assert(array1_tid >= 0);

    /* Set the character array size                                   */
    status = H5Tset_size (array1_tid, 3);
    assert(status >= 0);
    if(H5Tset_strpad(array1_tid,H5T_STR_NULLTERM)<0){
        printf("H5Tset_strpad is wrong\n");
        return -1;
    }

    str1_array_id = H5Tarray_create2(array1_tid,1,array_dims);
    assert(str1_array_id >= 0);
      

    /* Create the array data type for the character array             */
    array2_tid = H5Tarray_create2(H5T_NATIVE_SHORT, F41_ARRAY_RANKd, array_dimd);
    assert(array2_tid >= 0);

    /* Create the array data type for the character array             */
    array4_tid = H5Tarray_create2(H5T_NATIVE_DOUBLE, F41_ARRAY_RANK, array_dimf);
    assert(array4_tid >= 0);

    /* Create the memory data type                                    */
    Array1Structid = H5Tcreate (H5T_COMPOUND, sizeof(Array1Struct));
    assert(Array1Structid >= 0);

    /* Insert the arrays and variables into the structure             */
    status = H5Tinsert(Array1Structid, "a_name",
            HOFFSET(Array1Struct, a), H5T_NATIVE_INT);
    assert(status >= 0);

    status = H5Tinsert(Array1Structid, "b_name",
            HOFFSET(Array1Struct, b), str_array_id);
    assert(status >= 0);

    status = H5Tinsert(Array1Structid, "c_name",
            HOFFSET(Array1Struct, c), str_tid);
    assert(status >= 0);

    status = H5Tinsert(Array1Structid, "s_name",
            HOFFSET(Array1Struct, s), str1_array_id);
    assert(status >= 0);

    status = H5Tinsert(Array1Structid, "d_name",
            HOFFSET(Array1Struct, d), array2_tid);
    assert(status >= 0);

    status = H5Tinsert(Array1Structid, "e_name",
            HOFFSET(Array1Struct, e), H5T_NATIVE_FLOAT);
    assert(status >= 0);

    status = H5Tinsert(Array1Structid, "f_name",
            HOFFSET(Array1Struct, f), array4_tid);
    assert(status >= 0);

    status = H5Tinsert(Array1Structid, "g_name",
            HOFFSET(Array1Struct, g), H5T_NATIVE_CHAR);
    assert(status >= 0);

    /* Create the dataset                                             */
    dataset = H5Dcreate2(datafile, F41_DATASETNAME, Array1Structid,
            dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /* Write data to the dataset                                      */
    status = H5Dwrite(dataset, Array1Structid, H5S_ALL, H5S_ALL,
            H5P_DEFAULT, Array1);
    assert(status >= 0);

    /* Release resources                                              */
    status = H5Tclose(Array1Structid);
    assert(status >= 0);

    status = H5Tclose(array_tid);
    assert(status >= 0);

    status = H5Tclose(array1_tid);
    assert(status >= 0);

    status = H5Tclose(array2_tid);
    assert(status >= 0);

    status = H5Tclose(array4_tid);
    assert(status >= 0);

    status = H5Tclose(str_array_id);
    assert(status >= 0);

    status = H5Sclose(dataspace);
    assert(status >= 0);

    status = H5Dclose(dataset);
    assert(status >= 0);

    status = H5Fclose(datafile);
    assert(status >= 0);


}
