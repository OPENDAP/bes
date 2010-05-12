// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007, 2009 The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  

///////////////////////////////////////////////////////////////////////////////
/// \file h5das.cc
/// \brief Data attributes processing source
///
/// This file is part of h5_dap_handler, a C++ implementation of the DAP
/// handler for HDF5 data.
///
/// This is the HDF5-DAS that extracts DAS class descriptors converted from
///  HDF5 attribute of an hdf5 data file.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang <myang6@hdfgroup.org>
///
///////////////////////////////////////////////////////////////////////////////
#include "hdf5_handler.h"

/// A global variable that handles HDF-EOS5 files.
HE5Parser eos;

/// A variable for remembering visited paths to break ties if they exist.
HDF5PathFinder paths;

/// EOS parser related variables
struct yy_buffer_state;

/// This function parses Metadata in NASA EOS files.
int he5dasparse(void *arg);

/// Buffer state for NASA EOS metadata scanner
yy_buffer_state *he5das_scan_string(const char *str);

/// Checks whether HDF-EOS5 file has a valid projection for Grid generation.
/// Checks whether this file has "HDF4_DIMGROUP" - a quick way of identifying
/// a converted file by the h4toh5 tool that has dimension scale.
bool has_hdf4_dimgroup;

///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////
void depth_first(hid_t pid, const char *gname, DAS & das)
{
    /// To keep track of soft links.
    static int slinkindex;

    hsize_t nelems;
    
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
#if 0
        char *oname = NULL;
	try {
#endif
	    // Obtain the name of the object.
	    vector<char> oname(oname_size + 1);
	    if (H5Gget_objname_by_idx(pid, (hsize_t) i, &oname[0],
				      (size_t) (oname_size + 1)) < 0) {
		string msg = "hdf5 object name error from: ";
		msg += gname;
		throw InternalErr(__FILE__, __LINE__, msg);
	    }

	    int type = H5Gget_objtype_by_idx(pid, (hsize_t) i);
	    if (type < 0) {
		string msg = "hdf5 object type error from: ";
		msg += gname;
		throw InternalErr(__FILE__, __LINE__, msg);
	    }

	    switch (type) {

            case H5G_GROUP:{
                DBG(cerr << "=depth_first():H5G_GROUP " << oname << endl);
#ifndef CF
                add_group_structure_info(das, gname, &oname[0], true);
#endif
                string full_path_name = string(gname) + string(&oname[0]) + "/";
                // Check if it is converted from h4toh5 tool  and has dimension
                // scale.
                if(full_path_name.find("/HDF4_DIMGROUP/") != string::npos)
                    {
                        has_hdf4_dimgroup = true;
                    }
                
                hid_t cgroup = H5Gopen(pid, full_path_name.c_str());

                if (cgroup < 0) {
                    string msg = "opening hdf5 group failed for ";
                    msg += full_path_name;
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                int num_attr;
                if ((num_attr = H5Aget_num_attrs(cgroup)) < 0) {
                    string msg = "failed to obtain hdf5 attribute in group ";
                    msg += full_path_name;
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                string oid = get_hardlink(cgroup, full_path_name.c_str());

#ifndef CF
                read_objects(das, full_path_name.c_str(), cgroup, num_attr);
#endif
                // Break the cyclic loop created by hard links.
                if (oid == "") {   
                    depth_first(cgroup, full_path_name.c_str(), das);
                } else {
                    // Add attribute table with HARDLINK.
                    AttrTable *at =
                        das.add_table(full_path_name, new AttrTable);
                    at->append_attr("HDF5_HARDLINK", STRING,
                                    paths.get_name(oid));
                }
                
                if(H5Gclose(cgroup) < 0){
                    throw InternalErr(__FILE__, __LINE__,
                                      "H5Gclose() failed.");
                }
                break;
            } // case H5G_GROUP

            case H5G_DATASET:{
                DBG(cerr << "=depth_first():H5G_DATASET " << oname <<
                    endl);
#ifndef CF
                add_group_structure_info(das, gname, &oname[0], false);
#endif
                string full_path_name = string(gname) + string(&oname[0]);
                hid_t dset;
                // Open the dataset
                if ((dset = H5Dopen(pid, full_path_name.c_str())) < 0) {
                    string msg = "unable to open hdf5 dataset of group ";
                    msg += gname;
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                // Obtain number of attributes in this dataset.
                int num_attr;
                if ((num_attr = H5Aget_num_attrs(dset)) < 0) {
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

                if (H5Dclose(dset) < 0){
		   throw InternalErr(__FILE__, __LINE__, 
                                     "Could not close the dataset.");
		}
                break;
            }                   // case H5G_DATASET

            case H5G_TYPE:
		break;
#ifndef CF
            case H5G_LINK:
		slinkindex++;
		get_softlink(das, pid, &oname[0], slinkindex);
		break;
#endif

            default:
		break;
	    }
#if 0
	    if( oname ) delete[]oname;
	} // try
	catch (...) {
	    // if a memory allocation exception is thrown creating
	    // oname it is caught here, meaning oname is null. pwest
	    // Mar 18, 2009
	    if( oname ) delete[] oname;
	    throw;
	}
#endif
    } //  for (int i = 0; i < nelems; i++)

    DBG(cerr << "<depth_first():" << gname << endl);
}

///////////////////////////////////////////////////////////////////////////////
/// \fn print_attr(hid_t type, int loc, void *sm_buf)
/// will get the printed representation of an attribute.
///
/// This function is based on netcdf-dods server.
///
/// \param type  HDF5 data type id
/// \param loc    the number of array number
/// \param sm_buf pointer to an attribute
/// \return a char * to newly allocated memory, the caller must call delete []
/// \todo This probably needs to be re-considered! 
/// \todo Needs to be re-written. 
///////////////////////////////////////////////////////////////////////////////
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
                // Display byte in numerical form. This is for Aura file.
                // 
                // This generates an attribute like "Byte _FillValue -127".
                // It can cause IDV to crash since Java OPeNDAP expects
                // Byte value > 0.
                //
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
            } else if (H5Tget_size(type) == 0){
		throw InternalErr(__FILE__, __LINE__, "H5Tget_size() failed.");
	    }
            break;
        }

        case H5T_STRING: {
            int str_size = H5Tget_size(type);
	    if (str_size == 0){
		throw InternalErr(__FILE__, __LINE__, "H5Tget_size() failed.");
	    }
            DBG(cerr << "=print_attr(): H5T_STRING sm_buf=" << (char *) sm_buf
                << " size=" << str_size << endl);
            char *buf = NULL;
            // This try/catch block is here to protect the allocation of buf.
            try {
                buf = new char[str_size + 1];
                strncpy(buf, (char *) sm_buf, str_size);
                buf[str_size] = '\0';
                rep = new char[str_size + 3];
                snprintf(rep, str_size + 3, "%s", buf);
                rep[str_size + 2] = '\0';
                delete[] buf; buf = 0;
            }
            catch (...) {
                if( buf ) delete[] buf;
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
	if( rep ) delete[] rep;
	throw;
    }

    return rep;
}

#ifdef CF
///////////////////////////////////////////////////////////////////////////////
/// \fn GET_NAME(x)
/// For CF we have to use a special filter to get the atribute name, while
/// for a non-CF-aware build we just use the name. This is a macro, not a
/// real function.
///////////////////////////////////////////////////////////////////////////////
#define GET_NAME(x) eos.get_CF_name((x))
#else
#define GET_NAME(x) (x)
#endif

///////////////////////////////////////////////////////////////////////////////
// \fn write_metadata(DAS & das, const string & varname)
/// parses the metadata string and generates a structured attribute from it.
///
/// \param das DAS object: reference
/// \param varname absolute name of either a dataset or a group
/// \return true if parsed successfully
///////////////////////////////////////////////////////////////////////////////
bool write_metadata(DAS & das, const string & varname)
{

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
                he5das_scan_string(eos.metadata_Struct);

                if (he5dasparse(static_cast < void *>(&arg)) != 0
                    || arg.status() == false){
                    cerr << "HDF-EOS StructMetdata parse error!\n";
                    return false;
                }
                return true;
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
                he5das_scan_string(eos.metadata_core);

                if (he5dasparse(static_cast < void *>(&arg)) != 0
                    || arg.status() == false){
                    cerr << "HDF-EOS coremetadata parse error!\n";
                    return false;
                }
                return true;
            }
        }


        if (varname.find("CoreMetadata") != string::npos) {
            if (!eos.bmetadata_Core) {
                eos.bmetadata_core = true;
                AttrTable *at = das.get_table(varname);
                if (!at)
                    at = das.add_table(varname, new AttrTable);
                parser_arg arg(at);
                DBG(cerr << eos.metadata_Core << endl);
                he5das_scan_string(eos.metadata_Core);

                if (he5dasparse(static_cast < void *>(&arg)) != 0
                    || arg.status() == false){
                    cerr << "HDF-EOS CoreMetadata parse error!\n";
                    return false;
                }
                return true;
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
                he5das_scan_string(eos.metadata_product);

                if (he5dasparse(static_cast < void *>(&arg)) != 0
                    || arg.status() == false){
                    cerr << "HDF-EOS productmetadata parse error!\n";
                    return false;
                }
                return true;
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
                he5das_scan_string(eos.metadata_Archived);

                if (he5dasparse(static_cast < void *>(&arg)) != 0
                    || arg.status() == false){
                    cerr << "HDF-EOS ArchivedMetadata parse error!\n";
                    return false;
                }
                return true;
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
                he5das_scan_string(eos.metadata_subset);

                if (he5dasparse(static_cast < void *>(&arg)) != 0
                    || arg.status() == false){
                    cerr << "HDF-EOS subsetmetadata parse error!\n";
                    return false;
                }
                return true;
            }
        }
#endif                          // #ifndef CF    	
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// \fn read_objects(DAS & das, const string & varname, hid_t oid, int num_attr)
/// will fill in attributes of a dataset or a group into one DAS table.
///
/// \param das DAS object: reference
/// \param varname absolute name of either a dataset or a group
/// \param oid dset
/// \param num_attr number of attributes.
/// \return nothing
/// \see get_attr_info(hid_t dset, int index,
///                    DSattr_t * attr_inst_ptr,int *ignoreptr, char *error)
/// \see get_dap_type()
///////////////////////////////////////////////////////////////////////////////
void read_objects(DAS & das, const string & varname, hid_t oid, int num_attr) {

    // Obtain variable names. Put this variable name into das table
    // regardless of the existing attributes in this object.
    DBG(cerr << ">read_objects():"
        << "varname=" << varname << " id=" << oid << endl);
#ifdef NASA_EOS_META
    // Generate the structured attribute using the metadata parser.
    if(write_metadata(das, varname))
        return;
#endif
    // Prepare a variable for full path attribute.
    string hdf5_path = HDF5_OBJ_FULLPATH;

    // Rewrote to use C++ strings 3/2008 jhrg
    string newname;

    if(!has_hdf4_dimgroup){
        newname = varname;
    }
    else{
        // This is necessry for GrADS which doesn't like '/' character
        // in variable name.
        newname = get_short_name_dimscale(varname); 
    }

#ifdef CF
    if(eos.is_valid() && eos.get_swath_variable(varname)) {
        // Rename the variable if necessary.
        newname = eos.get_CF_name((char*) varname.c_str());
#ifdef SHORT_PATH
        // If it is not renamed, shorten the swath variable name
        // if --enable-short-path configuration option is enabled.
        if(newname == varname){
            newname = eos.get_short_name(varname);
        }
#endif        
        DBG(cerr << "newname: " << newname << endl);
    }
#ifdef SHORT_PATH    
    if (eos.is_valid() && eos.get_grid_variable(varname)) {
        newname = eos.get_short_name(varname);
    }
#endif    
#endif  
    
    if(newname.empty()){	  
        return; // Ignore attribute generation for /HDF4_DIMGROUP/.
    }
    
    DBG(cerr << "=read_objects(): new variable name=" << newname << endl);
    
    AttrTable *attr_table_ptr = das.get_table(newname);
    if (!attr_table_ptr) {
        DBG(cerr << "=read_objects(): adding a table with name " << newname
            << endl);
        attr_table_ptr = das.add_table(newname, new AttrTable);
    }
#ifndef CF
    attr_table_ptr->append_attr(hdf5_path.c_str(), STRING, varname);
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
                    attr_table_ptr->append_attr("name", get_dap_type(ty_id),
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
                                                get_dap_type(ty_id),
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
                                        get_dap_type(ty_id), print_rep);

                        tempvalue = tempvalue + elesize;
                        DBG(cerr << "tempvalue=" << tempvalue
                            << "elesize=" << elesize << endl);
                        delete[]print_rep; print_rep = 0;
                    }           // for (int sizeindex = 0; ...
                }               // for (int dim = 0; ...
            }			// if attr_inst.ndims != 0
	    delete[] value; value = 0;
            
            if(H5Aclose(attr_id) < 0){
                    throw InternalErr(__FILE__, __LINE__,
				      "unable to close attibute id");
                
            }
	}		// for (int j = 0; j < num_attr; j++)
    }			// try - protects print_rep and value
    catch(...) {
	if( print_rep ) delete[] print_rep;
	if( value ) delete[] value;
	throw;
    }
#ifdef CF
    if(eos.get_swath_variable(varname) && 
       eos.get_swath_coordinate_dimension_match(varname)){
        attr_table_ptr->append_attr("coordinates", STRING, 
                        eos.get_swath_coordinate_attribute());
    }

    if(newname == "lon" || newname == "lat"){
        write_swath_coordinate_unit_attribute(attr_table_ptr, newname);
    }

#endif
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
//////////////////////////////////////////////////////////////////////////
void find_gloattr(hid_t file, DAS & das)
{
    DBG(cerr << ">find_gloattr()" << endl);
    has_hdf4_dimgroup = false;  // Reset it all the time.
#ifdef CF
    if(eos.is_valid() && eos.valid_projection){
        write_grid_global_attribute(das);
        write_grid_coordinate_variable_attribute(das);
    }
    if(eos.get_swath()){
        write_swath_global_attribute(das);
    }
#endif

    hid_t root = H5Gopen(file, "/");
    try {
	if (root < 0)
	    throw InternalErr(__FILE__, __LINE__,
			      "unable to open HDF5 root group");
#ifndef CF
	das.add_table("HDF5_ROOT_GROUP", new AttrTable);
#endif

	get_hardlink(root, "/");    
	int num_attrs = H5Aget_num_attrs(root);
	if (num_attrs < 0)
	    throw InternalErr(__FILE__, __LINE__,
			      "unable to get attribute number");

	if (num_attrs == 0) {
	    if(H5Gclose(root) < 0){
		throw InternalErr(__FILE__, __LINE__,
                                  "Could not close the group.");
	    }
	    DBG(cerr << "<find_gloattr():no attributes" << endl);
	    return;
	}

        read_objects(das, "H5_GLOBAL", root, num_attrs);

	DBG(cerr << "=find_gloattr(): H5Gclose()" << endl);
	if(H5Gclose(root) < 0){
	   throw InternalErr(__FILE__, __LINE__, "Could not close the group.");
	}
	DBG(cerr << "<find_gloattr()" << endl);
    }
    catch (...) {
	if(H5Gclose(root) < 0){	
	   throw InternalErr(__FILE__, __LINE__, "Could not close the group.");
	}
	throw;
    }
}

///////////////////////////////////////////////////////////////////////////////
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
/// \todo This function may be removed. 
///////////////////////////////////////////////////////////////////////////////
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
	    throw InternalErr(__FILE__, __LINE__, "unable to get link value");
	}
        attr_table_ptr->append_attr(oname, STRING, buf);
        delete[]buf;
    }
    catch (...) {
	delete[] buf;
	throw;
    }
}

///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////
string get_hardlink(hid_t pgroup, const string & oname) {
    
    DBG(cerr << ">get_hardlink():" << oname << endl);

    // Get the target information at statbuf.
    H5G_stat_t statbuf;
    if (H5Gget_objinfo(pgroup, oname.c_str(), 0, &statbuf) < 0){
	throw InternalErr(__FILE__, __LINE__, "H5Gget_objinfo() failed.");
    }

    if (statbuf.nlink >= 2) {
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

///////////////////////////////////////////////////////////////////////////////
/// \fn read_comments(DAS & das, const string & varname, hid_t oid)
/// will fill in attributes of a group's comment into DAS table.
///
/// \param das DAS object: reference
/// \param varname absolute name of an object
/// \param oid object id
/// \return nothing
///////////////////////////////////////////////////////////////////////////////
void read_comments(DAS & das, const string & varname, hid_t oid)
{
    // Borrowed from the dump_comment(hid_t obj_id) function in h5dump.c.
    char comment[max_str_len - 2];
    comment[0] = '\0';
    if (H5Gget_comment(oid, ".", sizeof(comment), comment) < 0){
	throw InternalErr(__FILE__, __LINE__,
                          "Could not retrieve the comment.");
    }
    if (comment[0]) {
        // Insert this comment into the das table.
        AttrTable *at = das.get_table(varname);
        if (!at)
            at = das.add_table(varname, new AttrTable);
        at->append_attr("HDF5_COMMENT", STRING, comment);

    }
}

///////////////////////////////////////////////////////////////////////////////
/// \fn add_group_structure_info(DAS & das, const char* gname, char* oname,
/// bool is_group)
/// will insert group information in a structure format into DAS table.
///
/// This function adds a special attribute called "HDF5_ROOT_GROUP" if the \a
/// gname is "/". If \a is_group is true, it keeps appending new attribute
/// table called \a oname under the \a gname path. If \a is_group is false,
/// it appends a string attribute called \a oname. For details, see the
/// HDF5-DAP2 Mapping Technical Note from [1].
///
/// [1] http://www.hdfgroup.org/projects/opendap/opendap_docs.html
///
/// \param das DAS object: reference
/// \param gname absolute group pathname of an object
/// \param oname name of object
/// \param is_group indicates whether it's a dataset or group
/// \return nothing
///////////////////////////////////////////////////////////////////////////////
void add_group_structure_info(DAS & das, const char *gname, char *oname,
                              bool is_group)
{

    string search("/");
    string replace(".");
    string::size_type pos = 1;

    if(gname == NULL){
        throw InternalErr(__FILE__, __LINE__,
                          "Got a NULL group name.");
    }
    
    string full_path = string(gname);
    // Cut the last '/'.
    while ((pos = full_path.find(search, pos)) != string::npos) {
        full_path.replace(pos, search.size(), replace);
        pos++;
    }
    if (strncmp(gname, "/", sizeof(gname)) == 0) {
        full_path.replace(0, 1, "HDF5_ROOT_GROUP");
    }
    else {
        full_path.replace(0, 1, "HDF5_ROOT_GROUP.");
        full_path = full_path.substr(0, full_path.length() - 1);
    }

    DBG(cerr << full_path << endl);

    AttrTable *at = das.get_table(full_path);
    if(at == NULL){
        throw InternalErr(__FILE__, __LINE__,
                           "Failed to add group structure information for "
                          + full_path
                          + " attribute table."
                          + "This happens when a group name has . character.");
    }

    if (is_group) {
        at->append_container(oname);
    }
    else {
        at->append_attr("Dataset", "String", oname);
    }
}

#ifdef CF
///////////////////////////////////////////////////////////////////////////////
/// \fn write_grid_global_attribute(DAS & das)
/// will put pseudo global attributes for CF convention compatibility.
///
/// This function is provided as an example for NASA AURA data only.
/// You may modify this to add custom attributes to make the output
/// CF-convention compliant.
///
/// For details,
/// please refer to the technical note "Using DAP Clients to Visualize
/// HDF-EOS5 Grid Data" from [2].
/// 
/// [2] http://www.hdfgroup.org/projects/opendap/publications/cf.html
/// 
/// \param das DAS object: reference
///////////////////////////////////////////////////////////////////////////////
void write_grid_global_attribute(DAS & das)
{
    DBG(cerr << ">write_grid_global_attributes()" << endl);
    AttrTable *at;
    
    at = das.add_table("NC_GLOBAL", new AttrTable);
    at->append_attr("title", STRING, "NASA EOS Aura Grid");
    at->append_attr("Conventions", STRING, "CF-1.4");
    at->append_attr("dataType", STRING, "Grid");
    
    DBG(cerr << "<write_grid_global_attributes()" << endl);
}

///////////////////////////////////////////////////////////////////////////////
/// \fn write_grid_coordinate_variable_attribute(DAS & das)
/// inserts pseudo attributes for coordinate variables to meet the CF
/// convention.
///
/// This function is provided as an example for NASA AURA data only.
/// You may modify this to add custom attributes for coodinate variables
/// to make the output compliant to CF-convention. For details,
/// please refer to the technical note "Using DAP Clients to Visualize
/// HDF-EOS5 Grid Data" from [2].
/// 
/// [2] http://www.hdfgroup.org/projects/opendap/publications/cf.html
/// 
/// \param das DAS object: reference
/// \remarks This is necessary for CF compatibility only. The time and lev
/// coordinate variables have fake data so we put fake units.
///////////////////////////////////////////////////////////////////////////////
void write_grid_coordinate_variable_attribute(DAS & das)
{
    DBG(cerr << ">write_grid_coordinate_variable_attribute()" << endl);
    AttrTable *at;
    vector < string > tokens;

    if(eos.get_grid_lon() > 0){
        at = das.add_table("lon", new AttrTable);
        at->append_attr("grads_dim", STRING, "x");
        at->append_attr("grads_mapping", STRING, "linear");
        {
            std::ostringstream o;
            o << eos.get_grid_lon();
            at->append_attr("grads_size", STRING, o.str().c_str());
        }
        at->append_attr("units", STRING, "degrees_east");
        at->append_attr("long_name", STRING, "longitude");
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
    
    if(eos.get_grid_lat() > 0){    
        at = das.add_table("lat", new AttrTable);
        at->append_attr("grads_dim", STRING, "y");
        at->append_attr("grads_mapping", STRING, "linear");
        {
            std::ostringstream o;
            o << eos.get_grid_lat();
            at->append_attr("grads_size", STRING, o.str().c_str());
        }
        at->append_attr("units", STRING, "degrees_north");
        at->append_attr("long_name", STRING, "latitude");
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

    if(eos.get_grid_lev() > 0){    
        at = das.add_table("lev", new AttrTable);
        at->append_attr("units", STRING, "hPa");
        at->append_attr("long_name", STRING, "fake pressure level dimension");
        at->append_attr("positive", STRING, "down");
    }

    if(eos.get_grid_time() > 0){    
        at = das.add_table("time", new AttrTable);
        at->append_attr("long_name", STRING, "fake time dimension");
        at->append_attr("units", STRING, "hour"); 
    }
    
    DBG(cerr << "<write_grid_coordinate_variable_attribute()" << endl);
}

///////////////////////////////////////////////////////////////////////////////
/// \fn write_swath_global_attribute(DAS & das)
/// will put pseudo global attributes for CF convention
///
/// This function is provided as an example for NASA AURA data only.
/// You may  modify this to add custom attributes  to make the handler output
/// compliant to CF-convention. For details, please refer to the technical note
/// "Using DAP Clients to Visualize HDF-EOS5 Grid Data" from [2].
/// 
/// [2] http://www.hdfgroup.org/projects/opendap/publications/cf.html
/// 
/// \param das DAS object: reference
/// \see write_grid_global_attribute()
///////////////////////////////////////////////////////////////////////////////
void write_swath_global_attribute(DAS & das)
{
    AttrTable *at;
    at = das.add_table("NC_GLOBAL", new AttrTable);
    at->append_attr("title", STRING, "NASA EOS Aura Swath");
    at->append_attr("Conventions", STRING, "CF-1.4");

}

///////////////////////////////////////////////////////////////////////////////
/// \fn write_swath_coordinate_unit_attribute(AttrTable* at,
///                                           string varname)
/// inserts pseudo attributes for coordinate variables to meet the CF
/// convention.
///
/// This function is provided as an example for NASA AURA Swath data only.
/// NASA AURA swath files have either 1-D or 2-D lat / lon dataset.
/// Since CF-convention requires units and standard name on them,
/// we add the new attributes to make the output compatible.
/// 
/// \param at AttrTable of \a varname
/// \param varname dataset name - either lat or lon
/// \see write_grid_coordinate_variable_attribute()
///////////////////////////////////////////////////////////////////////////////
void write_swath_coordinate_unit_attribute(AttrTable* at, string varname)
{

    if(varname.find("lon") != string::npos){
        at->del_attr("units");
        at->append_attr("units",STRING, "degrees_east");
        at->append_attr("standard_name",STRING, "longitude");
    }

    if(varname.find("lat") != string::npos){
        at->del_attr("units");
        at->append_attr("units",STRING, "degrees_north");
        at->append_attr("standard_name",STRING, "latitude");
    }

}
#endif
