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
/// \file h5dmr.cc
/// \brief DMR(Data Metadata Response) request processing source
///
/// This file is part of h5_dap_handler, a C++ implementation of the DAP
/// handler for HDF5 data.
///
/// This file contains functions which use depth-first and breadth-first search to walk through
/// an HDF5 file and build the in-memory DMR.
/// The depth-first is for the case when HDF5 dimension scales are not used.
/// The breadth-first is for the case when HDF5 dimension scales are used to generate
/// an HDF5 file that follows the netCDF-4 data model. Using breadth-first ensures the 
//  the correct DAP4 DMR layout(group's variables first and then the group).
/// \author Muqun Yang    <myang6@hdfgroup.org>
///

#include <sstream>
#include "config_hdf5.h"

#include <libdap/InternalErr.h>
#include <BESDebug.h>

#include <libdap/mime_util.h>

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

// The HDF5CFUtil.h includes the utility function obtain_string_after_lastslash.
#include "HDF5CFUtil.h"
#include "h5dmr.h"

using namespace std;
using namespace libdap;
/// A variable for remembering visited paths to break cyclic HDF5 groups. 
HDF5PathFinder obj_paths;


/// An instance of DS_t structure defined in hdf5_handler.h.
static DS_t dt_inst; 

/// A function that map HDF5 attributes to DAP4
void map_h5_attrs_to_dap4(hid_t oid,D4Group* d4g,BaseType* d4b,Structure * d4s,int flag);

#if 0
//////////////////////////////////////////////////////////////////////////////////////////
/// bool depth_first(hid_t pid, char *gname, DMR & dmr, D4Group* par_grp, const char *fname)
/// \param pid group id
/// \param gname group name (the absolute path from the root group)
/// \param dmr reference of DMR object
//  \param par_grp DAP4 parent group
/// \param fname the HDF5 file name
///
/// \return 0, if failed.
/// \return 1, if succeeded.
///
/// \remarks hard link is treated as a dataset.
/// \remarks will return error message to the DAP interface.
/// \see depth_first(hid_t pid, char *gname, DDS & dds, const char *fname) in h5dds.cc
///////////////////////////////////////////////////////////////////////////////

//bool depth_first(hid_t pid, char *gname, DMR & dmr, D4Group* par_grp, const char *fname)
bool depth_first(hid_t pid, char *gname,  D4Group* par_grp, const char *fname)
{
    BESDEBUG("h5",
        ">depth_first() for dmr " 
        << " pid: " << pid
        << " gname: " << gname
        << " fname: " << fname
        << endl);
        
    /// To keep track of soft links.
    int slinkindex = 0;

    H5G_info_t g_info; 
    hsize_t nelems = 0;

    /// Obtain the number of all links.
    if(H5Gget_info(pid,&g_info) <0) {
      string msg =
            "h5_dmr handler: counting hdf5 group elements error for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    nelems = g_info.nlinks;
        
    ssize_t oname_size = 0;

    // Iterate through the file to see the members of the group from the root.
    for (hsize_t i = 0; i < nelems; i++) {

        vector <char>oname;

        // Query the length of object name.
        oname_size =
 	    H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,nullptr,
                (size_t)DODS_NAMELEN, H5P_DEFAULT);
        if (oname_size <= 0) {
            string msg = "h5_dmr handler: Error getting the size of the hdf5 object from the group: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the name of the object
        oname.resize((size_t) oname_size + 1);

        if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname.data(),
            (size_t)(oname_size+1), H5P_DEFAULT) < 0){
            string msg =
                    "h5_dmr handler: Error getting the hdf5 object name from the group: ";
             msg += gname;
                throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Check if it is the hard link or the soft link
        H5L_info_t linfo;
        if (H5Lget_info(pid,oname.data(),&linfo,H5P_DEFAULT)<0) {
            string msg = "hdf5 link name error from: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }
            
        // Information of soft links are stored as attributes 
        if(linfo.type == H5L_TYPE_SOFT) { 
            slinkindex++;
            size_t val_size = linfo.u.val_size;
            get_softlink(par_grp,pid,oname.data(),slinkindex,val_size);
            //get_softlink(par_grp,pid,gname,oname.data(),slinkindex,val_size);
            continue;
        }

        // Ignore external links
        if(linfo.type == H5L_TYPE_EXTERNAL) 
            continue;

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;

        if (H5OGET_INFO_BY_IDX(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
            string msg = "h5_dmr handler: Error obtaining the info for the object";
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

                BESDEBUG("h5", "=depth_first dmr ():H5G_GROUP " << full_path_name
                    << endl);

                vector <char>t_fpn;
                t_fpn.resize(full_path_name.size()+1);
                copy(full_path_name.begin(),full_path_name.end(),t_fpn.begin());
                t_fpn[full_path_name.size()] = '\0';

                hid_t cgroup = H5Gopen(pid, t_fpn.data(),H5P_DEFAULT);
                if (cgroup < 0){
                     throw InternalErr(__FILE__, __LINE__, "h5_dmr handler: H5Gopen() failed.");
		}

                string grp_name = string(oname.begin(),oname.end()-1);

                // Check the hard link loop and break the loop if it exists.
                string oid = get_hardlink_dmr(cgroup, full_path_name.c_str());
                if (oid == "") {
                    try {
                        D4Group* tem_d4_cgroup = new D4Group(grp_name);
                        // Map the HDF5 cgroup attributes to DAP4 group attributes.
                        // Note the last flag of map_h5_attrs_to_dap4 must be 0 for the group attribute mapping.
                        map_h5_attrs_to_dap4(cgroup,tem_d4_cgroup,nullptr,nullptr,0);

                        // Add this new DAP4 group 
                        par_grp->add_group_nocopy(tem_d4_cgroup);

                        // Continue searching the objects under this group
                        //depth_first(cgroup, t_fpn.data(), dmr, tem_d4_cgroup,fname);
                        depth_first(cgroup, t_fpn.data(), tem_d4_cgroup,fname);
                    }
                    catch(...) {
                        H5Gclose(cgroup);
                        throw;
                    }
                }
                else {
                    // This group has been visited.  
                    // Add the attribute table with the attribute name as HDF5_HARDLINK.
                    // The attribute value is the name of the group when it is first visited.
                    D4Group* tem_d4_cgroup = new D4Group(string(grp_name));

                    // Note attr_str_c is the DAP4 attribute string datatype
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
                string full_path_name = string(gname) + string(oname.begin(),oname.end()-1);

                // TOOOODOOOO
                // Obtain the hdf5 dataset handle stored in the structure dt_inst. 
                // All the metadata information in the handler is stored in dt_inst.
                // Work on this later, redundant for dmr since dataset is opened twice. KY 2015-07-01
                // Note: depth_first is for building DMR of an HDF5 file that doesn't use dim. scale.
                // so passing the last parameter as false.
                get_dataset(pid, full_path_name, &dt_inst,false);
               
                // Here we open the HDF5 dataset again to use the dataset id for dataset attributes.
                // This is not necessary for DAP2 since DAS and DDS are separated.
                hid_t dset_id = -1;
                if((dset_id = H5Dopen(pid,full_path_name.c_str(),H5P_DEFAULT)) <0) {
                   string msg = "cannot open the HDF5 dataset  ";
                   msg += full_path_name;
                   throw InternalErr(__FILE__, __LINE__, msg);
                }

                try {
                    read_objects(par_grp, full_path_name, fname,dset_id);
                }
                catch(...) {
                    H5Dclose(dset_id);
                    throw;
                }
                if(H5Dclose(dset_id)<0) {
                    string msg = "cannot close the HDF5 dataset  ";
                   msg += full_path_name;
                   throw InternalErr(__FILE__, __LINE__, msg);
                }
            }
                break;

            case H5O_TYPE_NAMED_DATATYPE:
                // ignore the named datatype
                break;
            default:
                break;
        }// switch(obj_type)
    } // for i is 0 ... nelems

    BESDEBUG("h5", "<depth_first() for dmr" << endl);
    return true;
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////
/// bool breadth_first(const hid_t file_id,hid_t pid, const char *gname, DMR & dmr, D4Group* par_grp, const char *fname,bool use_dimscale, vector <link_info_t> & hdf5_hls)
/// \param file_id file_id(this is necessary for searching the hardlinks of a dataset)
/// \param pid group id
/// \param gname group name (the absolute path from the root group)
/// \param dmr reference of DMR object
//  \param par_grp DAP4 parent group
/// \param fname the HDF5 file name
/// \param use_dimscale whether dimension scales are used.
/// \param hdf5_hls the vector to save all the hardlink info.
/// \return 0, if failed.
/// \return 1, if succeeded.
///
/// \remarks hard link is treated as a dataset.
/// \remarks will return error message to the DAP interface.
/// \remarks The reason to use breadth_first is that the DMR representation needs to show the dimension names and the variables under the group first and then the group names.
///  So we use this search. In the future, we may just use the breadth_first search for all cases.??
/// \see depth_first(hid_t pid, char *gname, DMR & dmr, const char *fname) in h5dds.cc
///////////////////////////////////////////////////////////////////////////////


// The reason to use breadth_first is that the DMR representation needs to show the dimension names and the variables under the group first and then the group names.
// So we use this search. In the future, we may just use the breadth_first search for all cases.?? 
//bool breadth_first(hid_t pid, char *gname, DMR & dmr, D4Group* par_grp, const char *fname,bool use_dimscale)
bool breadth_first(const hid_t file_id, hid_t pid, const char *gname, D4Group* par_grp, const char *fname,bool use_dimscale,vector<link_info_t> & hdf5_hls )
{
    BESDEBUG("h5",
        ">breadth_first() for dmr " 
        << " pid: " << pid
        << " gname: " << gname
        << " fname: " << fname
        << endl);
        
    /// To keep track of soft links.
    int slinkindex = 0;

    // Obtain the number of objects in this group
    H5G_info_t g_info; 
    hsize_t nelems = 0;
    if(H5Gget_info(pid,&g_info) <0) {
      string msg =
            "h5_dmr handler: counting hdf5 group elements error for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    nelems = g_info.nlinks;
        
    ssize_t oname_size;

    // First iterate through the HDF5 datasets under the group.
    for (hsize_t i = 0; i < nelems; i++) {

        vector <char>oname;

        // Query the length of object name.
        oname_size =
        H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,nullptr,
                (size_t)DODS_NAMELEN, H5P_DEFAULT);
        if (oname_size <= 0) {
            string msg = "h5_dmr handler: Error getting the size of the hdf5 object from the group: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the name of the object
        oname.resize((size_t) oname_size + 1);

        if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname.data(),
            (size_t)(oname_size+1), H5P_DEFAULT) < 0){
            string msg =
                    "h5_dmr handler: Error getting the hdf5 object name from the group: ";
             msg += gname;
                throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Check if it is the hard link or the soft link
        H5L_info_t linfo;
        if (H5Lget_info(pid,oname.data(),&linfo,H5P_DEFAULT)<0) {
            string msg = "hdf5 link name error from: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }
            
        // Information of soft links are stored as attributes 
        if(linfo.type == H5L_TYPE_SOFT) { 
            slinkindex++;

            // Size of a soft link value
            size_t val_size = linfo.u.val_size;
            get_softlink(par_grp,pid,oname.data(),slinkindex,val_size);
            continue;
         }

        // Ignore external links
        if(linfo.type == H5L_TYPE_EXTERNAL) 
            continue;

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;

        if (H5OGET_INFO_BY_IDX(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
            string msg = "h5_dmr handler: Error obtaining the info for the object";
            msg += string(oname.begin(),oname.end());
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        H5O_type_t obj_type = oinfo.type;

        if(H5O_TYPE_DATASET == obj_type) {

            // Obtain the absolute path of the HDF5 dataset
            string full_path_name = string(gname) + string(oname.begin(),oname.end()-1);

            // TOOOODOOOO
            // Obtain the hdf5 dataset handle stored in the structure dt_inst. 
            // All the metadata information in the handler is stored in dt_inst.
            // Work on this later, redundant for dmr since dataset is opened twice. KY 2015-07-01
            // Dimension scale is handled in this routine. So need to keep it. KY 2020-06-10
            bool is_pure_dim = false;
            get_dataset_dmr(file_id, pid, full_path_name, &dt_inst,use_dimscale,is_pure_dim,hdf5_hls);
               
            if(false == is_pure_dim) {
                hid_t dset_id = -1;
                if((dset_id = H5Dopen(pid,full_path_name.c_str(),H5P_DEFAULT)) <0) {
                    string msg = "cannot open the HDF5 dataset  ";
                    msg += full_path_name;
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                try {
                    read_objects(par_grp, full_path_name, fname,dset_id);
                }
                catch(...) {
                    H5Dclose(dset_id);
                    throw;
                }
                if(H5Dclose(dset_id)<0) {
                    string msg = "cannot close the HDF5 dataset  ";
                    msg += full_path_name;
                    throw InternalErr(__FILE__, __LINE__, msg);
                }
            }
            else {
                //Need to add this pure dimension to the corresponding DAP4 group
                D4Dimensions *d4_dims = par_grp->dims();
                auto d4dim_name = string(oname.begin(),oname.end()-1);   
                D4Dimension *d4_dim = d4_dims->find_dim(d4dim_name);
                if(d4_dim == nullptr) {
                    d4_dim = new D4Dimension(d4dim_name,dt_inst.nelmts);
                    d4_dims->add_dim_nocopy(d4_dim);
                }
                BESDEBUG("h5", "<h5dmr.cc: pure dimension: dataset name." << d4dim_name << endl);
                if(H5Tclose(dt_inst.type)<0) {
                      throw InternalErr(__FILE__, __LINE__, "Cannot close the HDF5 datatype.");       
                }
            }
 
        } 
    }
   
    // The attributes of this group. Doing this order to follow ncdump's way (variable,attribute then groups)
    map_h5_attrs_to_dap4(pid,par_grp,nullptr,nullptr,0);

    // Then HDF5 child groups
    for (hsize_t i = 0; i < nelems; i++) {

        vector <char>oname;

        // Query the length of object name.
        oname_size =
        H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,nullptr,
                (size_t)DODS_NAMELEN, H5P_DEFAULT);
        if (oname_size <= 0) {
            string msg = "h5_dmr handler: Error getting the size of the hdf5 object from the group: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the name of the object
        oname.resize((size_t) oname_size + 1);

        if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname.data(),
            (size_t)(oname_size+1), H5P_DEFAULT) < 0){
            string msg =
                    "h5_dmr handler: Error getting the hdf5 object name from the group: ";
             msg += gname;
                throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Check if it is the hard link or the soft link
        H5L_info_t linfo;
        if (H5Lget_info(pid,oname.data(),&linfo,H5P_DEFAULT)<0) {
            string msg = "hdf5 link name error from: ";
            msg += gname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }
            
        // Information of soft links are handled already, the softlinks need to be ignored, otherwise
        // the group it links will be mapped again in the block of if obj_type is H5O_TYPE_GROUP 
        if(linfo.type == H5L_TYPE_SOFT) { 
            continue;
        }

        // Ignore external links
        if(linfo.type == H5L_TYPE_EXTERNAL) 
            continue;

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;

        if (H5OGET_INFO_BY_IDX(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
            string msg = "h5_dmr handler: Error obtaining the info for the object in the breadth_first.";
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        H5O_type_t obj_type = oinfo.type;


        if(obj_type == H5O_TYPE_GROUP) {  

            // Obtain the full path name
            string full_path_name =
                string(gname) + string(oname.begin(),oname.end()-1) + "/";

            BESDEBUG("h5", "=breadth_first dmr ():H5G_GROUP " << full_path_name
                << endl);

            vector <char>t_fpn;
            t_fpn.resize(full_path_name.size()+1);
            copy(full_path_name.begin(),full_path_name.end(),t_fpn.begin());
            t_fpn[full_path_name.size()] = '\0';

            hid_t cgroup = H5Gopen(pid, t_fpn.data(),H5P_DEFAULT);
            if (cgroup < 0){
                throw InternalErr(__FILE__, __LINE__, "h5_dmr handler: H5Gopen() failed.");
            }

            auto grp_name = string(oname.begin(),oname.end()-1);

            // Check the hard link loop and break the loop if it exists.
            string oid = get_hardlink_dmr(cgroup, full_path_name.c_str());
            if (oid == "") {
                try {
                    auto tem_d4_cgroup = new D4Group(grp_name);

                    // Add this new DAP4 group 
                    par_grp->add_group_nocopy(tem_d4_cgroup);

                    // Continue searching the objects under this group
                    breadth_first(file_id,cgroup, t_fpn.data(), tem_d4_cgroup,fname,use_dimscale,hdf5_hls);
                }
                catch(...) {
                    H5Gclose(cgroup);
                    throw;
                }
            }
            else {
                // This group has been visited.  
                // Add the attribute table with the attribute name as HDF5_HARDLINK.
                // The attribute value is the name of the group when it is first visited.
                D4Group* tem_d4_cgroup = new D4Group(string(grp_name));

                // Note attr_str_c is the DAP4 attribute string datatype
                D4Attribute *d4_hlinfo = new D4Attribute("HDF5_HARDLINK",attr_str_c);

                d4_hlinfo->add_value(obj_paths.get_name(oid));
                tem_d4_cgroup->attributes()->add_attribute_nocopy(d4_hlinfo);
                par_grp->add_group_nocopy(tem_d4_cgroup);
            }

            if (H5Gclose(cgroup) < 0){
                throw InternalErr(__FILE__, __LINE__, "Could not close the group.");
            }
        }// end if
    } // for i is 0 ... nelems

    BESDEBUG("h5", "<breadth_first() " << endl);
    return true;
}

/////////////////////////////////////////////////////////////////////////////// 
///// \fn read_objects(DMR & dmr, D4Group *d4_grp, 
/////                            const string & varname, 
/////                            const string & filename,const hid_t dset_id) 
///// fills in information of a dataset (name, data type, data space) into the dap4 
///// group. 
///// This is a wrapper function that calls functions to read atomic types and structure.
///// 
/////    \param dmr reference to DMR 
/////    \param d4_group DAP4 group
/////    \param varname Absolute name of an HDF5 dataset.  
/////    \param filename The HDF5 dataset name that maps to the DDS dataset name. 
////     \param dset_id HDF5 dataset id.
/////    \throw error a string of error message to the dods interface. 
/////////////////////////////////////////////////////////////////////////////////
//
void
read_objects( D4Group * d4_grp, const string &varname, const string &filename, const hid_t dset_id)
{

    switch (H5Tget_class(dt_inst.type)) {

    // HDF5 compound maps to DAP structure.
    case H5T_COMPOUND:
        read_objects_structure(d4_grp, varname, filename,dset_id);
        break;

    case H5T_ARRAY:
        H5Tclose(dt_inst.type);
        throw InternalErr(__FILE__, __LINE__, "Currently don't support accessing data of Array datatype when array datatype is not inside the compound.");       
    
    default:
        read_objects_base_type(d4_grp,varname, filename,dset_id);
        break;
    }
    // We must close the datatype obtained in the get_dataset routine since this is the end of reading DDS.
    if(H5Tclose(dt_inst.type)<0) {
        throw InternalErr(__FILE__, __LINE__, "Cannot close the HDF5 datatype.");       
    }
}

/////////////////////////////////////////////////////////////////////////////// 
///// \fn read_objects_base_type(DMR & dmr, D4Group *d4_grp, 
/////                            const string & varname, 
/////                            const string & filename,const hid_t dset_id) 
///// fills in information of a dataset (name, data type, data space) with HDF5 atomic datatypes into the dap4 
///// group. 
///// 
/////    \param dmr reference to DMR 
/////    \param d4_grp DAP4 group
/////    \param varname Absolute name of an HDF5 dataset.  
/////    \param filename The HDF5 dataset name that maps to the DDS dataset name. 
////     \param dset_id HDF5 dataset id.
/////    \throw error a string of error message to the dods interface. 
/////////////////////////////////////////////////////////////////////////////////
//

//void
//read_objects_base_type(DMR & dmr, D4Group * d4_grp,const string & varname,
void
read_objects_base_type(D4Group * d4_grp,const string & varname,
                       const string & filename,hid_t dset_id)
{

    // Obtain the relative path of the variable name under the leaf group
    string newvarname = HDF5CFUtil::obtain_string_after_lastslash(varname);

    // Get a base type. It should be an HDF5 atomic datatype
    // datatype. 
    BaseType *bt = Get_bt(newvarname, varname,filename, dt_inst.type,true);
    if (!bt) {
        throw
            InternalErr(__FILE__, __LINE__,
                        "Unable to convert hdf5 datatype to dods basetype");
    }

    // First deal with scalar data. 
    if (dt_inst.ndims == 0) {
        // transform the DAP2 to DAP4 for this DAP base type and add it to d4_grp
        bt->transform_to_dap4(d4_grp,d4_grp);
        // Get it back - this may return null because the underlying type
        // may have no DAP2 manifestation.
        BaseType* new_var = d4_grp->var(bt->name());
        if(new_var){
            // Map the HDF5 dataset attributes to DAP4
            map_h5_attrs_to_dap4(dset_id,nullptr,new_var,nullptr,1);
            // If this variable is a hardlink, stores the HARDLINK info. as an attribute.
            map_h5_dset_hardlink_to_d4(dset_id,varname,new_var,nullptr,1);
        }
        delete bt; 
        bt = nullptr;
    }
    else {
        // Next, deal with Array data. This 'else clause' runs to
        // the end of the method. 
        auto ar = new HDF5Array(newvarname, filename, bt);
        delete bt; bt = nullptr;

        // set number of elements and variable name values.
        // This essentially stores in the struct.
        ar->set_memneed(dt_inst.need);
        ar->set_numdim(dt_inst.ndims);
        ar->set_numelm((int) (dt_inst.nelmts));
        ar->set_varpath(varname);

 
        // If we have dimension names(dimension scale is used.),we will see if we can add the names.       
        int dimnames_size = 0;
        if((unsigned int)((int)(dt_inst.dimnames.size())) != dt_inst.dimnames.size())
        {
            delete ar;
            throw
            InternalErr(__FILE__, __LINE__,
                        "number of dimensions: overflow");
        }
        dimnames_size = (int)(dt_inst.dimnames.size());
#if 0
//cerr<<"dimnames_size is "<<dimnames_size <<endl;
//cerr<<"ndims is "<<dt_inst.ndims <<endl;
#endif
            
        if(dimnames_size ==dt_inst.ndims) {

            for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
                if(dt_inst.dimnames[dim_index] !="") 
                    ar->append_dim(dt_inst.size[dim_index],dt_inst.dimnames[dim_index]);
                else 
                    ar->append_dim(dt_inst.size[dim_index]);
                    // D4dimension has to have a name. If no name, no D4dimension(from comments libdap4: Array.cc) 
            }
            dt_inst.dimnames.clear();
        }
        else {
            // For DAP4, no need to add dimension if no dimension name
            for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) 
                ar->append_dim(dt_inst.size[dim_index]); 
        }

        // We need to transform dimension info. to DAP4 group
        BaseType* new_var = nullptr;
        try {
            new_var = ar->h5dims_transform_to_dap4(d4_grp,dt_inst.dimnames_path);
        }
        catch(...) {
            delete ar;
            throw;
        }

        // clear DAP4 dimnames_path vector
        dt_inst.dimnames_path.clear();

        // Map HDF5 dataset attributes to DAP4
        map_h5_attrs_to_dap4(dset_id,nullptr,new_var,nullptr,1);

        // If this is a hardlink, map the Hardlink info. as an DAP4 attribute.
        map_h5_dset_hardlink_to_d4(dset_id,varname,new_var,nullptr,1);
#if 0
        // Test the attribute
        D4Attribute *test_attr = new D4Attribute("DAP4_test",attr_str_c);
        test_attr->add_value("test_grp_attr");
        new_var->attributes()->add_attribute_nocopy(test_attr);
#endif
        // Add this var to DAP4 group.
        d4_grp->add_var_nocopy(new_var);
        delete ar; ar = nullptr;
    }
    BESDEBUG("h5", "<read_objects_base_type(dmr)" << endl);

}

///////////////////////////////////////////////////////////////////////////////
/// \fn read_objects_structure(DMR & dmr,D4Group *d4_grp,const string & varname,
///                  const string & filename,hid_t dset_id)
/// fills in information of a structure dataset (name, data type, data space)
/// into a DAP4 group. HDF5 compound datatype will map to DAP structure.
/// 
///    \param dmr DMR for the HDF5 objects. 
///    \param d4_grp DAP4 group
///    \param varname Absolute name of structure
///    \param filename The HDF5 file  name that maps to the DDS dataset name.
///    \param dset_id HDF5 dataset ID
///    \throw error a string of error message to the dods interface.
///////////////////////////////////////////////////////////////////////////////
void
read_objects_structure(D4Group *d4_grp, const string & varname,
                       const string & filename,hid_t dset_id)
{
    // Obtain the relative path of the variable name under the leaf group
    string newvarname = HDF5CFUtil::obtain_string_after_lastslash(varname);

    // Map HDF5 compound datatype to Structure
    Structure *structure = Get_structure(newvarname, varname,filename, dt_inst.type,true);

    try {
        BESDEBUG("h5", "=read_objects_structure(): Dimension is " 
            << dt_inst.ndims << endl);

        if (dt_inst.ndims != 0) {   // Array of Structure
            BESDEBUG("h5", "=read_objects_structure(): array of size " <<
                dt_inst.nelmts << endl);
            BESDEBUG("h5", "=read_objects_structure(): memory needed = " <<
                dt_inst.need << endl);

            // Create the Array of structure.
            auto ar = new HDF5Array(newvarname, filename, structure);
            delete structure; structure = nullptr;


            // These parameters are used in the data read function.
            ar->set_memneed(dt_inst.need);
            ar->set_numdim(dt_inst.ndims);
            ar->set_numelm((int) (dt_inst.nelmts));
            ar->set_length((int) (dt_inst.nelmts));
            ar->set_varpath(varname);
 
            // If having dimension names, add the dimension names to DAP.
            int dimnames_size = 0;
            if((unsigned int)((int)(dt_inst.dimnames.size())) != dt_inst.dimnames.size())
            {
                delete ar;
                throw
                   InternalErr(__FILE__, __LINE__,
                               "number of dimensions: overflow");
            }
            dimnames_size = (int)(dt_inst.dimnames.size());
                

            if(dimnames_size ==dt_inst.ndims) {
                for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
                    if(dt_inst.dimnames[dim_index] !="")
                        ar->append_dim(dt_inst.size[dim_index],dt_inst.dimnames[dim_index]);
                    else 
                        ar->append_dim(dt_inst.size[dim_index]);
                }
                dt_inst.dimnames.clear();
            }
            else {
                for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) 
                    ar->append_dim(dt_inst.size[dim_index]);
                    
            }

            // We need to transform dimension info. to DAP4 group
            BaseType* new_var = ar->h5dims_transform_to_dap4(d4_grp,dt_inst.dimnames_path);
            dt_inst.dimnames_path.clear();

            // Map HDF5 dataset attributes to DAP4
            map_h5_attrs_to_dap4(dset_id,nullptr,new_var,nullptr,1);

            // If this is a hardlink, map the Hardlink info. as an DAP4 attribute.
            map_h5_dset_hardlink_to_d4(dset_id,varname,new_var,nullptr,1);

            // Add this var to DAP4 group
            if(new_var) 
                d4_grp->add_var_nocopy(new_var);
            delete ar; ar = nullptr;
        }//  end if 
        else {// A scalar structure

            structure->set_is_dap4(true);
            map_h5_attrs_to_dap4(dset_id,nullptr,nullptr,structure,2);
            map_h5_dset_hardlink_to_d4(dset_id,varname,nullptr,structure,2);
            if(structure) 
                d4_grp->add_var_nocopy(structure);
        }
    } // try  Structure 
    catch (...) {
        delete structure;
        throw;
    }
}


///////////////////////////////////////////////////////////////////////////////// 
///// \fn map_h5_attrs_to_dap4(hid_t h5_objid,D4Group* d4g,BaseType* d4b,Structure * d4s,int flag)
///// Map HDF5 attributes to DAP4 
///// 
/////    \param h5_objid  HDF5 object ID(either group or dataset)
/////    \param d4g DAP4 group if the object is group, flag must be 0 if d4_grp is not nullptr.
/////    \param d4b DAP BaseType if the object is dataset and the datatype of the object is atomic datatype.  
/////               The flag must be 1 if d4b is not nullptr.
/////    \param d4s DAP Structure if the object is dataset and the datatype of the object is compound. 
/////               The flag must be 2 if d4s is not nullptr.
////     \param flag flag to determine what kind of objects to map. The value must be 0,1 or 2.
/////    \throw error a string of error message to the dods interface. 
/////////////////////////////////////////////////////////////////////////////////
//

void map_h5_attrs_to_dap4(hid_t h5_objid,D4Group* d4g,BaseType* d4b,Structure * d4s,int flag) {

    // Get the object info
    H5O_info_t obj_info;
    if (H5OGET_INFO(h5_objid, &obj_info) <0) {
        string msg = "Fail to obtain the HDF5 object info. .";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Obtain the number of attributes
    auto num_attr = (int)(obj_info.num_attrs);
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

        // Get the corresponding DAP data type of the HDF5 datatype.
        // The following line doesn't work in HDF5 1.10.
#if 0
        //hid_t ty_id = attr_inst.type;
#endif
        hid_t ty_id = H5Aget_type(attr_id);
        if(ty_id <0) {
            H5Aclose(attr_id);
            throw InternalErr(__FILE__, __LINE__, "Cannot retrieve HDF5 attribute datatype successfully.");
        }

        string dap_type = get_dap_type(ty_id,true);

        // Need to have DAP4 representation of the attribute type
        D4AttributeType dap4_attr_type = daptype_strrep_to_dap4_attrtype(dap_type);

        // We encounter an unsupported DAP4 attribute type.
        if(attr_null_c == dap4_attr_type) {
            H5Tclose(ty_id);
            H5Aclose(attr_id);
            throw InternalErr(__FILE__, __LINE__, "unsupported DAP4 attribute type");
        }

        string attr_name = attr_inst.name;
        BESDEBUG("h5", "arttr_name= " << attr_name << endl);

        // Create the DAP4 attribute mapped from HDF5
        auto d4_attr = new D4Attribute(attr_name,dap4_attr_type);

        // We have to handle variable length string differently. 
        if (H5Tis_variable_str(ty_id))  
            write_vlen_str_attrs(attr_id,ty_id,&attr_inst,d4_attr,nullptr,true);
        else {

            vector<char> value;
#if 0
            //value.resize(attr_inst.need + sizeof(char));
#endif
            value.resize(attr_inst.need);
            BESDEBUG("h5", "arttr_inst.need=" << attr_inst.need << endl);
  
            // Need to obtain the memtype since we still find BE data.
            hid_t memtype = H5Tget_native_type(ty_id, H5T_DIR_ASCEND);
            // Read HDF5 attribute data.
            if (H5Aread(attr_id, memtype, (void *) (value.data())) < 0) {
                delete d4_attr;
                throw InternalErr(__FILE__, __LINE__, "unable to read HDF5 attribute data");
            }
            H5Aclose(memtype);

            // For scalar data, just read data once.
            if (attr_inst.ndims == 0) {
                for (int loc = 0; loc < (int) attr_inst.nelmts; loc++) {
                    print_rep = print_attr(ty_id, loc, value.data());
                    if (print_rep.c_str() != nullptr) {
                        d4_attr->add_value(print_rep);
                    }
                }

            }
            else {// The number of dimensions is > 0

                // Get the attribute datatype size
                int elesize = (int) H5Tget_size(ty_id);
                if (elesize == 0) {
                    H5Tclose(ty_id);
                    H5Aclose(attr_id); 
                    delete d4_attr;
                    throw InternalErr(__FILE__, __LINE__, "unable to get attibute size");
                }

                // Due to the implementation of print_attr, the attribute value will be 
                // written one by one.
                char *tempvalue = value.data();

                // Write this value. the "loc" can always be set to 0 since
                // tempvalue will be moved to the next value.
                for( hsize_t temp_index = 0; temp_index < attr_inst.nelmts; temp_index ++) {
                     print_rep = print_attr(ty_id, 0, tempvalue);
                    if (print_rep.c_str() != nullptr) {

                        BESDEBUG("h5", "print_rep= " << print_rep << endl);

                        d4_attr->add_value(print_rep);
                        tempvalue = tempvalue + elesize;
                        BESDEBUG("h5",
                                 "tempvalue= " << tempvalue
                                 << "elesize=" << elesize
                                 << endl);

                    }
                    else {
                        H5Tclose(ty_id);
                        H5Aclose(attr_id);
                        delete d4_attr;
                        throw InternalErr(__FILE__, __LINE__, "unable to convert attibute value to DAP");
                    }
                }//for(hsize_t temp_index=0; .....
            } // if attr_inst.ndims != 0
        }
        if(H5Tclose(ty_id) < 0) {
            H5Aclose(attr_id);
            delete d4_attr;
            throw InternalErr(__FILE__, __LINE__, "unable to close HDF5 type id");
        }
        if (H5Aclose(attr_id) < 0) {
            delete d4_attr;
            throw InternalErr(__FILE__, __LINE__, "unable to close attibute id");
        }

        if(0 == flag) // D4group
            d4g->attributes()->add_attribute_nocopy(d4_attr);
        else if (1 == flag) // HDF5 dataset with atomic datatypes 
            d4b->attributes()->add_attribute_nocopy(d4_attr);
        else if ( 2 == flag) // HDF5 dataset with compound datatype
            d4s->attributes()->add_attribute_nocopy(d4_attr);
        else {
            stringstream sflag;
            sflag << flag;
            string msg ="The add_dap4_attr flag has to be either 0,1 or 2.";
            msg+="The current flag is "+sflag.str();
            delete d4_attr;
            throw InternalErr(__FILE__, __LINE__, msg);
        }
    } // for (int j = 0; j < num_attr; j++)

    return;
}

///////////////////////////////////////////////////////////////////////////////// 
///// \fn map_h5_dset_hardlink_to_dap4(hid_t h5_dsetid,const string& full_path,BaseType* d4b,Structure * d4s,int flag)
///// Map HDF5 dataset hardlink info to a DAP4 attribute
///// 
/////    \param h5_dsetid  HDF5 dataset ID
/////    \param full_path  full_path of the HDF5 dataset
/////    \param d4b DAP BaseType if the object is dataset and the datatype of the object is atomic datatype.  
/////               The flag must be 1 if d4b is not nullptr.
/////    \param d4s DAP Structure if the object is dataset and the datatype of the object is compound. 
/////               The flag must be 2 if d4s is not nullptr.
////     \param flag flag to determine what kind of objects to map. The value must be 1 or 2.
/////    \throw error a string of error message to the dods interface. 
/////////////////////////////////////////////////////////////////////////////////


void map_h5_dset_hardlink_to_d4(hid_t h5_dsetid,const string & full_path, BaseType* d4b,Structure * d4s,int flag) {

    // Obtain the unique object number info. If no hardlinks, empty string will return.
    string oid = get_hardlink_dmr(h5_dsetid, full_path);

    // Find that this is a hardlink,add the hardlink info to a DAP4 attribute.
    if(false == oid.empty()) {

        D4Attribute *d4_hlinfo = new D4Attribute("HDF5_HARDLINK",attr_str_c);
        d4_hlinfo->add_value(obj_paths.get_name(oid));
 
        if (1 == flag) 
            d4b->attributes()->add_attribute_nocopy(d4_hlinfo);
        else if ( 2 == flag)
            d4s->attributes()->add_attribute_nocopy(d4_hlinfo);
        else 
            delete d4_hlinfo;
    }

}
 
///////////////////////////////////////////////////////////////////////////////
/// \fn get_softlink(D4Group* par_grp, hid_t h5obj_id, const string & oname, int index,size_t val_size)
/// will put softlink information into DAP4.
///
/// \param par_grp DAP4 group
/// \param h5_obj_id object id
/// \param oname object name: absolute name of a group
/// \param index Link index
/// \param val_size value size
/// \return void
/// \remarks In case of error, it throws an exception
///////////////////////////////////////////////////////////////////////////////
void get_softlink(D4Group* par_grp, hid_t h5obj_id,  const string & oname, int index, size_t val_size)
{
    BESDEBUG("h5", "dap4 >get_softlink():" << oname << endl);

    ostringstream oss;
    oss << string("HDF5_SOFTLINK");
    oss << "_";
    oss << index;
    string temp_varname = oss.str();


    BESDEBUG("h5", "dap4->get_softlink():" << temp_varname << endl);
    auto d4_slinfo = new D4Attribute;
    d4_slinfo->set_name(temp_varname);

    // Make the type as a container
    d4_slinfo->set_type(attr_container_c);

    string softlink_name = "linkname";

    auto softlink_src = new D4Attribute(softlink_name,attr_str_c);
    softlink_src->add_value(oname);

    d4_slinfo->attributes()->add_attribute_nocopy(softlink_src);
    string softlink_value_name ="LINKTARGET";
   
    // Get the link target information. We always return the link value in a string format.
    D4Attribute *softlink_tgt = nullptr;

    try {
        vector<char> buf;
        buf.resize(val_size + 1);

        // get link target name
        if (H5Lget_val(h5obj_id, oname.c_str(), (void*) buf.data(), val_size + 1, H5P_DEFAULT) < 0) {
            throw InternalErr(__FILE__, __LINE__, "unable to get link value");
        }
        softlink_tgt = new D4Attribute(softlink_value_name, attr_str_c);
        auto link_target_name = string(buf.begin(), buf.end());
        softlink_tgt->add_value(link_target_name);

        d4_slinfo->attributes()->add_attribute_nocopy(softlink_tgt);
    }
    catch (...) {
        delete softlink_tgt;
        throw;
    }

    par_grp->attributes()->add_attribute_nocopy(d4_slinfo);
}


///////////////////////////////////////////////////////////////////////////////
/// \fn get_hardlink(hid_t h5obj_id, const string & oname)
/// will put hardlink information into a DAS table.
///
/// \param h5obj_id object id
/// \param oname object name: absolute name of a group
///
/// \return true  if succeeded.
/// \return false if failed.
/// \remarks In case of error, it returns a string of error message
///          to the DAP interface.
///////////////////////////////////////////////////////////////////////////////
string get_hardlink_dmr( hid_t h5obj_id, const string & oname) {
    
    BESDEBUG("h5", "dap4->get_hardlink_dmr():" << oname << endl);

    // Get the object info
    H5O_info_t obj_info;
    if (H5OGET_INFO(h5obj_id, &obj_info) <0) { 
        throw InternalErr(__FILE__, __LINE__, "H5OGET_INFO() failed.");
    }

    // If the reference count is greater than 1,that means 
    // hard links are found. return the original object name this
    // hard link points to. 

    if (obj_info.rc >1) {

        string objno;

#if (H5_VERS_MAJOR == 1 && ((H5_VERS_MINOR == 12) || (H5_VERS_MINOR == 13)))
        char *obj_tok_str = nullptr;
        if(H5Otoken_to_str(h5obj_id, &(obj_info.token), &obj_tok_str) <0) {
            throw InternalErr(__FILE__, __LINE__, "H5Otoken_to_str failed.");
        } 
        objno.assign(obj_tok_str,obj_tok_str+strlen(obj_tok_str));
        H5free_memory(obj_tok_str);

#else
        ostringstream oss;
        oss << hex << obj_info.addr;
        objno = oss.str();
#endif

        BESDEBUG("h5", "dap4->get_hardlink_dmr() objno=" << objno << endl);

        // Add this hard link to the map.
        // obj_paths is a global variable defined at the beginning of this file.
        // it is essentially a id to obj name map. See HDF5PathFinder.h.
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
