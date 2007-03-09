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
      // Can this be string? <hyokyung 2007.02.20. 10:18:16>, probably not, that's the whole point of this operation
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
    DBG(cerr <<"=return_type(): COMPOUND " << endl);
    return COMPOUND;

    // Hmm. Is this really the correct thing to do? 7/25/2001 jhrg
    // Not sure? <hyokyung 2007.02.20. 10:23:39>
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

  
  DBG(cerr << ">Get_bt(" << varname << ")" << endl);
  
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
    

  default:
    break;
  }

  if (!temp_bt)
    return NULL;


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

    
#if 0
  case dods_array_c:
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
  
  DBG(cerr << ">Get_structure()" << endl);
  if(H5Tget_class(datatype) == H5T_COMPOUND) {
    
    temp_structure = factory.NewStructure(varname);
    (dynamic_cast <HDF5Structure *>(temp_structure))->set_did(dt_inst.dset);
    (dynamic_cast <HDF5Structure *>(temp_structure))->set_tid(dt_inst.type);
    
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
/// to a hdf5 file, read hdf5 file and extract all the dimensions of
/// each of its variables. Add the variables and their dimensions to the
/// instance of DDS.
///
/// It will use dynamic cast toput necessary information into subclass of dods
/// datatype. 
///
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of either a dataset or a group
///    \throw error a string of error message to the dods interface.
///    \param filename Added to the DDS (dds_table).
////////////////////////////////////////////////////////////////////////////////
void
read_objects_base_type(DDS & dds_table, const string & varname,
		       const string & filename)
{
  Array *ar;
  Grid *gr;
  Part pr;
  
  hid_t dimtype;
  hid_t dimid;

  int dim_index;
  int num_dimelm;
  int num_dim;

  char *dimname = NULL;
  char *newdimname = NULL; // <hyokyung 2007.02.27. 10:25:05>
  char *newname = NULL;
  char *temp_varname = new char[varname.length() + 1];
  
  char dimlab[DODS_MAX_RANK][6];
  char ORI_SLASH = '/';

  size_t dimsize;
    
  DBG(cerr << ">read_objects_base_type(dds)" << endl);
  varname.copy(temp_varname, string::npos);
  temp_varname[varname.length()] = 0;
  //The following code will change "/" into "_"
  // "/" is reserved in DODS <hyokyung 2007.02.20. 11:52:39>
#if 0
  while (strchr(temp_varname, ORI_SLASH) != NULL) {
    cptr = strchr(temp_varname, ORI_SLASH);
    *cptr = CHA_SLASH;
  }
#endif
  
  // This needs an attention <hyokyung 2007.02.20. 11:52:51>
  newname = strrchr(temp_varname, ORI_SLASH);
  newname++;

#if 0
  // make/test this mod sometime in the future.
  string::size_type pos = varname.rfind(ORI_SLASH);
  string newname = varname.substr(++pos, varname.end());
#endif

  dds_table.set_dataset_name(name_path(filename));


    
  BaseType *bt = Get_bt(newname, dt_inst.type,
			dynamic_cast<HDF5TypeFactory&>(*dds_table.get_factory()));
  if (!bt) {
    delete[]temp_varname;
    // NB: We're throwing InternalErr even though it's possible that
    // someone might ask for a HDF5 varaible which this server cannot
    // handle (for example, an HDF5 reference).
    throw
      InternalErr(__FILE__, __LINE__,
		  "Unable to convert hdf5 datatype to dods basetype");
  }
  
  // First deal with scalar data. 
  if (dt_inst.ndims == 0) {
    dds_table.add_var(bt);
  }
  // Next, deal with array data. 
  else {

    ar = dds_table.get_factory()->NewArray(temp_varname);

    (dynamic_cast < HDF5Array * >(ar))->set_did(dt_inst.dset);
    (dynamic_cast < HDF5Array * >(ar))->set_tid(dt_inst.type);
    (dynamic_cast < HDF5Array * >(ar))->set_memneed(dt_inst.need);
    (dynamic_cast < HDF5Array * >(ar))->set_numdim(dt_inst.ndims);
    (dynamic_cast < HDF5Array * >(ar))->set_numelm((int) (dt_inst.nelmts));
    ar->add_var(bt);

    // This needs to be fully supported! <hyokyung 2007.02.20. 11:53:11>

#ifdef DODSGRID

    // Check whether the dimension number matches.
    
    num_dim = get_dimnum(dt_inst.dset);

    if (num_dim == dt_inst.ndims) {
      // start building up the grid.
      //  gr = NewGrid(temp_varname);
      gr = dds_table.get_factory()->NewGrid(newname);
      pr = array;
      for (dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
	dimname = get_dimname(dt_inst.dset, dim_index);
	newdimname = correct_name(dimname);
	/*
	  1) obtain dimensional id, 
	  2) number of element in this dim.,
	  3) total dimensional size of this dimensional scale data
	  4) the HDF5 dimensional data type
	*/

	dimid =
	  get_diminfo(dt_inst.dset, dim_index, &num_dimelm,
		      &dimsize, &dimtype);

	// printf("num_dimelm at arrays%d\n",num_dimelm);
	ar->append_dim(num_dimelm, newdimname);
	free(dimname);
	if(newdimname != NULL)
	  free(newdimname);
      }
      DBG(cerr << "add_var()" << endl);
      gr->add_var(ar, pr);
      pr = maps;
      for (dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
	//get dimensional scale datasets. add to grid.
	dimname = get_dimname(dt_inst.dset, dim_index);
	newdimname = correct_name(dimname);
	dimid =
	  get_diminfo(dt_inst.dset, dim_index, &num_dimelm,
		      &dimsize, &dimtype);
	try{
	  bt = Get_bt(newdimname, dimtype,
		      dynamic_cast<HDF5TypeFactory&>(*dds_table.get_factory()));
	}
	catch(Error& e){
	  throw;
	}

	ar = dds_table.get_factory()->NewArray(newdimname);
	(dynamic_cast < HDF5Array * >(ar))->set_did(dimid);
	(dynamic_cast < HDF5Array * >(ar))->set_tid(dimtype);
	(dynamic_cast < HDF5Array * >(ar))->set_memneed(dimsize);
	(dynamic_cast < HDF5Array * >(ar))->set_numdim(1);
	(dynamic_cast < HDF5Array * >(ar))->set_numelm(num_dimelm);
	ar->add_var(bt);
	//  ar->append_dim(dimsize,dimname);//add this later
	//    cout <<"dimname at maps"<<newdimname<<endl;
	//   cout <<"num_dimelm at maps"<<num_dimelm<<endl;
	//  cout <<"dimid at maps" <<dimid<<endl;
	ar->append_dim(num_dimelm, newdimname);
	gr->add_var(ar, pr);
	free(dimname);
	free(newdimname);
      }
      dds_table.add_var(gr);
    } else {
      // Needs to have good comments; Kent has already forgotten this.
      // <hyokyung 2007.02.20. 11:53:56>
      for (int d = 0; d < dt_inst.ndims; ++d) {
	dimlab[d][0] = 'd';
	dimlab[d][1] = 'i';
	dimlab[d][2] = 'm';
	if (d > 9) {
	  dimlab[d][3] = (char) (d / 10 + 48);
	  dimlab[d][4] = (char) (d % 10 + 49);
	  dimlab[d][5] = '\0';
	} else {
	  dimlab[d][3] = (char) (d + 49);
	  dimlab[d][4] = '\0';
	}
	ar->append_dim(dt_inst.size[d], dimlab[d]);
      }
      dds_table.add_var(ar);
    }

#else
    for (int d = 0; d < dt_inst.ndims; ++d) {
      dimlab[d][0] = 'd';
      dimlab[d][1] = 'i';
      dimlab[d][2] = 'm';
      if (d > 9) {
	dimlab[d][3] = (char) (d / 10 + 48);
	dimlab[d][4] = (char) (d % 10 + 49);
	dimlab[d][5] = '\0';
      } else {
	dimlab[d][3] = (char) (d + 49);
	dimlab[d][4] = '\0';
      }
      ar->append_dim(dt_inst.size[d], dimlab[d]);
    }
    dds_table.add_var(ar);
#endif
  }
  delete[]temp_varname;
  DBG(cerr << "<read_objects_base_type(dds)" << endl);  
}

///
/// @see LoadStructreFrom... in hdf4_handler/hc2dap.cc 
void
read_objects_structure(DDS & dds_table, const string & varname,
		       const string & filename)
{
  Structure *structure = NULL;
  
  char *newname = NULL;
  char *temp_varname = new char[varname.length() + 1];

  
  varname.copy(temp_varname, string::npos);
  temp_varname[varname.length()] = 0;
  newname = strrchr(temp_varname, '/');
  newname++;
  
  dds_table.set_dataset_name(name_path(filename));

  
  structure = Get_structure(newname, dt_inst.type,
  			dynamic_cast<HDF5TypeFactory&>(*dds_table.get_factory()));
  if (!structure) {
    delete[]temp_varname;
    throw
      InternalErr(__FILE__, __LINE__,
		  "Unable to convert hdf5 compound datatype to dods structure");
  }
  else{
    dds_table.add_var(structure);
  }

}
  
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

