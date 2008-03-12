////////////////////////////////////////////////////////////////////////////////
/// \file H5Git.cc
///  iterates all HDF5 internals.
/// 
///  This file includes all the routines to search HDF5 group, dataset, links,
///  and attributes. since we are using HDF5 C APIs, we include all c functions
///  in this file.
///
//   Kent Yang 2001.05.14
///
///  Copyright (C) 2007 HDFGroup, Inc.
///
///  Copyright (C) 1999 National Center for Supercomputing Applications.
///                     All rights reserved.
///
////////////////////////////////////////////////////////////////////////////////


// #define DODS_DEBUG

#include <string.h>
#include <hdf5.h>

#include <debug.h>
#include <InternalErr.h>        // <hyokyung 2007.02.23. 14:17:32>

#include "H5Git.h"

/// Maximum size of error message buffer.
#define MAX_ERROR_MESSAGE 512

using namespace libdap;

////////////////////////////////////////////////////////////////////////////////
/// \fn get_attr_info(hid_t dset, int index, DSattr_t *attr_inst_ptr,
///                  int *ignoreptr, char *error)
///  will get attribute information.
///
/// This function will get attribute information: datatype, dataspace(dimension
/// sizes) and number of dimensions and put it into a data struct.
///
/// \param[in]  dset  dataset id
/// \param[in]  index  index of attribute
/// \param[out] attr_inst_ptr an attribute instance pointer
/// \param[out] ignoreptr  a flag to record whether it can be ignored.
/// \param[out] error error message to be reported(not used).
/// \return pointer to attribute structure
/// \throw InternalError 
////////////////////////////////////////////////////////////////////////////////
hid_t get_attr_info(hid_t dset, int index, DSattr_t * attr_inst_ptr,
                    int *ignoreptr, char *error)
{

    hid_t ty_id, attrid, space, memtype;
    H5T_class_t temp_type;
    hsize_t size[DODS_MAX_RANK];
    hsize_t maxsize[DODS_MAX_RANK];
    char *namebuf;
    size_t need;
    hsize_t nelmts = 1;

    // size_t need; 
    int j, ndims;

    *ignoreptr = 0;
    namebuf = (char *) malloc(DODS_NAMELEN);

	// This try/catch block is used to ensure namebuf is freed. jhrg 3/2008
	try {
    if ((attrid = H5Aopen_idx(dset, index)) < 0) {
        string msg = "dap_h5_handler: unable to open attribute by index ";
        msg += index;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    // obtain the attribute name. 
    if ((H5Aget_name(attrid, DODS_NAMELEN, namebuf)) < 0) {
        string msg =
            "dap_h5_handler: unable to obtain hdf5 attribute name for id=";
        msg += attrid;
        throw InternalErr(__FILE__, __LINE__, msg);
    }


    if ((attrid = H5Aopen_name(dset, namebuf)) < 0) {
        string msg =
            "dap_h5_handler: unable to obtain hdf5 attribute by name = ";
        msg += namebuf;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    // obtain the type of the attribute. 
    if ((ty_id = H5Aget_type(attrid)) < 0) {
        string msg =
            "dap_h5_handler: unable to obtain hdf5 attribute type for id = ";
        msg += attrid;
        throw InternalErr(__FILE__, __LINE__, msg);

    }
    temp_type = H5Tget_class(ty_id);

    if (temp_type < 0) {
        string msg =
            "dap_h5_handler: unable to obtain hdf5 datatype class for type_id = ";
        msg += ty_id;
        throw InternalErr(__FILE__, __LINE__, msg);

    }

    if ((temp_type == H5T_TIME) ||
        (temp_type == H5T_BITFIELD) ||
        (temp_type == H5T_OPAQUE) || (temp_type == H5T_ENUM)) {
        *ignoreptr = 1;
        attrid = 0;
        goto exit;
    }

    if (temp_type == H5T_REFERENCE) {
        *ignoreptr = 1;
        attrid = 0;
        goto exit;
    }

    if ((space = H5Aget_space(attrid)) < 0) {
        string msg =
            "dap_h5_handler: unable to obtain hdf5 data space for id = ";
        msg += attrid;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    ndims = H5Sget_simple_extent_dims(space, size, maxsize);

    // Check dimension size. 
    if (ndims > DODS_MAX_RANK) {
        string msg =
            "dap_h5_handler: number of dimensions exceeds hdf5_das server allowed.";
        msg += attrid;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    // return ndims and size[ndims]. 
    if (ndims) {
        for (j = 0; j < ndims; j++)
            nelmts *= size[j];
    }

    need = nelmts * H5Tget_size(ty_id);
    // We want to save memory type in the struct
    memtype = H5Tget_native_type(ty_id, H5T_DIR_ASCEND);
    (*attr_inst_ptr).type = memtype;
    (*attr_inst_ptr).ndims = ndims;
    (*attr_inst_ptr).nelmts = nelmts;
    (*attr_inst_ptr).need = need;
    strcpy((*attr_inst_ptr).name, namebuf);

    for (j = 0; j < ndims; j++) {
        (*attr_inst_ptr).size[j] = size[j];
    }
	}
	catch(...) {
		free(namebuf);
		throw;
	}
	
  exit:
    free(namebuf);
    return attrid;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn get_fileid(const char *filename)
/// gets HDF5 file id.
/// 
/// This function is used because H5Fopen cannot be directly used in a C++ code.
/// \param filename HDF5 filename
/// \return a file handler id
////////////////////////////////////////////////////////////////////////////////
hid_t get_fileid(const char *filename)
{
    return H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn get_dataset(hid_t pid, char *dname, DS_t * dt_inst_ptr, char *error)
/// obtain data information in a dataset datatype, dataspace(dimension sizes)
/// and number of dimensions and put these information into a pointer of data
/// struct.
///
/// \param[in] pid    parent object id(group id)
/// \param[in] dname  dataset name
/// \param[out] dt_inst_ptr  pointer to the attribute struct(* attr_inst_ptr)
/// \param[out] error error message to be reported.
/// \return     dataset id      
////////////////////////////////////////////////////////////////////////////////
hid_t get_dataset(hid_t pid, char *dname, DS_t * dt_inst_ptr, char *error)
{

    hid_t dset = -1;
    hid_t datatype, memtype, dataspace;
    H5T_class_t temp_type;
    hsize_t size[DODS_MAX_RANK];
    hsize_t maxsize[DODS_MAX_RANK];
    size_t need;
    hsize_t nelmts = 1;
    int j, ndims;

    DBG(cerr << ">get_dataset()" << endl);
    if ((dset = H5Dopen(pid, dname)) < 0) {
        snprintf(error, MAX_ERROR_MESSAGE,
                 "h5_dds handler:  failed to obtain dataset %s", dname);
        return -1;
    }

    if ((datatype = H5Dget_type(dset)) < 0) {
        snprintf(error, MAX_ERROR_MESSAGE,
                 "h5_dds handler:  failed to obtain datatype from  dataset %s",
                 dname);
        return -1;
    }

    if ((dataspace = H5Dget_space(dset)) < 0) {
        snprintf(error, MAX_ERROR_MESSAGE,
                 "h5_dds handler:  failed to obtain dataspace from  dataset %s",
                 dname);
        return -1;
    }

    temp_type = H5Tget_class(datatype);


    if (temp_type < 0) {
        sprintf(error,
                "h5_dds handler:  failed to obtain type class from %d",
                datatype);
        return -1;
    }


    if ((temp_type == H5T_TIME) ||
        (temp_type == H5T_BITFIELD) ||
        (temp_type == H5T_OPAQUE) || (temp_type == H5T_ENUM)) {
        //      ||
        //        (temp_type == H5T_REFERENCE)) {
        sprintf(error,
                "h5_dds handler: get_data0 - unexpected datatype at temp_type = %d",
                temp_type);
        return -1;
    }
    // Obtain number of attributes in this dataset. 
    if ((ndims = H5Sget_simple_extent_dims(dataspace, size, maxsize)) < 0) {
        strcpy(error,
               "h5_dds handler: get_data0 - unable to get number of dimensions");
        return -1;
    }
    // check dimension size. 
    if (ndims > DODS_MAX_RANK) {
        strcpy(error,
               "number of dimensions exceeds hdf5-dods server allowed");
        return -1;
    }
    // return ndims and size[ndims]. 
    if (ndims) {
        for (j = 0; j < ndims; j++)
            nelmts *= size[j];
    }

    need = nelmts * H5Tget_size(datatype);
    memtype = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
    (*dt_inst_ptr).dset = dset;
    (*dt_inst_ptr).dataspace = dataspace;
    (*dt_inst_ptr).type = memtype;
    (*dt_inst_ptr).ndims = ndims;
    (*dt_inst_ptr).nelmts = nelmts;
    (*dt_inst_ptr).need = need;
    strcpy((*dt_inst_ptr).name, dname);

    for (j = 0; j < ndims; j++) {
        (*dt_inst_ptr).size[j] = size[j];
    }
    DBG(cerr << "<get_dataset() dimension=" << ndims << " elements=" <<
        nelmts << endl);
    return dset;
}


////////////////////////////////////////////////////////////////////////////////
/// \fn get_data(hid_t dset, void *buf, char *error)
/// will get all data of a dataset and put it into buf.
///
/// \param[in] dset dataset id(dset)
/// \param[out] buf pointer to a buffer
/// \param[out] error error message to be reported.
/// \return -1, if failed.
/// \return 0, if succeeded.
////////////////////////////////////////////////////////////////////////////////
int get_data(hid_t dset, void *buf, char *error)
{

    hid_t datatype, dataspace;
    hid_t memtype;

    DBG(cerr << ">get_data()" << endl);
    if ((datatype = H5Dget_type(dset)) < 0) {
        sprintf(error, "failed to obtain datatype from  dataset %d", dset);
        return -1;
    }

    if ((dataspace = H5Dget_space(dset)) < 0) {
        sprintf(error, "failed to obtain dataspace from  dataset %d",
                dset);
        return -1;
    }
    //  memtype = get_memtype(datatype);
    //  using HDF5 H5Tget_native_type API
    memtype = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
    if (memtype < 0) {
        sprintf(error, "failed to obtain memory type");
        return -1;
    }

    if (memtype == H5T_STRING) {
        DBG(cerr << "=get_data(): H5T_STRING type is detected." << endl);
        if (H5Dread(dset, datatype, dataspace, dataspace, H5P_DEFAULT, buf)
            < 0) {
            sprintf(error, "failed to read data from  dataset %d", dset);
            printf("error %s\n", error);
            return -1;
        }
    } else {
        DBG(cerr << "=get_data(): H5T_STRING type is NOT detected." <<
            endl);
        if (H5Dread(dset, memtype, dataspace, dataspace, H5P_DEFAULT, buf)
            < 0) {
            sprintf(error, "failed to read data from  dataset %d", dset);
            return -1;
        }
    }

    if (H5Tget_class(datatype) != H5T_STRING) {
        H5Sclose(dataspace);
        H5Tclose(datatype);
        H5Dclose(dset);
    }
    DBG(cerr << "<get_data()" << endl);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// \fn get_strdata(int strindex, char *allbuf, char *buf, int elesize, char *error)
/// will get an individual string data from all string datas and put it into buf.
///
/// \param[in] strindex index of H5T_STRING array
/// \param[in] allbuf pointer to string buffer that has been built so far
/// \param[in] elesize size of string element in the array
/// \param[out] buf pointer to a buf
/// \param[out] error error message to be reported.
/// \return -1 if failed.
/// \return  0 if succeeded.
////////////////////////////////////////////////////////////////////////////////
int
get_strdata(int strindex, char *allbuf, char *buf, int elesize,
            char *error)
{

    int i;
    char *tempvalue = NULL;


    tempvalue = allbuf;         // The beginning of entier buffer.
    DBG(cerr
        << ">get_strdata(): "
        << " strindex=" << strindex << " allbuf=" << allbuf << endl);

    // Tokenize the convbuf. 
    for (i = 0; i < strindex; i++) {
        tempvalue = tempvalue + elesize;
    }
    strncpy(buf, tempvalue, elesize);
    buf[elesize] = '\0';        // <hyokyung 2007.05.29. 08:16:57>
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn get_slabdata(hid_t dset, int *offset, int *step, int *count, int num_dim,
///                  void *buf, char *error)
/// will get hyperslab data of a dataset and put it into buf.
///
/// \param[in] dset dataset id
/// \param[in] offset starting point
/// \param[in] step  stride
/// \param[in] count  count
/// \param[in] num_dim  number of array dimensions
/// \param[out] buf pointer to a buffer
/// \param[out] error error message to be reported.
/// \return 0 if failed
/// \return 1 otherwise
////////////////////////////////////////////////////////////////////////////////
int
get_slabdata(hid_t dset, int *offset, int *step, int *count, int num_dim,
             void *buf, char *error)
{
    hid_t dataspace, memspace, datatype, memtype;
    hsize_t *dyn_count = NULL, *dyn_step = NULL;
    hssize_t *dyn_offset = NULL;
    int i;

    DBG(cerr << ">get_slabdata() " << endl);
    if ((datatype = H5Dget_type(dset)) < 0) {
        sprintf(error,
                "h5_dods server:  failed to obtain datatype from  dataset %d",
                dset);
        return 0;
    }
    // Using H5T_get_native_type API
    memtype = H5Tget_native_type(datatype, H5T_DIR_ASCEND);

    if (memtype < 0) {
        sprintf(error, "fail to obtain memory type.");
        return 0;
    }
    if ((dataspace = H5Dget_space(dset)) < 0) {
        sprintf(error,
                "h5_dods server:  failed to obtain dataspace from  dataset %d",
                dset);
        free(dyn_count);
        free(dyn_offset);
        free(dyn_step);     
        return 0;
    }

    dyn_count = (hsize_t *) calloc(num_dim, sizeof(hsize_t));
    dyn_step = (hsize_t *) calloc(num_dim, sizeof(hsize_t));
    dyn_offset = (hssize_t *) calloc(num_dim, sizeof(hssize_t));

    if (!dyn_count) {
        sprintf(error,
                "h5_dods server: out of memory for hyperslab dataset %d",
                dset);
        free(dyn_count);
        free(dyn_offset);
        free(dyn_step);                
        return 0;
    }

    if (!dyn_step) {
        sprintf(error,
                "h5_dods server: out of memory for hyperslab dataset %d",
                dset);
        free(dyn_count);
        free(dyn_offset);
        free(dyn_step);
        return 0;
    }

    if (!dyn_offset) {
        sprintf(error,
                "h5_dods server: out of memory for hyperslab dataset %d",
                dset);
        return 0;
    }

    for (i = 0; i < num_dim; i++) {
        dyn_count[i] = (hsize_t) (*count);
        dyn_step[i] = (hsize_t) (*step);
        dyn_offset[i] = (hssize_t) (*offset);
        DBG(cerr << "count:" << dyn_count[i]
            << " step:" << dyn_step[i]
            << " offset:" << dyn_step[i] << endl);
        count++;
        step++;
        offset++;
    }

    if (H5Sselect_hyperslab
        (dataspace, H5S_SELECT_SET, (const hsize_t *) dyn_offset, dyn_step,
         dyn_count, NULL) < 0) {
        sprintf(error, "h5_dods server: selection error for dataspace %d",
                dataspace);
        return 0;
    }

    memspace = H5Screate_simple(num_dim, dyn_count, NULL);

    free(dyn_count);
    free(dyn_offset);
    free(dyn_step);

    if (memspace < 0) {
        sprintf(error, "error on opening space for dataset %d", dset);
        return 0;
    }

    if (H5Dread
        (dset, memtype, memspace, dataspace, H5P_DEFAULT,
         (void *) buf) < 0) {
        sprintf(error,
                "get_selecteddata: unable to get data for dataset %d",
                dset);
        return 0;
    }

    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Tclose(datatype);
    H5Dclose(dset);
    DBG(cerr << "<get_slabdata() " << endl);
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn maptogrid(hid_t dataset, int num_dim)
/// obtains number of dimensional scale in the dataset.
///
/// \param dataset original HDF5 dataset name that refers to dimensional scale
/// \param num_dim number of dimensions
/// \return a number
////////////////////////////////////////////////////////////////////////////////
H5GridFlag_t maptogrid(hid_t dataset, int num_dim)
{
    int new_h4h5 = 1;
    H5GridFlag_t to_grid_flag = NotGrid;

    if (map_to_grid(dataset, num_dim, new_h4h5))        // first test new h4h5 tool.
        to_grid_flag = NewH4H5Grid;
    else {                      // now try the old h4toh5 tool.
        new_h4h5 = 0;
        if (map_to_grid(dataset, num_dim, new_h4h5))
            to_grid_flag = OldH4H5Grid;
    }

    return to_grid_flag;

}

////////////////////////////////////////////////////////////////////////////////
/// \fn map_to_grid(hid_t dataset, int num_dim, int new_h4h5)
///  obtains number of dimensional scale in the dataset.
///
///
/// \param dataset original HDF5 dataset name that refers to dimensional scale
/// \param num_dim number of dimensions
/// \param new_h4h5 indicates whether dataset is new dimensional scale format
/// \return a number
////////////////////////////////////////////////////////////////////////////////
int map_to_grid(hid_t dataset, int num_dim, int new_h4h5)
{

    hid_t attr_id;
    hid_t type, space;
    hsize_t ssiz;
    int num_dim1, num_dim2;
    int num_attrs;
    int attr_namesize;
    unsigned int i;
    char dimscale[HDF5_DIMVARLEN];
    int temp_check_dimscale = 0;
    int temp_check_dimscalename = 0;
    int temp_return;

    DBG(cerr << ">map_to_grid()" << endl);
    num_dim1 = num_dim2 = -1;
    num_attrs = H5Aget_num_attrs(dataset);

    // 1.1 Found whether we have attribute "HDF5_DIMENSIONLIST", if no, map to array.
    for (i = 0; i < (unsigned int) num_attrs; i++) {

        attr_id = H5Aopen_idx(dataset, i);
        bzero(dimscale, sizeof(dimscale));
        attr_namesize = H5Aget_name(attr_id, HDF5_DIMVARLEN, dimscale);
        if (attr_namesize < 0) {
            throw
                InternalErr(__FILE__, __LINE__,
                            "error in getting attribute name\n");
        }

        if (new_h4h5
            && !strncmp(dimscale, HDF5_DIMENSIONLIST,
                        strlen(HDF5_DIMENSIONLIST)))
            temp_check_dimscale = 1;
        if (!new_h4h5
            && !strncmp(dimscale, OLD_HDF5_DIMENSIONLIST,
                        strlen(OLD_HDF5_DIMENSIONLIST)))
            temp_check_dimscale = 1;
        if (temp_check_dimscale) {
            type = H5Aget_type(attr_id);
            if (H5Tget_class(type) != H5T_REFERENCE) {
                throw
                    InternalErr(__FILE__, __LINE__,
                                "The type is supposed to be the reference type \n");
            }

            if (!H5Tequal(type, H5T_STD_REF_OBJ)) {
                throw
                    InternalErr(__FILE__, __LINE__,
                                "The type should be the object reference type \n");
            }

            space = H5Aget_space(attr_id);
            // The number of element for HDF5 dimensional object reference array
            // is the number of dimension of HDF5 corresponding array. 
            ssiz = H5Sget_simple_extent_npoints(space);
            num_dim1 = (int) ssiz;
            H5Tclose(type);
            H5Sclose(space);
            H5Aclose(attr_id);
            break;
        }
        H5Aclose(attr_id);
    }
    // 1.2 Found whether we have attribute "DIMENSION_NAMELIST", if no, map to array.
    for (i = 0; i < (unsigned int) num_attrs; i++) {
        attr_id = H5Aopen_idx(dataset, i);
        bzero(dimscale, sizeof(dimscale));
        attr_namesize = H5Aget_name(attr_id, HDF5_DIMVARLEN, dimscale);
        if (attr_namesize < 0) {
            throw
                InternalErr(__FILE__, __LINE__,
                            "error in getting attribute name\n");
        }

        if (new_h4h5
            && !strncmp(dimscale, HDF5_DIMENSIONNAMELIST,
                        strlen(HDF5_DIMENSIONNAMELIST)))
            temp_check_dimscalename = 1;
        if (!new_h4h5
            && !strncmp(dimscale, OLD_HDF5_DIMENSIONNAMELIST,
                        strlen(OLD_HDF5_DIMENSIONNAMELIST)))
            temp_check_dimscalename = 1;


        if (temp_check_dimscalename) {
            type = H5Aget_type(attr_id);
            if (H5Tget_class(type) != H5T_STRING) {
                throw
                    InternalErr(__FILE__, __LINE__,
                                "The type is supposed to be the string type \n");
            }

            space = H5Aget_space(attr_id);
            // number of element for HDF5 dimensional object reference array
            // is the number of dimension of HDF5 corresponding array. 
            ssiz = H5Sget_simple_extent_npoints(space);
            num_dim2 = (int) ssiz;
            H5Tclose(type);
            H5Sclose(space);
            H5Aclose(attr_id);
            break;
        }
        H5Aclose(attr_id);
    }


    temp_return = ((num_dim == num_dim1) ? (num_dim1 == num_dim2) : 0);

    // If no dimensional information was found according to new h4h5tool, check it for the old one. 
    return temp_return;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn correct_name(char *oldname)
/// changes the hdf5 name when the name contains '/' into '_'.
///
/// \param oldname     old name
/// \return the corrected name
////////////////////////////////////////////////////////////////////////////////
char *correct_name(char *oldname)
{

    char *cptr;
    char *newname = NULL;       // <hyokyung 2007.02.27. 10:25:33>
    char ORI_SLASH = '/';
    char CHA_SLASH = '_';

    DBG(cerr << ">correct_name(" << oldname << ")" << endl);
    if (oldname == NULL)
        return NULL;

    // The following code is for correcting name from "/" to "_" 
    newname = (char *) malloc((strlen(oldname) + 1) * sizeof(char));
    bzero(newname, (strlen(oldname) + 1) * sizeof(char));
    newname = strncpy(newname, oldname, strlen(oldname));

    while ((cptr = strchr(newname, ORI_SLASH)) != NULL) {
        *cptr = CHA_SLASH;
    }

    DBG(cerr << "<correct_name=>" << newname << endl);
    return newname;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn get_memtype(hid_t datatype)
/// gets machine architecture specific memory type from general HDF5 type.
/// 
/// \param datatype HDF5 general data type
/// \return platform specific datatype
/// \return -1 if data type mapping is impossible
////////////////////////////////////////////////////////////////////////////////
hid_t get_memtype(hid_t datatype)
{

    H5T_class_t typeclass;
    size_t typesize;

    typesize = H5Tget_size(datatype);
    typeclass = H5Tget_class(datatype);

    // We will only consider H5T_INTEGER, H5T_FLOAT in this case. 

    switch (typeclass) {

    case H5T_INTEGER:

        if (H5Tequal(datatype, H5T_STD_I8BE) ||
            H5Tequal(datatype, H5T_STD_I8LE) ||
            H5Tequal(datatype, H5T_STD_U8BE) ||
            H5Tequal(datatype, H5T_STD_U8LE) ||
            H5Tequal(datatype, H5T_NATIVE_CHAR) ||
            H5Tequal(datatype, H5T_NATIVE_SCHAR) ||
            H5Tequal(datatype, H5T_NATIVE_UCHAR)) {
            if (typesize == H5Tget_size(H5T_NATIVE_CHAR))
                return H5T_NATIVE_CHAR;
            else if (typesize == H5Tget_size(H5T_NATIVE_SHORT))
                return H5T_NATIVE_SHORT;
            else if (typesize == H5Tget_size(H5T_NATIVE_INT))
                return H5T_NATIVE_INT;
            else if (typesize == H5Tget_size(H5T_NATIVE_LONG))
                return H5T_NATIVE_LONG;
            else
                return -1;
        }

        else if (H5Tequal(datatype, H5T_STD_I16BE) ||
                 H5Tequal(datatype, H5T_STD_I16LE) ||
                 H5Tequal(datatype, H5T_NATIVE_SHORT)) {
            if (typesize == H5Tget_size(H5T_NATIVE_CHAR))
                return H5T_NATIVE_CHAR;
            else if (typesize == H5Tget_size(H5T_NATIVE_SHORT))
                return H5T_NATIVE_SHORT;
            else if (typesize == H5Tget_size(H5T_NATIVE_INT))
                return H5T_NATIVE_INT;
            else if (typesize == H5Tget_size(H5T_NATIVE_LONG))
                return H5T_NATIVE_LONG;
            else
                return -1;
        }

        else if (H5Tequal(datatype, H5T_STD_U16BE) ||
                 H5Tequal(datatype, H5T_STD_U16LE) ||
                 H5Tequal(datatype, H5T_NATIVE_USHORT)) {
            if (typesize == H5Tget_size(H5T_NATIVE_UCHAR))
                return H5T_NATIVE_UCHAR;
            else if (typesize == H5Tget_size(H5T_NATIVE_USHORT))
                return H5T_NATIVE_USHORT;
            else if (typesize == H5Tget_size(H5T_NATIVE_UINT))
                return H5T_NATIVE_UINT;
            else if (typesize == H5Tget_size(H5T_NATIVE_ULONG))
                return H5T_NATIVE_ULONG;
            else
                return -1;
        }

        else if (H5Tequal(datatype, H5T_STD_I32BE) ||
                 H5Tequal(datatype, H5T_STD_I32LE) ||
                 H5Tequal(datatype, H5T_NATIVE_INT)) {
            if (typesize == H5Tget_size(H5T_NATIVE_CHAR))
                return H5T_NATIVE_CHAR;
            else if (typesize == H5Tget_size(H5T_NATIVE_SHORT))
                return H5T_NATIVE_SHORT;
            else if (typesize == H5Tget_size(H5T_NATIVE_INT))
                return H5T_NATIVE_INT;
            else if (typesize == H5Tget_size(H5T_NATIVE_LONG))
                return H5T_NATIVE_LONG;
            else if (typesize == H5Tget_size(H5T_NATIVE_LLONG))
                return H5T_NATIVE_LLONG;
            else
                return -1;
        }

        else if (H5Tequal(datatype, H5T_STD_U32BE) ||
                 H5Tequal(datatype, H5T_STD_U32LE) ||
                 H5Tequal(datatype, H5T_NATIVE_UINT)) {
            if (typesize == H5Tget_size(H5T_NATIVE_UCHAR))
                return H5T_NATIVE_UCHAR;
            else if (typesize == H5Tget_size(H5T_NATIVE_USHORT))
                return H5T_NATIVE_USHORT;
            else if (typesize == H5Tget_size(H5T_NATIVE_UINT))
                return H5T_NATIVE_UINT;
            else if (typesize == H5Tget_size(H5T_NATIVE_ULONG))
                return H5T_NATIVE_ULONG;
            else if (typesize == H5Tget_size(H5T_NATIVE_ULLONG))
                return H5T_NATIVE_ULLONG;
            else
                return -1;
        }

        else if (H5Tequal(datatype, H5T_STD_I64BE) ||
                 H5Tequal(datatype, H5T_STD_I64LE) ||
                 H5Tequal(datatype, H5T_NATIVE_LONG) ||
                 H5Tequal(datatype, H5T_NATIVE_LLONG)) {
            if (typesize == H5Tget_size(H5T_NATIVE_CHAR))
                return H5T_NATIVE_CHAR;
            else if (typesize == H5Tget_size(H5T_NATIVE_SHORT))
                return H5T_NATIVE_SHORT;
            else if (typesize == H5Tget_size(H5T_NATIVE_INT))
                return H5T_NATIVE_INT;
            else if (typesize == H5Tget_size(H5T_NATIVE_LONG))
                return H5T_NATIVE_LONG;
            else if (typesize == H5Tget_size(H5T_NATIVE_LLONG))
                return H5T_NATIVE_LLONG;
            else
                return -1;
        }

        else if (H5Tequal(datatype, H5T_STD_U64BE) ||
                 H5Tequal(datatype, H5T_STD_U64LE) ||
                 H5Tequal(datatype, H5T_NATIVE_ULONG) ||
                 H5Tequal(datatype, H5T_NATIVE_ULLONG)) {
            if (typesize == H5Tget_size(H5T_NATIVE_UCHAR))
                return H5T_NATIVE_UCHAR;
            else if (typesize == H5Tget_size(H5T_NATIVE_USHORT))
                return H5T_NATIVE_USHORT;
            else if (typesize == H5Tget_size(H5T_NATIVE_UINT))
                return H5T_NATIVE_UINT;
            else if (typesize == H5Tget_size(H5T_NATIVE_ULONG))
                return H5T_NATIVE_ULONG;
            else if (typesize == H5Tget_size(H5T_NATIVE_ULLONG))
                return H5T_NATIVE_ULLONG;
            else
                return -1;
        }

        else {
            return -1;
        }

        break;

    case H5T_FLOAT:

        if (H5Tequal(datatype, H5T_IEEE_F32BE) ||
            H5Tequal(datatype, H5T_IEEE_F32LE) ||
            H5Tequal(datatype, H5T_NATIVE_FLOAT)) {
            if (typesize == H5Tget_size(H5T_NATIVE_FLOAT))
                return H5T_NATIVE_FLOAT;
            else if (typesize == H5Tget_size(H5T_NATIVE_DOUBLE))
                return H5T_NATIVE_DOUBLE;
            else
                return -1;
        }

        else if (H5Tequal(datatype, H5T_IEEE_F64BE) ||
                 H5Tequal(datatype, H5T_IEEE_F64LE) ||
                 H5Tequal(datatype, H5T_NATIVE_DOUBLE)) {
            if (typesize == H5Tget_size(H5T_NATIVE_FLOAT))
                return H5T_NATIVE_FLOAT;
            else if (typesize == H5Tget_size(H5T_NATIVE_DOUBLE))
                return H5T_NATIVE_DOUBLE;
            else
                return -1;
        }

        else
            return -1;

        break;

    case H5T_STRING:

        return H5T_STRING;
        break;

    default:
        return -1;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn check_h5str(hid_t h5type)
/// checks if type is HDF5 string type
/// 
/// \param h5type data type id
/// \return 1 if type is string
/// \return 0 otherwise
////////////////////////////////////////////////////////////////////////////////
int check_h5str(hid_t h5type)
{
    if (H5Tget_class(h5type) == H5T_STRING)
        return 1;
    else
        return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn has_matching_grid_dimscale(hid_t dataset, int ndims, int* sizes)
/// checks if dataset has an attribute called "DIMENSION_LIST" and matching indexes.
/// 
/// \param dataset dataset id
/// \param ndims number of dimensions
/// \param sizes size of each dimension
/// \return 1 if it has an attribute called "DIMENSION_LIST" and matching indexes.
/// \return 0 otherwise
////////////////////////////////////////////////////////////////////////////////
bool has_matching_grid_dimscale(hid_t dataset, int ndims, int *sizes)
{
    bool flag = false;

    char dimscale[HDF5_DIMVARLEN];

    hid_t attr_id;

    int attr_namesize;
    int i;
    int num_attrs;

    num_attrs = H5Aget_num_attrs(dataset);
    DBG(cerr << ">has_matching_grid_dimscale"
        << " ndims=" << ndims << " sizes[0]=" << sizes[0]
        << endl;
        )
        // Check "DIMENSION_LIST" attribute.
        for (i = 0; i < num_attrs; i++) {
        attr_id = H5Aopen_idx(dataset, i);
        bzero(dimscale, sizeof(dimscale));
        attr_namesize = H5Aget_name(attr_id, HDF5_DIMVARLEN, dimscale);
        if (attr_namesize < 0) {
            throw
                InternalErr(__FILE__, __LINE__,
                            "error in getting attribute name\n");
        }

        if (!strncmp(dimscale, "DIMENSION_LIST", strlen("DIMENSION_LIST"))) {
            DBG(cerr << "=has_matching_grid_dimscale():Got a grid:" << i <<
                ":" << dimscale << endl);
            flag = true;
        }
        H5Aclose(attr_id);
    }
    // Check number of dimensions.
    if (flag) {
        if ((attr_id = H5Aopen_name(dataset, "DIMENSION_LIST")) < 0) {
            throw
                InternalErr(__FILE__, __LINE__,
                            "Unable to open the attribute with the name as DIMENSION_LIST");
        }
        hid_t temp_dtype = H5Aget_type(attr_id);
        hid_t temp_dspace = H5Aget_space(attr_id);
        hsize_t temp_nelm = H5Sget_simple_extent_npoints(temp_dspace);
        if (ndims != (int) temp_nelm) {
            return false;
        }

        hvl_t *refbuf =
            (hvl_t *) calloc((size_t) temp_nelm, sizeof(hvl_t));

        if (H5Aread(attr_id, temp_dtype, refbuf) < 0) {
            cerr << "Cannot read object reference attributes." << endl;
        }

        hid_t *dimid = (hid_t *) malloc(sizeof(hid_t) * temp_nelm);

        // Check size of each dimension.
        for (int j = 0; j < (int) temp_nelm; j++) {
            DBG(cerr << "refbuf[" << j << "].len=" << (int) (refbuf[j].len)
                << endl);
            if ((int) refbuf[j].len > 0) {
                dimid[j] =
                    H5Rdereference(attr_id, H5R_OBJECT, refbuf[j].p);
                DBG(cerr << "dimid[" << j << "]=" << dimid[j] << endl);

                if (dimid[j] < 0) {
                    return false;
                } else {
                    char buf[DODS_NAMELEN];
                    H5Iget_name(dimid[j], (char *) buf, DODS_NAMELEN);
                    hid_t index_dset_id = H5Dopen(dataset, buf);
                    hid_t index_dspace = H5Dget_space(index_dset_id);
                    hsize_t index_ndim =
                        H5Sget_simple_extent_npoints(index_dspace);
                    if ((int) index_ndim != sizes[j]) {
                        flag = false;
                    }
                }

            } else {
                flag = false;
            }
        }                       // for (int j = 0; j < temp_nelm; j++)
        free(refbuf);
        free(dimid);
        H5Aclose(attr_id);
        H5Sclose(temp_dspace);
        H5Tclose(temp_dtype);
    }                           //  if(flag)

    return flag;
}

// $Log$ //
