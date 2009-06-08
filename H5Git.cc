///////////////////////////////////////////////////////////////////////////////
/// \file H5Git.cc
///  iterates all HDF5 internals.
/// 
///  This file includes all the routines to search HDF5 group, dataset, links,
///  and attributes. since we are using HDF5 C APIs, we include all c functions
///  in this file.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang    <myang6@hdfgroup.org>
///
///  Copyright (C) 2007-2009  The HDF Group
///
///  Copyright (C) 1999 National Center for Supercomputing Applications.
///                     All rights reserved.
///
///////////////////////////////////////////////////////////////////////////////

#include "H5Git.h"

using namespace libdap;

///////////////////////////////////////////////////////////////////////////////
/// \fn get_attr_info(hid_t dset, int index, DSattr_t *attr_inst_ptr,
///                  int *ignoreptr)
///  will get attribute information.
///
/// This function will get attribute information: datatype, dataspace(dimension
/// sizes) and number of dimensions and put it into a data struct.
///
/// \param[in]  dset  dataset id
/// \param[in]  index  index of attribute
/// \param[out] attr_inst_ptr an attribute instance pointer
/// \param[out] ignoreptr  a flag to record whether it can be ignored.
/// \return pointer to attribute structure
/// \throw InternalError 
///////////////////////////////////////////////////////////////////////////////
hid_t get_attr_info(hid_t dset, int index, DSattr_t * attr_inst_ptr,
                    int *ignoreptr)
{
    *ignoreptr = 0;
    hid_t attrid;
    if ((attrid = H5Aopen_idx(dset, index)) < 0) {
        string msg = "unable to open attribute by index ";
        msg += index;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    // Obtain the size of attribute name.
    ssize_t name_size =  H5Aget_name(attrid, 0, NULL);
    if (name_size < 0) {
        string msg = "unable to obtain hdf5 attribute name size for id ";
        msg += attrid;
        throw InternalErr(__FILE__, __LINE__, msg);
    };
    char* namebuf = new char[name_size+1];
    // Obtain the attribute name.    
    if ((H5Aget_name(attrid, name_size+1, namebuf)) < 0) {
        string msg = "unable to obtain hdf5 attribute name for id ";
        msg += attrid;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    if ((attrid = H5Aopen_name(dset, namebuf)) < 0) {
        string msg = "unable to obtain hdf5 attribute by name ";
        msg += namebuf;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    // Obtain the type of the attribute. 
    hid_t ty_id;
    if ((ty_id = H5Aget_type(attrid)) < 0) {
        string msg = "unable to obtain hdf5 attribute type for id ";
        msg += attrid;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    H5T_class_t temp_type = H5Tget_class(ty_id);
    if (temp_type < 0) {
        string msg = "unable to obtain hdf5 datatype class for type_id ";
        msg += ty_id;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    if ((temp_type == H5T_TIME) || (temp_type == H5T_BITFIELD)
        || (temp_type == H5T_OPAQUE) || (temp_type == H5T_ENUM)
        || (temp_type == H5T_REFERENCE)) {
        *ignoreptr = 1;
        attrid = 0;

        return attrid;
    }
        
    hid_t space;
    if ((space = H5Aget_space(attrid)) < 0) {
        string msg = "unable to obtain hdf5 data space for id ";
        msg += attrid;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    hsize_t size[DODS_MAX_RANK];
    hsize_t maxsize[DODS_MAX_RANK];
    int ndims = H5Sget_simple_extent_dims(space, size, maxsize);
    // Check dimension size. 
    if (ndims > DODS_MAX_RANK) {
        string msg = "number of dimensions exceeds allowed ";
        msg += attrid;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    // Return ndims and size[ndims]. 
    hsize_t nelmts = 1;
    if (ndims) {
        for (int j = 0; j < ndims; j++)
            nelmts *= size[j];
    }

    if (ndims < 0){
	string msg = "number of dimensions are less than allowed ";
	msg += attrid;
	throw InternalErr(__FILE__, __LINE__, msg);
    }

    size_t need = nelmts * H5Tget_size(ty_id);

    if (need == 0){
	throw InternalErr(__FILE__, __LINE__, "H5Tget_size() failed.");
    }
    // We want to save memory type in the struct.
    hid_t memtype = H5Tget_native_type(ty_id, H5T_DIR_ASCEND);

    if (memtype < 0){
	throw InternalErr(__FILE__, __LINE__, "H5Tget_native_type() failed");
    }
    (*attr_inst_ptr).type = memtype;
    (*attr_inst_ptr).ndims = ndims;
    (*attr_inst_ptr).nelmts = nelmts;
    (*attr_inst_ptr).need = need;
    strncpy((*attr_inst_ptr).name, namebuf, name_size+1);

    for (int j = 0; j < ndims; j++) {
        (*attr_inst_ptr).size[j] = size[j];
    }

    return attrid;
}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_dap_type(hid_t type)
/// returns the string representation of HDF5 type.
///
/// This function will get the text representation(string) of the corresponding
/// DODS datatype. DODS-HDF5 subclass method will use this function.
///
/// \return string
/// \param type datatype id
///////////////////////////////////////////////////////////////////////////////
string get_dap_type(hid_t type)
{
    size_t size = 0;
    H5T_sign_t sign;

    switch (H5Tget_class(type)) {

    case H5T_INTEGER:
        size = H5Tget_size(type);
        sign = H5Tget_sign(type);
        DBG(cerr << "=get_dap_type(): H5T_INTEGER" <<
            " sign = " << sign <<
            " size = " << size <<
            endl);
        if (size == 1){
            if (sign == H5T_SGN_NONE)      
                return BYTE;    
            else
                return INT16;
        }

        if (size == 2) {
            if (sign == H5T_SGN_NONE)
                return UINT16;
            else
                return INT16;
        }

        if (size == 4) {
            if (sign == H5T_SGN_NONE)
                return UINT32;
            else
                return INT32;
        }

	if (size < 0){
	    throw InternalErr(__FILE__, __LINE__, "size of datatype is invalid"); 
	}

        return INT_ELSE;

    case H5T_FLOAT:
        size = H5Tget_size(type);
        DBG(cerr << "=get_dap_type(): FLOAT size = " << size << endl);
        if (size == 4)
            return FLOAT32;
        if (size == 8)
            return FLOAT64;
	if (size < 0){
            throw InternalErr(__FILE__, __LINE__, "size of datatype is invalid");
        }
        return FLOAT_ELSE;

    case H5T_STRING:
        return STRING;

    case H5T_REFERENCE:
        return URL;

    case H5T_COMPOUND:
        return COMPOUND;

    case H5T_ARRAY:
        return ARRAY;

    default:
        return "Unmappable Type";
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_fileid(const char *filename)
/// gets HDF5 file id.
/// 
/// This function is used because H5Fopen cannot be directly used in a C++
/// code.
/// \param filename HDF5 filename
/// \return a file handler id
///////////////////////////////////////////////////////////////////////////////
hid_t get_fileid(const char *filename)
{
    return H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_dataset(hid_t pid, const string &dname, DS_t * dt_inst_ptr)
/// obtain data information in a dataset datatype, dataspace(dimension sizes)
/// and number of dimensions and put these information into a pointer of data
/// struct.
///
/// \param[in] pid    parent object id(group id)
/// \param[in] dname  dataset name
/// \param[out] dt_inst_ptr  pointer to the attribute struct(* attr_inst_ptr)
///////////////////////////////////////////////////////////////////////////////
void get_dataset(hid_t pid, const string &dname, DS_t * dt_inst_ptr)
{
    DBG(cerr << ">get_dataset()" << endl);
    hid_t dset;
    if ((dset = H5Dopen(pid, dname.c_str())) < 0) {
        throw Error(string("Could not open: ") + dname);
    }

    hid_t datatype;
    if ((datatype = H5Dget_type(dset)) < 0) {
        throw InternalErr(__FILE__, __LINE__, 
                          string("could not get data type from ") + dname);
    }
    
    hid_t dataspace;
    if ((dataspace = H5Dget_space(dset)) < 0) {
        throw InternalErr(__FILE__, __LINE__,
                          string("could not get data space from ") + dname);
    }

    H5T_class_t temp_type = H5Tget_class(datatype);
    if (temp_type < 0) {
        throw InternalErr(__FILE__, __LINE__,
                          string("could not get type class from ") + dname);
    }
    if ((temp_type == H5T_TIME) || (temp_type == H5T_BITFIELD)
        || (temp_type == H5T_OPAQUE) || (temp_type == H5T_ENUM)) {
        throw InternalErr(__FILE__, __LINE__, "unexpected type");
    }

    int ndims;
    hsize_t size[DODS_MAX_RANK];
    hsize_t maxsize[DODS_MAX_RANK];
    // Obtain number of attributes in this dataset. 
    if ((ndims = H5Sget_simple_extent_dims(dataspace, size, maxsize)) < 0) {
        throw InternalErr(__FILE__, __LINE__,
                          "could not get the number of dimensions");
    }
    // check dimension size. 
    if (ndims > DODS_MAX_RANK) {
        throw InternalErr(__FILE__, __LINE__,
                          "number of dimensions exceeds allowed");
    }
    // return ndims and size[ndims]. 
    hsize_t nelmts = 1;
    if (ndims) {
        for (int j = 0; j < ndims; j++)
            nelmts *= size[j];
    }

    size_t need = nelmts * H5Tget_size(datatype);
    if (need == 0){
	throw InternalErr(__FILE__, __LINE__, "H5Tget_size() failed");
    }
    hid_t memtype = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
    if (memtype < 0){
	throw InternalErr(__FILE__, __LINE__, "H5Tget_native_type() failed");
    }
    (*dt_inst_ptr).dset = dset;
    (*dt_inst_ptr).dataspace = dataspace;
    (*dt_inst_ptr).type = memtype;
    (*dt_inst_ptr).ndims = ndims;
    (*dt_inst_ptr).nelmts = nelmts;
    (*dt_inst_ptr).need = need;
    strncpy((*dt_inst_ptr).name, dname.c_str(), dname.length());
    (*dt_inst_ptr).name[dname.length()] = '\0';

    for (int j = 0; j < ndims; j++) {
        (*dt_inst_ptr).size[j] = size[j];
    }

    DBG(cerr << "<get_dataset() dimension=" << ndims << " elements=" <<
        nelmts << endl);
}


///////////////////////////////////////////////////////////////////////////////
/// \fn get_data(hid_t dset, void *buf)
/// will get all data of a \a dset dataset and put it into \a buf.
///
///
/// \param[in] dset dataset id(dset)
/// \param[out] buf pointer to a buffer
///////////////////////////////////////////////////////////////////////////////
void get_data(hid_t dset, void *buf)
{
    DBG(cerr << ">get_data()" << endl);
    hid_t datatype;
    if ((datatype = H5Dget_type(dset)) < 0) {
        throw InternalErr(__FILE__, __LINE__, "failed to get data type");
    }
    hid_t dataspace;
    if ((dataspace = H5Dget_space(dset)) < 0) {
        throw InternalErr(__FILE__, __LINE__, "failed to get data space");
    }
    //  Use HDF5 H5Tget_native_type API
    hid_t memtype = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
    if (memtype < 0) {
        throw InternalErr(__FILE__, __LINE__, "failed to get memory type");
    }
    //  For some reason, if you must handle the H5T_STRING type differently.
    //  Otherwise, the "tstring-at.h5" test will fail.
    if (memtype == H5T_STRING) {
        DBG(cerr << "=get_data(): H5T_STRING type is detected." << endl);
        if (H5Dread(dset, datatype, dataspace, dataspace, H5P_DEFAULT, buf)
            < 0) {
            throw InternalErr(__FILE__, __LINE__, "failed to read data");
        }
    } 
    else {
        DBG(cerr << "=get_data(): H5T_STRING type is NOT detected." << endl);
        if (H5Dread(dset, memtype, dataspace, dataspace, H5P_DEFAULT, buf)
            < 0) {
            throw InternalErr(__FILE__, __LINE__, "failed to read data");
        }

    }
    // This I don't understand... jhrg 4/16/08
    // If you remove the following if(){} block, the "tstring-at.h5" test
    // will fail. 
    if (H5Tget_class(datatype) != H5T_STRING) {

        H5Sclose(dataspace);
        H5Tclose(datatype);
        H5Dclose(dset);

    }

    DBG(cerr << "<get_data()" << endl);
}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_strdata(int strindex, char *allbuf, char *buf, int elesize)
/// will get an individual string data from all string data elements and put
/// it into buf. 
///
/// \param[in] strindex index of H5T_STRING array
/// \param[in] allbuf pointer to string buffer that has been built so far
/// \param[in] elesize size of string element in the array
/// \param[out] buf pointer to a buf
/// \return void
///////////////////////////////////////////////////////////////////////////////
void get_strdata(int strindex, char *allbuf, char *buf, int elesize)
{
    char *tempvalue = allbuf;   // The beginning of entire buffer.

    DBG(cerr << ">get_strdata(): "
        << " strindex=" << strindex << " allbuf=" << allbuf << endl);

    // Tokenize the convbuf. 
    for (int i = 0; i < strindex; i++) {
        tempvalue = tempvalue + elesize;
    }

    strncpy(buf, tempvalue, elesize);
    buf[elesize] = '\0';        
}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_slabdata(hid_t dset, int *offset, int *step, int *count, 
///                  int num_dim, void *buf)
/// will get hyperslab data of a dataset and put it into buf.
///
/// \param[in] dset dataset id
/// \param[in] offset starting point
/// \param[in] step  stride
/// \param[in] count  count
/// \param[in] num_dim  number of array dimensions
/// \param[out] buf pointer to a buffer
///////////////////////////////////////////////////////////////////////////////
int
get_slabdata(hid_t dset, int *offset, int *step, int *count, int num_dim,
             void *buf)
{
    DBG(cerr << ">get_slabdata() " << endl);

    hid_t datatype = H5Dget_type(dset);
    if (datatype < 0) {
        throw InternalErr(__FILE__, __LINE__, "could not get data type");
    }
    // Using H5T_get_native_type API
    hid_t memtype = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
    if (memtype < 0) {
        throw InternalErr(__FILE__, __LINE__, "could not get memory type");
    }

    hid_t dataspace = H5Dget_space(dset);
    if (dataspace < 0) {
        throw InternalErr(__FILE__, __LINE__, "could not get data space");
    }

    hsize_t *dyn_count = 0;
    hsize_t *dyn_step = 0;
    hssize_t *dyn_offset = 0;
    try {
        dyn_count = new hsize_t[num_dim];
        dyn_step = new hsize_t[num_dim];
        dyn_offset = new hssize_t[num_dim];

        for (int i = 0; i < num_dim; i++) {
            dyn_count[i] = (hsize_t) (*count);
            dyn_step[i] = (hsize_t) (*step);
            dyn_offset[i] = (hssize_t) (*offset);
            DBG(cerr
                << "count:" << dyn_count[i]
                << " step:" << dyn_step[i]
                << " offset:" << dyn_step[i]
                << endl);
            count++;
            step++;
            offset++;
        }

        if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, 
                                (const hsize_t *)dyn_offset, dyn_step,
                                dyn_count, NULL) < 0) {
            throw
                InternalErr(__FILE__, __LINE__, "could not select hyperslab");
        }

        hid_t memspace = H5Screate_simple(num_dim, dyn_count, NULL);
        if (memspace < 0) {
            throw InternalErr(__FILE__, __LINE__, "could not open space");
        }

        delete[] dyn_count;
        delete[] dyn_step;
        delete[] dyn_offset;

        if (H5Dread(dset, memtype, memspace, dataspace, H5P_DEFAULT,
                    (void *) buf) < 0) {
            throw InternalErr(__FILE__, __LINE__, "could not get data");
        }

        H5Sclose(dataspace);
        H5Sclose(memspace);
        H5Tclose(datatype);
        H5Dclose(dset);
    }
    catch (...) {
        // Memory allocation exceptions could have been thrown when
        // creating these, so check if these are not null before deleting.
        if( dyn_count ) delete[] dyn_count;
        if( dyn_step ) delete[] dyn_step;
        if( dyn_offset ) delete[] dyn_offset;

        throw;
    }
    DBG(cerr << "<get_slabdata() " << endl);
}

///////////////////////////////////////////////////////////////////////////////
/// \fn maptogrid(hid_t dataset, int num_dim)
/// obtains number of dimensional scale in the dataset.
///
/// \param dataset original HDF5 dataset name that refers to dimensional scale
/// \param num_dim number of dimensions
/// \return NotGrid, NewH4H5Grid or OldH4H5Grid (values of H5GridFlag_t)
///////////////////////////////////////////////////////////////////////////////
H5GridFlag_t maptogrid(hid_t dataset, int num_dim)
{
    int new_h4h5 = 1;
    if (map_to_grid(dataset, num_dim, new_h4h5)) // First, test new h4h5 tool.
        return NewH4H5Grid;

    new_h4h5 = 0;
    if (map_to_grid(dataset, num_dim, new_h4h5))
        return OldH4H5Grid;
        
    return NotGrid;
}

///////////////////////////////////////////////////////////////////////////////
/// \fn map_to_grid(hid_t dataset, int num_dim, int new_h4h5)
///  obtains number of dimensional scale in the dataset.
///
///
/// \param dataset original HDF5 dataset name that refers to dimensional scale
/// \param num_dim number of dimensions
/// \param new_h4h5 indicates whether dataset is new dimensional scale format
/// \return a number
///////////////////////////////////////////////////////////////////////////////
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
    if (num_attrs < 0){
	throw InternalErr(__FILE__, __LINE__, "Invalid number of attributes");
    }
    // 1.1 Found whether we have attribute "HDF5_DIMENSIONLIST", if no, map
    // to array.
    for (i = 0; i < (unsigned int) num_attrs; i++) {
        attr_id = H5Aopen_idx(dataset, i);
	if (attr_id < 0){
	   throw InternalErr(__FILE__, __LINE__, "cannot open the attribute specified");
	}
        memset(dimscale, 0, sizeof(dimscale));
        attr_namesize = H5Aget_name(attr_id, HDF5_DIMVARLEN, dimscale);
        if (attr_namesize < 0) {
            throw InternalErr(__FILE__, __LINE__, "error getting attr name");
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
                throw InternalErr(__FILE__, __LINE__,
                                  "The type should be the reference type");
            }

            if (!H5Tequal(type, H5T_STD_REF_OBJ)) {
                throw InternalErr(__FILE__, __LINE__,
                                  "The type should be the object reference type");
            }
 
            space = H5Aget_space(attr_id);
	    if (space < 0){
                 throw InternalErr(__FILE__, __LINE__, "Invalid space");
            }
            // The number of element for HDF5 dimensional object reference
            // array is the number of dimension of HDF5 corresponding array. 
            ssiz = H5Sget_simple_extent_npoints(space);
	    if (ssiz == 0){
               throw InternalErr(__FILE__, __LINE__, "Invalid number of elements in the dataspace");
            }
            num_dim1 = (int) ssiz;
            H5Tclose(type);
            H5Sclose(space);
            H5Aclose(attr_id);
            break;
        }
        H5Aclose(attr_id);
    }
    // 1.2 Found whether we have attribute "DIMENSION_NAMELIST", if no, map
    // to array.
    for (i = 0; i < (unsigned int) num_attrs; i++) {
        attr_id = H5Aopen_idx(dataset, i);
        if (attr_id < 0){
           throw InternalErr(__FILE__, __LINE__, "cannot open the attribute specified");
        }
        memset(dimscale, 0, sizeof(dimscale));
        attr_namesize = H5Aget_name(attr_id, HDF5_DIMVARLEN, dimscale);
        if (attr_namesize < 0) {
            throw
                InternalErr(__FILE__, __LINE__, "error in getting attr name");
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
                throw InternalErr(__FILE__, __LINE__,
                                  "The type shouldbe the string type");
            }

            space = H5Aget_space(attr_id);
            if (space < 0){
                 throw InternalErr(__FILE__, __LINE__, "Invalid space");
            }
	    // number of element for HDF5 dimensional object reference array
            // is the number of dimension of HDF5 corresponding array. 
            ssiz = H5Sget_simple_extent_npoints(space);
             if (ssiz == 0){
               throw InternalErr(__FILE__, __LINE__, "Invalid number of elements in the dataspace");
            }
	    num_dim2 = (int) ssiz;
            H5Tclose(type);
            H5Sclose(space);
            H5Aclose(attr_id);
            break;
        }
        H5Aclose(attr_id);
    }


    temp_return = ((num_dim == num_dim1) ? (num_dim1 == num_dim2) : 0);

    // If no dimensional information was found according to new h4h5tool,
    // check it for the old one.
    return temp_return;
}


///////////////////////////////////////////////////////////////////////////////
/// \fn check_h5str(hid_t h5type)
/// checks if type is HDF5 string type
/// 
/// \param h5type data type id
/// \return 1 if type is string
/// \return 0 otherwise
///////////////////////////////////////////////////////////////////////////////
int check_h5str(hid_t h5type)
{
    if (H5Tget_class(h5type) == H5T_STRING)
        return 1;
    else
        return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \fn has_matching_grid_dimscale(hid_t dataset, int ndims, int* sizes)
/// checks if dataset has an attribute called "DIMENSION_LIST" and matching
/// indexes.
/// 
/// \param dataset dataset id
/// \param ndims number of dimensions
/// \param sizes size of each dimension
/// \return 1 if it has an attribute called "DIMENSION_LIST" and matching
///           indexes.
/// \return 0 otherwise
///////////////////////////////////////////////////////////////////////////////
bool has_matching_grid_dimscale(hid_t dataset, int ndims, int *sizes)
{
    bool flag = false;

    char dimscale[HDF5_DIMVARLEN];

    hid_t attr_id;

    int attr_namesize;
    int i;
    int num_attrs;

    num_attrs = H5Aget_num_attrs(dataset);
    if (num_attrs < 0){
               throw InternalErr(__FILE__, __LINE__, "Invalid number of attributes");
            }
    DBG(cerr << ">has_matching_grid_dimscale"
        << " ndims=" << ndims << " sizes[0]=" << sizes[0]
        << endl);
    // Check "DIMENSION_LIST" attribute.
    for (i = 0; i < num_attrs; i++) {
        attr_id = H5Aopen_idx(dataset, i);
	if (attr_id < 0){
               throw InternalErr(__FILE__, __LINE__, "Cannot open the attribute");
        }
        memset(dimscale, 0, sizeof(dimscale));
        attr_namesize = H5Aget_name(attr_id, HDF5_DIMVARLEN, dimscale);
        if (attr_namesize < 0) {
            throw
                InternalErr(__FILE__, __LINE__, "error in getting attr name");
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
            throw InternalErr(__FILE__, __LINE__,
                              "Unable to open the DIMENSION_LIST attribute");
        }
        hid_t temp_dtype = H5Aget_type(attr_id);
	hid_t temp_dspace = H5Aget_space(attr_id);
        hsize_t temp_nelm = H5Sget_simple_extent_npoints(temp_dspace);

	if (temp_dtype < 0){
               throw InternalErr(__FILE__, __LINE__, "Cannot get the attribute datatype");
        }
	if (temp_dspace < 0){
               throw InternalErr(__FILE__, __LINE__, "Cannot get the copy of dataspace");
        }
	if (temp_nelm == 0){
               throw InternalErr(__FILE__, __LINE__, "Cannot determine the number of elements in the dataspace");
        }

        if (ndims != (int) temp_nelm) {
            return false;
        }

        hvl_t *refbuf = 0;
        hid_t *dimid = 0;
        try {
            hvl_t *refbuf = new hvl_t[temp_nelm];
            if (H5Aread(attr_id, temp_dtype, refbuf) < 0) {
                delete[] refbuf;
                throw InternalErr(__FILE__, __LINE__,
                                  "Cannot read object reference attributes");
            }

            hid_t *dimid = new hid_t[temp_nelm];

            // Check size of each dimension.
            for (int j = 0; j < (int) temp_nelm; j++) {
                DBG(cerr << "refbuf[" << j << "].len=" << (int) (refbuf[j].len)
                    << endl);
                if ((int) refbuf[j].len > 0) {
                    dimid[j] =
                        H5Rdereference(attr_id, H5R_OBJECT, refbuf[j].p);
                    DBG(cerr << "dimid[" << j << "]=" << dimid[j] << endl);

                    if (dimid[j] < 0) {
                        delete[] dimid;
                        delete[] refbuf;
                        return false;
                    } else {
                        char buf[DODS_NAMELEN];
                        H5Iget_name(dimid[j], (char *) buf, DODS_NAMELEN);
                        hid_t index_dset_id = H5Dopen(dataset, buf);
                        hid_t index_dspace = H5Dget_space(index_dset_id);
                        hsize_t index_ndim =
                            H5Sget_simple_extent_npoints(index_dspace);

			if (index_dset_id < 0){
               		    throw InternalErr(__FILE__, __LINE__, "H5Dopen() failed");
        		}
        		if (index_dspace < 0){
             		    throw InternalErr(__FILE__, __LINE__,"H5Dget_space() failed");
         		}
        		if (index_ndim == 0){
               		    throw InternalErr(__FILE__, __LINE__, "Cannot determine the number of elements in the dataspace");
        		}
	
                        if ((int) index_ndim != sizes[j]) {
                            flag = false;
                        }
                    }

                } 
                else {
                    flag = false;
                }
            }                       // for (int j = 0; j < temp_nelm; j++)

            H5Aclose(attr_id);
            H5Sclose(temp_dspace);
            H5Tclose(temp_dtype);

            delete[] refbuf; refbuf = 0 ;
            delete[] dimid; dimid = 0 ;
        }                           // try
        catch (...) {
            // memory allocation exceptions could have been thrown
            // when creating these two ptrs, so check if exist
            // before deleting.
            if( refbuf ) delete[] refbuf;
            if( dimid ) delete[] dimid;

            throw;
        }
    }                               // if(flag)

    return flag;
}

