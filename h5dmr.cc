// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007-2015 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

///////////////////////////////////////////////////////////////////////////////
/// \file h5dds.cc
/// \brief DDS/DODS request processing source
///
/// This file is part of h5_dap_handler, a C++ implementation of the DAP
/// handler for HDF5 data.
///
/// This file contains functions which use depth-first search to walk through
/// an HDF5 file and build the in-memory DDS.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang    <myang6@hdfgroup.org>
///

#include "config_hdf5.h"

#include <InternalErr.h>
#include <BESDebug.h>

#include <mime_util.h>

//#include "h5dds.h"
#include "hdf5_handler.h"
#include "HDF5Int32.h"
#include "HDF5UInt32.h"
#include "HDF5UInt16.h"
#include "HDF5Int16.h"
#include "HDF5Byte.h"
#include "HDF5Array.h"
#include "HDF5Str.h"
#include "HDF5Float32.h"
#include "HDF5Float64.h"
#include "HDF5Url.h"
#include "HDF5Structure.h"

//#include "h5get.h"
//#ifdef USE_DAP4
#include "HDF5CFUtil.h"
#include "h5dmr.h"
//#endif

/// A variable for remembering visited paths to break cyclic HDF5 groups. 
HDF5PathFinder obj_paths;


/// An instance of DS_t structure defined in hdf5_handler.h.
static DS_t dt_inst; 
void map_h5_attrs_to_d4(hid_t oid,D4Group* d4g,BaseType* d4b,Structure * d4s,int flag);
//void map_h5_attrs_to_d4(hid_t oid,D4Group* d4g,BaseType* d4b,HDF5Structure * d4s,int flag);
/// put them into ithe dds table.
///
/// \param pid group id
/// \param gname group name (the absolute path from the root group)
/// \param dds reference of DDS object
/// \param fname the HDF5 file name
///
/// \return 0, if failed.
/// \return 1, if succeeded.
///
/// \remarks hard link is treated as a dataset.
/// \remarks will return error message to the DAP interface.
/// \see depth_first(hid_t pid, char *gname, DAS & das, const char *fname) in h5das.cc
///////////////////////////////////////////////////////////////////////////////


//bool depth_first(hid_t pid, char *gname, DMR & dmr, D4Group* par_grp, const char *fname,bool use_dimscale)
bool depth_first(hid_t pid, char *gname, DMR & dmr, D4Group* par_grp, const char *fname)
//bool depth_first(hid_t pid, char *gname, D4Group* par_grp, const char *fname)
{
    BESDEBUG("h5",
        ">depth_first() for dmr " 
        << " pid: " << pid
        << " gname: " << gname
        << " fname: " << fname
        << endl);
        
    /// To keep track of soft links.
    int slinkindex = 0;

    // Iterate through the file to see the members of the group from the root.
    H5G_info_t g_info; 
    hsize_t nelems = 0;
    if(H5Gget_info(pid,&g_info) <0) {
      string msg =
            "h5_dds handler: counting hdf5 group elements error for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    nelems = g_info.nlinks;
        
    ssize_t oname_size;
    for (hsize_t i = 0; i < nelems; i++) {

        vector <char>oname;

        // Query the length of object name.
        oname_size =
 	    H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,NULL,
                (size_t)DODS_NAMELEN, H5P_DEFAULT);
        if (oname_size <= 0) {
            string msg = "h5_dds handler: Error getting the size of the hdf5 object from the group: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the name of the object
        oname.resize((size_t) oname_size + 1);

        //if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname,
        if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,&oname[0],
            (size_t)(oname_size+1), H5P_DEFAULT) < 0){
            string msg =
                    "h5_dds handler: Error getting the hdf5 object name from the group: ";
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
            
        // Information of soft links are stored as attributes 
        // TOOODOOO
        if(linfo.type == H5L_TYPE_SOFT) { 
            slinkindex++;
            size_t val_size = linfo.u.val_size;
            get_softlink(par_grp,pid,gname,&oname[0],slinkindex,val_size);
            continue;
         }

        // Ignore external links
        if(linfo.type == H5L_TYPE_EXTERNAL) 
            continue;

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;

        if (H5Oget_info_by_idx(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
            string msg = "h5_dds handler: Error obtaining the info for the object";
            msg += string(oname.begin(),oname.end());
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        H5O_type_t obj_type = oinfo.type;

        switch (obj_type) {  

            case H5O_TYPE_GROUP: 
            {

                // Obtain the full path name
                string full_path_name =
                    string(gname) + string(oname.begin(),oname.end()-1) + "/";
                    //string(gname) + string(oname) + "/";

                BESDEBUG("h5", "=depth_first dmr ():H5G_GROUP " << full_path_name
                    << endl);

                vector <char>t_fpn;
                t_fpn.resize(full_path_name.length()+1);
                copy(full_path_name.begin(),full_path_name.end(),t_fpn.begin());
                t_fpn[full_path_name.length()] = '\0';

                hid_t cgroup = H5Gopen(pid, &t_fpn[0],H5P_DEFAULT);
                if (cgroup < 0){
		   throw InternalErr(__FILE__, __LINE__, "h5_dds handler: H5Gopen() failed.");
		}

                string grp_name = string(oname.begin(),oname.end()-1);

                // Check the hard link loop and break the loop if it exists.
		//string oid = get_hardlink_dmr(pid, &oname[0]);
		//string oid = get_hardlink_dmr(pid, full_path_name.c_str());
		string oid = get_hardlink_dmr(cgroup, full_path_name.c_str());
                if (oid == "") {
                    try {

                        //D4Group* d4_cgroup = new D4Group(full_path_name.substr(0,full_path_name.size()-1));

                        D4Group* tem_d4_cgroup = new D4Group(grp_name);
                        map_h5_attrs_to_d4(cgroup,tem_d4_cgroup,NULL,NULL,0);
                        par_grp->add_group_nocopy(tem_d4_cgroup);

                        //D4Group* d4_cgroup = par_grp->find_child_grp(grp_name);
                        //depth_first(cgroup, &t_fpn[0],  d4_cgroup,fname);

                        depth_first(cgroup, &t_fpn[0], dmr, tem_d4_cgroup,fname);
                        //depth_first(cgroup, &t_fpn[0], dmr, tem_d4_cgroup,fname,use_dimscale);
                        // No need to delete tem_d4_cgroup.
                        //delete tem_d4_cgroup;
                    }
                    catch(...) {
                        H5Gclose(cgroup);
                        throw;
                    }
                }
                else {
//cerr<<"coming to hardlink "<<endl;
                    // This group has been visited.  
                    // Add the attribute table with the attribute name as HDF5_HARDLINK.
                    // The attribute value is the name of the group when it is first visited.
                    D4Group* tem_d4_cgroup = new D4Group(string(grp_name));
                    D4Attribute *d4_hlinfo = new D4Attribute("HDF5_HARDLINK",attr_str_c);
                    d4_hlinfo->add_value(obj_paths.get_name(oid));
                    tem_d4_cgroup->attributes()->add_attribute_nocopy(d4_hlinfo);
                    par_grp->add_group_nocopy(tem_d4_cgroup);
         
                }

                if (H5Gclose(cgroup) < 0){
		   throw InternalErr(__FILE__, __LINE__, "Could not close the group.");
		}
                break;
            }

            case H5O_TYPE_DATASET:
            {

                // Obtain the absolute path of the HDF5 dataset
                //string full_path_name = string(gname) + string(oname);
                string full_path_name = string(gname) + string(oname.begin(),oname.end()-1);

                // TOOOODOOOO
                // Obtain the hdf5 dataset handle stored in the structure dt_inst. 
                // All the metadata information in the handler is stored in dt_inst.
                // Work on this later, redundant for dmr since dataset is opened twice. KY 2015-07-01
                get_dataset(pid, full_path_name, &dt_inst,false);
                //get_dataset(pid, full_path_name, &dt_inst,use_dimscale);
               
                hid_t dset_id = -1;
                if((dset_id = H5Dopen(pid,full_path_name.c_str(),H5P_DEFAULT)) <0) {
                   string msg = "cannot open the HDF5 dataset  ";
                   msg += full_path_name;
                   throw InternalErr(__FILE__, __LINE__, msg);
                }

#if 0
                string oid = get_hardlink(dset_id, full_path_name);

                // Break the cyclic loop created by hard links

                // If this HDF5 dataset has been visited, 
                // Add the DAS table with the attribute name as HDF5_HARDLINK.
                // The attribute value is the name of the HDF5 dataset when it is first visited.
                if (false == oid.empty()) {

                    // Add attribute table with HARDLINK
                    AttrTable *at = das.get_table(full_path_name);
                    if(!at) {
                      at = das.add_table(full_path_name, new AttrTable);
                    }

                    // Note that "paths" is a global object to find the visited path. 
                    // It is defined at the beginning of this source code file.
                    at->append_attr("HDF5_HARDLINK", STRING, paths.get_name(oid));

                }

#endif
                read_objects(dmr, par_grp, full_path_name, fname,dset_id);

                
                H5Dclose(dset_id);
            }
                break;

#if 0
                string oid = get_hardlink(par_grp, dset, full_path_name);

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

                H5Dclose(dset_id);
                // Put the hdf5 dataset structure into DODS dds.
                //read_objects(dds, full_path_name, fname);
                //H5Tclose(dt_inst.type);
                break;
            }

#endif
            case H5O_TYPE_NAMED_DATATYPE:
                // ignore the named datatype
                break;
            default:
                break;
            }
            
        //if (oname) {delete[]oname; oname = NULL;}
    } // for i is 0 ... nelems

    BESDEBUG("h5", "<depth_first() " << endl);
    return true;
}

bool breadth_first(hid_t pid, char *gname, DMR & dmr, D4Group* par_grp, const char *fname,bool use_dimscale)
//bool depth_first(hid_t pid, char *gname, D4Group* par_grp, const char *fname)
{
    BESDEBUG("h5",
        ">depth_first() for dmr " 
        << " pid: " << pid
        << " gname: " << gname
        << " fname: " << fname
        << endl);
        
    /// To keep track of soft links.
    int slinkindex = 0;

    // Iterate through the file to see the members of the group from the root.
    H5G_info_t g_info; 
    hsize_t nelems = 0;
    if(H5Gget_info(pid,&g_info) <0) {
      string msg =
            "h5_dds handler: counting hdf5 group elements error for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    nelems = g_info.nlinks;
        
    ssize_t oname_size;
    for (hsize_t i = 0; i < nelems; i++) {

        vector <char>oname;

        // Query the length of object name.
        oname_size =
 	    H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,NULL,
                (size_t)DODS_NAMELEN, H5P_DEFAULT);
        if (oname_size <= 0) {
            string msg = "h5_dds handler: Error getting the size of the hdf5 object from the group: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the name of the object
        oname.resize((size_t) oname_size + 1);

        //if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname,
        if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,&oname[0],
            (size_t)(oname_size+1), H5P_DEFAULT) < 0){
            string msg =
                    "h5_dds handler: Error getting the hdf5 object name from the group: ";
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
            
        // Information of soft links are stored as attributes 
        // TOOODOOO
        if(linfo.type == H5L_TYPE_SOFT) { 
            slinkindex++;
            size_t val_size = linfo.u.val_size;
            get_softlink(par_grp,pid,gname,&oname[0],slinkindex,val_size);
            continue;
         }

        // Ignore external links
        if(linfo.type == H5L_TYPE_EXTERNAL) 
            continue;

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;

        if (H5Oget_info_by_idx(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
            string msg = "h5_dds handler: Error obtaining the info for the object";
            msg += string(oname.begin(),oname.end());
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        H5O_type_t obj_type = oinfo.type;

       if(H5O_TYPE_DATASET == obj_type) {

                // Obtain the absolute path of the HDF5 dataset
                //string full_path_name = string(gname) + string(oname);
                string full_path_name = string(gname) + string(oname.begin(),oname.end()-1);

                // TOOOODOOOO
                // Obtain the hdf5 dataset handle stored in the structure dt_inst. 
                // All the metadata information in the handler is stored in dt_inst.
                // Work on this later, redundant for dmr since dataset is opened twice. KY 2015-07-01
                get_dataset(pid, full_path_name, &dt_inst,use_dimscale);
               
                hid_t dset_id = -1;
                if((dset_id = H5Dopen(pid,full_path_name.c_str(),H5P_DEFAULT)) <0) {
                   string msg = "cannot open the HDF5 dataset  ";
                   msg += full_path_name;
                   throw InternalErr(__FILE__, __LINE__, msg);
                }

#if 0
                string oid = get_hardlink(dset_id, full_path_name);

                // Break the cyclic loop created by hard links

                // If this HDF5 dataset has been visited, 
                // Add the DAS table with the attribute name as HDF5_HARDLINK.
                // The attribute value is the name of the HDF5 dataset when it is first visited.
                if (false == oid.empty()) {

                    // Add attribute table with HARDLINK
                    AttrTable *at = das.get_table(full_path_name);
                    if(!at) {
                      at = das.add_table(full_path_name, new AttrTable);
                    }

                    // Note that "paths" is a global object to find the visited path. 
                    // It is defined at the beginning of this source code file.
                    at->append_attr("HDF5_HARDLINK", STRING, paths.get_name(oid));

                }

#endif
                read_objects(dmr, par_grp, full_path_name, fname,dset_id);

                
                H5Dclose(dset_id);
 


        } 
    }
   
    for (hsize_t i = 0; i < nelems; i++) {

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;

        if (H5Oget_info_by_idx(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
            string msg = "h5_dds handler: Error obtaining the info for the object in the breadth_first.";
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        H5O_type_t obj_type = oinfo.type;


        if(obj_type == H5O_TYPE_GROUP) {  

                    vector <char>oname;

        // Query the length of object name.
        oname_size =
 	    H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,NULL,
                (size_t)DODS_NAMELEN, H5P_DEFAULT);
        if (oname_size <= 0) {
            string msg = "h5_dds handler: Error getting the size of the hdf5 object from the group: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the name of the object
        oname.resize((size_t) oname_size + 1);

        //if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname,
        if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,&oname[0],
            (size_t)(oname_size+1), H5P_DEFAULT) < 0){
            string msg =
                    "h5_dds handler: Error getting the hdf5 object name from the group: ";
             msg += gname;
                throw InternalErr(__FILE__, __LINE__, msg);
        }



                // Obtain the full path name
                string full_path_name =
                    string(gname) + string(oname.begin(),oname.end()-1) + "/";
                    //string(gname) + string(oname) + "/";

                BESDEBUG("h5", "=depth_first dmr ():H5G_GROUP " << full_path_name
                    << endl);

                vector <char>t_fpn;
                t_fpn.resize(full_path_name.length()+1);
                copy(full_path_name.begin(),full_path_name.end(),t_fpn.begin());
                t_fpn[full_path_name.length()] = '\0';

                hid_t cgroup = H5Gopen(pid, &t_fpn[0],H5P_DEFAULT);
                if (cgroup < 0){
		   throw InternalErr(__FILE__, __LINE__, "h5_dds handler: H5Gopen() failed.");
		}

                string grp_name = string(oname.begin(),oname.end()-1);

                // Check the hard link loop and break the loop if it exists.
		//string oid = get_hardlink_dmr(pid, &oname[0]);
		//string oid = get_hardlink_dmr(pid, full_path_name.c_str());
		string oid = get_hardlink_dmr(cgroup, full_path_name.c_str());
                if (oid == "") {
                    try {

                        //D4Group* d4_cgroup = new D4Group(full_path_name.substr(0,full_path_name.size()-1));

                        D4Group* tem_d4_cgroup = new D4Group(grp_name);
                        map_h5_attrs_to_d4(cgroup,tem_d4_cgroup,NULL,NULL,0);
                        par_grp->add_group_nocopy(tem_d4_cgroup);

                        //D4Group* d4_cgroup = par_grp->find_child_grp(grp_name);
                        //depth_first(cgroup, &t_fpn[0],  d4_cgroup,fname);

                        breadth_first(cgroup, &t_fpn[0], dmr, tem_d4_cgroup,fname,use_dimscale);
                        // No need to delete tem_d4_cgroup.
                        //delete tem_d4_cgroup;
                    }
                    catch(...) {
                        H5Gclose(cgroup);
                        throw;
                    }
                }
                else {
//cerr<<"coming to hardlink "<<endl;
                    // This group has been visited.  
                    // Add the attribute table with the attribute name as HDF5_HARDLINK.
                    // The attribute value is the name of the group when it is first visited.
                    D4Group* tem_d4_cgroup = new D4Group(string(grp_name));
                    D4Attribute *d4_hlinfo = new D4Attribute("HDF5_HARDLINK",attr_str_c);
                    d4_hlinfo->add_value(obj_paths.get_name(oid));
                    tem_d4_cgroup->attributes()->add_attribute_nocopy(d4_hlinfo);
                    par_grp->add_group_nocopy(tem_d4_cgroup);
         
                }

                if (H5Gclose(cgroup) < 0){
		   throw InternalErr(__FILE__, __LINE__, "Could not close the group.");
		}
            }


            
        //if (oname) {delete[]oname; oname = NULL;}
    } // for i is 0 ... nelems

    BESDEBUG("h5", "<depth_first() " << endl);
    return true;
}
#if 0
///////////////////////////////////////////////////////////////////////////////
/// \fn Get_bt(const string &varname,  const string  &dataset, hid_t datatype)
/// returns the pointer to the base type
///
/// This function will create a new DODS object that corresponds to HDF5
/// dataset and return the pointer of a new object of DODS datatype. If an
/// error is found, an exception of type InternalErr is thrown. 
///
/// \param varname object name
/// \param dataset name of dataset where this object comes from
/// \param datatype datatype id
/// \return pointer to BaseType
///////////////////////////////////////////////////////////////////////////////
static BaseType *Get_bt(const string &vname,
                        const string &dataset,
                        hid_t datatype)
{
    BaseType *btp = NULL;

    try {

        BESDEBUG("h5", ">Get_bt varname=" << vname << " datatype=" << datatype
            << endl);

        size_t size = 0;
        int sign    = -2;
        switch (H5Tget_class(datatype)) {

        case H5T_INTEGER:
            size = H5Tget_size(datatype);
            sign = H5Tget_sign(datatype);
            BESDEBUG("h5", "=Get_bt() H5T_INTEGER size = " << size << " sign = "
                << sign << endl);

	    if (sign == H5T_SGN_ERROR) {
                throw InternalErr(__FILE__, __LINE__, "cannot retrieve the sign type of the integer");
            }
            if (size == 0) {
		throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
	    }
	    else if (size == 1) { // Either signed char or unsigned char
                // DAP2 doesn't support signed char, it maps to DAP int16.
                if (sign == H5T_SGN_2) // signed char to DAP int16
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
            else if (size == 8) {
                throw
                    InternalErr(__FILE__, __LINE__,
                                string("Unsupported HDF5 64-bit Integer type:")
                                + vname);
            }
            break;

        case H5T_FLOAT:
            size = H5Tget_size(datatype);
            BESDEBUG("h5", "=Get_bt() H5T_FLOAT size = " << size << endl);

	    if (size == 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
            }
            else if (size == 4) {
                btp = new HDF5Float32(vname, dataset);
            }
            else if (size == 8) {
                btp = new HDF5Float64(vname, dataset);
            }
            break;

        case H5T_STRING:
            btp = new HDF5Str(vname, dataset);
            break;

        // The array datatype is rarely,rarely used. So this
        // part of code is not reviewed.
 #if 0
        case H5T_ARRAY: {
            BaseType *ar_bt = 0;
            try {
                BESDEBUG("h5",
                    "=Get_bt() H5T_ARRAY datatype = " << datatype
                    << endl);

                // Get the base datatype of the array
                hid_t dtype_base = H5Tget_super(datatype);
                ar_bt = Get_bt(vname, dataset, dtype_base);
                btp = new HDF5Array(vname, dataset, ar_bt);
                delete ar_bt; ar_bt = 0;

                // Set the size of the array.
                int ndim = H5Tget_array_ndims(datatype);
                size = H5Tget_size(datatype);
                int nelement = 1;

		if (dtype_base < 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the base datatype");
 	        }
		if (ndim < 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the rank of the array datatype");
                }
		if (size == 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
                }
                BESDEBUG(cerr
                    << "=Get_bt()" << " Dim = " << ndim
                    << " Size = " << size
                    << endl);

                hsize_t size2[DODS_MAX_RANK];
                if(H5Tget_array_dims(datatype, size2) < 0){
                    throw
                        InternalErr(__FILE__, __LINE__,
                                    string("Could not get array dims for: ")
                                      + vname);
                }


                HDF5Array &h5_ar = static_cast < HDF5Array & >(*btp);
                for (int dim_index = 0; dim_index < ndim; dim_index++) {
                    h5_ar.append_dim(size2[dim_index]);
                    BESDEBUG("h5", "=Get_bt() " << size2[dim_index] << endl);
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
		if (h5_ar.d_type == H5T_NO_CLASS){
		    throw InternalErr(__FILE__, __LINE__, "cannot return the datatype class identifier");
		}
            }
            catch (...) {
                if( ar_bt ) delete ar_bt;
                if( btp ) delete btp;
                throw;
            }
            break;
        }
#endif

        // Reference map to DAP URL, check the technical note.
        case H5T_REFERENCE:
            btp = new HDF5Url(vname, dataset);
            break;
        
        default:
            throw InternalErr(__FILE__, __LINE__,
                              string("Unsupported HDF5 type:  ") + vname);
        }
    }
    catch (...) {
        if( btp ) delete btp;
        throw;
    }

    if (!btp)
        throw InternalErr(__FILE__, __LINE__,
                          string("Could not make a DAP variable for: ")
                          + vname);
                                                  
    BESDEBUG("h5", "<Get_bt()" << endl);
    return btp;
}


///////////////////////////////////////////////////////////////////////////////
/// \fn Get_structure(const string& varname, const string &dataset,
///     hid_t datatype)
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
///////////////////////////////////////////////////////////////////////////////
static Structure *Get_structure(const string &varname,
                                const string &dataset,
                                hid_t datatype)
{
    HDF5Structure *structure_ptr = NULL;
    char* memb_name = NULL;
    hid_t memb_type = -1;

    BESDEBUG("h5", ">Get_structure()" << datatype << endl);

    if (H5Tget_class(datatype) != H5T_COMPOUND)
        throw InternalErr(__FILE__, __LINE__,
                          string("Compound-to-structure mapping error for ")
                          + varname);

    try {
        structure_ptr = new HDF5Structure(varname, dataset);
        //structure_ptr->set_did(dt_inst.dset);
        //structure_ptr->set_tid(dt_inst.type);

        // Retrieve member types
        int nmembs = H5Tget_nmembers(datatype);
        BESDEBUG("h5", "=Get_structure() has " << nmembs << endl);
	if (nmembs < 0){
	   throw InternalErr(__FILE__, __LINE__, "cannot retrieve the number of elements");
	}
        for (int i = 0; i < nmembs; i++) {
            memb_name = H5Tget_member_name(datatype, i);
            H5T_class_t memb_cls = H5Tget_member_class(datatype, i);
            memb_type = H5Tget_member_type(datatype, i);
	    if (memb_name == NULL){
		throw InternalErr(__FILE__, __LINE__, "cannot retrieve the name of the member");
	    }
            if ((memb_cls < 0) | (memb_type < 0)) {
                // structure_ptr is deleted in the catch ... block
                // below. So if this exception is thrown, it will
                // get caught below and the ptr deleted.
                // pwest Mar 18, 2009
                //delete structure_ptr;
                throw InternalErr(__FILE__, __LINE__,
                                  string("Type mapping error for ")
                                  + string(memb_name) );
            }
            
            // ~Structure() will delete these if they are added.
            if (memb_cls == H5T_COMPOUND) {
                Structure *s = Get_structure(memb_name, memb_name,dataset, memb_type);
                structure_ptr->add_var(s);
                delete s; s = 0;
            } 
            else if(memb_cls == H5T_ARRAY) {

                BaseType *ar_bt = 0;
                BaseType *btp   = 0;
                Structure *s    = 0;
                hid_t     dtype_base = 0;

                try {

                    // Get the base memb_type of the array
                    dtype_base = H5Tget_super(memb_type);

                    // Set the size of the array.
                    int ndim = H5Tget_array_ndims(memb_type);
                    size_t size = H5Tget_size(memb_type);
                    int nelement = 1;

		    if (dtype_base < 0) {
                        throw InternalErr(__FILE__, __LINE__, "cannot return the base memb_type");
 	            }
		    if (ndim < 0) {
                        throw InternalErr(__FILE__, __LINE__, "cannot return the rank of the array memb_type");
                    }
		    if (size == 0) {
                        throw InternalErr(__FILE__, __LINE__, "cannot return the size of the memb_type");
                    }

                    hsize_t size2[DODS_MAX_RANK];
                    if(H5Tget_array_dims(memb_type, size2) < 0){
                        throw
                        InternalErr(__FILE__, __LINE__,
                                    string("Could not get array dims for: ")
                                      + string(memb_name));
                    }

                    H5T_class_t array_memb_cls = H5Tget_class(dtype_base);
                    if(array_memb_cls == H5T_NO_CLASS) {
                        throw InternalErr(__FILE__, __LINE__,
                                  string("cannot get the correct class for compound type member")
                                  + string(memb_name));
                    }
                    if(H5T_COMPOUND == array_memb_cls) {

                        s = Get_structure(memb_name, memb_name,dataset, dtype_base);
                        HDF5Array *h5_ar = new HDF5Array(memb_name, dataset, s);
                    
                        for (int dim_index = 0; dim_index < ndim; dim_index++) {
                            h5_ar->append_dim(size2[dim_index]);
                            nelement = nelement * size2[dim_index];
                        }

                        // May delete them later since all these can be obtained from the file ID.
                        //h5_ar->set_did(dt_inst.dset);
                        // Assign the array memb_type id.
                        //h5_ar->set_tid(memb_type);
                        h5_ar->set_memneed(size);
                        h5_ar->set_numdim(ndim);
                        h5_ar->set_numelm(nelement);
                        h5_ar->set_length(nelement);

	                structure_ptr->add_var(h5_ar);
                        delete h5_ar;
	
                    }
                    else if (H5T_INTEGER == array_memb_cls || H5T_FLOAT == array_memb_cls || H5T_STRING == array_memb_cls) { 
                        ar_bt = Get_bt(memb_name, dataset, dtype_base);
                        HDF5Array *h5_ar = new HDF5Array(memb_name,dataset,ar_bt);
                        //btp = new HDF5Array(memb_name, dataset, ar_bt);
                        //HDF5Array &h5_ar = static_cast < HDF5Array & >(*btp);
                    
                        for (int dim_index = 0; dim_index < ndim; dim_index++) {
                            h5_ar->append_dim(size2[dim_index]);
                            nelement = nelement * size2[dim_index];
                        }

                        // May delete them later
                        //h5_ar->set_did(dt_inst.dset);
                        // Assign the array memb_type id.
                        //h5_ar->set_tid(memb_type);
                        h5_ar->set_memneed(size);
                        h5_ar->set_numdim(ndim);
                        h5_ar->set_numelm(nelement);
                        h5_ar->set_length(nelement);

	                structure_ptr->add_var(h5_ar);
                        delete h5_ar;
                    }
                    if( ar_bt ) delete ar_bt;
                    if( btp ) delete btp;
                    if(s) delete s;
                    H5Tclose(dtype_base);

                }
                catch (...) {
                    if( ar_bt ) delete ar_bt;
                    if( btp ) delete btp;
                    if(s) delete s;
                    H5Tclose(dtype_base);
                    throw;
                }
 

            }
            else if (memb_cls == H5T_INTEGER || memb_cls == H5T_FLOAT || memb_cls == H5T_STRING)  {
                BaseType *bt = Get_bt(memb_name, dataset, memb_type);
                structure_ptr->add_var(bt);
                delete bt; bt = 0;
            }

            else {
                free(memb_name);
		throw InternalErr(__FILE__, __LINE__, "unsupported field datatype inside a compound datatype");
            }
            // Caller needs to free the memory allocated by the library for memb_name.
            free(memb_name);
        }
    }
    catch (...) {
        // if memory allocation exception thrown it will be caught
        // here, so should check if structure ptr exists before
        // deleting it. pwest Mar 18, 2009
        if( structure_ptr ) delete structure_ptr;
        if(memb_name!= NULL) 
            free(memb_name);
        if(memb_type != -1)
            H5Tclose(memb_type);
        throw;
    }

    BESDEBUG("h5", "<Get_structure()" << endl);

    return structure_ptr;
}
#endif

void
read_objects(DMR & dmr, D4Group * d4_grp, const string &varname, const string &filename, const hid_t dset_id)
{

    switch (H5Tget_class(dt_inst.type)) {

    // HDF5 compound maps to DAP structure.
    case H5T_COMPOUND:
        read_objects_structure(dmr,d4_grp, varname, filename,dset_id);
        break;

    case H5T_ARRAY:
    {
        H5Tclose(dt_inst.type);
        throw InternalErr(__FILE__, __LINE__, "Currently don't support accessing data of Array datatype when array datatype is not inside the compound.");       
    }
    default:
        read_objects_base_type(dmr, d4_grp,varname, filename,dset_id);
        break;
    }
    // We must close the datatype obtained in the get_dataset routine since this is the end of reading DDS.
    if(H5Tclose(dt_inst.type)<0) {
        throw InternalErr(__FILE__, __LINE__, "Cannot close the HDF5 datatype.");       
    }
}

void
read_objects_base_type(DMR & dmr, D4Group * d4_grp,const string & varname,
                       const string & filename,hid_t dset_id)
{
    // Obtain the DDS dataset name.
    //dds_table.set_dataset_name(name_path(filename)); 

//cerr<<"coming to read_objects_base_type "<<endl;
//cerr<<"var name is "<<varname <<endl;
    // Get a base type. It should be int, float, double, etc. -- atomic
    // datatype. 
    string newvarname = HDF5CFUtil::obtain_string_after_lastslash(varname);
//cerr<<"newvar name is "<<newvarname <<endl;
    //string newvarname = varname;
    //BaseType *bt = Get_bt(varname, filename, dt_inst.type);
    BaseType *bt = Get_bt(newvarname, varname,filename, dt_inst.type,true);
    if (!bt) {
//cerr<<"bt is NULL "<<endl;
        // NB: We're throwing InternalErr even though it's possible that
        // someone might ask for an HDF5 varaible which this server cannot
        // handle.
        throw
            InternalErr(__FILE__, __LINE__,
                        "Unable to convert hdf5 datatype to dods basetype");
    }

    // First deal with scalar data. 
    if (dt_inst.ndims == 0) {
//cerr<<"coming to the scalar data. "<<endl;
        //des_table.add_var(bt);
        BaseType* new_var = bt->transform_to_dap4(d4_grp,d4_grp);
        map_h5_attrs_to_d4(dset_id,NULL,new_var,NULL,1);

        map_h5_dset_hardlink_to_d4(dset_id,varname,new_var,NULL,1);
        if(new_var) 
            d4_grp->add_var_nocopy(new_var);
        delete bt; bt = 0;
    }
    else {
        // Next, deal with Array data. This 'else clause' runs to
        // the end of the method. jhrg
//cerr<<"coming to array data. "<<endl;

        HDF5Array *ar = new HDF5Array(newvarname, filename, bt);
        delete bt; bt = 0;
        //ar->set_did(dt_inst.dset);
        //ar->set_tid(dt_inst.type);
        ar->set_memneed(dt_inst.need);
        ar->set_numdim(dt_inst.ndims);
        ar->set_numelm((int) (dt_inst.nelmts));
        ar->set_varpath(varname);
	//for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) 
//            ar->append_dim(dt_inst.size[dim_index]); 


        
//cerr<<"varname is "<<varname <<endl;
//cerr<<"dt_inst.dimnames.size() is :" <<dt_inst.dimnames.size() <<endl;
        if(dt_inst.dimnames.size() ==dt_inst.ndims) {
	    for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
// DEBUGGING TOOODDOOO LATER, UNCOMMENT
//#if 0
                if(dt_inst.dimnames[dim_index] !="")
                    ar->append_dim(dt_inst.size[dim_index],dt_inst.dimnames[dim_index]);
                else 
//#endif
                    ar->append_dim(dt_inst.size[dim_index]);

            }
            dt_inst.dimnames.clear();
        }
        else {
        
            for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) 
                ar->append_dim(dt_inst.size[dim_index]); 

        }
        

        //BaseType* new_var = ar->transform_to_dap4(d4_grp,d4_grp);
        BaseType* new_var = ar->h5dims_transform_to_dap4(d4_grp);

        map_h5_attrs_to_d4(dset_id,NULL,new_var,NULL,1);
        map_h5_dset_hardlink_to_d4(dset_id,varname,new_var,NULL,1);
      
#if 0
        // Test the attribute
        D4Attribute *test_attr = new D4Attribute("DAP4_test",attr_str_c);
        test_attr->add_value("test_grp_attr");
        new_var->attributes()->add_attribute_nocopy(test_attr);
#endif
        if(new_var) 
            d4_grp->add_var_nocopy(new_var);
        //dmr.add_var(ar);
        delete ar; ar = 0;
    }

    BESDEBUG("h5", "<read_objects_base_type(dds)" << endl);
}

///////////////////////////////////////////////////////////////////////////////
/// \fn read_objects_structure(DDS & dds_table,const string & varname,
///                  const string & filename)
/// fills in information of a structure dataset (name, data type, data space)
/// into a DDS table. HDF5 compound datatype will map to DAP structure.
/// 
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of structure
///    \param filename The HDF5 file  name that maps to the DDS dataset name.
///    \throw error a string of error message to the dods interface.
///////////////////////////////////////////////////////////////////////////////
void
read_objects_structure(DMR & dmr, D4Group *d4_grp, const string & varname,
                       const string & filename,hid_t dset_id)
{
    //dds_table.set_dataset_name(name_path(filename));
    string newvarname = HDF5CFUtil::obtain_string_after_lastslash(varname);

    Structure *structure = Get_structure(newvarname, varname,filename, dt_inst.type,true);
    //Structure *structure = Get_structure(varname, filename, dt_inst.type,false);

    try {
        // Assume Get_structure() uses exceptions to signal an error. jhrg
        BESDEBUG("h5", "=read_objects_structure(): Dimension is " 
            << dt_inst.ndims << endl);

        if (dt_inst.ndims != 0) {   // Array of Structure
            BESDEBUG("h5", "=read_objects_structure(): array of size " <<
                dt_inst.nelmts << endl);
            BESDEBUG("h5", "=read_objects_structure(): memory needed = " <<
                dt_inst.need << endl);
            HDF5Array *ar = new HDF5Array(newvarname, filename, structure);
            delete structure; structure = 0;
            try {
                //ar->set_did(dt_inst.dset);
                //ar->set_tid(dt_inst.type);
                ar->set_memneed(dt_inst.need);
                ar->set_numdim(dt_inst.ndims);
                ar->set_numelm((int) (dt_inst.nelmts));
                ar->set_length((int) (dt_inst.nelmts));
                ar->set_varpath(varname);

        if(dt_inst.dimnames.size() ==dt_inst.ndims) {
	    for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
                if(dt_inst.dimnames[dim_index] !="")
                    ar->append_dim(dt_inst.size[dim_index],dt_inst.dimnames[dim_index]);
                else 
                    ar->append_dim(dt_inst.size[dim_index]);

            }
            dt_inst.dimnames.clear();
        }
 

               else {
                for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
                    ar->append_dim(dt_inst.size[dim_index]);
                    BESDEBUG("h5", "=read_objects_structure(): append_dim = " <<
                        dt_inst.size[dim_index] << endl);
                }
               }

                BaseType* new_var = ar->h5dims_transform_to_dap4(d4_grp);


                //BaseType* new_var = ar->transform_to_dap4(d4_grp,d4_grp);
                //map_h5_attrs_to_d4(cgroup,tem_d4_cgroup,NULL,NULL,0);
                map_h5_attrs_to_d4(dset_id,NULL,new_var,NULL,1);
                map_h5_dset_hardlink_to_d4(dset_id,varname,new_var,NULL,1);
 

                if(new_var) d4_grp->add_var_nocopy(new_var);
                        //dds_table.add_var(ar);
                delete ar; ar = 0;
            } // try Array *ar
            catch (...) {
                delete ar;
                throw;
            }
        } 
        else {// A scalar structure

//cerr<<"coming to scalar structure "<<endl;
            //HDF5Structure* new_var = dynamic_cast<HDF5Structure*> (structure->transform_to_dap4(d4_grp,d4_grp));
            //BaseType* new_var = structure->transform_to_dap4(d4_grp,d4_grp);
            structure->set_is_dap4(true);
            map_h5_attrs_to_d4(dset_id,NULL,NULL,structure,2);
            map_h5_dset_hardlink_to_d4(dset_id,varname,NULL,structure,2);
            //if(structure) d4_grp->add_var(structure);
            if(structure) d4_grp->add_var_nocopy(structure);
            //if(structure) d4_grp->add_var_nocopy(new_var);

            //dds_table.add_var(structure);
            //delete structure; structure = 0;
        }

    } // try     Structure *structure = Get_structure(...)
    catch (...) {
        delete structure;
        throw;
    }
}



// flag = 0, map to DAP4 group,
// flag = 1  map to DAP4 BaseType(including Array)
// flag = 2  map to DAP4 Structure
void map_h5_attrs_to_d4(hid_t h5_objid,D4Group* d4g,BaseType* d4b,Structure * d4s,int flag) {
//void map_h5_attrs_to_d4(hid_t h5_objid,D4Group* d4g,BaseType* d4b,HDF5Structure * d4s,int flag) {

    // Get the object info
    H5O_info_t obj_info;
    if (H5Oget_info(h5_objid, &obj_info) <0) {
        string msg = "Fail to obtain the HDF5 object info. .";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Obtain the number of attributes
    int num_attr = obj_info.num_attrs;
    if (num_attr < 0 ) {
        string msg = "Fail to get the number of attributes for the HDF5 object. ";
        throw InternalErr(__FILE__, __LINE__,msg);
    }
   
    string print_rep;
    vector<char>temp_buf;

    bool ignore_attr = false;
    hid_t attr_id = -1;
    for (int j = 0; j < num_attr; j++) {

	// Obtain attribute information.
	DSattr_t attr_inst;

        // Ignore the attributes of which the HDF5 datatype 
        // cannot be mapped to DAP4. The ignored attribute datatypes can be found 
        // at function get_attr_info in h5get.cc.
	attr_id = get_attr_info(h5_objid, j, true,&attr_inst, &ignore_attr);
	if (true == ignore_attr) { 
            H5Aclose(attr_id);
            continue;
        }

	// Since HDF5 attribute may be in string datatype, it must be dealt
	// properly. Get data type.
	hid_t ty_id = attr_inst.type;
	string dap_type = get_dap_type(ty_id,true);

        // Need to have DAP4 representation of the attribute type
	string attr_name = attr_inst.name;
//cerr<<"attribute name is "<<attr_name <<endl;
//cerr<<"dap_type is "<<dap_type <<endl;

        D4AttributeType dap4_attr_type = daptype_strrep_to_dap4_attrtype(dap_type);
        if(attr_null_c == dap4_attr_type) {
//cerr<<"coming to attr_null_cc "<<endl;
           H5Tclose(ty_id);
           H5Aclose(attr_id);
	   throw InternalErr(__FILE__, __LINE__, "unsupported DAP4 attribute type");
        }

        // Create the DAP4 attribute mapped from HDF5
        D4Attribute *d4_attr = new D4Attribute(attr_name,dap4_attr_type);

//cerr<<"coming before values. "<<endl;

        // We have to handle variable length string differently. 
        if (H5Tis_variable_str(ty_id)) { 

            BESDEBUG("h5","attribute name " << attr_name <<endl);
            BESDEBUG("h5","attribute size " <<attr_inst.need <<endl);
            BESDEBUG("h5","attribute type size " <<(int)(H5Tget_size(attr_inst.type))<<endl); 

            hid_t temp_space_id = H5Aget_space(attr_id);
            BESDEBUG("h5","attribute calculated size "<<(int)(H5Tget_size(attr_inst.type)) *(int)(H5Sget_simple_extent_npoints(temp_space_id)) <<endl);
            if(temp_space_id <0) {
                H5Tclose(ty_id);
                H5Aclose(attr_id);
	       	throw InternalErr(__FILE__, __LINE__, "unable to read HDF5 attribute data");

            }

            // Variable length string attribute values only store pointers of the actual string value.
            temp_buf.resize((size_t)attr_inst.need);
                
	    if (H5Aread(attr_id, ty_id, &temp_buf[0]) < 0) {
                H5Sclose(temp_space_id);
                H5Tclose(ty_id);
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
                    d4_attr->add_value(tempstring);
                    
                    //attr_table_ptr->append_attr(attr_name, dap_type, tempstring);
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
	    BESDEBUG("h5", "arttr_inst.need=" << attr_inst.need << endl);
  
	    // Read HDF5 attribute data.
	    if (H5Aread(attr_id, ty_id, (void *) (&value[0])) < 0) {
	        // value is deleted in the catch block below so
		// shouldn't be deleted here. pwest Mar 18, 2009
		throw InternalErr(__FILE__, __LINE__, "unable to read HDF5 attribute data");
	    }

	    // For scalar data, just read data once.
	    if (attr_inst.ndims == 0) {
		for (int loc = 0; loc < (int) attr_inst.nelmts; loc++) {
		    print_rep = print_attr(ty_id, loc, &value[0]);
		    if (print_rep.c_str() != NULL) {
                        d4_attr->add_value(print_rep);
                        // Don't know why using c_str to a const char*, add_value for DAP4 uses string.
                        // so don't apply the c_str function. KY 2015-07-01
			//attr_table_ptr->append_attr(attr_name, dap_type, print_rep.c_str());
                    }
		}

	    }
	    else {
	    	// If the hdf5 data type is HDF5 string or number of dimension is >  0;
		// handle this differently.
		BESDEBUG("h5", "=read_objects(): ndims=" << (int) attr_inst.
			ndims << endl);

                // Get the attribute datatype size
		int elesize = (int) H5Tget_size(ty_id);
		if (elesize == 0) {
		    BESDEBUG("h5", "=read_objects(): elesize=0" << endl);
                    H5Tclose(ty_id);
		    H5Aclose(attr_id); 
		    throw InternalErr(__FILE__, __LINE__, "unable to get attibute size");
		}

                // Due to the implementation of print_attr, the attribute value will be 
                // written one by one.
		char *tempvalue = &value[0];

                // Write this value. the "loc" can always be set to 0 since
                // tempvalue will be moved to the next value.
                for( hsize_t temp_index = 0; temp_index < attr_inst.nelmts; temp_index ++) {
		    print_rep = print_attr(ty_id, 0/*loc*/, tempvalue);
	            if (print_rep.c_str() != NULL) {
                        d4_attr->add_value(print_rep);
		        //attr_table_ptr->append_attr(attr_name, dap_type, print_rep.c_str());
			tempvalue = tempvalue + elesize;

			BESDEBUG("h5",
				 "tempvalue=" << tempvalue
				 << "elesize=" << elesize
				 << endl);

	            }
	            else {
                        H5Tclose(ty_id);
                        H5Aclose(attr_id);
                        throw InternalErr(__FILE__, __LINE__, "unable to convert attibute value to DAP");
		    }
                }
	    } // if attr_inst.ndims != 0
        }
        if(H5Tclose(ty_id) < 0) {
            H5Aclose(attr_id);
            throw InternalErr(__FILE__, __LINE__, "unable to close HDF5 type id");
        }
	if (H5Aclose(attr_id) < 0) {
	    throw InternalErr(__FILE__, __LINE__, "unable to close attibute id");
	}

        // D4group
       if(0 == flag) 
           d4g->attributes()->add_attribute_nocopy(d4_attr);
       else if (1 == flag) 
           d4b->attributes()->add_attribute_nocopy(d4_attr);
       else if ( 2 == flag)
           d4s->attributes()->add_attribute_nocopy(d4_attr);
    } // for (int j = 0; j < num_attr; j++)

#if 0
        string oid = get_hardlink_dmr(dset, full_path_name);

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


    }
#endif
//Revise the following later.
#if 0
    string oid = get_hardlink_dmr(dset, full_path_name);

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
#endif

    // 
#if 0
    // D4group
    if(0 == flag) 
        d4g->attributes()->add_attribute_nocopy(d4_attr);
    else if (1 == flag) 
        d4b->attributes()->add_attribute_nocopy(d4_attr);
    else if ( 2 == flag)
        d4s->attributes()->add_attribute_nocopy(d4_attr);
#endif
        
    return;
}

void map_h5_dset_hardlink_to_d4(hid_t h5_objid,const string & full_path, BaseType* d4b,Structure * d4s,int flag) {

    string oid = get_hardlink_dmr(h5_objid, full_path);

    if(false == oid.empty()) {

        D4Attribute *d4_hlinfo = new D4Attribute("HDF5_HARDLINK",attr_str_c);
        d4_hlinfo->add_value(obj_paths.get_name(oid));
 
        if (1 == flag) 
            d4b->attributes()->add_attribute_nocopy(d4_hlinfo);
        else if ( 2 == flag)
            d4s->attributes()->add_attribute_nocopy(d4_hlinfo);
    }

}
 
///////////////////////////////////////////////////////////////////////////////
/// \fn get_softlink(D4Group* par_grp, hid_t pgroup, const string & oname, int index)
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
void get_softlink(D4Group* par_grp, hid_t pgroup, const char *gname, const string & oname, int index, size_t val_size)
{
    BESDEBUG("h5", "dap4 >get_softlink():" << oname << endl);

    ostringstream oss;
    oss << string("HDF5_SOFTLINK");
    oss << "_";
    oss << index;
    string temp_varname = oss.str();

//"h5","temp_varname "<<temp_varname <<endl;

    BESDEBUG("h5", "=get_softlink():" << temp_varname << endl);
    D4Attribute *d4_slinfo = new D4Attribute;
    d4_slinfo->set_name(temp_varname);

    // Make the type as a container
    d4_slinfo->set_type(attr_container_c);

    string softlink_name = "linkname";

    D4Attribute *softlink_src = new D4Attribute(softlink_name,attr_str_c);
    softlink_src->add_value(oname);

    d4_slinfo->attributes()->add_attribute_nocopy(softlink_src);
    //attr_softlink_ptr->append_attr(softlink_name,STRING,oname);
    string softlink_value_name ="LINKTARGET";
   

    // Get the link target information. We always return the link value in a string format.

    //char *buf = 0;

    D4Attribute *softlink_tgt;

    try {
    	// TODO replace buf with vector<char> buf(val_size + 1);
    	// then access as a char * using &buf[0]
	//buf = new char[(val_size + 1) * sizeof(char)];
        vector<char> buf;
        buf.resize(val_size + 1);
	// get link target name
	if (H5Lget_val(pgroup, oname.c_str(), (void*) &buf[0],val_size + 1, H5P_DEFAULT)
	    < 0) {
            //delete[] buf;
	    throw InternalErr(__FILE__, __LINE__, "unable to get link value");
	}
        softlink_tgt = new D4Attribute(softlink_value_name,attr_str_c);
        string link_target_name = string(buf.begin(),buf.end());
        softlink_tgt->add_value(link_target_name);

        d4_slinfo->attributes()->add_attribute_nocopy(softlink_tgt);
        //attr_softlink_ptr->append_attr(softlink_value_name, STRING, buf);
        //delete[]buf;
    }
    catch (...) {
	//delete[] buf;
	delete softlink_tgt;
	throw;
    }

    par_grp->attributes()->add_attribute_nocopy(d4_slinfo);
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
string get_hardlink_dmr( hid_t pgroup, const string & oname) {
    
    BESDEBUG("h5", ">get_hardlink():" << oname << endl);
//cerr<<"oname is the hardlink is "<<oname <<endl;

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

        BESDEBUG("h5", "=get_hardlink() objno=" << objno << endl);

        if (!obj_paths.add(objno, oname)) {
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
