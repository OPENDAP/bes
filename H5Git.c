/*-------------------------------------------------------------------------
 * Copyright (C) 1999	National Center for Supercomputing Applications.
 *			All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

/* this file includes all the routines to search HDF5 group, dataset,links,
   and attributes. since we are HDF5 C APIs, we include all c functions in this
   file. Kent Yang 2001/5/14/ */

#include <stdlib.h>
#include <string.h>

#include <hdf5.h>

#include "H5Git.h"

#ifndef FALSE
#define FALSE 0
#endif

static herr_t count_elems(hid_t loc_id, const char *name, void *opdata);
static herr_t obj_info(hid_t loc_id, const char *name, void *opdata);

typedef struct retval {
    char *name;
    int type;
} retval_t;



/* Functions H5Gn_members,H5Gget_obj_info_idx,count_elems,obj_info are courtestly provided by Robert McGrath. I simply leave the comments on. Kent Yang */
/*-------------------------------------------------------------------------
 * Function:	H5Gn_members
 *
 * Purpose:	Return the number of members of a group.  The "members"
 *		are the datasets, groups, and named datatypes in the
 *		group.
 *
 *		This function wraps the H5Ginterate() function in
 *		a completely obvious way, uses the operator
 *		function 'count_members()' below;
 *
 * See also:	H5Giterate()
 *
 *		IN:  hid_t file:  the file id
 *		IN:  char *group_name: the name of the group
 *
 * Errors:
 *
 * Return:	Success:	The object number of members of
 *				the group.
 *
 *		Failure:	FAIL
 *
 * Programmer:	REMcG
 *		Monday, Aug 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Gn_members(hid_t loc_id, char *group_name)
{
    int res;
    int nelems = 0;

    res =
	H5Giterate(loc_id, group_name, NULL, count_elems,
		   (void *) &nelems);
    if (res < 0) {
	return res;
    } else {
	return (nelems);
    }
}



/*-------------------------------------------------------------------------
 * Function:	H5Gget_obj_info_idx
 *
 * Purpose:	Return the name and type of the member of the group
 *		at index 'idx', as defined by the H5Giterator()
 *		function.
 *
 *		This function wraps the H5Ginterate() function in
 *		a completely obvious way, uses the operator
 *		function 'get_objinfo()' below;
 *
 * See also:	H5Giterate()
 *
 *		IN:  hid_t file:  the file id
 *		IN:  char *group_name: the name of the group
 *		IN:  int idx:  the index of the member object (see
 *		               H5Giterate()
 * 		OUT:  char **objname:  the name of the member object 
 * 		OUT:  int *type:  the type of the object (dataset, 
 *			group, or named datatype)
 *
 * Errors:
 *
 * Return:	Success:	The object number of members of
 *				the group.
 *
 *		Failure:	FAIL
 *
 * Programmer:	REMcG
 *		Monday, Aug 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gget_obj_info_idx(hid_t loc_id, char *group_name, int idx,
		    char **objname, int *type)
{
    int res;
    retval_t retVal;

    res = H5Giterate(loc_id, group_name, &idx, obj_info, (void *) &retVal);
    if (res < 0) {
	return res;
    }
    *objname = retVal.name;
    *type = retVal.type;
    return 0;
}



/*-------------------------------------------------------------------------
 * Function:	count_elems
 *
 * Purpose:	this is the operator function called by H5Gn_members().
 *
 *		This function is passed to H5Ginterate().
 *
 * See also:	H5Giterate()
 *
 * 		OUT:  'opdata' is returned as an integer with the
 *			number of members in the group.
 *
 * Errors:
 *
 * Return:	Success:	The object number of members of
 *				the group.
 *
 *		Failure:	FAIL
 *
 * Programmer:	REMcG
 *		Monday, Aug 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

static herr_t
count_elems(hid_t loc_id, const char *name, void *opdata)
{
    herr_t res;
    H5G_stat_t statbuf;

    res = H5Gget_objinfo(loc_id, name, FALSE, &statbuf);
    if (res < 0) {
	return 1;
    }
    switch (statbuf.type) {
    case H5G_GROUP:
	(*(int *) opdata)++;
	break;
    case H5G_DATASET:
	(*(int *) opdata)++;
	break;
    case H5G_TYPE:
	(*(int *) opdata)++;
	break;
    default:
	(*(int *) opdata)++;	/* ???? count links or no? */
	break;
    }
    return 0;
}


/*-------------------------------------------------------------------------
 * Function:	obj_info
 *
 * Purpose:	this is the operator function called by H5Gn_members().
 *
 *		This function is passed to H5Ginterate().
 *
 * See also:	H5Giterate()
 *
 * 		OUT:  'opdata' is returned as a 'recvar_t', containing
 *			the object name and type.
 *
 * Errors:
 *
 * Return:	Success:	The object number of members of
 *				the group.
 *
 *		Failure:	FAIL
 *
 * Programmer:	REMcG
 *		Monday, Aug 2, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 *			group, or named datatype)
 */
static herr_t
obj_info(hid_t loc_id, const char *name, void *opdata)
{
    herr_t res;
    H5G_stat_t statbuf;

    res = H5Gget_objinfo(loc_id, name, FALSE, &statbuf);
    if (res < 0) {
	((retval_t *) opdata)->type = 0;
	((retval_t *) opdata)->name = NULL;
	return 1;
    } else {
	((retval_t *) opdata)->type = statbuf.type;
	((retval_t *) opdata)->name = strdup(name);
	return 1;
    }
}

/*-------------------------------------------------------------------------
 * Function:	get_Dattr_numb
 *
 * Purpose:	obtain number of attributes in a dataset or 
 *              a group.
 *
 *
 * Errors:
 *
 * Return:	object id:	dataset
 *		Failure:	-1
 *
 * In :	        parent object id(pid)
 *              parent object name
 * Out:         pointer of number of attributes(* num_attr_ptr)
 *	     
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 *			
 */


hid_t
get_Dattr_numb(hid_t pid, int *num_attr_ptr, char *dname, char *error)
{

    hid_t dset;
    int num_at;

    if ((dset = H5Dopen(pid, dname)) < 0) {
	sprintf(error,
		"h5_das server:  unable to open hdf5 dataset of group %d",
		pid);
	return -1;
    }

    /* obtain number of attributes in this dataset. */

    if ((num_at = H5Aget_num_attrs(dset)) < 0) {
	sprintf(error,
		"h5_das server:  failed to obtain hdf5 attribute in dataset %d",
		dset);
	return -1;
    }

    *num_attr_ptr = num_at;
    return dset;

}

/*-------------------------------------------------------------------------
 * Function:	get_Gattr_numb
 *
 * Purpose:	this function will get number of attributes in a dataset or 
 *              a group.
 *
 *
 * Errors:
 *
 * Return:	objectid:       group
 *		Failure:	-1
 *
 * In :	        parent object id(pid)
 *              parent object name
 * Out:         pointer of number of attributes(* num_attr_ptr)
 *	     
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 *			
 */


hid_t
get_Gattr_numb(hid_t pid, int *num_attr_ptr, char *dname, char *error)
{

    hid_t c_group;
    int num_at;

    if ((c_group = H5Gopen(pid, dname)) < 0) {
	sprintf(error,
		"h5_das server:  unable to open hdf5 group in group %d",
		pid);
	return -1;
    }

    /* obtain number of attributes in this dataset. */

    if ((num_at = H5Aget_num_attrs(c_group)) < 0) {
	sprintf(error,
		"h5_das server:  failed to obtain hdf5 attribute in group %d",
		c_group);
	return -1;
    }

    *num_attr_ptr = num_at;
    return c_group;

}

/*-------------------------------------------------------------------------
 * Function:	get_attr_info
 *
 * Purpose:	this function will get attribute information:
                datatype, dataspace(dimension sizes) and number of
                dimensions and put it into a data struct.
 *
 *
 * Errors:
 *
 * Return:	attribute id:	
 *		Failure:	-1
 *
 * In :	        parent object id(oid)
 *              parent object index
 * Out:         pointer to the attribute struct(* attr_inst_ptr)
 *	     
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 *			
 */

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

    /*  size_t need; */
    int j, ndims;

    /*  int buf_size =30; */

    *ignoreptr = 0;
    namebuf = malloc(DODS_NAMELEN);

    if ((attrid = H5Aopen_idx(dset, index)) < 0) {
	strcpy(error, " unable to obtain hdf5 attribute ");
	attrid = -1;
	goto exit;
    }

    /* obtain the attribute name. */
    if ((H5Aget_name(attrid, DODS_NAMELEN, namebuf)) < 0) {
	strcpy(error, " unable to obtain hdf5 attribute name");
	attrid = -1;
	goto exit;
    }


    if ((attrid = H5Aopen_name(dset, namebuf)) < 0) {
	strcpy(error, " unable to obtain hdf5 attribute ");
	attrid = -1;
	goto exit;
    }

    /* obtain the type of the attribute. */
    if ((ty_id = H5Aget_type(attrid)) < 0) {
	strcpy(error, " unable to get hdf5 attribute type ");
	attrid = -1;
	goto exit;
    }
    temp_type = H5Tget_class(ty_id);

    if (temp_type < 0) {
	strcpy(error, " unable to obtain hdf5 datatype ");
	attrid = -1;
	goto exit;
    }

    if ((temp_type == H5T_TIME) || (temp_type == H5T_BITFIELD)
	|| (temp_type == H5T_OPAQUE) || (temp_type == H5T_COMPOUND)
	|| (temp_type == H5T_ENUM)) {
	/* strcpy(error,"unexpected datatype for DODS "); */
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
	strcpy(error, " unable to get attribute data space");
	attrid = -1;
	goto exit;
    }

    ndims = H5Sget_simple_extent_dims(space, size, maxsize);

    /* check dimension size. */

    if (ndims > DODS_MAX_RANK) {
	strcpy(error,
	       "number of dimensions exceeds hdf5_das server allowed.");
	attrid = -1;
	goto exit;
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

    /* return ndims and size[ndims]. */

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


/* this function is used because H5Fopen cannot be directly used in a 
   C++ code. */

hid_t
get_fileid(const char *filename)
{
    return H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
}

/*-------------------------------------------------------------------------
 * Function:	get_dataset
 *
 * Purpose:	 obtain data information in a dataset
 *               datatype, dataspace(dimension sizes) and number of
 *               dimensions and put these information into a pointer of
 *               data struct.
 *
 *
 * Errors:
 *
 * Return:	dataset id:	
  *
 * In :	        parent object id(group id)
 *              dataset name
 * Out:         pointer to the attribute struct(* attr_inst_ptr)
 *	     
 *
 *-------------------------------------------------------------------------
 *			
 */

hid_t
get_dataset(hid_t pid, char *dname, DS_t * dt_inst_ptr, char *error)
{

    hid_t dset;
    hid_t datatype, dataspace;
    H5T_class_t temp_type;
    hsize_t size[DODS_MAX_RANK];
    hsize_t maxsize[DODS_MAX_RANK];
    char *namebuf;
    size_t need;
    hsize_t nelmts = 1;
    int j, ndims;
    int buf_size = 30;

    if ((dset = H5Dopen(pid, dname)) < 0) {
	sprintf(error, "h5_das server:  failed to obtain dataset %s",
		dname);
	return -1;
    }

    if ((datatype = H5Dget_type(dset)) < 0) {
	sprintf(error,
		"h5_das server:  failed to obtain datatype from  dataset %s",
		dname);
	return -1;
    }

    if ((dataspace = H5Dget_space(dset)) < 0) {
	sprintf(error,
		"h5_das server:  failed to obtain dataspace from  dataset %s",
		dname);
	return -1;
    }

    temp_type = H5Tget_class(datatype);


    if (temp_type < 0) {
	sprintf(error,
		"h5_das server:  failed to obtain type class from %d",
		datatype);
	return -1;
    }

    if ((temp_type == H5T_TIME) || (temp_type == H5T_BITFIELD)
	|| (temp_type == H5T_OPAQUE) || (temp_type == H5T_COMPOUND)
	|| (temp_type == H5T_ENUM) || (temp_type == H5T_REFERENCE)) {

	strcpy(error, "get data unexpected datatype at temp_type");
	return -1;
    }

    /* obtain number of attributes in this dataset. */

    if ((ndims = H5Sget_simple_extent_dims(dataspace, size, maxsize)) < 0) {
	strcpy(error, " unable to get number of dimensions");
	return -1;
    }

    /* check dimension size. */
    if (ndims > DODS_MAX_RANK) {
	strcpy(error,
	       "number of dimensions exceeds hdf5-dods server allowed");
	return -1;
    }

    namebuf = malloc(buf_size);

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

    /* return ndims and size[ndims]. */

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
    return dset;
}


/*-------------------------------------------------------------------------
 * Function:	get_data
 *
 * Purpose:	this function will get all data of a dataset
 *              and put it into buf.

 * Errors:
 *
 * Return:	-1, failed. otherwise,success.
  *
 * In :	        dataset id(dset)
 * Out:         pointer to a buf.
 *	     
 *
 *-------------------------------------------------------------------------
 */
int
get_data(hid_t dset, void *buf, char *error)
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
		    "h5_das server:  failed to read data from  dataset %d",
		    dset);
	    printf("error %s\n", error);
	    return -1;
	}
    } else {
	if (H5Dread(dset, memtype, dataspace, dataspace, H5P_DEFAULT, buf)
	    < 0) {
	    sprintf(error,
		    "h5_das server:  failed to read data from  dataset %d",
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

/*-------------------------------------------------------------------------
 * Function:	get_strdata
 *
 * Purpose:	this function will get all data of a dataset
 *              and put it into buf.

 * Errors:
 *
 * Return:	-1, failed. otherwise,success.
  *
 * In :	        dataset id(dset)
                strindex: index of H5T_STRING
 * Out:         pointer to a buf.
 *	     
 *
 *-------------------------------------------------------------------------
 */
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

/*-------------------------------------------------------------------------
 * Function:	get_slabdata
 *
 * Purpose:	this function will get hyperslab data of a dataset
 *              and put it into buf.

 * Errors:
 *
 * Return:	-1, failed. otherwise,success.
  *
 * In :	        dataset id(dset)
                offset : starting point
                step:  stride
                count: count
 * Out:         pointer to a buf.
 *	     
 *
 *-------------------------------------------------------------------------
 */
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


    dyn_count = calloc(num_dim, sizeof(hsize_t));
    dyn_step = calloc(num_dim, sizeof(hsize_t));
    dyn_offset = calloc(num_dim, sizeof(hssize_t));

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

    if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, dyn_offset, dyn_step,
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

/*-------------------------------------------------------------------------
 * Function:	get_dimname
 *
 * Purpose:	 obtain dimensional scale name
 *
 * Errors:
 *
 * Return:	dimensional name	
  *
 * In :	        hid_t: original HDF5 dataset name that refers to dim. scale
 *              int: index of the dimensional scale
 * Out:         this dimensional scale name
 *	     
 *
 *-------------------------------------------------------------------------
 *			
 */

char *
get_dimname(hid_t dataset, int index)
{
    hid_t attr_id;
    hid_t type, space;
    char *sdsdimname;
    char *dimname;
    char *newdimname;
    char *temp_buf;
    hsize_t ssiz;
    size_t type_size;
    int num_attrs;
    int attr_namesize;
    unsigned int i, k, k1;
    char dimscale[21];

    num_attrs = H5Aget_num_attrs(dataset);

    for (i = 0; i < num_attrs; i++) {
	attr_id = H5Aopen_idx(dataset, i);
	bzero(dimscale, sizeof(dimscale));
	attr_namesize = H5Aget_name(attr_id, 20, dimscale);

	/*  printf("i = %d\n",i);
	   printf("dimscale %s\n",dimscale); */
	if (attr_namesize < 0) {
	    printf("error in getting attribute name\n");
	    return NULL;
	}
	if (strncmp(dimscale, "HDF4_DIMENSION_LIST", 19) == 0) {

	    type = H5Aget_type(attr_id);
	    type_size = H5Tget_size(type);
	    space = H5Aget_space(attr_id);
	    ssiz = H5Sget_simple_extent_npoints(space);
	    sdsdimname = calloc((size_t) ssiz, type_size);

	    temp_buf = (char *) sdsdimname;
	    H5Aread(attr_id, type, sdsdimname);
	    newdimname = malloc(type_size);
	    dimname = malloc(type_size);
	    for (k = 0; k < ssiz; k++) {
		dimname = temp_buf;
		strncpy(newdimname, dimname, type_size);
		/*    printf("sdsdimname %s\n",dimname);
		   printf("newdimanme %s\n",newdimname); */
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
    return newdimname;
}





/*-------------------------------------------------------------------------
 * Function:	get_dimid
 *
 * Purpose:	 obtain dimensional scale ID
 *
 * Errors:
 *
 * Return:	dimensional scale id
  *
 * In :	        hid_t: original HDF5 dataset name that refers to dim. scale
 *              int: index of the object reference
 * Out:         this dimensional scale id
 *	        nelmptr: pointer to the number of element of this dimension
                dsizeptr: pointer to total size of this dimension
		dimtypeptr:pointer to dimensional type 
 *
 *-------------------------------------------------------------------------
 *			
 */

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

	    buf = calloc((size_t) ssiz, sizeof(char));
	    H5Aread(attr_id, H5T_STD_REF_OBJ, buf);

	    refbuf = (hobj_ref_t *) buf;
	    ssiz = H5Sget_simple_extent_npoints(space);
	    sdsdim = malloc(sizeof(hid_t) * ssiz);

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
    return tempid;
}

/*-------------------------------------------------------------------------
 * Function:	get_dimnum
 *
 * Purpose:	 obtain number of dimensional scale in the dataset
 *
 * Errors:
 *
 * Return:	number
  *
 * In :	        hid_t: original HDF5 dataset name that refers to dim. scale
 *              
 *	     
 *
 *-------------------------------------------------------------------------
 *			
 */

int
get_dimnum(hid_t dataset)
{


    hid_t attr_id;
    hid_t type, space;
    hsize_t ssiz;
    int num_dim;
    int num_attrs;
    int attr_namesize;
    unsigned int i;
    char dimscale[21];

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
	    /* number of element for HDF5 dimensional object reference array
	       is the number of dimension of HDF5 corresponding array. */
	    ssiz = H5Sget_simple_extent_npoints(space);
	    num_dim = (int) ssiz;
	    H5Aclose(attr_id);
	    break;
	}
	H5Aclose(attr_id);
    }
    return num_dim;
}

/*-------------------------------------------------------------------------
 * Function:    correct_name
 *
 * Purpose:     modify the hdf5 name when the name contains '/'. Change
                this character into '_'.
                
 *              
 * Return:      the corrected name
 *
 * In :	        old name
                
 
 *-------------------------------------------------------------------------
 */
char *
correct_name(char *oldname)
{

    char *cptr;
    char *newname;
    char ORI_SLASH = '/';
    char CHA_SLASH = '_';

    if (oldname == NULL) {
	printf("inputting name is wrong.\n");
	return NULL;
    }

    /* the following code is for correcting name from "/" to "_" */
    newname = malloc((strlen(oldname)+1)*sizeof(char));
    bzero(newname,(strlen(oldname)+1)*sizeof(char));
    newname = strncpy(newname, oldname, strlen(oldname));
  
    while((cptr=strchr(newname,ORI_SLASH)) != NULL){
	*cptr = CHA_SLASH;
    }

#if 0
    /* I don't understand this comment, but the code break a number
       of datasets. The section above was commented out but I'm undoing that.
       jhrg 7/3/06 */
    /* Now we want to try DODS ferret demo */
    cptr = strrchr(oldname, ORI_SLASH);
    cptr++;
    newname = malloc((strlen(cptr) + 1) * sizeof(char));
    bzero(newname, strlen(cptr) + 1);
    strncpy(newname, cptr, strlen(cptr));
#endif

    return newname;
}

hid_t
get_memtype(hid_t datatype)
{

    H5T_class_t typeclass;
    size_t typesize;

    typesize = H5Tget_size(datatype);
    typeclass = H5Tget_class(datatype);

    /* we will only consider H5T_INTEGER,H5T_FLOAT in this case. */

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


int
check_h5str(hid_t h5type)
{
    if (H5Tget_class(h5type) == H5T_STRING) {
	return 1;
    } else
	return 0;
}
