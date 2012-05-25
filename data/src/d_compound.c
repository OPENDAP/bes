/*
Test compound data type for HDF5 handler with default option.

Compilation instruction:

%/path/to/hdf5/bin/h5cc -o d_compound d_compound.c 

To generate the test file, run
%./d_compound

To view the test file, run
%/path/to/hdf5/bin/h5dump d_compound.h5

Copyright (C) 2012 The HDF Group
*/


#include "hdf5.h"
#include <stdlib.h>

#define H5FILE_NAME "d_compound.h5"

typedef struct {
	int     serial_no;
	char    *location;
	double  temperature;
	double  pressure;
} sensor_t;	/* Compound type */


int
	main (void)
{
	hid_t   file, dataset; /* file and dataset handles */
	hid_t 	dataspace, filetype, memtype, strtype; /* handles */
	hid_t   attr;           /* attribute identifiers */

	hsize_t  dimsf[1] = {4};       /* dataset dimensions */

	herr_t status;

	sensor_t data5[4];

	/*
	* Initialize data.
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
	* Create a new file using H5F_ACC_TRUNC access,
	* default file creation properties, and default file
	* access properties.
	*/
	file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create variable-length string datatype.
     */
    strtype = H5Tcopy (H5T_C_S1);
    status = H5Tset_size (strtype, H5T_VARIABLE);


	/*
	* Create the compound datatype for memory.
	*/
	memtype = H5Tcreate(H5T_COMPOUND, sizeof (sensor_t));
	status  = H5Tinsert(memtype, "Serial number", 
		HOFFSET (sensor_t, serial_no), H5T_NATIVE_INT);
	status = H5Tinsert(memtype, "Location", HOFFSET (sensor_t, location), 
		strtype);
	status = H5Tinsert(memtype, "Temperature (F)", 
		HOFFSET (sensor_t, temperature), H5T_NATIVE_DOUBLE);
	status = H5Tinsert(memtype, "Pressure (inHg)", 
		HOFFSET (sensor_t, pressure), H5T_NATIVE_DOUBLE);

	/*
	* Create the compound datatype for the file.  Because the standard
	* types we are using for the file may have different sizes than
	* the corresponding native types, we must manually calculate the
	* offset of each member.
	*/
	filetype = H5Tcreate(H5T_COMPOUND, 8 + sizeof(hvl_t) + 8 + 8);
	status = H5Tinsert (filetype, "Serial number", 0, H5T_STD_I32BE);
	status = H5Tinsert(filetype, "Location", 8, strtype);
	status = H5Tinsert(filetype, "Temperature (F)", 8 + sizeof(hvl_t), 
		H5T_IEEE_F64BE);
	status = H5Tinsert(filetype, "Pressure (inHg)", 8 + sizeof(hvl_t) + 8, 
		H5T_IEEE_F64BE);

	/*
	* Create dataspace.  Setting maximum size to NULL sets the maximum
	* size to be the current size.
	*/
	dataspace = H5Screate_simple(1, dimsf, NULL);

	/*
	* Create the dataset and write the compound data to it.
	*/
	dataset = H5Dcreate2(file, "compound", filetype, dataspace, H5P_DEFAULT, 
		H5P_DEFAULT, H5P_DEFAULT);
	status = H5Dwrite(dataset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data5);

	/*
	* Create the attribute and write the compound data to it.
	*/
	attr = H5Acreate2(dataset, "value", filetype, dataspace, H5P_DEFAULT, 
		H5P_DEFAULT);
	status = H5Awrite(attr, memtype, data5);

	H5Aclose(attr);
	H5Sclose(dataspace);
	H5Dclose(dataset);
	H5Tclose(filetype);
	H5Fclose(file);

	return 0;
}

