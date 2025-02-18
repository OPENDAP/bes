// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 401 E University Avenue,
// Suite 200, Champaign, IL 61820

///////////////////////////////////////////////////////////////////////////////
/// \file h5dmr.cc
/// \brief DMR(Data Metadata Response) request processing source
///
/// This file is part of HDF5 handler, a C++ implementation of the DAP4
/// protocol to access HDF5 data.
///
/// This file contains functions that use the breadth-first search to walk through
/// an HDF5 file and build the in-memory DMR.
/// Using breadth-first ensures the correct DAP4 DMR layout(group's variables first and then the groups).
/// \author Kent Yang    <myang6@hdfgroup.org>
///

#include <sstream>
#include <memory>

#include <libdap/InternalErr.h>
#include <libdap/mime_util.h>
#include <libdap/D4Maps.h>

#include <BESDebug.h>

#include "hdf5_handler.h"
#include "HDF5Int32.h"
#include "HDF5UInt32.h"
#include "HDF5UInt16.h"
#include "HDF5Int16.h"
#include "HDF5Array.h"
#include "HDF5Float32.h"
#include "HDF5Float64.h"
#include "HDF5Url.h"
#include "HDF5Structure.h"
#include "h5dmr.h"

// The HDF5CFUtil.h includes the utility function obtain_string_after_lastslash.
#include "HDF5CFUtil.h"
#include "h5commoncfdap.h"

// For HDF-EOS5 handling
#include "HE5Parser.h"
#include "HE5Checker.h"
#include "HDF5CFProj.h"
#include "HDF5CFProj1D.h"
#include "HDF5MissLLArray.h"

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

void array_add_dimensions_dimscale(HDF5Array* ar);
bool array_add_dimensions_non_dimscale(HDF5Array *ar, const string &varname, const eos5_dim_info_t &eos5_dim_info);
void read_objects_basetype_add_eos5_grid_mapping(const eos5_dim_info_t &eos5_dim_info, BaseType *new_var,const HDF5Array *ar);
void write_dap4_attr(hid_t attr_id, libdap::D4Attribute *d4_attr, hid_t ty_id, const DSattr_t &attr_inst);
void write_dap4_attr_value(D4Attribute *d4_attr, hid_t ty_id, hsize_t nelmts, char *tempvalue,
                           size_t elesize = 0);

//////////////////////////////////////////////////////////////////////////////////////////
/// bool breadth_first(const hid_t file_id,hid_t pid, const char *gname, 
///                     D4Group* par_grp, const char *fname,bool use_dimscale,bool is_eos5,
///                     vector<link_info_t> & hdf5_hls, eos5_dim_info_t & eos5_dim_info,
///                     vector<string> & handled_cv_names)
/// \param file_id file_id(this is necessary for searching the hardlinks of a dataset)
/// \param pid group id
/// \param gname group name (the absolute path from the root group)
/// \param par_grp DAP4 parent group
/// \param fname the HDF5 file name( necessary parameters for HDF5 Arrays)
/// \param use_dimscale whether dimension scales are used.
/// \param is_eos5  whether this is an HDF-EOS5 file.
/// \param hdf5_hls the vector to save all the hardlink info.
/// \param eos5_dim_info in/out the struct to save the HDF-EOS5 dimension info.
/// \param handled_cv_names in/out the vector to remember the handled cv names(for the use of DAP4 coverage support)
/// \return true, 
///
/// \remarks hard link is treated as a dataset.
/// \remarks will return error message to the DAP interface.
/// \remarks The reason to use breadth_first is that the DMR representation
/// \remarks needs to show the dimension names and the variables under the group before showing the group names.
///  So we use this search. This is for the default option. 
/// \see depth_first(hid_t pid, char *gname, DDS & dds, const char *fname) in h5dds.cc
///////////////////////////////////////////////////////////////////////////////

bool breadth_first(hid_t file_id, hid_t pid, const char *gname,
                   D4Group* par_grp, const char *fname,
                   bool use_dimscale,bool is_eos5, 
                   vector<link_info_t> & hdf5_hls,
                   eos5_dim_info_t & eos5_dim_info,
                   vector<string> & handled_cv_names)
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
    if (H5Gget_info(pid,&g_info) < 0) {
        string msg =
            "h5_dmr: counting hdf5 group elements error for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    nelems = g_info.nlinks;

    // First iterate through the HDF5 datasets under the group.
    for (hsize_t i = 0; i < nelems; i++) {

        vector <char> oname;
        obtain_hdf5_object_name(pid, i, gname, oname);
        if (true == check_soft_external_links(par_grp, pid, slinkindex,gname, oname,true))
            continue;

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;
        if (H5OGET_INFO_BY_IDX(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT) <0 ) {
            string msg = "h5_dmr handler: Error obtaining the info for the object";
            msg += string(oname.begin(),oname.end());
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        H5O_type_t obj_type = oinfo.type;

        if (H5O_TYPE_DATASET == obj_type) {

            // Obtain the absolute path of the HDF5 dataset
            string full_path_name = string(gname) + string(oname.begin(),oname.end()-1);

            // Obtain the hdf5 dataset handle stored in the structure dt_inst. 
            // All the metadata information in the handler is stored in dt_inst.
            // Dimension scale is handled in this routine.  KY 2020-06-10
            bool is_pure_dim = false;
            get_dataset_dmr(file_id, pid, full_path_name, &dt_inst,use_dimscale,is_eos5,
                            is_pure_dim,hdf5_hls,handled_cv_names);
               
            // pure dimensions are netCDF-4's dimensions only. They are not variables in the netCDF-4 term.
            if (false == is_pure_dim) {
                handle_actual_dataset(par_grp, pid, full_path_name, fname, use_dimscale,
                                      is_eos5, eos5_dim_info);
            }
            else
                handle_pure_dimension(par_grp, pid, oname, is_eos5, full_path_name);
        } 
    }
   
    // The attributes of this group. Doing this order to follow ncdump's way (variable,attribute then groups)
    map_h5_attrs_to_dap4(pid,par_grp,nullptr,nullptr,0);

    if (is_eos5 && !use_dimscale)
        handle_eos5_datasets(par_grp, gname, eos5_dim_info);

    // The fullnamepath of the group is not necessary since dmrpp only needs the dataset path to retrieve info.
    // It only increases the dmr file size. So comment out for now.  KY 2022-10-13
#if 0
    if (is_eos5)
        map_h5_varpath_to_dap4_attr(par_grp,nullptr,nullptr,gname,0);
#endif

    // Then HDF5 child groups
    for (hsize_t i = 0; i < nelems; i++) {

        vector <char>oname;
        obtain_hdf5_object_name(pid, i, gname, oname);
        if (true == check_soft_external_links(par_grp, pid, slinkindex,gname, oname,false))
            continue;

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;

        if (H5OGET_INFO_BY_IDX(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT) < 0 ) {
            string msg = "h5_dmr handler: Error obtaining the info for the object in the breadth_first.";
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        H5O_type_t obj_type = oinfo.type;

        if (obj_type == H5O_TYPE_GROUP) {
            handle_child_grp(file_id, pid, gname, par_grp, fname, use_dimscale, is_eos5, hdf5_hls, eos5_dim_info,
                             handled_cv_names, oname);
        }
    } // for i is 0 ... nelems

    BESDEBUG("h5", "<breadth_first() " << endl);
    return true;
}

void obtain_hdf5_object_name(hid_t pid, hsize_t obj_index, const char *gname, vector<char> &oname) {

    ssize_t oname_size;

    // Query the length of this object name.
    oname_size = H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,obj_index,
                                    nullptr,(size_t)DODS_NAMELEN, H5P_DEFAULT);
    if (oname_size <= 0) {
        string msg = "h5_dmr handler: Error getting the size of the hdf5 object from the group: ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Obtain the name of the object
    oname.resize((size_t) oname_size + 1);

    if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,obj_index,
                           oname.data(),(size_t)(oname_size+1), H5P_DEFAULT) < 0) {
        string msg = "h5_dmr handler: Error getting the hdf5 object name from the group: ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
}

bool check_soft_external_links(D4Group *par_grp, hid_t pid, int & slinkindex,
                               const char *gname, const vector<char> &oname, bool handle_softlink) {

    bool ret_value = false;

    // Check the HDF5 link info.
    H5L_info_t linfo;
    if (H5Lget_info(pid,oname.data(),&linfo,H5P_DEFAULT) < 0) {
        string msg = "hdf5 link name error from: ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Information of soft links are stored as attributes
    if (linfo.type == H5L_TYPE_SOFT) {
        ret_value = true;
        if (handle_softlink == false) {
            slinkindex++;
            // Size of a soft link value
            size_t val_size = linfo.u.val_size;
            get_softlink(par_grp, pid, oname.data(), slinkindex, val_size);
        }
    }
    else if (linfo.type == H5L_TYPE_EXTERNAL) // Ignore external links
        ret_value = true;

    return ret_value;
}

void handle_actual_dataset(D4Group *par_grp, hid_t pid, const string &full_path_name, const string &fname,
                           bool use_dimscale, bool is_eos5, eos5_dim_info_t &eos5_dim_info) {

    hid_t dset_id = -1;
    if ((dset_id = H5Dopen(pid,full_path_name.c_str(),H5P_DEFAULT)) < 0) {
        string msg = "cannot open the HDF5 dataset  ";
        msg += full_path_name;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    try {
        read_objects(par_grp, pid, full_path_name, fname,dset_id,use_dimscale,is_eos5,eos5_dim_info);
    }
    catch(...) {
        H5Dclose(dset_id);
        throw;
    }
    if (H5Dclose(dset_id) < 0) {
        string msg = "cannot close the HDF5 dataset  ";
        msg += full_path_name;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

}

void handle_pure_dimension(D4Group *par_grp, hid_t pid, const vector<char>& oname, bool is_eos5,
                           const string &full_path_name) {

    // Need to add this pure dimension to the corresponding DAP4 group
    D4Dimensions *d4_dims = par_grp->dims();
    string d4dim_name;
    if (is_eos5) {
        auto tempdim_name = string(oname.begin(),oname.end()-1);
        d4dim_name = handle_string_special_characters(tempdim_name);
    }
    else
        d4dim_name = string(oname.begin(),oname.end()-1);

    const D4Dimension *d4_dim = d4_dims->find_dim(d4dim_name);
    if (d4_dim == nullptr) {

        hsize_t nelmts = dt_inst.nelmts;

        // For pure dimension, if the dimension size is 0,
        // the dimension is unlimited or a NULL data space, so we need to use the object reference
        // to retrieve the variable this dimension is attached and then for the same variable retrieve
        // the size of this dimension.
        if (dt_inst.nelmts == 0)
            nelmts = obtain_unlim_pure_dim_size(pid,full_path_name);

        auto d4_dim_unique = make_unique<D4Dimension>(d4dim_name, nelmts);
        d4_dims->add_dim_nocopy(d4_dim_unique.release());
    }

    BESDEBUG("h5", "<h5dmr.cc: pure dimension: dataset name." << d4dim_name << endl);

    if (H5Tclose(dt_inst.type)<0) {
          throw InternalErr(__FILE__, __LINE__, "Cannot close the HDF5 datatype.");
    }

}

void handle_eos5_datasets(D4Group* par_grp, const char *gname, eos5_dim_info_t &eos5_dim_info) {

    // To support HDF-EOS5 grids, we have to add extra variables.
    // These variables are geo-location related variables such as latitude and longitude.
    // These geo-location variables are DAP4 coverage map variable candidates.
    // And to follow the DAP4 coverage specification, we need to define map variables.
    // The map variables need to be in *front* of all the variables that use the map variables.
    // So here we have to insert these extra variables if an HDF-EOS5 grid is found.
    // We may need to remember the full path of these extra variables. These will be
    // used as the coordinates of this group's data variables. For the geographic projection,
    // this is not necessary.
    add_possible_eos5_grid_vars(par_grp, eos5_dim_info);

    //Also need to add DAP4 dimensions to this group if there are HDF-EOS5 dimensions.
    unordered_map<string,vector<HE5Dim>> grppath_to_dims = eos5_dim_info.grppath_to_dims;
    vector<string> dim_names;
    auto par_grp_name = string(gname);
    if (par_grp_name.size() > 1)
        par_grp_name = par_grp_name.substr(0,par_grp_name.size()-1);

    BESDEBUG("h5", "<h5dmr.cc: eos5 handling - parent group name: " << par_grp_name << endl);

    // We need to ensure the special characters are handled.
    bool is_eos5_dims = obtain_eos5_grp_dim(par_grp_name,grppath_to_dims,dim_names);

    if (is_eos5_dims) {

        vector<HE5Dim> grp_eos5_dim = grppath_to_dims[par_grp_name];
        D4Dimensions *d4_dims = par_grp->dims();
        for (unsigned grp_dim_idx = 0; grp_dim_idx < dim_names.size(); grp_dim_idx++) {

            const D4Dimension *d4_dim = d4_dims->find_dim(dim_names[grp_dim_idx]);
            if (d4_dim == nullptr) {
                auto d4_dim_unique =
                        make_unique<D4Dimension>(dim_names[grp_dim_idx],grp_eos5_dim[grp_dim_idx].size);
                d4_dims->add_dim_nocopy(d4_dim_unique.release());
            }
        }
    }
}

void handle_child_grp(hid_t file_id, hid_t pid, const char *gname,
                      D4Group* par_grp, const char *fname,
                      bool use_dimscale,bool is_eos5,
                      vector<link_info_t> & hdf5_hls,
                      eos5_dim_info_t & eos5_dim_info,
                      vector<string> & handled_cv_names,
                      const vector<char>& oname) {

    // Obtain the full path name
    string full_path_name =
        string(gname) + string(oname.begin(),oname.end()-1) + "/";

    BESDEBUG("h5", "=breadth_first dmr ():H5G_GROUP " << full_path_name
        << endl);

    vector <char>t_fpn;
    t_fpn.resize(full_path_name.size() + 1);
    copy(full_path_name.begin(),full_path_name.end(),t_fpn.begin());
    t_fpn[full_path_name.size()] = '\0';

    hid_t cgroup = H5Gopen(pid, t_fpn.data(),H5P_DEFAULT);
    if (cgroup < 0){
        throw InternalErr(__FILE__, __LINE__, "h5_dmr handler: H5Gopen() failed.");
    }

    auto grp_name = string(oname.begin(),oname.end()-1);

    // Check the hard link loop and break the loop if it exists.
    string oid = get_hardlink_dmr(cgroup, full_path_name);
    if (oid.empty()) {

        try {
            if (is_eos5)
                grp_name = handle_string_special_characters(grp_name);
            auto tem_d4_cgroup_ptr = make_unique<D4Group>(grp_name);
            auto tem_d4_cgroup = tem_d4_cgroup_ptr.release();

            // Add this new DAP4 group
            par_grp->add_group_nocopy(tem_d4_cgroup);

            // Continue searching the objects under this group
            breadth_first(file_id,cgroup, t_fpn.data(), tem_d4_cgroup,fname,
                          use_dimscale,is_eos5,hdf5_hls,eos5_dim_info,handled_cv_names);
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
        auto tem_d4_cgroup_unique = make_unique<D4Group>(grp_name);
        auto tem_d4_cgroup = tem_d4_cgroup_unique.release();

        // Note attr_str_c is the DAP4 attribute string datatype
        auto d4_hlinfo_unique = make_unique<D4Attribute>("HDF5_HARDLINK",attr_str_c);
        auto d4_hlinfo = d4_hlinfo_unique.release();

        d4_hlinfo->add_value(obj_paths.get_name(oid));
        tem_d4_cgroup->attributes()->add_attribute_nocopy(d4_hlinfo);
        par_grp->add_group_nocopy(tem_d4_cgroup);
    }

    if (H5Gclose(cgroup) < 0){
        throw InternalErr(__FILE__, __LINE__, "Could not close the group.");
    }
}
/////////////////////////////////////////////////////////////////////////////// 
///// \fn read_objects( D4Group *d4_grp, 
/////                            hid_t pid,
/////                            const string & varname, 
/////                            const string & filename,
/////                            hid_t dset_id,
/////                            bool use_dimscale
/////                            bool is_eos5
/////                            eos5_dim_info_t eos5_dim_info)
///// fills in information of a dataset (name, data type, data space) into the dap4 
///// group. 
///// This is a wrapper function that calls functions to read atomic types and structure.
///// 
/////    \param d4_group DAP4 group
/////    \param pid  HDF5 parent group id.
/////    \param varname Absolute name of an HDF5 dataset.  
/////    \param filename The HDF5 dataset name that maps to the DMR dataset name.
////     \param dset_id HDF5 dataset id.
/////    \param use_dimscale boolean that indicates if dimscale is used.
/////    \param is_eos5 boolean that indicates if this is an HDF-EOS5 file.
/////    \param eos5_dim_info a struct to handle eos5 dimension info.
/////    \throw error a string of error message to the dap interface.
/////////////////////////////////////////////////////////////////////////////////
//
void
read_objects( D4Group * d4_grp, hid_t pid, const string &varname, const string &filename, hid_t dset_id,bool use_dimscale,
              bool is_eos5, eos5_dim_info_t & eos5_dim_info) {

    // NULL space data, ignore.
    if (dt_inst.ndims == -1 && dt_inst.nelmts == 0) 
        return;

    switch (H5Tget_class(dt_inst.type)) {

    // HDF5 compound maps to DAP structure.
    case H5T_COMPOUND:
        read_objects_structure(d4_grp, varname, filename,dset_id,use_dimscale,is_eos5);
        break;

    case H5T_ARRAY:
        H5Tclose(dt_inst.type);
        throw InternalErr(__FILE__, __LINE__,
                          "Currently don't support accessing data of Array datatype when array datatype is not inside the compound.");
    
    default:
        read_objects_base_type(d4_grp,pid,varname, filename,dset_id,use_dimscale,is_eos5,eos5_dim_info);
        break;
    }
    // Close the datatype obtained in the get_dataset_dmr() since the datatype is no longer used.
    if (H5Tclose(dt_inst.type) < 0) {
        throw InternalErr(__FILE__, __LINE__, "Cannot close the HDF5 datatype.");       
    }
}

void array_add_dimensions_dimscale(HDF5Array *ar){

    for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
        if (dt_inst.dimnames[dim_index].empty() == false)
            ar->append_dim_ll(dt_inst.size[dim_index], dt_inst.dimnames[dim_index]);
        else
            ar->append_dim_ll(dt_inst.size[dim_index]);
    }
    dt_inst.dimnames.clear();

}

bool array_add_dimensions_non_dimscale(HDF5Array *ar, const string &varname, const eos5_dim_info_t &eos5_dim_info) {

    // Without using the dimension scales, the HDF5 file may still have dimension names(HDF-EOS5 etc.).
    // We search the file to see if there are dimension names. If yes, add them here.
    vector<string> dim_names;
    bool is_eos5_dims = obtain_eos5_dim(varname, eos5_dim_info.varpath_to_dims, dim_names);

    if (is_eos5_dims) {
        for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++)
            ar->append_dim_ll(dt_inst.size[dim_index], dim_names[dim_index]);
    } else {
        for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++)
            ar->append_dim_ll(dt_inst.size[dim_index]);
    }
    return is_eos5_dims;
}

void read_objects_basetype_add_eos5_grid_mapping(const eos5_dim_info_t &eos5_dim_info,
                                                 BaseType *new_var,const HDF5Array *ar) {

    if ((eos5_dim_info.dimpath_to_cvpath.empty() == false) && (ar->get_numdim() > 1))
        add_possible_var_cv_info(new_var, eos5_dim_info);
    if (eos5_dim_info.gridname_to_info.empty() == false)
        make_attributes_to_cf(new_var, eos5_dim_info);

}

/////////////////////////////////////////////////////////////////////////////// 
///// \fn read_objects_base_type(D4Group *d4_grp,hid_t pid,
/////                            const string & varname, 
/////                            const string & filename,hid_t dset_id, bool use_dimscale
///                              bool is_eos5, eos5_dim_info_t eos5_dim_info)
///// fills in information of a dataset (name, data type, data space) with HDF5 atomic datatypes into the dap4 
///// group. 
/////
/////    \param d4_grp DAP4 group
/////    \param pid HDF5 parent group id
/////    \param varname Absolute name of an HDF5 dataset.  
/////    \param filename The HDF5 dataset name that maps to the DDS dataset name. 
////     \param dset_id HDF5 dataset id.
/////    \param use_dimscale boolean that indicates if dimscale is used.
/////    \param eos5_dim_info a struct to handle eos5 dimension info.
/////    \param is_eos5 boolean that indicates if this is an HDF-EOS5 file.
/////    \param eos5_dim_info a struct to handle eos5 dimension info.
/////    \throw error a string of error message to the dods interface. 
/////////////////////////////////////////////////////////////////////////////////
//

void
read_objects_base_type(D4Group * d4_grp, hid_t pid, const string & varname, const string & filename, hid_t dset_id,
                       bool use_dimscale, bool is_eos5, eos5_dim_info_t &eos5_dim_info)
{

    string newvarname = obtain_new_varname(varname, use_dimscale, is_eos5);

    hid_t datatype = H5Dget_type(dset_id);

    // If this is an HDF5 variable length float/integer type, we need to handle it differently.
    if (H5Tget_class(datatype) == H5T_VLEN) {
        handle_vlen_int_float(d4_grp, pid, newvarname, varname, filename,dset_id);
        H5Tclose(datatype);
        return;
    }

    H5Tclose(datatype);
    // Get a base type. It should be an HDF5 atomic datatype.
    BaseType *bt = Get_bt_enhanced(d4_grp,pid, newvarname,varname, filename, dt_inst.type);
    if (!bt)
        throw InternalErr(__FILE__, __LINE__,"Unable to convert hdf5 datatype to dods basetype");

    // First deal with scalar data. 
    if (dt_inst.ndims == 0) {

        // Mark this base type as an DAP4 object
        bt->set_is_dap4(true);
        read_objects_basetype_attr_hl(varname, bt, dset_id, is_eos5);

        // Attach this object to the DAP4 group
        d4_grp->add_var_nocopy(bt);
    }
    else {

        // Next, deal with Array data. This 'else clause' runs to
        // the end of the method.
        auto ar_unique = make_unique<HDF5Array>(newvarname, filename, bt);
        HDF5Array *ar = ar_unique.get();
        delete bt;
        bt = nullptr;

        // set number of elements and variable name values.
        // This essentially stores in the struct.
        ar->set_memneed(dt_inst.need);
        ar->set_numdim(dt_inst.ndims);
        ar->set_numelm((dt_inst.nelmts));
        ar->set_varpath(varname);

        // If we have dimension names(dimension scale is used.),we will see if we can add the names.       
        int dimnames_size = 0;
        if ((unsigned int) ((int) (dt_inst.dimnames.size())) != dt_inst.dimnames.size())
            throw InternalErr(__FILE__, __LINE__,"number of dimensions: overflow");

        dimnames_size = (int) (dt_inst.dimnames.size());

        bool is_eos5_dims = false;
        if (dimnames_size == dt_inst.ndims)
            array_add_dimensions_dimscale(ar);
        else {
            // With using the dimension scales, the HDF5 file may still have dimension names such as HDF-EOS5.
            // We search if there are dimension names. If yes, add them here.
            is_eos5_dims = array_add_dimensions_non_dimscale(ar, varname, eos5_dim_info);
        }

        // We need to transform dimension info. to DAP4 group
        BaseType *new_var = nullptr;

        if (is_eos5_dims)
            new_var = ar->h5dims_transform_to_dap4(d4_grp, eos5_dim_info.varpath_to_dims.at(varname));
        else
            new_var = ar->h5dims_transform_to_dap4(d4_grp, dt_inst.dimnames_path);

        // clear DAP4 dimnames_path vector
        dt_inst.dimnames_path.clear();

        read_objects_basetype_attr_hl(varname, new_var, dset_id,  is_eos5);

        // Here we need to add grid_mapping information if necessary.
        if (is_eos5_dims && !use_dimscale)
            read_objects_basetype_add_eos5_grid_mapping(eos5_dim_info, new_var, ar);

        // Add this var to DAP4 group.
        d4_grp->add_var_nocopy(new_var);
    }
    BESDEBUG("h5", "<read_objects_base_type(dmr)" << endl);
}


void read_objects_basetype_attr_hl(const string &varname, BaseType *bt, hid_t dset_id,  bool is_eos5) {

    // Mark this base type as an DAP4 object
    bt->set_is_dap4(true);

    // Map the HDF5 dataset attributes to DAP4
    map_h5_attrs_to_dap4(dset_id,nullptr,bt,nullptr,1);

    // If this variable is a hardlink, stores the HARDLINK info. as an attribute.
    map_h5_dset_hardlink_to_d4(dset_id,varname,bt,nullptr,1);

    // If this is an HDF-EOS5 object, we need to map the full path of this object to a DAP4 attribute.
    if (is_eos5)
        map_h5_varpath_to_dap4_attr(nullptr,bt,nullptr,varname,1);

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
    string newvarname = obtain_new_varname(varname, use_dimscale, is_eos5);

    // Map HDF5 compound datatype to Structure
    Structure *structure = Get_structure(newvarname, varname,filename, dt_inst.type,true);

    // TODO: compound datatype should not be used by HDF-EOS5. Still we may add those support.
    try {
        BESDEBUG("h5", "=read_objects_structure(): Dimension is " 
            << dt_inst.ndims << endl);

        if (dt_inst.ndims != 0) // Array of Structure
            read_objects_structure_arrays(d4_grp, structure, varname,newvarname, filename, dset_id, is_eos5);
        else // A scalar structure
            read_objects_structure_scalar(d4_grp, structure, varname,dset_id, is_eos5);
    } // try  Structure 
    catch (...) {
        delete structure;
        throw;
    }
}

void read_objects_structure_arrays(D4Group *d4_grp, Structure *structure, const string & varname,
                                   const string &newvarname, const string & filename, hid_t dset_id, bool is_eos5)
{
    BESDEBUG("h5", "=read_objects_structure(): array of size " << dt_inst.nelmts << endl);
    BESDEBUG("h5", "=read_objects_structure(): memory needed = " << dt_inst.need << endl);

    // Create the Array of structure.
    auto ar_unique = make_unique<HDF5Array>(newvarname, filename, structure);
    HDF5Array *ar = ar_unique.get();
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
        throw InternalErr(__FILE__, __LINE__,
                       "number of dimensions: overflow");
    }
    dimnames_size = (int)(dt_inst.dimnames.size());

    if (dimnames_size ==dt_inst.ndims) {
        for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
            if (dt_inst.dimnames[dim_index].empty() ==false)
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

}

void read_objects_structure_scalar(D4Group *d4_grp, Structure *structure, const string & varname,
                                   hid_t dset_id, bool is_eos5)
{
    structure->set_is_dap4(true);
    map_h5_attrs_to_dap4(dset_id,nullptr,nullptr,structure,2);
    map_h5_dset_hardlink_to_d4(dset_id,varname,nullptr,structure,2);
    if (is_eos5)
        map_h5_varpath_to_dap4_attr(nullptr,nullptr,structure,varname,2);
    if(structure)
        d4_grp->add_var_nocopy(structure);
}

string obtain_new_varname(const string &varname, bool use_dimscale, bool is_eos5) {

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
    return newvarname;
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
    auto num_attrs = (int)(obj_info.num_attrs);
    if (num_attrs < 0 ) {
        string msg = "Fail to get the number of attributes for the HDF5 object. ";
        throw InternalErr(__FILE__, __LINE__,msg);
    }

    bool ignore_attr = false;
    hid_t attr_id = -1;

    for (int j = 0; j < num_attrs; j++) {

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
        if (ty_id < 0) {
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
        BESDEBUG("h5", "attr_name= " << attr_name << endl);

        // Create the DAP4 attribute mapped from HDF5
        //auto d4_attr = new D4Attribute(attr_name,dap4_attr_type);
        auto d4_attr_unique = make_unique<D4Attribute>(attr_name, dap4_attr_type);
        D4Attribute *d4_attr = d4_attr_unique.release();

        // Check if utf8 string.
        if (dap4_attr_type == attr_str_c && check_if_utf8_str(ty_id) )
                d4_attr->set_utf8_str_flag(true);

        // We have to handle variable length string differently. 
        if (H5Tis_variable_str(ty_id))  
            write_vlen_str_attrs(attr_id,ty_id,&attr_inst,d4_attr,nullptr,true);
        else
            write_dap4_attr(attr_id, d4_attr, ty_id, attr_inst);

        H5Tclose(ty_id);
        H5Aclose(attr_id);

        if(0 == flag) // D4group
            d4g->attributes()->add_attribute_nocopy(d4_attr);
        else if (1 == flag) // HDF5 dataset with atomic datatypes 
            d4b->attributes()->add_attribute_nocopy(d4_attr);
        else if ( 2 == flag) // HDF5 dataset with compound datatype
            d4s->attributes()->add_attribute_nocopy(d4_attr);
        else {
            stringstream sflag;
            sflag << flag;
            string msg = "The add_dap4_attr flag has to be either 0,1 or 2.";
            msg +="The current flag is "+sflag.str();
            delete d4_attr;
            throw InternalErr(__FILE__, __LINE__, msg);
        }
    } // for (int j = 0; j < num_attr; j++)

}

void write_dap4_attr(hid_t attr_id, libdap::D4Attribute *d4_attr, hid_t ty_id, const DSattr_t &attr_inst) {

    vector<char> value;
    value.resize(attr_inst.need);
    BESDEBUG("h5", "arttr_inst.need=" << attr_inst.need << endl);

    // Need to obtain the memtype since we still find BE data.
    hid_t memtype = H5Tget_native_type(ty_id, H5T_DIR_ASCEND);

    // Read HDF5 attribute data.
    if (H5Aread(attr_id, memtype, (void *) (value.data())) < 0) {
        H5Tclose(ty_id);
        H5Aclose(attr_id);
        delete d4_attr;
        throw InternalErr(__FILE__, __LINE__, "unable to read HDF5 attribute data");
    }
    H5Aclose(memtype);

    // Due to the implementation of print_attr, the attribute value will be
    // written one by one.
    char *tempvalue = value.data();

    // For scalar data, just read data once.
    if (attr_inst.ndims == 0)
        write_dap4_attr_value(d4_attr,ty_id,1,tempvalue);
    else {// The number of dimensions is > 0

        // Get the attribute datatype size
        auto elesize = (int) H5Tget_size(ty_id);
        if (elesize == 0) {
            H5Tclose(ty_id);
            H5Aclose(attr_id);
            delete d4_attr;
            throw InternalErr(__FILE__, __LINE__, "unable to get attibute size");
        }

        write_dap4_attr_value(d4_attr,ty_id,attr_inst.nelmts,tempvalue, elesize);
    } // if attr_inst.ndims != 0
}

void write_dap4_attr_value(D4Attribute *d4_attr, hid_t ty_id, hsize_t nelmts, char *tempvalue, size_t elesize) {

    // Write this value. the "loc" can always be set to 0 since
    // tempvalue will be moved to the next value.
    string print_rep;
    for (hsize_t temp_index = 0; temp_index < nelmts; temp_index++) {

        print_rep = print_attr(ty_id, 0, tempvalue);
        if (print_rep.c_str() != nullptr) {

            BESDEBUG("h5", "print_rep= " << print_rep << endl);
            d4_attr->add_value(print_rep);
            tempvalue = tempvalue + elesize;
            BESDEBUG("h5",
                     "tempvalue= " << tempvalue
                                   << "elesize=" << elesize
                                   << endl);
        } else {
            H5Tclose(ty_id);
            delete d4_attr;
            throw InternalErr(__FILE__, __LINE__, "unable to convert attribute value to DAP");
        }
    }
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

        auto d4_hlinfo_unique = make_unique<D4Attribute>("HDF5_HARDLINK",attr_str_c);
        auto d4_hlinfo = d4_hlinfo_unique.release();
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

    auto d4_attr_unique = make_unique<D4Attribute>("fullnamepath",attr_str_c);
    auto d4_attr = d4_attr_unique.release();
    d4_attr->add_value(varpath);

    if (0 == flag) // D4group
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
    BESDEBUG("h5", "dap4 get_softlink():" << oname << endl);

    ostringstream oss;
    oss << string("HDF5_SOFTLINK");
    oss << "_";
    oss << index;
    string temp_varname = oss.str();

    BESDEBUG("h5", "dap4->get_softlink():link name " << temp_varname << endl);

    auto d4_slinfo_unique = make_unique<D4Attribute>();
    auto d4_slinfo = d4_slinfo_unique.release();
    d4_slinfo->set_name(temp_varname);

    // Make the type as a container
    d4_slinfo->set_type(attr_container_c);

    string softlink_name = "linkname";

    auto softlink_src_unique = make_unique<D4Attribute>(softlink_name, attr_str_c);
    auto softlink_src = softlink_src_unique.release();
    softlink_src->add_value(oname);

    d4_slinfo->attributes()->add_attribute_nocopy(softlink_src);
    string softlink_value_name ="LINKTARGET";

    vector<char> buf;
    buf.resize(val_size + 1);

    // get link target name
    if (H5Lget_val(h5obj_id, oname.c_str(), (void*) buf.data(), val_size + 1, H5P_DEFAULT) < 0) {
        throw InternalErr(__FILE__, __LINE__, "unable to get link value");
    }
    auto softlink_tgt_unique = make_unique<D4Attribute>(softlink_value_name, attr_str_c);
    auto softlink_tgt = softlink_tgt_unique.release();

    auto link_target_name = string(buf.begin(), buf.end());
    softlink_tgt->add_value(link_target_name);

    d4_slinfo->attributes()->add_attribute_nocopy(softlink_tgt);
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
    if (H5OGET_INFO(h5obj_id, &obj_info) <0)
        throw InternalErr(__FILE__, __LINE__, "H5OGET_INFO() failed.");

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
        // it is essentially an id to obj name map. See HDF5PathFinder.h.
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

// This function is to retrieve structmetadata from an HDF-EOS5 file.
string read_struct_metadata(hid_t s_file_id) {

    BESDEBUG("h5","Coming to read_struct_metadata()  "<<endl);
    
    string total_strmeta_value;
    string ecs_group = "/HDFEOS INFORMATION";
    hid_t ecs_grp_id = -1;
    if ((ecs_grp_id = H5Gopen(s_file_id, ecs_group.c_str(),H5P_DEFAULT)) < 0) {
        string msg = "h5_ecs_meta: unable to open the HDF5 group  ";
        msg += ecs_group;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    H5G_info_t g_info;
    hsize_t nelems = 0;

    if (H5Gget_info(ecs_grp_id,&g_info) < 0) {
        string msg = "h5_ecs_meta: unable to obtain the HDF5 group info. for ";
        msg +=ecs_group;
        H5Gclose(ecs_grp_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    nelems = g_info.nlinks;

    // Initialize the total number of structure metadata.
    int strmeta_num_total   = 0;
    bool strmeta_no_suffix  = true;

    // Define a vector of string to hold all dataset names.
    vector<string> s_oname(nelems);

    // Define an EOSMetadata array that can describe the metadata type for each object
    // We initialize the value to OtherMeta.
    vector<bool> smetatype(nelems,false);

    obtain_struct_metadata_info(ecs_grp_id, s_oname, smetatype,
                                strmeta_num_total, strmeta_no_suffix, nelems);

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

    int strmeta_num = obtain_struct_metadata_value(ecs_grp_id, s_oname,smetatype, nelems,
                                                strmeta_value, total_strmeta_value);

    // Now we need to handle the concatenation of the metadata
    if ((strmeta_num_total > 0) && (strmeta_num != -1) ) {
        // The no suffix one has been taken care.
        for (int i = 0; i <strmeta_num_total; i++) 
            total_strmeta_value +=strmeta_value[i];
    }

    return total_strmeta_value;
}

void obtain_struct_metadata_info(hid_t ecs_grp_id, vector<string> &s_oname, vector<bool> &smetatype,
                                 int &strmeta_num_total, bool &strmeta_no_suffix, hsize_t nelems) {

    string ecs_group = "/HDFEOS INFORMATION";
    ssize_t oname_size      = 0;
    for (hsize_t i = 0; i < nelems; i++) {

        // Query the length of the object name.
        oname_size = H5Lget_name_by_idx(ecs_grp_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                                        i, nullptr,0, H5P_DEFAULT);
        if (oname_size <= 0) {
            string msg = "hdf5 object name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the name of the object.
        vector<char> oname(oname_size + 1);
        if (H5Lget_name_by_idx(ecs_grp_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i,
                               oname.data(),(size_t) (oname_size + 1), H5P_DEFAULT) < 0) {
            string msg = "hdf5 object name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Check if this object is an HDF5 dataset, not, throw an error.
        // First, check if it is the hard link or the soft link
        H5L_info_t linfo;
        if (H5Lget_info(ecs_grp_id, oname.data(), &linfo, H5P_DEFAULT) < 0) {
            string msg = "hdf5 link name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // This is the soft link.
        if (linfo.type == H5L_TYPE_SOFT) {
            string msg = "hdf5 link name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the object type
        H5O_info_t oinfo;
        if (H5OGET_INFO_BY_IDX(ecs_grp_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                               i, &oinfo, H5P_DEFAULT) < 0) {
            string msg = "Cannot obtain the object info ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if (oinfo.type != H5O_TYPE_DATASET) {
            string msg = "hdf5 link name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // We want to remove the last '\0' character added by C .
        string s_one_oname(oname.begin(), oname.end() - 1);
        s_oname[i] = s_one_oname;

        // Calculate how many elements we have for each category(StructMetadata, CoreMetadata, etc.)
        if (((s_one_oname.find("StructMetadata")) == 0) ||
            ((s_one_oname.find("structmetadata")) == 0)) {

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
            } else if (strmeta_num_total > 0)
                strmeta_num_total++;
            // either no suffix or the first time to loop the one having the suffix.
            else if ((0 == s_one_oname.compare("StructMetadata")) ||
                    (0 == s_one_oname.compare("structmetadata")))
                    strmeta_no_suffix = false;
            else strmeta_num_total++;
        }
        oname.clear();
        s_one_oname.clear();
    }
}

int obtain_struct_metadata_value(hid_t ecs_grp_id, const vector<string> &s_oname, const vector<bool> &smetatype,
                                 hsize_t nelems, vector<string> &strmeta_value, string &total_strmeta_value) {

    int strmeta_num = -1;
    // Now we want to retrieve the metadata value and combine them into one string.
    // Here we have to remember the location of every element of the metadata if
    // this metadata has a suffix.
    for (hsize_t i = 0; i < nelems; i++) {

        // We only read StructMetadata string.
        // Struct Metadata is generated by the HDF-EOS5 library, so the
        // name "StructMetadata.??" won't change for real struct metadata.
        //However, we still assume that somebody may not use the HDF-EOS5
        // library to add StructMetadata, the name may be "structmetadata".
        if (((s_oname[i].find("StructMetadata"))!=0) && ((s_oname[i].find("structmetadata"))!=0))
            continue;

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

        if ((H5Dread(s_dset_id,s_ty_id,H5S_ALL,H5S_ALL,H5P_DEFAULT,
                     s_buf.data()))<0) {

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

        if (true == smetatype[i]) {
            strmeta_num = obtain_struct_metadata_value_internal(ecs_grp_id, s_oname, strmeta_value,total_strmeta_value,
                                                                finstr, i);
        }
        tempstr.clear();
        finstr.clear();
    }
    return strmeta_num;
}

int obtain_struct_metadata_value_internal(hid_t ecs_grp_id, const vector<string> &s_oname,
                                           vector<string> &strmeta_value, string &total_strmeta_value,
                                           const string &finstr, hsize_t i)
{
    int strmeta_num = -1;

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
    // strmeta_value at this point should be empty before assigning any value.
    else if (strmeta_value[strmeta_num].empty() == false) {
        string msg = "The structmeta value array at this index should be empty string  ";
        H5Gclose(ecs_grp_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    // assign the string vector to this value.
    else
        strmeta_value[strmeta_num] = finstr;

    return strmeta_num;
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
    unordered_map<string, eos5_grid_info_t> gridname_to_info;

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

    for (const auto &gd:p.grid_list) 
      build_gd_info(gd, gridname_to_info);
    
    for (const auto &za:p.za_list) 
      build_grp_dim_path(za.name,za.dim_list,grppath_to_dims,HE5_TYPE::ZA);

    for (const auto &za:p.za_list) 
      build_var_dim_path(za.name,za.data_var_list,varpath_to_dims,HE5_TYPE::ZA,false);


#if 0
for (const auto& it:varpath_to_dims) {
    cout<<"var path is "<<it.first <<endl; 
    for (const auto &sit:it.second)
        cout<<"var dimension name is "<<sit <<endl; 
}
       
for (const auto& it:grppath_to_dims) {
    cout<<"grp path is "<<it.first <<endl; 
    for (const auto &sit:it.second) {
        cout<<"grp dimension name is "<<sit.name<<endl; 
        cout<<"grp dimension size is "<<sit.size<<endl; 
    }
}

for (const auto &it:gridname_to_info) {
    cout<<"grid name is "<<it.first <<endl; 
    cout<<"grid x dimension name is "<<it.second.xdim_fqn << " and the size is: "<< it.second.xdim_size << endl; 
    cout<<"grid y dimension name is "<<it.second.ydim_fqn << " and the size is: "<< it.second.ydim_size << endl; 
}
#endif 

    eos5_dim_info.varpath_to_dims = varpath_to_dims;
    eos5_dim_info.grppath_to_dims = grppath_to_dims;
    eos5_dim_info.gridname_to_info = gridname_to_info;
}

void build_grp_dim_path(const string & eos5_obj_name, const vector<HE5Dim>& dim_list, unordered_map<string,
                        vector<HE5Dim>>& grppath_to_dims, HE5_TYPE e5_type) {

    string eos_name_prefix = "/HDFEOS/";
    string eos5_grp_path;

    switch (e5_type) {  
       case HE5_TYPE::SW: 
            eos5_grp_path = eos_name_prefix + "SWATHS/" + eos5_obj_name;      
            break;
       case HE5_TYPE::GD: 
            eos5_grp_path = eos_name_prefix + "GRIDS/" + eos5_obj_name;      
            break;
       case HE5_TYPE::ZA: 
            eos5_grp_path = eos_name_prefix + "ZAS/" + eos5_obj_name;      
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

        // Here we need to remove the Unlimited dimension if the size is -1 since we currently don't support unlimited dimension.
        // TODO: need to rehandle this when adding the unlimited dimension support.
        if (eos5dim.name =="Unlimited" && eos5dim.size == -1)
            continue;

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

void build_var_dim_path(const string & eos5_obj_name, const vector<HE5Var>& var_list, unordered_map<string,
                        vector<string>>& varpath_to_dims, HE5_TYPE e5_type, bool is_geo) {

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

    // Note: in this routine, we should handle the special characters of var path because we need to remember
    //       the path for the dmrpp module to correctly open the HDF5 dataset.
    for (const auto & eos5var:var_list) {

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

// Obtain the dimension names of the variable with the path of varname.
bool obtain_eos5_dim(const string & varname, const unordered_map<string, vector<string>>& varpath_to_dims,
                     vector<string> & dimnames) {

    bool ret_value = false;
    unordered_map<string,vector<string>>::const_iterator vit = varpath_to_dims.find(varname);
    if (vit != varpath_to_dims.end()){
        for (const auto &sit:vit->second)
            dimnames.push_back(HDF5CFUtil::obtain_string_after_lastslash(sit));
        ret_value = true;
    }
    return ret_value;
} 

// Obtain the dimension names of DAP4 group with the path of grpname.
bool obtain_eos5_grp_dim(const string & grpname, const unordered_map<string, vector<HE5Dim>>& grppath_to_dims,
                         vector<string> & dimnames) {

    bool ret_value = false;
    unordered_map<string,vector<HE5Dim>>::const_iterator vit = grppath_to_dims.find(grpname);
    if (vit != grppath_to_dims.end()){
        for (const auto &sit:vit->second)
            dimnames.push_back(HDF5CFUtil::obtain_string_after_lastslash(sit.name));
        ret_value = true;
    }
    return ret_value;
}

hsize_t obtain_unlim_pure_dim_size(hid_t pid, const string &dname) {

    hsize_t ret_value = 0;

    hid_t dset_id = -1;
    if((dset_id = H5Dopen(pid,dname.c_str(),H5P_DEFAULT)) <0) {
        string msg = "cannot open the HDF5 dataset  ";
        msg += dname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    htri_t has_reference_list = -1;
    string reference_name= "REFERENCE_LIST";
    has_reference_list = H5Aexists(dset_id,reference_name.c_str());

    if (has_reference_list > 0)
        ret_value = obtain_unlim_pure_dim_size_internal(dset_id, dname, reference_name);

    if (H5Dclose(dset_id) <0) {
        string msg = "cannot close the HDF5 dataset  ";
        msg += dname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
 
    return ret_value;
}

hsize_t obtain_unlim_pure_dim_size_internal(hid_t dset_id, const string &dname, const string &reference_name) {

    hsize_t ret_value = 0;

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

    if (H5T_COMPOUND == H5Tget_class(atype_id))
        ret_value = obtain_unlim_pure_dim_size_internal_value(dset_id, attr_id, atype_id, reference_name, dname);

    H5Aclose(attr_id);
    H5Tclose(atype_id);

    return ret_value;
}

hsize_t obtain_unlim_pure_dim_size_internal_value(hid_t dset_id, hid_t attr_id, hid_t atype_id,
                                                  const string &reference_name, const string &dname) {

    typedef struct s_t {
        hobj_ref_t s_ref;
        int s_index;
    } s_t;

    hsize_t ret_value = 0;
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
    if (H5Rget_obj_type2(dset_id, H5R_OBJECT, &((ref_list[0]).s_ref),&obj_type) <0) {
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

        // Check if this is a simple 
        if (H5Sget_simple_extent_type(did_space) != H5S_SIMPLE) {
            H5Aclose(attr_id);
            H5Tclose(atype_id);
            H5Sclose(did_space);
            H5Dclose(dset_id);
            H5Dclose(did_ref);
            string msg = "The dataspace must be a simple HDF5 dataspace for the variable  " + dname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        int did_space_num_dims = H5Sget_simple_extent_ndims(did_space);
        if (did_space_num_dims <0) {
            H5Aclose(attr_id);
            H5Tclose(atype_id);
            H5Sclose(did_space);
            H5Dclose(dset_id);
            H5Dclose(did_ref);
            string msg = "The number of dimensions must be > 0 for the variable  " + dname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        vector<hsize_t> did_dims(did_space_num_dims);
        vector<hsize_t> did_max_dims(did_space_num_dims);

        if (H5Sget_simple_extent_dims(did_space,did_dims.data(),did_max_dims.data()) <0) {
            H5Aclose(attr_id);
            H5Tclose(atype_id);
            H5Sclose(did_space);
            H5Dclose(dset_id);
            H5Dclose(did_ref);
            string msg = "Cannot obtain the dimension information for the variable  " + dname;
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        
        hsize_t cur_unlimited_dim_size  = 0;

        int num_unlimited_dims = 0;
        for (unsigned i = 0; i<did_space_num_dims; i++) {
            if (did_max_dims[i] == H5S_UNLIMITED) {
                cur_unlimited_dim_size = did_dims[i];
                num_unlimited_dims++;
            }
        }
        if (num_unlimited_dims >1)
            throw InternalErr(__FILE__,__LINE__,"This variable has more than 1 unlimited dimensions. This is not supported.");
        
        ret_value = cur_unlimited_dim_size;

        H5Dclose(did_ref);
        H5Sclose(did_space);
    }

    H5Sclose(aspace_id);

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

            auto t_a = dynamic_cast<Array *>(*vi);

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
        size_t d4map_size = d4_maps->size();
        while (d4map_size != 0) {
            D4Map * d4_map = d4_maps->get_map(0);
            d4_maps->remove_map(d4_map);
            delete d4_map;
            d4map_size = d4_maps->size();
        }
    }

    // Then coordinates
    for (auto &cv_map:coname_array_maps) {

        D4Maps *d4_maps = (cv_map.second)->maps();
        size_t d4map_size = d4_maps->size();
        while (d4map_size != 0) {
            D4Map * d4_map = d4_maps->get_map(0);
            d4_maps->remove_map(d4_map);
            delete d4_map;
            d4map_size = d4_maps->size();
        }

    }

    // Reorder the variables so that the map variables are at the front.
    // We decide to use map instead of unordered_map since now the order of variables is important.
    map<string,Array*> ordered_dc_co_array_maps;
    map<string,Array*> ordered_coname_array_maps;

    // Loop through dsname_array_maps, then search coname_array_maps, if this element is not in the coname_array_maps,
    // add this to dc_co_array_maps.
    for (const auto &dsname_array_map:dsname_array_maps) {
        
        bool found_coname = false;
        for (const auto &coname_array_map:coname_array_maps) {
            if (coname_array_map.first == dsname_array_map.first) {
                found_coname = true;
                break;
            }
        }

        if (found_coname == false) 
            ordered_dc_co_array_maps.insert(dsname_array_map);
    }

    for (const auto &coname_array_map:coname_array_maps)
        ordered_coname_array_maps.insert(coname_array_map);

    reorder_vars(d4_root,ordered_coname_array_maps,ordered_dc_co_array_maps);

}

void add_dap4_coverage_default_internal(D4Group* d4_grp, unordered_map<string, Array*> &dsname_array_maps,
                                        unordered_map<string, Array*> &coname_array_maps) {


    Constructor::Vars_iter vi = d4_grp->var_begin();
    Constructor::Vars_iter ve = d4_grp->var_end();

    for (; vi != ve; vi++) {

        const BaseType *v = *vi;

        // Only Arrays can have maps.
        if (libdap::dods_array_c == v->type()) {

            auto t_a = dynamic_cast<Array *>(*vi);

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

                // For the dimensions not covered by the found coordinates attribute, check dimension scales.
                add_dimscale_maps(t_a,dsname_array_maps,handled_dim_names);
            }
            else 
                add_dimscale_maps(t_a,dsname_array_maps,handled_dim_names);

        }

    }

    // Go over children groups.
    for (D4Group::groupsIter gi = d4_grp->grp_begin(), ge = d4_grp->grp_end(); gi != ge; ++gi)
        add_dap4_coverage_default_internal(*gi, dsname_array_maps, coname_array_maps);

}

void obtain_coord_names(Array* ar, vector<string> & coord_names) {

    D4Attributes *d4_attrs = ar->attributes();
    D4Attribute *d4_attr = d4_attrs->find("coordinates");
    if (d4_attr != nullptr && d4_attr->type() == attr_str_c) {
        if (d4_attr->num_values() == 1) {
            string tempstring = d4_attr->value(0);
            char sep=' ';
            HDF5CFUtil::Split_helper(coord_names,tempstring,sep);
        }
        // From our observations, the coordinates attribute is always just one string.
        // So this else block may never be executed.
        else
             obtain_multi_string_coord_names(d4_attr, coord_names);
    }
}

void obtain_multi_string_coord_names(D4Attribute *d4_attr, vector<string> & coord_names) {

    for (D4Attribute::D4AttributeIter av_i = d4_attr->value_begin(), av_e = d4_attr->value_end(); av_i != av_e; av_i++) {
        vector <string> tempstr_vec;
        char sep=' ';
        HDF5CFUtil::Split_helper(tempstr_vec,*av_i,sep);
        for (const auto &tve:tempstr_vec)
            coord_names.push_back(tve);
    }
}
// Generate absolute path(FQN) for all coordinate variables. Note the output is the coord_names.
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

// This is for the case when the coordinates attribute is something like "lat lon", no path is provided.
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

            auto t_a = dynamic_cast<Array *>(*vi);
            if (coord_name == t_a->name()) {
                // Find the coordinate variable, But We need to return the absolute path of the variable.
                coord_name = t_a->FQN();
                found_cv = true;
                break;
            }
        }
    }

    if (found_cv == false && (d4_grp->get_parent())){
        auto d4_grp_par = dynamic_cast<D4Group*>(d4_grp->get_parent());
        found_cv = obtain_no_path_cv(d4_grp_par,coord_name);
    }
    return found_cv;
}

void handle_absolute_path_cv(const D4Group *d4_grp, string &coord_name) {

    // For the time being, we don't check if this cv with absolute path exists.
    // However, we need to check if the coordinates are under the current group or the ancestor groups.
    string d4_grp_fqn = d4_grp->FQN();
    string cv_path = HDF5CFUtil::obtain_string_before_lastslash(coord_name);
    if (d4_grp_fqn.find(cv_path) != 0) 
        coord_name="";

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
    if (pos != 0)
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

    // Now we need to find the absolute path of the coordinate variable. 
    if (find_coord)
        handle_relative_path_cvname_internal(d4_grp, coord_name, sep_count);

}

void handle_relative_path_cvname_internal(const D4Group *d4_grp, string &coord_name, unsigned short sep_count) {

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
        if (libdap::dods_array_c == v->type())
            obtain_ds_name_array_maps_internal(*vi, dsn_array_maps, handled_all_cv_names);
    }

    for (D4Group::groupsIter gi = d4_grp->grp_begin(), ge = d4_grp->grp_end(); gi != ge; ++gi) {
        BESDEBUG("h5",  "In group:  " << (*gi)->name() << endl);
        obtain_ds_name_array_maps(*gi, dsn_array_maps, handled_all_cv_names);
    }

}

void obtain_ds_name_array_maps_internal(BaseType *v, unordered_map<string,Array*>&dsn_array_maps,
                                        const vector<string>& handled_all_cv_names)
{
    auto t_a = dynamic_cast<Array *>(v);

    // Dimension scales must be 1-dimension. So we save many unnecessary operations.
    // Find the dimension scale, insert to the global unordered_map.
    if (t_a->dimensions() == 1) {
        string t_a_fqn = t_a->FQN();
        if (find(handled_all_cv_names.begin(),handled_all_cv_names.end(),t_a_fqn) != handled_all_cv_names.end())
            dsn_array_maps.emplace(t_a_fqn,t_a);
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

            auto d4_map_unique = make_unique<D4Map>(it_ma->first, it_ma->second);
            D4Map *d4_map = d4_map_unique.release();

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
    // Note the array of coname_array_maps is the map array. It is gradually built.
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
    
                auto t_a = dynamic_cast<Array *>(*vi);

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

    // Need to check the parent group and call recursively for the coordinates not found in this group.
    if (coord_names.empty() == false && d4_grp->get_parent()) {
        auto d4_grp_par = dynamic_cast<D4Group*>(d4_grp->get_parent());
        add_coord_maps(d4_grp_par,var,coord_names,coname_array_maps,handled_dim_names);
    }
}


// Add the valid coordinate variables(via dimension scales) to the var's DAP4 maps.
// Loop through the handled dimension names(by the coordinate variables) set
// Then check this var's dimensions. Only add this var's unhandled coordinates(dimension scales) if these dimension scales exist. 
void add_dimscale_maps(libdap::Array* var, std::unordered_map<std::string,libdap::Array*> & dc_array_maps,
                       const std::unordered_set<std::string> & handled_dim_names) {

    BESDEBUG("h5","Coming to add_dimscale_maps() "<<endl);

    Array::Dim_iter di = var->dim_begin();
    Array::Dim_iter de = var->dim_end();

    for (; di != de; di++) {

        const D4Dimension * d4_dim = var->dimension_D4dim(di);

        // DAP4 dimension may not exist, so need to check.
        if (d4_dim) {

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
 
void reorder_vars(D4Group *d4_grp, const map<string,Array*> &coname_array_maps,
                  const map<string,Array*> & dc_array_maps) {

    Constructor::Vars_iter vi = d4_grp->var_begin();
    Constructor::Vars_iter ve = d4_grp->var_end();

    vector<int> cv_pos;
    vector<BaseType *> cv_obj_ptr;

    int v_index = 0;
    for (; vi != ve; vi++) {

        BaseType *v = *vi;

        // We only need to re-order arrays. 
        if (libdap::dods_array_c == v->type()) {

            // We need to remember the coordinate variable positions and remember the corresponding arrays.
            for (const auto &coname_array_map:coname_array_maps) {
                if (coname_array_map.first == v->FQN()) {
                    cv_pos.push_back(v_index);
                    cv_obj_ptr.push_back(v);
                }
            }
            for (const auto &dc_array_map:dc_array_maps) {
                if (dc_array_map.first == v->FQN()) {
                    cv_pos.push_back(v_index);
                    cv_obj_ptr.push_back(v);
                }
            }
        }
        v_index++;
    }

// Leave the following #if 0 block for debugging.
#if 0
for (const auto &cv_p:cv_pos) 
cerr<< ": "<<cv_p <<endl;

for (const auto &cv_obj_p:cv_obj_ptr) 
cerr<< "name: "<<cv_obj_p->FQN() <<endl;
#endif

    // Obtain the front variables. The number of front variables is set to be the same as the coordinate variables
    // and dimension scales.
    // We also need to remember the positions since it is possible that these front variables contain
    // the coordinate/dimension variables.
    // We will not move those coordinate/dimension variables in the front.
    vector<BaseType *>front_v_ptr;
    auto stop_index = (int)(cv_pos.size());

    // If we do have coordinate/dimension variables,find those variables and re-order.
    // (We cannot assume that we always have these variables).
    if (stop_index > 0)
        reorder_vars_internal(d4_grp, cv_pos, cv_obj_ptr, stop_index);

    // Now go the children groups. Because the coordinate variables is always on the ancestor groups or this group,
    // the re-ordering routine guarantees that a map variable is in front of all other variables that use this map.
    for (D4Group::groupsIter gi = d4_grp->grp_begin(), ge = d4_grp->grp_end(); gi != ge; ++gi)
        reorder_vars(*gi, coname_array_maps, dc_array_maps);

}

void reorder_vars_internal(D4Group* d4_grp, const vector<int> &cv_pos, const vector<BaseType *>& cv_obj_ptr, int stop_index) {
    Constructor::Vars_iter vi = d4_grp->var_begin();
    Constructor::Vars_iter ve = d4_grp->var_end();
    vi = d4_grp->var_begin();
    ve = d4_grp->var_end();

    int v_index = 0;
    vector<BaseType *> front_v_ptr;

    for (; vi != ve; vi++) {
        BaseType *v = *vi;
        front_v_ptr.push_back(v);
        v_index++;
        if (v_index == stop_index)
            break;
    }

    // Check the overlaps of cvs with the front variables.
    // For example, if there are 3 coordinate variables, c1,c2,c3; the first 3 variables
    // are v1,v2,c1. In this case, c1 doesn't need to move. We only switch v1 with c2 and v2 with c3.
    // Usually there are only a few coordinate variables. So even the nested loops are not costly.

    // First, obtain the overlapped cv and front v positions.
    vector<int> overlap_cv_pos;
    vector<int> overlap_front_pos;
    for (int i = 0; i < stop_index; i++) {
        for (int j = 0; j < stop_index; j++) {
            if (i == cv_pos[j]) {
                overlap_cv_pos.push_back(cv_pos[j]);
                overlap_front_pos.push_back(i);
                break;
            }
        }
    }

    // No need to move a cv if this cv is in the front(overlapped with the first few vars) already.
    // Now we need to find the cv and front variables that need to move.

    // The cvs that need to be moved.
    vector<int> mov_cv_pos;
    vector<BaseType *> mov_cv_ptr;
    for (int i = 0; i < stop_index; i++) {
        bool overlapped_cv = false;
        for (const auto &overlap_cv_p: overlap_cv_pos) {
            if (cv_pos[i] == overlap_cv_p) {
                overlapped_cv = true;
                break;
            }
        }
        if (overlapped_cv == false) {
            mov_cv_pos.push_back(cv_pos[i]);
            mov_cv_ptr.push_back(cv_obj_ptr[i]);
        }
    }

    // The front non-cv variables that need to be moved.
    vector<int> mov_front_pos;
    vector<BaseType *> mov_front_v_ptr;
    for (int i = 0; i < stop_index; i++) {
        bool overlapped_front_cv = false;
        for (const auto &overlap_front_p: overlap_front_pos) {
            if (i == overlap_front_p) {
                overlapped_front_cv = true;
                break;
            }
        }
        if (overlapped_front_cv == false) {
            mov_front_pos.push_back(i);
            mov_front_v_ptr.push_back(front_v_ptr[i]);
        }
    }

    reorder_vars_internal_final_phase(d4_grp, mov_cv_pos, mov_front_pos, mov_front_v_ptr, mov_cv_ptr);

}

void reorder_vars_internal_final_phase(D4Group* d4_grp, const vector<int> &mov_cv_pos,
                                       const vector<int> &mov_front_pos, const vector<BaseType *> &mov_front_v_ptr,
                                       const vector<BaseType *> &mov_cv_ptr) {

    // sanity check
    if (mov_cv_pos.size() != mov_front_pos.size()) {
        string err_msg = "The number of moved coordinate variables is not the same as ";
        err_msg += "the number of moved non-coordinate variables";
        throw InternalErr(__FILE__, __LINE__, err_msg);
    }

// Leave the following #if 0 for the time being. This is for debugging purpose. KY 2023-04-13
#if 0
        for (int i = 0; i <mov_cv_pos.size();i++) {
        cerr<<"mov_front_pos: "<<mov_front_pos[i] <<endl;
        cerr<<"mov_cv_pos: "<<mov_cv_pos[i] <<endl;
        }
#endif

    // Move the map variables to the front, move the front non-coordinate variables to the original map variable location.
    for (unsigned int i = 0; i < mov_front_pos.size(); i++) {
        d4_grp->set_var_index(mov_cv_ptr[i], mov_front_pos[i]);
        d4_grp->set_var_index(mov_front_v_ptr[i], mov_cv_pos[i]);
    }

}

#if 0
bool is_cvar(const BaseType *v, const unordered_map<string,Array*> &coname_array_maps, const unordered_map<string,Array*> & dc_array_maps) {

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
#endif

void add_possible_eos5_grid_vars(D4Group* d4_grp, eos5_dim_info_t &eos5_dim_info) {

    BESDEBUG("h5","coming to add_possible_eos5_grid_vars"<<endl);

    eos5_grid_info_t eg_info;

#if 0
for (const auto & ed_info:eos5_dim_info.gridname_to_info) {
    cerr<<"grid name: "<<ed_info.first <<endl;
    cerr<<" projection: "<<ed_info.second.projection <<endl;
    cerr<<"           "<<"xdim fqn:" << ed_info.second.xdim_fqn <<endl;
    cerr<<"           "<<"ydim fqn:" << ed_info.second.ydim_fqn <<endl;
    cerr<<"           "<<"xdim size:" << ed_info.second.xdim_size <<endl;
    cerr<<"           "<<"ydim size:" << ed_info.second.ydim_size <<endl;
    cerr<<"           "<<"xdim point_lower:" << ed_info.second.point_lower <<endl;
    cerr<<"           "<<"xdim point_upper:" << ed_info.second.point_upper <<endl;
    cerr<<"           "<<"xdim point_left:" << ed_info.second.point_lower <<endl;
    cerr<<"           "<<"xdim point_right:" << ed_info.second.point_right <<endl;

}

for (const auto & d_v_info:eos5_dim_info.dimpath_to_cvpath) {
    cerr<<" dimension name 1" <<d_v_info.first.dpath0 <<endl;
    cerr<<" dimension name 2" <<d_v_info.first.dpath1 <<endl;
    cerr<<" cv name 1" <<d_v_info.second.vpath0 <<endl;
    cerr<<" cv name 2" <<d_v_info.second.vpath1 <<endl;
    cerr<<" cv name 3" <<d_v_info.second.cf_gmap_path <<endl;


}
#endif

    bool add_grid_var = is_eos5_grid_grp(d4_grp,eos5_dim_info,eg_info);

    if (add_grid_var && eg_info.projection == HE5_GCTP_GEO)
        add_eos5_grid_vars_geo(d4_grp, eg_info);
    else if (add_grid_var && (eg_info.projection == HE5_GCTP_SNSOID ||
                            eg_info.projection == HE5_GCTP_PS ||
                           eg_info.projection == HE5_GCTP_LAMAZ))
        add_eos5_grid_vars_non_geo(d4_grp, eos5_dim_info, eg_info);

}

void add_eos5_grid_vars_geo(D4Group* d4_grp, const eos5_grid_info_t & eg_info) {

    BaseType *ar_bt_lat = nullptr;
    BaseType *ar_bt_lon = nullptr;
    HDF5MissLLArray *ar_lat = nullptr;
    HDF5MissLLArray *ar_lon = nullptr;

    try {

        auto ar_bt_lat_unique = make_unique<Float32>("YDim");
        ar_bt_lat = ar_bt_lat_unique.get();
        auto ar_lat_unique = make_unique<HDF5MissLLArray>(true,
                                                          1,
                                                          eg_info,
                                                          "YDim",
                                                          ar_bt_lat);
        ar_lat = ar_lat_unique.release();

        string ydimpath = d4_grp->FQN() + "YDim";
        ar_lat->append_dim_ll(eg_info.ydim_size, ydimpath);

        auto d4_dim0_unique = make_unique<D4Dimension>("YDim", eg_info.ydim_size);
        auto d4_dim0 = d4_dim0_unique.release();
        (ar_lat->dim_begin())->dim = d4_dim0;

        // The DAP4 group needs also to store these dimensions.
        D4Dimensions *dims = d4_grp->dims();
        dims->add_dim_nocopy(d4_dim0);

        auto ar_bt_lon_unique = make_unique<Float32>("XDim");

        ar_bt_lon = ar_bt_lon_unique.get();
        auto ar_lon_unique = make_unique<HDF5MissLLArray>(
                false,
                1,
                eg_info,
                "XDim",
                ar_bt_lon);
        ar_lon = ar_lon_unique.release();

        string xdimpath = d4_grp->FQN() + "XDim";
        ar_lon->append_dim_ll(eg_info.xdim_size, xdimpath);

        auto d4_dim1_unique = make_unique<D4Dimension>("XDim", eg_info.xdim_size);
        auto d4_dim1 = d4_dim1_unique.release();

        (ar_lon->dim_begin())->dim = d4_dim1;

        // The DAP4 group needs also to store these dimensions.
        dims = d4_grp->dims();
        dims->add_dim_nocopy(d4_dim1);

        // Set this variable to DAP4 is critical for DAP4 dimensions and attributes handling.
        ar_lat->set_is_dap4(true);
        ar_lon->set_is_dap4(true);

        // Add the CF units attribute to ar_lat and ar_lon.
        add_var_dap4_attr(ar_lat, "units", attr_str_c, "degrees_north");
        add_var_dap4_attr(ar_lon, "units", attr_str_c, "degrees_east");
        d4_grp->add_var_nocopy(ar_lat);
        d4_grp->add_var_nocopy(ar_lon);
    }
    catch (...) {
        delete ar_lat;
        delete ar_lon;
        throw InternalErr(__FILE__, __LINE__, "Unable to allocate the HDFMissLLArray instance.");
    }
}

void add_eos5_grid_vars_non_geo(D4Group* d4_grp, eos5_dim_info_t &eos5_dim_info,  const eos5_grid_info_t & eg_info) {

    HDF5CFProj *dummy_proj_cf = nullptr;
    BaseType *ar_bt_dim0 = nullptr;
    BaseType *ar_bt_dim1 = nullptr;
    HDF5CFProj1D *ar_dim0 = nullptr;
    HDF5CFProj1D *ar_dim1 = nullptr;

    BaseType *ar_bt_lat = nullptr;
    BaseType *ar_bt_lon = nullptr;
    HDF5MissLLArray *ar_lat = nullptr;
    HDF5MissLLArray *ar_lon = nullptr;

    try {
        string dummy_proj_cf_name = "eos5_cf_projection";
        auto dummy_proj_cf_unique = make_unique<HDF5CFProj>(dummy_proj_cf_name, dummy_proj_cf_name);
        dummy_proj_cf = dummy_proj_cf_unique.release();
        dummy_proj_cf->set_is_dap4(true);

        if (eg_info.projection == HE5_GCTP_SNSOID) {

            add_var_dap4_attr(dummy_proj_cf, "grid_mapping_name", attr_str_c, "sinusoidal");
            add_var_dap4_attr(dummy_proj_cf, "longitude_of_central_meridian", attr_float64_c, "0.0");
            add_var_dap4_attr(dummy_proj_cf, "earth_radius", attr_float64_c, "6371007.181");
            add_var_dap4_attr(dummy_proj_cf, "_CoordinateAxisTypes", attr_str_c, "GeoX GeoY");
        } else if (eg_info.projection == HE5_GCTP_PS)
            add_ps_cf_grid_mapping_attrs(dummy_proj_cf, eg_info);
        else if (eg_info.projection == HE5_GCTP_LAMAZ)
            add_lamaz_cf_grid_mapping_attrs(dummy_proj_cf, eg_info);

        d4_grp->add_var_nocopy(dummy_proj_cf);

        auto ar_bt_dim1_unique = make_unique<Float64>("XDim");
        ar_bt_dim1 = ar_bt_dim1_unique.get();

        auto ar_dim1_unique = make_unique<HDF5CFProj1D>(eg_info.point_left, eg_info.point_right,
                                                        eg_info.xdim_size, "XDim", ar_bt_dim1);
        ar_dim1 = ar_dim1_unique.release();

        // Handle dimensions
        string xdimpath = d4_grp->FQN() + "XDim";
        ar_dim1->append_dim_ll(eg_info.xdim_size, xdimpath);

        // Need to add DAP4 dimensions
        auto d4_dim1_unique = make_unique<D4Dimension>("XDim", eg_info.xdim_size);
        auto d4_dim1 = d4_dim1_unique.release();

        (ar_dim1->dim_begin())->dim = d4_dim1;

        // The DAP4 group needs also to store these dimensions.
        D4Dimensions *dims = d4_grp->dims();
        dims->add_dim_nocopy(d4_dim1);

        auto ar_bt_dim0_unique = make_unique<Float64>("YDim");
        ar_bt_dim0 = ar_bt_dim0_unique.get();

        auto ar_dim0_unique = make_unique<HDF5CFProj1D>
                (eg_info.point_upper, eg_info.point_lower, eg_info.ydim_size, "XDim", ar_bt_dim0);
        ar_dim0 = ar_dim0_unique.release();

        string ydimpath = d4_grp->FQN() + "YDim";
        ar_dim0->append_dim_ll(eg_info.ydim_size, ydimpath);

        // Need to add DAP4 dimensions
        auto d4_dim0_unique = make_unique<D4Dimension>("YDim", eg_info.ydim_size);
        auto d4_dim0 = d4_dim0_unique.release();

        (ar_dim0->dim_begin())->dim = d4_dim0;

        // The DAP4 group needs also to store these dimensions.
        dims = d4_grp->dims();
        dims->add_dim_nocopy(d4_dim0);

        ar_dim1->set_is_dap4(true);
        ar_dim0->set_is_dap4(true);

        add_gm_spcvs_attrs(ar_dim0, true);
        add_gm_spcvs_attrs(ar_dim1, false);

        d4_grp->add_var_nocopy(ar_dim1);
        d4_grp->add_var_nocopy(ar_dim0);

        auto ar_bt_lat_unique = make_unique<Float64>("Latitude");
        ar_bt_lat = ar_bt_lat_unique.get();

        auto ar_lat_unique = make_unique<HDF5MissLLArray>(true, 2, eg_info, "Latitude", ar_bt_lat);
        ar_lat = ar_lat_unique.release();
        ar_lat->append_dim_ll(eg_info.ydim_size, ydimpath);
        ar_lat->append_dim_ll(eg_info.xdim_size, xdimpath);

        // Need to add DAP4 dimensions for this 2-D var.
        Array::Dim_iter d = ar_lat->dim_begin();
        d->dim = d4_dim0;
        d++;
        d->dim = d4_dim1;
        add_var_dap4_attr(ar_lat, "units", attr_str_c, "degrees_north");

        auto ar_bt_lon_unique = make_unique<Float64>("Longitude");
        ar_bt_lon = ar_bt_lon_unique.get();

        auto ar_lon_unique = make_unique<HDF5MissLLArray>(false,
                                                          2,
                                                          eg_info,
                                                          "Longitude",
                                                          ar_bt_lon);
        ar_lon = ar_lon_unique.release();
        ar_lon->append_dim_ll(eg_info.ydim_size, ydimpath);
        ar_lon->append_dim_ll(eg_info.xdim_size, xdimpath);

        add_var_dap4_attr(ar_lon, "units", attr_str_c, "degrees_east");

        d = ar_lon->dim_begin();
        d->dim = d4_dim0;
        d++;
        d->dim = d4_dim1;

        ar_lat->set_is_dap4(true);
        ar_lon->set_is_dap4(true);

        d4_grp->add_var_nocopy(ar_lat);
        d4_grp->add_var_nocopy(ar_lon);

        // Now we need to add eos5 grid mapping, dimension and coordinate info to eos5_dim_info.
        eos5_dname_info_t edname_info;
        eos5_cname_info_t ecname_info;
        edname_info.dpath0 = ydimpath;
        edname_info.dpath1 = xdimpath;
        ecname_info.vpath0 = d4_grp->FQN() + "Latitude";
        ecname_info.vpath1 = d4_grp->FQN() + "Longitude";
        ecname_info.cf_gmap_path = d4_grp->FQN() + dummy_proj_cf_name;

        pair<eos5_dname_info_t, eos5_cname_info_t> t_pair;
        t_pair = make_pair(edname_info, ecname_info);
        eos5_dim_info.dimpath_to_cvpath.push_back(t_pair);

#if 0
        for (const auto & d_v_info:eos5_dim_info.dimpath_to_cvpath) {
            cerr<<" dimension name 1: " <<d_v_info.first.dpath0 <<endl;
            cerr<<" dimension name 2: " <<d_v_info.first.dpath1 <<endl;
            cerr<<" cv name 1: " <<d_v_info.second.vpath0 <<endl;
            cerr<<" cv name 2: " <<d_v_info.second.vpath1 <<endl;
            cerr<<" dummy cf projection var name: " <<d_v_info.second.cf_gmap_path <<endl;
        }
#endif

    }
    catch (...) {
        delete dummy_proj_cf;
        delete ar_dim0;
        delete ar_dim1;
        delete ar_lat;
        delete ar_lon;
        throw InternalErr(__FILE__, __LINE__, "Unable to allocate the HDFMissLLArray instance.");
    }
}

bool is_eos5_grid_grp(D4Group *d4_group,const eos5_dim_info_t &eos5_dim_info, eos5_grid_info_t &eg_info) {

    bool ret_value = false;
    string grp_fqn = d4_group->FQN();

    for (const auto & ed_info:eos5_dim_info.gridname_to_info) {
        string eos_mod_path = handle_string_special_characters_in_path(ed_info.first);
        if (grp_fqn == (eos_mod_path + "/")) {
            eg_info = ed_info.second;
            ret_value = true;
            break;
        }
    }

    if (ret_value == true)
        ret_value = no_eos5_grid_vars_in_grp(d4_group, eg_info);

    return ret_value;
}

bool no_eos5_grid_vars_in_grp(D4Group *d4_group, const eos5_grid_info_t &eg_info) {

    bool ret_value = true;

    // Even if we find the correct eos group, we need to ensure that the variables we want to add
    // do not exist. That is: we need to check the variable names like Latitude/Longitude etc. don't exist in
    // this group. This seems unnecessary, but we do observe that data producers may add CF variables by themselves.
    // The handler needs to ensure that it will keep using the variables added by the data producers first.

    Constructor::Vars_iter vi = d4_group->var_begin();
    Constructor::Vars_iter ve = d4_group->var_end();

    for (; vi != ve; vi++) {

        const BaseType *v = *vi;
        string vname = v->name();
        if (eg_info.projection == HE5_GCTP_GEO) {
            if (vname == "YDim" || vname == "XDim") {
                ret_value = false;
                break;
            }

        }
        else {
            if (vname == "YDim" || vname == "XDim" || vname =="Latitude" || vname == "Longitude"
                || vname == "eos5_cf_projection") {
                ret_value = false;
                break;
            }
        }
    }
    return ret_value;
}

void build_gd_info(const HE5Grid &gd,unordered_map<string,eos5_grid_info_t>& gridname_to_info) {

    string grid_name = "/HDFEOS/GRIDS/"+gd.name;
    eos5_grid_info_t eg_info;
    eg_info.xdim_fqn = grid_name+"/XDim";
    eg_info.ydim_fqn = grid_name+"/YDim";

    bool find_xdim = false;
    bool find_ydim = false;

    // I have to use the grid dimension name and size information since the struct metadata
    // doesn't contain the dimension size information of a variable. It has to be deduced from
    // the grid dimension information. Note: application can choose their own dimension names in
    // the grid. This is a flaw in the HDF-EOS5 library since the variable's dimension names 
    // are always XDim or YDim. So far I only see one variation in the NASA products. Use Xdim rather than Ydim.
    // An error will be generated if neither XDim/Xdim nor YDim/Ydim is found.

    for (const auto &dim:gd.dim_list) {

        if ((dim.name == "XDim" || dim.name == "Xdim") && find_xdim == false) {
            eg_info.xdim_size = dim.size;
            find_xdim = true;
        }
        else if ((dim.name == "YDim" || dim.name == "Ydim") && find_ydim == false) {
            eg_info.ydim_size = dim.size;
            find_ydim = true;
        }
        
        if (find_xdim == true && find_ydim == true)
            break;

    }

    if (find_xdim == true && find_ydim == true) {

        eg_info.point_lower = gd.point_lower;
        eg_info.point_upper = gd.point_upper;
        eg_info.point_left = gd.point_left;
        eg_info.point_right = gd.point_right;
        eg_info.pixelregistration = gd.pixelregistration;
        eg_info.gridorigin = gd.gridorigin;
        eg_info.projection = gd.projection;

        for (int i = 0; i <13;i++)
            eg_info.param[i] = gd.param[i];

        eg_info.zone = gd.zone;
        eg_info.sphere = gd.sphere;
        gridname_to_info[grid_name] = eg_info;       

    }

    else {
       
        string dimname_list;
        for (const auto &dim:gd.dim_list) {
            dimname_list += dim.name;
            dimname_list += " ";
        }
        string err_msg = "This HDF-EOS5 grid dimension list doesn't contain XDim, Xdim, YDim or Ydim.";
        err_msg +=  " The dimension names of this grid are: "+dimname_list;
        throw InternalErr(__FILE__,__LINE__,err_msg);

    }

}

void add_ps_cf_grid_mapping_attrs(libdap::BaseType *var, const eos5_grid_info_t & eg_info) {

    // The following information is added according to the HDF-EOS5 user's guide and
    // CF 1.7 grid_mapping requirement.

    // Longitude down below pole of map
    double vert_lon_pole =  HE5_EHconvAng(eg_info.param[4],HE5_HDFE_DMS_DEG);

    // Latitude of true scale
    double lat_true_scale = HE5_EHconvAng(eg_info.param[5],HE5_HDFE_DMS_DEG);

    // False easting
    double fe = eg_info.param[6];

    // False northing
    double fn = eg_info.param[7];

    add_var_dap4_attr(var,"grid_mapping_name",attr_str_c,"polar_stereographic");

    ostringstream s_vert_lon_pole;
    s_vert_lon_pole << vert_lon_pole;

    // I did this map is based on my best understanding. KY
    // CF: straight_vertical_longitude_from_pole
    add_var_dap4_attr(var,"straight_vertical_longitude_from_pole", attr_float64_c, s_vert_lon_pole.str());

    ostringstream s_lat_true_scale;
    s_lat_true_scale << lat_true_scale;
    add_var_dap4_attr(var,"standard_parallel", attr_float64_c, s_lat_true_scale.str());

    if(fe == 0.0)
        add_var_dap4_attr(var,"false_easting",attr_float64_c,"0.0");
    else {
        ostringstream s_fe;
        s_fe << fe;
        add_var_dap4_attr(var,"false_easting",attr_float64_c,s_fe.str());
    }

    if(fn == 0.0)
        add_var_dap4_attr(var,"false_northing",attr_float64_c,"0.0");
    else {
        ostringstream s_fn;
        s_fn << fn;
        add_var_dap4_attr(var,"false_northing",attr_float64_c,s_fn.str());
    }

    if(lat_true_scale >0)
        add_var_dap4_attr(var,"latitude_of_projection_origin",attr_float64_c,"+90.0");
    else
        add_var_dap4_attr(var, "latitude_of_projection_origin",attr_float64_c,"-90.0");

    add_var_dap4_attr(var, "_CoordinateAxisTypes", attr_str_c, "GeoX GeoY");

    // From CF, PS has another parameter,
    // Either standard_parallel (EPSG 9829) or scale_factor_at_projection_origin (EPSG 9810)
    // I cannot find the corresponding parameter from the EOS5.

}

void add_lamaz_cf_grid_mapping_attrs(libdap::BaseType *var, const eos5_grid_info_t & eg_info) {

    double lon_proj_origin = HE5_EHconvAng(eg_info.param[4],HE5_HDFE_DMS_DEG);
    double lat_proj_origin = HE5_EHconvAng(eg_info.param[5],HE5_HDFE_DMS_DEG);
    double fe = eg_info.param[6];
    double fn = eg_info.param[7];

    add_var_dap4_attr(var,"grid_mapping_name", attr_str_c, "lambert_azimuthal_equal_area");

    ostringstream s_lon_proj_origin;
    s_lon_proj_origin << lon_proj_origin;
    add_var_dap4_attr(var,"longitude_of_projection_origin", attr_float64_c, s_lon_proj_origin.str());

    ostringstream s_lat_proj_origin;
    s_lat_proj_origin << lat_proj_origin;

    add_var_dap4_attr(var,"latitude_of_projection_origin", attr_float64_c, s_lat_proj_origin.str());

    if(fe == 0.0)
        add_var_dap4_attr(var,"false_easting",attr_float64_c,"0.0");
    else {
        ostringstream s_fe;
        s_fe << fe;
        add_var_dap4_attr(var,"false_easting",attr_float64_c,s_fe.str());
    }

    if(fn == 0.0)
        add_var_dap4_attr(var,"false_northing",attr_float64_c,"0.0");
    else {
        ostringstream s_fn;
        s_fn << fn;
        add_var_dap4_attr(var,"false_northing",attr_float64_c,s_fn.str());
    }

    add_var_dap4_attr(var,"_CoordinateAxisTypes", attr_str_c, "GeoX GeoY");
}

// coordinates and grid_mapping attributes may be added to this HDF-EOS5 var
void add_possible_var_cv_info(libdap::BaseType *var, const eos5_dim_info_t &eos5_dim_info) {

    bool have_cv_dim0 = false;
    bool have_cv_dim1 = false;
    string dim0_cv_name1;
    string dim0_cv_name2;
    string dim0_gm_name;
    string dim1_cv_name1;
    string dim1_cv_name2;
    string dim1_gm_name;

    auto t_a = dynamic_cast<Array*>(var);

    Array::Dim_iter di = t_a->dim_begin();
    Array::Dim_iter de = t_a->dim_end();

    for (; di != de; di++) {

        const D4Dimension * d4_dim = t_a->dimension_D4dim(di);

        // DAP4 dimension may not exist, so need to check.
        if(d4_dim) { 

            // Fully Qualified name(absolute path) of the dimension
            // Both "XDim" and "YDim" in the EOS grid should appear in this var.
            string dim_fqn = d4_dim->fully_qualified_name();

            for (const auto &dim_to_cv:eos5_dim_info.dimpath_to_cvpath) {
                if (dim_fqn == dim_to_cv.first.dpath0) {

                    dim0_cv_name1 = dim_to_cv.second.vpath0;
                    dim0_cv_name2 = dim_to_cv.second.vpath1;
                    dim0_gm_name = dim_to_cv.second.cf_gmap_path;

                    have_cv_dim0 = true;
                }
                else if (dim_fqn == dim_to_cv.first.dpath1) {

                    dim1_cv_name1 = dim_to_cv.second.vpath0;
                    dim1_cv_name2 = dim_to_cv.second.vpath1;
                    dim1_gm_name = dim_to_cv.second.cf_gmap_path;
 
                    have_cv_dim1 = true;
                }

                if (have_cv_dim0 && have_cv_dim1) 
                    break;
            }
        }

        if (have_cv_dim0 && have_cv_dim1) 
            break;
    }

    // We know the dimension names in each grid are different. 
    // We can have only one set of match in each grid.
    if (have_cv_dim0 && have_cv_dim1) {
        if (dim0_cv_name1 != dim1_cv_name1 || dim0_cv_name2 !=dim1_cv_name2 || dim0_gm_name !=dim1_gm_name) 
            throw InternalErr(__FILE__,__LINE__,"Inconsistent coordinates for this EOS5 Grid");
        else {// Add the CV attributes.
            string coord_value = dim0_cv_name1 + " "+dim0_cv_name2;
            add_var_dap4_attr(var,"coordinates",attr_str_c,coord_value);
            add_var_dap4_attr(var,"grid_mapping",attr_str_c,dim0_gm_name);
        }
    }

}

void make_attributes_to_cf(BaseType *var, const eos5_dim_info_t &eos5_dim_info) {

    bool check_attr = false;
    for (const auto & ed_info:eos5_dim_info.gridname_to_info) { 
        if (ed_info.second.projection == HE5_GCTP_GEO) {
            check_attr = true;
            break;
        }
    }

    if (check_attr == true) {

        D4Attributes *d4_attrs = var->attributes();
        bool have_scale_factor = false;
        bool have_add_offset = false;
        for (auto ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end(); ii != ee; ++ii) {
            if ((*ii)->name() == "ScaleFactor") { 
                (*ii)->set_name("scale_factor");
                have_scale_factor = true;
            }
            else  if ((*ii)->name() == "Offset") { 
                (*ii)->set_name("add_offset");
                have_add_offset = true;
            }
            if (have_scale_factor && have_add_offset)
                break;
        }
    }
}

void handle_vlen_int_float(D4Group *d4_grp, hid_t pid, const string &vname, const string &var_path,
                           const string &filename, hid_t dset_id) {
cerr<<"coming to handle_vlen_int_float"<<endl;

    hid_t vlen_type = H5Dget_type(dset_id);
    hid_t vlen_basetype = H5Tget_super(vlen_type);
    if (H5Tget_class(vlen_basetype) != H5T_INTEGER && H5Tget_class(vlen_basetype) != H5T_FLOAT) 
        throw InternalErr(__FILE__, __LINE__,"Only support float or intger variable-length datatype.");

    hid_t vlen_base_memtype = H5Tget_native_type(vlen_basetype, H5T_DIR_ASCEND);
    hid_t vlen_memtype = H5Tvlen_create(vlen_base_memtype);

    // Will not support the scalar type. Why just create a 1-D array?
    hid_t vlen_space = H5Dget_space(dset_id);
    if (H5Sget_simple_extent_type(vlen_space) != H5S_SIMPLE)
        throw InternalErr(__FILE__, __LINE__,"Only support array of float or intger variable-length datatype.");

    int vlen_ndims = H5Sget_simple_extent_ndims(vlen_space);
    hssize_t vlen_number_elements = H5Sget_simple_extent_npoints(vlen_space);
    vector<hvl_t> vlen_data(vlen_number_elements);
    H5Dread(dset_id, vlen_memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, vlen_data.data());
    
    size_t max_vlen_length = 0;
    for (ssize_t i = 0; i<vlen_number_elements; i++) {
        if (max_vlen_length<vlen_data[i].len)   
            max_vlen_length = vlen_data[i].len;
    }

    H5Dvlen_reclaim(vlen_memtype, vlen_space, H5P_DEFAULT, (void*)(vlen_data.data()));
    H5Sclose(vlen_space);
    
    // Now we need to create two variables: one to store vlen data, another to store vlen index.
    // First: vlen data.
    BaseType *bt = Get_bt_enhanced(d4_grp,pid, vname,var_path, filename, vlen_basetype);
    if (!bt)
        throw InternalErr(__FILE__, __LINE__,"Unable to convert hdf5 datatype to dods basetype");

    H5Tclose(vlen_base_memtype);
    H5Tclose(vlen_basetype);
    H5Tclose(vlen_type);
    H5Tclose(vlen_memtype);
 
    // Just see if it can work out.
        auto ar_unique = make_unique<HDF5Array>(vname, filename, bt);
        HDF5Array *ar = ar_unique.release();

        // set number of elements and variable name values.
        // This essentially stores in the struct.
        ar->set_varpath(vname);
    string vlen_length_dimname = vname + "_vlen";
    ar->append_dim_ll(max_vlen_length);
    //ar->append_dim_ll(max_vlen_length, vlen_length_dimname);
  
#if 0
    for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
        if (dt_inst.dimnames[dim_index].empty() == false)
            ar->append_dim_ll(dt_inst.size[dim_index], dt_inst.dimnames[dim_index]);
        else
            ar->append_dim_ll(dt_inst.size[dim_index]);
    }
    
    string vlen_length_dimname = vname + "_vlen";
    ar->append_dim_ll(max_vlen_length, vlen_length_dimname);
    dt_inst.dimnames.clear();

        // We need to transform dimension info. to DAP4 group
        BaseType *new_var = nullptr;
     vector<string> temp_dimnames_path = dt_inst.dimnames_path;
     temp_dimnames_path.push_back(vlen_length_dimname);
            new_var = ar->h5dims_transform_to_dap4(d4_grp, dt_inst.dimnames_path);

        // clear DAP4 dimnames_path vector
        dt_inst.dimnames_path.clear();

        read_objects_basetype_attr_hl(var_path, new_var, dset_id,  false);

        d4_grp->add_var_nocopy(new_var);

#endif
        d4_grp->add_var_nocopy(ar);


}
        
