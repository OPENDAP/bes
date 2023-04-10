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
#include <libdap/D4Maps.h>

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
#include "HDF5RequestHandler.h"

// The HDF5CFUtil.h includes the utility function obtain_string_after_lastslash.
#include "HDF5CFUtil.h"
#include "h5dmr.h"

#include "he5dds.tab.hh"
#include "HE5Parser.h"
#include "HE5Checker.h"

using namespace std;
using namespace libdap;
/// A variable for remembering visited paths to break cyclic HDF5 groups. 
HDF5PathFinder obj_paths;


/// An instance of DS_t structure defined in hdf5_handler.h.
static DS_t dt_inst; 

struct yy_buffer_state;

yy_buffer_state *he5dds_scan_string(const char *str);
int he5ddsparse(HE5Parser *he5parser);
int he5ddslex_destroy();


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
//bool breadth_first(const hid_t file_id, hid_t pid, const char *gname, D4Group* par_grp, const char *fname,bool use_dimscale,bool is_eos5, vector<link_info_t> & hdf5_hls,unordered_map<string, vector<string>>& varpath_to_dims)
bool breadth_first(const hid_t file_id, hid_t pid, const char *gname, D4Group* par_grp, const char *fname,bool use_dimscale,bool is_eos5, vector<link_info_t> & hdf5_hls,const eos5_dim_info_t & eos5_dim_info, vector<string> & handled_cv_names)
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
            get_dataset_dmr(file_id, pid, full_path_name, &dt_inst,use_dimscale,is_eos5,is_pure_dim,hdf5_hls,handled_cv_names);
               
            if(false == is_pure_dim) {
                hid_t dset_id = -1;
                if((dset_id = H5Dopen(pid,full_path_name.c_str(),H5P_DEFAULT)) <0) {
                    string msg = "cannot open the HDF5 dataset  ";
                    msg += full_path_name;
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                try {
                    read_objects(par_grp, full_path_name, fname,dset_id,use_dimscale,is_eos5,eos5_dim_info.varpath_to_dims);
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
                string d4dim_name;
                if (is_eos5) { 
                    auto tempdim_name = string(oname.begin(),oname.end()-1);
                    d4dim_name = handle_string_special_characters(tempdim_name);
                }
                else 
                    d4dim_name = string(oname.begin(),oname.end()-1);   
                D4Dimension *d4_dim = d4_dims->find_dim(d4dim_name);
                if(d4_dim == nullptr) {
                    hsize_t nelmts = dt_inst.nelmts;
                    // For pure dimension, if the dimension size is 0, 
                    // the dimension is unlimited and we need to use the object reference
                    // to retrieve the variable this dimension is attached and then for the same variable retrieve
                    // the size of this dimension.
                    if (dt_inst.nelmts == 0) 
                        nelmts = obtain_unlim_pure_dim_size(pid,full_path_name);
 
                    d4_dim = new D4Dimension(d4dim_name,nelmts);
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

    // For HDF-EOS5 files, We also need to add DAP4 dimensions to this group if there are HDF-EOS5 dimensions.
    if (is_eos5 && !use_dimscale) {

        unordered_map<string,vector<HE5Dim>> grppath_to_dims = eos5_dim_info.grppath_to_dims;
        vector<string> dim_names;
        auto par_grp_name = string(gname);
        if (par_grp_name.size()>1)
            par_grp_name = par_grp_name.substr(0,par_grp_name.size()-1);
#if 0
cout <<"par_grp_name is "<<par_grp_name <<endl;
#endif
        // We need to ensure the special characters are handled.
        bool is_eos5_dims = obtain_eos5_grp_dim(par_grp_name,grppath_to_dims,dim_names);
        if (is_eos5_dims) {
            vector<HE5Dim> grp_eos5_dim = grppath_to_dims[par_grp_name];
            D4Dimensions *d4_dims = par_grp->dims();
            for (unsigned grp_dim_idx = 0; grp_dim_idx<dim_names.size();grp_dim_idx++) {
                D4Dimension *d4_dim = d4_dims->find_dim(dim_names[grp_dim_idx]);
                if (d4_dim == nullptr) {
                    d4_dim = new D4Dimension(dim_names[grp_dim_idx],grp_eos5_dim[grp_dim_idx].size);
                    d4_dims->add_dim_nocopy(d4_dim);
                }
            }
        }

    }
    // The fullnamepath of the group is not necessary since dmrpp only needs the dataset path to retrieve info.
    // It only increases the dmr file size. So comment out for now.  KY 2022-10-13
#if 0
    if (is_eos5)
        map_h5_varpath_to_dap4_attr(par_grp,nullptr,nullptr,gname,0);
#endif

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

                    if (is_eos5)
                        grp_name = handle_string_special_characters(grp_name);
                    auto tem_d4_cgroup = new D4Group(grp_name);

                    // Add this new DAP4 group 
                    par_grp->add_group_nocopy(tem_d4_cgroup);

                    // Continue searching the objects under this group
                    breadth_first(file_id,cgroup, t_fpn.data(), tem_d4_cgroup,fname,use_dimscale,is_eos5,hdf5_hls,eos5_dim_info,handled_cv_names);
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
                auto tem_d4_cgroup = new D4Group(string(grp_name));

                // Note attr_str_c is the DAP4 attribute string datatype
                auto d4_hlinfo = new D4Attribute("HDF5_HARDLINK",attr_str_c);

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
///// \fn read_objects( D4Group *d4_grp, 
/////                            const string & varname, 
/////                            const string & filename,
/////                            const hid_t dset_id,
/////                            bool use_dimscale
/////                            bool is_eos5) 
///// fills in information of a dataset (name, data type, data space) into the dap4 
///// group. 
///// This is a wrapper function that calls functions to read atomic types and structure.
///// 
/////    \param d4_group DAP4 group
/////    \param varname Absolute name of an HDF5 dataset.  
/////    \param filename The HDF5 dataset name that maps to the DDS dataset name. 
////     \param dset_id HDF5 dataset id.
/////    \param use_dimscale boolean that indicates if dimscale is used.
/////    \param is_eos5 boolean that indicates if this is an HDF-EOS5 file.
/////    \throw error a string of error message to the dods interface. 
/////////////////////////////////////////////////////////////////////////////////
//
void
read_objects( D4Group * d4_grp, const string &varname, const string &filename, const hid_t dset_id,bool use_dimscale, bool is_eos5, const unordered_map<string, vector<string>>& varpath_to_dims)
{

    switch (H5Tget_class(dt_inst.type)) {

    // HDF5 compound maps to DAP structure.
    case H5T_COMPOUND:
#if 0
        read_objects_structure(d4_grp, varname, filename,dset_id,use_dimscale,is_eos5,varpath_to_dims);
#endif
        read_objects_structure(d4_grp, varname, filename,dset_id,use_dimscale,is_eos5);
        break;

    case H5T_ARRAY:
        H5Tclose(dt_inst.type);
        throw InternalErr(__FILE__, __LINE__, "Currently don't support accessing data of Array datatype when array datatype is not inside the compound.");       
    
    default:
        read_objects_base_type(d4_grp,varname, filename,dset_id,use_dimscale,is_eos5,varpath_to_dims);
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
/////                            const string & filename,const hid_t dset_id, bool use_dimscale) 
///// fills in information of a dataset (name, data type, data space) with HDF5 atomic datatypes into the dap4 
///// group. 
///// 
/////    \param dmr reference to DMR 
/////    \param d4_grp DAP4 group
/////    \param varname Absolute name of an HDF5 dataset.  
/////    \param filename The HDF5 dataset name that maps to the DDS dataset name. 
////     \param dset_id HDF5 dataset id.
/////    \param use_dimscale boolean that indicates if dimscale is used.
/////    \throw error a string of error message to the dods interface. 
/////////////////////////////////////////////////////////////////////////////////
//

//void
//read_objects_base_type(DMR & dmr, D4Group * d4_grp,const string & varname,
void
read_objects_base_type(D4Group * d4_grp,const string & varname,
                       const string & filename,hid_t dset_id, bool use_dimscale, bool is_eos5,const unordered_map<string, vector<string>>& varpath_to_dims)
{

    // Obtain the relative path of the variable name under the leaf group
    string newvarname = HDF5CFUtil::obtain_string_after_lastslash(varname);
    if (use_dimscale) {
        const string nc4_non_coord="_nc4_non_coord_";
        size_t nc4_non_coord_size= nc4_non_coord.size();
        if (newvarname.find(nc4_non_coord) == 0)
            newvarname = newvarname.substr(nc4_non_coord_size,newvarname.size()-nc4_non_coord_size);
    }
    if (is_eos5) 
        newvarname = handle_string_special_characters(newvarname);


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
            if (is_eos5)
                map_h5_varpath_to_dap4_attr(nullptr,new_var,nullptr,varname,1);
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
        ar->set_numelm((dt_inst.nelmts));
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
cerr<<"dimnames_size is "<<dimnames_size <<endl;
cerr<<"ndims is "<<dt_inst.ndims <<endl;
#endif
            
        bool is_eos5_dims = false;
        if(dimnames_size ==dt_inst.ndims) {

            for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
                if(dt_inst.dimnames[dim_index] !="") 
                    ar->append_dim_ll(dt_inst.size[dim_index],dt_inst.dimnames[dim_index]);
                else 
                    ar->append_dim_ll(dt_inst.size[dim_index]);
                    // D4dimension has to have a name. If no name, no D4dimension(from comments libdap4: Array.cc) 
            }
            dt_inst.dimnames.clear();
        }
        else {
            // With using the dimension scales, the HDF5 file may still have dimension names such as HDF-EOS5.
            // We search if there are dimension names. If yes, add them here.
            vector<string> dim_names;
            is_eos5_dims = obtain_eos5_dim(varname,varpath_to_dims,dim_names);
#if 0
cout<<"final varname is "<<varname <<endl;
for (const auto & dname:dim_names)
    cout<<"dname is "<<dname<<endl;
#endif
                       
            // For DAP4, no need to add dimension if no dimension name
            if (is_eos5_dims) {
                for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) 
                    ar->append_dim_ll(dt_inst.size[dim_index],dim_names[dim_index]);
            }
            else {
                for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) 
                    ar->append_dim_ll(dt_inst.size[dim_index]); 
            }
        }

        // We need to transform dimension info. to DAP4 group
        BaseType* new_var = nullptr;
        try {
            if (is_eos5_dims) {
#if 0
vector<string>test_dim_path = varpath_to_dims.at(varname);
for (const auto &td:test_dim_path)
cout<<"dimpath final "<<td<<endl;
#endif
                new_var = ar->h5dims_transform_to_dap4(d4_grp,varpath_to_dims.at(varname));
            }
            else {
#if 0
 for (const auto td:dt_inst.dimnames_path)
cout<<"dimpath final non-eos5 "<<td<<endl;
#endif
                new_var = ar->h5dims_transform_to_dap4(d4_grp,dt_inst.dimnames_path);
            }
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
        if (is_eos5)
            map_h5_varpath_to_dap4_attr(nullptr,new_var,nullptr,varname,1);
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
/// \fn read_objects_structure(D4Group *d4_grp,const string & varname,
///                  const string & filename,hid_t dset_id, bool use_dimscale, bool is_eos5)
/// fills in information of a structure dataset (name, data type, data space)
/// into a DAP4 group. HDF5 compound datatype will map to DAP structure.
/// 
///    \param d4_grp DAP4 group
///    \param varname Absolute name of structure
///    \param filename The HDF5 file  name that maps to the DDS dataset name.
///    \param dset_id HDF5 dataset ID
///    \param use_dimscale boolean that indicates if dimscale is used.
///    \param is_eos5 boolean that indicates if this is an HDF-EOS5 file.
///    \throw error a string of error message to the dods interface.
///////////////////////////////////////////////////////////////////////////////
void
read_objects_structure(D4Group *d4_grp, const string & varname,
                       const string & filename,hid_t dset_id,bool use_dimscale,bool is_eos5)
{
    // Obtain the relative path of the variable name under the leaf group
    string newvarname = HDF5CFUtil::obtain_string_after_lastslash(varname);
    if (use_dimscale) {
        const string nc4_non_coord="_nc4_non_coord_";
        size_t nc4_non_coord_size= nc4_non_coord.size();
        if (newvarname.find(nc4_non_coord) == 0)
            newvarname = newvarname.substr(nc4_non_coord_size,newvarname.size()-nc4_non_coord_size);
    }
    if (is_eos5) 
        newvarname = handle_string_special_characters(newvarname);
    

    // Map HDF5 compound datatype to Structure
    Structure *structure = Get_structure(newvarname, varname,filename, dt_inst.type,true);

    // TODO: compound datatype should not be used by HDF-EOS5. Still we may add those support.
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
            ar->set_numelm(dt_inst.nelmts);
            ar->set_length(dt_inst.nelmts);
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
                        ar->append_dim_ll(dt_inst.size[dim_index],dt_inst.dimnames[dim_index]);
                    else 
                        ar->append_dim_ll(dt_inst.size[dim_index]);
                }
                dt_inst.dimnames.clear();
            }
            else {
                for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) 
                    ar->append_dim_ll(dt_inst.size[dim_index]);
                    
            }

            // We need to transform dimension info. to DAP4 group
            BaseType* new_var = ar->h5dims_transform_to_dap4(d4_grp,dt_inst.dimnames_path);
            dt_inst.dimnames_path.clear();

            // Map HDF5 dataset attributes to DAP4
            map_h5_attrs_to_dap4(dset_id,nullptr,new_var,nullptr,1);

            // If this is a hardlink, map the Hardlink info. as an DAP4 attribute.
            map_h5_dset_hardlink_to_d4(dset_id,varname,new_var,nullptr,1);
            if (is_eos5)
                map_h5_varpath_to_dap4_attr(nullptr,new_var,nullptr,varname,1);
 
            
            // Add this var to DAP4 group
            if(new_var) 
                d4_grp->add_var_nocopy(new_var);
            delete ar; ar = nullptr;
        }//  end if 
        else {// A scalar structure

            structure->set_is_dap4(true);
            map_h5_attrs_to_dap4(dset_id,nullptr,nullptr,structure,2);
            map_h5_dset_hardlink_to_d4(dset_id,varname,nullptr,structure,2);
            if (is_eos5)
                map_h5_varpath_to_dap4_attr(nullptr,nullptr,structure,varname,2);
 

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

        if (dap4_attr_type == attr_str_c) {
            
            H5T_cset_t c_set_type = H5Tget_cset(ty_id);
            if (c_set_type < 0)
                throw InternalErr(__FILE__, __LINE__, "Cannot get hdf5 character set type for the attribute.");
            if (HDF5RequestHandler::get_escape_utf8_attr() == false && (c_set_type == 1)) 
                d4_attr->set_utf8_str_flag(true);
        }

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
                auto elesize = (int) H5Tget_size(ty_id);
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
                }
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

        auto d4_hlinfo = new D4Attribute("HDF5_HARDLINK",attr_str_c);
        d4_hlinfo->add_value(obj_paths.get_name(oid));
 
        if (1 == flag) 
            d4b->attributes()->add_attribute_nocopy(d4_hlinfo);
        else if ( 2 == flag)
            d4s->attributes()->add_attribute_nocopy(d4_hlinfo);
        else 
            delete d4_hlinfo;
    }

}
//////////////////////////////////////////////////////////////////////////////// 
///// \fn map_h5_varpath_to_dap4_attr(D4Group* d4g,BaseType* d4b,Structure * d4s,const string & varpath,short flag)
///// Map HDF5 the variable full path to a DAP4 attribute 
///// 
/////    \param d4g DAP4 group if the object is group, flag must be 0 if d4_grp is not nullptr.
/////    \param d4b DAP BaseType if the object is dataset and the datatype of the object is atomic datatype.  
/////               The flag must be 1 if d4b is not nullptr.
/////    \param d4s DAP Structure if the object is dataset and the datatype of the object is compound. 
/////               The flag must be 2 if d4s is not nullptr.
/////    \param varpath the fullpath of the object name.
////     \param flag flag to determine what kind of objects to map. The value must be 0,1 or 2.
/////    \throw error a string of error message to the dap interface. 
/////////////////////////////////////////////////////////////////////////////////
//

void map_h5_varpath_to_dap4_attr(D4Group* d4g,BaseType* d4b,Structure * d4s,const string & varpath, short flag) {

    auto d4_attr = new D4Attribute("fullnamepath",attr_str_c);
    d4_attr->add_value(varpath);
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

    return;
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

#if (H5_VERS_MAJOR == 1 && ((H5_VERS_MINOR == 12) || (H5_VERS_MINOR == 13) || (H5_VERS_MINOR == 14)))
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

string read_struct_metadata(hid_t s_file_id) {

    BESDEBUG("h5","Coming to read_struct_metadata()  "<<endl);
    
    string total_strmeta_value;
    string ecs_group = "/HDFEOS INFORMATION";
    hid_t ecs_grp_id = -1;
    if ((ecs_grp_id = H5Gopen(s_file_id, ecs_group.c_str(),H5P_DEFAULT))<0) {
        string msg =
            "h5_ecs_meta: unable to open the HDF5 group  ";
        msg +=ecs_group;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    H5G_info_t g_info;
    hsize_t nelems = 0;

    if (H5Gget_info(ecs_grp_id,&g_info) <0) {
       string msg =
            "h5_ecs_meta: unable to obtain the HDF5 group info. for ";
        msg +=ecs_group;
        H5Gclose(ecs_grp_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    nelems = g_info.nlinks;

    ssize_t oname_size      = 0;

    // Initalize the total number for different metadata.
    int strmeta_num         = -1;
    int strmeta_num_total   = 0;
        
    bool strmeta_no_suffix  = true;

    // Define a vector of string to hold all dataset names.
    vector<string> s_oname(nelems);

    // Define an EOSMetadata array that can describe the metadata type for each object
    // We initialize the value to OtherMeta.
    vector<bool> smetatype(nelems,false);

    for (hsize_t i = 0; i < nelems; i++) {

        // Query the length of the object name.
        oname_size =
            H5Lget_name_by_idx(ecs_grp_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,nullptr,
                0, H5P_DEFAULT); 
        if (oname_size <= 0) {
            string msg = "hdf5 object name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the name of the object.
        vector<char> oname(oname_size + 1);
        if (H5Lget_name_by_idx(ecs_grp_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname.data(),
                (size_t)(oname_size+1), H5P_DEFAULT)<0){
            string msg = "hdf5 object name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Check if this object is an HDF5 dataset, not, throw an error.
        // First, check if it is the hard link or the soft link
        H5L_info_t linfo;
        if (H5Lget_info(ecs_grp_id,oname.data(),&linfo,H5P_DEFAULT)<0) {
            string msg = "hdf5 link name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // This is the soft link.
        if (linfo.type == H5L_TYPE_SOFT){
            string msg = "hdf5 link name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the object type
        H5O_info_t oinfo;
        if (H5OGET_INFO_BY_IDX(ecs_grp_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
            string msg = "Cannot obtain the object info ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if(oinfo.type != H5O_TYPE_DATASET) {
            string msg = "hdf5 link name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }
 
        // We want to remove the last '\0' character added by C .
        string s_one_oname(oname.begin(),oname.end()-1);
        s_oname[i] = s_one_oname;

        // Calculate how many elements we have for each category(StructMetadata, CoreMetadata, etc.)
        if (((s_one_oname.find("StructMetadata"))==0) ||
           ((s_one_oname.find("structmetadata"))==0)){

            smetatype[i] = true;

            // Do we have suffix for the metadata?
            // If this metadata doesn't have any suffix, it should only come to this loop once.
            // That's why, when checking the first time, no_suffix is always true.
            // If we have already found that it doesn't have any suffix,
            // it should not go into this loop. throw an error.
            if (false == strmeta_no_suffix) {
                string msg = "StructMetadata/structmetadata without suffix should only appear once. ";
                H5Gclose(ecs_grp_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            else if(strmeta_num_total >0) 
                strmeta_num_total++;
            else { // either no suffix or the first time to loop the one having the suffix.   
                if ((0 == s_one_oname.compare("StructMetadata"))||
                    (0 == s_one_oname.compare("structmetadata")))
                    strmeta_no_suffix = false;
                else strmeta_num_total++;
            }
#if 0
"h5","strmeta_num_total= "<<strmeta_num_total <<endl;
if(strmeta_no_suffix) "h5","structmeta data has the suffix" <<endl;
else "h5","structmeta data doesn't have the suffix" <<endl;
#endif
        }

        oname.clear();
        s_one_oname.clear();

    }

    // Define a vector of string to hold StructMetadata.
    // StructMetadata must exist for a valid HDF-EOS5 file.
    vector<string> strmeta_value;
    if (strmeta_num_total <= 0) {
        string msg = "hdf5 object name error from: ";
        H5Gclose(ecs_grp_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    else {
        strmeta_value.resize(strmeta_num_total);
        for (int i = 0; i < strmeta_num_total; i++) 
            strmeta_value[i]="";
    }


    // Now we want to retrieve the metadata value and combine them into one string.
    // Here we have to remember the location of every element of the metadata if
    // this metadata has a suffix.
    for (hsize_t i = 0; i < nelems; i++) {

         // DDS parser only needs to parse the struct Metadata. So check
        // if st_only flag is true, will only read StructMetadata string.
        // Struct Metadata is generated by the HDF-EOS5 library, so the
        // name "StructMetadata.??" won't change for real struct metadata. 
        //However, we still assume that somebody may not use the HDF-EOS5
        // library to add StructMetadata, the name may be "structmetadata".
        if (((s_oname[i].find("StructMetadata"))!=0) && 
            ((s_oname[i].find("structmetadata"))!=0)){
            continue; 
        }
        
        // Open the dataset, dataspace, datatype, number of elements etc. for this metadata
        hid_t s_dset_id      = -1;
        hid_t s_space_id     = -1;
        hid_t s_ty_id        = -1;      
        hssize_t s_nelms     = -1;
        size_t dtype_size    = -1;

        if ((s_dset_id = H5Dopen(ecs_grp_id,s_oname[i].c_str(),H5P_DEFAULT))<0){
            string msg = "Cannot open HDF5 dataset  ";
            msg += s_oname[i];
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if ((s_space_id = H5Dget_space(s_dset_id))<0) {
            string msg = "Cannot open the data space of HDF5 dataset  ";
            msg += s_oname[i];
            H5Dclose(s_dset_id);
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if ((s_ty_id = H5Dget_type(s_dset_id)) < 0) {
            string msg = "Cannot get the data type of HDF5 dataset  ";
            msg += s_oname[i];
            H5Sclose(s_space_id);
            H5Dclose(s_dset_id);
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        if ((s_nelms = H5Sget_simple_extent_npoints(s_space_id))<0) {
            string msg = "Cannot get the number of points of HDF5 dataset  ";
            msg += s_oname[i];
            H5Tclose(s_ty_id);
            H5Sclose(s_space_id);
            H5Dclose(s_dset_id);
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        if ((dtype_size = H5Tget_size(s_ty_id))==0) {

            string msg = "Cannot get the data type size of HDF5 dataset  ";
            msg += s_oname[i];
            H5Tclose(s_ty_id);
            H5Sclose(s_space_id);
            H5Dclose(s_dset_id);
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the real value of the metadata
        vector<char> s_buf(dtype_size*s_nelms +1);

        if ((H5Dread(s_dset_id,s_ty_id,H5S_ALL,H5S_ALL,H5P_DEFAULT,s_buf.data()))<0) {

            string msg = "Cannot read HDF5 dataset  ";
            msg += s_oname[i];
            H5Tclose(s_ty_id);
            H5Sclose(s_space_id);
            H5Dclose(s_dset_id);
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Now we can safely close datatype, data space and dataset IDs.
        H5Tclose(s_ty_id);
        H5Sclose(s_space_id);
        H5Dclose(s_dset_id);


        // Convert from the vector<char> to a C++ string.
        string tempstr(s_buf.begin(),s_buf.end());
        s_buf.clear();
        size_t temp_null_pos = tempstr.find_first_of('\0');

        // temp_null_pos returns the position of nullptr,which is the last character of the string. 
        // so the length of string before null is EQUAL to
        // temp_null_pos since pos starts at 0.
        string finstr = tempstr.substr(0,temp_null_pos);

        // For the DDS parser, only return StructMetadata
        if (true == smetatype[i]) {

            // Now obtain the corresponding value in integer type for the suffix. '0' to 0 etc. 
            try {
                strmeta_num = get_strmetadata_num(s_oname[i]);
            }
            catch(...) {
                H5Gclose(ecs_grp_id);
                throw InternalErr(__FILE__,__LINE__,"Obtain structmetadata suffix error.");

            }
            // This is probably not necessary, since structmetadata may always have a suffix.           
            // Leave here just in case the rules change or a special non-HDF-EOS5 library generated file.
            // when strmeta_num is -1, it means no suffix for this metadata. So the total structmetadata
            // is this string only.
            if (-1 == strmeta_num) 
                total_strmeta_value = finstr;
            // strmeta_value at this point should be empty before assigning any values.
            else if (strmeta_value[strmeta_num]!="") {
                string msg = "The structmeta value array at this index should be empty string  ";
                H5Gclose(ecs_grp_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
            // assign the string vector to this value.
            else 
                strmeta_value[strmeta_num] = finstr;
        }
        tempstr.clear();
        finstr.clear();
    }


    // Now we need to handle the concatenation of the metadata
    if ((strmeta_num_total > 0) && (strmeta_num != -1) ) {
        // The no suffix one has been taken care.
        for (int i = 0; i <strmeta_num_total; i++) 
            total_strmeta_value +=strmeta_value[i];
    }

    return total_strmeta_value;
}

// Helper function for read_ecs_metadata. Get the number after metadata.
int get_strmetadata_num(const string & meta_str) {

    size_t dot_pos = meta_str.find(".");
    if (dot_pos == string::npos) // No dot
        return -1;
    else { 
        string num_str = meta_str.substr(dot_pos+1);
        stringstream ssnum(num_str);
        int num;
        ssnum >> num;
        if (ssnum.fail()) 
            throw InternalErr(__FILE__,__LINE__,"Suffix after dots is not a number.");
        return num;
    }
}

void obtain_eos5_dims(hid_t fileid, eos5_dim_info_t &eos5_dim_info) {

    unordered_map<string, vector<string>> varpath_to_dims;
    unordered_map<string, vector<HE5Dim>> grppath_to_dims;

    string st_str = read_struct_metadata(fileid);
    
    // Parse the structmetadata
    HE5Parser p;
    HE5Checker c;
    he5dds_scan_string(st_str.c_str());
    he5ddsparse(&p);
    he5ddslex_destroy();

    // Retrieve ProjParams from StructMetadata
    p.add_projparams(st_str);
#if 0
    p.print();
#endif

    // Check if the HDF-EOS5 grid has the valid parameters, projection codes.
    if (c.check_grids_unknown_parameters(&p)) {
        throw InternalErr("Unknown HDF-EOS5 grid paramters found in the file");
    }

    if (c.check_grids_missing_projcode(&p)) {
        throw InternalErr("The HDF-EOS5 is missing project code ");
    }

    // We gradually add the support of different projection codes
    if (c.check_grids_support_projcode(&p)) {
        throw InternalErr("The current project code is not supported");
    }

    // HDF-EOS5 provides default pixel and origin values if they are not defined.
    c.set_grids_missing_pixreg_orig(&p);

    // HDF-EOS5 provides default pixel and origin values if they are not defined.
    c.set_grids_missing_pixreg_orig(&p);

    // Check if this multi-grid file shares the same grid.
    // TODO: NEED TO check if the following function needs to be called 
    //       when handling the HDF-EOS5 grid.
#if 0
    bool grids_mllcv = c.check_grids_multi_latlon_coord_vars(&p);
#endif

    for (const auto &sw:p.swath_list) 
      build_grp_dim_path(sw.name,sw.dim_list,grppath_to_dims,HE5_TYPE::SW);

    for (const auto &sw:p.swath_list) 
      build_var_dim_path(sw.name,sw.data_var_list,varpath_to_dims,HE5_TYPE::SW,false);

    for (const auto &sw:p.swath_list) 
      build_var_dim_path(sw.name,sw.geo_var_list,varpath_to_dims,HE5_TYPE::SW,true);

    for (const auto &gd:p.grid_list) 
      build_grp_dim_path(gd.name,gd.dim_list,grppath_to_dims,HE5_TYPE::GD);

    for (const auto &gd:p.grid_list) 
      build_var_dim_path(gd.name,gd.data_var_list,varpath_to_dims,HE5_TYPE::GD,false);

    for (const auto &za:p.za_list) 
      build_grp_dim_path(za.name,za.dim_list,grppath_to_dims,HE5_TYPE::ZA);

    for (const auto &za:p.za_list) 
      build_var_dim_path(za.name,za.data_var_list,varpath_to_dims,HE5_TYPE::ZA,false);


#if 0
for (auto it:varpath_to_dims) {
    cout<<"var path is "<<it.first <<endl; 
    for (auto sit:it.second)
        cout<<"var dimension name is "<<sit <<endl; 
}
       
for (auto it:grppath_to_dims) {
    cout<<"grp path is "<<it.first <<endl; 
    for (auto sit:it.second) {
        cout<<"grp dimension name is "<<sit.name<<endl; 
        cout<<"grp dimension size is "<<sit.size<<endl; 
    }
}   swath_2_3d_2x2yz.h5.dmr.bescmd.baseline
#endif 

    eos5_dim_info.varpath_to_dims = varpath_to_dims;
    eos5_dim_info.grppath_to_dims = grppath_to_dims;
}

void build_grp_dim_path(const string & eos5_obj_name, const vector<HE5Dim>& dim_list, unordered_map<string, vector<HE5Dim>>& grppath_to_dims, HE5_TYPE e5_type) {

    string eos_name_prefix = "/HDFEOS/";
    string eos5_grp_path;
    string new_eos5_obj_name = eos5_obj_name;

    switch (e5_type) {  
       case HE5_TYPE::SW: 
            eos5_grp_path = eos_name_prefix + "SWATHS/"+new_eos5_obj_name;      
            break;
       case HE5_TYPE::GD: 
            eos5_grp_path = eos_name_prefix + "GRIDS/"+new_eos5_obj_name;      
            break;
       case HE5_TYPE::ZA: 
            eos5_grp_path = eos_name_prefix + "ZAS/"+new_eos5_obj_name;      
            break;
       default:
            break;
    }

#if 0
    for (const auto & eos5dim:dim_list) {
        cout << "EOS5 Dim Name=" << eos5dim.name << endl;
        cout << "EOS5 Dim Size=" << eos5dim.size << endl;
    }
#endif

    vector <HE5Dim> grp_dims;
    for (const auto &eos5dim:dim_list) {  
        HE5Dim eos5_dimp;
        string new_eos5dim_name = eos5dim.name;
        string dim_fpath = eos5_grp_path +"/" + handle_string_special_characters(new_eos5dim_name);
        eos5_dimp.name = dim_fpath;
        eos5_dimp.size = eos5dim.size;
        grp_dims.push_back(eos5_dimp); 
    }

    pair<string,vector<HE5Dim>> gtod = make_pair(eos5_grp_path,grp_dims);
    grppath_to_dims.insert(gtod);

          
}

void build_var_dim_path(const string & eos5_obj_name, const vector<HE5Var>& var_list, unordered_map<string, vector<string>>& varpath_to_dims, HE5_TYPE e5_type, bool is_geo) {

    string eos_name_prefix = "/HDFEOS/";
    string eos5_data_grp_name = "/Data Fields/";
    string eos5_geo_grp_name = "/Geolocation Fields/";
    string eos5_dim_name_prefix;
    string new_eos5_obj_name = eos5_obj_name;

    switch (e5_type) {  
       case HE5_TYPE::SW: 
            eos5_dim_name_prefix = eos_name_prefix + "SWATHS/"+handle_string_special_characters(new_eos5_obj_name) +"/";      
            break;
       case HE5_TYPE::GD: 
            eos5_dim_name_prefix = eos_name_prefix + "GRIDS/"+handle_string_special_characters(new_eos5_obj_name) +"/";      
            break;
       case HE5_TYPE::ZA: 
            eos5_dim_name_prefix = eos_name_prefix + "ZAS/"+handle_string_special_characters(new_eos5_obj_name) +"/";      
            break;
       default:
            break;
    }

    for (const auto & eos5var:var_list) {
#if 0
        cout << "EOS5 Var Name=" << eos5var.name << endl;
#endif
        string var_path;
        vector<string> var_dim_names;

        switch (e5_type) {  

            case HE5_TYPE::SW: 
            {
                if (is_geo) 
                    var_path = eos_name_prefix + "SWATHS/"+eos5_obj_name + eos5_geo_grp_name + eos5var.name;
                else 
                    var_path = eos_name_prefix + "SWATHS/"+eos5_obj_name + eos5_data_grp_name + eos5var.name;
                break;
            }

            case HE5_TYPE::GD: 
            {
                var_path = eos_name_prefix + "GRIDS/"+eos5_obj_name + eos5_data_grp_name + eos5var.name;
                break;
            }

            case HE5_TYPE::ZA: 
            {
                var_path = eos_name_prefix + "ZAS/"+eos5_obj_name + eos5_data_grp_name + eos5var.name;
                break;
            }
            default:
                break;
        }

#if 0
cout <<"var_path is "<<var_path <<endl;
        for (const auto &eos5dim:eos5var.dim_list)  {
            cout << "EOS Var Dim Name=" << eos5dim.name << endl;
        }
#endif
        for (const auto &eos5dim:eos5var.dim_list) {  
            string new_eos5dim_name = eos5dim.name;
            string dim_fpath = eos5_dim_name_prefix + handle_string_special_characters(new_eos5dim_name);
            var_dim_names.push_back(dim_fpath); 
        }
        pair<string,vector<string>> vtod = make_pair(var_path,var_dim_names);
        varpath_to_dims.insert(vtod);
    }

}

bool obtain_eos5_dim(const string & varname, const unordered_map<string, vector<string>>& varpath_to_dims, vector<string> & dimnames) {

    bool ret_value = false;
    unordered_map<string,vector<string>>::const_iterator vit = varpath_to_dims.find(varname);
    if (vit != varpath_to_dims.end()){
        for (const auto &sit:vit->second)
            dimnames.push_back(HDF5CFUtil::obtain_string_after_lastslash(sit));
        ret_value = true;
    }
    return ret_value;
} 

bool obtain_eos5_grp_dim(const string & varname, const unordered_map<string, vector<HE5Dim>>& grppath_to_dims, vector<string> & dimnames) {

    bool ret_value = false;
    unordered_map<string,vector<HE5Dim>>::const_iterator vit = grppath_to_dims.find(varname);
    if (vit != grppath_to_dims.end()){
        for (const auto &sit:vit->second)
            dimnames.push_back(HDF5CFUtil::obtain_string_after_lastslash(sit.name));
        ret_value = true;
    }
    return ret_value;
}

hsize_t obtain_unlim_pure_dim_size(hid_t pid, const string &dname) {

    hsize_t ret_value = 0;

    typedef struct s_t {
        hobj_ref_t s_ref;
        int s_index;
    } s_t;
    
    hid_t dset_id = -1;
    if((dset_id = H5Dopen(pid,dname.c_str(),H5P_DEFAULT)) <0) {
        string msg = "cannot open the HDF5 dataset  ";
        msg += dname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    htri_t has_reference_list = -1;
    string reference_name= "REFERENCE_LIST";
    has_reference_list = H5Aexists(dset_id,reference_name.c_str());

    if (has_reference_list >0) {

        hid_t attr_id = H5Aopen(dset_id,reference_name.c_str(),H5P_DEFAULT);
        if (attr_id <0 ) {
            H5Dclose(dset_id);
            string msg = "Cannot open the attribute " + reference_name + " of HDF5 dataset "+ dname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        hid_t atype_id = H5Aget_type(attr_id);
        if (atype_id <0) {
            H5Aclose(attr_id);
            H5Dclose(dset_id);
            string msg = "Cannot get the datatype of the attribute " + reference_name + " of HDF5 dataset "+ dname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if (H5T_COMPOUND == H5Tget_class(atype_id)) {
            
            hid_t aspace_id = H5Aget_space(attr_id);
            if (aspace_id < 0) {
                H5Aclose(attr_id);
                H5Tclose(atype_id);
                H5Dclose(dset_id);
                string msg = "Cannot obtain the data space ID  for the attribute  " + reference_name;
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            hssize_t num_ele_ref = H5Sget_simple_extent_npoints(aspace_id);
            if (num_ele_ref < 0) {
                H5Aclose(attr_id);
                H5Tclose(atype_id);
                H5Dclose(dset_id);
                string msg = "Cannot obtain the number of elements for space of the attribute  " + reference_name;
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            size_t ele_size = H5Tget_size(atype_id);
            if (ele_size == 0) {
                H5Aclose(attr_id);
                H5Tclose(atype_id);
                H5Dclose(dset_id);
                string msg = "Cannot obtain the datatype size of the attribute  " + reference_name;
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            if (sizeof(s_t)!=ele_size) {
                H5Aclose(attr_id);
                H5Tclose(atype_id);
                H5Dclose(dset_id);
                string msg = "The data type size is not the same as the struct. ";
                throw InternalErr(__FILE__, __LINE__, msg);
            }
            vector<s_t> ref_list(num_ele_ref);

            if (H5Aread(attr_id,atype_id,ref_list.data()) <0)  {
                H5Aclose(attr_id);
                H5Tclose(atype_id);
                H5Dclose(dset_id);
                string msg = "Cannot obtain the referenced object for the variable  " + dname;
                throw InternalErr(__FILE__, __LINE__, msg);
            }            

            // To obtain the dimension size, we only need to grab the first referred object. 
            // The size is the same for all objects. KY 2022-12-14

            H5O_type_t obj_type; 
            if (H5Rget_obj_type2(dset_id, H5R_OBJECT, &((ref_list[0]).s_ref),&obj_type)<0) {
                H5Aclose(attr_id);
                H5Tclose(atype_id);
                H5Dclose(dset_id);
                string msg = "Cannot obtain the referenced object for the variable  " + dname;
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            if (obj_type == H5O_TYPE_DATASET) {

                hid_t did_ref = H5Rdereference2(dset_id, H5P_DEFAULT, H5R_OBJECT, &((ref_list[0]).s_ref));
                if (did_ref < 0) {
                    H5Aclose(attr_id);
                    H5Tclose(atype_id);
                    H5Dclose(dset_id);
                    string msg = "Cannot de-reference the object for the variable  " + dname;
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                hid_t did_space = H5Dget_space(did_ref);
                if (did_space < 0) {
                    H5Aclose(attr_id);
                    H5Tclose(atype_id);
                    H5Dclose(dset_id);
                    H5Dclose(did_ref);
                    string msg = "Cannot open the space of the de-referenced object for the variable  " + dname;
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                hssize_t num_ele_dim = H5Sget_simple_extent_npoints(did_space);
                if (num_ele_dim < 0) {
                    H5Aclose(attr_id);
                    H5Tclose(atype_id);
                    H5Dclose(dset_id);
                    H5Dclose(did_ref);
                    H5Sclose(did_space);
                    string msg = "Cannot obtain the number of elements for space of the attribute  " + reference_name;
                    throw InternalErr(__FILE__, __LINE__, msg);
                }
                ret_value =(hsize_t)num_ele_dim;

                H5Dclose(did_ref);
                H5Sclose(did_space);
            }

            H5Sclose(aspace_id);
        }
       
        H5Aclose(attr_id);
        H5Tclose(atype_id);
        
    }
    
    if(H5Dclose(dset_id)<0) {
        string msg = "cannot close the HDF5 dataset  ";
        msg += dname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
 
    return ret_value;
}

// The main routine to handle the coverage support. The handled_all_cv_names should be detected before
// calling this routine. It includes all valid dimension scale variables. that is: the pure netCDF4-like
// dimensions are not included.
void add_dap4_coverage_default(D4Group* d4_root, const vector<string>& handled_all_cv_names) {

    // We need to construct the var name to Array map,using unordered_map for quick search.
    // Dimension scale path to array maps(Grid and non-coordinate dimensions)
    unordered_map<string, Array*> dsname_array_maps;
    obtain_ds_name_array_maps(d4_root,dsname_array_maps, handled_all_cv_names);


    // Coordinate to array(Swath or other cases)
    unordered_map<string, Array*> coname_array_maps;
 
    Constructor::Vars_iter vi = d4_root->var_begin();
    Constructor::Vars_iter ve = d4_root->var_end();

    for (; vi != ve; vi++) {

        const BaseType *v = *vi;

        // Only Array can have maps.
        if (libdap::dods_array_c == v->type()) {

            auto t_a = static_cast<Array *>(*vi);

            vector<string> coord_names;
            unordered_set<string> handled_dim_names;
            obtain_coord_names(t_a,coord_names);

            // Having the coordinate attributes,
            if (coord_names.empty()==false) {
                make_coord_names_fpath(d4_root,coord_names);
                remove_empty_coord_names(coord_names);
                add_coord_maps(d4_root,t_a,coord_names,coname_array_maps,handled_dim_names);
                add_dimscale_maps(t_a,dsname_array_maps,handled_dim_names);
            }
            else  // Just use the dimension scales.
                add_dimscale_maps(t_a,dsname_array_maps,handled_dim_names);
 
        }
    }

    // Go over the children groups.
    for (D4Group::groupsIter gi = d4_root->grp_begin(), ge = d4_root->grp_end(); gi != ge; ++gi) 
        add_dap4_coverage_default_internal(*gi, dsname_array_maps,coname_array_maps);

    // Now we need to remove the maps from the coordinate variables and the dimension scales. 
    // First dimension scales.
    for (auto &ds_map:dsname_array_maps) {
        D4Maps *d4_maps = (ds_map.second)->maps();
#if 0
        if (d4_maps->size() !=1) 
            throw InternalErr(__FILE__, __LINE__, "The number of dims of a dimension scale should be 1");
#endif

        for (unsigned i =0; i<d4_maps->size();i++) {
            D4Map * d4_map = d4_maps->get_map(i);
            d4_maps->remove_map(d4_map);
            delete d4_map;
        }
    }

    // Then coordinates
    for (auto &cv_map:coname_array_maps) {
        D4Maps *d4_maps = (cv_map.second)->maps();

        D4Maps::D4MapsIter d4map_i = d4_maps->map_begin();
        D4Maps::D4MapsIter d4map_e = d4_maps->map_end();

        for(; d4map_i !=d4map_e;d4map_i++) { 
            D4Map *tmp_map = *d4map_i;
            d4_maps->remove_map(*d4map_i);
            delete tmp_map;
        }

    }

    // Reorder the variables so that the map variables are at the front.
    unordered_map<string,Array*> dc_co_array_maps;
    // STOP
    // Loop through dsname_array_maps, then search coname_array_maps, if this element is not in the coname_array_maps, add this to dc_co_array_maps.
    for (const auto dsname_array_map:dsname_array_maps) {
        
        bool found_coname = false;
        for (const auto coname_array_map:coname_array_maps) {
            if (coname_array_map.first == dsname_array_map.first) {
                found_coname = true;
                break;
            }
        }

        if (found_coname == false) 
            dc_co_array_maps.insert(dsname_array_map);
    }


    reorder_vars(d4_root,coname_array_maps,dc_co_array_maps);

}

void add_dap4_coverage_default_internal(D4Group* d4_grp, unordered_map<string, Array*> &dsname_array_maps, unordered_map<string, Array*> &coname_array_maps) {


    Constructor::Vars_iter vi = d4_grp->var_begin();
    Constructor::Vars_iter ve = d4_grp->var_end();

    for (; vi != ve; vi++) {

        const BaseType *v = *vi;

        // Only Arrays can have maps.
        if (libdap::dods_array_c == v->type()) {

            auto t_a = static_cast<Array *>(*vi);

            vector<string> coord_names;
            unordered_set<string> handled_dim_names;

            // Obtain the coordinate names if having the coordinates attribute.
            obtain_coord_names(t_a,coord_names);

            if (coord_names.empty()==false) {

                // Obtain FQN of  all coordinates 
                make_coord_names_fpath(d4_grp,coord_names);
                remove_empty_coord_names(coord_names);

                // Add coordinates to the coordinate-array map. Also need to return handled dimension names.
                add_coord_maps(d4_grp,t_a,coord_names,coname_array_maps,handled_dim_names);

                // For the dimensions not covered by found coordinates attribute, check dimension scales.
                add_dimscale_maps(t_a,dsname_array_maps,handled_dim_names);
            }
            else 
                add_dimscale_maps(t_a,dsname_array_maps,handled_dim_names);

#if 0
for (unsigned i = 0; i <coord_names.size();i++)
cerr<<"coordinate name final is "<<coord_names[i] <<endl;
cerr<<"var FQN is "<<t_a->FQN() <<endl;
#endif
 
        }

    }

    // Go over children groups.
    for (D4Group::groupsIter gi = d4_grp->grp_begin(), ge = d4_grp->grp_end(); gi != ge; ++gi) {
        //    BESDEBUG(MODULE, prolog << "In group:  " << (*gi)->name() << endl);
        add_dap4_coverage_default_internal(*gi, dsname_array_maps, coname_array_maps);
    }
}

void obtain_coord_names(Array* ar, vector<string> & coord_names) {

    D4Attributes *d4_attrs = ar->attributes();
    D4Attribute *d4_attr = d4_attrs->find("coordinates");
    if (d4_attr != nullptr) {
        if (d4_attr->type() == attr_str_c) {
            if (d4_attr->num_values() == 1) {
                string tempstring = d4_attr->value(0);
                char sep=' ';
                HDF5CFUtil::Split_helper(coord_names,tempstring,sep);
            }
            // From our observations, the coordinates attribute is always just one string. 
            // So this else block may never be executed.
            else {

                for (D4Attribute::D4AttributeIter av_i = d4_attr->value_begin(), av_e = d4_attr->value_end(); av_i != av_e; av_i++) {
                    vector <string> tempstr_vec;
                    char sep=' ';
                    HDF5CFUtil::Split_helper(tempstr_vec,*av_i,sep);
                    for (const auto &tve:tempstr_vec)
                        coord_names.push_back(tve);
                }
            }
        }
    }
}

// Generate absolute path(FQN) for all coordinate variables.
void make_coord_names_fpath(D4Group* d4_grp,  vector<string> &coord_names) {

    for (auto &cname:coord_names) {

        // No path inside the coordinate name.
        if (cname.find('/')==string::npos) { 
            if (false == obtain_no_path_cv(d4_grp,cname))
                cname ="";
        }
        else if(cname[0] == '/') // the absolute path is specified
            handle_absolute_path_cv(d4_grp,cname);
        else  //  The relative path is specified.
            handle_relative_path_cv(d4_grp, cname);
    }

}

// This is for case when coordinates is something like "lat lon", no path is provided.
// We then search the variable names at this group and the ancestor groups until we find them.
// Note this function only applies to one coordinate at each time.
bool obtain_no_path_cv(D4Group *d4_grp, string &coord_name) {
    
    bool found_cv = false;
    
    Constructor::Vars_iter vi = d4_grp->var_begin();
    Constructor::Vars_iter ve = d4_grp->var_end();

    for (; vi != ve; vi++) {

        const BaseType *v = *vi;

        // Currently we only consider the cv that is an array.
        if (libdap::dods_array_c == v->type()) {

            auto t_a = static_cast<Array *>(*vi);
            if (coord_name == t_a->name()) {
                // Find the coordinate variable, But We need to return the absolute path of the variable.
                coord_name = t_a->FQN();
                found_cv = true;
                break;
            }
        }
    }

    if (found_cv == false) {
        if (d4_grp->get_parent()) {
            auto d4_grp_par = static_cast<D4Group*>(d4_grp->get_parent());
            found_cv = obtain_no_path_cv(d4_grp_par,coord_name);
        }
    }
    return found_cv;
}

void handle_absolute_path_cv(const D4Group *d4_grp, const string &coord_name) {
    // For the time being, we don't check if this cv with absolute path exists.
    return;
}

// Handle the coordinate attribute that includes relative paths such as coordinates={../../foo etc}
void handle_relative_path_cv(const D4Group *d4_grp, string &coord_name) {

    // The only valid relative path is among the path of the ancestor groups according to CF.
    // So if the identified coordinate variable (cv) names are not under the ancestor groups, we
    // don't consider a valid coordinate and skip it. Also we assume the "../" is used
    // in the relative path as a way to go to the parent group.

    bool find_coord = true;
    string sep = "../";
    unsigned short sep_count = 0;
    size_t pos = coord_name.find(sep, 0);
    if (pos !=0)
        find_coord = false;
    else {
        while(pos != string::npos)
        {   
            sep_count++;
            size_t temp_pos = pos;
            pos = coord_name.find(sep,pos+1);
            // If we find something not like ../../../??,this is invalid.
            if ((pos !=string::npos) && (pos !=(temp_pos+3))) {
                find_coord = false;
                break;
            }
        }
    }
    string msg = "The coordinate attribute that includes the relative path ";
    msg +="must contain at least one ../ string but this coordinate with the value <";
    msg +=coord_name +'>'+" doesn't contain one.";

    // This exception may be lifted if we care more on the execution of operations than  the values of coordinates and the maps.
    if (sep_count == 0)
         throw InternalErr(__FILE__, __LINE__, msg); 

    // Now we need to find the absolute path of the coordinate variable. 
    if (find_coord) {

        // Obtain variable's name and FQN
        string grp_fqn  = d4_grp->FQN();

        size_t co_path_pos = 0;
            
        size_t var_back_st_pos = grp_fqn.size()-1;
        if (var_back_st_pos >0)
            var_back_st_pos--;

        // To find the coordinate variable path, we need to search backward and then reduce the number of "../".
        for (size_t i =var_back_st_pos; i >=0;i--) {
	    if (grp_fqn[i] == '/') {
                sep_count--;
                if(sep_count == 0) {
                    co_path_pos = i;
                    break;
                }
            }
        }

        if (sep_count > 0) { // Invalid relative paths. 
            //We should not include this coordinate in the map. 
            coord_name="";
        }
        else {//build up the coordinate variable full path

            // The path includes the '/' at the end.
            string the_path = grp_fqn.substr(0,co_path_pos+1);
            string the_name = HDF5CFUtil::obtain_string_after_lastslash(coord_name);
            coord_name = the_path + the_name;
        }
    }
}

void remove_empty_coord_names(vector<string> & coord_names) {

    for (auto it = coord_names.begin(); it !=coord_names.end();) {
        if (*it =="")
            it = coord_names.erase(it);
        else 
            ++it;
    }

}

// Obtain global dimension scale to array maps for this group for the coverage support.
// Note: the handled_all_cv_names are the coordinte variables based on dimension scales  detected before adding DAP4 maps.
// This vector is const and should not be changed.
void obtain_ds_name_array_maps(D4Group *d4_grp,unordered_map<string,Array*>&dsn_array_maps, 
                               const vector<string>& handled_all_cv_names) {

    // Loop through all the variables in this group.
    Constructor::Vars_iter vi = d4_grp->var_begin();
    Constructor::Vars_iter ve = d4_grp->var_end();

    for (; vi != ve; vi++) {

        const BaseType *v = *vi;

        // Only Array can have maps.
        if (libdap::dods_array_c == v->type()) {

            auto t_a = static_cast<Array *>(*vi);
            // Dimension scales must be 1-dimension. So we save many unnecessary operations.
            // Find the dimension scale, insert to the global unordered_map.
            if (t_a->dimensions() == 1) {
                string t_a_fqn = t_a->FQN();
                if (find(handled_all_cv_names.begin(),handled_all_cv_names.end(),t_a_fqn) != handled_all_cv_names.end()) 
                    dsn_array_maps.emplace(t_a_fqn,t_a);
            }
        }
    }

#if 0
for (const auto &dmap:dsn_array_maps) 
    cout <<"dim map name before: " << dmap.first <<endl;
#endif


    for (D4Group::groupsIter gi = d4_grp->grp_begin(), ge = d4_grp->grp_end(); gi != ge; ++gi) {
        BESDEBUG("h5",  "In group:  " << (*gi)->name() << endl);
        obtain_ds_name_array_maps(*gi, dsn_array_maps, handled_all_cv_names);
    }

}

// Add the valid coordinate variables(via CF's coordinates attribute of this variable) to the var's DAP4 maps.
void add_coord_maps(D4Group *d4_grp, Array *var, vector<string> &coord_names, 
                    unordered_map<string,Array*> & coname_array_maps, 
                    unordered_set<string> & handled_dim_names) {

    // Search if the coord name(in full path) can be found in the current coname_array_maps. 
    // If the coord name is found, add it to the DAP4 map.
    for (auto cv_it =coord_names.begin(); cv_it != coord_names.end();) {

        unordered_map<string, Array*>::const_iterator it_ma = coname_array_maps.find(*cv_it);
        if (it_ma != coname_array_maps.end()) {

            auto d4_map = new D4Map(it_ma->first, it_ma->second);
            var->maps()->add_map(d4_map);

            // Obtain the dimension full paths. These dimensions are handled dimensions. The dimension scales of 
            // these dimensions should NOT be included in the final coverage maps. 
            obtain_handled_dim_names(it_ma->second,handled_dim_names);
            
            // Also need to remove this coordinate name from the coord_names vector since this coordinate is handled.
            // Note the coord_names is only for this variable. 
            cv_it = coord_names.erase(cv_it);
            
        }
        else 
            ++cv_it;
    }

   // Now we need to search the rest of this variable's coordinates.
    for (auto cv_it =coord_names.begin(); cv_it != coord_names.end();) {

        bool found_cv = false;
        Constructor::Vars_iter vi = d4_grp->var_begin();
        Constructor::Vars_iter ve = d4_grp->var_end();

        for (; vi != ve; vi++) {
    
            const BaseType *v = *vi;
    
            // DAP4 requires the maps to be Arrays. So we only consider the cv that is an array.
            // Note the DSG(Discrete Sample Geometry) data may contain scalar coordinates. DAP4
            // cannot handle such a case now. So no maps will be generated for scalar coordiates.
            if (libdap::dods_array_c == v->type()) {
    
                auto t_a = static_cast<Array *>(*vi);
                // Find it.
                if (*cv_it == t_a->FQN()) {
    
                    // Add the maps
                    auto d4_map = new D4Map(t_a->FQN(), t_a);
                    var->maps()->add_map(d4_map);

                    // Need to add this coordinate to the coname_array_maps 
                    coname_array_maps.emplace(t_a->FQN(),t_a);

                    // Obtain the dimension full paths. These dimensions are handled dimensions. The dimension scales of
                    // these dimensions should NOT be included in the final coverage maps.
                    obtain_handled_dim_names(t_a,handled_dim_names);

                    found_cv = true;
                    break;
                }
            }
        }

        // Done with this coordinate, erase it from the coordinate names vector, search the next.
        if (found_cv) 
            cv_it = coord_names.erase(cv_it);
        else
            ++cv_it;
    }

    // Need to check the parent group and call recursively for the coordinates not found in this group..
    if (coord_names.empty() == false && d4_grp->get_parent()) {
        auto d4_grp_par = static_cast<D4Group*>(d4_grp->get_parent());
        add_coord_maps(d4_grp_par,var,coord_names,coname_array_maps,handled_dim_names);
    }
}

// Add the valid coordinate variables(via dimension scales) to the var's DAP4 maps.
// Loop through the handled dimension names(by the coordinate variables) set
// Then check this var's dimensions. Only add this var's unhandled coordinates(dimension scales) if these dimension scales exist. 
void add_dimscale_maps(libdap::Array* var, std::unordered_map<std::string,libdap::Array*> & dc_array_maps, const std::unordered_set<std::string> & handled_dim_names) {

    BESDEBUG("h5","Coming to add_dimscale_maps() "<<endl);

    Array::Dim_iter di = var->dim_begin();
    Array::Dim_iter de = var->dim_end();

    for (; di != de; di++) {

        const D4Dimension * d4_dim = var->dimension_D4dim(di);

        // DAP4 dimension may not exist, so need to check.
        if(d4_dim) { 

            // Fully Qualified name(absolute path) needs to be used as a key for search.
            string dim_fqn = d4_dim->fully_qualified_name();

            // Find that this dimension is not handled, we need to check if there is a coordinate variable via dimension scale 
            // for this dimension.
            if (handled_dim_names.find(dim_fqn) == handled_dim_names.end()) {

                unordered_map<string, Array*>::const_iterator it_ma = dc_array_maps.find(dim_fqn);

                // Find a valid coordinate variable for this dimension, insert this coordinate variable as a DAP4 map to this var.
                if (it_ma != dc_array_maps.end()) {
                    auto d4_map = new D4Map(it_ma->first, it_ma->second);
                    var->maps()->add_map(d4_map);
                }
            }
        }
    }
    return;
}


// Obtain handled dimension names of this variable. An unordered set is used for quick search.
void obtain_handled_dim_names(Array *var, unordered_set<string> & handled_dim_names) {

    Array::Dim_iter di = var->dim_begin();
    Array::Dim_iter de = var->dim_end();

    for (; di != de; di++) {
        const D4Dimension * d4_dim = var->dimension_D4dim(di);
        if (d4_dim) 
            handled_dim_names.insert(d4_dim->fully_qualified_name());
    }
}
 
void reorder_vars(D4Group *d4_grp, const unordered_map<string,Array*> &coname_array_maps, const unordered_map<string,Array*> & dc_array_maps) {

    
    Constructor::Vars_iter vi = d4_grp->var_begin();
    Constructor::Vars_iter ve = d4_grp->var_end();

    set<int> cv_pos;
    set<BaseType *> cv_obj_ptr;

    int v_index = 0;
    for (; vi != ve; vi++) {

        BaseType *v = *vi;
        // We only need to re-order arrays. 
        if (libdap::dods_array_c == v->type()) {
            for (const auto coname_array_map:coname_array_maps) {
                if (coname_array_map.first == v->FQN()) {
                    cv_pos.insert(v_index);
                    cv_obj_ptr.insert(v);
                }
            }
            for (const auto dc_array_map:dc_array_maps) {
                if (dc_array_map.first == v->FQN()) {
                    cv_pos.insert(v_index);
                    cv_obj_ptr.insert(v);
                }
            }
        }
        v_index++;
    }

    set<int> front_v_pos;
    set<BaseType *>front_v_ptr;

    int stop_index = (int)(cv_pos.size()) + 1;

#if 0
    for (; vi != ve; vi++) {

        BaseType *v = *vi;
        // We only need to re-order arrays. 
        if (libdap::dods_array_c == v->type()) {
            //auto t_a = static_cast<Array *>(*vi);
            v_copy = v;
            break;
        }

    }

    vi++;

    BaseType* v_copy2;
    for (; vi != ve; vi++) {

        BaseType *v = *vi;
        // We only need to re-order arrays. 
        if (libdap::dods_array_c == v->type()) {
            //auto t_a = static_cast<Array *>(*vi);
            v_copy2 = v;
            break;
        }

    }

//#if 0
    d4_grp->set_var_index(v_copy2,0);
    d4_grp->set_var_index(v_copy,1);
//#endif
#endif

#if 0
    vi = d4_grp->var_begin();
    ve = d4_grp->var_end();

    for (; vi != ve; vi++) {

        BaseType *v = *vi;
        // We only need to re-order arrays. 
        if (libdap::dods_array_c == v->type()) {
            *vi = &v_copy2_val;
            //delete v;
            break;
        }

    }

    for (; vi != ve; vi++) {

        BaseType *v = *vi;
        // We only need to re-order arrays. 
        if (libdap::dods_array_c == v->type()) {
            *vi = &v_copy_val;
            //delete v;
            break;
        }

    }
#endif

#if 0
    for (; vi != ve; ) {

        BaseType *v = *vi;

        // We only need to re-order arrays. 
        if (libdap::dods_array_c == v->type()) {
cerr<<"first v name is "<<v->name() <<endl;

            bool is_cv = is_cvar(v,coname_array_maps,dc_array_maps);
            if (!is_cv) {

                auto t_a = static_cast<Array *>(*vi);
                Array v_copy = *t_a;
cerr<<"coming before del_var"<<endl;
cerr<<"v name is "<<v->name() <<endl;
                // delete and add var.
                Constructor::Vars_iter vi_copy = vi ;
                //vi++;
                if (vi !=ve) {
                BaseType *v_temp = *vi;
cerr<<"v_temp  name is "<<v_temp->name() <<endl;
}
                
                d4_grp->del_var(vi_copy);
cerr<<"coming after del_var"<<endl;
                d4_grp->add_var(&v_copy);
cerr<<"v_copy name "<<v_copy.name() <<endl;
cerr<<"coming after add_var_nocopy"<<endl;
                vi++;
            }
            else 
                vi++;

        }
        else 
            vi++;

    }
#endif

    for (D4Group::groupsIter gi = d4_grp->grp_begin(), ge = d4_grp->grp_end(); gi != ge; ++gi) {
        //    BESDEBUG(MODULE, prolog << "In group:  " << (*gi)->name() << endl);
        reorder_vars(*gi, coname_array_maps, dc_array_maps);
    }   
    
}

bool is_cvar(BaseType *v, const unordered_map<string,Array*> &coname_array_maps, const unordered_map<string,Array*> & dc_array_maps) {

    bool ret_value = false;
    unordered_map<string, Array*>::const_iterator it_ma = coname_array_maps.find(v->FQN());
    if (it_ma != coname_array_maps.end()) 
        ret_value = true;
    else {
        it_ma = dc_array_maps.find(v->FQN());
        if (it_ma != dc_array_maps.end()) 
            ret_value = true;
    }
    return ret_value;
}

