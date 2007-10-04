////////////////////////////////////////////////////////////////////////////////
/// \file h5das.cc
/// \brief Data attributes processing source
///
/// This file is part of h5_dap_handler, a C++ implementation of the DAP handler
/// for HDF5 data.
///
/// This is the HDF5-DAS that extracts DAS class descriptors converted from
///  HDF5 attribute of an hdf5 data file.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang <ymuqun@hdfgroup.org>
///
/// Copyright (c) 2007 HDF Group
///
/// Copyright (C) 1999 National Center for Supercomputing Applications.
///
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////
// #define DODS_DEBUG
#include "debug.h"
#include "h5das.h"
#include "InternalErr.h"
#include "common.h"
#include "H5Git.h"
#include "parser.h"
#include "Str.h"
#include "H5EOS.h"
#include "H5PathFinder.h"


H5EOS eos;
H5PathFinder paths;

static char Msgt[255];		// used as scratch in various places
static int slinkindex;		// used by depth_first()

// This vesion should really be 1.0, to match the version of the handler,
// but that will break all sorts of client code. jhrg 1/31/06
static const char STRING[]="String";
static const char BYTE[]="Byte";
static const char INT32[]="Int32";
static const char INT16[]="Int16";
static const char FLOAT64[]="Float64";
static const char FLOAT32[]="Float32";
static const char UINT16[]="UInt16";
static const char UINT32[]="UInt32";
static const char INT_ELSE[]="Int_else";
static const char FLOAT_ELSE[]="Float_else";

/// EOS parser related variables
struct yy_buffer_state;
int hdfeos_dasparse(void *arg);      // defined in hdfeos.tab.cc
yy_buffer_state *hdfeos_das_scan_string(const char *str);



////////////////////////////////////////////////////////////////////////////////
/// \fn depth_first(hid_t pid, char *gname, DAS & das, const char *fname)
/// depth first traversal of hdf5 file attributes.
///
/// This function will walk through hdf5 group and using depth-
/// first approach to obtain all the group and dataset attributes 
/// of a hdf5 file.
/// During the process of depth first search, DAS table will be filled.
/// In case of errors, it will return error message to the DODS interface.
///
/// \param pid    dataset id(group id)
/// \param gname  group name(absolute name from root group).
/// \param das    reference of DAS object.
/// \param fname  filename
/// \return true  if succeeded
/// \return false if failed
///
/// \todo This is like the same code of DDS! => Can we combine the two into one?
///   How about using virtual function?
/// \todo gname is kind of redundant with pid, needs to be removed.
/// \todo why need fname?? KY 02/27/2007
////////////////////////////////////////////////////////////////////////////////
bool
depth_first(hid_t pid, char *gname, DAS & das, const char *fname)
{
  
  int num_attr = -1;
  int nelems;   // Iterate through the file to see members of the root group.


  DBG(cerr << ">depth_first():" << gname << endl);
  
  read_comments(das, gname, pid);
  
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
        "h5_das handler: getting the size of hdf5 object name error from";
      msg += gname;
      throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Obtain the name of the object 
    oname = new char[(size_t)oname_size+1];
    if(H5Gget_objname_by_idx(pid,(hsize_t)i,oname,(size_t)(oname_size+1))<0){
      string msg =
        "h5_das handler: getting the hdf5 object name error from";
      msg += gname;
      delete []oname;
      throw InternalErr(__FILE__, __LINE__, msg);
    }

    type = H5Gget_objtype_by_idx(pid,(hsize_t)i);
    if(type <0) {
      string msg =
        "h5_das handler: getting the hdf5 object type error from";
      msg += gname;
      delete []oname;
      throw InternalErr(__FILE__, __LINE__, msg);
    }
    
    
    switch (type) {

    case H5G_GROUP:{
      DBG(cerr << "=depth_first():H5G_GROUP " << oname << endl);

      add_group_structure_info(das, gname, oname, true);
      
      string full_path_name = string(gname) + string(oname) + "/";
      char *t_fpn = new char[full_path_name.length() + 1];
      strcpy(t_fpn, full_path_name.c_str());
      
      hid_t cgroup = H5Gopen(pid,t_fpn);
      
      if (cgroup < 0) {
	string msg =
	  "h5_das handler: opening hdf5 group failed for ";
	msg += t_fpn;
	msg += string("\n") + string(Msgt);
	delete[]t_fpn;
        delete []oname;
	throw InternalErr(__FILE__, __LINE__, msg);
      }
      
      if ((num_attr = H5Aget_num_attrs(cgroup)) < 0) {
        string msg =
	  "dap_h5_handler:  failed to obtain hdf5 attribute in group  ";
        msg += t_fpn;
        throw InternalErr(__FILE__, __LINE__, msg);
      }

      try {
	int oid = get_hardlink(cgroup, t_fpn);
#ifndef CF	
	read_objects(das, t_fpn, cgroup, num_attr);
#endif
	// Break the cyclic loop created by hard links.
	if(oid == 0){	// <hyokyung 2007.06.11. 13:53:12>
	  depth_first(cgroup, t_fpn, das, fname);
	}
	else{
	  // Add attribute table with HARDLINK
	  AttrTable* at = das.add_table(t_fpn, new AttrTable);
	  at->append_attr("HDF5_HARDLINK", STRING, paths.get_name(oid));
	}
      }
      catch(Error & e) {
	delete[]oname;
	delete[]t_fpn;
	throw;
      }

      delete[]t_fpn;
      H5Gclose(cgroup); // also need error handling.
      break;
    } // case H5G_GROUP

    case H5G_DATASET:{
      DBG(cerr << "=depth_first():H5G_DATASET " << oname << endl);

      add_group_structure_info(das, gname, oname, false);
      
      string full_path_name = string(gname) + string(oname);

      char *t_fpn = new char[full_path_name.length() + 1];

      strcpy(t_fpn, full_path_name.c_str());
      hid_t dset;
      // Open the dataset 
      if((dset = H5Dopen(pid,t_fpn))<0) {
	string msg =
	  "dap_h5_handler:  unable to open hdf5 dataset of group ";
        msg += gname;
        delete[]t_fpn;
        throw InternalErr(__FILE__, __LINE__, msg);
      }

      
      // Obtain number of attributes in this dataset. 
      if ((num_attr = H5Aget_num_attrs(dset)) < 0) {
        string msg =
	  "dap_h5_handler:  failed to obtain hdf5 attribute in dataset  ";
        msg += t_fpn;
        delete[]t_fpn;
        throw InternalErr(__FILE__, __LINE__, msg);
      }
      try {
	int oid = get_hardlink(dset, t_fpn);
	// Break the cyclic loop created by hard links.
	read_objects(das, t_fpn, dset, num_attr);
	if(oid !=0){
	  // Add attribute table with HARDLINK
	  AttrTable* at = das.add_table(t_fpn, new AttrTable);
	  at->append_attr("HDF5_HARDLINK", STRING, paths.get_name(oid));
	}

      }
      catch(Error & e) {
	delete[]t_fpn;
	throw;
      }

      H5Dclose(dset); // Need error handling
      delete[]t_fpn;
      break;
    }
    case H5G_TYPE:
      break;
#ifndef CF
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
#endif
      
    default:
      break;
    }
    type = -1;
    delete[]oname;

  } //  for (int i = 0; i < nelems; i++)
  
  DBG(cerr << "<depth_first():" << gname << endl);  
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn print_attr(hid_t type, int loc, void *sm_buf)
/// will get the printed representation of an attribute.
/// 
/// This function is based on netcdf-dods server.
/// 
/// \param hid_t  HDF5 data type id
/// \param loc    the number of array number
/// \param sm_buf pointer to an attribute
/// \return a char * to newly allocated memory, the caller must call delete []
/// \todo This probably needs to be re-considered! <hyokyung 2007.02.20. 11:56:18> 
/// \todo Needs to be re-written. <hyokyung 2007.02.20. 11:56:38>
////////////////////////////////////////////////////////////////////////////////
static char *
print_attr(hid_t type, int loc, void *sm_buf)
{
  int ll;
  int str_size = 0;
  
  char* buf = NULL;
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

    // change void pointer into the corresponding integer datatype. 
    // 32 should be long enough to hold one integer and one floating point number.

    rep = new char[32];

    bzero(rep, 32);
    // Garbage, Hacking! <hyokyung 2007.02.20. 11:56:49>
    if (H5Tequal(type, H5T_STD_U8BE) ||
	H5Tequal(type, H5T_STD_U8LE) ||
	H5Tequal(type, H5T_NATIVE_UCHAR)) {

      gp.tcp = (char *) sm_buf;
      tuchar = *(gp.tcp + loc);
      // represent uchar with numerical form since at
      // NASA aura files, type of missing value is unsigned char. ky 2007-5-4
      sprintf(rep, "%u", tuchar);
      //sprintf(rep, "%c", tuchar);
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
   
      // display byte in numerical form. This is for Aura file. 2007/5/4
      sprintf(rep, "%d", *(gp.tcp + loc));
      //sprintf(rep, "%c", *(gp.tcp + loc));
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
    str_size = H5Tget_size(type);
    DBG(cerr
	<< "=print_attr(): H5T_STRING sm_buf=" << (char*)sm_buf
	<< " size=" << str_size
	<< endl);

    buf = new char[str_size+1];
    strncpy(buf, (char*)sm_buf, str_size);
    buf[str_size] = '\0';
    rep = new char[str_size+3];    
    sprintf(rep, "\"%s\"", buf);
    rep[str_size+2] = '\0';
    break;

  default:
    rep = new char[1];
    rep[0] = '\0';
    break;
  }

  return rep;
}


////////////////////////////////////////////////////////////////////////////////
/// \fn print_type(hid_t type)
/// will get the corresponding DODS datatype.
/// This function will return the "text representation" of the correponding
/// datatype translated from HDF5. 
/// For unknown datatype, put it to string. 
/// \return static string
/// \param type datatype id
/// \todo  For unknown type, is null string correct?
///  <hyokyung 2007.02.20. 11:57:43>
////////////////////////////////////////////////////////////////////////////////
string
print_type(hid_t type)
{
  size_t size = 0;
  H5T_sign_t sign;

  switch (H5Tget_class(type)) {


  case H5T_INTEGER:
    // <hyokyung 2007.03. 8. 09:30:36>
    size = H5Tget_size(type);
    sign = H5Tget_sign(type);
    if (size == 1)
      return BYTE;

    if (size == 2){
      if(sign == H5T_SGN_2)
	return INT16;
      else
	return UINT16;
    }
    
    if (size == 4){
      if(sign == H5T_SGN_2)
	return INT32;
      else
	return UINT32;
    }
    return INT_ELSE;
    
  case H5T_FLOAT:
    if (H5Tget_size(type) == 4)
      return FLOAT32;
    else if (H5Tget_size(type) == 8)
      return FLOAT64;
    else
      return FLOAT_ELSE; // <hyokyung 2007.03. 8. 10:01:48>

  case H5T_STRING:
    return STRING;

  default: 
    return "Unmappable Type"; // <hyokyung 2007.02.20. 11:58:34>
  }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn read_objects(DAS & das, const string & varname, hid_t oid, int num_attr)
/// will fill in attributes of a dataset or a group into one DAS table.
///
/// \param das DAS object: reference
/// \param varname absolute name of either a dataset or a group
/// \param oid dset
/// \param num_attr number of attributes.
/// \return nothing
/// \see get_attr_info(hid_t dset, int index,
///                    DSattr_t * attr_inst_ptr,int *ignoreptr, char *error)
/// \see print_type()
////////////////////////////////////////////////////////////////////////////////
void
read_objects(DAS & das, const string & varname, hid_t oid, int num_attr)
{
  
  AttrTable *attr_table_ptr;
  
  DSattr_t attr_inst;

  char *hdf5_path;  
  char *newname;
  char *tempvalue;
  char *print_rep = NULL;  
  char *value;
  char ORI_SLASH = '/';
  char attr_name[5];
  
  hid_t ty_id;
  hid_t attr_id;

  int ignore_attr;
  int loc;

  ignore_attr = 0;

  // Obtain variable names. Put this variable name into das table 
  // regardless of the existing attributes in this object. 


  DBG(cerr << ">read_objects():"
      << "varname=" << varname
      << " id=" << oid
      <<endl);

  strcpy(attr_name, "name");
#ifndef CF      
#ifdef NASA_EOS_META
  if(eos.is_valid()){

    if(varname.find("StructMetadata") != string::npos){
      if(!eos.bmetadata_Struct){
	eos.bmetadata_Struct = true;
	AttrTable *at = das.get_table(varname);
	if (!at)
	  at = das.add_table(varname, new AttrTable);
	parser_arg arg(at);
	DBG(cerr << eos.metadata_Struct << endl);
	hdfeos_das_scan_string(eos.metadata_Struct);

	if (hdfeos_dasparse(static_cast < void *>(&arg)) != 0
	    || arg.status() == false)
	  cerr << "HDF-EOS StructMetdata parse error!\n";
	return;
      }
    }      


    if(varname.find("coremetadata") != string::npos){
      if(!eos.bmetadata_core){
	eos.bmetadata_core = true;
	AttrTable *at = das.get_table(varname);
	if (!at)
	  at = das.add_table(varname, new AttrTable);
	parser_arg arg(at);
	DBG(cerr << eos.metadata_core << endl);
	hdfeos_das_scan_string(eos.metadata_core);

	if (hdfeos_dasparse(static_cast < void *>(&arg)) != 0
	    || arg.status() == false)
	  cerr << "HDF-EOS coremetadata parse error!\n";
	return;
      }
    }      

    if(varname.find("Coremetadata") != string::npos){
      if(!eos.bmetadata_Core){
	eos.bmetadata_core = true;
	AttrTable *at = das.get_table(varname);
	if (!at)
	  at = das.add_table(varname, new AttrTable);
	parser_arg arg(at);
	DBG(cerr << eos.metadata_Core << endl);
	hdfeos_das_scan_string(eos.metadata_Core);

	if (hdfeos_dasparse(static_cast < void *>(&arg)) != 0
	    || arg.status() == false)
	  cerr << "HDF-EOS CoreMetadata parse error!\n";
	return;
      }
    }      

    if(varname.find("productmetadata") != string::npos){
      if(!eos.bmetadata_product){
	eos.bmetadata_core = true;
	AttrTable *at = das.get_table(varname);
	if (!at)
	  at = das.add_table(varname, new AttrTable);
	parser_arg arg(at);
	DBG(cerr << eos.metadata_product << endl);
	hdfeos_das_scan_string(eos.metadata_product);

	if (hdfeos_dasparse(static_cast < void *>(&arg)) != 0
	    || arg.status() == false)
	  cerr << "HDF-EOS productmetadata parse error!\n";
	return;
      }
    }      
    
    if(varname.find("ArchivedMetadata") != string::npos){
      if(!eos.bmetadata_Archived){
	eos.bmetadata_core = true;
	AttrTable *at = das.get_table(varname);
	if (!at)
	  at = das.add_table(varname, new AttrTable);
	parser_arg arg(at);
	DBG(cerr << eos.metadata_Archived << endl);
	hdfeos_das_scan_string(eos.metadata_Archived);

	if (hdfeos_dasparse(static_cast < void *>(&arg)) != 0
	    || arg.status() == false)
	  cerr << "HDF-EOS ArchivedMetadata parse error!\n";
	return;
      }
    }      

    if(varname.find("subsetmetadata") != string::npos){
      if(!eos.bmetadata_subset){
	eos.bmetadata_subset = true;
	AttrTable *at = das.get_table(varname);
	if (!at)
	  at = das.add_table(varname, new AttrTable);
	parser_arg arg(at);
	DBG(cerr << eos.metadata_subset << endl);
	hdfeos_das_scan_string(eos.metadata_subset);

	if (hdfeos_dasparse(static_cast < void *>(&arg)) != 0
	    || arg.status() == false)
	  cerr << "HDF-EOS subsetmetadata parse error!\n";
	return;
      }
    }      

  }
#endif
#endif  // #ifdef CF    

  // Prepare a variable for full path attribute.
  hdf5_path = new char[strlen(HDF5_OBJ_FULLPATH) + 1];
  bzero(hdf5_path, strlen(HDF5_OBJ_FULLPATH));
  strcpy(hdf5_path, HDF5_OBJ_FULLPATH);
  
  char *temp_varname = new char[varname.length() + 1];
  varname.copy(temp_varname, string::npos);
  temp_varname[varname.length()] = '\0';

// <hyokyung 2007.09. 6. 11:55:39>  
#ifdef SHORT_PATH 
  // Prepare a shortened name.
  char *new_varname = new char[varname.length()];
  // Look for last '/' character in the variable name.
  newname = strrchr(temp_varname, ORI_SLASH);
  if (newname != NULL)
    newname++;
  else
    newname = temp_varname;


  // This full path must not be root, root attribute is handled by find_gloattr().
  if (temp_varname[varname.length() - 1] == '/') {
    strncpy(new_varname, temp_varname, varname.length() - 1);
    new_varname[varname.length() - 1] = '\0';
    
    newname = strrchr(new_varname, ORI_SLASH);
    if (newname != NULL)
      newname++;
    else
      newname = new_varname;
  }
#else
  newname = temp_varname;
#endif
  
  // <hyokyung 2007.08. 2. 12:37:33>
#ifdef CF  
  if(strlen(newname) > 15)
    return;
#endif
  
  attr_table_ptr = das.get_table(newname);
  try {
    // How's this going to work? <hyokyung 2007.02.20. 13:27:34>
    if (!attr_table_ptr){
      DBG(cerr
	  << "=read_objects(): adding a table with name " << newname
	  << endl);
      attr_table_ptr = das.add_table(newname, new AttrTable);
    }
  }
  catch(Error & e) { // Why no error handling? <hyokyung 2007.02.20. 13:27:12>

    delete[]hdf5_path;
    delete[]temp_varname;
#ifdef SHORT_PATH    
    delete[]new_varname; // <hyokyung 2007.09. 6. 11:55:33>
#endif    
    throw;
  }

  char *fullpath = new char[strlen(temp_varname) + 3];

  bzero(fullpath, strlen(temp_varname) + 3);
  sprintf(fullpath, "\"%s\"", temp_varname);
#ifndef CF  
  try {
    attr_table_ptr->append_attr(hdf5_path, STRING, fullpath);
  }
  catch(Error & e) {
    delete[]hdf5_path;
    delete[]temp_varname;
#ifdef SHORT_PATH        
    delete[]new_varname;
#endif    
    delete[]fullpath;
    throw;
  }
#endif
  delete[]fullpath;
  delete[]hdf5_path;

  // Check the number of attributes in this HDF5 object and
  // put HDF5 attribute information into DAS table. 

  for (int j = 0; j < num_attr; j++) {

    // Obtain attribute information.
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
#ifdef SHORT_PATH      
      delete[]new_varname;
#endif      
      throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Since HDF5 attribute may be in string datatype,
    // it must be dealt properly. 

    // get data type. 
    ty_id = attr_inst.type;

    value = new char[attr_inst.need + sizeof(char)];

    if (!value) {
      delete[]temp_varname;
#ifdef SHORT_PATH      
      delete[]new_varname;
#endif      
      string msg = "h5_das handler: no enough memory";
      throw InternalErr(__FILE__, __LINE__, msg);
    }
    bzero(value, (attr_inst.need + sizeof(char)));
    DBG(cerr << "arttr_inst.need=" << attr_inst.need << endl);
    // Read HDF5 attribute data. 

    if (ty_id == H5T_STRING) {
      // ty_id: No conversion is needed. <hyokyung 2007.02.20. 13:28:08>
      if (H5Aread(attr_id, ty_id, (void *) value) < 0) {
	string msg =
	  "h5_das handler: unable to read HDF5 attribute data";
	delete[]temp_varname;
#ifdef SHORT_PATH	
	delete[]new_varname;
#endif	
	delete[]value;
	throw InternalErr(__FILE__, __LINE__, msg);
      }
    } else {
      if (H5Aread(attr_id, ty_id, (void *) value) < 0) {

	string msg =
	  "h5_das handler: unable to read HDF5 attribute data";
	delete[]temp_varname;
#ifdef SHORT_PATH	
	delete[]new_varname;
#endif	
	delete[]value;
	throw InternalErr(__FILE__, __LINE__, msg);
      }
      DBG(cerr << "=read_objects(): value=" << value << endl);
    }

    // add all the attributes in the array

    tempvalue = value;
    //  Create the "name" attribute if we can find long_name.
    //  Make it compatible with HDF4 server. 
    // .. if we can... Why? <hyokyung 2007.02.20. 13:28:18>
    if (strcmp(attr_inst.name, "long_name") == 0) {
      for (loc = 0; loc < (int) attr_inst.nelmts; loc++) {
	try {
	  print_rep = print_attr(ty_id, loc, tempvalue);


	  attr_table_ptr->append_attr(attr_name,
				      print_type(ty_id),
				      print_rep);
	  delete[]print_rep;
	}
	catch(Error & e) {
	  delete[]temp_varname;
#ifdef SHORT_PATH	  
	  delete[]new_varname;
#endif	  
	  delete[]value;
	  delete [] print_rep;
	  throw;
	}
      }
    } // if (strcmp(attr_inst.name, "long_name") == 0)
    // For scalar data, just read data once a time,
    // change it into DODS string. 

    if (attr_inst.ndims == 0) {
      for (loc = 0; loc < (int) attr_inst.nelmts; loc++) {
	try {
	  print_rep = print_attr(ty_id, loc, tempvalue);
#ifdef CF	  
	  attr_table_ptr->append_attr(eos.set_grads_attribute(attr_inst.name),
#else
	  attr_table_ptr->append_attr(attr_inst.name,
#endif				      
				      print_type(ty_id),
				      print_rep);
	  delete[]print_rep;
	}
	catch(Error & e) {
	  delete[] temp_varname;
#ifdef SHORT_PATH	  
	  delete[] new_varname;
#endif	  
	  delete[] value;
	  delete[] print_rep;
	}
      }
    } // if attr_inst.ndims == 0
    else {

      // 1. If the hdf5 data type is HDF5 string and ndims is not 0;
      // we will handle this differently. 
      DBG(cerr
	  << "=read_objects(): ndims="<< (int)attr_inst.ndims
	  << endl);
      loc = 0;
      int elesize = (int) H5Tget_size(attr_inst.type);

      if (elesize == 0) {
	string msg = "h5_das handler: unable to get attibute size";

	delete[]temp_varname;
#ifdef SHORT_PATH	
	delete[]new_varname;
#endif	
	delete[]value;
	throw InternalErr(__FILE__, __LINE__, msg);
      }
      
      for (int dim = 0; dim < (int) attr_inst.ndims; dim++) {
	// Can we use STL here? <hyokyung 2007.02.20. 13:28:47>

	for (int sizeindex = 0;
	     sizeindex < (int) attr_inst.size[dim]; sizeindex++) {
	  if (H5Tget_class(attr_inst.type) == H5T_STRING) {
	    try {
	      char *print_rep =
		print_attr(ty_id, loc, tempvalue);
#ifdef CF	      
	      attr_table_ptr->append_attr(eos.set_grads_attribute(attr_inst.name),
#else					  
	      attr_table_ptr->append_attr(attr_inst.name,
#endif					  
					  print_type(ty_id),
					  print_rep);
	      tempvalue = tempvalue + elesize;
	      DBG(cerr
		  << "tempvalue=" << tempvalue
		  << "elesize=" << elesize
		  << endl);
	      // <hyokyung 2007.02.27. 09:31:25>
	      delete[]print_rep;
	    }
	    catch(Error & e) {
	      delete[] temp_varname;
#ifdef SHORT_PATH	      
	      delete[] new_varname;
#endif 	      
	      delete[] value;
	      delete[] print_rep;
	      throw;
	    }
	  } // if (H5Tget_class(attr_inst.type) == H5T_STRING)

	  else {
	    try {

	      char *print_rep =
		print_attr(ty_id, loc, tempvalue);
	      DBG(cerr << "print_rep=" <<  print_rep << endl);
#ifdef CF	      
	      attr_table_ptr->append_attr(eos.set_grads_attribute(attr_inst.name),
#else					  
   	      attr_table_ptr->append_attr(attr_inst.name,
#endif					  
					  print_type(ty_id),
					  print_rep);
	      tempvalue = tempvalue + elesize; // <hyokyung 2007.06. 4. 15:47:21>
	      delete[]print_rep;
	    }
	    catch(Error & e) {
	      delete[]temp_varname;
#ifdef SHORT_PATH	      
	      delete[]new_varname;
#endif	      
	      delete[]value;
	      delete[]print_rep;
	      throw;
	    }
	  } // if (H5Tget_class(attr_inst.type) != H5T_STRING)
	} // for (int sizeindex = 0; sizeindex < (int) attr_inst.size[dim]; sizeindex++)
      } // for (int dim = 0; dim < (int) attr_inst.ndims; dim++)
      loc++;
    } // if attr_inst.ndims != 0
    delete[]value;
  } //   for (int j = 0; j < num_attr; j++)
#ifdef SHORT_PATH      
  delete[]new_varname;
#endif  
  delete[]temp_varname;
  DBG(cerr << "<read_objects()" <<endl);  
}

///////////////////////////////////////////////////////////////////////
/// \fn find_gloattr(hid_t file, DAS & das)
/// will fill in attributes of the root group into one DAS table.
///
/// The attribute is treated as global attribute.
///
/// \param das DAS object reference
/// \param file HDF5 file id
/// \error a string of error message to the dods interface.
/// \return true  if succeed
/// \return false if failed
/// \see get_attr_info()
/// \see print_type()
//////////////////////////////////////////////////////////////////////////

bool
find_gloattr(hid_t file, DAS & das)
{

  hid_t root;
  int num_attrs;
  DBG(cerr << ">find_gloattr()" <<endl);
  
#ifdef CF
  add_dimension_attributes(das);
#endif  


  root = H5Gopen(file, "/");
  
  // <hyokyung 2007.09.27. 12:09:40>  
  das.add_table("HDF5_ROOT_GROUP", new AttrTable);
  
  if (root < 0) {
    string msg = "unable to open HDF5 root group";
    throw InternalErr(__FILE__, __LINE__, msg);
  }
  get_hardlink(root, "/"); // <hyokyung 2007.06.15. 09:06:02>
  num_attrs = H5Aget_num_attrs(root);

  if (num_attrs < 0) {
    string msg = "fail to get attribute number";
    throw InternalErr(__FILE__, __LINE__, msg);
  }

  if (num_attrs == 0) {
    H5Gclose(root);
    DBG(cerr << "<find_gloattr():no attributes" <<endl);        
    return true;
  }

  try {
    read_objects(das, "H5_GLOBAL", root, num_attrs);
  }

  catch(Error & e) {
    DBG(cerr << "=find_gloattr():Error" <<endl);    
    H5Gclose(root);
    throw;
  }
  DBG(cerr << "=find_gloattr(): H5Gclose()" <<endl);  
  H5Gclose(root);
  DBG(cerr << "<find_gloattr()" <<endl);
  return true;
}


////////////////////////////////////////////////////////////////////////////////
/// \fn get_softlink(DAS & das, hid_t pgroup, const string & oname, int index)
/// will put softlink information into a DAS table.
///
/// \param das DAS object: reference
/// \param pgroup object id
/// \param oname object name: absolute name of a group
/// \param index Link index
///
/// \return true  if succeeded.
/// \return false if failed.
/// \remarks In case of error, it returns a string of error message
///          to the DAP interface.
/// \warning This is only a test, not supported in current version.
/// \todo This function may be removed. <hyokyung 2007.02.20. 13:29:12>
////////////////////////////////////////////////////////////////////////////////
bool
get_softlink(DAS & das, hid_t pgroup, const string & oname, int index)
{

  char *buf = NULL;
  char *finbuf = NULL;
  string finaltrans = "";
  H5G_stat_t statbuf;
  AttrTable *attr_table_ptr = NULL;
  char *temp_varname = NULL;
#if 0
  char *cptr;
  char ORI_SLASH = '/';
  char CHA_SLASH = '_';
#endif
  char str_num[6];

  DBG(cerr << ">get_softlink():" << oname << endl);
  
  // softlink attribute name will be "HDF5_softlink" + "link index". 
  sprintf(str_num, "%d", index);
  const char *temp_oname = oname.c_str();
  temp_varname = new char[strlen(HDF5_softlink) + 7];

  strcpy(temp_varname, HDF5_softlink);
  strcat(temp_varname, str_num);
  
  DBG(cerr << "=get_softlink():" << temp_varname << endl);
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
  // get the target information at statbuf. 
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
  // get link target name 
  if (H5Gget_linkval(pgroup, temp_oname, statbuf.linklen + 1, buf) < 0) {
    string msg = "h5das handler: unable to get link value. ";

    delete[]temp_varname;
    delete[]buf;
    throw InternalErr(__FILE__, __LINE__, msg);

    return false;
  }

  int c = strlen(buf) + 3;
  finbuf = new char[c];

  try {
    bzero(finbuf, c);
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
  DBG(cerr << "<get_softlink(): after temp_varname" << endl);
  delete[]buf;
  DBG(cerr << "<get_softlink(): after buf:" << finbuf << endl);  
  delete[]finbuf;
  DBG(cerr << "<get_softlink(): after finbuf" <<  endl);  
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn get_hardlink(hid_t pgroup, const string & oname)
/// will put hardlink information into a DAS table.
///
/// \param pgroup object id
/// \param oname object name: absolute name of a group
///
/// \return true  if succeeded.
/// \return false if failed.
/// \remarks In case of error, it returns a string of error message
///          to the DAP interface.
/// \warning This is only a test, not supported in current version.
////////////////////////////////////////////////////////////////////////////////
int
get_hardlink(hid_t pgroup, const string & oname)
{

  H5G_stat_t statbuf;
  
  haddr_t     objno = 0;              // Compact form of object's location 

  DBG(cerr << ">get_hardlink():" << oname << endl);

  const char *temp_oname = oname.c_str();
  // Get the target information at statbuf. 
  herr_t ret = H5Gget_objinfo(pgroup, temp_oname, 0, &statbuf);
  
  if (ret < 0) {
    throw InternalErr(__FILE__, __LINE__,
		      "h5_das handler: cannot get hdf5 group information");
  }

  DBG(cerr << "=get_hardlink(): number of links " << statbuf.nlink << endl);

  if(statbuf.nlink >= 2){
    objno = (haddr_t)statbuf.objno[0] | ((haddr_t)statbuf.objno[1] << (8 * sizeof(long)));
    DBG(cerr << "=get_hardlink() objno=" << objno << endl);    
    if(!paths.add(objno, oname)){
      return objno;
    }
    else{
      return 0;
    }
      
  }
  else{
    return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn read_comments(DAS & das, const string & varname, hid_t oid)
/// will fill in attributes of a group's comment into DAS table.
///
/// \param das DAS object: reference
/// \param varname absolute name of an object
/// \param oid object id
/// \return nothing
////////////////////////////////////////////////////////////////////////////////
void
read_comments(DAS & das, const string & varname, hid_t oid)
{

  char comment[max_str_len-2];
  char comment2[max_str_len];

  // Borrowed from the dump_comment(hid_t obj_id) function in h5dump.c.
  comment[0] = '\0';  
  H5Gget_comment(oid, ".", sizeof(comment), comment);
  if (comment[0]) {
    sprintf(comment2, "\"%s\"", comment);
    comment2[max_str_len - 1] =  '\0';
    // Insert this comment into the das table.
    DBG(cerr
	<< "=read_comments(): comment="<< comment2
	<< " varname="<< varname
	<< endl);
    AttrTable *at = das.get_table(varname);
    if (!at){
      DBG(cerr
	  << "=read_comments(): creating a new attribute table for "
	  << varname
	  << endl);
      at = das.add_table(varname, new AttrTable);
      at->append_attr("HDF5_COMMENT", STRING, comment2);
    }
    else{
      at->append_attr("HDF5_COMMENT", STRING, comment2);	
    }

  }    
}

void add_group_structure_info(DAS & das, char* gname, char* oname, bool is_group)
{

  string search("/");
  string replace(".");
  string::size_type pos = 1;
  
  string full_path = string(gname);
  // Cut the last '/'.
  while ( (pos = full_path.find(search, pos)) != string::npos ) {
    full_path.replace( pos, search.size(), replace );
    pos++;
  }
  if(strcmp(gname, "/") == 0){
    full_path.replace(0, 1, "HDF5_ROOT_GROUP");
  }
  else{
    full_path.replace(0, 1, "HDF5_ROOT_GROUP.");
    full_path = full_path.substr(0, full_path.length()-1);
  }
  
  DBG(cerr << full_path << endl);

  AttrTable *at = das.get_table(full_path);
  

  
  if(is_group) {
    at->append_container(oname);  
  }
  else{
    unsigned int str_size = strlen(oname);    
    char* rep = new char[str_size+3];    
    sprintf(rep, "\"%s\"", oname);
    rep[str_size+2] = '\0';
    
    at->append_attr("Dataset", "String", rep);
  }

}
  
#ifdef CF
////////////////////////////////////////////////////////////////////////////////
/// \fn add_dimension_attributes(DAS & das)
/// will put pseudo attributes for CF(a.k.a COARDS) convention compatibility
///
/// \param das DAS object: reference
/// \remarks This is necessary for grads compatibility only
/// \todo This function may be removed. <hyokyung 2007.02.20. 13:29:12>
////////////////////////////////////////////////////////////////////////////////
void add_dimension_attributes(DAS & das)
{
  DBG(cerr << ">add_dimension_attributes()" << endl);
  AttrTable *at;

//   at = das.add_table("NC_GLOBAL", new AttrTable);
//   at->append_attr("title", STRING, "\"Test\"");
//   at->append_attr("Conventions", STRING, "\"COARDS, GrADS\"");
//   at->append_attr("dataType", STRING, "\"Grid\"");

//   at = das.add_table("XDim", new AttrTable);
//   at->append_attr("units", STRING, "\"degrees_east\"");
//   at->append_attr("grads_mapping", STRING, "\"linear\"");
  
//   at = das.add_table("YDim", new AttrTable);
//   at->append_attr("units", STRING, "\"degrees_north\"");
//   at->append_attr("grads_mapping", STRING, "\"linear\"");

//   at = das.add_table("ZDim", new AttrTable);
//   at->append_attr("units", STRING, "\"milibar\"");

//   at = das.add_table("nCandidate", new AttrTable);
//   at->append_attr("units", STRING, "\"level\"");


  
  DBG(cerr << "<add_dimension_attributes()" << endl);
}
#endif
// $Log$
