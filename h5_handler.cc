/*-------------------------------------------------------------------------
 * Copyright (C) 1999	National Center for Supercomputing Applications.
 *			All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

/* This is the HDF5-DAS which extracts DAS class descriptors converted from
   HDF5 attribute of an hdf5 data file. */

#include "config_hdf5.h"
#include "h5_das.h"
#include "h5dds.h"
//#include "H5Git.h"

#include "debug.h"
#include "InternalErr.h"

/* the following "C" functions will be used in this routine. */

extern "C" {
    int H5Gn_members(hid_t loc_id, char *group_name);
    herr_t H5Gget_obj_info_idx(hid_t loc_id, char *group_name, int idx,
			       char **objname, int *type);
    hid_t get_attr_info(hid_t dset, int index, DSattr_t * attr_inst,
			int *ignore_ptr, char *);
    hid_t get_Dattr_numb(hid_t pid, int *num_attr, char *dname, char *);
    hid_t get_Gattr_numb(hid_t pid, int *num_attr, char *dname, char *);
    hid_t get_fileid(const char *filename);
    hid_t get_memtype(hid_t);
} 

static char Msgt[255];
static int slinkindex;
const static string cgi_version = DODS_SERVER_VERSION;

int 
main(int argc, char *argv[])
{
    DBG(cerr << "Starting the HDF server." << endl);

    try {
	DODSFilter df(argc, argv);
	if (df.get_cgi_version() == "")
	    df.set_cgi_version(cgi_version);

	// Handle the version info as a special case since there's no need to
	// open the hdf5 file. This makes it possible to move the file open
	// and close code out of the individual case statements. 02/04/04
	// jhrg
	if (df.get_response() == DODSFilter::Version_Response) {
	      df.send_version_info();
	      return 0;
	}

	hid_t file1 = get_fileid(df.get_dataset_name().c_str());
	if (file1 < 0)
	    throw Error(no_such_file,
			string("Could not open hdf5 file: ")
			+ df.get_dataset_name());

	switch (df.get_response()) {
	  case DODSFilter::DAS_Response: {
	      DAS das;
	      find_gloattr(file1, das);
	      depth_first(file1, "/", das, df.get_dataset_name().c_str());
	      df.send_das(das);
	      break;
	  }

	  case DODSFilter::DDS_Response: {
	      DDS dds;
	      depth_first(file1, "/", dds, df.get_dataset_name().c_str());
	      df.send_dds(dds, true);
	      break;
	  }

	  case DODSFilter::DataDDS_Response: {
	      DDS dds;
	      depth_first(file1, "/", dds, df.get_dataset_name().c_str());
	      df.send_data(dds, stdout);
	      break;
	  }

	  default:
	    df.print_usage();	// Throws Error
	}

	if (H5Fclose(file1) < 0)
	    throw Error(unknown_error,
			string("Could not close the HDF5 file: ")
			+ df.get_dataset_name());

    }
    catch (Error &e) {
        string s;
	s = string("h5_handler: ") + e.get_error_message() + "\n";
        ErrMsgT(s);
	set_mime_text(cout, dods_error, cgi_version);
	e.print(cout);
	return 1;
    }
    catch (...) {
        string s("h5_handler: Unknown exception");
	ErrMsgT(s);
	Error e(unknown_error, s);
	set_mime_text(cout, dods_error, cgi_version);
	e.print(cout);
	return 1;
    }
    
    DBG(cerr << "HDF5 server exitied successfully." << endl);
    return 0;
}

/*-------------------------------------------------------------------------
 * Function:	depth_first
 *
 * Purpose:	this function will walk through hdf5 group and using depth-
 *              first approach to obtain all the group and dataset attributes 
 *         	of a hdf5 file.
 *            

 * Errors:      will return error message to the interface.
 * Return:	false, failed. otherwise,success.
  *
 * In :	        dataset id(group id)
                gname: group name(absolute name from root group).
                das: reference of DAS object.
                error: error message to dods inteface.
		fname: file name.
 * Out:         in the process of depth first search. DAS table will be filled.
 *	     
 *
 *-------------------------------------------------------------------------
 */
bool
depth_first(hid_t pid, char *gname, DAS & das, const char *fname)
{
    int num_attr = -1;

    /* Iterate through the file to see members of the root group */
    int nelems = H5Gn_members(pid, gname);
    if (nelems < 0) {
	string msg =
	    "h5_das handler: counting hdf5 group elements error for ";
	msg += gname;
	throw InternalErr(__FILE__, __LINE__, msg);
    }

    for (int i = 0; i < nelems; i++) {

	char *oname = NULL;
	int type = -1;
	herr_t ret = H5Gget_obj_info_idx(pid, gname, i, &oname, &type);

	if (ret < 0) {
	    string msg =
		"h5_das handler: getting hdf5 object information error from";
	    msg += gname;
	    throw InternalErr(__FILE__, __LINE__, msg);
	}

	switch (type) {

	case H5G_GROUP:{
		string full_path_name =
		    string(gname) + string(oname) + "/";
		hid_t pgroup = H5Gopen(pid, gname);
		char *t_fpn = new char[full_path_name.length() + 1];

		strcpy(t_fpn, full_path_name.c_str());
		hid_t dset =
		    get_Gattr_numb(pgroup, &num_attr, oname, Msgt);
		if (dset < 0) {
		    string msg =
			"h5_das handler: open hdf5 dataset wrong for ";
		    msg += t_fpn;
		    msg += string("\n") + string(Msgt);
		    delete[]t_fpn;
		    throw InternalErr(__FILE__, __LINE__, msg);
		}

		try {
		    read_objects(das, t_fpn, dset, num_attr);
		    depth_first(pgroup, t_fpn, das, fname);
		}
		catch(Error & e) {
		    delete[]t_fpn;
		    throw;
		}

		delete[]t_fpn;
		H5Gclose(pgroup);
		break;
	    }

	case H5G_DATASET:{

		string full_path_name = string(gname) + string(oname);
		hid_t dgroup = H5Gopen(pid, gname);

		if (dgroup < 0) {
		    string msg = "h5_dds handler: cannot open hdf5 group";

		    msg += gname;
		    throw InternalErr(__FILE__, __LINE__, msg);
		}

		char *t_fpn = new char[full_path_name.length() + 1];

		strcpy(t_fpn, full_path_name.c_str());
		hid_t dset =
		    get_Dattr_numb(dgroup, &num_attr, t_fpn, Msgt);

		if (dset < 0) {
		    string msg = (string) Msgt;

		    delete[]t_fpn;
		    throw InternalErr(__FILE__, __LINE__, msg);
		}

		try {
		    read_objects(das, t_fpn, dset, num_attr);
		}
		catch(Error & e) {
		    delete[]t_fpn;
		    throw;
		}

		H5Dclose(dset);
		delete[]t_fpn;
		break;
	    }
	case H5G_TYPE:

	    break;

	case H5G_LINK:{

		string full_path_name = string(gname) + string(oname);
		char *t_fpn = new char[full_path_name.length() + 1];

		slinkindex++;
		try {
		    get_softlink(das, pid, oname, slinkindex);
		}
		catch(Error & e) {
		    delete[]t_fpn;
		    throw;
		}
		delete[]t_fpn;
		break;
	    }
	default:
	    break;
	}
	type = -1;
    }


    return true;
}

/*-------------------------------------------------------------------------
 * Function:	print_attr(this function is based on netcdf-dods server).
 *
 * Purpose:      will get the printed representation of an attribute.
 *              

 * Errors:
 *
 * Return:	return a char * to newly allocated memory, the caller
                must call delete [].
  *
 * In :	        hid_t type:  HDF5 data type id
                int loc:     the number of array number
                void* sm_buf: pointer to an attribute


 *	     
 *
 *-------------------------------------------------------------------------
 */

static char *
print_attr(hid_t type, int loc, void *sm_buf)
{
#if 0
    int i;
#endif
    int ll;
    char f_fmt[10];
    char d_fmt[10];
    char gps[30];
    char *rep;

    union {
	char *tcp;
	short *tsp;
	unsigned short *tusp;
	int *tip;
	long *tlp;
	float *tfp;
	double *tdp;
    } gp;

    unsigned char tuchar;

    strcpy(f_fmt, "%.10g");
    strcpy(d_fmt, "%.17g");

    switch (H5Tget_class(type)) {

    case H5T_INTEGER:

    /****  change void pointer into the corresponding integer datatype. ****/
    /**** 32 should be long enough to hold one integer and one floating point
	  number. ****/

	rep = new char[32];

	bzero(rep, 32);
	if (H5Tequal(type, H5T_STD_U8BE) ||
	    H5Tequal(type, H5T_STD_U8LE) ||
	    H5Tequal(type, H5T_NATIVE_UCHAR)) {

	    gp.tcp = (char *) sm_buf;
	    tuchar = *(gp.tcp + loc);
	    sprintf(rep, "%c", tuchar);
	}

	else if (H5Tequal(type, H5T_STD_U16BE) ||
		 H5Tequal(type, H5T_STD_U16LE) ||
		 H5Tequal(type, H5T_NATIVE_USHORT)) {
	    gp.tusp = (unsigned short *) sm_buf;
	    sprintf(rep, "%hu", *(gp.tusp + loc));
	}

	else if (H5Tequal(type, H5T_STD_U32BE) ||
		 H5Tequal(type, H5T_STD_U32LE) ||
		 H5Tequal(type, H5T_NATIVE_UINT)) {

	    gp.tip = (int *) sm_buf;
	    sprintf(rep, "%u", *(gp.tip + loc));
	}

	else if (H5Tequal(type, H5T_STD_U64BE) ||
		 H5Tequal(type, H5T_STD_U64LE) ||
		 H5Tequal(type, H5T_NATIVE_ULONG) ||
		 H5Tequal(type, H5T_NATIVE_ULLONG)) {

	    gp.tlp = (long *) sm_buf;
	    sprintf(rep, "%lu", *(gp.tlp + loc));
	}

	else if (H5Tequal(type, H5T_STD_I8BE) ||
		 H5Tequal(type, H5T_STD_I8LE) ||
		 H5Tequal(type, H5T_NATIVE_CHAR)) {

	    gp.tcp = (char *) sm_buf;
	    sprintf(rep, "%c", *(gp.tcp + loc));
	}

	else if (H5Tequal(type, H5T_STD_I16BE) ||
		 H5Tequal(type, H5T_STD_I16LE) ||
		 H5Tequal(type, H5T_NATIVE_SHORT)) {

	    gp.tsp = (short *) sm_buf;
	    sprintf(rep, "%hd", *(gp.tsp + loc));
	}

	else if (H5Tequal(type, H5T_STD_I32BE) ||
		 H5Tequal(type, H5T_STD_I32LE) ||
		 H5Tequal(type, H5T_NATIVE_INT)) {

	    gp.tip = (int *) sm_buf;
	    sprintf(rep, "%d", *(gp.tip + loc));
	}

	else if (H5Tequal(type, H5T_STD_I64BE) ||
		 H5Tequal(type, H5T_STD_I64LE) ||
		 H5Tequal(type, H5T_NATIVE_LONG) ||
		 H5Tequal(type, H5T_NATIVE_LLONG)) {

	    gp.tlp = (long *) sm_buf;
	    sprintf(rep, "%ld", *(gp.tlp + loc));
	}

	break;

    case H5T_FLOAT:

	rep = new char[32];

	bzero(rep, 32);
	if (H5Tget_size(type) == 4) {
	    gp.tfp = (float *) sm_buf;
	    sprintf(gps, f_fmt, *(gp.tfp + loc));
	    ll = strlen(gps);

	    if (!strchr(gps, '.') && !strchr(gps, 'e'))
		gps[ll++] = '.';

	    gps[ll] = '\0';
	    sprintf(rep, "%s", gps);
	}
	else if (H5Tget_size(type) == 8) {

	    gp.tdp = (double *) sm_buf;
	    sprintf(gps, d_fmt, *(gp.tdp + loc));
	    ll = strlen(gps);
	    if (!strchr(gps, '.') && !strchr(gps, 'e'))
		gps[ll++] = '.';
	    gps[ll] = '\0';
	    sprintf(rep, "%s", gps);
	}
	break;

    case H5T_STRING:

	rep = new char[H5Tget_size(type) + 3];

	bzero(rep, H5Tget_size(type) + 3);
	sprintf(rep, "\"%s\"", (char *) sm_buf);
	break;

	// Is this correct? Note: We have to allocate storage since the
	// caller will use delete on the value returned here. 7/25/2001 jhrg
      default:
	rep = new char[1];
	rep[0] = '\0';
	break;
    }

    return rep;
}

/*-------------------------------------------------------------------------
 * Function:	print_type
 *
 * Purpose:	this function will get the corresponding DODS datatype.
 *              This function will return the "text representation" of
                the correponding datatype translated from HDF5. 
 *              for unknown datatype, put it to string.

 * Errors:
 *
 * Return:	static string
 *
 * In :	        datatype id
 *	     
 *
 *-------------------------------------------------------------------------
 */
string
print_type(hid_t type)
{

    switch (H5Tget_class(type)) {

    case H5T_INTEGER:
	if (H5Tequal(type, H5T_STD_I8BE) ||
	    H5Tequal(type, H5T_STD_I8LE) ||
	    H5Tequal(type, H5T_NATIVE_SCHAR))
	    return BYTE;

	else if (H5Tequal(type, H5T_STD_I16BE) ||
		 H5Tequal(type, H5T_STD_I16LE) ||
		 H5Tequal(type, H5T_NATIVE_SHORT))
	    return INT16;

	else if (H5Tequal(type, H5T_STD_I32BE) ||
		 H5Tequal(type, H5T_STD_I32LE) ||
		 H5Tequal(type, H5T_NATIVE_INT))
	    return INT32;

	else if (H5Tequal(type, H5T_STD_U16BE) ||
		 H5Tequal(type, H5T_STD_U16LE) ||
		 H5Tequal(type, H5T_NATIVE_USHORT))
	    return UINT16;

	else if (H5Tequal(type, H5T_STD_U32BE) ||
		 H5Tequal(type, H5T_STD_U32LE) ||
		 H5Tequal(type, H5T_NATIVE_UINT))
	    return UINT32;
	else
	    return STRING;


    case H5T_FLOAT:
	if (H5Tget_size(type) == 4)
	    return FLOAT32;
	else if (H5Tget_size(type) == 8)
	    return FLOAT64;

	else
	    return STRING;

    case H5T_STRING:
	return STRING;

      default:
	return "";
    }
}

/*-------------------------------------------------------------------------
 * Function:	read_objects
 *
 * Purpose:	this function will fill in attributes of a dataset or a 
                group into one DAS table. 
		It will call functions  get_attr_info, print_type, 
		print_rep 
 *              

 * Errors:
 *
 * Return:	false, failed. otherwise,success.
  *
 * In :	        DAS object: reference
                object name: absolute name of either a dataset or a group
                error: a string of error message to the dods interface.
                object id: dset
		num_attr: number of attributes.
 *	     
 *
 *-------------------------------------------------------------------------
 */

void
read_objects(DAS & das, const string & varname, hid_t oid, int num_attr)
{
    hid_t ty_id, attr_id;
    hid_t memtype;
    char *value;
    char *tempvalue;
    char *print_rep;
    DSattr_t attr_inst;
    AttrTable *attr_table_ptr;
    int ignore_attr;
    int loc;
    char ORI_SLASH = '/';
#if 0
    char CHA_SLASH = '_';
    char *cptr;
#endif

    ignore_attr = 0;

    /* obtain variable names. Put this variable name into das table 
       regardless of the existing attributes in this object. */

    char *newname;
    char attr_name[5];
    char *hdf5_path;

    strcpy(attr_name, "name");


    hdf5_path = new char[strlen(HDF5_OBJ_FULLPATH) + 1];

    bzero(hdf5_path, strlen(HDF5_OBJ_FULLPATH));
    strcpy(hdf5_path, HDF5_OBJ_FULLPATH);
    char *temp_varname = new char[varname.length() + 1];
    char *new_varname = new char[varname.length()];

    varname.copy(temp_varname, string::npos);
    temp_varname[varname.length()] = '\0';

    // The following code will change "/" into "_", however,for ferret demo
    // we will use newname and ignore "/", only the relative name
    // is returned.
#if 0
    while (strchr(temp_varname, ORI_SLASH) != NULL) {
	cptr = strchr(temp_varname, ORI_SLASH);
	*cptr = CHA_SLASH;
    }
#endif

    newname = strrchr(temp_varname, ORI_SLASH);
    if (newname != NULL)
	newname++;
    else
	newname = temp_varname;

    /* this full path must not be root, root attribute is handled by
       find_gloattr */
    if (temp_varname[varname.length() - 1] == '/') {
	strncpy(new_varname, temp_varname, varname.length() - 1);
	new_varname[varname.length() - 1] = '\0';
	newname = strrchr(new_varname, ORI_SLASH);
	if (newname != NULL)
	    newname++;
	else
	    newname = new_varname;
    }

    attr_table_ptr = das.get_table(newname);
    try {
	if (!attr_table_ptr)
	    attr_table_ptr = das.add_table(newname, new AttrTable);
    }
    catch(Error & e) {
	delete[]hdf5_path;
	delete[]temp_varname;
	delete[]new_varname;
	throw;
    }

    char *fullpath = new char[strlen(temp_varname) + 3];

    bzero(fullpath, strlen(temp_varname) + 3);
    sprintf(fullpath, "\"%s\"", temp_varname);
    try {
	attr_table_ptr->append_attr(hdf5_path, STRING, fullpath);
    }
    catch(Error & e) {
	delete[]hdf5_path;
	delete[]temp_varname;
	delete[]new_varname;
	delete[]fullpath;
	throw;
    }

    delete[]fullpath;
    delete[]hdf5_path;

    /* check the number of attributes in this HDF5 object,
       put HDF5 attribute information into DAS table. */

    for (int j = 0; j < num_attr; j++) {

	// obtain attribute information.
	attr_id = get_attr_info(oid, j, &attr_inst, &ignore_attr, Msgt);
	if (attr_id == 0 && ignore_attr == 1) {
	    continue;
	}

	if (attr_id < 0) {

	    string msg =
		"h5_dds handler: unable to get HDF5 attribute information from ";
	    msg += temp_varname;
	    msg += string("\n") + string(Msgt);
	    delete[]temp_varname;
	    delete[]new_varname;
	    throw InternalErr(__FILE__, __LINE__, msg);
	}

	/* HDF5 attribute may be in string datatype, it must be dealt with 
	   properly. */

	/* get data type. */
	ty_id = attr_inst.type;

	memtype = get_memtype(ty_id);
	if (memtype < 0) {
	    string msg =
		"h5_das handler: unable to get HDF5 attribute data type.";
	    delete[]temp_varname;
	    delete[]new_varname;
	    throw InternalErr(__FILE__, __LINE__, msg);
	}
	value = new char[attr_inst.need + sizeof(char)];

	if (!value) {
	    delete[]temp_varname;
	    delete[]new_varname;
	    string msg = "h5_das handler: no enough memory";
	    throw InternalErr(__FILE__, __LINE__, msg);
	}
	bzero(value, (attr_inst.need + sizeof(char)));

	/* read HDF5 attribute data. */

	if (memtype == H5T_STRING) {
	    if (H5Aread(attr_id, ty_id, (void *) value) < 0) {
		string msg =
		    "h5_das handler: unable to read HDF5 attribute data";
		delete[]temp_varname;
		delete[]new_varname;
		delete[]value;
		throw InternalErr(__FILE__, __LINE__, msg);
	    }
	} else {
	    if (H5Aread(attr_id, memtype, (void *) value) < 0) {
		string msg =
		    "h5_das handler: unable to read HDF5 attribute data";
		delete[]temp_varname;
		delete[]new_varname;
		delete[]value;
		throw InternalErr(__FILE__, __LINE__, msg);
	    }
	}

	// add all the attributes in the array

	tempvalue = value;
	/* create the "name" attribute if we can find long_name.
	   Make it compatible with HDF4 server. */
	if (strcmp(attr_inst.name, "long_name") == 0) {
	    for (loc = 0; loc < (int) attr_inst.nelmts; loc++) {
		try {
		    char *print_rep = print_attr(ty_id, loc, tempvalue);

		    attr_table_ptr->append_attr(attr_name,
						print_type(ty_id),
						print_rep);
		    delete[]print_rep;
		}
		catch(Error & e) {
		    delete[]temp_varname;
		    delete[]new_varname;
		    delete[]value;
		    delete [] print_rep;
		    throw;
		}
	    }
	}
	// for scalar data, just read data once a time, change it into
	// DODS string. 

	if (attr_inst.ndims == 0) {
	    for (loc = 0; loc < (int) attr_inst.nelmts; loc++) {
		try {
		    char *print_rep = print_attr(ty_id, loc, tempvalue);

		    attr_table_ptr->append_attr(attr_inst.name,
						print_type(ty_id),
						print_rep);
		    delete[]print_rep;
		}
		catch(Error & e) {
		    delete[]temp_varname;
		    delete[]new_varname;
		    delete[]value;
		    delete [] print_rep;
		}
	    }
	} else {

	    /* 1. if the hdf5 data type is HDF5 string and ndims is not 0;
	       we will handle this differently. */

	    loc = 0;
	    int elesize = (int) H5Tget_size(attr_inst.type);

	    if (elesize == 0) {
		string msg = "h5_das handler: unable to get attibute size";

		delete[]temp_varname;
		delete[]new_varname;
		delete[]value;
		throw InternalErr(__FILE__, __LINE__, msg);
	    }

	    for (int dim = 0; dim < (int) attr_inst.ndims; dim++) {

		for (int sizeindex = 0;
		     sizeindex < (int) attr_inst.size[dim]; sizeindex++) {

		    if (H5Tget_class(attr_inst.type) == H5T_STRING) {
			try {
			    char *print_rep =
				print_attr(ty_id, loc, tempvalue);
			    attr_table_ptr->append_attr(attr_inst.name,
							print_type(ty_id),
							print_rep);
			    tempvalue = tempvalue + elesize;
			    printf("print_rep %s\n", print_rep);
			    delete[]print_rep;
			}
			catch(Error & e) {
			    delete[]temp_varname;
			    delete[]new_varname;
			    delete[]value;
			    delete [] print_rep;
			    throw;
			}
		    }

		    else {
			try {
			    char *print_rep =
				print_attr(ty_id, loc, tempvalue);
			    attr_table_ptr->append_attr(attr_inst.name,
							print_type(ty_id),
							print_rep);
			    delete[]print_rep;
			}
			catch(Error & e) {
			    delete[]temp_varname;
			    delete[]new_varname;
			    delete[]value;
			    delete[]print_rep;
			    throw;
			}
		    }
		}
	    }
	    loc++;
	}
	delete[]value;
    }

    delete[]new_varname;
    delete[]temp_varname;
}

/*-------------------------------------------------------------------------
 * Function:	find_gloattr
 *
 * Purpose:	this function will fill in attributes ofthe root group into 
                one DAS table. The attribute is treated as global attribute. 
		It will call functions  get_attr_info, print_type, print_rep 
 *              

 * Errors:
 *
 * Return:	false, failed. otherwise,success.
  *
 * In :	        DAS object: reference
                object name: absolute name of either a dataset or a group
                error: a string of error message to the dods interface.
                object id: file id 
 *	     
 *
 *-------------------------------------------------------------------------
 */
bool
find_gloattr(hid_t file, DAS & das)
{

    hid_t root;
    int num_attrs;
#if 0
    int i;
#endif

    root = H5Gopen(file, "/");

    if (root < 0) {
	string msg = "unable to open HDF5 root group";
	throw InternalErr(__FILE__, __LINE__, msg);
    }

    num_attrs = H5Aget_num_attrs(root);

    if (num_attrs < 0) {
	string msg = "fail to get attribute number";
	throw InternalErr(__FILE__, __LINE__, msg);
    }

    if (num_attrs == 0) {
	H5Gclose(root);
	return true;
    }

    try {
	read_objects(das, "H5_GLOBAL", root, num_attrs);
    }

    catch(Error & e) {
	H5Gclose(root);
	throw;
    }
    H5Gclose(root);
    return true;
}

/*-------------------------------------------------------------------------
 * Function:	get_softlink(is only a test, not supported in current version.)
 *
 * Purpose:	this function will put softlink information into 
                a DAS table. 
              
 *              

 * Errors:
 *
 * Return:	false, failed. otherwise,success.
  *
 * In :	        DAS object: reference
                object name: absolute name of  a group
                error: a string of error message to the dods interface.
                object id: group id 
 *	     
 *
 *-------------------------------------------------------------------------
 */

bool
get_softlink(DAS & das, hid_t pgroup, const string & oname, int index)
{

    char *buf;
    string finaltrans;
    H5G_stat_t statbuf;
    AttrTable *attr_table_ptr;
    char *temp_varname;
#if 0
    char *cptr;
    char ORI_SLASH = '/';
    char CHA_SLASH = '_';
#endif
    char str_num[6];

    /* softlink attribute name will be "HDF5_softlink" + "link index". */
    sprintf(str_num, "%d", index);
    const char *temp_oname = oname.c_str();
    temp_varname = new char[strlen(HDF5_softlink) + 7];

    strcpy(temp_varname, HDF5_softlink);
    strcat(temp_varname, str_num);

    attr_table_ptr = das.get_table(temp_varname);
    if (!attr_table_ptr) {
	try {
	    attr_table_ptr = das.add_table(temp_varname, new AttrTable);
	}
	catch(Error & e) {
	    delete[]temp_varname;
	    throw;
	}
    }
    /* get the target information at statbuf. */
    herr_t ret = H5Gget_objinfo(pgroup, temp_oname, 0, &statbuf);

    if (ret < 0) {
	delete[]temp_varname;
	throw InternalErr(__FILE__, __LINE__,
		   "h5_das handler: cannot get hdf5 group information");
    }
    buf = new char[statbuf.linklen * sizeof(char) + 1];

    if (!buf) {
	delete[]temp_varname;
	throw InternalErr(__FILE__, __LINE__, 
		   "h5_das handler: no enough memory to hold softlink");
    }
    /* get link target name */
    if (H5Gget_linkval(pgroup, temp_oname, statbuf.linklen + 1, buf) < 0) {
	string msg = "h5das handler: unable to get link value. ";

	delete[]temp_varname;
	delete[]buf;
	throw InternalErr(__FILE__, __LINE__, msg);

	return false;
    }

    char *finbuf = new char (strlen(buf) + 3);

    try {
	bzero(finbuf, strlen(buf) + 3);
	sprintf(finbuf, "\"%s\"", buf);
	attr_table_ptr->append_attr(oname, STRING, finbuf);
    }

    catch(Error & e) {
	delete[]temp_varname;
	delete[]buf;
	delete[]finbuf;
	throw;
    }

    delete[]temp_varname;
    delete[]buf;
    delete[]finbuf;

    return true;
}
