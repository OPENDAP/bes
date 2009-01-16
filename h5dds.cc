////////////////////////////////////////////////////////////////////////////////
/// \file h5dds.cc
/// \brief DDS/DODS request processing source
///
/// This file is part of h5_dap_handler, a C++ implementation of the DAP handler
/// for HDF5 data.
///
/// This file contains functions which use depth-first search to walk through
/// an hdf5 file and build the in-memeory DDS.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang <myang6@hdfgroup.org>
///
/// Copyright (c) 2007 The HDF Group
///
/// Copyright (c) 1999 National Center for Supercomputing Applications.
/// 
/// All rights 
// #define DODS_DEBUG

#include "config_hdf5.h"

#include <InternalErr.h>
#include <util.h>
#include <debug.h>

#include "h5dds.h"
#include "HDF5Int32.h"
#include "HDF5UInt32.h"
#include "HDF5UInt16.h"
#include "HDF5Int16.h"
#include "HDF5Byte.h"
#include "HDF5Array.h"
#include "HDF5ArrayEOS.h"
#include "HDF5Str.h"
#include "HDF5Float32.h"
#include "HDF5Float64.h"
#include "HDF5Grid.h"
#include "HDF5GridEOS.h"
#include "HDF5Url.h"
#include "HDF5Structure.h"
#include "H5Git.h"
#include "H5EOS.h"

extern H5EOS eos;
extern string get_hardlink(hid_t, const string &);

/// This variable is used to generate internal error message.
static char Msgt[MAX_ERROR_MESSAGE];

/// An instance of DS_t structure defined in common.h.
static DS_t dt_inst;

////////////////////////////////////////////////////////////////////////////////
/// \fn depth_first(hid_t pid, char *gname, DDS & dds, const char *fname)
/// will fill DDS table.
///
/// This function will walk through hdf5 \a gname group
/// using depth-first approach toobtain data information
/// (data type and data pattern) of all hdf5 datasets and then
/// put them into dds table.
///
/// \param pid group id
/// \param gname group name (absolute name from root group)
/// \param dds reference of DDS object
/// \param fname file name
///
/// \return 0, if failed.
/// \return 1, if succeeded.
///
/// \remarks hard link is treated as a dataset.
/// \remarks will return error message to the DAP interface.
/// \see depth_first(hid_t pid, char *gname, DAS & das, const char *fname)
////////////////////////////////////////////////////////////////////////////////
bool depth_first(hid_t pid, char *gname, DDS & dds, const char *fname)
{

    // Iterate through the file to see members of the root group 
    DBG(cerr << ">depth_first() pid: " << pid << " gname: " << gname <<
        " fname: " << fname << endl);

    hsize_t nelems = 0;
    if (H5Gget_num_objs(pid, &nelems) < 0) {
        string msg =
            "h5_das handler: counting hdf5 group elements error for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    DBG(cerr << " nelems = " << nelems << endl);

    for (int i = 0; i < nelems; i++) {
        char *oname = NULL;
        int type = -1;
        ssize_t oname_size = 0;

        // Query the length
        oname_size =
            H5Gget_objname_by_idx(pid, (hsize_t) i, NULL,
                                  (size_t) DODS_NAMELEN);

        if (oname_size <= 0) {
            string msg = "Error getting the size of hdf5 the object: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        // Obtain the name of the object 
        oname = new char[(size_t) oname_size + 1];
        if (H5Gget_objname_by_idx
            (pid, (hsize_t) i, oname, (size_t) (oname_size + 1)) < 0) {
            string msg =
                "h5_dds handler: getting the hdf5 object name error from";
            msg += gname;
            delete[]oname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        type = H5Gget_objtype_by_idx(pid, (hsize_t) i);
        if (type < 0) {
            string msg =
                "h5_dds handler: getting the hdf5 object type error from";
            msg += gname;
            delete[]oname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        switch (type) {         // Can we use virtual function? <hyokyung
				// 2007.02.20. 10:17:24>

	  case H5G_GROUP: {
	      string full_path_name =
		  string(gname) + string(oname) + "/";

	      DBG(cerr << "=depth_first():H5G_GROUP " << full_path_name
		  << endl);

	      // Check the hard link loop and break the loop.

	      char *t_fpn = new char[full_path_name.length() + 1];

	      strcpy(t_fpn, full_path_name.c_str());
	      hid_t cgroup = H5Gopen(pid, t_fpn);
	      try {
		  string oid = get_hardlink(pid, oname);
		  if (oid == "") {
		      depth_first(cgroup, t_fpn, dds, fname);
		  }

	      }
	      catch(...) {

		  delete[]oname;
		  delete[]t_fpn;
		  throw;
	      }
	      H5Gclose(cgroup);
	      delete[]t_fpn;
	      break;
	  }

	  case H5G_DATASET:{
	      string full_path_name = string(gname) + string(oname);
	      // Obtain hdf5 dataset handle. 
	      get_dataset(pid, full_path_name, &dt_inst);
	      // Put hdf5 dataset structure into DODS dds.
	      // read_objects throws InternalErr.

	      read_objects(dds, full_path_name, fname);
	      break;
	  }

        case H5G_TYPE:
        default:
            break;
        }

        delete[]oname;
    } // for i is 0 ... nelems

    DBG(cerr << "<depth_first() " << endl);
    return true;
}


////////////////////////////////////////////////////////////////////////////////
/// \fn return_type(hid_t type)
/// returns the string representation of HDF5 type.
///
/// This function will get the text representation(string) of the corresponding
/// DODS datatype. DODS-HDF5 subclass method will use this function.
///
/// \return string
/// \param type datatype id
////////////////////////////////////////////////////////////////////////////////
string return_type(hid_t type)
{
    size_t size = 0;
    H5T_sign_t sign;

    switch (H5Tget_class(type)) {

    case H5T_INTEGER:
        //  <hyokyung 2007.02.27. 13:29:14>
        size = H5Tget_size(type);
        sign = H5Tget_sign(type);
        DBG(cerr << "=return_type(): H5T_INTEGER sign = " << sign << " size = " <<
            size << endl);
        if (size == 1){
            if (sign == H5T_SGN_NONE)       // <hyokyung 2009.01.14. 10:42:50>
		return BYTE;	
	    else
		return INT8;
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
        return INT_ELSE;

    case H5T_FLOAT:
        size = H5Tget_size(type);
        DBG(cerr << "=return_type(): FLOAT size = " << size << endl);
        if (size == 4)
            return FLOAT32;
        if (size == 8)
            return FLOAT64;
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

////////////////////////////////////////////////////////////////////////////////
/// \fn Get_bt(string varname, hid_t datatype, const HDF5TypeFactory &factory)
/// returns the pointer of base type
///
/// This function will create a new DODS object that corresponds to HDF5
/// dataset and return a pointer of a new object of DODS datatype. If an error
/// is found, an exception of type InternalErr is thrown. 
///
/// \param varname object name
/// \param dataset name of dataset where this object comes from
/// \param datatype datatype id
/// \return pointer to BaseType
////////////////////////////////////////////////////////////////////////////////
static BaseType *Get_bt(const string &varname,
			const string &dataset,
			hid_t datatype)
{
    BaseType *btp = NULL;
    
    // get_short_name() returns a shortened name if SHORT_PATH is defined.
    // It does nothing if the symbol is undefined. jhrg 5/1/08
    string vname = get_short_name(varname);

    try {

	DBG(cerr << ">Get_bt varname=" << varname << " datatype=" << datatype
	    << endl);

	size_t size = 0;
	int sign = -2;
	switch (H5Tget_class(datatype)) {

	  case H5T_INTEGER:
	    size = H5Tget_size(datatype);
	    sign = H5Tget_sign(datatype);
	    DBG(cerr << "=Get_bt() H5T_INTEGER size = " << size << " sign = "
		<< sign << endl);

	    if (size == 1) {
	      // <hyokyung 2009.01.13. 13:46:29>
	      if (sign == H5T_SGN_2)
		btp = new HDF5Int16(vname, dataset);
	      else
		btp = new HDF5Byte(vname, dataset);
	    }
	    else if (size == 2) {
		if (sign == H5T_SGN_2)
		    btp = new HDF5Int16(vname, dataset);
		else
		    btp = new HDF5UInt16(vname, dataset);
	    }
	    else if (size == 4) {
		if (sign == H5T_SGN_2)
		    btp = new HDF5Int32(vname, dataset);
		else
		    btp = new HDF5UInt32(vname, dataset);
	    }
	    // <hyokyung 2007.06.15. 12:42:09>
	    else if (size == 8) {
	      throw InternalErr(__FILE__, __LINE__,
				string("Unsupported HDF5 64-bit Integer type:  ") + vname);
	      //	      <hyokyung 2009.01. 9. 15:13:52>	    
	      // 		if (sign == H5T_SGN_2) 
	      // 		    btp = new HDF5Int32(vname, dataset);
	      // 		else
	      // 		    btp = new HDF5UInt32(vname, dataset);
	    }

	    break;

	  case H5T_FLOAT:
	    size = H5Tget_size(datatype);
	    DBG(cerr << "=Get_bt() H5T_FLOAT size = " << size << endl);

	    if (size == 4) {
		btp = new HDF5Float32(vname, dataset);
	    }
	    else if (size == 8) {
		btp = new HDF5Float64(vname, dataset);
	    }
	    break;

	  case H5T_STRING:
	    btp = new HDF5Str(vname, dataset);
	    break;

	  case H5T_ARRAY: {
	      BaseType *ar_bt = 0;
	      try {
		  DBG(cerr << "=Get_bt() H5T_ARRAY datatype = " << datatype << endl);

		  // Get the array's base datatype.
		  hid_t dtype_base = H5Tget_super(datatype);
		  ar_bt = Get_bt(vname, dataset, dtype_base);
		  btp = new HDF5Array(vname, dataset, ar_bt);
		  delete ar_bt; ar_bt = 0;

		  // Set the size of the array.
		  int ndim = H5Tget_array_ndims(datatype);
		  size = H5Tget_size(datatype);
		  int nelement = 1;

		  DBG(cerr << "=Get_bt()" << " Dim = " << ndim << " Size = " << size
		      << endl);

		  hsize_t size2[DODS_MAX_RANK];
		  int perm[DODS_MAX_RANK];		// not used
		  H5Tget_array_dims(datatype, size2, perm);

		  HDF5Array &h5_ar = dynamic_cast < HDF5Array & >(*btp);
		  for (int dim_index = 0; dim_index < ndim; dim_index++) {
		      h5_ar.append_dim(size2[dim_index]);
		      DBG(cerr << "=Get_bt() " << size2[dim_index] << endl);
		      nelement = nelement * size2[dim_index];
		  }

		  h5_ar.set_did(dt_inst.dset);
		  // Assign the array datatype id.
		  h5_ar.set_tid(datatype);
		  h5_ar.set_memneed(size);
		  h5_ar.set_numdim(ndim);
		  h5_ar.set_numelm(nelement);
		  h5_ar.set_length(nelement);
		  h5_ar.d_type = H5Tget_class(dtype_base);
	      }
	      catch (...) {
		  delete ar_bt;
		  throw;
	      }
	      break;
	  }

	  case H5T_REFERENCE:
	    btp = new HDF5Url(vname, dataset);
	    break;
        
	  default:
	    throw InternalErr(__FILE__, __LINE__,
			      string("Unsupported HDF5 type:  ") + vname);
	}
    }
    catch (...) {
	delete btp;
	throw;
    }

    if (!btp)
	throw InternalErr(__FILE__, __LINE__,
			  string("Could not make a DAP variable for: ")
			  + vname);
						  
    switch (btp->type()) {

      case dods_byte_c: {
	  HDF5Byte &v = dynamic_cast < HDF5Byte & >(*btp);
	  v.set_did(dt_inst.dset);
	  v.set_tid(dt_inst.type);
	  break;
      }
	
      case dods_int16_c: {
	  HDF5Int16 &v = dynamic_cast < HDF5Int16 & >(*btp);
	  v.set_did(dt_inst.dset);
	  v.set_tid(dt_inst.type);
	  break;
      }
      case dods_uint16_c: {
	  HDF5UInt16 &v = dynamic_cast < HDF5UInt16 & >(*btp);
	  v.set_did(dt_inst.dset);
	  v.set_tid(dt_inst.type);
	  break;
      }
      case dods_int32_c: {
	  HDF5Int32 &v = dynamic_cast < HDF5Int32 & >(*btp);
	  v.set_did(dt_inst.dset);
	  v.set_tid(dt_inst.type);
	  break;
      }
      case dods_uint32_c: {
	  HDF5UInt32 &v = dynamic_cast < HDF5UInt32 & >(*btp);
	  v.set_did(dt_inst.dset);
	  v.set_tid(dt_inst.type);
	  break;
      }
      case dods_float32_c: {
	  HDF5Float32 &v = dynamic_cast < HDF5Float32 & >(*btp);
	  v.set_did(dt_inst.dset);
	  v.set_tid(dt_inst.type);
	  break;
      }
      case dods_float64_c: {
	  HDF5Float64 &v = dynamic_cast < HDF5Float64 & >(*btp);
	  v.set_did(dt_inst.dset);
	  v.set_tid(dt_inst.type);
	  break;
      }
      case dods_str_c: {
	  HDF5Str &v = dynamic_cast < HDF5Str & >(*btp);
	  v.set_did(dt_inst.dset);
	  v.set_tid(dt_inst.type);
	  break;
      }
      case dods_array_c:
	break;
	
      case dods_url_c: {
	  HDF5Url &v = dynamic_cast < HDF5Url & >(*btp);
	  v.set_did(dt_inst.dset);
	  v.set_tid(dt_inst.type);
	  break;
      }
      default:
	delete btp;
	throw InternalErr(__FILE__, __LINE__,
			  string("error counting hdf5 group elements for ") 
			  + vname);
    }
    DBG(cerr << "<Get_bt()" << endl);
    return btp;
}


////////////////////////////////////////////////////////////////////////////////
/// \fn Get_structure(string varname, hid_t datatype)
/// returns a pointer of structure type. An exception is thrown if an error
/// is encountered.
/// 
/// This function will create a new DODS object that corresponds to HDF5
/// compound dataset and return a pointer of a new structure object of DODS.
///
/// \param varname object name
/// \param dataset name of the dataset from which this structure created
/// \param datatype datatype id
/// \return pointer to Structure type
///
////////////////////////////////////////////////////////////////////////////////
static Structure *Get_structure(const string &varname,
				const string &dataset,
				hid_t datatype)
{
    HDF5Structure *structure_ptr = NULL;

    string vname = get_short_name(varname);

    DBG(cerr << ">Get_structure()" << datatype << endl);

    if (H5Tget_class(datatype) != H5T_COMPOUND)
	throw InternalErr(__FILE__, __LINE__,
			  string("Compound-to-structure mapping error for ")
			  + vname);

    try {
	structure_ptr = new HDF5Structure(vname, dataset);
	//FIXME: HDF5Structure &v = *structure_ptr;
	structure_ptr->set_did(dt_inst.dset);
	structure_ptr->set_tid(dt_inst.type);

	// Retrieve member types
	int nmembs = H5Tget_nmembers(datatype);
	DBG(cerr << "=Get_structure() has " << nmembs << endl);
	for (int i = 0; i < nmembs; i++) {
	    char *memb_name = H5Tget_member_name(datatype, i);
	    H5T_class_t memb_cls = H5Tget_member_class(datatype, i);
	    hid_t memb_type = H5Tget_member_type(datatype, i);

	    if (memb_cls < 0 | memb_type < 0) {
		throw InternalErr(__FILE__, __LINE__,
				  string("Type mapping error for ")
				  + string(memb_name) );
	    }
	    
	    // ~Structure() will delete these if they are added.
	    if (memb_cls == H5T_COMPOUND) {
		Structure *s = Get_structure(memb_name, dataset, memb_type);
		structure_ptr->add_var(s);
		delete s; s = 0;
	    } 
	    else {
		BaseType *bt = Get_bt(memb_name, dataset, memb_type);
		structure_ptr->add_var(bt);
		delete bt; bt = 0;
	    }
	}
    }
    catch (...) {
	delete structure_ptr;
	throw;
    }

    DBG(cerr << "<Get_structure()" << endl);

    return structure_ptr;
}

static hid_t get_dimension_list_attr_id(H5GridFlag_t check_grid, hid_t dset,
					const string &name1, 
					const string &name2) {
    hid_t attr_id;
    if (check_grid == NewH4H5Grid) {
	if ((attr_id = H5Aopen_name(dset, name1.c_str())) < 0)
	    throw InternalErr(__FILE__, __LINE__,
			      string("Unable to open ") + name1 
			      + string(" attribute"));
    } 
    else {
	if ((attr_id = H5Aopen_name(dset, name2.c_str())) < 0)
	    throw InternalErr(__FILE__, __LINE__,
			      string("Unable to open ") + name2
			      + string(" attribute"));
    }

    return attr_id;
}

// This function modifies the Grid pointer 'gr' as a side effect.
static void process_grid(const H5GridFlag_t check_grid, 
			 Grid *gr) {

    DBG(cerr << "add_var()" << endl);

    // Obtain dimensional scale name, it should be a list of
    // dimensional names. Here we will distinguish old h4h5 tool or
    // new h4h5 tool.
    hid_t attr_id = get_dimension_list_attr_id(check_grid, dt_inst.dset,
					       "HDF5_DIMENSIONNAMELIST",
					       "OLD_HDF5_DIMENSIONNAMELIST");
    hid_t temp_dtype  = H5Aget_type(attr_id);
    size_t temp_tsize = H5Tget_size(temp_dtype);
    hid_t temp_dspace = H5Aget_space(attr_id);
    hsize_t temp_nelm = H5Sget_simple_extent_npoints(temp_dspace);

    char *dimname = new char[temp_nelm * temp_tsize];
    try {
	if (H5Aread(attr_id, temp_dtype, dimname) < 0)
	    throw InternalErr(__FILE__, __LINE__, 
			      "Unable to get the attribute");
	H5Tclose(temp_dtype);
	H5Sclose(temp_dspace);
	H5Aclose(attr_id);

	// obtain dimensional scale data information 
	attr_id = get_dimension_list_attr_id(check_grid, dt_inst.dset,
					     "HDF5_DIMENSIONLIST",
					     "OLD_HDF5_DIMENSIONLIST");
	temp_dtype = H5Aget_type(attr_id);
	temp_tsize = H5Tget_size(temp_dtype);
	temp_dspace = H5Aget_space(attr_id);
	temp_nelm = H5Sget_simple_extent_npoints(temp_dspace);

	char *buf = new char[temp_nelm * temp_tsize];
	memset(buf, 0, temp_nelm * temp_tsize);
	hid_t *dimid = 0;
	char *EachDimName = 0;
	try {
	    if (H5Aread(attr_id, H5T_STD_REF_OBJ, buf) < 0)
		throw InternalErr(__FILE__, __LINE__,
				  "Cannot read object reference attributes.");
	    hobj_ref_t *refbuf = (hobj_ref_t *) buf;
	    dimid = new hid_t[temp_nelm];

	    if (!dimid)
		throw InternalErr(__FILE__, __LINE__,
				  "Error allocating memory");

	    for (unsigned int j = 0; j < temp_nelm; j++) {
		dimid[j] = H5Rdereference(attr_id, H5R_OBJECT, refbuf);
		if (dimid[j] < 0)
		    throw InternalErr(__FILE__, __LINE__,
				      "cannot dereference the object.");
		refbuf++;
	    }

	    H5Aclose(attr_id);
	    H5Sclose(temp_dspace);
	    H5Tclose(temp_dtype);

	    // Start building Grid.
	    char *TempNamePointer = dimname;
	    //    size_t name_size = temp_tsize;
	    EachDimName = new char[temp_tsize/*name_size*/];

	    for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
		// Get dimensional scale datasets. Add them to grid.
		temp_dspace = H5Dget_space(dimid[dim_index]);
		temp_nelm = H5Sget_simple_extent_npoints(temp_dspace);
		temp_dtype = H5Dget_type(dimid[dim_index]);
		hid_t memtype = H5Tget_native_type(temp_dtype, H5T_DIR_ASCEND);
		temp_tsize = H5Tget_size(memtype);

		strcpy(EachDimName, TempNamePointer);
		TempNamePointer = TempNamePointer + temp_tsize/*name_size*/;
		BaseType *bt = 0;
		HDF5Array *map = 0;
		try {
		    bt = Get_bt(EachDimName, gr->dataset(), memtype);
		    map = new HDF5Array(EachDimName, gr->dataset(), bt);
		    delete bt; bt = 0;
		    map->set_did(dimid[dim_index]);
		    map->set_tid(memtype);
		    map->set_memneed(temp_tsize * temp_nelm);
		    map->set_numdim(1);
		    map->set_numelm(temp_nelm);
		    map->append_dim(temp_nelm, EachDimName);
		    gr->add_var(map, maps);
		    delete map; map = 0;
		}
		catch(...) {
		    if( bt ) delete bt;
		    if( map ) delete map;
		    throw;
		}
	    } // for dim_index is 0 .. dt_inst.ndims

	    delete[] buf;
	    delete[] dimid;
	    delete[] EachDimName;
	}
	catch(...) {
	    delete[] buf;
	    delete[] dimid;
	    delete[] EachDimName;
	    throw;
	}

	delete[] dimname;
    }
    catch(...) {
	delete[] dimname;
	throw;
    }
}

static void process_grid_matching_dimscale(const H5GridFlag_t check_grid, 
					   Grid *gr) {
    hid_t attr_id = H5Aopen_name(dt_inst.dset, "DIMENSION_LIST");
    hid_t temp_dtype = H5Aget_type(attr_id);
    hid_t temp_dspace = H5Aget_space(attr_id);
    hsize_t temp_nelm = H5Sget_simple_extent_npoints(temp_dspace);

    hvl_t *refbuf = 0;
    memset(refbuf, 0, temp_nelm);
    hid_t *dimid = 0;
    try {
	refbuf = new hvl_t[temp_nelm];
	memset(refbuf, 0, temp_nelm);
	dimid = new hid_t[temp_nelm];

	// Should this throw an exception? jhrg
	if (H5Aread(attr_id, temp_dtype, refbuf) < 0)
	    cerr << "Cannot read object reference attributes." << endl;

	for (unsigned int j = 0; j < temp_nelm; j++) {
	    dimid[j] = H5Rdereference(attr_id, H5R_OBJECT, refbuf[j].p);
	}
	
	// Is there a way to know the size of dimension name in advance? 
	char buf2[DODS_NAMELEN];

	for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
	    H5Iget_name(dimid[dim_index], (char *) buf2, DODS_NAMELEN);
	    DBG(cerr << "name: " << buf2 << endl);
	    // Open dataset.
	    // Is it OK to search from the current dset (i.e.
	    // dt_inst.dset) ? 
	    hid_t dset_id = H5Dopen(dt_inst.dset, buf2);
	    DBG(cerr << "dataset id: " << dset_id << endl);
	    // Get the size of the array.
	    temp_dspace = H5Dget_space(dset_id);
	    hsize_t temp_nelm_dim = H5Sget_simple_extent_npoints(temp_dspace);
	    DBG(cerr << "nelem = " << temp_nelm_dim << endl);
	    temp_dtype = H5Dget_type(dset_id);
	    hid_t memtype = H5Tget_native_type(temp_dtype, H5T_DIR_ASCEND);
	    size_t temp_tsize = H5Tget_size(memtype);

	    string each_dim_name(buf2);

	    each_dim_name = get_short_name(each_dim_name);
	    BaseType *bt = 0;
	    HDF5Array *map = 0;
	    try {
		bt = Get_bt(each_dim_name, gr->dataset(), memtype);
		map = new HDF5Array(each_dim_name, gr->dataset(), bt);
		delete bt; bt = 0;
		map->set_did(dset_id);
		map->set_tid(memtype);
		map->set_memneed(temp_tsize * temp_nelm_dim);
		map->set_numdim(1);
		map->set_numelm(temp_nelm);

		map->append_dim(temp_nelm_dim, each_dim_name);
		gr->add_var(map, maps);
		delete map; map = 0;
	    }
	    catch(...) {
		if( bt ) delete bt;
		if( map ) delete map;
		throw;
	    }
	} // for ()

	delete[] dimid;
	delete[] refbuf;
    }
    catch(...) {
	delete[] dimid;
	delete[] refbuf;
	throw;
    }
}

static void process_grid_nasa_eos(const string &varname, 
				  Array *array, Grid *gr, DDS &dds_table) {
    // Next fill the map part of the grid.
    // Retrieve the dimension lists from the parsed metadata.
    vector < string > tokens;
    eos.get_dimensions(varname, tokens);
    DBG(cerr << "=read_objects_base_type():Number of dimensions "
	<< dt_inst.ndims << endl);

    for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
	DBG(cerr << "=read_objects_base_type():Dim name " <<
	    tokens.at(dim_index) << endl);

	string str_dim_name = tokens.at(dim_index);

	// Retrieve the full path to the each dimension name.
	string str_grid_name = eos.get_grid_name(varname);
	string str_dim_full_name = str_grid_name + str_dim_name;

	int dim_size = eos.get_dimension_size(str_dim_full_name);

#ifdef SHORT_PATH
	str_dim_full_name = str_dim_name;
#endif

#ifdef CF
	// Rename dimension name according to CF convention.
	str_dim_full_name =
	    eos.get_CF_name((char *) str_dim_full_name.c_str());
#endif

	BaseType *bt = 0;
	Array *ar = 0;
	try {
	    bt = new HDF5Float32(str_dim_full_name, gr->dataset());
	    ar = new HDF5Array(str_dim_full_name, gr->dataset(), bt);
	    delete bt; bt = 0;

	    ar->append_dim(dim_size, str_dim_full_name);
	    array->append_dim(dim_size, str_dim_full_name);

	    gr->add_var(ar, maps);
	    delete ar; ar = 0;
	    
	}
	catch (...) {
	    if( bt ) delete bt;
	    if( ar ) delete ar;
	    throw;
	}
    }

#ifdef CF
  // Add all shared dimension data.
  if (!eos.is_shared_dimension_set()) {

    int j;
    BaseType *bt = 0;
    Array *ar = 0;    
    vector < string > dimension_names;
    eos.get_all_dimensions(dimension_names);

    for(j=0; j < dimension_names.size(); j++){
      int shared_dim_size = eos.get_dimension_size(dimension_names.at(j));    
      string str_cf_name = eos.get_CF_name((char*)dimension_names.at(j).c_str());
      bt = new HDF5Float32(str_cf_name, gr->dataset());
      ar = new HDF5ArrayEOS(str_cf_name,gr->dataset(), bt);

      ar->add_var(bt);
      delete bt; bt = 0;
      ar->append_dim(shared_dim_size, str_cf_name);
      dds_table.add_var(ar);
      delete ar; ar = 0;
      // Set the flag for "shared dimension" true.
    }
    eos.set_shared_dimension();
  }    
#endif
}

////////////////////////////////////////////////////////////////////////////////
/// \fn read_objects_base_type(DDS & dds_table,
///                            const string & varname,
///                            const string & filename)
/// fills in information of a dataset (name, data type, data space) into one
/// DDS table.
/// 
/// Given a reference to an instance of class DDS and a filename that refers
/// to an hdf5 file, read hdf5 file and extract all the dimensions of
/// each of its variables. Add the variables and their dimensions to the
/// instance of DDS.
///
/// It will use dynamic cast toput necessary information into subclass of dods
/// datatype. 
///
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of either a dataset or a group
///    \param filename Added to the DDS (dds_table).
///    \throw error a string of error message to the dods interface.
////////////////////////////////////////////////////////////////////////////////
void
read_objects_base_type(DDS & dds_table, const string & a_name,
                       const string & filename)
{
    dds_table.set_dataset_name(name_path(filename));
    string varname = a_name;

    // Get base type. It should be int, float and double etc. atomic
    // datatype. 
    BaseType *bt = Get_bt(varname, filename, dt_inst.type);

    if (!bt) {
        // NB: We're throwing InternalErr even though it's possible that
        // someone might ask for an HDF5 varaible which this server cannot
        // handle.
        throw
            InternalErr(__FILE__, __LINE__,
                        "Unable to convert hdf5 datatype to dods basetype");
    }

    // First deal with scalar data. 
    if (dt_inst.ndims == 0) {
        dds_table.add_var(bt);
	delete bt; bt = 0;
    }
    else {
	// Next, deal with Array and Grid data. This 'else clause' runs to
	// the end of the method. jhrg
        varname = get_short_name(varname);
#ifdef CF
        if (varname.length() > DODS_CF_NAMELEN)	// <hyokyung 2009.01.16. 09:45:30>
            return;
#endif
        HDF5Array *ar = new HDF5Array(varname, filename, bt);
	delete bt; bt = 0;
        ar->set_did(dt_inst.dset);
        ar->set_tid(dt_inst.type);
        ar->set_memneed(dt_inst.need);
        ar->set_numdim(dt_inst.ndims);
        ar->set_numelm((int) (dt_inst.nelmts));

#ifdef NASA_EOS_GRID
        if (!(eos.is_valid() && eos.is_grid(varname)))
            for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++)
                ar->append_dim(dt_inst.size[dim_index]);
#else
	for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++)
	    ar->append_dim(dt_inst.size[dim_index]);
#endif

        // This needs to be fully supported! <hyokyung 2007.02.20. 11:53:11>
        // DODSGRID is defined in common.h by default.

#ifndef DODSGRID
        // Not define DODS Grid. It has to be an array.
        dds_table.add_var(ar);
        delete ar; ar = 0;

#else // DODSGRID is defined

        // Check whether this HDF5 dataset can be mapped to the grid data
        // type. It should check whether the attribute includes dimension
        // list. If yes and everything is valid, map to DAP grid; Otherwise,
        // map to DAP array.
        H5GridFlag_t check_grid = maptogrid(dt_inst.dset, dt_inst.ndims);

        if (check_grid != NotGrid) {    // !NotGrid means it's a Grid.
	    Grid *gr = new HDF5Grid(varname, filename);
	    // First fill the array part of the grid.
	    gr->add_var(ar, array);
	    delete ar; ar = 0;

	    process_grid(check_grid, gr);

	    dds_table.add_var(gr);
	    delete gr; gr = 0;
        }
        else if (has_matching_grid_dimscale
                 (dt_inst.dset, dt_inst.ndims, dt_inst.size)) {

            // Construct a grid instead of returning a simple array.

            Grid *gr = new HDF5Grid(varname, filename);
            gr->add_var(ar, array);
            delete ar; ar = 0;

	    process_grid_matching_dimscale(check_grid, gr);

	    dds_table.add_var(gr);
	    delete gr; gr = 0;
        }
#ifdef NASA_EOS_GRID
        // Check if eos class has this dataset as Grid.
        else if (eos.is_valid() && eos.is_grid(varname)) {
            DBG(cerr << "EOS Grid: " << varname << endl);
            // Generate grid based on the parsed StructMetada.
            Grid *gr = new HDF5GridEOS(varname, filename);

	    process_grid_nasa_eos(varname, ar, gr, dds_table);
            gr->add_var(ar, array);
            delete ar; ar = 0;

            dds_table.add_var(gr);
            delete gr; gr = 0;
	}
#endif                          // #ifdef  NASA_EOS_GRID
        else {                  // cannot be mapped to grid, must be an array.
            dds_table.add_var(ar);
            delete ar; ar = 0;
        }
#endif                          // #ifndef DODSGRID
    }

    DBG(cerr << "<read_objects_base_type(dds)" << endl);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn read_objects_structure(DDS & dds_table,const string & varname,
///                  const string & filename)
/// fills in information of a structure dataset (name, data type, data space)
/// into a DDS table.
/// 
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of structure
///    \param filename Added to the DDS (dds_table).
///    \throw error a string of error message to the dods interface.
////////////////////////////////////////////////////////////////////////////////  
void
read_objects_structure(DDS & dds_table, const string & varname,
                       const string & filename)
{
    dds_table.set_dataset_name(name_path(filename));

    Structure *structure = Get_structure(varname, filename, dt_inst.type);
    try {
	// Assume Get_structure() uses exceptions to signal an error. jhrg
	DBG(cerr << "=read_objects_structure(): Dimension is " 
	    << dt_inst.ndims << endl);

	if (dt_inst.ndims != 0) {   // Array of Structure
	    int dim_index;
	    DBG(cerr << "=read_objects_structure(): array of size " <<
		dt_inst.nelmts << endl);
	    DBG(cerr << "=read_objects_structure(): memory needed = " <<
		dt_inst.need << endl);
	    HDF5Array *ar = new HDF5Array(varname, filename, structure);
	    delete structure; structure = 0;
	    try {
		ar->set_did(dt_inst.dset);
		ar->set_tid(dt_inst.type);
		ar->set_memneed(dt_inst.need);
		ar->set_numdim(dt_inst.ndims);
		ar->set_numelm((int) (dt_inst.nelmts));
		ar->set_length((int) (dt_inst.nelmts));

		for (dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
		    ar->append_dim(dt_inst.size[dim_index]);
		    DBG(cerr << "=read_objects_structure(): append_dim = " <<
			dt_inst.size[dim_index] << endl);
		}

		dds_table.add_var(ar);
		delete ar; ar = 0;
	    } // try Array *ar
	    catch (...) {
		delete ar;
		throw;
	    }
	} else {
	    dds_table.add_var(structure);
	    delete structure; structure = 0;

	}
    } // try     Structure *structure = Get_structure(...)
    catch (...) {
	delete structure;
	throw;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn read_objects(DDS & dds_table,const string & varname,
///                  const string & filename)
/// fills in information of a dataset (name, data type, data space) into one
/// DDS table.
/// 
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of either a dataset or a group
///    \param filename Added to the DDS (dds_table).
///    \throw error a string of error message to the dods interface.
////////////////////////////////////////////////////////////////////////////////  
void
read_objects(DDS & dds_table, const string &varname, const string &filename)
{

    switch (H5Tget_class(dt_inst.type)) {

    case H5T_COMPOUND:
        read_objects_structure(dds_table, varname, filename);
        break;

    default:
        read_objects_base_type(dds_table, varname, filename);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn get_short_name(string varname)
/// returns a short name.
///
/// This function returns a short name from \a varname.
/// Short name is defined as  a string from the last '/' to the end of string
/// excluding the '/'.
/// 
/// \param varname a full object name that has a full group path information
/// \return a shortened string
////////////////////////////////////////////////////////////////////////////////  
string get_short_name(string varname)
{
#ifdef SHORT_PATH
    int pos = varname.find_last_of('/', varname.length() - 1);
    return varname.substr(pos + 1);
#else
    return varname;
#endif
}

