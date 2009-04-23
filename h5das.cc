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
/// \author Muqun Yang <myang6@hdfgroup.org>
///
/// Copyright (c) 2007 The HDF Group
///
/// Copyright (C) 1999 National Center for Supercomputing Applications.
///
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#include "config_hdf5.h"

#include <string>
#include <sstream>

#include <InternalErr.h>
#include <Str.h>
#include <parser.h>
#include <debug.h>

#include "h5das.h"
#include "common.h"
#include "H5Git.h"
#include "H5EOS.h"
#include "H5PathFinder.h"

// Added for the fix for ticket 1163. jhrg 7/31/08
#define ATTR_STRING_QUOTE_FIX

/// A global variable that handles HDF-EOS5 files.
H5EOS eos;

/// A variable for remembering visited paths to break ties if they exist.
H5PathFinder paths;

/// EOS parser related variables
struct yy_buffer_state;

/// This function parses Metadata in NASA EOS files.
int hdfeos_dasparse(void *arg);

/// Buffer state for NASA EOS metadata scanner
yy_buffer_state *hdfeos_das_scan_string(const char *str);

extern bool valid_projection;	// <hyokyung 2009.01.16. 10:41:39>

////////////////////////////////////////////////////////////////////////////////
/// \fn depth_first(hid_t pid, const char *gname, DAS & das)
/// depth first traversal of hdf5 file attributes.
///
/// This function will walk through hdf5 group using depth-
/// first approach to obtain all the group and dataset attributes
/// of an hdf5 file.
/// During the process of depth first search, DAS table will be filled.
/// In case of errors, an exception will be thrown.
///
/// \param pid    dataset id(group id)
/// \param gname  group name(absolute name from root group)
/// \param das    reference of DAS object
/// \return void
///
////////////////////////////////////////////////////////////////////////////////
void depth_first(hid_t pid, const char *gname, DAS & das)
{
    /// To keep track of soft links.
    static int slinkindex;

    hsize_t nelems;
    if (H5Gget_num_objs(pid, &nelems) < 0) {
        string msg = "counting hdf5 group elements error for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    read_comments(das, gname, pid);

    if (H5Gget_num_objs(pid, (hsize_t *) & nelems) < 0) {
        string msg = "counting hdf5 group elements error for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    for (int i = 0; i < nelems; i++) {
        // Query the length of object name.
        ssize_t oname_size = H5Gget_objname_by_idx(pid, (hsize_t) i, NULL,
						   (size_t) DODS_NAMELEN);

        if (oname_size <= 0) {
            string msg = "hdf5 object name error from: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        char *oname = NULL;
	try {
	    // Obtain the name of the object.
	    oname = new char[(size_t) oname_size + 1];
	    if (H5Gget_objname_by_idx(pid, (hsize_t) i, oname,
				      (size_t) (oname_size + 1)) < 0) {
		// oname is deleted in the catch ... block below so
		// shouldn't be deleted here. pwest Mar 18, 2009
		//delete[] oname;
		string msg = "hdf5 object name error from: ";
		msg += gname;
		throw InternalErr(__FILE__, __LINE__, msg);
	    }

	    int type = H5Gget_objtype_by_idx(pid, (hsize_t) i);
	    if (type < 0) {
		// oname is deleted in the catch ... block below so
		// shouldn't be deleted here. pwest Mar 18, 2009
	    	//delete[] oname;
		string msg = "hdf5 object type error from: ";
		msg += gname;
		throw InternalErr(__FILE__, __LINE__, msg);
	    }

	    switch (type) {

	      case H5G_GROUP:{
		  DBG(cerr << "=depth_first():H5G_GROUP " << oname << endl);
#ifndef CF
		  add_group_structure_info(das, gname, oname, true);
#endif
		  string full_path_name = string(gname) + string(oname) + "/";
		  hid_t cgroup = H5Gopen(pid, full_path_name.c_str());

		  if (cgroup < 0) {
		      // oname is deleted in the catch ... block below so
		      // shouldn't be deleted here. pwest Mar 18, 2009
		      //delete[] oname;
		      string msg = "opening hdf5 group failed for ";
		      msg += full_path_name;
		      throw InternalErr(__FILE__, __LINE__, msg);
		  }

		  int num_attr;
		  if ((num_attr = H5Aget_num_attrs(cgroup)) < 0) {
		      // oname is deleted in the catch ... block below so
		      // shouldn't be deleted here. pwest Mar 18, 2009
		      //delete[] oname;
		      string msg = "failed to obtain hdf5 attribute in group ";
		      msg += full_path_name;
		      throw InternalErr(__FILE__, __LINE__, msg);
		  }

		  string oid = get_hardlink(cgroup, full_path_name.c_str());
#ifndef CF
		  read_objects(das, full_path_name.c_str(), cgroup, num_attr);
#endif
		  // Break the cyclic loop created by hard links.
		  if (oid == "") {    // <hyokyung 2007.06.11. 13:53:12>
		      depth_first(cgroup, full_path_name.c_str(), das);
		  } else {
		      // Add attribute table with HARDLINK.
		      AttrTable *at =
			  das.add_table(full_path_name, new AttrTable);
		      at->append_attr("HDF5_HARDLINK", STRING,
				      paths.get_name(oid));
		  }

		  H5Gclose(cgroup);       // also need error handling.
		  break;
	      } // case H5G_GROUP

	      case H5G_DATASET:{
		  DBG(cerr << "=depth_first():H5G_DATASET " << oname <<
		      endl);
#ifndef CF
		  add_group_structure_info(das, gname, oname, false);
#endif
		  string full_path_name = string(gname) + string(oname);
		  hid_t dset;
		  // Open the dataset
		  if ((dset = H5Dopen(pid, full_path_name.c_str())) < 0) {
		      // oname is deleted in the catch ... block below so
		      // shouldn't be deleted here. pwest Mar 18, 2009
		      //delete[] oname;
		      string msg = "unable to open hdf5 dataset of group ";
		      msg += gname;
		      throw InternalErr(__FILE__, __LINE__, msg);
		  }

		  // Obtain number of attributes in this dataset.
		  int num_attr;
		  if ((num_attr = H5Aget_num_attrs(dset)) < 0) {
		      // oname is deleted in the catch ... block below so
		      // shouldn't be deleted here. pwest Mar 18, 2009
		      //delete[] oname;
		      string msg = "failed to get hdf5 attribute in dataset ";
		      msg += full_path_name;
		      throw InternalErr(__FILE__, __LINE__, msg);
		  }

		  string oid = get_hardlink(dset, full_path_name);
		  // Break the cyclic loop created by hard links.
		  // Should this be wrapped in #ifndef CF #endif? jhrg 4/17/08
		  read_objects(das, full_path_name, dset, num_attr);
		  if (!oid.empty()) {
		      // Add attribute table with HARDLINK
		      AttrTable *at =
			  das.add_table(full_path_name, new AttrTable);
		      at->append_attr("HDF5_HARDLINK", STRING,
				      paths.get_name(oid));
		  }

		  H5Dclose(dset); // Need error handling
		  break;
	      }                   // case H5G_DATASET

	      case H5G_TYPE:
		break;
#ifndef CF
	      case H5G_LINK:
		slinkindex++;
		get_softlink(das, pid, oname, slinkindex);
		break;
#endif

	      default:
		break;
	    }

	    delete[]oname;
	} // try
	catch (...) {
	    // if a memory allocation exception is thrown creating
	    // oname it is caught here, meaning oname is null. pwest
	    // Mar 18, 2009
	    if( oname ) delete[] oname;
	    throw;
	}
    } //  for (int i = 0; i < nelems; i++)

    DBG(cerr << "<depth_first():" << gname << endl);
}

////////////////////////////////////////////////////////////////////////////////
/// \fn print_attr(hid_t type, int loc, void *sm_buf)
/// will get the printed representation of an attribute.
///
/// This function is based on netcdf-dods server.
///
/// \param type  HDF5 data type id
/// \param loc    the number of array number
/// \param sm_buf pointer to an attribute
/// \return a char * to newly allocated memory, the caller must call delete []
/// \todo This probably needs to be re-considered! <hyokyung 2007.02.20. 11:56:18>
/// \todo Needs to be re-written. <hyokyung 2007.02.20. 11:56:38>
////////////////////////////////////////////////////////////////////////////////
static char *print_attr(hid_t type, int loc, void *sm_buf) {
    union {
        char *tcp;
        short *tsp;
        unsigned short *tusp;
        int *tip;
        long *tlp;
        float *tfp;
        double *tdp;
    } gp;

    char *rep = 0;		// This holds the return value
    try {
	switch (H5Tget_class(type)) {
	  case H5T_INTEGER: {
	      // change void pointer into the corresponding integer datatype.
	      // 32 should be long enough to hold one integer and one
	      // floating point number.
	      rep = new char[32];
	      memset(rep, 0, 32);


	      if (H5Tequal(type, H5T_STD_U8BE) || H5Tequal(type, H5T_STD_U8LE)
		  || H5Tequal(type, H5T_NATIVE_UCHAR)) {

		  gp.tcp = (char *) sm_buf;
		  unsigned char tuchar = *(gp.tcp + loc);
		  // represent uchar with numerical form since at NASA aura
		  // files, type of missing value is unsigned char. ky
		  // 2007-5-4
		  snprintf(rep, 32, "%u", tuchar);
	      }
	      else if (H5Tequal(type, H5T_STD_U16BE)
		       || H5Tequal(type, H5T_STD_U16LE)
		       || H5Tequal(type, H5T_NATIVE_USHORT)) {
		  gp.tusp = (unsigned short *) sm_buf;
		  snprintf(rep, 32, "%hu", *(gp.tusp + loc));
	      }

	      else if (H5Tequal(type, H5T_STD_U32BE)
		       || H5Tequal(type, H5T_STD_U32LE)
		       || H5Tequal(type, H5T_NATIVE_UINT)) {

		  gp.tip = (int *) sm_buf;
		  snprintf(rep, 32, "%u", *(gp.tip + loc));
	      }

	      else if (H5Tequal(type, H5T_STD_U64BE)
		       || H5Tequal(type, H5T_STD_U64LE)
		       || H5Tequal(type, H5T_NATIVE_ULONG)
		       || H5Tequal(type, H5T_NATIVE_ULLONG)) {

		  gp.tlp = (long *) sm_buf;
		  snprintf(rep, 32, "%lu", *(gp.tlp + loc));
	      }

	      else if (H5Tequal(type, H5T_STD_I8BE)
		       || H5Tequal(type, H5T_STD_I8LE)
		       || H5Tequal(type, H5T_NATIVE_CHAR)) {

		  gp.tcp = (char *) sm_buf;
		  // display byte in numerical form. This is for Aura file.
		  // 2007/5/4
		  // This generates an attribute like "Byte _FillValue -127".
		  // It can cause IDV to crash since Java OPeNDAP expects
		  // Byte value > 0.
		  // See ticket: http://scm.opendap.org/trac/ticket/1199
		  snprintf(rep, 32, "%d", *(gp.tcp + loc));
	      }

	      else if (H5Tequal(type, H5T_STD_I16BE)
		       || H5Tequal(type, H5T_STD_I16LE)
		       || H5Tequal(type, H5T_NATIVE_SHORT)) {

		  gp.tsp = (short *) sm_buf;
		  snprintf(rep, 32, "%hd", *(gp.tsp + loc));
	      }

	      else if (H5Tequal(type, H5T_STD_I32BE)
		       || H5Tequal(type, H5T_STD_I32LE)
		       || H5Tequal(type, H5T_NATIVE_INT)) {

		  gp.tip = (int *) sm_buf;
		  snprintf(rep, 32, "%d", *(gp.tip + loc));
	      }

	      else if (H5Tequal(type, H5T_STD_I64BE)
		       || H5Tequal(type, H5T_STD_I64LE)
		       || H5Tequal(type, H5T_NATIVE_LONG)
		       || H5Tequal(type, H5T_NATIVE_LLONG)) {

		  gp.tlp = (long *) sm_buf;
		  snprintf(rep, 32, "%ld", *(gp.tlp + loc));
	      }

	      break;
	  }

	  case H5T_FLOAT: {
	      rep = new char[32];
	      memset(rep, 0, 32);
	      char gps[30];
	      if (H5Tget_size(type) == 4) {
		  gp.tfp = (float *) sm_buf;
		  snprintf(gps, 30, "%.10g", *(gp.tfp + loc));
		  int ll = strlen(gps);

		  if (!strchr(gps, '.') && !strchr(gps, 'e'))
		      gps[ll++] = '.';

		  gps[ll] = '\0';
		  snprintf(rep, 32, "%s", gps);
	      } else if (H5Tget_size(type) == 8) {
		  gp.tdp = (double *) sm_buf;
		  snprintf(gps, 30, "%.17g", *(gp.tdp + loc));
		  int ll = strlen(gps);
		  if (!strchr(gps, '.') && !strchr(gps, 'e'))
		      gps[ll++] = '.';
		  gps[ll] = '\0';
		  snprintf(rep, 32, "%s", gps);
	      }
	      break;
	  }

	  case H5T_STRING: {
	      int str_size = H5Tget_size(type);
	      DBG(cerr << "=print_attr(): H5T_STRING sm_buf=" << (char *) sm_buf
		  << " size=" << str_size << endl);
	      char *buf = NULL;
	      // This try/catch block is here to protect the allocation of buf
	      try {
		  buf = new char[str_size + 1];
		  strncpy(buf, (char *) sm_buf, str_size);
		  buf[str_size] = '\0';
		  rep = new char[str_size + 3];
		  snprintf(rep, str_size + 3, "\"%s\"", buf);
		  rep[str_size + 2] = '\0';
		  delete[] buf; buf = 0;
	      }
	      catch (...) {
		  // if memory allocation exceptions are thrown
		  // creating either buf or rep then they would
		  // still be null respectively, so need to check if
		  // the exist before deleting. pwest Mar 18, 2009
		  if( buf ) delete[] buf;
		  // rep is deleted in the catch ... below so
		  // shouldn't be deleted here. pwest Mar 18, 2009
		  //if( rep ) delete[] rep;
		  throw;
	      }
	      break;
	  }

	  default:
	    rep = new char[1];
	    rep[0] = '\0';
	    break;
	} // switch(H5Tget_class(type))
    } // try
    catch (...) {
	// if a memory allocation exception is thrown creating rep
	// then it is caught here and rep would be null. pwest Mar 18, 2009
	if( rep ) delete[] rep;
	throw;
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
string print_type(hid_t type)
{
    size_t size = 0;
    H5T_sign_t sign;

    switch (H5Tget_class(type)) {

      case H5T_INTEGER:
        // <hyokyung 2007.03. 8. 09:30:36>
        size = H5Tget_size(type);
        sign = H5Tget_sign(type);
        if (size == 1){
	  if (sign == H5T_SGN_2){
            return INT16;
	  }
	  else{
	    return BYTE;
	  }
	}

        if (size == 2) {
            if (sign == H5T_SGN_2)
                return INT16;
            else
                return UINT16;
        }

        if (size == 4) {
            if (sign == H5T_SGN_2)
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
            return FLOAT_ELSE;  // <hyokyung 2007.03. 8. 10:01:48>

      case H5T_STRING:
        return STRING;

      default:
        return "Unmappable Type";       // <hyokyung 2007.02.20. 11:58:34>
    }
}

// For CF we have to use a special filter to get the atribute name, while
// for a non-CF-aware build we just use the name. 3/2008 jhrg
#ifdef CF
#define GET_NAME(x) eos.get_CF_name((x))
#else
#define GET_NAME(x) (x)
#endif

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
void read_objects(DAS & das, const string & varname, hid_t oid, int num_attr) {

    // Obtain variable names. Put this variable name into das table
    // regardless of the existing attributes in this object.

    DBG(cerr << ">read_objects():"
        << "varname=" << varname << " id=" << oid << endl);

#ifdef NASA_EOS_META
    if (eos.is_valid()) {
#ifndef CF      
        if (varname.find("StructMetadata") != string::npos) {
            if (!eos.bmetadata_Struct) {
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
#endif
        if (varname.find("coremetadata") != string::npos) {
            if (!eos.bmetadata_core) {
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

        if (varname.find("Coremetadata") != string::npos) {
            if (!eos.bmetadata_Core) {
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
#ifndef CF
        if (varname.find("productmetadata") != string::npos) {
            if (!eos.bmetadata_product) {
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

        if (varname.find("ArchivedMetadata") != string::npos) {
            if (!eos.bmetadata_Archived) {
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

        if (varname.find("subsetmetadata") != string::npos) {
            if (!eos.bmetadata_subset) {
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
#endif                          // #ifndef CF    	
    }

#endif                          // #ifdef NASA_EOS_META


    // Prepare a variable for full path attribute.
    string hdf5_path = HDF5_OBJ_FULLPATH;

    // <hyokyung 2007.09. 6. 11:55:39>
    // Rewrote to use C++ strings 3/2008 jhrg
    string newname;

#ifdef CF    
    newname = eos.get_short_name(varname);
    if(newname == ""){
      newname = varname;
    }
#else
    newname = varname;
#endif
    DBG(cerr << "=read_objects(): new variable name=" << newname << endl);
    
    AttrTable *attr_table_ptr = das.get_table(newname);
    if (!attr_table_ptr) {
        DBG(cerr << "=read_objects(): adding a table with name " << newname
            << endl);
        attr_table_ptr = das.add_table(newname, new AttrTable);
    }
#ifndef CF
#ifndef ATTR_STRING_QUOTE_FIX
    string fullpath = string("\"") + varname + string("\"");
    attr_table_ptr->append_attr(hdf5_path.c_str(), STRING, fullpath);
#else
    attr_table_ptr->append_attr(hdf5_path.c_str(), STRING, varname);
#endif // ATTR_STRINNG_QUOTE_FIX
#endif // CF

    // Check the number of attributes in this HDF5 object and
    // put HDF5 attribute information into DAS table.
    char *print_rep = NULL;
    char *value = NULL;
    try {
	for (int j = 0; j < num_attr; j++) {
	    // Obtain attribute information.
	    DSattr_t attr_inst;
	    int ignore_attr = 0;
	    hid_t attr_id = get_attr_info(oid, j, &attr_inst, &ignore_attr);
	    if (attr_id == 0 && ignore_attr == 1)
		continue;

	    // Since HDF5 attribute may be in string datatype, it must be dealt
	    // properly. Get data type.
	    hid_t ty_id = attr_inst.type;
	    char *value = new char[attr_inst.need + sizeof(char)];
	    memset(value, 0, attr_inst.need + sizeof(char));

	    DBG(cerr << "arttr_inst.need=" << attr_inst.need << endl);
	    // Read HDF5 attribute data.

	    if (H5Aread(attr_id, ty_id, (void *) value) < 0) {
	      // value is deleted in the catch block below so
	      // shouldn't be deleted here. pwest Mar 18, 2009
	      //delete[] value;
	      throw InternalErr(__FILE__, __LINE__,
				"unable to read HDF5 attribute data");
	    }
	    // Add all attributes in the array.
            //  Create the "name" attribute if we can find long_name.
            //  Make it compatible with HDF4 server.
            if (strcmp(attr_inst.name, "long_name") == 0) {
                for (int loc = 0; loc < (int) attr_inst.nelmts; loc++) {
                    print_rep = print_attr(ty_id, loc, value);
                    attr_table_ptr->append_attr("name", print_type(ty_id),
                                                print_rep);
                    delete[]print_rep; print_rep = 0;
                }
            }

            // For scalar data, just read data once a time,
            // Change it into DODS string.
            if (attr_inst.ndims == 0) {
                for (int loc = 0; loc < (int) attr_inst.nelmts; loc++) {
                    print_rep = print_attr(ty_id, loc, value);
                    // GET_NAME is defined at the top of this function.
                    attr_table_ptr->append_attr(GET_NAME(attr_inst.name),
                                                print_type(ty_id),
                                                print_rep);

                    delete[]print_rep; print_rep = 0;
                }
            }
            else {
                // 1. If the hdf5 data type is HDF5 string and ndims is not 0;
                // we will handle this differently.
                DBG(cerr << "=read_objects(): ndims=" << (int) attr_inst.
                    ndims << endl);

                int elesize = (int) H5Tget_size(attr_inst.type);
                if (elesize == 0) {
		    // value is deleted in the catch ... block below
		    // so shouldn't be deleted here. pwest Mar 18, 2009
		    //delete[] value;
                    throw InternalErr(__FILE__, __LINE__,
				      "unable to get attibute size");
                }

		char *tempvalue = value;
                for (int dim = 0; dim < (int) attr_inst.ndims; dim++) {
                    for (int sizeindex = 0;
                         sizeindex < (int) attr_inst.size[dim];
                         sizeindex++) {
                            print_rep = print_attr(ty_id, 0/*loc*/, tempvalue);
                            attr_table_ptr->
                                append_attr(GET_NAME(attr_inst.name),
                                            print_type(ty_id), print_rep);

                            tempvalue = tempvalue + elesize;
                            DBG(cerr << "tempvalue=" << tempvalue
                                << "elesize=" << elesize << endl);
                            // <hyokyung 2007.02.27. 09:31:25>
                            delete[]print_rep; print_rep = 0;
                    }           // for (int sizeindex = 0; ...
                }               // for (int dim = 0; ...
            }			// if attr_inst.ndims != 0
	    delete[] value; value = 0;
	}		// for (int j = 0; j < num_attr; j++)
    }			// try - protects print_rep and value
    catch(...) {
	if( print_rep ) delete[] print_rep;
	if( value ) delete[] value;
	throw;
    }

    DBG(cerr << "<read_objects()" << endl);
}

///////////////////////////////////////////////////////////////////////
/// \fn find_gloattr(hid_t file, DAS & das)
/// will fill in attributes of the root group into one DAS table.
///
/// The attribute is treated as global attribute.
///
/// \param das DAS object reference
/// \param file HDF5 file id
/// \exception msg string of error message to the dods interface.
/// \return void
/// \see get_attr_info()
/// \see print_type()
//////////////////////////////////////////////////////////////////////////
void find_gloattr(hid_t file, DAS & das)
{
    DBG(cerr << ">find_gloattr()" << endl);

#ifdef CF
    if(eos.is_valid() && valid_projection){
      add_dimension_attributes(das);
    }
    if(eos.is_swath()){
      write_dimension_attributes_swath(das);
    }
#endif

    hid_t root = H5Gopen(file, "/");
    try {
	if (root < 0)
	    throw InternalErr(__FILE__, __LINE__,
			      "unable to open HDF5 root group");
#ifndef CF
	// <hyokyung 2007.09.27. 12:09:40>
	das.add_table("HDF5_ROOT_GROUP", new AttrTable);
#endif

	get_hardlink(root, "/");    // <hyokyung 2007.06.15. 09:06:02>
	int num_attrs = H5Aget_num_attrs(root);
	if (num_attrs < 0)
	    throw InternalErr(__FILE__, __LINE__,
			      "unable to get attribute number");

	if (num_attrs == 0) {
	    H5Gclose(root);
	    DBG(cerr << "<find_gloattr():no attributes" << endl);
	    return;
	}

        read_objects(das, "H5_GLOBAL", root, num_attrs);

	DBG(cerr << "=find_gloattr(): H5Gclose()" << endl);
	H5Gclose(root);
	DBG(cerr << "<find_gloattr()" << endl);
    }
    catch (...) {
	H5Gclose(root);
	throw;
    }
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
/// \return void
/// \remarks In case of error, it throws an exception
/// \warning This is only a test, not supported in current version.
/// \todo This function may be removed. <hyokyung 2007.02.20. 13:29:12>
////////////////////////////////////////////////////////////////////////////////
void get_softlink(DAS & das, hid_t pgroup, const string & oname, int index)
{
    DBG(cerr << ">get_softlink():" << oname << endl);

    ostringstream oss;
    oss << string(HDF5_softlink);
    oss << index;
    string temp_varname = oss.str();

    DBG(cerr << "=get_softlink():" << temp_varname << endl);
    AttrTable *attr_table_ptr = das.get_table(temp_varname);
    if (!attr_table_ptr)
	attr_table_ptr = das.add_table(temp_varname, new AttrTable);

    // get the target information at statbuf.
    H5G_stat_t statbuf;
    herr_t ret = H5Gget_objinfo(pgroup, oname.c_str(), 0, &statbuf);
    if (ret < 0)
        throw InternalErr(__FILE__, __LINE__,
                          "cannot get hdf5 group information");

    char *buf = 0;
    try {
	buf = new char[(statbuf.linklen + 1) * sizeof(char)];
	// get link target name
	if (H5Gget_linkval(pgroup, oname.c_str(), statbuf.linklen + 1, buf)
	    < 0) {
	    // buf is deleted in the catch ... block below so
	    // shouldn't be deleted here. pwest Mar 18, 2009
	    //delete[] buf;
	    throw InternalErr(__FILE__, __LINE__, "unable to get link value");
	}

#ifndef ATTR_STRING_QUOTE_FIX
	string finbuf = string("\"") + string(buf) + string("\"");
        attr_table_ptr->append_attr(oname, STRING, finbuf);
#else
        attr_table_ptr->append_attr(oname, STRING, buf);
#endif
        delete[]buf;
    }
    catch (...) {
	delete[] buf;
	throw;
    }
#ifndef ATTR_STRING_QUOTE_FIX
    DBG(cerr << "<get_softlink(): after buf:" << finbuf << endl);
#endif    
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

string get_hardlink(hid_t pgroup, const string & oname) {
    DBG(cerr << ">get_hardlink():" << oname << endl);

    // Get the target information at statbuf.
    H5G_stat_t statbuf;
    H5Gget_objinfo(pgroup, oname.c_str(), 0, &statbuf);

    if (statbuf.nlink >= 2) {
#if 0
        objno = (haddr_t)statbuf.objno[0]
	    | ((haddr_t)statbuf.objno[1] << (8 * sizeof(long)));
#endif
        ostringstream oss;
        oss << hex << statbuf.objno[0] << statbuf.objno[1];
        string objno = oss.str();

        DBG(cerr << "=get_hardlink() objno=" << objno << endl);

        if (!paths.add(objno, oname)) {
            return objno;
        }
	else {
            return "";
        }

    }
    else {
        return "";
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
void read_comments(DAS & das, const string & varname, hid_t oid)
{
    // Borrowed from the dump_comment(hid_t obj_id) function in h5dump.c.
    char comment[max_str_len - 2];
    comment[0] = '\0';
    H5Gget_comment(oid, ".", sizeof(comment), comment);
    if (comment[0]) {
#ifndef ATTR_STRING_QUOTE_FIX
        string quoted_comment = string("\"") + string(comment) + string("\"");
#endif
        // Insert this comment into the das table.
        AttrTable *at = das.get_table(varname);
        if (!at)
            at = das.add_table(varname, new AttrTable);

#ifndef ATTR_STRING_QUOTE_FIX
        at->append_attr("HDF5_COMMENT", STRING, quoted_comment);
#else
        at->append_attr("HDF5_COMMENT", STRING, comment);
#endif
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \fn add_group_structure_info(DAS & das, const char* gname, char* oname, bool is_group)
/// will insert group information in a structure format into DAS table.
///
/// This function adds a special attribute called "HDF5_ROOT_GROUP" if the \a
/// gname is "/". If \a is_group is true, it keeps appending new attribute
/// table called \a oname under the \a gname path. If \a is_group is false, it appends
/// a string attribute called \a oname.
///
/// \param das DAS object: reference
/// \param gname absolute group pathname of an object
/// \param oname name of object
/// \param is_group indicates whether it's a dataset or group
/// \return nothing
////////////////////////////////////////////////////////////////////////////////
void add_group_structure_info(DAS & das, const char *gname, char *oname,
        bool is_group)
{

    string search("/");
    string replace(".");
    string::size_type pos = 1;

    string full_path = string(gname);
    // Cut the last '/'.
    while ((pos = full_path.find(search, pos)) != string::npos) {
        full_path.replace(pos, search.size(), replace);
        pos++;
    }
    if (strcmp(gname, "/") == 0) {
        full_path.replace(0, 1, "HDF5_ROOT_GROUP");
    }
    else {
        full_path.replace(0, 1, "HDF5_ROOT_GROUP.");
        full_path = full_path.substr(0, full_path.length() - 1);
    }

    DBG(cerr << full_path << endl);

    AttrTable *at = das.get_table(full_path);

    if (is_group) {
        at->append_container(oname);
    }
    else {
#ifndef ATTR_STRING_QUOTE_FIX
        string quoted_oname = string("\"") + string(oname) + string("\"");
        at->append_attr("Dataset", "String", quoted_oname);
#else
        at->append_attr("Dataset", "String", oname);
#endif
    }
}

#ifdef CF
////////////////////////////////////////////////////////////////////////////////
/// \fn add_dimension_attributes(DAS & das)
/// will put pseudo attributes for CF(a.k.a COARDS) convention compatibility.
/// This function is an example for NASA AURA data.
/// You need to modify this to add custom attributes that match dimension names.
///
/// \param das DAS object: reference
/// \remarks This is necessary for GrADS compatibility only
////////////////////////////////////////////////////////////////////////////////
void add_dimension_attributes(DAS & das)
{
    DBG(cerr << ">add_dimension_attributes()" << endl);
    AttrTable *at;
    vector < string > tokens;
    
    at = das.add_table("NC_GLOBAL", new AttrTable);
    at->append_attr("title", STRING, "\"NASA EOS Aura Grid\"");
    at->append_attr("Conventions", STRING, "\"COARDS, GrADS\"");
    at->append_attr("dataType", STRING, "\"Grid\"");
    //    at->append_attr("history", STRING,
    //                    "\"Tue Jan 1 00:00:00 CST 2008 : imported by GrADS Data Server 1.3\"");

    if(eos.get_dimension_size("XDim") > 0){
      at = das.add_table("lon", new AttrTable);
      at->append_attr("grads_dim", STRING, "\"x\"");
      at->append_attr("grads_mapping", STRING, "\"linear\"");
      {
	std::ostringstream o;
	o << "\"" << eos.get_dimension_size("XDim") << "\"";            
	at->append_attr("grads_size", STRING, o.str().c_str());
      }
      at->append_attr("units", STRING, "\"degrees_east\"");
      at->append_attr("long_name", STRING, "\"longitude\"");
      {
	std::ostringstream o;
	o << (eos.point_left / 1000000.0);      
	at->append_attr("minimum", FLOAT32, o.str().c_str());
      }
    
      {
	std::ostringstream o;
	o << (eos.point_right / 1000000.0);
	at->append_attr("maximum", FLOAT32, o.str().c_str());
      }
      {
	std::ostringstream o;
	o << (eos.gradient_x / 1000000.0);
	at->append_attr("resolution", FLOAT32, o.str().c_str());
      }
    }
    
    if(eos.get_dimension_size("YDim") > 0){    
      at = das.add_table("lat", new AttrTable);
      at->append_attr("grads_dim", STRING, "\"y\"");
      at->append_attr("grads_mapping", STRING, "\"linear\"");
      {
	std::ostringstream o;
	o << "\"" << eos.get_dimension_size("YDim") << "\"";                  
	at->append_attr("grads_size", STRING, o.str().c_str());
      }
      at->append_attr("units", STRING, "\"degrees_north\"");
      at->append_attr("long_name", STRING, "\"latitude\"");
      {
	std::ostringstream o;
	o << (eos.point_lower / 1000000.0);      
	at->append_attr("minimum", FLOAT32, o.str().c_str());
      }
    
      {
	std::ostringstream o;
	o << (eos.point_upper / 1000000.0);            
	at->append_attr("maximum", FLOAT32, o.str().c_str());
      }
    
      {
	std::ostringstream o;
	o << (eos.gradient_y / 1000000.0);
	at->append_attr("resolution", FLOAT32, o.str().c_str());      
      }
    }    
    DBG(cerr << "<add_dimension_attributes()" << endl);
}

void write_dimension_attributes_swath(DAS & das)
{

  AttrTable *at;
  
  // Let's try IDV without NC_GLOBAL to see it's required.  <hyokyung 2009.02.11. 12:08:52>
  at = das.add_table("NC_GLOBAL", new AttrTable);
  at->append_attr("title", STRING, "\"NASA EOS Swath\"");
  at->append_attr("Conventions", STRING, "\"CF-1.0\"");

  at = das.add_table("lon", new AttrTable);
  at->append_attr("units", STRING, "\"degrees_east\"");
  at->append_attr("long_name", STRING, "\"longitude\"");

  at = das.add_table("lat", new AttrTable);
  at->append_attr("units", STRING, "\"degrees_north\"");
  at->append_attr("long_name", STRING, "\"latitude\"");
  at->append_attr("coordinates", STRING, "\"lon lat\"");
  // For all swaths, insert the coordinates attribute if lat, lon dimension names match.



  
}

#endif
