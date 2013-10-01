// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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


/// A variable for remembering visited paths to break cyclic HDF5 groups. 
HDF5PathFinder paths;


///////////////////////////////////////////////////////////////////////////////
/// \fn depth_first(hid_t pid, const char *gname, DAS & das)
/// depth first traversal of hdf5 file attributes.
///
/// This function will walk through an hdf5 group using depth-
/// first approach to obtain all the group and dataset attributes
/// of an hdf5 file.
/// During the process of the depth first search, DAS table will be filled.
/// In case of errors, an exception will be thrown.
///
/// \param pid    dataset id(group id)
/// \param gname  group name(absolute name from the root group)
/// \param das    reference of DAS object
/// \return void
///////////////////////////////////////////////////////////////////////////////
void depth_first(hid_t pid, const char *gname, DAS & das)
{
    /// To keep track of soft links.
     int slinkindex = 0;

    // Although HDF5 comments are rarely used, we still keep this
    // function.
    read_comments(das, gname, pid);

    H5G_info_t g_info;
    hsize_t nelems;

    if (H5Gget_info(pid,&g_info) <0) {
       string msg =
            "h5_das handler: unable to obtain the HDF5 group info. for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    nelems = g_info.nlinks;

    ssize_t oname_size;
    for (hsize_t i = 0; i < nelems; i++) {

	// Query the length of object name.
	oname_size =
            H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,NULL,
                (size_t)DODS_NAMELEN, H5P_DEFAULT); 

	if (oname_size <= 0) {
	    string msg = "hdf5 object name error from: ";
	    msg += gname;
	    throw InternalErr(__FILE__, __LINE__, msg);
	}
	// Obtain the name of the object.
	vector<char> oname(oname_size + 1);
        if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,&oname[0],
                (size_t)(oname_size+1), H5P_DEFAULT)<0){
	    string msg = "hdf5 object name error from: ";
	    msg += gname;
	    throw InternalErr(__FILE__, __LINE__, msg);
	}

        // Check if it is the hard link or the soft link
        H5L_info_t linfo;
        if (H5Lget_info(pid,&oname[0],&linfo,H5P_DEFAULT)<0) {
            string msg = "hdf5 link name error from: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // This is the soft link.
        if (linfo.type == H5L_TYPE_SOFT){ 
            slinkindex++;
            size_t val_size = linfo.u.val_size;
            get_softlink(das, pid, gname,&oname[0], slinkindex,val_size);
            continue;
        }

        // Obtain the object type
        H5O_info_t oinfo;
        if (H5Oget_info_by_idx(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
            string msg = "Cannot obtain the object info ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        H5O_type_t obj_type = oinfo.type;

	switch (obj_type) {

	case H5O_TYPE_GROUP: {

	    DBG(cerr << "=depth_first():H5G_GROUP " << &oname[0] << endl);

            // This function will store the HDF5 group hierarchy into an DAP attribute. 
	    add_group_structure_info(das, gname, &oname[0], true);

	    string full_path_name = string(gname) + string(&oname[0]) + "/";

	    hid_t cgroup = H5Gopen(pid, full_path_name.c_str(),H5P_DEFAULT);
	    if (cgroup < 0) {
		string msg = "opening hdf5 group failed for ";
		msg += full_path_name;
		throw InternalErr(__FILE__, __LINE__, msg);
	    }

	    int num_attr;

            // Get the object info
            H5O_info_t obj_info;
            if (H5Oget_info(cgroup, &obj_info) <0) { 
                H5Gclose(cgroup);
		string msg = "Obtaining the hdf5 group info. failed for ";
		msg += full_path_name;
		throw InternalErr(__FILE__, __LINE__, msg);
            }   

            // Obtain the number of attributes
            num_attr = obj_info.num_attrs;
            if (num_attr < 0 ) {
               H5Gclose(cgroup);
               string msg = "Fail to get the number of attributes for group ";
               msg += full_path_name;
               throw InternalErr(__FILE__, __LINE__,msg);
            }

            // Read all attributes in this group and map to DAS.
	    read_objects(das, full_path_name.c_str(), cgroup, num_attr);

            // Check if this group has been visited by using the hardlink
	    string oid = get_hardlink(cgroup, full_path_name.c_str());

	    // Break the cyclic loop created by hard links.
	    if (oid.empty()) {// The group has never been visited, go to the next level.
		depth_first(cgroup, full_path_name.c_str(), das);
	    }
	    else {

		// This group has been visited.  
                // Add the attribute table with the attribute name as HDF5_HARDLINK.
                // The attribute value is the name of the group when it is first visited.
		AttrTable *at = das.get_table(full_path_name);
                if(!at){
		  at = das.add_table(full_path_name, new AttrTable);
                }

                // Note that "paths" is a global object to find the visited path. 
                // It is defined at the beginning of this source code file.
		at->append_attr("HDF5_HARDLINK", STRING, paths.get_name(oid));
	    }

	    if (H5Gclose(cgroup) < 0) {
		throw InternalErr(__FILE__, __LINE__, "H5Gclose() failed.");
	    }
	    break;
	} // case H5G_GROUP

	case H5O_TYPE_DATASET: {

	    DBG(cerr << "=depth_first():H5G_DATASET " << &oname[0] <<
		    endl);

            // This function will store the HDF5 group hierarchy into an DAP attribute.
	    add_group_structure_info(das, gname, &oname[0], false);

	    string full_path_name = string(gname) + string(&oname[0]);
	    hid_t dset;

	    // Open the dataset
	    if ((dset = H5Dopen(pid, full_path_name.c_str(),H5P_DEFAULT)) < 0) {
		string msg = "unable to open the hdf5 dataset of the group ";
		msg += gname;
		throw InternalErr(__FILE__, __LINE__, msg);
	    }

            // Get the object info
	    int num_attr;
            H5O_info_t obj_info;
            if (H5Oget_info(dset, &obj_info) <0) { 
                H5Dclose(dset);
		string msg = "Obtaining the info. failed for the dataset ";
		msg += full_path_name;
		throw InternalErr(__FILE__, __LINE__, msg);
            }   

            // Obtain the number of attributes
            num_attr = obj_info.num_attrs;
            if (num_attr < 0 ) {
               H5Dclose(dset);
               string msg = "Fail to get the number of attributes for dataset ";
               msg += full_path_name;
               throw InternalErr(__FILE__, __LINE__,msg);
            }

            // Read all attributes in this dataset and map to DAS.
	    read_objects(das, full_path_name, dset, num_attr);

	    string oid = get_hardlink(dset, full_path_name);

	    // Break the cyclic loop created by hard links

            // If this HDF5 dataset has been visited, 
            // Add the DAS table with the attribute name as HDF5_HARDLINK.
            // The attribute value is the name of the HDF5 dataset when it is first visited.
	    if (!oid.empty()) {
		// Add attribute table with HARDLINK
                AttrTable *at = das.get_table(full_path_name);
                if(!at) {
		  at = das.add_table(full_path_name, new AttrTable);
                }

                // Note that "paths" is a global object to find the visited path. 
                // It is defined at the beginning of this source code file.
		at->append_attr("HDF5_HARDLINK", STRING, paths.get_name(oid));
	    }

	    if (H5Dclose(dset) < 0) {
		throw InternalErr(__FILE__, __LINE__, "Could not close the dataset.");
	    }
	    break;
	} // case H5G_DATASET

	case H5O_TYPE_NAMED_DATATYPE:
            // ignore the named datatype
	    break;

	default:
	    break;
	}
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
/// \todo Due to the priority of the handler work, this function will not be 
/// \todo re-written in this re-engineering process. KY 2011-Nov. 14th
///////////////////////////////////////////////////////////////////////////////
string print_attr(hid_t type, int loc, void *sm_buf) {
//static char *print_attr(hid_t type, int loc, void *sm_buf) {
    union {
        char *tcp;
        short *tsp;
        unsigned short *tusp;
        int *tip;
        long *tlp;
        float *tfp;
        double *tdp;
    } gp;

    //char *rep = NULL;		// This holds the return value
    vector<char> rep;

    //try {
	switch (H5Tget_class(type)) {

        case H5T_INTEGER: {
            // change void pointer into the corresponding integer datatype.
            // 32 should be long enough to hold one integer and one
            // floating point number.
            //rep = new char[32];
            //memset(rep, 0, 32);
             rep.resize(32);

            if (H5Tequal(type, H5T_STD_U8BE) || H5Tequal(type, H5T_STD_U8LE)
                || H5Tequal(type, H5T_NATIVE_UCHAR)) {
                gp.tcp = (char *) sm_buf;
                unsigned char tuchar = *(gp.tcp + loc);
                // represent uchar with numerical form since for NASA aura
                // files, type of missing value is unsigned char. ky
                // 2007-5-4
                snprintf(&rep[0], 32, "%u", tuchar);
            }

            else if (H5Tequal(type, H5T_STD_U16BE)
                     || H5Tequal(type, H5T_STD_U16LE)
                     || H5Tequal(type, H5T_NATIVE_USHORT)) {
                gp.tusp = (unsigned short *) sm_buf;
                snprintf(&rep[0], 32, "%hu", *(gp.tusp + loc));
            }

            else if (H5Tequal(type, H5T_STD_U32BE)
                     || H5Tequal(type, H5T_STD_U32LE)
                     || H5Tequal(type, H5T_NATIVE_UINT)) {

                gp.tip = (int *) sm_buf;
                snprintf(&rep[0], 32, "%u", *(gp.tip + loc));
            }

            else if (H5Tequal(type, H5T_STD_U64BE)
                     || H5Tequal(type, H5T_STD_U64LE)
                     || H5Tequal(type, H5T_NATIVE_ULONG)
                     || H5Tequal(type, H5T_NATIVE_ULLONG)) {

                gp.tlp = (long *) sm_buf;
                snprintf(&rep[0], 32, "%lu", *(gp.tlp + loc));
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
                snprintf(&rep[0], 32, "%d", *(gp.tcp + loc));
            }

            else if (H5Tequal(type, H5T_STD_I16BE)
                     || H5Tequal(type, H5T_STD_I16LE)
                     || H5Tequal(type, H5T_NATIVE_SHORT)) {

                gp.tsp = (short *) sm_buf;
                snprintf(&rep[0], 32, "%hd", *(gp.tsp + loc));
            }

            else if (H5Tequal(type, H5T_STD_I32BE)
                     || H5Tequal(type, H5T_STD_I32LE)
                     || H5Tequal(type, H5T_NATIVE_INT)) {

                gp.tip = (int *) sm_buf;
                snprintf(&rep[0], 32, "%d", *(gp.tip + loc));
            }

            else if (H5Tequal(type, H5T_STD_I64BE)
                     || H5Tequal(type, H5T_STD_I64LE)
                     || H5Tequal(type, H5T_NATIVE_LONG)
                     || H5Tequal(type, H5T_NATIVE_LLONG)) {

                gp.tlp = (long *) sm_buf;
                snprintf(&rep[0], 32, "%ld", *(gp.tlp + loc));
            }

            break;
        }

        case H5T_FLOAT: {
            rep.resize(32);
            char gps[30];

            if (H5Tget_size(type) == 4) {
                
                // Represent the float number.
                // Some space may be wasted. But it is okay.
                gp.tfp = (float *) sm_buf;
                snprintf(gps, 30, "%.10g", *(gp.tfp + loc));
                int ll = strlen(gps);

                // Add the dot to assure this is a floating number
                if (!strchr(gps, '.') && !strchr(gps, 'e'))
                    gps[ll++] = '.';

                gps[ll] = '\0';
                snprintf(&rep[0], 32, "%s", gps);
            } 
            else if (H5Tget_size(type) == 8) {

                gp.tdp = (double *) sm_buf;
                snprintf(gps, 30, "%.17g", *(gp.tdp + loc));
                int ll = strlen(gps);
                if (!strchr(gps, '.') && !strchr(gps, 'e'))
                    gps[ll++] = '.';
                gps[ll] = '\0';
                snprintf(&rep[0], 32, "%s", gps);
            } 
            else if (H5Tget_size(type) == 0){
		throw InternalErr(__FILE__, __LINE__, "H5Tget_size() failed.");
	    }
            break;
        }

        case H5T_STRING: {
            int str_size = H5Tget_size(type);
            if(H5Tis_variable_str(type) == true) 
                //    cerr <<"variable length string "<<endl;
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
                //rep = new char[str_size + 3];
                rep.resize(str_size+3);
                snprintf(&rep[0], str_size + 3, "%s", buf);
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
	    break;
	} // switch(H5Tget_class(type))
    //} // try
    //catch (...) {
//	if( rep ) delete[] rep;
//	throw;
 //   }

    string rep_str(rep.begin(),rep.end());
    return rep_str;
}


///////////////////////////////////////////////////////////////////////////////
// \fn read_objects(DAS & das, const string & varname, hid_t oid, int num_attr)
/// will fill in attributes of a dataset or a group into one DAS table.
///
/// \param das DAS object: reference
/// \param varname absolute name of an HDF5 dataset or an HDF5 group  
/// \param oid  HDF5 object id(a handle)
/// \param num_attr number of attributes.
/// \return nothing
/// \see get_attr_info(hid_t dset, int index,
///                    DSattr_t * attr_inst_ptr,int *ignoreptr, char *error)
/// \see get_dap_type()
///////////////////////////////////////////////////////////////////////////////
void read_objects(DAS & das, const string & varname, hid_t oid, int num_attr)
{

    DBG(cerr << ">read_objects():"
	    << "varname=" << varname << " id=" << oid << endl);

    // Prepare a variable for full path attribute.
    string hdf5_path = HDF5_OBJ_FULLPATH;

    // Obtain the DAS table of which the name is the variable name.
    // If not finding the table, add a table of which the name is the variable name.
    AttrTable *attr_table_ptr = das.get_table(varname);
    if (!attr_table_ptr) {
	DBG(cerr << "=read_objects(): adding a table with name " << varname
		<< endl);
	attr_table_ptr = das.add_table(varname, new AttrTable);
    }

    // Add a DAP attribute that stores the HDF5 absolute path 
    attr_table_ptr->append_attr(hdf5_path.c_str(), STRING, varname);

    // Check the number of attributes in this HDF5 object and
    // put HDF5 attribute information into the DAS table.
    //char *print_rep = NULL;
    string print_rep;
    vector<char>temp_buf;

    //try {
        bool ignore_attr = false;
        hid_t attr_id;
	for (int j = 0; j < num_attr; j++) {

	    // Obtain attribute information.
	    DSattr_t attr_inst;

            // Ignore the attributes of which the HDF5 datatype 
            // cannot be mapped to DAP2. The ignored attribute datatypes can be found 
            // at function get_attr_info in h5get.cc.
	    attr_id = get_attr_info(oid, j, &attr_inst, &ignore_attr);
	    if (ignore_attr) continue;

	    // Since HDF5 attribute may be in string datatype, it must be dealt
	    // properly. Get data type.
	    hid_t ty_id = attr_inst.type;
	    string dap_type = get_dap_type(ty_id);
	    string attr_name = attr_inst.name;


            // We have to handle variable length string differently. 
            if (H5Tis_variable_str(attr_inst.type)) { 

                DBG(cerr <<"attribute name " << attr_name <<endl);
                DBG(cerr <<"attribute size " <<attr_inst.need <<endl);
                DBG(cerr <<"attribute type size " <<(int)(H5Tget_size(attr_inst.type))<<endl); 

                hid_t temp_space_id = H5Aget_space(attr_id);
                 DBG(cerr <<"attribute calculated size "<<(int)(H5Tget_size(attr_inst.type)) *(int)(H5Sget_simple_extent_npoints(temp_space_id)) <<endl);

                // Variable length string attribute values only store pointers of the actual string value.
                temp_buf.resize((size_t)attr_inst.need);
                
	        if (H5Aread(attr_id, ty_id, &temp_buf[0]) < 0) {
                    H5Sclose(temp_space_id);
                    H5Aclose(attr_id);
	       	    throw InternalErr(__FILE__, __LINE__, "unable to read HDF5 attribute data");
                }

                char *temp_bp;
                temp_bp = &temp_buf[0];
                char* onestring;
                for (unsigned int temp_i = 0; temp_i <attr_inst.nelmts; temp_i++) {

                    // This line will assure that we get the real variable length string value.
                    onestring =*(char **)temp_bp;

                    // Change the C-style string to C++ STD string just for easy appending the attributes in DAP.
                    if (onestring !=NULL) {
                        string tempstring(onestring);
                        // cerr <<"temp_string attr "<<tempstring <<endl;
                        attr_table_ptr->append_attr(attr_name, dap_type, tempstring);
                    }

                    // going to the next value.
                    temp_bp +=H5Tget_size(attr_inst.type);
                }
                if (temp_buf.empty() != true) {
                   // Reclaim any VL memory if necessary.
                   H5Dvlen_reclaim(attr_inst.type,temp_space_id,H5P_DEFAULT,&temp_buf[0]);
                   temp_buf.clear();
                }
                H5Sclose(temp_space_id);
            }
            else {

                vector<char> value;
                value.resize(attr_inst.need + sizeof(char));
	        DBG(cerr << "arttr_inst.need=" << attr_inst.need << endl);
  
	        // Read HDF5 attribute data.
	        if (H5Aread(attr_id, ty_id, (void *) (&value[0])) < 0) {
	           // value is deleted in the catch block below so
		   // shouldn't be deleted here. pwest Mar 18, 2009
		   throw InternalErr(__FILE__, __LINE__, "unable to read HDF5 attribute data");
	        }
	        DBG(cerr << "H5Aread(" << attr_inst.name << ")=" << value << endl);

	        // For scalar data, just read data once.
	        if (attr_inst.ndims == 0) {
		    for (int loc = 0; loc < (int) attr_inst.nelmts; loc++) {
		        print_rep = print_attr(ty_id, loc, &value[0]);
		        //if (print_rep != NULL) {
		        if (print_rep.c_str() != NULL) {
			    attr_table_ptr->append_attr(attr_name, dap_type, print_rep.c_str());
		            //delete[] print_rep;
		            //print_rep = NULL;
                        }
		    }

	        }
	        else {
	    	    // If the hdf5 data type is HDF5 string or number of dimension is >  0;
		    // handle this differently.
		    DBG(cerr << "=read_objects(): ndims=" << (int) attr_inst.
			ndims << endl);

                    // Get the attribute datatype size
		    int elesize = (int) H5Tget_size(attr_inst.type);
		    if (elesize == 0) {
		        DBG(cerr << "=read_objects(): elesize=0" << endl);
		        //delete[] value;
		        if (H5Aclose(attr_id) < 0) {
			    throw InternalErr(__FILE__, __LINE__, "unable to close attibute id");
		        }

		        throw InternalErr(__FILE__, __LINE__, "unable to get attibute size");
		    }

                    // Due to the implementation of print_attr, the attribute value will be 
                    // written one by one.
		    char *tempvalue = &value[0];
                    // cerr <<"nelems = "<<(int)attr_inst.nelmts <<endl;

//		    for (int dim = 0; dim < (int) attr_inst.ndims; dim++) {
//		        for (int sizeindex = 0; sizeindex < (int) attr_inst.size[dim]; sizeindex++) {

                            // Write this value. the "loc" can always be set to 0 since
                            // tempvalue will be moved to the next value.
                   for( hsize_t temp_index = 0; temp_index < attr_inst.nelmts; temp_index ++) {
			    print_rep = print_attr(ty_id, 0/*loc*/, tempvalue);
			    //if (print_rep != NULL) {
			    if (print_rep.c_str() != NULL) {
			        attr_table_ptr->append_attr(attr_name, dap_type, print_rep.c_str());
			        tempvalue = tempvalue + elesize;

			        DBG(cerr
				    << "tempvalue=" << tempvalue
				    << "elesize=" << elesize
				    << endl);

			        //delete[] print_rep;
			        //print_rep = NULL;
			    }
			    else {
                                if (H5Aclose(attr_id) < 0) {
                                   throw InternalErr(__FILE__, __LINE__, "unable to close HDF5 attibute id");
                                }

                                throw InternalErr(__FILE__, __LINE__, "unable to convert attibute value to DAP");
			    }
                        }
		 //       } // for (int sizeindex = 0; ...
		 //   } // for (int dim = 0; ...
	        } // if attr_inst.ndims != 0
            }
	    if (H5Aclose(attr_id) < 0) {
		throw InternalErr(__FILE__, __LINE__, "unable to close attibute id");

	    }
	} // for (int j = 0; j < num_attr; j++)
    //} // try - protects print_rep and value
#if 0
    catch (...) {
	if (print_rep)
	    delete[] print_rep;
	throw;
    }
#endif

    DBG(cerr << "<read_objects()" << endl);
}

///////////////////////////////////////////////////////////////////////
/// \fn find_gloattr(hid_t file, DAS & das)
/// will fill in attributes of the root group into one DAS table.
///
/// The attributes are treated as global attributes.
///
/// \param das DAS object reference
/// \param file HDF5 file id
/// \exception msg string of error message to the dods interface.
/// \return void
//////////////////////////////////////////////////////////////////////////
void find_gloattr(hid_t file, DAS & das)
{
    DBG(cerr << ">find_gloattr()" << endl);

    hid_t root = H5Gopen(file, "/",H5P_DEFAULT);
    try {
	if (root < 0)
	    throw InternalErr(__FILE__, __LINE__,
			      "unable to open the HDF5 root group");
        
        // In the default option of the HDF5 handler, the 
        // HDF5 file structure(group hierarchy) will be mapped to 
        // a DAP attribute HDF5_ROOT_GROUP. In a sense, this created
        // attribute can be treated as an HDF5 attribute under the root group,
        // so to say, a global attribute. 
	das.add_table("HDF5_ROOT_GROUP", new AttrTable);

        // Since the root group is the first HDF5 object to visit(in HDF5RequestHandler.cc, find_gloattr() 
        // is before the depth_first()), it will always be not visited. However, to find the cyclic groups 
        // to root, we still need to add the object name to the global variable name list "paths" defined at
        // the beginning of the h5das.cc file.
	get_hardlink(root, "/");    

        // Obtain the number of "real" attributes of the root group.
	int num_attrs;
        H5O_info_t obj_info;
        if (H5Oget_info(root, &obj_info) <0) {
            H5Gclose(root);
            string msg = "Obtaining the info. failed for the root group ";
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        
        // Obtain the number of attributes
        num_attrs = obj_info.num_attrs;
	if (num_attrs < 0) {
            H5Gclose(root);
	    throw InternalErr(__FILE__, __LINE__,
			      "unable to get the number of attributes for the HDF root group ");

        }
	if (num_attrs == 0) {
	    if(H5Gclose(root) < 0){
		throw InternalErr(__FILE__, __LINE__,
                                  "Could not close the group.");
	    }
	    DBG(cerr << "<find_gloattr():no attributes" << endl);
	    return;
	}

        // Map the HDF5 root attributes to DAP and save it in a DAS table "H5_GLOBAL". 
        // In theory, we can just "/" as the table name. To help clients better understand,
        // we use "H5_GLOBAL" which is a more meaningful name.
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
void get_softlink(DAS & das, hid_t pgroup, const char *gname, const string & oname, int index, size_t val_size)
{
    DBG(cerr << ">get_softlink():" << oname << endl);

    ostringstream oss;
    oss << string("HDF5_SOFTLINK");
    oss << "_";
    oss << index;
    string temp_varname = oss.str();

//cerr <<"temp_varname "<<temp_varname <<endl;

    DBG(cerr << "=get_softlink():" << temp_varname << endl);
    AttrTable *attr_table_ptr = das.get_table(gname);
    if (!attr_table_ptr)
	attr_table_ptr = das.add_table(gname, new AttrTable);

    AttrTable *attr_softlink_ptr;
    attr_softlink_ptr = attr_table_ptr->append_container(temp_varname);

    string softlink_name = "linkname";
    attr_softlink_ptr->append_attr(softlink_name,STRING,oname);
    string softlink_value_name ="LINKTARGET";
   

    // Get the link target information. We always return the link value in a string format.

    char *buf = 0;
    try {
    	// TODO replace buf with vector<char> buf(val_size + 1);
    	// then access as a char * using &buf[0]
	buf = new char[(val_size + 1) * sizeof(char)];
	// get link target name
	if (H5Lget_val(pgroup, oname.c_str(), (void*) buf,val_size + 1, H5P_DEFAULT)
	    < 0) {
	    throw InternalErr(__FILE__, __LINE__, "unable to get link value");
	}
        attr_softlink_ptr->append_attr(softlink_value_name, STRING, buf);
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

    // Get the object info
    H5O_info_t obj_info;
    if (H5Oget_info(pgroup, &obj_info) <0) { 
        throw InternalErr(__FILE__, __LINE__, "H5Oget_info() failed.");
    }

    // If the reference count is greater than 1,that means 
    // hard links are found. return the original object name this
    // hard link points to. 

    if (obj_info.rc >1) {

        ostringstream oss;
        oss << hex << obj_info.addr;
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

    // Obtain the comment size
    int comment_size;
    comment_size=(int)(H5Oget_comment(oid,NULL,0));
    if (comment_size <0) {
       throw InternalErr(__FILE__, __LINE__,
                          "Could not retrieve the comment size.");
    }

    if (comment_size > 0) {
        vector<char> comment;
        comment.resize(comment_size+1);
        if (H5Oget_comment(oid,&comment[0],comment_size+1)<0) {
	    throw InternalErr(__FILE__, __LINE__,
                          "Could not retrieve the comment.");
        }

        // Insert this comment into the das table.
        AttrTable *at = das.get_table(varname);
        if (!at)
            at = das.add_table(varname, new AttrTable);
        at->append_attr("HDF5_COMMENT", STRING, &comment[0]);

        //delete[] comment;
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


    string h5_spec_char("/");
    string dap_notion(".");
    string::size_type pos = 1;

    if(gname == NULL){
        throw InternalErr(__FILE__, __LINE__,
                          "The wrong HDF5 group name.");
    }
    
    string full_path = string(gname);

    // Change the HDF5 special character '/' with DAP notion '.' 
    // to make sure the group structure can be handled by DAP properly.
    while ((pos = full_path.find(h5_spec_char, pos)) != string::npos) {
        full_path.replace(pos, h5_spec_char.size(), dap_notion);
        pos++;
    }

    // If the HDF5 file includes only the root group, replacing
    // the "/" with the string "HDF5_ROOT_GROUP".
    // Otherwise, replacing the first "/" with the string "HDF5_ROOT_GROUP.",
    // (note the . after "HDF5_ROOT_GROUP." .  Then cutting the last "/".

    // TODO I don't think sizeof(gname) is right. jhrg 9/26/13
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

    // group will be mapped to a container
    if (is_group) {
        at->append_container(oname);
    }
    else {
        at->append_attr("Dataset", "String", oname);
    }
}



