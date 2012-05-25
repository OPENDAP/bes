/*
  Test all unsupported types such as int64, float128, enum, bitfield, etc.
  
  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_unsupported t_unsupported.c 

  To generate the test file, run
  %./t_unsupported

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_unsupported.h5

  Copyright (C) 2012 The HDF Group
 */


#include "hdf5.h"
#include <stdlib.h>

#define H5FILE_NAME "t_unsupported.h5"
#define DIM   2		/* dataset dimensions */
#define DIM2  4
#define DIM3  7
#define DS2DIM	3
#define DS2DIM2	16
#define F_BASET         H5T_STD_I16BE       /* file base type */
#define M_BASET         H5T_NATIVE_INT      /* memory base type */

typedef enum {
    SOLID,
    LIQUID,
    GAS,
    PLASMA
} phase_t;	/* enumerated type */

typedef struct {
    int     serial_no;
    char    *location;
    double  temperature;
    double  pressure;
} sensor_t;	/* Compound type */

typedef struct s1_t {
    unsigned int a;
    unsigned int b;
    float c;
} s1_t; 	/* Compound datatype */


int
main (void)
{
    hid_t       file, grp, dataset, dataset2;	/* file and dataset handles */
    hid_t 	dataspace, filetype, memtype, strtype, datatype;/* handles */
    hsize_t     dimsf[1], dimsf2[2];		/* dataset dimensions */
    hid_t	aid;		/* dataspace identifiers */
    hid_t   	attr, attr2;	/* attribute identifiers */
    herr_t      status;
    long long	data[DIM] = {1, 2};  	/* data to write */ 		
    long double  data2[DIM] = {0.345, 5.893};
    unsigned char data3[DIM2][DIM3];
    phase_t data4[DIM2][DIM3], val;
    char *names[4] = {"SOLID", "LIQUID", "GAS", "PLASMA"};
    sensor_t data5[DIM2];
    char data6[DIM2*DIM3], str[DIM3] = "OPAQUE";
    hobj_ref_t *wbuf,      	/* buffer to write to disk */
               *rbuf,       	/* buffer read from disk */
               *tbuf;       	/* temp. buffer read from disk */
    uint32_t   *tu32;      	/* Temporary pointer to uint32 data */
    hsize_t coords[4][2] = {{0,  1}, {2, 11}, {1,  0}, {2,  4} },
            start[2]  = {0, 0},
            stride[2] = {2, 11},
            count[2]  = {2, 2},
            block[2]  = {1, 3};
    hdset_reg_ref_t wdata[DIM];	/* Write buffer */
    char wdata2[DS2DIM][DS2DIM2] = {"The quick brown", "fox jumps over ", "the 5 lazy dogs"};
    int wdata3[4][3][5];      
    int i, j, k;

    /*
     * Create a new file using H5F_ACC_TRUNC access,
     * default file creation properties, and default file
     * access properties.
     */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Test array datatype.
     */
    dimsf[0]  = 4;
    dimsf2[0] = 3;
    dimsf2[1] = 5;    

    /*
     * Initialize data.  i is the element in the dataspace, j and k the
     * elements within the array datatype.
     */
    for (i=0; i<4; i++)
        for (j=0; j<3; j++)
            for (k=0; k<5; k++) 
                wdata3[i][j][k] = i;

    /*
     * Create array datatypes for file and memory.
     */
    filetype = H5Tarray_create (H5T_STD_I64LE, 2, dimsf2);
    memtype = H5Tarray_create (H5T_NATIVE_INT, 2, dimsf2);

    /*
     * Create dataspace.
     */
    dataspace = H5Screate_simple (1, dimsf, NULL);

    /*
     * Create a dataset and an attribute and write the array data to it.
     */
    dataset = H5Dcreate (file, "array", filetype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite (dataset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata3[0][0]);
 
    attr = H5Acreate2 (dataset, "value", filetype, dataspace, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr, memtype, wdata3[0][0]);

    H5Sclose(dataspace);
    H5Tclose(memtype);
    H5Aclose(attr);
    H5Dclose(dataset);
    H5Tclose(filetype);
    

    /* 
     * Test region reference.
     */
    dimsf2[0] = DS2DIM;
    dimsf2[1] = DS2DIM2; 

    /*
     * Create a dataset with character data.
     */
    dataspace = H5Screate_simple (2, dimsf2, NULL);
    dataset = H5Dcreate (file, "dataset", H5T_STD_I8LE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite (dataset, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata2);
    
    /*
     * Create reference to a list of elements in "region_ref".
     */
    status = H5Sselect_elements (dataspace, H5S_SELECT_SET, 4, coords[0]);
    status = H5Rcreate (&wdata[0], file, "dataset", H5R_DATASET_REGION, dataspace);
  
    /*
     * Create reference to a hyperslab in "region_ref".
     */
    status = H5Sselect_hyperslab (dataspace, H5S_SELECT_SET, start, stride, count, block);
    status = H5Rcreate (&wdata[1], file, "dataset", H5R_DATASET_REGION, dataspace);
    
    H5Sclose(dataspace);

    /*
     * Create dataspace.  Setting maximum size to NULL sets the maximum
     * size to be the current size.
     */
    dimsf[0] = DIM;
    dataspace = H5Screate_simple (1, dimsf, NULL);

    /*
     * Create a dataset and an attribute and write the region references to it.
     */
    dataset2 = H5Dcreate2 (file, "region_ref", H5T_STD_REF_DSETREG, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite (dataset2, H5T_STD_REF_DSETREG, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata);

    attr = H5Acreate2 (dataset2, "value", H5T_STD_REF_DSETREG, dataspace, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr, H5T_STD_REF_DSETREG, wdata);
   
    H5Sclose(dataspace);
    H5Aclose(attr);
    H5Dclose(dataset);
    H5Dclose(dataset2);

    /*
     * Test object reference.
     */
    dimsf[0] = DIM2;     

    /* Allocate write & read buffers */
    wbuf = (hobj_ref_t*) malloc(sizeof(hobj_ref_t) * DIM2);
    rbuf = (hobj_ref_t*) malloc(sizeof(hobj_ref_t) * DIM2);
    tbuf = (hobj_ref_t*) malloc(sizeof(hobj_ref_t) * DIM2);

    /* Create dataspace for datasets */
    dataspace = H5Screate_simple(1, dimsf, NULL);

    /* Create a group */
    grp = H5Gcreate2(file, "group1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /* Create a dataset (inside Group1) */
    dataset = H5Dcreate2(grp, "dataset1", H5T_STD_U32BE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

   for(tu32 = (uint32_t *)((void*)wbuf), i = 0; i < DIM2; i++)
        *tu32++ = i * 3;

    /* Write selection to disk */
    H5Dwrite(dataset, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, wbuf);
    H5Dclose(dataset);

     /* Create another dataset (inside Group1) */
    dataset = H5Dcreate2(grp, "dataset2", H5T_STD_U8BE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Dclose(dataset);
   
    /* Create a datatype to refer to */
    datatype = H5Tcreate(H5T_COMPOUND, sizeof(s1_t));

    /* Insert fields */
    H5Tinsert(datatype, "a", HOFFSET(s1_t,a), H5T_STD_I32BE);

    H5Tinsert(datatype, "b", HOFFSET(s1_t,b), H5T_IEEE_F32BE);

    H5Tinsert(datatype, "c", HOFFSET(s1_t,c), H5T_IEEE_F32BE);

    /* Save datatype for later */
    H5Tcommit2(grp, "datatype1", datatype, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /* Close datatype */
    H5Tclose(datatype);

    /* Create a dataset and a attribute */
    dataset = H5Dcreate2(file, "object_ref", H5T_STD_REF_OBJ, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    attr = H5Acreate2 (dataset, "value", H5T_STD_REF_OBJ, dataspace, H5P_DEFAULT, H5P_DEFAULT);

    /* Create reference to dataset */
    H5Rcreate(&wbuf[0], file, "/group1/dataset1", H5R_OBJECT, -1);

    /* Create reference to dataset */
    H5Rcreate(&wbuf[1], file, "/group1/dataset2", H5R_OBJECT, -1);

    /* Create reference to group */
    H5Rcreate(&wbuf[2], file, "/group1", H5R_OBJECT, -1);

    /* Create reference to named datatype */
    H5Rcreate(&wbuf[3], file, "/group1/datatype1", H5R_OBJECT, -1);

    /* Write selection to disk */
    H5Dwrite(dataset, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, wbuf);
    H5Awrite (attr, H5T_STD_REF_OBJ, wbuf);
   
    H5Aclose(attr);
    H5Dclose(dataset);

    H5Sclose(dataspace);
    H5Gclose(grp);

    free(wbuf);
    free(rbuf);
    free(tbuf);

    /*
     * Test opaque datatype.
     */

    /*
     * Initialize data.
     */
    for (i=0; i<DIM2; i++) {
        for (j=0; j<DIM3-1; j++)
            data6[j + i * DIM3] = str[j];
        data6[DIM3 - 1 + i * DIM3] = (char) i + '0';
    }

    /*
     * Create opaque datatype and set the tag to something appropriate.
     * For this example we will write and view the data as a character
     * array.
     */
    filetype = H5Tcreate (H5T_OPAQUE, DIM3);
    status = H5Tset_tag (filetype, "Character array");

    /*
     * Create dataspace.  Setting maximum size to NULL sets the maximum
     * size to be the current size.
     */
    dataspace = H5Screate_simple (1, dimsf, NULL);

    /*
     * Create the dataset and write the opaque data to it.
     */
    dataset = H5Dcreate2 (file, "opaque", filetype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite (dataset, filetype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data6);

    /*
     * Create the attribute and write the opaque data to it.
     */
    attr = H5Acreate2 (dataset, "value", filetype, dataspace, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr, filetype, data6);

    H5Aclose (attr);
    H5Dclose (dataset);
    H5Sclose (dataspace);
    H5Tclose (filetype);


    /*
     * Test compound datatype.
     */
    
    data5[0].serial_no = 1153;
    data5[0].location = "Exterior (static)";
    data5[0].temperature = 53.23;
    data5[0].pressure = 24.57;
    data5[1].serial_no = 1184;
    data5[1].location = "Intake";
    data5[1].temperature = 55.12;
    data5[1].pressure = 22.95;
    data5[2].serial_no = 1027;
    data5[2].location = "Intake manifold";
    data5[2].temperature = 103.55;
    data5[2].pressure = 31.23;
    data5[3].serial_no = 1313;
    data5[3].location = "Exhaust manifold";
    data5[3].temperature = 1252.89;
    data5[3].pressure = 84.11;

    /*
     * Create variable-length string datatype.
     */
    strtype = H5Tcopy (H5T_C_S1);
    status = H5Tset_size (strtype, H5T_VARIABLE);
 
    /*
     * Create the compound datatype for memory.
     */
    memtype = H5Tcreate (H5T_COMPOUND, sizeof (sensor_t));
    status  = H5Tinsert (memtype, "Serial number", HOFFSET (sensor_t, serial_no), H5T_NATIVE_INT);
    status = H5Tinsert (memtype, "Location", HOFFSET (sensor_t, location), strtype);
    status = H5Tinsert (memtype, "Temperature (F)", HOFFSET (sensor_t, temperature), H5T_NATIVE_DOUBLE);
    status = H5Tinsert (memtype, "Pressure (inHg)", HOFFSET (sensor_t, pressure), H5T_NATIVE_DOUBLE);
 
    /*
     * Create the compound datatype for the file.  Because the standard
     * types we are using for the file may have different sizes than
     * the corresponding native types, we must manually calculate the
     * offset of each member.
     */
    filetype = H5Tcreate (H5T_COMPOUND, 8 + sizeof (hvl_t) + 8 + 8);
    status = H5Tinsert (filetype, "Serial number", 0, H5T_STD_I64BE);
    status = H5Tinsert (filetype, "Location", 8, strtype);
    status = H5Tinsert (filetype, "Temperature (F)", 8 + sizeof (hvl_t), H5T_IEEE_F64BE);
    status = H5Tinsert (filetype, "Pressure (inHg)", 8 + sizeof (hvl_t) + 8, H5T_IEEE_F64BE);

    /*
     * Create dataspace.  Setting maximum size to NULL sets the maximum
     * size to be the current size.
     */
    dataspace = H5Screate_simple (1, dimsf, NULL);

    /*
     * Create the dataset and write the compound data to it.
     */
    dataset = H5Dcreate2 (file, "compound", filetype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite (dataset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data5);

    /*
     * Create the attribute and write the compound data to it.
     */
    attr = H5Acreate2 (dataset, "value", filetype, dataspace, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr, memtype, data5);

    H5Aclose (attr);
    H5Dclose (dataset);
    H5Sclose (dataspace);
    H5Tclose (filetype);
  
    /*
     * Test long, long long, unsigned long long and long double datatypes.
     */

    /*
     * Describe the size of the array and create the data space for fixed
     * size dataset.
     */
    dimsf[0] = DIM;
    dataspace = H5Screate_simple(1, dimsf, NULL);

    /*
     * Create a new dataset within the file using defined dataspace and
     * datatype and default dataset creation properties.
     */
    dataset  = H5Dcreate2(file, "long_long",   H5T_NATIVE_LLONG,   dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    dataset2 = H5Dcreate2(file, "long_double", H5T_NATIVE_LDOUBLE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data to the dataset using default transfer properties.
     */
    status = H5Dwrite(dataset,  H5T_NATIVE_LLONG,   H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

    status = H5Dwrite(dataset2, H5T_NATIVE_LDOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data2);

    /*
     * Create scalar attributes.
     */
    aid    = H5Screate(H5S_SCALAR);
    
    attr   = H5Acreate2(dataset, "value", H5T_NATIVE_LLONG, aid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_LLONG, &data[0]);

    attr2  = H5Acreate2(dataset2, "value", H5T_NATIVE_LDOUBLE, aid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr2, H5T_NATIVE_LDOUBLE, &data2[0]);

    /*
     * Close attributes.
     */
    H5Sclose(aid);
    H5Aclose(attr);
    H5Aclose(attr2);

    /*
     * Close/release resources.
     */
    H5Sclose(dataspace);

    H5Dclose(dataset);
    H5Dclose(dataset2);

    /*
     * Test bitfleld type.
     */
    dimsf2[0] = DIM2;
    dimsf2[1] = DIM3;

    
    /*
     * Initialize data.  We will manually pack 4 2-bit integers into
     * each unsigned char data element.
     */
    for (i=0; i<DIM2; i++)
        for (j=0; j<DIM3; j++) {
            data3[i][j] = 0;
            data3[i][j] |= (i * j - j) & 0x03;          /* Field "A" */
            data3[i][j] |= (i & 0x03) << 2;             /* Field "B" */
            data3[i][j] |= (j & 0x03) << 4;             /* Field "C" */
            data3[i][j] |= ( (i + j) & 0x03 ) <<6;      /* Field "D" */
        }

    dataspace = H5Screate_simple (2, dimsf2, NULL);

    dataset = H5Dcreate2(file, "bitfield", /*H5T_STD_B8LE*/H5T_NATIVE_B8, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Dwrite (dataset, H5T_NATIVE_B8, H5S_ALL, H5S_ALL, H5P_DEFAULT, data3[0]);
    
    attr   = H5Acreate2(dataset, "value", /*H5T_STD_B8LE*/H5T_NATIVE_B8, dataspace, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_B8, data3[0]);

    H5Aclose(attr);
    H5Sclose(dataspace);
    H5Dclose(dataset);

    /*
     * Test enumerated datatype.
     */
    for (i=0; i<DIM2; i++)
    	for (j=0; j<DIM3; j++)
            data4[i][j] = (phase_t) ( (i + 1) * j - j) % (int) (PLASMA + 1);

    filetype = H5Tenum_create(F_BASET);
    memtype  = H5Tenum_create(M_BASET);

    for (i = (int) SOLID; i <= (int) PLASMA; i++) {
    	/*
         * Insert enumerated value for memtype.
         */
        val = (phase_t) i;
        status = H5Tenum_insert (memtype, names[i], &val);
        /*
         * Insert enumerated value for filetype.  We must first convert
         * the numerical value val to the base type of the destination.
         */
        status = H5Tconvert (M_BASET, F_BASET, 1, &val, NULL, H5P_DEFAULT);
        status = H5Tenum_insert (filetype, names[i], &val);
    }

    dataspace = H5Screate_simple (2, dimsf2, NULL);

    dataset = H5Dcreate2(file, "enum", filetype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    status = H5Dwrite (dataset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data4[0]);

    attr = H5Acreate (dataset, "value", filetype, dataspace, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr, memtype, data4[0]);

    H5Aclose(attr);
    H5Tclose(filetype);
    H5Sclose(dataspace);
    H5Dclose(dataset);

    H5Fclose(file);
    
    return 0;
}

