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
///  Copyright (C) 2007	HDFGroup, Inc.
///
///  Copyright (C) 1999	National Center for Supercomputing Applications.
///			All rights reserved.
///
////////////////////////////////////////////////////////////////////////////////

// \todo  Should check the current APIs to see if it needs to be updated.
// <hyokyung 2007.02.20. 13:33:06>


// #define DODS_DEBUG

#include <string.h>
#include <hdf5.h>
#include "debug.h"
#include "H5Git.h"
#include "InternalErr.h" // <hyokyung 2007.02.23. 14:17:32>

// #ifndef FALSE
// #define FALSE 0
// #endif

////////////////////////////////////////////////////////////////////////////////
/// \fn get_attr_info(hid_t dset, int index, DSattr_t *attr_inst_ptr,
///                  int *ignoreptr, char *error)
///  will get attribute information.
///
/// This function will get attribute information: datatype, dataspace(dimension
/// sizes) and number of dimensions and put it into a data struct.
///
/// \param dset  parent object id
/// \param index parent object index
/// \return pointer to attribute structure
/// \throw InternalError 
////////////////////////////////////////////////////////////////////////////////
hid_t
get_attr_info(hid_t dset, int index, DSattr_t * attr_inst_ptr,
              int *ignoreptr, char *error)
{

    hid_t ty_id, attrid, space;
    H5T_class_t temp_type;
    hsize_t size[DODS_MAX_RANK];
#if 0
    , dim_n_size;
#endif
    hsize_t maxsize[DODS_MAX_RANK];
#if 0
    char *attr_name;
#endif
    char *namebuf;
    size_t need;
    hsize_t nelmts = 1;

    // size_t need; 
    int j, ndims;

    *ignoreptr = 0;
    namebuf = (char*)malloc(DODS_NAMELEN);

    if ((attrid = H5Aopen_idx(dset, index)) < 0) {
      	string msg =
	    "dap_h5_handler: unable to open attribute by index ";
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
	(temp_type == H5T_OPAQUE) ||
	// (temp_type == H5T_COMPOUND)|| <hyokyung 2007.03. 8. 11:02:12>
	(temp_type == H5T_ENUM)) {
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
    
#if 0
    // JRB - this test is unnecessary for DODS/OpenDAP.  Since we are
    // read-only, we don't care if any dimensions are unlimited or not
    for (j = 0; j < ndims; j++) {
        if (maxsize[j] == H5S_UNLIMITED) {
            strcpy(error,
                   "unexpected length of dimensions for hdf5_das server");
            attrid = -1;
            goto exit;
        }

    }
#endif

    // return ndims and size[ndims]. 
    if (ndims) {
        for (j = 0; j < ndims; j++)
            nelmts *= size[j];
    }

    need = nelmts * H5Tget_size(ty_id);
    (*attr_inst_ptr).type = ty_id;
    (*attr_inst_ptr).ndims = ndims;
    (*attr_inst_ptr).nelmts = nelmts;
    (*attr_inst_ptr).need = need;
    strcpy((*attr_inst_ptr).name, namebuf);

    for (j = 0; j < ndims; j++) {
        (*attr_inst_ptr).size[j] = size[j];
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
/// \return	dataset id	
////////////////////////////////////////////////////////////////////////////////
hid_t get_dataset(hid_t pid, char *dname, DS_t * dt_inst_ptr, char *error)
{

    hid_t dset = -1;
    hid_t datatype, dataspace;
    H5T_class_t temp_type;
    hsize_t size[DODS_MAX_RANK];
    hsize_t maxsize[DODS_MAX_RANK];
    char *namebuf;
    size_t need;
    hsize_t nelmts = 1;
    int j, ndims;
    int buf_size = 30;

    DBG(cerr << "<get_dataset()" << endl);
    if ((dset = H5Dopen(pid, dname)) < 0) {
        sprintf(error, "h5_dds handler:  failed to obtain dataset %s",
                dname);
        return -1;
    }

    if ((datatype = H5Dget_type(dset)) < 0) {
        sprintf(error,
                "h5_dds handler:  failed to obtain datatype from  dataset %s",
                dname);
        return -1;
    }

    if ((dataspace = H5Dget_space(dset)) < 0) {
        sprintf(error,
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
	(temp_type == H5T_BITFIELD)||
	(temp_type == H5T_OPAQUE) ||
	// (temp_type == H5T_COMPOUND) || <hyokyung 2007.03. 1. 15:12:57>
	(temp_type == H5T_ENUM) ||
	(temp_type == H5T_REFERENCE)) {
        //  <hyokyung 2007.03. 1. 15:10:03>
        sprintf(error, "h5_dds handler: get_data0 - unexpected datatype at temp_type = %d", temp_type);
        return -1;
    }


    // Obtain number of attributes in this dataset. 
    if ((ndims = H5Sget_simple_extent_dims(dataspace, size, maxsize)) < 0) {
        strcpy(error, "h5_dds handler: get_data0 - unable to get number of dimensions");
        return -1;
    }

    // check dimension size. 
    if (ndims > DODS_MAX_RANK) {
        strcpy(error,
               "number of dimensions exceeds hdf5-dods server allowed");
        return -1;
    }

    namebuf = (char*) malloc(buf_size);

#if 0
    // JRB - this test is unnecessary for DODS/OpenDAP.  Since we are
    // read-only, we don't care if any dimensions are unlimited or not
    for (j = 0; j < ndims; j++) {
        if (maxsize[j] == H5S_UNLIMITED) {
            strcpy(error,
                   "unexpected length of dimensions for hdf5-dods server");
            return -1;
        }
    }
#endif

    // return ndims and size[ndims]. 
    if (ndims) {
        for (j = 0; j < ndims; j++)
            nelmts *= size[j];
    }

    need = nelmts * H5Tget_size(datatype);
    (*dt_inst_ptr).dset = dset;
    (*dt_inst_ptr).dataspace = dataspace;
    (*dt_inst_ptr).type = datatype;
    (*dt_inst_ptr).ndims = ndims;
    (*dt_inst_ptr).nelmts = nelmts;
    (*dt_inst_ptr).need = need;
    strcpy((*dt_inst_ptr).name, dname);

    for (j = 0; j < ndims; j++) {
        (*dt_inst_ptr).size[j] = size[j];
    }
    DBG(cerr << ">get_dataset()" << endl);
    return dset;
}


////////////////////////////////////////////////////////////////////////////////
/// \fn get_data(hid_t dset, void *buf, char *error)
/// will get all data of a dataset and put it into buf.
///
/// \param[in] dset dataset id(dset)
/// \param[out] buf pointer to a buffer
/// \return -1, if failed.
/// \return 0, if succeeded.
////////////////////////////////////////////////////////////////////////////////
int get_data(hid_t dset, void *buf, char *error)
{

    hid_t datatype, dataspace;
    hid_t memtype;

    if ((datatype = H5Dget_type(dset)) < 0) {
        sprintf(error, "failed to obtain datatype from  dataset %d", dset);
        return -1;
    }

    if ((dataspace = H5Dget_space(dset)) < 0) {
        sprintf(error, "failed to obtain dataspace from  dataset %d",
                dset);
        return -1;
    }

    memtype = get_memtype(datatype);
    if (memtype < 0) {
        sprintf(error, "failed to obtain memory type");
        return -1;
    }

    if (memtype == H5T_STRING) {
        if (H5Dread(dset, datatype, dataspace, dataspace, H5P_DEFAULT, buf)
            < 0) {
            sprintf(error,
                    "failed to read data from  dataset %d",
                    dset);
            printf("error %s\n", error);
            return -1;
        }
    } else {
        if (H5Dread(dset, memtype, dataspace, dataspace, H5P_DEFAULT, buf)
            < 0) {
            sprintf(error,
                    "failed to read data from  dataset %d",
                    dset);
            return -1;
        }
    }

    if (H5Tget_class(datatype) != H5T_STRING) {
        H5Sclose(dataspace);
        H5Tclose(datatype);
        H5Dclose(dset);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// \fn get_strdata(hid_t dset, int strindex, char *allbuf, char *buf,char *error)
/// will get all data of a dataset and put it into buf.
///
/// \param[in] dset dataset id(dset)
/// \param[in] strindex index of H5T_STRING
/// \param[in] allbuf pointer to string buffer that has been built so far
/// \param[out] buf pointer to a buf
/// \return -1 if failed.
/// \return  0 if succeeded.
////////////////////////////////////////////////////////////////////////////////
int
get_strdata(hid_t dset, int strindex, char *allbuf, char *buf, char *error)
{

    hid_t datatype;
    int elesize;
    int i;
    char *tempvalue;

    tempvalue = allbuf;

    if ((datatype = H5Dget_type(dset)) < 0) {
        sprintf(error, "failed to obtain datatype from  dataset %d", dset);
        return -1;
    }

    elesize = (int) H5Tget_size(datatype);
    if (elesize == 0) {
        sprintf(error, "failed to obtain type size from  dataset");
        return -1;
    }

    for (i = 0; i < strindex; i++)
        tempvalue = tempvalue + elesize;


    sprintf(buf, "%s", tempvalue);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn get_slabdata(hid_t dset, int *offset, int *step, int *count, int num_dim,
///     hsize_t data_size, void *buf, char *error)
/// will get hyperslab data of a dataset and put it into buf.
///
/// \param[in] dset dataset id
/// \param[in] offset starting point
/// \param[in] step  stride
/// \param[in] count  count
/// \param[out] buf pointer to a buffer
/// \return 0 if failed
/// \return 1 otherwise
/// \todo return 0 if succeed?
////////////////////////////////////////////////////////////////////////////////
int
get_slabdata(hid_t dset, int *offset, int *step, int *count, int num_dim,
             hsize_t data_size, void *buf, char *error)
{


    hid_t dataspace, memspace, datatype, memtype;
    hsize_t *dyn_count = NULL, *dyn_step = NULL;
    hssize_t *dyn_offset = NULL;
    int i;


    if ((datatype = H5Dget_type(dset)) < 0) {
        sprintf(error,
                "h5_dods server:  failed to obtain datatype from  dataset %d",
                dset);
        return 0;
    }

    memtype = get_memtype(datatype);
    if (memtype < 0) {
        sprintf(error, "fail to obtain memory type.");
        return 0;
    }
    if ((dataspace = H5Dget_space(dset)) < 0) {
        sprintf(error,
                "h5_dods server:  failed to obtain dataspace from  dataset %d",
                dset);
        return 0;
    }


    dyn_count = (hsize_t*) calloc(num_dim, sizeof(hsize_t));
    dyn_step = (hsize_t*)calloc(num_dim, sizeof(hsize_t));
    dyn_offset = (hssize_t*)calloc(num_dim, sizeof(hssize_t));

    if (!dyn_count) {
        sprintf(error,
                "h5_dods server: out of memory for hyperslab dataset %d",
                dset);
        return 0;
    }

    if (!dyn_step) {
        sprintf(error,
                "h5_dods server: out of memory for hyperslab dataset %d",
                dset);
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
        count++;
        step++;
        offset++;
    }

    if (H5Sselect_hyperslab
        (dataspace, H5S_SELECT_SET, (const hsize_t*)dyn_offset, dyn_step, dyn_count,
         NULL) < 0) {
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
#if 0
    rank = H5Sget_simple_extent_ndims(memspace);
    status_n = H5Sget_simple_extent_dims(memspace, dims1_out, NULL);
    /*   printf("rank mem %d,dimensions %lu x %lu \n",rank,(unsigned long) (dims1_out[0]),
       (unsigned long)(dims1_out[1]));
       printf("read data start\n");fflush(stdout); */

#endif

    if (H5Dread
        (dset, memtype, memspace, dataspace, H5P_DEFAULT,
         (void *) buf) < 0) {
        sprintf(error,
                "get_selecteddata: unable to get data for dataset %d",
                dset);
        return 0;
    }
#if 0
    if (H5Dread
        (dset, datatype, dataspace, dataspace, H5P_DEFAULT,
         (void *) tempbuf) < 0) {
        fprintf(stdout, "error \n");
        return -1;
    }

    /*    printf("datatype is %d\n",datatype);fflush(stdout);
       printf("about to read data\n");fflush(stdout); */
    if (H5Dread
        (dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
         (void *) buf) < 0) {
        fprintf(stdout, "error \n");
        fflush(stderr);
        return -1;
    }
#endif


    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Tclose(datatype);
    H5Dclose(dset);
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn get_dimname(hid_t dataset, int index)
/// obtains dimensional scale name.
///
/// \param hid_t original HDF5 dataset name that refers to dimensional scale
/// \param index index of the dimensional scale
/// \return dimensional name	
////////////////////////////////////////////////////////////////////////////////
char *get_dimname(hid_t dataset, int index)
{
    hid_t attr_id;
    hid_t type, space;
    char *sdsdimname;
    char *dimname;
    char *newdimname = NULL; // <hyokyung 2007.02.16. 13:08:06>
    char *temp_buf;
    hsize_t ssiz;
    size_t type_size;
    int num_attrs;
    int attr_namesize;
    unsigned int i, k, k1;
    char dimscale[21];

    DBG(cerr << ">get_dimname(" << dataset << "," << index << ")" << endl);    
    num_attrs = H5Aget_num_attrs(dataset);
    DBG(cerr << "=get_dimname num_attrs=" << num_attrs << endl);    
    for (i = 0; i < num_attrs; i++) {
        attr_id = H5Aopen_idx(dataset, i);
        bzero(dimscale, sizeof(dimscale));
        attr_namesize = H5Aget_name(attr_id, 20, dimscale);
	// printf("i = %d\n",i);
        printf("dimscale %s\n",dimscale); 
        if (attr_namesize < 0) {
            printf("error in getting attribute name\n");
            return NULL;
        }
	// We may keep this and add more contents! <hyokyung 2007.02.20. 13:34:22>
        if (strncmp(dimscale, "HDF4_DIMENSION_LIST", 19) == 0) {

            type = H5Aget_type(attr_id);
            type_size = H5Tget_size(type);
            space = H5Aget_space(attr_id);
            ssiz = H5Sget_simple_extent_npoints(space);
            sdsdimname = (char*)calloc((size_t) ssiz, type_size);

            temp_buf = (char *) sdsdimname;
            H5Aread(attr_id, type, sdsdimname);
            newdimname = (char*)malloc(type_size);
            dimname = (char*)malloc(type_size);
            for (k = 0; k < ssiz; k++) {
                dimname = temp_buf;
                strncpy(newdimname, dimname, type_size);
		// printf("sdsdimname %s\n",dimname);
                // printf("newdimanme %s\n",newdimname); 
                for (k1 = 0; k1 < type_size; k1++) {
                    temp_buf++;
                }
                if (k == index)
                    break;
            }
            break;
            free(sdsdimname);
        }
        H5Aclose(attr_id);

	
    }
    DBG(cerr << "<get_dimname->" << newdimname << endl);    
    return newdimname;
}



////////////////////////////////////////////////////////////////////////////////
/// \fn get_diminfo(hid_t dataset, int index, int *nelmptr, size_t * dsizeptr,
///            hid_t * dimtypeptr)
/// obtains dimensional scale ID.
///
/// \param dataset original HDF5 dataset name that refers to dim. scale
/// \param index index of the object reference
/// \param nelmptr  pointer to the number of element of this dimension
/// \param dsizeptr pointer to total size of this dimension
/// \param dimtypeptr pointer to dimensional type
/// \return dimensional scale id
///
/// \todo Needs to be re-written or enhanced since
/// <ol>
///    <li> dimensional scale has been updated.
///    <li> needs to fulfill Aura's time.
/// </ol>
// <hyokyung 2007.02.20. 13:39:08>
////////////////////////////////////////////////////////////////////////////////
hid_t
get_diminfo(hid_t dataset, int index, int *nelmptr, size_t * dsizeptr,
            hid_t * dimtypeptr)
{
    hid_t attr_id;
    hid_t tempid;
    hid_t *sdsdim;
    hid_t type, datatype, space;
    hobj_ref_t *refbuf;
    char *buf;
    hsize_t datasetsize, ssiz;
    size_t typesize;
    int nelm;
    int num_attrs;
    int attr_namesize;
    unsigned int i, j;
    char dimscale[21];

    DBG(cerr << ">get_diminfo()" << endl);    
    num_attrs = H5Aget_num_attrs(dataset);

    for (i = 0; i < num_attrs; i++) {
        attr_id = H5Aopen_idx(dataset, i);
        bzero(dimscale, sizeof(dimscale));
        attr_namesize = H5Aget_name(attr_id, 20, dimscale);
        if (attr_namesize < 0) {
            printf("error in getting attribute name\n");
            return -1;
        }
        if (strncmp(dimscale, "DIMSCALE", 8) == 0) {
            type = H5Aget_type(attr_id);
            if (H5Tget_class(type) != H5T_REFERENCE)
                return -1;
            if (!H5Tequal(type, H5T_STD_REF_OBJ))
                return -1;
            space = H5Aget_space(attr_id);
            ssiz = H5Sget_simple_extent_npoints(space);
            ssiz *= H5Tget_size(type);

            buf = (char*)calloc((size_t) ssiz, sizeof(char));
            H5Aread(attr_id, H5T_STD_REF_OBJ, buf);

            refbuf = (hobj_ref_t *) buf;
            ssiz = H5Sget_simple_extent_npoints(space);
            sdsdim = (hid_t*)malloc(sizeof(hid_t) * ssiz);

            for (j = 0; j < ssiz; j++) {

                sdsdim[j] = H5Rdereference(attr_id, H5R_OBJECT, refbuf);
                /*printf("sdsdim[j] %d\n",sdsdim[j]); */

                if (sdsdim[j] < 0) {
                    printf("cannot dereference the object.\n");
                    return -1;
                }

                if (j == index) {
                    tempid = sdsdim[j];
                    datasetsize = H5Dget_storage_size(sdsdim[j]);
                    datatype = H5Dget_type(sdsdim[j]);
                    typesize = H5Tget_size(datatype);
                    nelm = datasetsize / typesize;
                    break;
                }
                refbuf++;
            }
            free(sdsdim);
            free(buf);
            H5Aclose(attr_id);
            break;
        }
        H5Aclose(attr_id);
    }
    *dsizeptr = (size_t) datasetsize;
    *nelmptr = nelm;
    *dimtypeptr = datatype;
    DBG(cerr << "<get_diminfo()" << endl);        
    return tempid;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn get_dimnum(hid_t dataset)
/// obtains number of dimensional scale in the dataset.
///
/// \param dataset original HDF5 dataset name that refers to dimensional scale
/// \return a number
////////////////////////////////////////////////////////////////////////////////
int get_dimnum(hid_t dataset)
{


    hid_t attr_id;
    hid_t type, space;
    hsize_t ssiz;
    int num_dim;
    int num_attrs;
    int attr_namesize;
    unsigned int i;
    char dimscale[21];

    DBG(cerr << ">get_dimnum()" << endl);    
    num_attrs = H5Aget_num_attrs(dataset);

    for (i = 0; i < num_attrs; i++) {
        attr_id = H5Aopen_idx(dataset, i);
        bzero(dimscale, sizeof(dimscale));
        attr_namesize = H5Aget_name(attr_id, 20, dimscale);
        if (attr_namesize < 0) {
            printf("error in getting attribute name\n");
            return -1;
        }
        if (strncmp(dimscale, "DIMSCALE", 8) == 0) {
            type = H5Aget_type(attr_id);
            if (H5Tget_class(type) != H5T_REFERENCE)
                return -1;
            if (!H5Tequal(type, H5T_STD_REF_OBJ))
                return -1;
            space = H5Aget_space(attr_id);
            // number of element for HDF5 dimensional object reference array
            // is the number of dimension of HDF5 corresponding array. 
            ssiz = H5Sget_simple_extent_npoints(space);
            num_dim = (int) ssiz;
            H5Aclose(attr_id);
            break;
        }
        H5Aclose(attr_id);
    }
    DBG(cerr << "<get_dimnum()" << endl);    
    return num_dim;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn correct_name(char *oldname)
/// changes the hdf5 name when the name contains '/' into '_'.
///
/// \param oldname     old name
/// \return the corrected name
////////////////////////////////////////////////////////////////////////////////
char* correct_name(char *oldname)
{

    char *cptr;
    char *newname = NULL; // <hyokyung 2007.02.27. 10:25:33>
    char ORI_SLASH = '/';
    char CHA_SLASH = '_';

    DBG(cerr << ">correct_name(" << oldname << ")" << endl);
    if (oldname == NULL)
        return NULL;

    // The following code is for correcting name from "/" to "_" 
    newname = (char*)malloc((strlen(oldname) + 1) * sizeof(char));
    bzero(newname, (strlen(oldname) + 1) * sizeof(char));
    newname = strncpy(newname, oldname, strlen(oldname));

    while ((cptr = strchr(newname, ORI_SLASH)) != NULL) {
        *cptr = CHA_SLASH;
    }

#if 0
    // I don't understand this comment, but the code breaks a number
    //  of datasets. The section above was commented out but I'm undoing that.
    //  jhrg 7/3/06 
    // Now we want to try DODS ferret demo 
    cptr = strrchr(oldname, ORI_SLASH);
    cptr++;
    newname = malloc((strlen(cptr) + 1) * sizeof(char));
    bzero(newname, strlen(cptr) + 1);
    strncpy(newname, cptr, strlen(cptr));
#endif
    DBG(cerr << "<correct_name=>" << newname <<  endl);
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

// $Log$ //
