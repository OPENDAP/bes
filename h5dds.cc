////////////////////////////////////////////////////////////////////////////////
/// \file h5dds.cc
/// \brief DDS/DODS request processing source
///
/// This file is part of h5_dap_handler, A C++ implementation of the DAP handler
/// for HDF5 data.
///
/// This file contains functions which uses depth-first search to walk through
/// a hdf5 file and build the in-memeory DDS.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang <ymuqun@hdfgroup.org>
///
/// Copyright (c) 2007 HDF Group
///
/// Copyright (c) 1999 National Center for Supercomputing Applications.
/// 
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////
// #define DODS_DEBUG
#include "config_hdf5.h"
#include "debug.h"
#include "h5dds.h"
#include "HDF5Structure.h"
#include "HDF5TypeFactory.h"
#include "InternalErr.h"
#include "H5Git.h"


#include "H5EOS.h"
extern H5EOS eos;


static char Msgt[MAX_ERROR_MESSAGE];
static DS_t dt_inst;	// ??? 7/25/2001 jhrg

////////////////////////////////////////////////////////////////////////////////
/// \fn depth_first(hid_t pid, char *gname, DDS & dds, const char *fname)
/// will fill DDS table.
///
/// This function will walk through hdf5 group and using depth-first approach to
/// obtain data information(data type and data pattern) of all hdf5 dataset and
/// put it into dds table.
///
/// \param pid group id
/// \param gname group name(absolute name from root group).
/// \param dds reference of DDS object.
/// \param fname file name.
///
/// \return 0, if failed.
/// \return 1, if succeeded.
///
/// \remarks hard link is treated as a dataset at hdf5. 
/// \remarks will return error message to the DAP interface.
/// \see depth_first(hid_t pid, char *gname, DAS & das, const char *fname)
////////////////////////////////////////////////////////////////////////////////
bool
depth_first(hid_t pid, char *gname, DDS & dds, const char *fname)
{
  // Iterate through the file to see members of the root group 
  DBG(cerr << ">depth_first()" << endl);

  int nelems; 
  if(H5Gget_num_objs(pid,(hsize_t *)&nelems)<0) {
    string msg =
      "h5_das handler: counting hdf5 group elements error for ";
    msg += gname;
    throw InternalErr(__FILE__, __LINE__, msg);
  }

  for (int i = 0; i < nelems; i++) {
    char *oname = NULL;
    int type = -1;


    ssize_t oname_size = 0;
    // Query the length
    oname_size= H5Gget_objname_by_idx(pid,(hsize_t)i,NULL, (size_t)DODS_NAMELEN);

    if(oname_size <=0) {
      string msg =
        "h5_dds handler: getting the size of hdf5 object name error from";
      msg += gname;
      throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Obtain the name of the object 
    oname = new char[(size_t)oname_size+1];
    if(H5Gget_objname_by_idx(pid,(hsize_t)i,oname,(size_t)(oname_size+1))<0){
      string msg =
        "h5_dds handler: getting the hdf5 object name error from";
      msg += gname;
      delete []oname;
      throw InternalErr(__FILE__, __LINE__, msg);
    }

    type = H5Gget_objtype_by_idx(pid,(hsize_t)i);
    if(type <0) {
      string msg =
        "h5_dds handler: getting the hdf5 object type error from";
      msg += gname;
      delete []oname;
      throw InternalErr(__FILE__, __LINE__, msg);
    }

    switch (type) { // Can we use virtual function? <hyokyung 2007.02.20. 10:17:24>

    case H5G_GROUP:{
      string full_path_name =
	string(gname) + string(oname) + "/";

      char *t_fpn = new char[full_path_name.length() + 1];

      strcpy(t_fpn, full_path_name.c_str());
      hid_t cgroup = H5Gopen(pid, t_fpn);
      try {
	depth_first(cgroup, t_fpn, dds, fname);
      }
      catch(Error & e) {

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
      // Can this be string? <hyokyung 2007.02.20. 10:18:16>,
      // probably not, that's the whole point of this operation
      char *t_fpn = new char[full_path_name.length() + 1]; 

      strcpy(t_fpn, full_path_name.c_str());
 
#if 0 
      hid_t dgroup = H5Gopen(pid, gname); // Can this be string? <hyokyung 2007.02.20. 10:17:55>

      if (dgroup < 0) {
	string msg = "h5_dds handler: cannot open hdf5 group";

	msg += gname;
	throw InternalErr(__FILE__, __LINE__, msg);
      }
#endif

      // obtain hdf5 dataset handle. 
      //      if ((get_dataset(dgroup, t_fpn, &dt_inst, Msgt)) < 0) {
      if ((get_dataset(pid, t_fpn, &dt_inst, Msgt)) < 0) {
	string msg =
	  "h5_dds handler: get hdf5 dataset wrong for ";
	msg += t_fpn;
	msg += string("\n") + string(Msgt);
	delete[]t_fpn;
	throw InternalErr(__FILE__, __LINE__, msg);
      }
      // Put hdf5 dataset structure into DODS dds.
      // read_objects throws InternalErr.
      try {
	read_objects(dds, t_fpn, fname); // Is t_fpn a must? <hyokyung 2007.02.20. 10:22:20>
      }
      catch(Error & e) {
	delete[]t_fpn;
	throw;
      }

      delete[]t_fpn;
      break;
    }
    case H5G_TYPE:
      break;

    default:
      break;
    }

    type = -1;
#ifndef KENT_OLD_WAY
    delete[]oname;
#endif

  }
  DBG(cerr << "<depth_first()" << endl);
  return true;
}


////////////////////////////////////////////////////////////////////////////////
/// \fn	return_type(hid_t type)
/// returns the string representation of HDF5 type.
///
/// This function will get the text representation(string) of the corresponding
/// DODS datatype. DODS-HDF5 subclass method will use this function.
///
/// \return string
/// \param type datatype id
/// \todo Check with DODS datatype: BYTE - 8bit INT16 - 16 bits
//        <hyokyung 2007.02.20. 10:23:02>
////////////////////////////////////////////////////////////////////////////////
string
return_type(hid_t type)
{
  size_t size = 0;
  H5T_sign_t sign;
  
  switch (H5Tget_class(type)) {

  case H5T_INTEGER:
    //  <hyokyung 2007.02.27. 13:29:14>
    size = H5Tget_size(type);
    sign = H5Tget_sign(type);
    DBG(cerr << "=return_type(): INT sign = " << sign << " size = " << size << endl);
    if (size == 1)
      return BYTE;

    if (size == 2){
      if(sign == H5T_SGN_NONE)
	return UINT16;
      else
	return INT16;
    }
    
    if (size == 4){
      if(sign == H5T_SGN_NONE)
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
    
    // HDF5 Compound type maps to DODS Structure. <hyokyung 2007.03. 1. 12:30:03>
  case H5T_COMPOUND:
    return COMPOUND;

  case H5T_ARRAY:
    // cerr << "comes array" << endl;
    return ARRAY;    
    // Hmm. Is this really the correct thing to do? 7/25/2001 jhrg
    // I'm not sure what James meant. <hyokyung 2007.02.20. 10:23:39>
  default:
    return "Unmappable Type";
  }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn Get_bt(string varname, hid_t datatype, HDF5TypeFactory &factory)
/// returns the pointer of base type
///
/// This function will create a new DODS object that corresponds with HDF5
/// dataset and return pointer of a new object of DODS datatype.
///
/// \param varname object name
/// \param dataype datatype id
/// \param factory DODS object class generator
/// \return pointer to BaseType
////////////////////////////////////////////////////////////////////////////////
static BaseType *
Get_bt(string varname, hid_t datatype, HDF5TypeFactory &factory)
{

  BaseType *temp_bt = NULL;

  size_t size = 0;
  int sign = -2;  

  
  DBG(cerr << ">Get_bt varname=" << varname << " datatype=" << datatype << endl);
  
  switch (H5Tget_class(datatype)) {

  case H5T_INTEGER:
    size = H5Tget_size(datatype);
    sign = H5Tget_sign(datatype);
    DBG(cerr << "=Get_bt() H5T_INTEGER size = " << size << " sign = " << sign << endl);
    
    if (size == 1) {
      temp_bt = factory.NewByte(varname);
    }
    
    if (size == 2){
      if(sign == H5T_SGN_2)
	temp_bt = factory.NewInt16(varname);
      else
	temp_bt = factory.NewUInt16(varname);
    }
    
    if (size == 4){
      if(sign == H5T_SGN_2)
	temp_bt = factory.NewInt32(varname);
      else
	temp_bt = factory.NewUInt32(varname);
    }
    break;

  case H5T_FLOAT:
    size = H5Tget_size(datatype);
    DBG(cerr << "=Get_bt() H5T_FLOAT size = " << size << endl);
	
    if (size == 4) {
      temp_bt = factory.NewFloat32(varname);
    }
    if(size == 8) {
      temp_bt = factory.NewFloat64(varname);
    } 
    break;

  case H5T_STRING:
    temp_bt = factory.NewStr(varname);
    break;
    
  case H5T_ARRAY:
    DBG(cerr << "=Get_bt() H5T_ARRAY datatype = " << datatype << endl);
    hsize_t size2[DODS_MAX_RANK];
    
    int perm[DODS_MAX_RANK];
    
    // Get the array's base datatype
    hid_t dtype_base=H5Tget_super(datatype);
    BaseType* ar_bt = Get_bt(varname, dtype_base, factory);
    
    
    // Set the size of the array.
    int ndim = H5Tget_array_ndims(datatype);

    int size = H5Tget_size(datatype);

    int nelement;

    nelement = 1;
    
    DBG(cerr << "=Get_bt()" << " Dim = " << ndim << " Size = " << size << endl);
    
    H5Tget_array_dims(datatype, size2, perm);
    
    // temp_bt = factory.NewArray(varname, 0);
    temp_bt = factory.NewArray(varname);
    temp_bt->add_var(ar_bt);
    
    for (int dim_index=0; dim_index < ndim;dim_index++) {
      (dynamic_cast < HDF5Array * >(temp_bt))->append_dim(size2[dim_index]);
      DBG(cerr << "=Get_bt() " << size2[dim_index] << endl);
      nelement = nelement * size2[dim_index];
    }
    
    (dynamic_cast < HDF5Array * >(temp_bt))->set_did(dt_inst.dset);
    // Assign the array datatype id.
    (dynamic_cast < HDF5Array * >(temp_bt))->set_tid(datatype);
    (dynamic_cast < HDF5Array * >(temp_bt))->set_memneed(size);
    (dynamic_cast < HDF5Array * >(temp_bt))->set_numdim(ndim);
    (dynamic_cast < HDF5Array * >(temp_bt))->set_numelm(nelement);
    (dynamic_cast < HDF5Array * >(temp_bt))->set_length(nelement);
    (dynamic_cast < HDF5Array * >(temp_bt))->d_type = H5Tget_class(dtype_base);
    delete ar_bt;
    
    break;
    
  default:
    break;
  }

  if (!temp_bt){
    return NULL;
  }


  switch (temp_bt->type()) {

  case dods_byte_c:

    (dynamic_cast < HDF5Byte * >(temp_bt))->set_did(dt_inst.dset);
    // dt_inst is a global struct! This is not right. <hyokyung 2007.02.20. 10:36:05>
    (dynamic_cast < HDF5Byte * >(temp_bt))->set_tid(dt_inst.type);
    break;

  case dods_int16_c:

    (dynamic_cast < HDF5Int16 * >(temp_bt))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5Int16 * >(temp_bt))->set_tid(dt_inst.type);
    break;

  case dods_uint16_c:

    (dynamic_cast < HDF5UInt16 * >(temp_bt))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5UInt16 * >(temp_bt))->set_tid(dt_inst.type);
    break;

  case dods_int32_c:

    (dynamic_cast < HDF5Int32 * >(temp_bt))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5Int32 * >(temp_bt))->set_tid(dt_inst.type);
    break;

  case dods_uint32_c:

    (dynamic_cast < HDF5UInt32 * >(temp_bt))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5UInt32 * >(temp_bt))->set_tid(dt_inst.type);
    break;

  case dods_float32_c:

    (dynamic_cast < HDF5Float32 * >(temp_bt))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5Float32 * >(temp_bt))->set_tid(dt_inst.type);
    break;

  case dods_float64_c:
    (dynamic_cast < HDF5Float64 * >(temp_bt))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5Float64 * >(temp_bt))->set_tid(dt_inst.type);
    break;

  case dods_str_c:
    (dynamic_cast < HDF5Str * >(temp_bt))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5Str * >(temp_bt))->set_tid(dt_inst.type);
    (dynamic_cast < HDF5Str * >(temp_bt))->set_arrayflag(STR_NOFLAG);
    break;
    
  case dods_array_c:
    break;
    
#if 0
  case dods_url_c:
    (dynamic_cast < HDF5Url * >(temp_bt))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5Url * >(temp_bt))->set_tid(dt_inst.type);

  case dods_list_c:
  case dods_structure_c:
  case dods_sequence_c:
  case dods_grid_c:

#endif

  default:
    string msg =
      "h5_dds handler: counting hdf5 group elements error for ";
    msg += varname;
    throw InternalErr(__FILE__, __LINE__, msg);
    return NULL;
  }
  DBG(cerr << "<Get_bt()" << endl);		  
  return temp_bt;
}



// Function:	Get_structure
//
// Purpose:	this function will create a new DODS object that corresponds 
//              with  HDF5 compound dataset and return pointer of a new structure
//              object of DODS
//
// Errors:
// Return:	pointer of structure type. 
// In :	        datatype id, object name
//
// <hyokyung 2007.03. 2. 13:22:20>  
static Structure *
Get_structure(string varname, hid_t datatype, HDF5TypeFactory &factory)
{

  Structure *temp_structure = NULL;
  
  DBG(cerr << ">Get_structure()" << datatype <<  endl);
  if(H5Tget_class(datatype) == H5T_COMPOUND) {
    
    temp_structure = factory.NewStructure(varname);
    (dynamic_cast <HDF5Structure *>(temp_structure))->set_did(dt_inst.dset);
    (dynamic_cast <HDF5Structure *>(temp_structure))->set_tid(dt_inst.type);
    // Retrieve member types
    int nmembs = H5Tget_nmembers(datatype);
    DBG(cerr << "=Get_structure() has " << nmembs <<  endl);
    for (int i=0;i<nmembs;i++)
      {
	// Copied from depth_first().
	char *memb_name = NULL;
	hid_t memb_type;      // Compound member datatype
	H5T_class_t memb_cls;
	size_t memb_offset;
	
	memb_name = H5Tget_member_name(datatype,i);
	memb_offset = H5Tget_member_offset(datatype,i);
	DBG(cerr << "=Get_structure() name = " << memb_name << " offset = " << memb_offset << endl);
	// Get member type class 
	if((memb_cls = H5Tget_member_class (datatype, i))<0) {
	  string msg =
	    "h5_dds handler: hdf5 compound member type to DODS type mapping error for ";
	  msg += i;
	  throw InternalErr(__FILE__, __LINE__, msg);
	}
      
	// Get member type ID 
	if((memb_type = H5Tget_member_type(datatype, i))<0) {
	  string msg =
	    "h5_dds handler: hdf5 compound member type to DODS type mapping error for ";
	  msg += i;
	  throw InternalErr(__FILE__, __LINE__, msg);
	}
	
	if(memb_cls == H5T_COMPOUND){
	  temp_structure->add_var(Get_structure(memb_name, memb_type, factory));
	}
	else{
	  BaseType *bt = Get_bt(memb_name, memb_type, factory);
	  temp_structure->add_var(bt);
	}
	

	// Don't close it. Closing can give Unmappable type later in DODS.
	
	/*
	// Close member type ID	  
	if(H5Tclose(memb_type)<0) {
	  string msg =
	    "h5_dds handler: hdf5 compound member type close() function error.";
	  throw InternalErr(__FILE__, __LINE__, msg);
	}
        */
      }
     
  }
  else{
    string msg =
      "h5_dds handler: hdf5 compound to DODS structure mapping error for ";
    msg += varname;
    throw InternalErr(__FILE__, __LINE__, msg);
	
  }
  DBG(cerr << "<Get_structure()" << endl);		    
  return temp_structure;  
}

////////////////////////////////////////////////////////////////////////////////
/// \fn read_objects_base_type(DDS & dds_table,
///                            const string & varname,
///		               const string & filename)
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
read_objects_base_type(DDS & dds_table, const string & varname,
		       const string & filename)
{
  Array *ar;
  Grid *gr;
  Part pr; // enum type (see BaseType.h line 100)
  

  
  dds_table.set_dataset_name(name_path(filename));


  // Get base type. It should be int, float and double etc. atomic datatype.   
  BaseType *bt = Get_bt(varname, dt_inst.type,
			dynamic_cast<HDF5TypeFactory&>(*dds_table.get_factory()));
  if (!bt) {
    // NB: We're throwing InternalErr even though it's possible that
    // someone might ask for an HDF5 varaible which this server cannot
    // handle (for example, an HDF5 reference).
    throw
      InternalErr(__FILE__, __LINE__,
		  "Unable to convert hdf5 datatype to dods basetype");
  }
  
  // First deal with scalar data. 
  if (dt_inst.ndims == 0) {
    dds_table.add_var(bt);
  }
  // Next, deal with array and grid data. 
  else {
    int dim_index;
    ar = dds_table.get_factory()->NewArray(varname);

    (dynamic_cast < HDF5Array * >(ar))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5Array * >(ar))->set_tid(dt_inst.type);
    (dynamic_cast < HDF5Array * >(ar))->set_memneed(dt_inst.need);
    (dynamic_cast < HDF5Array * >(ar))->set_numdim(dt_inst.ndims);
    (dynamic_cast < HDF5Array * >(ar))->set_numelm((int) (dt_inst.nelmts));
    ar->add_var(bt);

    for (dim_index=0; dim_index < dt_inst.ndims;dim_index++) 
      ar->append_dim(dt_inst.size[dim_index]);
    delete bt;

    // This needs to be fully supported! <hyokyung 2007.02.20. 11:53:11>

#ifdef DODSGRID

    H5GridFlag_t check_grid;


    /* 1. Check whether this HDF5 dataset can be mapped to the grid data
       Should check whether the attribute includes dimension list;
       If yes and everything is valid, map to DAP grid;
       Otherwise, map to DAP array. */
 
    check_grid = maptogrid(dt_inst.dset,dt_inst.ndims);
    
    if (check_grid != NotGrid) {
      hid_t  attr_id;
      hid_t temp_dtype;
      hid_t temp_dspace;
      hid_t  memtype;
      size_t temp_tsize;
      size_t name_size;
      hsize_t temp_nelm;
      char * dimname;
      char *EachDimName;
      hid_t *dimid;
      hobj_ref_t *refbuf;
      char *buf;

           
      gr = dds_table.get_factory()->NewGrid(varname);/* using varname rather than new_name */
      // First fill the array part of the grid.
      pr = array;
      gr->add_var(ar,pr);
      delete ar;
 
      DBG(cerr << "add_var()" << endl);

      /* obtain dimensional scale name, it should be a list of dimensional name,
         Here we will distinguish old h4h5 tool or new h4h5 tool */
      if(check_grid == NewH4H5Grid) {
        if((attr_id=H5Aopen_name(dt_inst.dset,HDF5_DIMENSIONNAMELIST))<0){
          throw 
	    InternalErr(__FILE__,__LINE__,
			"Unable to open the attribute with the name as DIMENSION_NAMELIST");
        }
      }
      else {
	if((attr_id=H5Aopen_name(dt_inst.dset,OLD_HDF5_DIMENSIONNAMELIST))<0){
          throw
	    InternalErr(__FILE__,__LINE__,
			"Unable to open the attribute with the name as OLD_DIMENSION_NAMELIST");
        }
      }

      temp_dtype = H5Aget_type(attr_id);
      temp_tsize = H5Tget_size(temp_dtype);

      temp_dspace = H5Aget_space(attr_id);
      temp_nelm   = H5Sget_simple_extent_npoints(temp_dspace);
      dimname     = (char*)calloc((size_t) temp_nelm, temp_tsize);
      name_size   = temp_tsize;
      if(H5Aread(attr_id, temp_dtype, dimname)<0) {
	throw 
	  InternalErr(__FILE__,__LINE__,
		      "Unable to obtain the attribute");
      }
      H5Tclose(temp_dtype);
      H5Sclose(temp_dspace);
      H5Aclose(attr_id);

      //obtain dimensional scale data information 
      if(check_grid == NewH4H5Grid) {
        if((attr_id=H5Aopen_name(dt_inst.dset,HDF5_DIMENSIONLIST))<0){
          throw 
	    InternalErr(__FILE__,__LINE__,
			"Unable to open the attribute with the name as DIMENSION_LIST");
        }
      }
      else {
	if((attr_id=H5Aopen_name(dt_inst.dset,OLD_HDF5_DIMENSIONLIST))<0){
          throw
	    InternalErr(__FILE__,__LINE__,
			"Unable to open the attribute with the name as OLD_DIMENSION_LIST");
        }
      }

      temp_dtype  = H5Aget_type(attr_id);
      temp_tsize  = H5Tget_size(temp_dtype);
      temp_dspace = H5Aget_space(attr_id);
      temp_nelm   = H5Sget_simple_extent_npoints(temp_dspace);
      buf = (char*)calloc((size_t)(temp_nelm*temp_tsize), sizeof(char));
      if(H5Aread(attr_id, H5T_STD_REF_OBJ, buf)<0) {
	throw 
	  InternalErr(__FILE__,__LINE__,
		      "Cannot read object reference attributes.");
      }
      refbuf = (hobj_ref_t *) buf;
      dimid  = (hid_t *)malloc(sizeof(hid_t)*temp_nelm);

      for (int j = 0; j < temp_nelm; j++) {
	dimid[j] = H5Rdereference(attr_id, H5R_OBJECT, refbuf);
	if (dimid[j] < 0) {
	  throw 
            InternalErr(__FILE__,__LINE__,
			"cannot dereference the object.");
	}
	refbuf++;
      }
      H5Aclose(attr_id);
      H5Sclose(temp_dspace);
      H5Tclose(temp_dtype);
      
 
      // Start building Grid.
      char *TempNamePointer = dimname;
      EachDimName       = (char*)malloc(name_size+sizeof(char));
      pr = maps;

      for (dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
	// Get dimensional scale datasets. Add them to grid.
        temp_dspace = H5Dget_space(dimid[dim_index]);
        temp_nelm   = H5Sget_simple_extent_npoints(temp_dspace);
        temp_dtype  = H5Dget_type(dimid[dim_index]);
        memtype     = H5Tget_native_type(temp_dtype,H5T_DIR_ASCEND);
        temp_tsize  = H5Tget_size(memtype);
        strcpy(EachDimName,TempNamePointer);
        TempNamePointer=TempNamePointer+name_size;

	try{
	  bt = Get_bt(EachDimName, memtype,
		      dynamic_cast<HDF5TypeFactory&>(*dds_table.get_factory()));
	}
	catch(Error& e){
	  throw;
	}

        ar = new HDF5Array;
	ar = dds_table.get_factory()->NewArray(EachDimName);
	(dynamic_cast < HDF5Array * >(ar))->set_did(dimid[dim_index]);
	(dynamic_cast < HDF5Array * >(ar))->set_tid(memtype);
	(dynamic_cast < HDF5Array * >(ar))->set_memneed(temp_tsize*temp_nelm);
	(dynamic_cast < HDF5Array * >(ar))->set_numdim(1);
	(dynamic_cast < HDF5Array * >(ar))->set_numelm(temp_nelm);
	ar->add_var(bt);
	ar->append_dim(temp_nelm, EachDimName);
	gr->add_var(ar, pr);
        delete ar;
      }
      
      dds_table.add_var(gr);
      delete gr;
      if(dimname!=NULL) free(dimname);
      if(EachDimName!=NULL) free(EachDimName);
      if(dimid!=NULL) free(dimid);
    }
    
#ifdef NASA_EOS_GRID
    // Check if eos class has this name as grid.
    else if(eos.is_valid() && eos.is_grid(varname)){
      DBG(cerr << "EOS Grid: " << varname << endl);
      // Generate grid based on the parsed StructMetada.
      gr = (dynamic_cast < HDF5TypeFactory * >(dds_table.get_factory()))->NewGridEOS(varname);
      // First fill the array part of the grid.
      pr = array;
      gr->add_var(ar,pr);
      delete ar;
      
      // Next fill the map part of the grid.
      pr = maps;
      // Retrieve the dimension lists from parsed metadata.
      vector<string> tokens;
      eos.get_dimensions(varname, tokens);
      DBG(cerr << "Number of dimensions " << dt_inst.ndims << endl);

      
      for (dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {      
	DBG(cerr << ": " << tokens.at(dim_index) << endl);
	
	string str_dim_name = tokens.at(dim_index);
	
	// Retriev the full path to the each dimension name.
	string str_grid_name = eos.get_grid_name(varname);
	string str_dim_full_name = str_grid_name + str_dim_name;
	
	int dim_size = eos.get_dimension_size(str_dim_full_name);

	bt = dds_table.get_factory()->NewFloat32(str_dim_full_name);
	ar = new Array(str_dim_full_name, 0);
	ar->add_var(bt);            
	ar->append_dim(dim_size, str_dim_full_name);
	gr->add_var(ar, pr);
	delete ar;
      }
      
      dds_table.add_var(gr);      
      delete gr;
      
    }
#endif    
    else {// cannot be mapped to grid, it has to be an array.
      dds_table.add_var(ar);
      delete ar;
    }
#else
    // Not define DODS Grid. It has to be an array. 
    dds_table.add_var(ar);
    delete ar;
#endif
  }
  DBG(cerr << "<read_objects_base_type(dds)" << endl);  
}

///
/// @see LoadStructreFrom... in hdf4_handler/hc2dap.cc 
void
read_objects_structure(DDS & dds_table, const string & varname,
		       const string & filename)
{
  Array *ar;
  Structure *structure = NULL;
  
  char *newname = NULL;
  char *temp_varname = new char[varname.length() + 1];

  varname.copy(temp_varname, string::npos);
  temp_varname[varname.length()] = 0;
  newname = strrchr(temp_varname, '/');
  newname++;
  
  dds_table.set_dataset_name(name_path(filename));
  
  structure = Get_structure(varname, dt_inst.type,
			    dynamic_cast<HDF5TypeFactory&>(*dds_table.get_factory()));
  if (!structure) {
    delete[]temp_varname;    
    throw
      InternalErr(__FILE__, __LINE__,
		  "Unable to convert hdf5 compound datatype to dods structure");
  }
  DBG(cerr << "=read_objects_structure(): Dimension is " << dt_inst.ndims << endl);

  if (dt_inst.ndims != 0) {   // Array of Structure
    int dim_index;
    DBG(cerr << "=read_objects_structure(): Creating array of size " <<  dt_inst.nelmts << endl);
    DBG(cerr << "=read_objects_structure(): memory needed = " << dt_inst.need << endl);
    ar = dds_table.get_factory()->NewArray(temp_varname, 0);
    (dynamic_cast < HDF5Array * >(ar))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5Array * >(ar))->set_tid(dt_inst.type);
    (dynamic_cast < HDF5Array * >(ar))->set_memneed(dt_inst.need);
    (dynamic_cast < HDF5Array * >(ar))->set_numdim(dt_inst.ndims);
    (dynamic_cast < HDF5Array * >(ar))->set_numelm((int) (dt_inst.nelmts));
    (dynamic_cast < HDF5Array * >(ar))->set_length((int) (dt_inst.nelmts));
    ar->add_var(structure);
    for (dim_index=0; dim_index < dt_inst.ndims;dim_index++) {
      ar->append_dim(dt_inst.size[dim_index]);
      DBG(cerr << "=read_objects_structure(): append_dim = " << dt_inst.size[dim_index] << endl);
    }
    delete structure;
    dds_table.add_var(ar);
    delete ar;
  }
  else{
    dds_table.add_var(structure);
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
read_objects(DDS & dds_table, const string & varname,
	     const string & filename)
{
  switch(H5Tget_class(dt_inst.type)){
  case H5T_COMPOUND:
    read_objects_structure(dds_table, varname, filename);
    break;

  default:
    read_objects_base_type(dds_table, varname, filename);
    break;
  }
}

