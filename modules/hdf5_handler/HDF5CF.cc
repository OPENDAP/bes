// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5CF.cc
/// \brief Implementation of the basic mapping of HDF5 to DAP by following CF
///
///  It includes basic functions to 
///  1) retrieve HDF5 metadata info.
///  2) translate HDF5 objects into DAP DDS and DAS by following CF conventions.
///
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2023 The HDF Group
///
/// All rights reserved.

#include <memory>
#include <sstream>
#include <algorithm>
#include <climits>
#include "HDF5CF.h"
#include "h5cfdaputil.h"
#include "HDF5RequestHandler.h"
#include "h5apicompatible.h"
#include "BESDebug.h"

using namespace HDF5CF;

Var::Var(const Var *var):
    newname(var->newname),
    name(var->name),
    fullpath(var->fullpath),
    dtype(var->dtype),
    rank(var->rank),
    comp_ratio(var->comp_ratio),
    total_elems(var->total_elems),
    zero_storage_size(var->zero_storage_size),
    unsupported_attr_dtype(var->unsupported_attr_dtype),
    unsupported_attr_dspace(var->unsupported_attr_dspace),
    unsupported_dspace(var->unsupported_dspace),
    dimnameflag(var->dimnameflag),
    coord_attr_add_path(var->coord_attr_add_path)
{

#if 0
    newname = var->newname;
    name = var->name;
    fullpath = var->fullpath;
    rank = var->rank;
    total_elems = var->total_elems;
    zero_storage_size = var->zero_storage_size;
    dtype = var->dtype;
    comp_ratio = var->comp_ratio;
    unsupported_attr_dtype = var->unsupported_attr_dtype;
    unsupported_attr_dspace = var->unsupported_attr_dspace;
    unsupported_dspace = var->unsupported_dspace;
    unsupported_attr_dspace = var->unsupported_attr_dspace;
    dimnameflag = var->dimnameflag;
    coord_attr_add_path = var->coord_attr_add_path;
#endif

    for (const auto &vattr:var->attrs) {
        auto attr_unique = make_unique<Attribute>();
        auto attr = attr_unique.release();
        attr->name = vattr->name;
        attr->newname = vattr->newname;
        attr->dtype = vattr->dtype;
        attr->count = vattr->count;
        attr->strsize = vattr->strsize;
        attr->fstrsize = vattr->fstrsize;
        attr->value = vattr->value;
        attrs.push_back(attr);
    }

    for (const auto &vdim:var->dims) {
        auto dim_unique = make_unique<Dimension>(vdim->size);
        auto dim = dim_unique.release();
        dim->name = vdim->name;
        dim->newname = vdim->newname;
        dim->unlimited_dim = vdim->unlimited_dim;
        dims.push_back(dim);
    }

}

bool CVar::isLatLon() const
{

    bool ret_value = false;
    if (CV_EXIST == this->cvartype || CV_MODIFY == this->cvartype || CV_SPECIAL == this->cvartype) {
        string attr_name = "units";
        string lat_unit_value = "degrees_north";
        string lon_unit_value = "degrees_east";

        for (const auto &attr:this->attrs) {

            if ((H5FSTRING == attr->getType()) || (H5VSTRING == attr->getType())) {
                if (attr_name == attr->newname) {
                    string attr_value1(attr->getValue().begin(), attr->getValue().end());

                    if (attr->getCount() == 1) {
                        string attr_value(attr->getValue().begin(), attr->getValue().end());
                        if (attr_value.compare(0, lat_unit_value.size(), lat_unit_value) == 0) {
                            if (attr_value.size() == lat_unit_value.size()) {
                                ret_value = true;
                                break;
                            }
                            else if (attr_value.size() == (lat_unit_value.size() + 1)) {
                                if (attr_value[attr_value.size() - 1] == '\0'
                                    || attr_value[attr_value.size() - 1] == ' ') {
                                    ret_value = true;
                                    break;
                                }
                            }
                        }
                        else if (attr_value.compare(0, lon_unit_value.size(), lon_unit_value) == 0) {
                            if (attr_value.size() == lon_unit_value.size()) {
                                ret_value = true;
                                break;
                            }
                            else if (attr_value.size() == (lon_unit_value.size() + 1)) {
                                if (attr_value[attr_value.size() - 1] == '\0'
                                    || attr_value[attr_value.size() - 1] == ' ') {
                                    ret_value = true;
                                    break;
                                }
                            }

                        }

                    }
                }
            }
        }
    }
    else if (this->cvartype == CV_LAT_MISS || this->cvartype == CV_LON_MISS) ret_value = true;
    return ret_value;

}
File::~File()
{

    if (this->fileid >= 0) {
        if (this->rootid >= 0) {
            for_each(this->groups.begin(), this->groups.end(), delete_elem());
            for_each(this->vars.begin(), this->vars.end(), delete_elem());
            for_each(this->root_attrs.begin(), this->root_attrs.end(), delete_elem());
            H5Gclose(rootid);
        }
    }
}

Group::~Group()
{
    for_each(this->attrs.begin(), this->attrs.end(), delete_elem());
}

Var::~Var()
{
    for_each(this->dims.begin(), this->dims.end(), delete_elem());
    for_each(this->attrs.begin(), this->attrs.end(), delete_elem());
}


void File::Retrieve_H5_Info(const char * /*path*/, hid_t file_id, bool include_attr)
{

    BESDEBUG("h5", "coming to Retrieve_H5_Info" <<endl);

    if (true == include_attr) {

        // Obtain the BES key to check the ignored objects
        // We will only use DAS to output these information.
        this->check_ignored = HDF5RequestHandler::get_check_ignore_obj();
        if (true == this->check_ignored) this->add_ignored_info_page_header();

    }

    hid_t root_id;
    if ((root_id = H5Gopen(file_id, "/", H5P_DEFAULT)) < 0) {
        throw1("Cannot open the HDF5 root group ");
    }
    this->rootid = root_id;
    try {
        this->Retrieve_H5_Obj(root_id, "/", include_attr);
    }
    catch (...) {
        throw;
    }

    // Obtain attributes only necessary
    if (true == include_attr) {

        // Find the file(root group) attribute

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;
        int num_attrs = 0;

        if (H5OGET_INFO(root_id, &oinfo) < 0)
            throw1("Error obtaining the info for the root group");

        num_attrs = (int)(oinfo.num_attrs);
        bool temp_unsup_attr_atype = false;
        bool temp_unsup_attr_dspace = false;

        for (int j = 0; j < num_attrs; j++) {
            auto attr_unique = make_unique<Attribute>();
            auto attr = attr_unique.release();
            try {
                this->Retrieve_H5_Attr_Info(attr, root_id, j, temp_unsup_attr_atype, temp_unsup_attr_dspace);
            }
            catch (...) {
                delete attr;
                throw;

            }
            this->root_attrs.push_back(attr);
        }

        this->unsupported_attr_dtype = temp_unsup_attr_atype;
        this->unsupported_attr_dspace = temp_unsup_attr_dspace;
    }
}

void File::Retrieve_H5_Obj(hid_t grp_id, const char*gname, bool include_attr) 
{

    // Iterate through the file to see the members of the group from the root.
    H5G_info_t g_info;
    hsize_t nelems = 0;

    if (H5Gget_info(grp_id, &g_info) < 0)
    throw2("Counting hdf5 group elements error for ", gname);
    nelems = g_info.nlinks;

    ssize_t oname_size = 0;
    for (hsize_t i = 0; i < nelems; i++) {

        hid_t cgroup = -1;
        hid_t cdset = -1;
        Group *group = nullptr;
        Var *var = nullptr;
        Attribute *attr = nullptr;

        try {

            size_t dummy_name_len = 1;

            // Query the length of object name.
            oname_size = H5Lget_name_by_idx(grp_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, nullptr, dummy_name_len,
                H5P_DEFAULT);
            if (oname_size <= 0)
            throw2("Error getting the size of the hdf5 object from the group: ", gname);

            // Obtain the name of the object 
            vector<char> oname;
            oname.resize((size_t) oname_size + 1);

            if (H5Lget_name_by_idx(grp_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, oname.data(), (size_t) (oname_size + 1),
                H5P_DEFAULT) < 0)
            throw2("Error getting the hdf5 object name from the group: ", gname);

            // Check if it is a hard link or a soft link
            H5L_info_t linfo;
            if (H5Lget_info(grp_id, oname.data(), &linfo, H5P_DEFAULT) < 0)
            throw2("HDF5 link name error from ", gname);

            // We ignore soft links and external links for the CF options 
            if (H5L_TYPE_SOFT == linfo.type || H5L_TYPE_EXTERNAL == linfo.type) {
                if (true == include_attr && true == check_ignored) {
                    this->add_ignored_info_links_header();
                    string full_path_name;
                    string temp_oname(oname.begin(), oname.end());
                    full_path_name = (
                        (string(gname) != "/") ?
                            (string(gname) + "/" + temp_oname.substr(0, temp_oname.size() - 1)) :
                            ("/" + temp_oname.substr(0, temp_oname.size() - 1)));
                    this->add_ignored_info_links(full_path_name);

                }
                continue;
            }

            // Obtain the object type, such as group or dataset. 
            H5O_info_t oinfo;

            if (H5OGET_INFO_BY_IDX(grp_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, &oinfo, H5P_DEFAULT) < 0)
            throw2("Error obtaining the info for the object ", string(oname.begin(), oname.end()));

            H5O_type_t obj_type = oinfo.type;

            switch (obj_type) {

            case H5O_TYPE_GROUP: {

                // Obtain the full path name
                string full_path_name;
                string temp_oname(oname.begin(), oname.end());

                full_path_name = (
                    (string(gname) != "/") ?
                        (string(gname) + "/" + temp_oname.substr(0, temp_oname.size() - 1)) :
                        ("/" + temp_oname.substr(0, temp_oname.size() - 1)));

                cgroup = H5Gopen(grp_id, full_path_name.c_str(), H5P_DEFAULT);
                if (cgroup < 0)
                    throw2("Error opening the group ", full_path_name);

                auto group_unique = make_unique<Group>();
                group = group_unique.release();
                group->path = full_path_name;
                group->newname = full_path_name;

                // Retrieve group attribute if the attribute flag is true
                if (true == include_attr) {

                    auto num_attrs = (int)(oinfo.num_attrs);
                    bool temp_unsup_attr_dtype = false;
                    bool temp_unsup_attr_dspace = false;

                    for (int j = 0; j < num_attrs; j++) {

                        auto attr_unique = make_unique<Attribute>();
                        attr = attr_unique.release();
                        Retrieve_H5_Attr_Info(attr, cgroup, j, temp_unsup_attr_dtype, temp_unsup_attr_dspace);
                        group->attrs.push_back(attr);
                        attr = nullptr;
                    }

                    group->unsupported_attr_dtype = temp_unsup_attr_dtype;
                    group->unsupported_attr_dspace = temp_unsup_attr_dspace;
                }
                this->groups.push_back(group);
                Retrieve_H5_Obj(cgroup, full_path_name.c_str(), include_attr);
                if (H5Gclose(cgroup) < 0)
                throw2("Error closing the group ", full_path_name);
            }
                break;
            case H5O_TYPE_DATASET: {

                // Obtain the absolute path of the HDF5 dataset
                string temp_oname(oname.begin(), oname.end());
                string full_path_name = (
                    (string(gname) != "/") ?
                        (string(gname) + "/" + temp_oname.substr(0, temp_oname.size() - 1)) :
                        ("/" + temp_oname.substr(0, temp_oname.size() - 1)));

                auto var_unique = make_unique<Var>();
                var = var_unique.release();
                var->name = temp_oname.substr(0, temp_oname.size() - 1);
                var->fullpath = full_path_name;

                // newname is for the final CF name
                var->newname = full_path_name;

                cdset = H5Dopen(grp_id, full_path_name.c_str(), H5P_DEFAULT);
                if (cdset < 0)
                    throw2("Error opening the HDF5 dataset ", full_path_name);

                // Retrieve the HDF5 dataset datatype, return the flag for unsupported types.
                bool temp_unsup_var_dtype = false;
                Retrieve_H5_VarType(var, cdset, full_path_name, temp_unsup_var_dtype);

                // Update the unsupported datatype flag
                if (!this->unsupported_var_dtype && temp_unsup_var_dtype) this->unsupported_var_dtype = true;

                // Retrieve the HDF5 dataset data space, return the flag for unsupported dataspaces.
                bool temp_unsup_var_dspace = false;
                Retrieve_H5_VarDim(var, cdset, full_path_name, temp_unsup_var_dspace);

                // Update the unsupported data space flag
                if (!this->unsupported_var_dspace && temp_unsup_var_dspace) this->unsupported_var_dspace = true;

                hsize_t d_storage_size = H5Dget_storage_size(cdset);
                var->zero_storage_size =(d_storage_size ==0);
                var->comp_ratio = Retrieve_H5_VarCompRatio(var, cdset);

                // Retrieve the attribute info. if asked
                if (true == include_attr) {

                    auto num_attrs = (int)(oinfo.num_attrs);
                    bool temp_unsup_attr_dtype = false;
                    bool temp_unsup_attr_dspace = false;

                    for (int j = 0; j < num_attrs; j++) {

                        auto attr_unique = make_unique<Attribute>();
                        attr = attr_unique.release();

                        Retrieve_H5_Attr_Info(attr, cdset, j, temp_unsup_attr_dtype, temp_unsup_attr_dspace);
                        var->attrs.push_back(attr);
                        attr = nullptr;
                    }

                    var->unsupported_attr_dtype = temp_unsup_attr_dtype;
                    var->unsupported_attr_dspace = temp_unsup_attr_dspace;

                    if (!this->unsupported_var_attr_dspace && temp_unsup_attr_dspace)
                        this->unsupported_var_attr_dspace = true;
                }

                this->vars.push_back(var);
                if (H5Dclose(cdset) < 0)
                throw2("Error closing the HDF5 dataset ", full_path_name);
            }
                break;

            case H5O_TYPE_NAMED_DATATYPE: {
                // ignore the named datatype
                if (true == include_attr && true == check_ignored) {
                    this->add_ignored_info_namedtypes(string(gname), string(oname.begin(), oname.end()));
                }
            }
                break;
            default:
                break;
            } // "switch (obj_type)"
        } // try
        catch (...) {

            if (attr != nullptr) {
                delete attr;
                attr = nullptr;
            }

            if (var != nullptr) {
                delete var;
                var = nullptr;
            }

            if (group != nullptr) {
                delete group;
                group = nullptr;
            }

            if (cgroup != -1) H5Gclose(cgroup);

            if (cdset != -1) H5Dclose(cdset);
            throw;

        } // catch
    } // "for (hsize_t i = 0; i < nelems; i++)"

}

// Retrieve HDF5 dataset compression ratio
float File::Retrieve_H5_VarCompRatio(const Var *var, const hid_t dset_id) const 
{

    float comp_ratio = 1.0;
    // Obtain the data type of the variable. 
    hid_t dset_create_plist = H5Dget_create_plist(dset_id);
    if (dset_create_plist < 0)
    throw1("unable to obtain hdf5 dataset creation property list ");
    H5D_layout_t dset_layout = H5Pget_layout(dset_create_plist);
    if (dset_layout < 0) {
        H5Pclose(dset_create_plist);
        throw1("unable to obtain hdf5 dataset creation property list storage layout");
    }

    if (dset_layout == H5D_CHUNKED) {

        hsize_t dstorage_size = H5Dget_storage_size(dset_id);
        if (dstorage_size > 0 && var->total_elems > 0) {
            hid_t ty_id = -1;

            // Obtain the data type of the variable.
            if ((ty_id = H5Dget_type(dset_id)) < 0)
            throw1("unable to obtain hdf5 datatype for the dataset ");
            size_t type_size = H5Tget_size(ty_id);
            comp_ratio = ((float)((var->total_elems) * type_size))/(float)dstorage_size;
            H5Tclose(ty_id);
        }

    }
    H5Pclose(dset_create_plist);
    return comp_ratio;

}
// Retrieve HDF5 dataset datatype
void File::Retrieve_H5_VarType(Var *var, hid_t dset_id, const string & varname, bool &unsup_var_dtype) const 
{

    hid_t ty_id = -1;

    // Obtain the data type of the variable. 
    if ((ty_id = H5Dget_type(dset_id)) < 0)
        throw2("unable to obtain hdf5 datatype for the dataset ", varname);

    // The following datatype class and datatype will not be supported for the CF option.
    //   H5T_TIME,  H5T_BITFIELD
    //   H5T_OPAQUE,  H5T_ENUM
    //   H5T_REFERENCE, H5T_COMPOUND
    //   H5T_VLEN,H5T_ARRAY
    //  64-bit integer

    // Note: H5T_REFERENCE H5T_COMPOUND and H5T_ARRAY can be mapped to DAP2 DDS for the default option.
    // H5T_COMPOUND, H5T_ARRAY can be mapped to DAP2 DAS for the default option.
    // 1-D variable length of string can also be mapped for the CF option.
    // The variable length string class is H5T_STRING rather than H5T_VLEN,
    // We also ignore the mapping of integer 64 bit since DAP2 doesn't
    // support 64-bit integer. In theory, DAP2 doesn't support long double
    // (128-bit or 92-bit floating point type).
    // 

    var->dtype = HDF5CFUtil::H5type_to_H5DAPtype(ty_id);
    if (false == HDF5CFUtil::cf_strict_support_type(var->dtype,_is_dap4)) 
        unsup_var_dtype = true;

    if (H5Tclose(ty_id) < 0)
        throw1("Unable to close the HDF5 datatype ");
}

// Retrieve the HDF5 dataset dimension information
void File::Retrieve_H5_VarDim(Var *var, hid_t dset_id, const string & varname, bool &unsup_var_dspace) 
{

    vector<hsize_t> dsize;
    vector<hsize_t> maxsize;

    hid_t dspace_id = -1;
    hid_t ty_id = -1;

    try {
        if ((dspace_id = H5Dget_space(dset_id)) < 0)
        throw2("Cannot get hdf5 dataspace id for the variable ", varname);

        H5S_class_t space_class = H5S_NO_CLASS;
        if ((space_class = H5Sget_simple_extent_type(dspace_id)) < 0)
        throw2("Cannot obtain the HDF5 dataspace class for the variable ", varname);

        if (H5S_NULL == space_class)
            unsup_var_dspace = true;
        else {
            if (false == unsup_var_dspace) {

                hssize_t h5_total_elms = H5Sget_simple_extent_npoints(dspace_id);
                if (h5_total_elms < 0)
                    throw2("Cannot get the total number of elements of HDF5 dataset ", varname);
                else
                    var->total_elems = (size_t) h5_total_elms;
                int ndims = H5Sget_simple_extent_ndims(dspace_id);
                if (ndims < 0)
                    throw2("Cannot get the hdf5 dataspace number of dimension for the variable ", varname);

                var->rank = ndims;
                if (ndims != 0) {
                    dsize.resize(ndims);
                    maxsize.resize(ndims);
                }

                // The netcdf DAP client supports the representation of the unlimited dimension. 
                // So we need to check.
                if (H5Sget_simple_extent_dims(dspace_id, dsize.data(), maxsize.data()) < 0)
                    throw2("Cannot obtain the dim. info for the variable ", varname);

                for (int i = 0; i < ndims; i++) {
                    auto dim_unique = make_unique<Dimension>(dsize[i]);
                    auto dim = dim_unique.release();
                    if (maxsize[i] == H5S_UNLIMITED) {
                        dim->unlimited_dim = true;
                        if (false == have_udim) 
                            have_udim = true;
                    }
                    var->dims.push_back(dim);
                }
            }
        }

        var->unsupported_dspace = unsup_var_dspace;

        if (H5Sclose(dspace_id) < 0)
            throw1("Cannot close the HDF5 dataspace .");

    }

    catch (...) {

        if (dspace_id != -1) H5Sclose(dspace_id);

        if (ty_id != -1) H5Tclose(ty_id);
        throw;
    }

}

// Retrieve the HDF5 attribute information.
void File::Retrieve_H5_Attr_Info(Attribute * attr, hid_t obj_id, const int j, bool &unsup_attr_dtype,
    bool &unsup_attr_dspace) const

{

    hid_t attrid = -1;
    hid_t ty_id = -1;
    hid_t aspace_id = -1;
    hid_t memtype = -1;

    try {

        // Obtain the attribute ID.
        if ((attrid = H5Aopen_by_idx(obj_id, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, (hsize_t) j, H5P_DEFAULT,
            H5P_DEFAULT)) < 0)
            throw1("Unable to open attribute by index ");

        // Obtain the size of attribute name.
        ssize_t name_size = H5Aget_name(attrid, 0, nullptr);
        if (name_size < 0)
            throw1("Unable to obtain the size of the hdf5 attribute name  ");

        string attr_name;
        attr_name.resize(name_size + 1);

        // Obtain the attribute name.    
        if ((H5Aget_name(attrid, name_size + 1, &attr_name[0])) < 0)
            throw1("unable to obtain the hdf5 attribute name  ");

        // Obtain the type of the attribute. 
        if ((ty_id = H5Aget_type(attrid)) < 0)
        throw2("unable to obtain hdf5 datatype for the attribute ", attr_name);

        // The following datatype class and datatype will not be supported for the CF option.
        //   H5T_TIME,  H5T_BITFIELD
        //   H5T_OPAQUE,  H5T_ENUM
        //   H5T_REFERENCE, H5T_COMPOUND
        //   H5T_VLEN,H5T_ARRAY
        //  64-bit integer

        // Note: H5T_REFERENCE H5T_COMPOUND and H5T_ARRAY can be mapped to DAP2 DDS for the default option.
        // H5T_COMPOUND, H5T_ARRAY can be mapped to DAP2 DAS for the default option.
        // 1-D variable length of string can also be mapped for the CF option.
        // The variable length string class is H5T_STRING rather than H5T_VLEN,
        // We also ignore the mapping of integer 64 bit since DAP2 doesn't
        // support 64-bit integer. In theory, DAP2 doesn't support long double
        // (128-bit or 92-bit floating point type).
        // 
        attr->dtype = HDF5CFUtil::H5type_to_H5DAPtype(ty_id);
        if (false == HDF5CFUtil::cf_strict_support_type(attr->dtype,_is_dap4)) 
            unsup_attr_dtype = true;

        if(H5VSTRING == attr->dtype || H5FSTRING == attr->dtype) {
            H5T_cset_t c_set_type = H5Tget_cset(ty_id);
            if(c_set_type <0) 
                throw2("Cannot get hdf5 character set type for the attribute ", attr_name);
            // This is a UTF-8 string
            if(c_set_type == H5T_CSET_UTF8)
                attr->is_cset_ascii = false;
        }

        if ((aspace_id = H5Aget_space(attrid)) < 0)
            throw2("Cannot get hdf5 dataspace id for the attribute ", attr_name);

        int ndims = H5Sget_simple_extent_ndims(aspace_id);
        if (ndims < 0)
            throw2("Cannot get the hdf5 dataspace number of dimension for attribute ", attr_name);

        hsize_t nelmts = 1;

        // if it is a scalar attribute, just define number of elements to be 1.
        if (ndims != 0) {

            vector<hsize_t> asize;
            vector<hsize_t> maxsize;
            asize.resize(ndims);
            maxsize.resize(ndims);

            // Obtain the attribute data space information.
            if (H5Sget_simple_extent_dims(aspace_id, asize.data(), maxsize.data()) < 0)
            throw2("Cannot obtain the dim. info for the attribute ", attr_name);

            // Here we need to take care of 0-length attribute. This is legal in HDF5.
            for (int dim_count = 0;dim_count < ndims; dim_count ++) {
                // STOP adding unsupported_attr_dspace!
                if (asize[dim_count] == 0) {
                    unsup_attr_dspace = true;
                    break;
                }
            }

            if (false == unsup_attr_dspace) {
                // Return ndims and size[ndims]. 
                for (int dim_count = 0; dim_count< ndims; dim_count++)
                    nelmts *= asize[dim_count];
            }
            else
                nelmts = 0;
        } // "if(ndims != 0)"

        size_t ty_size = H5Tget_size(ty_id);
        if (0 == ty_size)
            throw2("Cannot obtain the dtype size for the attribute ", attr_name);

        memtype = H5Tget_native_type(ty_id, H5T_DIR_ASCEND);
        if (memtype < 0)
            throw2("Cannot obtain the memory datatype for the attribute ", attr_name);

        // Store the name and the count
        string temp_aname(attr_name.begin(), attr_name.end());
        attr->name = temp_aname.substr(0, temp_aname.size() - 1);
        attr->newname = attr->name;
        attr->count = nelmts;

        // Release HDF5 resources.
        if (H5Tclose(ty_id) < 0)
            throw1("Cannot successfully close the attribute datatype.");
        if (H5Tclose(memtype) < 0)
            throw1("Cannot successfully close the attribute memory datatype.");
        if (H5Sclose(aspace_id) < 0)
            throw1("Cannot successfully close the HDF5 dataspace.");
        if (H5Aclose(attrid) < 0)
            throw1("Cannot successfully close the HDF5 attribute.");

    } // try
    catch (...) {

        if (ty_id != -1) H5Tclose(ty_id);

        if (memtype != -1) H5Tclose(memtype);

        if (aspace_id != -1) H5Sclose(aspace_id);

        if (attrid != -1) H5Aclose(attrid);

        throw;
    }

}

// Retrieve all HDF5 supported attribute values.
void File::Retrieve_H5_Supported_Attr_Values() 
{

    for (auto &root_attr:this->root_attrs)
        Retrieve_H5_Attr_Value(root_attr, "/");

    for (const auto &grp:this->groups) {
        for (auto &grp_attr:grp->attrs) 
            Retrieve_H5_Attr_Value(grp_attr, grp->path);
    }

    for (const auto &var:this->vars) {
        for (auto &var_attr:var->attrs) {
            Retrieve_H5_Attr_Value(var_attr, var->fullpath);
        }
    }
}

void File::Retrieve_H5_Var_Attr_Values(Var *var) 
{
    for (auto &attr:var->attrs) 
        Retrieve_H5_Attr_Value(attr, var->fullpath);
}

// Retrieve the values of a specific HDF5 attribute.
void File::Retrieve_H5_Attr_Value(Attribute *attr, const string & obj_name) const
{

    // Define HDF5 object Ids.
    hid_t obj_id = -1;
    hid_t attr_id = -1;
    hid_t ty_id = -1;
    hid_t memtype_id = -1;
    hid_t aspace_id = -1;

    try {

        // Open the object that hold this attribute
        obj_id = H5Oopen(this->fileid, obj_name.c_str(), H5P_DEFAULT);
        if (obj_id < 0)
            throw2("Cannot open the object ", obj_name);

        attr_id = H5Aopen(obj_id, (attr->name).c_str(), H5P_DEFAULT);
        if (attr_id < 0)
            throw4("Cannot open the attribute ", attr->name, " of object ", obj_name);

        ty_id = H5Aget_type(attr_id);
        if (ty_id < 0)
            throw4("Cannot obtain the datatype of  the attribute ", attr->name, " of object ", obj_name);

        memtype_id = H5Tget_native_type(ty_id, H5T_DIR_ASCEND);
        if (memtype_id < 0)
            throw2("Cannot obtain the memory datatype for the attribute ", attr->name);

        size_t ty_size = H5Tget_size(memtype_id);
        if (0 == ty_size)
            throw4("Cannot obtain the dtype size for the attribute ", attr->name, " of object ", obj_name);

        size_t total_bytes = attr->count * ty_size;

        // We have to handle variable length string differently. 
        if (H5VSTRING == attr->dtype) {

            // Variable length string attribute values only store pointers of the actual string value.
            vector<char> temp_buf;
            temp_buf.resize(total_bytes);

            if (H5Aread(attr_id, memtype_id, temp_buf.data()) < 0)
                throw4("Cannot obtain the value of the attribute ", attr->name, " of object ", obj_name);

            char *temp_bp = nullptr;
            const char *ptr_1stvlen_ptr = temp_buf.data();
            temp_bp = temp_buf.data();
            const char* onestring = nullptr;
            string total_vstring;

            attr->strsize.resize(attr->count);

            for (unsigned int temp_i = 0; temp_i < attr->count; temp_i++) {

                // This line will assure that we get the real variable length string value.
                onestring = *(char **) temp_bp;
                if (onestring != nullptr) {
                    total_vstring += string(onestring);
                    attr->strsize[temp_i] = (string(onestring)).size();
                }
                else
                    attr->strsize[temp_i] = 0;

                // going to the next value.
                temp_bp += ty_size;
            }

            if (ptr_1stvlen_ptr != nullptr) {
                aspace_id = H5Aget_space(attr_id);
                if (aspace_id < 0)
                    throw4("Cannot obtain space id for ", attr->name, " of object ", obj_name);

                // Reclaim any VL memory if necessary.
                if (H5Dvlen_reclaim(memtype_id, aspace_id, H5P_DEFAULT, temp_buf.data()) < 0)
                    throw4("Cannot reclaim VL memory for ", attr->name, " of object ", obj_name);

                H5Sclose(aspace_id);
            }

            if (HDF5CFUtil::H5type_to_H5DAPtype(ty_id) != H5VSTRING)
            throw4("Error to obtain the VL string type for attribute ", attr->name, " of object ", obj_name);

            attr->value.resize(total_vstring.size());

            copy(total_vstring.begin(), total_vstring.end(), attr->value.begin());

        }
        else {

            if (attr->dtype == H5FSTRING) {
                attr->fstrsize = ty_size;
            }

            attr->value.resize(total_bytes);

            // Read HDF5 attribute data.
            if (H5Aread(attr_id, memtype_id, (void *) &attr->value[0]) < 0)
            throw4("Cannot obtain the dtype size for the attribute ", attr->name, " of object ", obj_name);

            if (attr->dtype == H5FSTRING) {

                size_t sect_size = ty_size;
                int num_sect = 1;
                if (sect_size > 0)
                    num_sect =
                        (total_bytes % sect_size == 0) ? (total_bytes / sect_size) : (total_bytes / sect_size + 1);
                else
                    throw4("The attribute datatype size is not a positive integer ", attr->name, " of object ",
                        obj_name);

                vector<size_t> sect_newsize;
                sect_newsize.resize(num_sect);

                auto total_fstring = string(attr->value.begin(), attr->value.end());

                string new_total_fstring = HDF5CFUtil::trim_string(memtype_id, total_fstring, num_sect, sect_size,
                    sect_newsize);
                attr->value.resize(new_total_fstring.size());
                copy(new_total_fstring.begin(), new_total_fstring.end(), attr->value.begin());
                attr->strsize.resize(num_sect);
                for (int temp_i = 0; temp_i < num_sect; temp_i++)
                    attr->strsize[temp_i] = sect_newsize[temp_i];

#if 0
                // "h5","new string value " <<string(attr->value.begin(), attr->value.end()) <<endl;
                cerr<<"Attr name is "<<attr->name <<endl;
                for (int temp_i = 0; temp_i <num_sect; temp_i ++)
                cerr<<"string new section size = " << attr->strsize[temp_i] <<endl;
#endif
            }
        }

        if (H5Tclose(memtype_id) < 0)
            throw1("Fail to close the HDF5 memory datatype ID.");
        if (H5Tclose(ty_id) < 0)
            throw1("Fail to close the HDF5 datatype ID.");
        if (H5Aclose(attr_id) < 0)
            throw1("Fail to close the HDF5 attribute ID.");
        if (H5Oclose(obj_id) < 0)
            throw1("Fail to close the HDF5 object ID.");

    }

    catch (...) {

        if (memtype_id != -1) H5Tclose(memtype_id);

        if (ty_id != -1) H5Tclose(ty_id);

        if (aspace_id != -1) H5Sclose(aspace_id);

        if (attr_id != -1) H5Aclose(attr_id);

        if (obj_id != -1) H5Oclose(obj_id);

        throw;
    }

}

// Handle the unsupported datatype
void File::Handle_Unsupported_Dtype(bool include_attr) 
{

    if (true == include_attr) {
        Handle_Group_Unsupported_Dtype();
        Handle_VarAttr_Unsupported_Dtype();
    }

    Handle_Var_Unsupported_Dtype();
}

//Leave this code here.
#if 0
// First the root attributes
if (true == include_attr) {
    if (true == this->unsupported_attr_dtype) {
        for (vector<Attribute *>::iterator ira = this->root_attrs.begin();
            ira != this->root_attrs.end(); ) {
            H5DataType temp_dtype = (*ira)->getType();
            if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                delete (*ira);
                ira = this->root_attrs.erase(ira);

            }
            else {
                ++ira;
            }
        }
    }
}

// Then the group attributes
if (false == this->groups.empty()) {
    for (vector<Group *>::iterator irg = this->groups.begin();
        irg != this->groups.end(); ++irg) {
        if (false == (*irg)->attrs.empty()) {
            if (true == (*irg)->unsupported_attr_dtype) {
                for (vector<Attribute *>::iterator ira = (*irg)->attrs.begin();
                    ira != (*irg)->attrs.end(); ) {
                    H5DataType temp_dtype = (*ira)->getType();
                    if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                        delete (*ira);
                        ira = (*irg)->attrs.erase(ira);
                    }
                    else {
                        ++ira;
                    }
                }
            }
        }
    }
}
}

 // Then the variable(HDF5 dataset) and the correponding attributes.
if (false == this->vars.empty()) {
if (true == include_attr) {
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end();++irv ) {
        if (false == (*irv)->attrs.empty()) {
            if (true == (*irv)->unsupported_attr_dtype) {
                for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end(); ) {
                    H5DataType temp_dtype = (*ira)->getType();
                    if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                        delete (*ira);
                        ira = (*irv)->attrs.erase(ira);
                        //ira--;
                    }
                    else {
                        ++ira;
                    }
                }
            }
        }
    }
}
if (true == this->unsupported_var_dtype) {
    // "h5","having unsupported variable datatype" <<endl;
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ) {
        H5DataType temp_dtype = (*irv)->getType();
        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
            delete (*irv);
            irv = this->vars.erase(irv);
            //irv--;
        }
        else
        ++irv;
    }
}
}
#endif

void File::Handle_Group_Unsupported_Dtype() 
{

    // First root
    if (false == this->root_attrs.empty()) {
        if (true == this->unsupported_attr_dtype) {
            for (auto ira = this->root_attrs.begin(); ira != this->root_attrs.end();) {
                H5DataType temp_dtype = (*ira)->getType();
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4)) {
                    delete (*ira);
                    ira = this->root_attrs.erase(ira);
                }
                else {
                    ++ira;
                }
            }
        }
    }

    // Then the group attributes
    if (false == this->groups.empty()) {
        for (const auto &grp:this->groups) {
            if (false == grp->attrs.empty()) {
                if (true == grp->unsupported_attr_dtype) {
                    for (auto ira = grp->attrs.begin(); ira != grp->attrs.end();) {
                        H5DataType temp_dtype = (*ira)->getType();
                        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4)) {
                            delete (*ira);
                            ira = grp->attrs.erase(ira);
                        }
                        else {
                            ++ira;
                        }
                    }
                }
            }
        }
    }
}

// Generate group unsupported datatype Information, this is for the BES ignored object key
void File::Gen_Group_Unsupported_Dtype_Info() 
{

    // First root
    if (false == this->root_attrs.empty()) {
        //if (true == this->unsupported_attr_dtype) {
            for (const auto &root_attr:this->root_attrs) {
                H5DataType temp_dtype = root_attr->getType();
                // TODO: Don't know why we still include 64-bit integer here.
                //if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype) || temp_dtype == H5INT64 || temp_dtype == H5UINT64) {
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4) 
                    || temp_dtype == H5INT64 || temp_dtype == H5UINT64) {
                    this->add_ignored_info_attrs(true, "/", root_attr->name);
                }
            }
        //}
    }

    // Then the group attributes
    if (false == this->groups.empty()) {
        for (const auto &grp:this->groups) {
            if (false == grp->attrs.empty()) {
                //if (true == grp->unsupported_attr_dtype) {
                    for (const auto &grp_attr:grp->attrs) {
                        H5DataType temp_dtype = grp_attr->getType();
                        // TODO: Don't know why we still include 64-bit integer here.
                        //if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype) || temp_dtype == H5INT64 || temp_dtype==H5UINT64 ) {
                        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4) 
                            || temp_dtype == H5INT64 || temp_dtype==H5UINT64 ) {
                            this->add_ignored_info_attrs(true, grp->path, grp_attr->name);
                        }
                    }
                //}
            }
        }
    }
}

// Handler unsupported variable datatype
void File::Handle_Var_Unsupported_Dtype() 
{
    if (false == this->vars.empty()) {
        if (true == this->unsupported_var_dtype) {
            for (auto irv = this->vars.begin(); irv != this->vars.end();) {
                H5DataType temp_dtype = (*irv)->getType();
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4)) {
                    delete (*irv);
                    irv = this->vars.erase(irv);
                }
                else {
                    ++irv;

                }
            }
        }
    }
}

// Generate unsupported variable type info. This is for the ignored objects.
void File::Gen_Var_Unsupported_Dtype_Info() 
{

    if (false == this->vars.empty()) {
        //if (true == this->unsupported_var_dtype) {
            for (const auto &var:this->vars) {
                H5DataType temp_dtype = var->getType();
                //TODO: don't know why 64-bit integer is still listed here.
                //if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)||(H5INT64 == temp_dtype) ||(H5UINT64 == temp_dtype)) {
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4)
                    ||(H5INT64 == temp_dtype) ||(H5UINT64 == temp_dtype)) {
                    this->add_ignored_info_objs(false, var->fullpath);
                }
            }
        //}
    }

}

// Handling unsupported datatypes for variable(HDF5 dataset) and the corresponding attributes.
void File::Handle_VarAttr_Unsupported_Dtype() 
{
    if (false == this->vars.empty()) {
        for (const auto &var:this->vars) {
            if (false == var->attrs.empty()) {
                if (true == var->unsupported_attr_dtype) {
                    for (auto ira = var->attrs.begin(); ira != var->attrs.end();) {
                        H5DataType temp_dtype = (*ira)->getType();
                        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4)) {
                            delete (*ira);
                            ira = var->attrs.erase(ira);
                        }
                        else {
                            ++ira;
                        }
                    }
                }
            }
        }
    }
}

// Generated unsupported var/attribute unsupported datatype Info when the BES ignored object key is on.
void File::Gen_VarAttr_Unsupported_Dtype_Info() 
{

    if (false == this->vars.empty()) {
        for (const auto &var:this->vars) {
            if (false == var->attrs.empty()) {
                //if (true == var->unsupported_attr_dtype) {
                    for (const auto &attr:var->attrs) {
                        H5DataType temp_dtype = attr->getType();
                        // TODO: check why 64-bit integer is still listed here.
                        //if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype) || (temp_dtype==H5INT64) || (temp_dtype == H5UINT64)) {
                        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4) 
                            || (temp_dtype==H5INT64) || (temp_dtype == H5UINT64)) {
                            this->add_ignored_info_attrs(false, var->fullpath, attr->name);
                        }
                    }
                //}
            }
        }
    }
}

// Generated unsupported datatype information for HDF5 dimension scales. 
// The datatypes of HDF5 dimension scales("DIMENSION_LIST" and "REFERENCE_LIST")
// are not supported. However, the information
// are retrieved by the handlers, so we don't want to report them as ignored objects.
void File::Gen_DimScale_VarAttr_Unsupported_Dtype_Info() 
{

    for (const auto &var:this->vars) {

        // If the attribute REFERENCE_LIST comes with the attribut CLASS, the
        // attribute REFERENCE_LIST is okay to ignore. No need to report.
        bool is_ignored = ignored_dimscale_ref_list(var);
        if (false == var->attrs.empty()) {
            //if (true == var->unsupported_attr_dtype) {
                for (const auto &attr:var->attrs) {
                    H5DataType temp_dtype = attr->getType();
                    // TODO: check why 64-bit is still listed here.
                    if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4) 
                        || (temp_dtype == H5INT64) || (temp_dtype == H5UINT64)) {
                        // "DIMENSION_LIST" is okay to ignore and "REFERENCE_LIST"
                        // is okay to ignore if the variable has another attribute
                        // CLASS="DIMENSION_SCALE"
                        if (("DIMENSION_LIST" != attr->name)
                            && ("REFERENCE_LIST" != attr->name || true == is_ignored))
                            this->add_ignored_info_attrs(false, var->fullpath, attr->name);
                    }
                }
            //}
        }
    }
}

// Handle unsupported dataspace for group attributes.
void File::Handle_GroupAttr_Unsupported_Dspace() 
{

    // First root
    if (false == this->root_attrs.empty()) {
        if (true == this->unsupported_attr_dspace) {
            for (auto ira = this->root_attrs.begin(); ira != this->root_attrs.end();) {
                // Remove 0-size attribute
                if ((*ira)->count == 0) {
                    delete (*ira);
                    ira = this->root_attrs.erase(ira);
                }
                else {
                    ++ira;
                }
            }
        }
    }

    // Then the group attributes
    if (false == this->groups.empty()) {
        for (auto irg = this->groups.begin(); irg != this->groups.end(); ++irg) {
            if (false == (*irg)->attrs.empty()) {
                if (true == (*irg)->unsupported_attr_dspace) {
                    for (auto ira = (*irg)->attrs.begin(); ira != (*irg)->attrs.end();) {
                        if ((*ira)->count == 0) {
                            delete (*ira);
                            ira = (*irg)->attrs.erase(ira);
                        }
                        else {
                            ++ira;
                        }
                    }
                }
            }
        }
    }
}

// Handle unsupported data space information for variable and attribute 
void File::Handle_VarAttr_Unsupported_Dspace() 
{

    if (false == this->vars.empty()) {
        if (true == this->unsupported_var_attr_dspace) {
            for (const auto &var:this->vars) {
                if (false == var->attrs.empty()) {
                    if (true == var->unsupported_attr_dspace) {
                        for (auto ira = var->attrs.begin(); ira != var->attrs.end();) {
                            if (0 == (*ira)->count) {
                                delete (*ira);
                                ira = var->attrs.erase(ira);
                            }
                            else {
                                ++ira;
                            }
                        }
                    }
                }
            }
        }
    }
}

// Handle unsupported data space. 
void File::Handle_Unsupported_Dspace(bool include_attr) 
{

    // The unsupported data space 
    if (false == this->vars.empty()) {
        if (true == this->unsupported_var_dspace) {
            for (auto irv = this->vars.begin(); irv != this->vars.end();) {
                if (true == (*irv)->unsupported_dspace) {
                    delete (*irv);
                    irv = this->vars.erase(irv);
                }
                else {
                    ++irv;

                }
            }
        }
    }

    if (true == include_attr) {
        Handle_GroupAttr_Unsupported_Dspace();
        Handle_VarAttr_Unsupported_Dspace();
    }
}

// Generated unsupported dataspace Info when the BES ignored object key is on.
void File::Gen_Unsupported_Dspace_Info() 
{

    // Notice in this function, we deliberately don't put the case when an attribute dimension has 0 length.
    // Since doing this requires non-trivial change of the source code and the 0-size attribute case is really, really rare,
    // so we just "ignore" this case in the "ignored" information.
    // In fact, the zero size variable is allowed in both HDF5 and DAP2. So we don't ignore 0-size HDF5 dataset. So
    // the only case this function checks is the H5S_NULL case.
    if (false == this->vars.empty()) {
        if (true == this->unsupported_var_dspace) {
            for (const auto &var:this->vars) {
                if (true == var->unsupported_dspace) {
                    this->add_ignored_info_objs(true, var->fullpath);
                }
            }
        }
    }

}

// Handle other unsupported information.
void File::Handle_Unsupported_Others(bool include_attr) 
{

    if (true == this->check_ignored && true == include_attr) {

        if (true == HDF5RequestHandler::get_drop_long_string()) {

            // netCDF java doesn't have  limitation for attributes
#if 0
            for (vector<Attribute *>::iterator ira = this->root_attrs.begin(); ira != this->root_attrs.end(); ++ira) {
                if (H5FSTRING == (*ira)->dtype || H5VSTRING == (*ira)->dtype) {
                    if ((*ira)->getBufSize() > NC_JAVA_STR_SIZE_LIMIT) {
                        this->add_ignored_droplongstr_hdr();
                        this->add_ignored_grp_longstr_info("/", (*ira)->name);
                    }
                }
            }

            for (vector<Group *>::iterator irg = this->groups.begin(); irg != this->groups.end(); ++irg) {
                for (vector<Attribute *>::iterator ira = (*irg)->attrs.begin(); ira != (*irg)->attrs.end(); ++ira) {
                    if (H5FSTRING == (*ira)->dtype || H5VSTRING == (*ira)->dtype) {
                        if ((*ira)->getBufSize() > NC_JAVA_STR_SIZE_LIMIT) {
                            this->add_ignored_droplongstr_hdr();
                            this->add_ignored_grp_longstr_info((*irg)->path, (*ira)->name);
                        }
                    }
                }
            }
#endif
            for (const auto &var:this->vars) {
                if (true == Check_DropLongStr(var, nullptr)) {
                    this->add_ignored_droplongstr_hdr();
                    this->add_ignored_var_longstr_info(var, nullptr);
                }
                // netCDF java doesn't have  limitation for attributes
#if 0
                for (auto ira = (*irv)->attrs.begin(); ira != (*irv)->attrs.end(); ++ira) {
                    if (true == Check_DropLongStr((*irv), (*ira))) {
                        this->add_ignored_droplongstr_hdr();
                        this->add_ignored_var_longstr_info((*irv), (*ira));
                    }
                }
#endif
            }
        }
    }

}

// Flatten the object name, mainly call get_CF_string.
void File::Flatten_Obj_Name(bool include_attr) 
{
    for (auto &var:this->vars) {
        var->newname = get_CF_string(var->newname);
        for (auto &dim:var->dims) 
            dim->newname = get_CF_string(dim->newname);
    }

    if (true == include_attr) {

        for (auto &root_attr:this->root_attrs) {
            root_attr->newname = get_CF_string(root_attr->newname);
        }

        for (auto &grp:this->groups) {
            grp->newname = get_CF_string(grp->newname);
            for (auto &grp_attr:grp->attrs) 
                grp_attr->newname = get_CF_string(grp_attr->newname);
        }

        for (const auto &var:this->vars) {
            for (auto attr:var->attrs) 
                attr->newname = get_CF_string(attr->newname);
        }
    } 
}

// Variable name clashing
void File::Handle_Var_NameClashing(set<string>&objnameset) 
{

    Handle_General_NameClashing(objnameset, this->vars);
}

// Group name clashing
void File::Handle_Group_NameClashing(set<string> &objnameset) 
{

    pair<set<string>::iterator, bool> setret;

    // Now for DAS, we need to handle name clashing for
    // DAS tables, namely we need to make sure the global attribute
    // table(HDF5_GLOBAL) and the attribute tables mapped from 
    // HDF5 groups will not have name clashing with the variable name
    // lists. If having the name clashing, the global attribute table and
    // the attribute tables generated from the groups will be changed.
    // The file attribute name clashing

    setret = objnameset.insert(FILE_ATTR_TABLE_NAME);
    if (false == setret.second) {

        int clash_index = 1;
        string fa_clash_name = FILE_ATTR_TABLE_NAME;
        HDF5CFUtil::gen_unique_name(fa_clash_name, objnameset, clash_index);
        FILE_ATTR_TABLE_NAME = fa_clash_name;
    }

    // The group attribute name clashing
    Handle_General_NameClashing(objnameset, this->groups);

}

//Object attribute name clashing
void File::Handle_Obj_AttrNameClashing() 
{

    // Now handling the possible name clashing for attributes
    // For attribute clashing, we only need to resolve the name clashing
    // for attributes within each variable, file attributes and attributes
    // within each group. The name clashing for attributes should be very rare.
    // Potentially the checking and the correcting may be costly.
    // This is another reason for special products, we may not even need to check
    // the name clashings. KY 2011-12-24

    set<string> objnameset;

    // For root attributes   
    Handle_General_NameClashing(objnameset, this->root_attrs);

    // For group attributes
    for (const auto &grp:this->groups) {
        objnameset.clear();
        Handle_General_NameClashing(objnameset, grp->attrs);
    }

    // For variable attributes
    for (const auto &var:this->vars) {
        objnameset.clear();
        Handle_General_NameClashing(objnameset, var->attrs);
    }
}

// Handle General name clashing
//class T must have member string newname. In our case, T is either groups, attributes or vars.
template<class T> void File::Handle_General_NameClashing(set<string>&objnameset, vector<T*>& objvec) 
{

#if 0
//    set<string> objnameset;
#endif

    pair<set<string>::iterator, bool> setret;
    set<string>::iterator iss;

    vector<string> clashnamelist;

    map<int, int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;

    /*class*/

    for (const auto &obj:objvec) {
        setret = objnameset.insert(obj->newname);
        if (false == setret.second) {
            clashnamelist.insert(clashnamelist.end(), obj->newname);
            cl_to_ol[cl_index] = ol_index;
            cl_index++;
        }
        ol_index++;
    }

    // Now change the clashed elements to unique elements, 
    // Generate the set which has the same size as the original vector.
    for (auto &clashname:clashnamelist) {
        int clash_index = 1;
        string temp_clashname = clashname + '_';
        HDF5CFUtil::gen_unique_name(temp_clashname, objnameset, clash_index);
        clashname = temp_clashname;
    }

    // Now go back to the original vector, make it unique.
    for (unsigned int i = 0; i < clashnamelist.size(); i++)
        objvec[cl_to_ol[i]]->newname = clashnamelist[i];

}

// Handle General object name clashing
void File::Handle_GeneralObj_NameClashing(bool include_attr, set<string>& objnameset) 
{

    Handle_Var_NameClashing(objnameset);
    if (true == include_attr) {
        Handle_Group_NameClashing(objnameset);
        Handle_Obj_AttrNameClashing();
    }
}

// Get CF name, flatten the path, change the non-alphanumeric letters to underscore.
string File::get_CF_string(string s)
{

    if ("" == s) return s;
    string insertString(1, '_');

    // Always start with _ if the first character is not a letter
    if (true == isdigit(s[0])) s.insert(0, insertString);

    for (unsigned int i = 0; i < s.size(); i++)
        if ((false == isalnum(s[i])) && (s[i] != '_')) s[i] = '_';

    return s;

}

// For the connection of variable dimensions. Build dimname to dimsize(unlimited) maps
void File::Insert_One_NameSizeMap_Element(const string &name, hsize_t size, bool unlimited)
{
    pair<map<string, hsize_t>::iterator, bool> mapret;
    mapret = dimname_to_dimsize.insert(pair<string, hsize_t>(name, size));
    if (false == mapret.second)
    throw4("The dimension name ", name, " should map to ", size);

    pair<map<string, bool>::iterator, bool> mapret2;
    mapret2 = dimname_to_unlimited.insert(pair<string, bool>(name, unlimited));
    if (false == mapret2.second)
    throw3("The dimension name ", name, " unlimited dimension info. should be provided.");

}

// Similar to Inset_One_NameSizeMap_Element but the maps are provided as parameters.
void File::Insert_One_NameSizeMap_Element2(map<string, hsize_t>& name_to_size, map<string, bool>& name_to_unlimited,
    const string &name, hsize_t size, bool unlimited) const
{
    pair<map<string, hsize_t>::iterator, bool> mapret;
    mapret = name_to_size.insert(pair<string, hsize_t>(name, size));
    if (false == mapret.second)
    throw4("The dimension name ", name, " should map to ", size);

    pair<map<string, bool>::iterator, bool> mapret2;
    mapret2 = name_to_unlimited.insert(pair<string, bool>(name, unlimited));
    if (false == mapret2.second)
    throw3("The dimension name ", name, " unlimited dimension info. should be provided.");

}

// For dimension names added by the handlers, by default,
// Each dimension will have a unique dimension name. For example,
// Int foo[100][200] will be Int foo[Fakedim1][Fakedim2]
// If you have many variables, the dimension names may be too many.
// To reduce numbers, we ASSUME that the dimension having the same
// size shares the same dimension. In this way, the number of dimension names
// will be reduced.
// For example, Int foo2[100][300] will be Int foo2[Fakedim1][Fakedim3]
// instead of foo2[Fakedim3][Fakedim4]. However, that may impose
// another problem. Suppose Int Foosame[100][100] becomes
// Int Foosame[FakeDim1][FakeDim1]. This doesn't make sense for some
// applications. The function Adjust_Duplicate_FakeDim_Name will make sure
// this case will not happen. 
void File::Add_One_FakeDim_Name(Dimension *dim) 
{

    stringstream sfakedimindex;
    string fakedimstr = "FakeDim";
    pair<set<string>::iterator, bool> setret;
    map<hsize_t, string>::iterator im;
    pair<map<hsize_t, string>::iterator, bool> mapret;

    sfakedimindex << addeddimindex;
    string added_dimname = fakedimstr + sfakedimindex.str();

    // Build up the size to fakedim map.
    mapret = dimsize_to_fakedimname.insert(pair<hsize_t, string>(dim->size, added_dimname));
    if (false == mapret.second) { //The dim size exists, use the corresponding name.
        dim->name = dimsize_to_fakedimname[dim->size];
        dim->newname = dim->name;
    }
    else { // Insert this (dimsize,dimname) pair to dimsize_to_fakedimname map successfully.
           //First make sure this new dim name doesn't have name clashing
           // with previous dim names, after the checking, inserting to the
           // dimname list set.
           // dimnamelist is a private memeber of File.
        setret = dimnamelist.insert(added_dimname);
        if (false == setret.second) {
            int clash_index = 1;
            string temp_clashname = added_dimname + '_';
            HDF5CFUtil::gen_unique_name(temp_clashname, dimnamelist, clash_index);
            dim->name = temp_clashname;
            dim->newname = dim->name;
            setret = dimnamelist.insert(dim->name);
            if (false == setret.second)
            throw2("Fail to insert the unique dimsizede name ", dim->name);

            // We have to adjust the dim. name of the dimsize_to_fakedimname map, since the
            // dimname has been updated for this size.
            dimsize_to_fakedimname.erase(dim->size);
            mapret = dimsize_to_fakedimname.insert(pair<hsize_t, string>(dim->size, dim->name));
            if (false == mapret.second)
            throw4("The dimension size ", dim->size, " should map to ", dim->name);
        }

        // New dim name is inserted successfully, update the dimname_to_dimsize map.
        dim->name = added_dimname;
        dim->newname = dim->name;
        Insert_One_NameSizeMap_Element(dim->name, dim->size, dim->unlimited_dim);

        // Increase the dimindex since the new dimname has been inserted.
        addeddimindex++;
    }           // else
}

// See the function comments of Add_One_FakeDim_Name
void File::Adjust_Duplicate_FakeDim_Name(Dimension * dim) 
{

    // No need to adjust the dimsize_to_fakedimname map, only create a new Fakedim
    // The simplest way is to increase the dim index and resolve any name clashings with other dim names.
    // Note: No need to update the dimsize_to_dimname map since the original "FakeDim??" of this size
    // can be used as a dimension name of other variables. But we need to update the dimname_to_dimsize map
    // since this is a new dim name.
    stringstream sfakedimindex;
    pair<set<string>::iterator, bool> setret;

    addeddimindex++;
    sfakedimindex << addeddimindex;
    string added_dimname = "FakeDim" + sfakedimindex.str();
    setret = dimnamelist.insert(added_dimname);
    if (false == setret.second) {
        int clash_index = 1;
        string temp_clashname = added_dimname + '_';
        HDF5CFUtil::gen_unique_name(temp_clashname, dimnamelist, clash_index);
        dim->name = temp_clashname;
        dim->newname = dim->name;
        setret = dimnamelist.insert(dim->name);
        if (false == setret.second)
        throw2("Fail to insert the unique dimsizede name ", dim->name);
    }
    dim->name = added_dimname;
    dim->newname = dim->name;
    Insert_One_NameSizeMap_Element(dim->name, dim->size, dim->unlimited_dim);

    // Need to prepare for the next unique FakeDim. 
    addeddimindex++;
}
// 1. See the function comments of Add_One_FakeDim_Name
// 2. This is for the case when we have foo[100][100],foo2[100][100]. 
// The Adjust_Duplicate_FakeDim_Name() assures the uniqueness of Dimension
// names for each var,like foo[FakeDim0][FakeDim1],foo2[FakeDim0][FakeDim3].. 
// but the dimension names are not desirable for products like SMAP level 3.
// For similar vars, the dimension names like foo[FakeDim0][FakeDim1] and
// foo2[FakeDim0][FakeDim1] ,foo3[FakeDim0][FakeDim1][FakeDim2] are the way to go.

void File::Adjust_Duplicate_FakeDim_Name2(Dimension * dim, int dup_dim_size_index) 
{

    // Search if vector of dup (dimsize dimname) pair has the current size
    // If yes, insert the dim name from the vector. 
    // Note: in case we have foo[100][100][100] etc., we need to remember the
    // vector index, to obtain the correct dup_dim_size, dup_dim_name pair.
    // if no, build up the new FakeDim. 
    bool dup_dim_size_exist = false;
    int temp_dup_dim_size_index = 0;
    for (const auto &one_dup_dimsize_dimname:dup_dimsize_dimname) {
        // The dup vector may include different size, so we need to check
        // if having the same size.
        if(dim->size ==  one_dup_dimsize_dimname.first) { 
            temp_dup_dim_size_index++;
            // Make sure we obtain the correct index in the vector
            if(dup_dim_size_index == temp_dup_dim_size_index) {
                  dup_dim_size_exist = true;
                  dim->name = one_dup_dimsize_dimname.second;
                  dim->newname = dim->name;
                  break;
            }
        }
    }

    // If we cannot find this dimension in the existing (dup dimsize dimname) vector,
    // create the FakeDim by increasing the index and update the dup vector etc.
    if(dup_dim_size_exist == false) {

        stringstream sfakedimindex;
        pair<set<string>::iterator, bool> setret;
    
        sfakedimindex << addeddimindex;
        string added_dimname = "FakeDim" + sfakedimindex.str();
        setret = dimnamelist.insert(added_dimname);
        if (false == setret.second) {
            throw2("Inside Adjust_Duplicate_FakeDim_Name2(), Fail to insert the unique dim name ", dim->name);
        }
        dim->name = added_dimname;
        dim->newname = dim->name;
        Insert_One_NameSizeMap_Element(dim->name, dim->size, dim->unlimited_dim);
        // push back to the duplicate size, name vector.
        dup_dimsize_dimname.emplace_back(dim->size,dim->name);
        addeddimindex++;

    }

}

// Replace all dimension names, this function is currently not used. So comment out. May delete it in the future.
#if 0
void File::Replace_Dim_Name_All(const string orig_dim_name, const string new_dim_name)  {

    // The newname of the original dimension should also be replaced by new_dim_name
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
            if((*ird)->name == orig_dim_name) {
                (*ird)->name = new_dim_name;
                (*ird)->newname = new_dim_name;
            }

        }
    }
}
#endif

#if 0
void File::Use_Dim_Name_With_Size_All(const string dim_name, const size_t dim_size)  {

    // The newname of the original dimension should also be replaced by new_dim_name
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
            if((*ird)->size == orig_dim_name) {
                (*ird)->name = new_dim_name;
                (*ird)->newname = new_dim_name;
            }

        }
    }
}
#endif

// Often times we need to add a CF attribute with string datatype because some products don't provide them 
// Examples are units, comment etc.
void File::Add_Str_Attr(Attribute* attr, const string &attrname, const string& strvalue) const
{

    attr->name = attrname;
    attr->newname = attr->name;
    attr->dtype = H5FSTRING;
    attr->count = 1;
    attr->fstrsize = strvalue.size();
    attr->strsize.resize(1);
    attr->strsize[0] = attr->fstrsize;
    attr->value.resize(strvalue.size());
    copy(strvalue.begin(), strvalue.end(), attr->value.begin());
}

#if 0
bool
File:: Var_Has_Attr(Var*var,const string &attrname) {

    for (vector<Attribute *>:: iterator ira =var->attrs.begin(); ira !=var->attrs.end(); ++ira) {

        // We only check the original attribute name
        // Remove the original "coordinates" attribute.
        if((*ira)->name == attrname || (*ira)->newname == attrname) {
            return true;
        }
    }
    return false;
}
#endif

// Rretrieve the variable attribute in string.var_path is the variable path.
string File::Retrieve_Str_Attr_Value(Attribute *attr, const string & var_path) const
{

    if (attr != nullptr && var_path != "") {
        Retrieve_H5_Attr_Value(attr, var_path);
        string orig_attr_value(attr->value.begin(), attr->value.end());
        return orig_attr_value;
    }
    return "";

}

//Check if the attribute value of this variable is the input value.
bool File::Is_Str_Attr(Attribute* attr, const string& varfullpath, const string &attrname, const string& strvalue)
{
    bool ret_value = false;
    if (attrname == get_CF_string(attr->newname)) {
        Retrieve_H5_Attr_Value(attr, varfullpath);
        string attr_value(attr->value.begin(), attr->value.end());
        if (attr_value == strvalue) ret_value = true;
    }
    return ret_value;
}

// If this latitude or longitude units follows the CF 
bool File::has_latlon_cf_units(Attribute *attr, const string &varfullpath, bool is_lat)
{
    string attr_name = "units";
    if (true == is_lat) {
        string lat_unit_value = "degrees_north";
        return Is_Str_Attr(attr, varfullpath, attr_name, lat_unit_value);
    }
    else {
        string lon_unit_value = "degrees_east";
        return Is_Str_Attr(attr, varfullpath, attr_name, lon_unit_value);
    }
}

// This function is mainly to add _FillValue.
void File::Add_One_Float_Attr(Attribute* attr, const string &attrname, float float_value) const
{
    attr->name = attrname;
    attr->newname = attr->name;
    attr->dtype = H5FLOAT32;
    attr->count = 1;
    attr->value.resize(sizeof(float));
    memcpy(&(attr->value[0]), (void*) (&float_value), sizeof(float));
}

// Products like GPM use string type for MissingValue, we need to change them to the corresponding variable datatype and 
// get the value corrected.
void File::Change_Attr_One_Str_to_Others(Attribute* attr, const Var*var) const
{

    char *pEnd;
    // string to long int number.
    long int num_sli = 0;
    if (attr->dtype != H5FSTRING)
    throw2("Currently we only convert fixed-size string to other datatypes. ", attr->name);
    if (attr->count != 1)
        throw4("The fixed-size string count must be 1 and the current count is ", attr->count, " for the attribute ",
            attr->name);

    Retrieve_H5_Attr_Value(attr, var->fullpath);
    string attr_value;
    attr_value.resize(attr->value.size());
    copy(attr->value.begin(), attr->value.end(), attr_value.begin());

    switch (var->dtype) {

    case H5UCHAR: {
        num_sli = strtol(&(attr->value[0]), &pEnd, 10);
        if (num_sli < 0 || num_sli > UCHAR_MAX)
            throw5("Attribute type is unsigned char, the current attribute ", attr->name, " has the value ", num_sli,
                ". It is overflowed. ");
        else {
            auto num_suc = (unsigned char) num_sli;
            attr->dtype = H5UCHAR;
            attr->value.resize(sizeof(unsigned char));
            memcpy(&(attr->value[0]), (void*) (&num_suc), sizeof(unsigned char));
        }

    }
        break;
    case H5CHAR: {
        num_sli = strtol(&(attr->value[0]), &pEnd, 10);
        if (num_sli < SCHAR_MIN || num_sli > SCHAR_MAX)
            throw5("Attribute type is signed char, the current attribute ", attr->name, " has the value ", num_sli,
                ". It is overflowed. ");
        else {
            auto num_sc = (char) num_sli;
            attr->dtype = H5CHAR;
            attr->value.resize(sizeof(char));
            memcpy(&(attr->value[0]), (void*) (&num_sc), sizeof(char));
        }

    }
        break;
    case H5INT16: {
        num_sli = strtol(&(attr->value[0]), &pEnd, 10);
        if (num_sli < SHRT_MIN || num_sli > SHRT_MAX)
            throw5("Attribute type is 16-bit integer, the current attribute ", attr->name, " has the value ", num_sli,
                ". It is overflowed. ");
        else {
            auto num_ss = (short) num_sli;
            attr->dtype = H5INT16;
            attr->value.resize(sizeof(short));
            memcpy(&(attr->value[0]), (void*) (&num_ss), sizeof(short));
        }

    }
        break;
    case H5UINT16: {
        num_sli = strtol(&(attr->value[0]), &pEnd, 10);
        if (num_sli < 0 || num_sli > USHRT_MAX)
            throw5("Attribute type is unsigned 16-bit integer, the current attribute ", attr->name, " has the value ",
                num_sli, ". It is overflowed. ");
        else {
            auto num_uss = (unsigned short) num_sli;
            attr->dtype = H5UINT16;
            attr->value.resize(sizeof(unsigned short));
            memcpy(&(attr->value[0]), (void*) (&num_uss), sizeof(unsigned short));
        }
    }
        break;
    case H5INT32: {
        num_sli = strtol(&(attr->value[0]), &pEnd, 10);
        //No need to check overflow, the number will always be in the range.
#if 0
        //if(num_sli <LONG_MIN || num_sli >LONG_MAX)
        //    throw5("Attribute type is 32-bit integer, the current attribute ",attr->name, " has the value ",num_sli, ". It is overflowed. ");
#endif
        attr->dtype = H5INT32;
        attr->value.resize(sizeof(long int));
        memcpy(&(attr->value[0]), (void*) (&num_sli), sizeof(long int));

    }
        break;
    case H5UINT32: {
        unsigned long int num_suli = strtoul(&(attr->value[0]), &pEnd, 10);
        // No need to check since num_suli will not be bigger than ULONG_MAX.
        attr->dtype = H5UINT32;
        attr->value.resize(sizeof(unsigned long int));
        memcpy(&(attr->value[0]), (void*) (&num_suli), sizeof(unsigned long int));
    }
        break;
    case H5FLOAT32: {
        float num_sf = strtof(&(attr->value[0]), nullptr);
        // Don't think it is necessary to check if floating-point is overflowed for this routine. ignore it now. KY 2014-09-22
        attr->dtype = H5FLOAT32;
        attr->value.resize(sizeof(float));
        memcpy(&(attr->value[0]), (void*) (&num_sf), sizeof(float));
    }
        break;
    case H5FLOAT64: {
        double num_sd = strtod(&(attr->value[0]), nullptr);
        // Don't think it is necessary to check if floating-point is overflowed for this routine. ignore it now. KY 2014-09-22
        attr->dtype = H5FLOAT64;
        attr->value.resize(sizeof(double));
        memcpy(&(attr->value[0]), (void*) (&num_sd), sizeof(double));
    }
        break;

    default:
        throw4("Unsupported HDF5 datatype that the string is converted to for the attribute ", attr->name,
            " of the variable ", var->fullpath);
    } // "switch(var->dtype)"

}

// Change a string type attribute to the input value 
void File::Replace_Var_Str_Attr(Var* var, const string &attr_name, const string& strvalue)
{

    bool rep_attr = true;
    bool rem_attr = false;
    for (auto &attr:var->attrs) {
        if (attr->name == attr_name) {
            if (true == Is_Str_Attr(attr, var->fullpath, attr_name, strvalue))
                rep_attr = false;
            else
                rem_attr = true;
            break;
        }
    }

    // Remove the attribute if the attribute value is not strvalue
    if (true == rem_attr) {
        for (auto ira = var->attrs.begin(); ira != var->attrs.end(); ira++) {
            if ((*ira)->name == attr_name) {
                delete (*ira);
                var->attrs.erase(ira);
                break;
            }
        }
    }

    // Add the attribute with strvalue
    if (true == rep_attr) {
        auto attr_unique = make_unique<Attribute>();
        auto attr = attr_unique.release();
        Add_Str_Attr(attr, attr_name, strvalue);
        var->attrs.push_back(attr);
    }
}

// Check if this variable is latitude,longitude.We check the three name pairs(lat,lon),(latitude,longitude),(Latitude,Longitude)
bool File::Is_geolatlon(const string & var_name, bool is_lat) const
{

    bool ret_value = false;
    if (true == is_lat) {
        string lat1 = "lat";
        string lat2 = "latitude";
        string lat3 = "Latitude";

        if (var_name.compare(lat1) == 0 || var_name.compare(lat2) == 0 || var_name.compare(lat3) == 0) ret_value = true;
    }

    else {
        string lon1 = "lon";
        string lon2 = "longitude";
        string lon3 = "Longitude";
        if (var_name.compare(lon1) == 0 || var_name.compare(lon2) == 0 || var_name.compare(lon3) == 0) ret_value = true;

    }
    return ret_value;
}

// Add supplementary attributes.
void File::Add_Supplement_Attrs(bool add_path) 
{

    if (false == add_path) return;

    // Adding variable original name(origname) and full path(fullpath)
    for (auto &var:this->vars) {
        auto attr_unique = make_unique<Attribute>();
        auto attr = attr_unique.release();
        const string varname = var->name;
        const string attrname = "origname";
        Add_Str_Attr(attr, attrname, varname);
        var->attrs.push_back(attr);
    }

    for (auto &var:this->vars) {
        // Turn off the fullnamepath attribute when zero_storage_size is 0.
        // Use the BES key since quite a few testing cases will be affected.
        // KY 2020-03-23
        if (var->zero_storage_size==false 
           || HDF5RequestHandler::get_no_zero_size_fullnameattr() == false) {
            auto attr_unique = make_unique<Attribute>();
            auto attr = attr_unique.release();
            const string varname = var->fullpath;
            const string attrname = "fullnamepath";
            Add_Str_Attr(attr, attrname, varname);
            var->attrs.push_back(attr);
        }
    }

    // Adding group path
    for (const auto &grp:this->groups) {
        // Only when this group has attributes, the original path of the group has some values. So add it.
        if (false == grp->attrs.empty()) {

            const string varname = grp->path;
            const string attrname = "fullnamepath";
            auto attr_unique = make_unique<Attribute>();
            auto attr = attr_unique.release();
            Add_Str_Attr(attr, attrname, varname);
            grp->attrs.push_back(attr);
        }
    }
}

// Variable target will not be deleted, but rather its contents are replaced.
// We may make this as an operator = in the future.
// Note: the attributes can not be replaced.
void File::Replace_Var_Info(const Var *src, Var *target)
{

#if 0
    for_each (target->dims.begin (), target->dims.end (),
        delete_elem ());
    for_each (target->attrs.begin (), target->attrs.end (),
        delete_elem ());
#endif

    target->newname = src->newname;
    target->name = src->name;
    target->fullpath = src->fullpath;
    target->rank = src->rank;
    target->dtype = src->dtype;
    target->unsupported_attr_dtype = src->unsupported_attr_dtype;
    target->unsupported_dspace = src->unsupported_dspace;
#if 0
    for (auto ira = target->attrs.begin();
        ira!=target->attrs.end(); ++ira) {
        delete (*ira);
        target->attrs.erase(ira);
        ira--;
    }
#endif
    for (auto ird = target->dims.begin(); ird != target->dims.end();) {
        delete (*ird);
        ird = target->dims.erase(ird);
    }

    // Somehow attributes cannot be replaced. 
#if 0
    for (vector<Attribute*>::iterator ira = src->attrs.begin();
        ira!=src->attrs.end(); ++ira) {
        Attribute* attr= new Attribute();
        attr->name = (*ira)->name;
        attr->newname = (*ira)->newname;
        attr->dtype =(*ira)->dtype;
        attr->count =(*ira)->count;
        attr->strsize = (*ira)->strsize;
        attr->fstrsize = (*ira)->fstrsize;
        attr->value =(*ira)->value;
        target->attrs.push_back(attr);
    }
#endif

    for (const auto& sdim:src->dims) {
        auto dim_unique = make_unique<Dimension>(sdim->size);
        auto dim = dim_unique.release();
        dim->name = sdim->name;
        dim->newname = sdim->newname;
        target->dims.push_back(dim);
    }

}

// Replace the attributes of target with src.
void File::Replace_Var_Attrs(const Var *src, Var *target)
{

#if 0
    for_each (target->dims.begin (), target->dims.end (),
        delete_elem ());
    for_each (target->attrs.begin (), target->attrs.end (),
        delete_elem ());
#endif

    for (auto ira = target->attrs.begin(); ira != target->attrs.end();) {
        delete (*ira);
        ira = target->attrs.erase(ira);
    }
    for (const auto &sattr:src->attrs) {
        auto attr_unique = make_unique<Attribute>();
        auto attr = attr_unique.release();
        attr->name = sattr->name;
        attr->newname = sattr->newname;
        attr->dtype = sattr->dtype;
        attr->count = sattr->count;
        attr->strsize = sattr->strsize;
        attr->fstrsize = sattr->fstrsize;
        attr->value = sattr->value;
        target->attrs.push_back(attr);
    }

}

// Check if a variable with a var name is under a specific group with group name
// note: the variable's size at each dimension is also returned. The user must allocate the 
// memory for the dimension sizes of an array(vector is preferred).
bool File::is_var_under_group(const string &varname, const string &grpname, const int var_rank,
    vector<size_t> & var_size) const
{

    bool ret_value = false;
    for (const auto &var:this->vars) {

        if (var->rank == var_rank) {
            if (var->name == varname) {

                // Obtain the variable path
                string var_path = HDF5CFUtil::obtain_string_before_lastslash(var->fullpath);

                // Check if we find the variable under this group
                if (grpname == var_path) {
                    ret_value = true;
                    for (int i = 0; i < var_rank; i++)
                        var_size[i] = var->getDimensions()[i]->size;
                    break;
                }
            }
        }
    } 

    return ret_value;
}

bool File::Have_Grid_Mapping_Attrs(){

    bool ret_value = false;
    for (const auto& var:this->vars) {
        for (const auto& attr:var->attrs) {
            if(attr->name =="grid_mapping") {
                ret_value = true;
                break;
            }
        }
        if(true == ret_value)
            break;
    }

    return ret_value;
}

void File::Handle_Grid_Mapping_Vars(){

    for (auto &var:this->vars) {
        string attr_value;
        for (auto ira = var->attrs.begin(); ira != var->attrs.end(); ++ira) {
            if((*ira)->name =="grid_mapping") {
                Retrieve_H5_Attr_Value(*ira, var->fullpath);
                attr_value.resize((*ira)->value.size());
                copy((*ira)->value.begin(), (*ira)->value.end(), attr_value.begin());
                break;
            }
 
        }
        if(attr_value.find('/') ==string::npos){
            string new_name = Check_Grid_Mapping_VarName(attr_value,var->fullpath);
            if(new_name != "")
                Replace_Var_Str_Attr(var,"grid_mapping",new_name);
 
        }
        else {
            string new_name = Check_Grid_Mapping_FullPath(attr_value);
            //Using new_name as the attribute value
            if(new_name != "")
                Replace_Var_Str_Attr(var,"grid_mapping",new_name);
        }
    }

}

string File::Check_Grid_Mapping_VarName(const string & a_value,const string & var_fpath) const {
    
    string var_path = HDF5CFUtil::obtain_string_before_lastslash(var_fpath);
    string gmap_new_name;
    for (const auto &var:this->vars) {
        if(var->name == a_value){
            if(var_path == HDF5CFUtil::obtain_string_before_lastslash(var->fullpath)) {
                gmap_new_name = var->newname;
                break;
            }
        }
    }
    return gmap_new_name;
}


string File::Check_Grid_Mapping_FullPath(const string & a_value) const {

    string gmap_new_name;
    for (const auto &var:this->vars) {
        if(var->fullpath == a_value){
            gmap_new_name = var->newname;
            break;
        }
    }

    return gmap_new_name;
}

void File::remove_netCDF_internal_attributes(bool include_attr) {

    if(true == include_attr) {

        for (const auto &var:this->vars) {

            bool var_has_dimscale = false;
            
            for (auto ira = var->attrs.begin(); ira != var->attrs.end();) {
                if((*ira)->name == "CLASS") {
                    string class_value = Retrieve_Str_Attr_Value(*ira,var->fullpath);

                    // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
                    // "DIMENSION_SCALE", which is 15.
                    if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                        delete(*ira);
                        ira = var->attrs.erase(ira);
                        var_has_dimscale = true;
                        
                    }
#if 0
                        else if(1) {// Add a BES key,also delete

                        }
#endif
                    else {
                        ++ira;
                    }
                }
#if 0
                else if((*ira)->name == "NAME") {// Add a BES Key 
                    string name_value = Retrieve_Str_Attr_Value(*ira,var->fullpath);
                    if( 0 == name_value.compare(0,var->name.size(),var->name)) {
                        delete(*ira);
                        ira =var->attrs.erase(ira);
                    }
                    else {
                        string netcdf_dim_mark= "This is a netCDF dimension but not a netCDF variable";
                        if( 0 == name_value.compare(0,netcdf_dim_mark.size(),netcdf_dim_mark)) {
                            delete(*ira);
                            ira =var->attrs.erase(ira);
                        }
                        else {
                            ++ira;
                        }
                    }

                }
#endif
                else if((*ira)->name == "_Netcdf4Dimid") {
                        delete(*ira);
                        ira =var->attrs.erase(ira);
                }
                else if((*ira)->name == "_Netcdf4Coordinates") {
                        delete(*ira);
                        ira =var->attrs.erase(ira);
                }
#if 0
                else if((*ira)->name == "_nc3_strict") {
                        delete((*ira));
                        ira =var->attrs.erase(ira);
                }
#endif
                else {
                    ++ira;
                }
            }

            if(true == var_has_dimscale) {
                for(auto ira = var->attrs.begin();
                    ira != var->attrs.end();++ira) {
                    if((*ira)->name == "NAME") {// Add a BES Key 
                        delete(*ira);
                        ira =var->attrs.erase(ira);
                        break;
                    }
                }
            }
        }
    }

}
// Add ignored page header info. Mainly a helper message.
void File::add_ignored_info_page_header()
{
    ignored_msg =
        " \n This page is for HDF5 CF hyrax data providers or distributors to check if any HDF5 object or attribute information are ignored during the mapping. \n\n";
}

// Add ignored object header info. Mainly a helper message.
void File::add_ignored_info_obj_header()
{

    ignored_msg += " Some HDF5 objects or the object information are ignored when mapping to DAP2 by the HDF5 OPeNDAP";
    ignored_msg += " handler due to the restrictions of DAP2, CF conventions or CF tools.";
    ignored_msg += " Please use HDF5 tools(h5dump or HDFView) to check carefully and make sure that these objects";
    ignored_msg +=
        " are OK to ignore for your service. For questions or requests to find a way to handle the ignored objects, please";
    ignored_msg += " contact the HDF5 OPeNDAP handler developer or send an email to help@hdfgroup.org.\n";

    ignored_msg += " \n In general, ignored HDF5 objects include HDF5 soft links, external links and named datatype.\n";
    ignored_msg +=
        " \n The HDF5 datasets(variables in the CF term) and attributes that have the following datatypes are ignored: \n";
    ignored_msg +=
        " Signed and unsigned 64-bit integers, HDF5 compound, HDF5 variable size(excluding variable length string),";
    ignored_msg += " HDF5 reference, HDF5 enum, HDF5 opaque , HDF5 bitfield, HDF5 Array and HDF5 Time datatypes.\n";

    ignored_msg +=
        " \n The HDF5 datasets(variables in the CF term) and attributes associated with the following dimensions are ignored: \n";
    ignored_msg += " 1) variables that have HDF5 NULL dataspace(H5S_NULL)(rarely occurred)\n";
    ignored_msg += " 2) attributes that have any zero size dimensions(not reported due to extreme rarity and non-trivial coding)\n\n";

}

// Add the ignored links information.Mainly a helper message.
void File::add_ignored_info_links_header()
{

    if (false == this->have_ignored) {
        add_ignored_info_obj_header();
        have_ignored = true;
    }
    // Add ignored datatype header.
    string lh_msg = "******WARNING******\n";
    lh_msg += "IGNORED soft links or external links are: ";
    if (ignored_msg.rfind(lh_msg) == string::npos) ignored_msg += lh_msg + "\n";

}

// Leave the code for the time being.
#if 0
void
File:: add_ignored_info_obj_dtype_header() {

    // Add ignored datatype header.
    ignored_msg += " \n Variables and attributes ignored due to the unsupported datatypes. \n";
    ignored_msg += " In general, the unsupported datatypes include: \n";
    ignored_msg += " Signed and unsigned 64-bit integers, HDF5 compound, HDF5 variable size(excluding variable length string),";
    ignored_msg += " HDF5 reference, HDF5 enum, HDF5 opaque , HDF5 bitfield, HDF5 Array and HDF5 Time datatypes.\n";

}

void
File:: add_ignored_info_obj_dspace_header() {

    // Add ignored dataspace header.
    ignored_msg += " \n Variables and attributes ignored due to the unsupported dimensions. \n";
    ignored_msg += " In general, the unsupported dimensions include: \n";
    ignored_msg += " 1) variables that have HDF5 NULL dataspace(H5S_NULL)(rarely occurred)\n";
    ignored_msg += " 2) variables that have any zero size dimensions\n";

}
#endif

// Add the ignored link info.
void File::add_ignored_info_links(const string & link_path)
{
    if (ignored_msg.find("Link paths: ") == string::npos)
        ignored_msg += " Link paths: " + link_path;
    else
        ignored_msg += " " + link_path;
}

// Add the ignored name datatype info.
void File::add_ignored_info_namedtypes(const string& grp_name, const string& named_dtype_name)
{

    if (false == this->have_ignored) {
        add_ignored_info_obj_header();
        have_ignored = true;
    }

    string ignored_HDF5_named_dtype_hdr = "\n******WARNING******";
    ignored_HDF5_named_dtype_hdr += "\n IGNORED HDF5 named datatype objects:\n";
    string ignored_HDF5_named_dtype_msg = " Group name: " + grp_name + "  HDF5 named datatype name: " + named_dtype_name.substr(0,named_dtype_name.size()-1)
        + "\n";
    if (ignored_msg.find(ignored_HDF5_named_dtype_hdr) == string::npos)
        ignored_msg += ignored_HDF5_named_dtype_hdr + ignored_HDF5_named_dtype_msg;
    else
        ignored_msg += ignored_HDF5_named_dtype_msg;

}

// Add the ignored attribute information. When is_grp is true, the ignored group attribute names are added.
// Otherwise, the ignored dataset attribute names are added.
void File::add_ignored_info_attrs(bool is_grp, const string & obj_path, const string & attr_name)
{

    if (false == this->have_ignored) {
        add_ignored_info_obj_header();
        have_ignored = true;
    }


    string ignored_warning_str = "\n******WARNING******";
    string ignored_HDF5_grp_hdr = ignored_warning_str + "\n Ignored attributes under root and groups:\n";
    string ignored_HDF5_grp_msg = " Group path: " + obj_path + "  Attribute names: " + attr_name + "\n";
    string ignored_HDF5_var_hdr = ignored_warning_str + "\n Ignored attributes for variables:\n";
    string ignored_HDF5_var_msg = " Variable path: " + obj_path + "  Attribute names: " + attr_name + "\n";


    if (true == is_grp) {
        if (ignored_msg.find(ignored_HDF5_grp_hdr) == string::npos)
            ignored_msg += ignored_HDF5_grp_hdr + ignored_HDF5_grp_msg;
        else
            ignored_msg += ignored_HDF5_grp_msg;
    }
    else {
        if (ignored_msg.find(ignored_HDF5_var_hdr) == string::npos)
            ignored_msg += ignored_HDF5_var_hdr + ignored_HDF5_var_msg;
        else
            ignored_msg += ignored_HDF5_var_msg;
    }

}

//Ignored object information. When is_dim_related is true, ignored data space info. is present.
//When is_dim_related is false, ignored data type info. is present.
void File::add_ignored_info_objs(bool is_dim_related, const string & obj_path)
{

    if (false == this->have_ignored) {
        add_ignored_info_obj_header();
        have_ignored = true;
    }

    string ignored_warning_str = "\n******WARNING******";
    string ignored_HDF5_dtype_var_hdr = ignored_warning_str + "\n IGNORED variables due to unsupported datatypes:\n";
    string ignored_HDF5_dspace_var_hdr = ignored_warning_str + "\n IGNORED variables due to unsupported dimensions:\n";
    string ignored_HDF5_var_msg = " Variable path: " + obj_path + "\n";

    if (true == is_dim_related) {
        if (ignored_msg.find(ignored_HDF5_dspace_var_hdr) == string::npos)
            ignored_msg += ignored_HDF5_dspace_var_hdr + ignored_HDF5_var_msg;
        else
            ignored_msg += ignored_HDF5_var_msg;

    }
    else {
        if (ignored_msg.find(ignored_HDF5_dtype_var_hdr) == string::npos)
            ignored_msg += ignored_HDF5_dtype_var_hdr + ignored_HDF5_var_msg;
        else
            ignored_msg += ignored_HDF5_var_msg;
    }

}

// No ignored info.
void File::add_no_ignored_info()
{

    ignored_msg += "There are no ignored HDF5 objects or attributes.";

}

// This function should only be used when the HDF5 file is following the netCDF data model.
// Check if we should not report the Dimension scale related attributes as ignored.
bool File::ignored_dimscale_ref_list(const Var *var) const
{

    bool ignored_dimscale = true;
    // Only when "General_Product == this->product_type && GENERAL_DIMSCALE== this->gproduct_pattern)" 

    bool has_dimscale = false;
    bool has_reference_list = false;
    for (const auto &attr:var->attrs) {
        if (attr->name == "REFERENCE_LIST" && false == HDF5CFUtil::cf_strict_support_type(attr->getType(),_is_dap4))
            has_reference_list = true;
        if (attr->name == "CLASS") {
            Retrieve_H5_Attr_Value(attr, var->fullpath);
            string class_value;
            class_value.resize(attr->value.size());
            copy(attr->value.begin(), attr->value.end(), class_value.begin());

            // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
            // "DIMENSION_SCALE", which is 15.
            if (0 == class_value.compare(0, 15, "DIMENSION_SCALE")) {
                has_dimscale = true;
            }
        }

        if (true == has_dimscale && true == has_reference_list) {
            ignored_dimscale = false;
            break;
        }

    }
    return ignored_dimscale;
}

// Check if the long string can should be dropped from a dataset or an attribute. Users can set up a BES key to turn it off or on.
bool File::Check_DropLongStr(const Var *var, const Attribute * attr) const
{

    bool drop_longstr = false;
    if (nullptr == attr) {
        if (H5FSTRING == var->dtype || H5VSTRING == var->dtype) {
            try {
                drop_longstr = Check_VarDropLongStr(var->fullpath, var->dims, var->dtype);
            }
            catch (...) {
                throw1("Check_VarDropLongStr fails ");
            }
        }
    }
    // No limitation for the attributes. KY 2018-02-26
#if 0
    else {
        if (H5FSTRING == attr->dtype || H5VSTRING == attr->dtype) {
            if (attr->getBufSize() > NC_JAVA_STR_SIZE_LIMIT) {
                drop_longstr = true;
            }
        }

    }
#endif
    return drop_longstr;
}

// Check if a long string dataset should be dropped. Users can turn on a BES key not to drop the long string.
// However, the Java clients may not access.
//
bool File::Check_VarDropLongStr(const string & varpath, const vector<Dimension *>& dims, H5DataType dtype) const
    
{

    bool drop_longstr = false;

    hid_t dset_id = H5Dopen2(this->fileid, varpath.c_str(), H5P_DEFAULT);
    if (dset_id < 0)
        throw2("Cannot open the dataset  ", varpath);

    hid_t dtype_id = -1;
    if ((dtype_id = H5Dget_type(dset_id)) < 0) {
        H5Dclose(dset_id);
        throw2("Cannot obtain the datatype of the dataset  ", varpath);
    }

    size_t ty_size = H5Tget_size(dtype_id);
    if (ty_size == 0) {
        H5Tclose(dtype_id);
        H5Dclose(dset_id);
        throw2("Cannot obtain the datatype size of the dataset  ", varpath);
    }

    if (H5FSTRING == dtype) { // Fixed-size, just check the number of elements.
        if (ty_size > NC_JAVA_STR_SIZE_LIMIT) drop_longstr = true;
    }
    else if (H5VSTRING == dtype) {

        unsigned long long total_elms = 1;
        if (dims.empty() == false) {
            for (const auto &dim:dims)
                total_elms = total_elms * (dim->size);
        }
        vector<char> strval;
        strval.resize(total_elms * ty_size);
        hid_t read_ret = H5Dread(dset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*) strval.data());
        if (read_ret < 0) {
            H5Tclose(dtype_id);
            H5Dclose(dset_id);
            throw2("Cannot read the data of the dataset  ", varpath);
        }

        vector<string> finstrval;
        finstrval.resize(total_elms);
        char*temp_bp = strval.data();
        char*onestring = nullptr;
        for (unsigned long long i = 0; i < total_elms; i++) {
            onestring = *(char**) temp_bp;
            if (onestring != nullptr) {
                finstrval[i] = string(onestring);
                if(finstrval[i].size()>NC_JAVA_STR_SIZE_LIMIT) {
                    drop_longstr = true;
                    break;
                }
            }
            temp_bp += ty_size;
        }

        if (false == strval.empty()) {
            herr_t ret_vlen_claim;
            hid_t dspace_id = H5Dget_space(dset_id);
            if (dspace_id < 0) {
                H5Tclose(dtype_id);
                H5Dclose(dset_id);
                throw2("Cannot obtain the dataspace id.", varpath);
            }
            ret_vlen_claim = H5Dvlen_reclaim(dtype_id, dspace_id, H5P_DEFAULT, (void*) strval.data());
            if (ret_vlen_claim < 0) {
                H5Tclose(dtype_id);
                H5Sclose(dspace_id);
                H5Dclose(dset_id);
                throw2("Cannot reclaim the vlen space  ", varpath);
            }
            if (H5Sclose(dspace_id) < 0) {
                H5Tclose(dtype_id);
                H5Dclose(dset_id);
                throw2("Cannot close the HDF5 data space.", varpath);
            }
        }
    }
    if (H5Tclose(dtype_id) < 0) {
        H5Dclose(dset_id);
        throw2("Cannot close the HDF5 data type.", varpath);
    }
    if (H5Dclose(dset_id) < 0)
        throw2("Cannot close the HDF5 data type.", varpath);
    
    return drop_longstr;
}
#if 0
bool File::Check_VarDropLongStr(const string & varpath, const vector<Dimension *>& dims, H5DataType dtype)
    
{

    bool drop_longstr = false;

    unsigned long long total_elms = 1;
    if (dims.size() != 0) {
        for (unsigned int i = 0; i < dims.size(); i++)
            total_elms = total_elms * ((dims[i])->size);
    }

    if (total_elms > NC_JAVA_STR_SIZE_LIMIT)
        drop_longstr = true;

    else { // We need to check both fixed-size and variable-length strings.

        hid_t dset_id = H5Dopen2(this->fileid, varpath.c_str(), H5P_DEFAULT);
        if (dset_id < 0)
        throw2("Cannot open the dataset  ", varpath);

        hid_t dtype_id = -1;
        if ((dtype_id = H5Dget_type(dset_id)) < 0) {
            H5Dclose(dset_id);
            throw2("Cannot obtain the datatype of the dataset  ", varpath);
        }

        size_t ty_size = H5Tget_size(dtype_id);
        if (ty_size == 0) {
            H5Tclose(dtype_id);
            H5Dclose(dset_id);
            throw2("Cannot obtain the datatype size of the dataset  ", varpath);
        }

        if (H5FSTRING == dtype) { // Fixed-size, just check the number of elements.
            if ((ty_size * total_elms) > NC_JAVA_STR_SIZE_LIMIT) drop_longstr = true;
        }
        else if (H5VSTRING == dtype) {

            vector<char> strval;
            strval.resize(total_elms * ty_size);
            hid_t read_ret = H5Dread(dset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*) strval.data());
            if (read_ret < 0) {
                H5Tclose(dtype_id);
                H5Dclose(dset_id);
                throw2("Cannot read the data of the dataset  ", varpath);
            }

            vector<string> finstrval;
            finstrval.resize(total_elms);
            char*temp_bp = strval.data();
            char*onestring = nullptr;
            for (unsigned long long i = 0; i < total_elms; i++) {
                onestring = *(char**) temp_bp;
                if (onestring != nullptr)
                    finstrval[i] = string(onestring);
                else
                    // We will add a NULL if onestring is NULL.
                    finstrval[i] = "";
                temp_bp += ty_size;
            }

            if (false == strval.empty()) {
                herr_t ret_vlen_claim;
                hid_t dspace_id = H5Dget_space(dset_id);
                if (dspace_id < 0) {
                    H5Tclose(dtype_id);
                    H5Dclose(dset_id);
                    throw2("Cannot obtain the dataspace id.", varpath);
                }
                ret_vlen_claim = H5Dvlen_reclaim(dtype_id, dspace_id, H5P_DEFAULT, (void*) strval.data());
                if (ret_vlen_claim < 0) {
                    H5Tclose(dtype_id);
                    H5Sclose(dspace_id);
                    H5Dclose(dset_id);
                    throw2("Cannot reclaim the vlen space  ", varpath);
                }
                if (H5Sclose(dspace_id) < 0) {
                    H5Tclose(dtype_id);
                    H5Dclose(dset_id);
                    throw2("Cannot close the HDF5 data space.", varpath);
                }
            }
            unsigned long long total_str_size = 0;
            for (unsigned long long i = 0; i < total_elms; i++) {
                total_str_size += finstrval[i].size();
                if (total_str_size > NC_JAVA_STR_SIZE_LIMIT) {
                    drop_longstr = true;
                    break;
                }
            }
        }
        if (H5Tclose(dtype_id) < 0) {
            H5Dclose(dset_id);
            throw2("Cannot close the HDF5 data type.", varpath);
        }
        if (H5Dclose(dset_id) < 0)
        throw2("Cannot close the HDF5 data type.", varpath);
    }
    return drop_longstr;
}
#endif


// Provide if the long string is dropped.
void File::add_ignored_grp_longstr_info(const string& grp_path, const string & attr_name)
{
    ignored_msg += "The HDF5 group: " + grp_path + " has an empty-set string attribute: " + attr_name + "\n";

}

// Provide if the long variable string is dropped.
void File::add_ignored_var_longstr_info(const Var *var, const Attribute *attr) 
{

    if (nullptr == attr)
        ignored_msg += "String variable: " + var->fullpath + " value is set to empty.\n";
    else {
        ignored_msg += "The variable: " + var->fullpath + " has an empty-set string attribute: " + attr->name + "\n";

    }

}

// The warnings of the drop of the long string header
void File::add_ignored_droplongstr_hdr()
{

    if (false == this->have_ignored) this->have_ignored = true;
    string hdr = "\n\n The values of the following string variables ";
    hdr += " are set to empty because at least one string size in this variable exceeds netCDF Java string limit(32767 bytes).\n";
    hdr += "To obtain the values, change the BES key H5.EnableDropLongString=true at the handler BES";
    hdr += " configuration file(h5.conf)\nto H5.EnableDropLongString=false.\n\n";

    if (ignored_msg.rfind(hdr) == string::npos) ignored_msg += hdr;

}

// Sometimes, we need to release the temporary added resources.
void File::release_standalone_var_vector(vector<Var*>&temp_vars)
{

    for (auto i = temp_vars.begin(); i != temp_vars.end();) {
        delete (*i);
        i = temp_vars.erase(i);
    }

}
