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
/// \file HDFEOS5CF.cc
/// \brief Implementation of the mapping of HDF-EOS5 products to DAP by following CF
///
///  It includes functions to 
///  1) retrieve HDF5 metadata info.
///  2) translate HDF5 objects into DAP DDS and DAS by following CF conventions.
///
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2023 The HDF Group
///
/// All rights reserved.

#include "HDF5CF.h"
#include "HDF5RequestHandler.h"
#include "h5cfdaputil.h"
#include "BESDebug.h"

using namespace std;
using namespace libdap;
using namespace HDF5CF;

// A constructor of EOS5CVar
EOS5CVar::EOS5CVar(const Var*var)
{

    newname = var->newname;
    name = var->name;
    fullpath = var->fullpath;
    rank = var->rank;
    total_elems = var->total_elems;
    zero_storage_size = var->zero_storage_size;
    dtype = var->dtype;
    unsupported_attr_dtype = var->unsupported_attr_dtype;
    unsupported_dspace = var->unsupported_dspace;
    coord_attr_add_path = false;

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

    // For the coordinate variable specific fields, we just fill in the default one in the ctr
    // If needed, the caller of this function should fill in those information after calling this function.
    eos_type = OTHERVARS;
    is_2dlatlon = false;
    point_lower = 0.0;
    point_upper = 0.0;
    point_left = 0.0;
    point_right = 0.0;
    xdimsize = 0;
    ydimsize = 0;
    eos5_pixelreg = HE5_HDFE_CENTER;
    eos5_origin = HE5_HDFE_GD_UL;
    eos5_projcode = HE5_GCTP_GEO;
    zone = -1;
    sphere = 0;
    std::fill_n(param, 13, 0);

}

//This method will effectively remove any dimnames like
// ???/XDim or ???/YDim from the dimension name set. 
// Use this function in caution.
void EOS5CFGrid::Update_Dimnamelist()
{

    BESDEBUG("h5", "coming to Update_Dimnamelist" <<endl);

    // If I put both "XDim" and "YDim" into one for loop, Mac g++ compiler 
    // gives segmentation fault, which doesn't make sense.
    // I simply split them into two loops. It doesn't affect performance much.
    // KY 2012-2-14
    for (auto it = this->vardimnames.begin(); it != this->vardimnames.end(); ++it) {
        string xydimname_candidate = HDF5CFUtil::obtain_string_after_lastslash(*it);
        if ("XDim" == xydimname_candidate) {
            this->vardimnames.erase(*it);
            break;
        }
    }

    for (auto it = this->vardimnames.begin(); it != this->vardimnames.end(); ++it) {
        string xydimname_candidate = HDF5CFUtil::obtain_string_after_lastslash(*it);
        if ("YDim" == xydimname_candidate) {
            this->vardimnames.erase(*it);
            break;
        }
    }

}

// A destructor of EOS5File
EOS5File::~EOS5File()
{
    for (vector<EOS5CVar *>::const_iterator i = this->cvars.begin(); i != this->cvars.end(); ++i)
        delete *i;

    for (vector<EOS5CFGrid *>::const_iterator i = this->eos5cfgrids.begin(); i != this->eos5cfgrids.end(); ++i)
        delete *i;

    for (vector<EOS5CFSwath *>::const_iterator i = this->eos5cfswaths.begin(); i != this->eos5cfswaths.end(); ++i)
        delete *i;

    for (vector<EOS5CFZa *>::const_iterator i = this->eos5cfzas.begin(); i != this->eos5cfzas.end(); ++i)
        delete *i;

}

// Helper function to make the name follow the CF conventions.
string EOS5File::get_CF_string(string s)
{

    // We need to remove the first "/" from the full name.
    if (s[0] != '/')
        return File::get_CF_string(s);
    else {
        s.erase(0, 1);
        return File::get_CF_string(s);
    }
}

// Retrieve the HDF5 information for HDF-EOS5 
void EOS5File::Retrieve_H5_Info(const char *file_fullpath, hid_t file_id, bool /*include_attr*/) 
{
    // Since we need to check the attribute info in order to determine if the file is augmented to netCDF-4,
    // we need to retrieve the attribute info also.
    File::Retrieve_H5_Info(file_fullpath, file_id, true);
}

void EOS5File::Retrieve_H5_CVar_Supported_Attr_Values()
{

    for (const auto &cvar:this->cvars) {

        // When the coordinate variables exist in the file, retrieve the attribute values.
        if ((CV_EXIST == cvar->cvartype) || (CV_MODIFY == cvar->cvartype)) {
            for (const auto &attr:cvar->attrs)
                Retrieve_H5_Attr_Value(attr, cvar->fullpath);

        }
    }

}

// Retrieve the attribute values for the HDF-EOS5
void EOS5File::Retrieve_H5_Supported_Attr_Values() 
{

    File::Retrieve_H5_Supported_Attr_Values();
    for (const auto &cvar:this->cvars) {

        // When the coordinate variables exist in the file, retrieve the attribute values.
        if ((CV_EXIST == cvar->cvartype) || (CV_MODIFY == cvar->cvartype)) {
            for (const auto &attr:cvar->attrs)
                Retrieve_H5_Attr_Value(attr, cvar->fullpath);
        }
    }
}

// Adjust attribute value
void EOS5File::Adjust_H5_Attr_Value(const Attribute* ) const
{
    // For future usage.

}

// Handle unsupported datatype
void EOS5File::Handle_Unsupported_Dtype(bool include_attr)
{

    if (true == check_ignored) {
        Gen_Unsupported_Dtype_Info(include_attr);
    }

    File::Handle_Unsupported_Dtype(include_attr);
    Handle_EOS5_Unsupported_Dtype(include_attr);
}

// Handle EOS5 unsupported datatype,add EOS5 coordinate variables
void EOS5File::Handle_EOS5_Unsupported_Dtype(bool include_attr) 
{

    for (auto ircv = this->cvars.begin(); ircv != this->cvars.end();) {
        if (true == include_attr) {
            for (auto ira = (*ircv)->attrs.begin(); ira != (*ircv)->attrs.end();) {
                H5DataType temp_dtype = (*ira)->getType();
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4)) {
                    delete (*ira);
                    ira = (*ircv)->attrs.erase(ira);
                }
                else {
                    ++ira;

                }
            }
        }

        H5DataType temp_dtype = (*ircv)->getType();
        if (!HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4)) {
            delete (*ircv);
            ircv = this->cvars.erase(ircv);
        }
        else {
            ++ircv;
        }
    }
}

// Generate unsupported datatype information
void EOS5File::Gen_Unsupported_Dtype_Info(bool include_attr)
{

    if (true == include_attr) {

        File::Gen_Group_Unsupported_Dtype_Info();
        File::Gen_Var_Unsupported_Dtype_Info();
        Gen_VarAttr_Unsupported_Dtype_Info();

    }

}

// Generate variable attribute datatype info.
void EOS5File::Gen_VarAttr_Unsupported_Dtype_Info() 
{

    // Dimension scale info for general variables
    Gen_DimScale_VarAttr_Unsupported_Dtype_Info();

    // HDF-EOS5 variable attribute unsupported datatype 
    Gen_EOS5_VarAttr_Unsupported_Dtype_Info();

}

void EOS5File::Gen_EOS5_VarAttr_Unsupported_Dtype_Info() 
{

    for (const auto &cvar:this->cvars) {
        // If the attribute REFERENCE_LIST comes with the attribute CLASS, the
        // attribute REFERENCE_LIST is okay to ignore. No need to report.
        bool is_ignored = ignored_dimscale_ref_list(cvar);
        if (false == cvar->attrs.empty()) {
            for (const auto &attr:cvar->attrs) {
                H5DataType temp_dtype = attr->getType();
                // TODO: check why 64-bit integer is included.
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4) || (temp_dtype == H5INT64) ||(temp_dtype == H5UINT64)) {
                    // "DIMENSION_LIST" is okay to ignore and "REFERENCE_LIST"
                    // is okay to ignore if the variable has another attribute
                    // CLASS="DIMENSION_SCALE"
                    if (("DIMENSION_LIST" != attr->name)
                        && ("REFERENCE_LIST" != attr->name || true == is_ignored))
                        this->add_ignored_info_attrs(false, cvar->fullpath, attr->name);
                }
            }
        }
    }
}

// Handle unsupported data space.
void EOS5File::Handle_Unsupported_Dspace(bool include_attr)
{

    // Generate unsupported info.
    if (true == check_ignored) {
        Gen_Unsupported_Dspace_Info();
    }

    File::Handle_Unsupported_Dspace(include_attr);
    Handle_EOS5_Unsupported_Dspace(include_attr);

}

// Handle EOS5 unsupported data space.
void EOS5File::Handle_EOS5_Unsupported_Dspace(bool include_attr) 
{

    if (true == this->unsupported_var_dspace) {
        for (auto ircv = this->cvars.begin(); ircv != this->cvars.end();) {
            if (true == (*ircv)->unsupported_dspace) {
                delete (*ircv);
                ircv = this->cvars.erase(ircv);
            }
            else {
                ++ircv;
            }
        }
    }

    if (true == include_attr) {
        if (true == this->unsupported_var_attr_dspace) {
            for (auto ircv = this->cvars.begin(); ircv != this->cvars.end(); ++ircv) {
                if (false == (*ircv)->attrs.empty()) {
                    if (true == (*ircv)->unsupported_attr_dspace) {
                        for (auto ira = (*ircv)->attrs.begin(); ira != (*ircv)->attrs.end();) {
                            if (0 == (*ira)->count) {
                                delete (*ira);
                                ira = (*ircv)->attrs.erase(ira);
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

// Generating unsupported data space.
void EOS5File::Gen_Unsupported_Dspace_Info()
{

    File::Gen_Unsupported_Dspace_Info();

}

// Handle other unsupported EOS5 information
void EOS5File::Handle_Unsupported_Others(bool include_attr) 
{

    remove_netCDF_internal_attributes(include_attr);
#if 0
    if(true == include_attr) {
        for (auto irv = this->vars.begin();
             irv != this->vars.end(); ++irv) {
                for (auto ira = (*irv)->attrs.begin();
                     ira != (*irv)->attrs.end();) {
                    if((*ira)->name == "CLASS") {
                        string class_value = Retrieve_Str_Attr_Value(*ira,(*irv)->fullpath);

                        // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
                        // "DIMENSION_SCALE", which is 15.
                        if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                            delete((*ira));
                            ira = (*irv)->attrs.erase(ira);
                        }
#if 0
                        else if(1) {// Add a BES key,also delete

                        }
#endif
                        else {
                            ++ira;
                        }
                    }
                    //else if((*ira)->name == "NAME" && 1) {// Add a BES Key later if necessary
                    else if((*ira)->name == "NAME") {// Add a BES Key 
                        string name_value = Retrieve_Str_Attr_Value(*ira,(*irv)->fullpath);
                        if( 0 == name_value.compare(0,(*irv)->name.size(),(*irv)->name)) {
                            delete((*ira));
                            ira =(*irv)->attrs.erase(ira);
                        }
                        else {
                            string netcdf_dim_mark= "This is a netCDF dimension but not a netCDF variable";
                            if( 0 == name_value.compare(0,netcdf_dim_mark.size(),netcdf_dim_mark)) {
                                delete((*ira));
                                ira =(*irv)->attrs.erase(ira);
                            }
                            else {
                                ++ira;
                            }
                        }

                    }
                    else if((*ira)->name == "_Netcdf4Dimid") {
                        delete((*ira));
                        ira =(*irv)->attrs.erase(ira);
                    }

                    else {
                        ++ira;
                    }
                }
            }
#endif
    if(true == include_attr) {
        for (auto irv = this->cvars.begin(); irv != this->cvars.end(); ++irv) {
            for (auto ira = (*irv)->attrs.begin(); ira != (*irv)->attrs.end();) {
                if((*ira)->name == "CLASS") {
                    string class_value = Retrieve_Str_Attr_Value(*ira,(*irv)->fullpath);

                    // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
                    // "DIMENSION_SCALE", which is 15.
                    if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                        delete(*ira);
                        ira = (*irv)->attrs.erase(ira);
                        // Add another block to set a key
                    }
                    else {
                        ++ira;
                    }
                }
                else if((*ira)->name == "NAME") {// Add a BES Key later
                    delete(*ira);
                    ira=(*irv)->attrs.erase(ira);
                    //"NAME" attribute causes the file netCDF-4 failed.
#if 0
                    string name_value = Retrieve_Str_Attr_Value(*ira,(*irv)->fullpath);
                    if( 0 == name_value.compare(0,(*irv)->name.size(),(*irv)->name)) {
                        delete(*ira);
                        ira =(*irv)->attrs.erase(ira);
                    }
                    else {
                        string netcdf_dim_mark= "This is a netCDF dimension but not a netCDF variable";
                        if( 0 == name_value.compare(0,netcdf_dim_mark.size(),netcdf_dim_mark)) {
                            delete(*ira);
                            ira =(*irv)->attrs.erase(ira);
                        }
                        else {
                            ++ira;
                        }
                    }
#endif
                }
                else if((*ira)->name == "_Netcdf4Dimid") {
                    delete(*ira);
                    ira =(*irv)->attrs.erase(ira);
                }

                else {
                    ++ira;
                }
            }
        }
    }
    
 
    // We cannot use the general routine from the base class since
    // the information of ignored ECS metadata variables is transferred
    // to DAS. The ignored ECS metadata variables should not be reported.
    //File::Handle_Unsupported_Others(include_attr);
    if (true == this->check_ignored && true == include_attr) {

        // netCDF Java lifts the string size restriction for attributes. So comment out for the time being. KY 2018/08/10
        if (true == HDF5RequestHandler::get_drop_long_string()) {
#if 0
            for (auto ira = this->root_attrs.begin(); ira != this->root_attrs.end(); ++ira) {
                if (H5FSTRING == (*ira)->dtype || H5VSTRING == (*ira)->dtype) {
                    if ((*ira)->getBufSize() > NC_JAVA_STR_SIZE_LIMIT) {
                        this->add_ignored_droplongstr_hdr();
                        this->add_ignored_grp_longstr_info("/", (*ira)->name);
                    }
                }
            }

            for (auto irg = this->groups.begin(); irg != this->groups.end(); ++irg) {
                for (auto ira = (*irg)->attrs.begin(); ira != (*irg)->attrs.end(); ++ira) {
                    if (H5FSTRING == (*ira)->dtype || H5VSTRING == (*ira)->dtype) {
                        if ((*ira)->getBufSize() > NC_JAVA_STR_SIZE_LIMIT) {
                            this->add_ignored_droplongstr_hdr();
                            this->add_ignored_grp_longstr_info((*irg)->path, (*ira)->name);
                        }
                    }

                }
            }
#endif
            for (auto irv = this->vars.begin(); irv != this->vars.end(); ++irv) {
                if (true == Check_DropLongStr((*irv), nullptr)) {
                    string ecsmeta_grp = "/HDFEOS INFORMATION";
                    // Ignored ECS metadata should not be reported.
                    if ((*irv)->fullpath.find(ecsmeta_grp) != 0
                        || ((*irv)->fullpath.rfind("/") != ecsmeta_grp.size())) {
                        this->add_ignored_droplongstr_hdr();
                        this->add_ignored_var_longstr_info((*irv), nullptr);
                    }
                }
#if 0
                for (auto ira = (*irv)->attrs.begin(); ira != (*irv)->attrs.end(); ++ira) {
                    if (true == Check_DropLongStr((*irv), (*ira))) {
                        this->add_ignored_droplongstr_hdr();
                        this->add_ignored_var_longstr_info((*irv), (*ira));
                    }
                }
#endif
            }
#if 0
            for (auto irv = this->cvars.begin(); irv != this->cvars.end(); ++irv) {
                for (auto ira = (*irv)->attrs.begin(); ira != (*irv)->attrs.end(); ++ira) {
                    if (true == Check_DropLongStr((*irv), (*ira))) {
                        this->add_ignored_droplongstr_hdr();
                        this->add_ignored_var_longstr_info((*irv), (*ira));
                    }
                }
            }
#endif
        }
    }

    if (false == this->have_ignored) this->add_no_ignored_info();

}

// Adjust HDF-EOS5 dimension info.
void EOS5File::Adjust_EOS5Dim_Info(HE5Parser*strmeta_info) 
{

    BESDEBUG("h5", "coming to Adjust_EOS5Dim_Info" <<endl);

    // Condense redundant XDim, YDim in the grid/swath/za dimension list
    for (unsigned int i = 0; i < strmeta_info->swath_list.size(); ++i) {
        HE5Swath& he5s = strmeta_info->swath_list.at(i);

        Adjust_EOS5Dim_List(he5s.dim_list);

        // Correct the possible wrong dimension size,this only happens for the unlimited dimension,
        // WE JUST NEED TO CORRECT the EOS group dimension size. 
        // STEPS:
        // 1. Merge SWATH data_var_list and geo_var_list
        // Function parameters will be the object dim. list(he5s.dim_list), EOS5Type(SWATH,GRID...) and varlist
        // Need to use Obtain_Var_EOS5Type_GroupName to find var's group name and Get_Var_EOS5_Type(var) to find 
        // Var's EOS5Type. 
        // After checking group and type, check "if(he5v.name == var->name)" and change the he5v dim. size to var size.
        if(this->have_udim == true) {
            vector<HE5Var> svlist = he5s.geo_var_list;
            svlist.insert(svlist.end(),he5s.data_var_list.begin(),he5s.data_var_list.end());
            // Only apply when the unlimited dimension is found!! So we don't have to go over this for every file.
            Adjust_EOS5DimSize_List(he5s.dim_list,svlist,SWATH,he5s.name);
        }
 
        for (unsigned int j = 0; j < he5s.geo_var_list.size(); ++j) {
            Adjust_EOS5VarDim_Info((he5s.geo_var_list)[j].dim_list, he5s.dim_list, he5s.name, SWATH);
        }
        for (unsigned int j = 0; j < he5s.data_var_list.size(); ++j) {
            Adjust_EOS5VarDim_Info((he5s.data_var_list)[j].dim_list, he5s.dim_list, he5s.name, SWATH);
        }
    }

    for (unsigned int i = 0; i < strmeta_info->grid_list.size(); ++i) {

        HE5Grid& he5g = strmeta_info->grid_list.at(i);

        Adjust_EOS5Dim_List(he5g.dim_list);

        // Correct possible wrong dimension size in the eosdim list.
        if(this->have_udim == true) {
            // Only apply when the unlimited dimension is found!! So we don't have to go over this for every file.
            Adjust_EOS5DimSize_List(he5g.dim_list,he5g.data_var_list,GRID,he5g.name);
        }
 
        for (unsigned int j = 0; j < he5g.data_var_list.size(); ++j) {
            Adjust_EOS5VarDim_Info((he5g.data_var_list)[j].dim_list, he5g.dim_list, he5g.name, GRID);
        }
    }

    for (unsigned int i = 0; i < strmeta_info->za_list.size(); ++i) {
        HE5Za& he5z = strmeta_info->za_list.at(i);

        Adjust_EOS5Dim_List(he5z.dim_list);

        // Correct possible wrong dimension size in the eosdim list.
        if(this->have_udim == true) {
            // Only apply when the unlimited dimension is found!! So we don't have to go over this for every file.
            Adjust_EOS5DimSize_List(he5z.dim_list,he5z.data_var_list,ZA,he5z.name);
        }
 
        for (unsigned int j = 0; j < he5z.data_var_list.size(); ++j) {
            Adjust_EOS5VarDim_Info((he5z.data_var_list)[j].dim_list, he5z.dim_list, he5z.name, ZA);
        }
    }
}

// Adjust HDF-EOS5 dimension list. 
void EOS5File::Adjust_EOS5Dim_List(vector<HE5Dim>& groupdimlist) const
{

    BESDEBUG("h5", "Coming to Adjust_EOS5Dim_List"<<endl);

    // The negative dimension sizes are found in some HDF-EOS5 files.
    // We need to remove them.
    Remove_NegativeSizeDims(groupdimlist);

    // Condense redundant XDim, YDim in the grid/swath/za dimension list
    Condense_EOS5Dim_List(groupdimlist);

}

//  The negative dimension sizes are found in some HDF-EOS5 files.
//  We need to remove them.
void EOS5File::Remove_NegativeSizeDims(vector<HE5Dim>& groupdimlist) const 
{

    BESDEBUG("h5", "Coming to Remove_NegativeSizeDims" <<endl);

    // We find one product has dimension with name:  Unlimited, size: -1; this dimension
    // will not be used by any variables. The "Unlimited" dimension is useful for extended
    // datasets when data is written. It is not useful for data accessing as far as I know.
    // So we will remove it from the list. 
    // This algorithm will also remove  any dimension with size <=0. KY 2011-1-14
    // Note: Unlimited dimension is supported by the handler but not by using this "Unlimited" name.
    // For the unlimited dimension support, check class Dimension and function Retrieve_H5_VarDim.
    for (auto id = groupdimlist.begin(); id != groupdimlist.end();) {
        if ((*id).size <= 0) {
            id = groupdimlist.erase(id);
        }
        else {
            ++id;
        }
    }
}

//  Condense redundant XDim, YDim in the grid/swath/za dimension list
//  Some products use Xdim rather XDim, Ydim rather than Ydim. 
//  This is significant for grids. We need to make them "XDim" and "YDim".
//  See comments of function Adjust_EOS5VarDim_Info for the reason.
void EOS5File::Condense_EOS5Dim_List(vector<HE5Dim>& groupdimlist) const
{

    BESDEBUG("h5", "Coming to Condense_EOS5Dim_List"<<endl);
    set<int> xdimsizes;
    set<int> ydimsizes;
    pair<set<int>::iterator, bool> setret;
    vector<HE5Dim>::iterator id;

    for (id = groupdimlist.begin(); id != groupdimlist.end();) {
        if ("XDim" == (*id).name || "Xdim" == (*id).name) {
            setret = xdimsizes.insert((*id).size);
            if (false == setret.second) {
                id = groupdimlist.erase(id);
            }
            else if ("Xdim" == (*id).name) {
                (*id).name = "XDim";
                ++id;
            }
            else {
                ++id;
            }

        }
        else {
            ++id;
        }
    }

    for (id = groupdimlist.begin(); id != groupdimlist.end();) {
        if ("YDim" == (*id).name || "Ydim" == (*id).name) {
            setret = ydimsizes.insert((*id).size);
            if (false == setret.second) {
                id = groupdimlist.erase(id);
            }
            else if ("Ydim" == (*id).name) {
                (*id).name = "YDim";
                ++id;
            }
            else {
                ++id;
            }
        }
        else {
            ++id;
        }
    }
}

void EOS5File:: Adjust_EOS5DimSize_List(vector<HE5Dim>& eos5objdimlist,const vector<HE5Var> & eos5objvarlist, 
                                        const EOS5Type eos5type, const string & eos5objname) const
{

    set<string>updated_dimlist;
    pair<set<string>::iterator,bool> set_insert_ret;

    for(unsigned int i = 0; i<eos5objvarlist.size();i++) {
        HE5Var he5v = eos5objvarlist.at(i);
        for(unsigned int j = 0; j<he5v.dim_list.size();j++) {
            HE5Dim he5d = he5v.dim_list.at(j);
            set_insert_ret = updated_dimlist.insert(he5d.name);
            if(set_insert_ret.second == true) {
                // Find out the index of this dimension in eos5objdimlist 
                unsigned int objdimlist_index = 9999;
                bool has_objdimlist_index = false;
                for(unsigned int k = 0; k <eos5objdimlist.size();k++) {
                    if(eos5objdimlist[k].name == he5d.name) {
                        objdimlist_index = k;
                        has_objdimlist_index = true;
                        break;
                    }
                }
                if(has_objdimlist_index == false)
                    throw2("Cannot find the dimension in the EOS5 object dimension list for the dimension ", he5d.name);
                for (const auto &var:this->vars) {

                    EOS5Type vartype = Get_Var_EOS5_Type(var);
                    // Compare the EOS5 object type: SWATH,GRID or ZA
                    // eos5objvarlist only stores the variable name, not the path. So we have to ensure the path matches.
                    if(vartype == eos5type) {
                        string var_eos5gname = Obtain_Var_EOS5Type_GroupName(var,vartype);
                        // Compare the EOS5 object name
                        // Now we need to match the var name from eos5objvarlist with the var name. 
                        if(var_eos5gname == eos5objname) {
                            if(var->name == he5v.name) {
                                if (he5v.dim_list.size() != var->dims.size())
                                    throw2("Number of dimensions don't match with the structmetadata for variable ", var->name);
                                // Change dimension size
                                (eos5objdimlist[objdimlist_index]).size = (var->dims[j])->size;
                                break;
                            }
                            
                        }
                    }
                }
            }

        }
        // Don't need to go over every var, just find enough.
        if(updated_dimlist.size() == eos5objdimlist.size())// Finish updating the eos5objdimlist
            break;
    }
#if 0
for(unsigned int k = 0; k <eos5objdimlist.size();k++) {
    cerr<<"eos5 obj dim name is "<<eos5objdimlist[k].name << " Size is "<< eos5objdimlist[k].size << endl;
}
#endif
}


// Adjust HDF-EOS5 Variable,dimension information.
void EOS5File::Adjust_EOS5VarDim_Info(vector<HE5Dim>& vardimlist, vector<HE5Dim>& groupdimlist,
    const string & eos5_obj_name, EOS5Type eos5type) 
{

    BESDEBUG("h5", "Coming to Adjust_EOS5VarDim_Info"<<endl);
    set<string> dimnamelist;
    pair<set<string>::iterator, bool> setret;

    // For EOS5 Grids: Dimension names XDim and YDim are predefined. 
    // Even the data producers make a mistake to define "xdim", "ydim" etc in the grid
    // dimension name list, the variable will still pick up "XDim" and "YDim" as their
    // dimension names So we assume that 'xdim", "ydim" etc will never appear in the
    // variable name list.  
    for (unsigned int i = 0; i < vardimlist.size(); ++i) {

        HE5Dim& he5d = vardimlist.at(i);
        bool dim_in_groupdimlist = false;
        for (unsigned int j = 0; j < groupdimlist.size(); ++j) {
            HE5Dim he5gd = groupdimlist.at(j);
            if (he5gd.name == he5d.name) {
                he5d.size = he5gd.size;
                dim_in_groupdimlist = true;
                break;
            }
        }

        if (false == dim_in_groupdimlist)
        throw2("The EOS5 group dimension name list doesn't include the dimension ", he5d.name);

        // Some variables have data like float foo[nlevel= 10][nlevel= 10],need to make the dimname unique
        // to ensure the coordinate variables to be generated correctly.
        // 
        setret = dimnamelist.insert(he5d.name);
        if (false == setret.second) {
            int clash_index = 1;
            string temp_clashname = he5d.name + '_';
            HDF5CFUtil::gen_unique_name(temp_clashname, dimnamelist, clash_index);

            string ori_dimname = he5d.name;

            he5d.name = temp_clashname;

            // We have to add this dim. to this  dim. list if this dim doesn't exist in the dim. list.
            bool dim_exist = false;
            for (unsigned int j = 0; j < groupdimlist.size(); ++j) {
                if (he5d.name == groupdimlist[j].name && he5d.size == groupdimlist[j].size) {
                    dim_exist = true;
                    break;
                }
            }

            // Add the new dim. to the dim. list
            if (false == dim_exist) {
                ori_dimname = eos5_obj_name + "/" + ori_dimname;
                string dup_dimname = eos5_obj_name + "/" + he5d.name;
                if (GRID == eos5type) {
                    ori_dimname = "/GRIDS/" + ori_dimname;
                    dup_dimname = "/GRIDS/" + dup_dimname;
                }
                else if (SWATH == eos5type) {
                    ori_dimname = "/SWATHS/" + ori_dimname;
                    dup_dimname = "/SWATHS/" + dup_dimname;
                }
                else if (ZA == eos5type) {
                    ori_dimname = "/ZAS/" + ori_dimname;
                    dup_dimname = "/ZAS/" + dup_dimname;
                }

                // Need to remember the dimname and dupdimname relation in case the situation happens at other variables.
                dimname_to_dupdimnamelist.insert(pair<string, string>(ori_dimname, dup_dimname));
                groupdimlist.push_back(he5d);
            }

        }        //end of if(false == setret.second)
    }        // end of for (unsigned int i = 0; i <vardimlist.size(); ++i)

}

// Add EOS5 FIle information
void EOS5File::Add_EOS5File_Info(HE5Parser * strmeta_info, bool grids_mllcv) 
{

    BESDEBUG("h5", "Coming to Add_EOS5File_Info"<<endl);
    string fslash_str = "/";
    string grid_str = "/GRIDS/";
    string swath_str = "/SWATHS/";
    string za_str = "/ZAS/";

    // Assign the original number of grids. These number will be useful
    // to generate the final DAP object names for grids/swaths/zas that don't have coordinate
    // variables. For example, OMI level 2G product has latitude and longitude with 3-D arrays.
    // There is no way to make the lat/lon become CF coordinate variables.  To still follow the
    // HDF-EOS5 object name conventions, the original number of grid is expected.
    // Since this happens only for grids, we just keep the original number for grids now.
    this->orig_num_grids = (int)(strmeta_info->grid_list.size());

    //
    for (unsigned int i = 0; i < strmeta_info->grid_list.size(); i++) {
        HE5Grid he5g = strmeta_info->grid_list.at(i);
        auto eos5grid_unique = make_unique<EOS5CFGrid>();
        auto eos5grid = eos5grid_unique.release();
        eos5grid->name = he5g.name;
        eos5grid->dimnames.resize(he5g.dim_list.size());

        for (unsigned int j = 0; j < he5g.dim_list.size(); j++) {

            HE5Dim he5d = he5g.dim_list.at(j);
            if ("XDim" == he5d.name) eos5grid->xdimsize = he5d.size;
            if ("YDim" == he5d.name) eos5grid->ydimsize = he5d.size;

            // Here we add the grid name connecting with "/" to
            // adjust the dim names to assure the uniqueness of
            // the dimension names for multiple grids.
            // For single grid, we don't have to do that.
            // However, considering the rare case that one
            // can have one grid, one swath and one za, the dimnames
            // without using the group names may cause the name clashing.
            // so still add the group path.
            string unique_dimname = grid_str + he5g.name + fslash_str + he5d.name;

            (eos5grid->dimnames)[j] = unique_dimname;

            pair<map<hsize_t, string>::iterator, bool> mapret1;
            mapret1 = eos5grid->dimsizes_to_dimnames.insert(pair<hsize_t, string>((hsize_t) he5d.size, unique_dimname));

            // Create the dimname to dimsize map. This will be used to create the missing coordinate
            // variables. Based on our understanding, dimension names should be unique for 
            // grid/swath/zonal average. We will throw an error if we find the same dimension name used.
            pair<map<string, hsize_t>::iterator, bool> mapret2;
            mapret2 = eos5grid->dimnames_to_dimsizes.insert(pair<string, hsize_t>(unique_dimname, (hsize_t) he5d.size));
            if (false == mapret2.second)
            throw5("The dimension name ", unique_dimname, " with the dimension size ", he5d.size, "is not unique");

        } // "for (int j=0; j <he5g.dim_list.size(); j++)"

        // Check if having  Latitude/Longitude. We will use those Latitude and Longitude as CVs if possible.
        EOS5SwathGrid_Set_LatLon_Flags(eos5grid, he5g.data_var_list);

        // Using map for possible the third-D CVs.
        map<string, string> dnames_to_1dvnames;
        EOS5Handle_nonlatlon_dimcvars(he5g.data_var_list, GRID, he5g.name, dnames_to_1dvnames);
        eos5grid->dnames_to_1dvnames = dnames_to_1dvnames;
        eos5grid->point_lower = he5g.point_lower;
        eos5grid->point_upper = he5g.point_upper;
        eos5grid->point_left = he5g.point_left;
        eos5grid->point_right = he5g.point_right;

        eos5grid->eos5_pixelreg = he5g.pixelregistration;
        eos5grid->eos5_origin = he5g.gridorigin;
        eos5grid->eos5_projcode = he5g.projection;

        for (unsigned int k = 0; k < 13; k++)
            eos5grid->param[k] = he5g.param[k];
        eos5grid->zone = he5g.zone;
        eos5grid->sphere = he5g.sphere;

        this->eos5cfgrids.push_back(eos5grid);

    } // "for(int i=0; i < strmeta_info->grid_list.size(); i++)"

    // Adding this here seems a hack. 
    this->grids_multi_latloncvs = grids_mllcv;

    // Second Swath
    for (unsigned int i = 0; i < strmeta_info->swath_list.size(); i++) {

        HE5Swath he5s = strmeta_info->swath_list.at(i);
        auto eos5swath_unique = make_unique<EOS5CFSwath>();
        auto eos5swath = eos5swath_unique.release();
        eos5swath->name = he5s.name;
        eos5swath->dimnames.resize(he5s.dim_list.size());

        for (unsigned int j = 0; j < he5s.dim_list.size(); j++) {

            HE5Dim he5d = he5s.dim_list.at(j);

            // Here we add the swath name connecting with "/" to
            // adjust the dim names to assure the uniqueness of
            // the dimension names for multiple swaths.
            // For single swath, we don't have to do that.
            // However, considering the rare case that one
            // can have one grid, one swath and one za, the dimnames
            // without using the group names may cause the name clashing.
            // so still add the group path.
            string unique_dimname = swath_str + he5s.name + fslash_str + he5d.name;
            (eos5swath->dimnames)[j] = unique_dimname;

            // Create the dimsize to dimname map for those variables missing dimension names.
            // Note: For different dimnames sharing the same dimsizes, we only pick up the first one.
            pair<map<hsize_t, string>::iterator, bool> mapret1;
            mapret1 = eos5swath->dimsizes_to_dimnames.insert(
                pair<hsize_t, string>((hsize_t) he5d.size, unique_dimname));

            // Create the dimname to dimsize map. This will be used to create the missing coordinate
            // variables. Based on our understanding, dimension names should be unique for 
            // grid/swath/zonal average. We will throw an error if we find the same dimension name used.
            pair<map<string, hsize_t>::iterator, bool> mapret2;
            mapret2 = eos5swath->dimnames_to_dimsizes.insert(
                pair<string, hsize_t>(unique_dimname, (hsize_t) he5d.size));
            if (false == mapret2.second)
            throw5("The dimension name ", unique_dimname, " with the dimension size ", he5d.size, "is not unique");

        } // "for (int j=0; j <he5s.dim_list.size(); j++)"

        // Check if having  Latitude/Longitude.
        EOS5SwathGrid_Set_LatLon_Flags(eos5swath, he5s.geo_var_list);

        // Using map for possible the third-D CVs.
        map<string, string> dnames_to_geo1dvnames;
        EOS5Handle_nonlatlon_dimcvars(he5s.geo_var_list, SWATH, he5s.name, dnames_to_geo1dvnames);
        eos5swath->dnames_to_geo1dvnames = dnames_to_geo1dvnames;
        this->eos5cfswaths.push_back(eos5swath);
    } // "for (int i=0; i < strmeta_info->swath_list.size(); i++)"

    // Third Zonal average
    for (unsigned int i = 0; i < strmeta_info->za_list.size(); i++) {

        HE5Za he5z = strmeta_info->za_list.at(i);

        auto eos5za_unique = make_unique<EOS5CFZa>();
        auto eos5za = eos5za_unique.release();
        eos5za->name = he5z.name;
        eos5za->dimnames.resize(he5z.dim_list.size());

        for (unsigned int j = 0; j < he5z.dim_list.size(); j++) {

            HE5Dim he5d = he5z.dim_list.at(j);

            // Here we add the grid name connecting with "/" to
            // adjust the dim names to assure the uniqueness of
            // the dimension names for multiple grids.
            // For single grid, we don't have to do that.
            string unique_dimname = za_str + he5z.name + fslash_str + he5d.name;
            (eos5za->dimnames)[j] = unique_dimname;
            pair<map<hsize_t, string>::iterator, bool> mapret1;
            mapret1 = eos5za->dimsizes_to_dimnames.insert(pair<hsize_t, string>((hsize_t) he5d.size, unique_dimname));

            // Create the dimname to dimsize map. This will be used to create the missing coordinate
            // variables. Based on our understanding, dimension names should be unique for 
            // grid/swath/zonal average. We will throw an error if we find the same dimension name used.
            pair<map<string, hsize_t>::iterator, bool> mapret2;
            mapret2 = eos5za->dimnames_to_dimsizes.insert(pair<string, hsize_t>(unique_dimname, (hsize_t) he5d.size));
            if (false == mapret2.second)
            throw5("The dimension name ", unique_dimname, " with the dimension size ", he5d.size, "is not unique");

        } // "for (int j=0; j <he5z.dim_list.size(); j++) "

        // Using map for possible the third-D CVs.
        map<string, string> dnames_to_1dvnames;
        EOS5Handle_nonlatlon_dimcvars(he5z.data_var_list, ZA, he5z.name, dnames_to_1dvnames);
        eos5za->dnames_to_1dvnames = dnames_to_1dvnames;
        this->eos5cfzas.push_back(eos5za);
    } // "for(int i=0; i < strmeta_info->za_list.size(); i++)"

// Debugging info,leave it here. They are very useful.
#if 0
    for (auto irg = this->eos5cfgrids.begin();
        irg != this->eos5cfgrids.end(); ++irg) {

        cerr<<"grid name "<<(*irg)->name <<endl;
        cerr<<"eos5_pixelreg"<<(*irg)->eos5_pixelreg <<endl;
        cerr<<"eos5_origin"<<(*irg)->eos5_pixelreg <<endl;
        cerr<<"point_lower "<<(*irg)->point_lower <<endl;
        cerr<<"xdimsize "<<(*irg)->xdimsize <<endl;

        if((*irg)->has_g2dlatlon) cerr<<"has g2dlatlon"<<endl;
        if((*irg)->has_2dlatlon) cerr<<"has 2dlatlon"<<endl;
        if((*irg)->has_1dlatlon) cerr<<"has 1dlatlon"<<endl;
        if((*irg)->has_nolatlon) cerr<<"has no latlon" <<endl;
        if(this->grids_multi_latloncvs) cerr<<"having multiple lat/lon from structmeta" <<endl;
        else cerr<<"no multiple lat/lon from structmeta" <<endl;

// Dimension names
        "h5","number of dimensions "<<(*irg)->dimnames.size() <<endl;
        for (auto irv = (*irg)->dimnames.begin();
            irv != (*irg)->dimnames.end(); ++irv)
        cerr<<"dim names" <<*irv <<endl;

// mapping size to name
        for (auto im1 = (*irg)->dimsizes_to_dimnames.begin();
            im1 !=(*irg)->dimsizes_to_dimnames.end();++im1) {
            cerr<<"size to name "<< (int)((*im1).first) <<"=> "<<(*im1).second <<endl;
        }

// mapping dime names to 1d varname
        for (auto im2 = (*irg)->dnames_to_1dvnames.begin();
            im2 !=(*irg)->dnames_to_1dvnames.end();++im2) {
            cerr<<"dimanme to 1d var name "<< (*im2).first <<"=> "<<(*im2).second <<endl;
        }
    }

//Swath
    for (auto irg = this->eos5cfswaths.begin();
        irg != this->eos5cfswaths.end(); ++irg) {

        cerr<<"swath name "<<(*irg)->name <<endl;
        if((*irg)->has_nolatlon) cerr<<"has no latlon" <<endl;
        if((*irg)->has_1dlatlon) cerr<<"has 1dlatlon"<<endl;
        if((*irg)->has_2dlatlon) cerr<<"has 2dlatlon"<<endl;

// Dimension names
        for (auto irv = (*irg)->dimnames.begin();
            irv != (*irg)->dimnames.end(); ++irv)
        cerr<<"dim names" <<*irv <<endl;

// mapping size to name
        for (auto im1 = (*irg)->dimsizes_to_dimnames.begin();
            im1 !=(*irg)->dimsizes_to_dimnames.end();++im1) {
            cerr<<"size to name "<< (int)((*im1).first) <<"=> "<<(*im1).second <<endl;
        }

// mapping dime names to 1d varname
        for (auto im2 = (*irg)->dnames_to_geo1dvnames.begin();
            im2 !=(*irg)->dnames_to_geo1dvnames.end();++im2) {
            cerr<<"dimname to 1d varname "<< (*im2).first <<"=> "<<(*im2).second <<endl;
        }
    }

    for (auto irg = this->eos5cfzas.begin();
        irg != this->eos5cfzas.end(); ++irg) {

        cerr<<"za name now"<<(*irg)->name <<endl;

// Dimension names
        for (auto irv = (*irg)->dimnames.begin();
            irv != (*irg)->dimnames.end(); ++irv)
        cerr<<"dim names" <<*irv <<endl;

// mapping size to name
        for (auto im1 = (*irg)->dimsizes_to_dimnames.begin();
            im1 !=(*irg)->dimsizes_to_dimnames.end();++im1) {
            cerr<<"size to name "<< (int)((*im1).first) <<"=> "<<(*im1).second <<endl;
        }

// mapping dime names to 1d varname
        for (auto im2 = (*irg)->dnames_to_1dvnames.begin();
            im2 !=(*irg)->dnames_to_1dvnames.end();++im2) {
            cerr<<"dimname to 1d varname "<< (*im2).first <<"=> "<<(*im2).second <<endl;
        }
    }
#endif 

}

// Check if EOS5 Swath and Grid hold Latitude and Longitude fields.
template<class T>
void EOS5File::EOS5SwathGrid_Set_LatLon_Flags(T* eos5gridswath, vector<HE5Var> &eos5varlist) const
{

    BESDEBUG("h5", "Coming to EOS5SwathGrid_Set_LatLon_Flags"<<endl);
    bool find_lat = false;
    bool find_lon = false;
    bool has_1dlat = false;
    bool has_1dlon = false;
    bool has_2dlat = false;
    string lat_xdimname;
    string lat_ydimname;
    string lon_xdimname;
    string lon_ydimname;
    bool has_2dlon = false;
    bool has_g2dlat = false;
    bool has_g2dlon = false;

    for (unsigned int i = 0; i < eos5varlist.size(); ++i) {
        HE5Var he5v = eos5varlist.at(i);
        if ("Latitude" == he5v.name) {
            find_lat = true;
            auto num_dims = (int)(he5v.dim_list.size());
            if (1 == num_dims)
                has_1dlat = true;
            else if (2 == num_dims) {
                lat_ydimname = (he5v.dim_list)[0].name;
                lat_xdimname = (he5v.dim_list)[1].name;
                has_2dlat = true;
            }
            else if (num_dims > 2)
                has_g2dlat = true;
            else
                throw1("The number of dimension should not be 0 for grids or swaths");
        } // "if ("Latitude" == he5v.name)"

        if ("Longitude" == he5v.name) {
            find_lon = true;
            auto num_dims = (int)(he5v.dim_list.size());
            if (1 == num_dims)
                has_1dlon = true;
            else if (2 == num_dims) {
                lon_ydimname = (he5v.dim_list)[0].name;
                lon_xdimname = (he5v.dim_list)[1].name;
                has_2dlon = true;
            }
            else if (num_dims > 2)
                has_g2dlon = true;
            else
                throw1("The number of dimension should not be 0 for grids or swaths");
        } // "if ("Longitude" == he5v.name)"

        if (true == find_lat && true == find_lon) {
            if (true == has_1dlat && true == has_1dlon) eos5gridswath->has_1dlatlon = true;

            // Make sure we have lat[YDIM][XDIM] and lon[YDIM][XDIM]
            if (true == has_2dlat && true == has_2dlon && lat_ydimname == lon_ydimname && lat_xdimname == lon_xdimname)
                eos5gridswath->has_2dlatlon = true;

            if (true == has_g2dlat && true == has_g2dlon) eos5gridswath->has_g2dlatlon = true;

            eos5gridswath->has_nolatlon = false;
            break;
        } // "if (true == find_lat && true == find_lon) "
    } // "for (unsigned int i = 0; i < eos5varlist.size(); ++i)"
}

// This function builds up the map from dimension names to coordinate variables
// for non-latitude and longitude fields. 
void EOS5File::EOS5Handle_nonlatlon_dimcvars(vector<HE5Var> & eos5varlist, EOS5Type eos5type, const string &groupname,
    map<string, string>& dnamesgeo1dvnames) const 
{

    BESDEBUG("h5", "Coming to EOS5Handle_nonlatlon_dimcvars"<<endl);

    set<string> nocvdimnames;
    string grid_str = "/GRIDS/";
    string xdim_str = "XDim";
    string ydim_str = "YDim";
    string fslash_str = "/";
    string eos5typestr;

    if (GRID == eos5type) {
        string xdimname = grid_str + groupname + fslash_str + xdim_str;
        nocvdimnames.insert(xdimname);
        string ydimname = grid_str + groupname + fslash_str + ydim_str;
        nocvdimnames.insert(ydimname);
        eos5typestr = "/GRIDS/";
    }
    else if (SWATH == eos5type)
        eos5typestr = "/SWATHS/";
    else if (ZA == eos5type)
        eos5typestr = "/ZAS/";
    else
        throw1("Unsupported HDF-EOS5 type, this type is not swath, grid or zonal average");

    // This assumption is pretty bold. It says: Any 1-D var that has a unique dim. name 
    // in the var list is a 3rd-dim cv. We need to review this as time goes on. KY 2017-10-19
    pair<map<string, string>::iterator, bool> mapret;
    for (unsigned int i = 0; i < eos5varlist.size(); ++i) {
        HE5Var he5v = eos5varlist.at(i);
        if (1 == he5v.dim_list.size()) {
            HE5Dim he5d = he5v.dim_list.at(0);
            string dimname;
            dimname = eos5typestr + groupname + fslash_str + he5d.name;
            string varname; // using the new var name format
            varname = eos5typestr + groupname + fslash_str + he5v.name;
            mapret = dnamesgeo1dvnames.insert(pair<string, string>(dimname, varname));

            // If another geo field already shares the same dimname, we need to
            // disqualify this geofield as the coordinate variable since it is not
            // unique anymore.
            if (false == mapret.second) nocvdimnames.insert(dimname);
        }
    }

    // Manage the coordinate variables. We only want to leave fields that uniquely hold
    // the dimension name to be the possible cv candidate.
    for (auto itset = nocvdimnames.begin(); itset != nocvdimnames.end(); ++itset)
        dnamesgeo1dvnames.erase(*itset);
}

// Adjust variable names after obtain the parsing information.
void EOS5File::Adjust_Var_NewName_After_Parsing() const
{

    BESDEBUG("h5", "Coming to Adjust_Var_NewName_After_Parsing"<<endl);
    for (const auto &var:this->vars) {
        Obtain_Var_NewName(var);
    }
}

void EOS5File::Obtain_Var_NewName(Var *var) const
{

    BESDEBUG("h5", "Coming to Obtain_Var_NewName"<<endl);
    string fslash_str = "/";
    string eos5typestr;

    EOS5Type vartype = Get_Var_EOS5_Type(var);

    // Actually the newname is used to check if we have the existing
    // third dimension coordinate variable. To avoid the check of
    // fullpath again, we will make newname to have the unique information 
    // in the path to identify the objects(Essentially "HDFEOS" is removed). 
    switch (vartype) {
    case GRID: {
        eos5typestr = "/GRIDS/";
        string eos5_groupname = Obtain_Var_EOS5Type_GroupName(var, vartype);
#if 0
        // var->newname = ((1 == num_grids)?var->name:
        //  eos5typestr + eos5_groupname + fslash_str + var->name);
#endif
        var->newname = eos5typestr + eos5_groupname + fslash_str + var->name;
    }
        break;

    case SWATH: {
        eos5typestr = "/SWATHS/";
        string eos5_groupname = Obtain_Var_EOS5Type_GroupName(var, vartype);
#if 0
        //  var->newname = ((1 == num_swaths)?var->name:
        //  eos5typestr + eos5_groupname + fslash_str + var->name);
#endif
        var->newname = eos5typestr + eos5_groupname + fslash_str + var->name;
    }
        break;
    case ZA: {
        eos5typestr = "/ZAS/";
        string eos5_groupname = Obtain_Var_EOS5Type_GroupName(var, vartype);
#if 0
        //  var->newname = ((1 == num_zas)?var->name:
        //  eos5typestr + eos5_groupname + fslash_str + var->name);
#endif
        var->newname = eos5typestr + eos5_groupname + fslash_str + var->name;
    }
        break;
    case OTHERVARS: {
        string eos5infopath = "/HDFEOS INFORMATION";
        if (var->fullpath.size() > eos5infopath.size()) {
            if (eos5infopath == var->fullpath.substr(0, eos5infopath.size())) var->newname = var->name;
        }
        else
            var->newname = var->fullpath;
    }
        break;
    default:
        throw1("Non-supported EOS type");
    } 
}

// Get the HDF-EOS5 type: The type is either grids, swaths or zonal average
EOS5Type EOS5File::Get_Var_EOS5_Type(const Var* var) const
{

    BESDEBUG("h5", "Coming to Get_Var_EOS5_Type"<<endl);

    string EOS5GRIDPATH = "/HDFEOS/GRIDS";
    string EOS5SWATHPATH = "/HDFEOS/SWATHS";
    string EOS5ZAPATH = "/HDFEOS/ZAS";

    if (var->fullpath.size() >= EOS5GRIDPATH.size()) {
        if (EOS5GRIDPATH == var->fullpath.substr(0, EOS5GRIDPATH.size())) return GRID;
    }
    if (var->fullpath.size() >= EOS5SWATHPATH.size()) {
        if (EOS5SWATHPATH == var->fullpath.substr(0, EOS5SWATHPATH.size())) return SWATH;
    }
    if (var->fullpath.size() >= EOS5ZAPATH.size()) {
        if (EOS5ZAPATH == var->fullpath.substr(0, EOS5ZAPATH.size())) return ZA;
    }
    return OTHERVARS;

}

// Add dimension information from the parseing info.
void EOS5File::Add_Dim_Name(HE5Parser *strmeta_info) 
{

    BESDEBUG("h5", "Coming to Add_Dim_Name"<<endl);
    for (const auto &var:this->vars) {
        Obtain_Var_Dims(var, strmeta_info);
#if 0
        for (auto ird = (*irv)->dims.begin();
            ird != (*irv)->dims.end();++ird) {
            cerr<<"dim name right after change "<<(*ird)->newname <<endl;
        }
#endif

    }
}

// CHECK if finding the same variables from the parser.
bool EOS5File::Obtain_Var_Dims(const Var *var, HE5Parser * strmeta_info) 
{

    BESDEBUG("h5", "Coming to Obtain_Var_Dims"<<endl);
    string varname_from_parser;
    EOS5Type vartype = Get_Var_EOS5_Type(var);

    if (GRID == vartype) {

        auto num_grids = (int)(strmeta_info->grid_list.size());

        for (int i = 0; i < num_grids; ++i) {
            HE5Grid he5g = strmeta_info->grid_list.at(i);
            if (he5g.name == Obtain_Var_EOS5Type_GroupName(var, vartype)) {
                EOS5CFGrid *eos5cfgrid = (this->eos5cfgrids)[i];
                bool var_is_parsed = Set_Var_Dims(eos5cfgrid, var, he5g.data_var_list, he5g.name, num_grids, GRID);
                if (false == var_is_parsed) {
                    map<hsize_t, string> dimsizes_to_dimnames = eos5cfgrid->dimsizes_to_dimnames;
                    // Check if this grid includes data fields(variables) that don't have any dimension names.
                    // This rarely happens. But we do find one NASA Aura product that has this problem. Although
                    // this has been fixed, we should anticipiate that the similar problem may happen in the future. 
                    // So check here to avoid the potential problems. KY 2012-1-9
                    Set_NonParse_Var_Dims(eos5cfgrid, var, dimsizes_to_dimnames, num_grids, vartype);
                }
            }
        }

    }
    else if (SWATH == vartype) {

        auto num_swaths = (int)(strmeta_info->swath_list.size());

        for (int i = 0; i < num_swaths; ++i) {

            HE5Swath he5s = strmeta_info->swath_list.at(i);

            if (he5s.name == Obtain_Var_EOS5Type_GroupName(var, vartype)) {

                EOS5CFSwath *eos5cfswath = (this->eos5cfswaths)[i];

                bool var_is_parsed = true;
                int swath_fieldtype_flag = Check_EOS5Swath_FieldType(var);
                if (1 == swath_fieldtype_flag)
                    var_is_parsed = Set_Var_Dims(eos5cfswath, var, he5s.geo_var_list, he5s.name, num_swaths, SWATH);
                else if (0 == swath_fieldtype_flag)
                    var_is_parsed = Set_Var_Dims(eos5cfswath, var, he5s.data_var_list, he5s.name, num_swaths, SWATH);
                else
                    // Neither Geo nor Data(For example, added by the augmentation tool)
                    var_is_parsed = false;

                if (false == var_is_parsed) {
                    map<hsize_t, string> dimsizes_to_dimnames = eos5cfswath->dimsizes_to_dimnames;
                    Set_NonParse_Var_Dims(eos5cfswath, var, dimsizes_to_dimnames, num_swaths, vartype);
                }
            } // end of inner if 
        } // end of for  
    } // end of else if 

    else if (ZA == vartype) {

        auto num_zas = (int)(strmeta_info->za_list.size());

        for (int i = 0; i < num_zas; ++i) {
            HE5Za he5z = strmeta_info->za_list.at(i);
            if (he5z.name == Obtain_Var_EOS5Type_GroupName(var, vartype)) {
                EOS5CFZa *eos5cfza = (this->eos5cfzas)[i];
                bool var_is_parsed = Set_Var_Dims(eos5cfza, var, he5z.data_var_list, he5z.name, num_zas, ZA);
                if (false == var_is_parsed) {
                    map<hsize_t, string> dimsizes_to_dimnames = eos5cfza->dimsizes_to_dimnames;
                    Set_NonParse_Var_Dims(eos5cfza, var, dimsizes_to_dimnames, num_zas, vartype);
                }
            }
        }
    }
    return false;
}

// Set dimension info.(dimension names and sizes) to variables.
template<class T>
bool EOS5File::Set_Var_Dims(T* eos5data, const Var *var, vector<HE5Var> &he5var, const string& groupname, int num_groups,
    EOS5Type eos5type) 
{

    BESDEBUG("h5", "Coming to Set_Var_Dims"<<endl);

    bool is_parsed = false;
    string eos5typestr;
    string fslash_str = "/";

    if (GRID == eos5type)
        eos5typestr = "/GRIDS/";
    else if (SWATH == eos5type)
        eos5typestr = "/SWATHS/";
    else if (ZA == eos5type)
        eos5typestr = "/ZAS/";
    else
        throw1("Unsupported HDF-EOS5 type, this type is not swath, grid or zonal average");

    for (unsigned int i = 0; i < he5var.size(); i++) {

        HE5Var he5v = he5var.at(i);

        if (he5v.name == var->name) {
            if (he5v.dim_list.size() != var->dims.size())
                throw2("Number of dimensions don't match with the structmetadata for variable ", var->name);
            is_parsed = true;

            // Some variables have the same dim. names shared. For examples, we
            // see variables that have int foo[nlevels][nlevels]. To generate the CVs,
            // we have to make the dimension name unique for one variable. So we will
            // change the dimension names. The variable for the same example will be 
            // int foo[nlevels][nlevels_1]. Note this is not required by CF conventions.
            // This is simply due to the missing of the third coordinate variable for some 
            // NASA products. Another way is to totally ignore this kind of variables which
            // we will wait for users' responses.

            // Here is the killer, if different dim. names share the same size,
            // Currently there are no ways to know which dimension name is corresponding to
            // which size. HDF-EOS model gives too much freedom to users. The DimList in
            // the StructMetadata doesn't reflect the order at all. See two example files
            // CH4 in TES-Aura_L3-CH4_r0000010410_F01_07.he5 and NO2DayColumn in
            // HIRDLS-Aura_L3SCOL_v06-00-00-c02_2005d022-2008d077.he5.
            // Fortunately it seems that it doesn't matter for us to make the mapping from
            // dimension names to coordinate variables.
            // KY 2012-1-10

            // Dimension list of some OMI level 2 products doesn't include all dimension name and size
            // pairs. For example, Latitude[1644][60]. We have no way to find the dimension name of
            // the dimension with the size of 1644. The dimension name list of the variable also
            // includes the wrong dimension name. In this case, a dimension with the dimension size =1 
            // is allocated in the latitude's dimension list. The latest version still has this bug.
            // To serve this kind of files, we create a fakedim name for the unmatched size.
            // KY 2012-1-13

            set<hsize_t> dimsize_have_name_set;
            pair<set<hsize_t>::iterator, bool> setret1;
            set<string> thisvar_dimname_set;
            pair<set<string>::iterator, bool> setret2;

            for (unsigned int j = 0; j < he5v.dim_list.size(); j++) {
                HE5Dim he5d = he5v.dim_list.at(j);
                for (const auto &dim:var->dims) {

                    if ((hsize_t) (he5d.size) == dim->size) {
                        // This will assure that the same size dims be assigned to different dims
                        if ("" == dim->name) {
                            string dimname_candidate = eos5typestr + groupname + fslash_str + he5d.name;
                            setret2 = thisvar_dimname_set.insert(dimname_candidate);
                            if (true == setret2.second) {
                                dim->name = dimname_candidate;
                                // Should check in the future if the newname may cause potential inconsistency. KY:2012-3-9
                                dim->newname = (num_groups == 1) ? he5d.name : dim->name;
                                eos5data->vardimnames.insert(dim->name);
                                // Since there is no way to figure out the unlimited dimension info. of an individual variable
                                // from the dimension list. Here we just provide the dimnames to unlimited dimension mapping
                                // based on the variable mapping. KY 2016-02-18
                                eos5data->dimnames_to_unlimited[dim->name] = dim->unlimited_dim;
                            }
                        }
                    }
                }
            } // for (unsigned int j=0; j<he5v.dim_list.size();j++)

            // We have to go through the dimension list of this variable again to assure that every dimension has a name.
            // This is how that FakeDim is added. We still need it just in case. KY 2017-10-19
            for (const auto &dim:var->dims) {
                if ("" == dim->name)
                    Create_Unique_DimName(eos5data, thisvar_dimname_set, dim, num_groups, eos5type);
            }
        } // "if (he5v.name == var->name) "
    } // "for (unsigned int i = 0; i < he5var.size(); i++)"
    return is_parsed;
}

// Create unique dimension names. Se the comments below.
template<class T>
void EOS5File::Create_Unique_DimName(T*eos5data, set<string>& thisvar_dimname_set, Dimension *dim, int num_groups,
    EOS5Type eos5type) 
{

    BESDEBUG("h5", "Coming to Create_Unique_DimName"<<endl);
    map<hsize_t, string>::iterator itmap1;
    map<string, hsize_t>::iterator itmap2;
    pair<set<string>::iterator, bool> setret2;
    itmap1 = (eos5data->dimsizes_to_dimnames).find(dim->size);

    // Even if we find this dimension matches the dimsizes_to_dimnames map, we have to check if the dimension
    // name has been used for this size. This is to make sure each dimension has a unique name in a variable.
    // For example, float foo[100][100] can be float foo[nlevels = 100][nlevels_1 = 100].
    // Step 1: Check if there is a dimension name that matches the size

    if (itmap1 != (eos5data->dimsizes_to_dimnames).end()) {
        string dimname_candidate = (eos5data->dimsizes_to_dimnames)[dim->size];

        // First check local var dimname set
        setret2 = thisvar_dimname_set.insert(dimname_candidate);

        if (false == setret2.second) {

            // Will see if other dimension names have this size
            bool match_some_dimname = Check_All_DimNames(eos5data, dimname_candidate, dim->size);

            if (false == match_some_dimname) {

                // dimname_candidate is updated.
                Get_Unique_Name(eos5data->vardimnames, dimname_candidate);
                thisvar_dimname_set.insert(dimname_candidate);

                // Finally generate a new dimension(new dim. name with a size);Update all information
                Insert_One_NameSizeMap_Element2(eos5data->dimnames_to_dimsizes, eos5data->dimnames_to_unlimited,
                    dimname_candidate, dim->size, dim->unlimited_dim);
                eos5data->dimsizes_to_dimnames.insert(pair<hsize_t, string>(dim->size, dimname_candidate));
                eos5data->dimnames.push_back(dimname_candidate);
            }
        }

        // The final dimname_candidate(perhaps updated) should be assigned to the name of this dimension
        dim->name = dimname_candidate;
        if (num_groups > 1)
            dim->newname = dim->name;
        else {
            string dname = HDF5CFUtil::obtain_string_after_lastslash(dim->name);
            if ("" == dname)
                throw3("The dimension name ", dim->name, " of the variable  is not right");
            else
                dim->newname = dname;
        }
    }

    else { // No dimension names match or close to march this dimension name, we will create a fakedim. 
           // Check Add_One_FakeDim_Name in HDF5CF.cc Fakedimname must be as a string reference.
        string Fakedimname = Create_Unique_FakeDimName(eos5data, eos5type);
        thisvar_dimname_set.insert(Fakedimname);

        // Finally generate a new dimension(new dim. name with a size);Update all information
        Insert_One_NameSizeMap_Element2(eos5data->dimnames_to_dimsizes, eos5data->dimnames_to_unlimited, Fakedimname,
            dim->size, dim->unlimited_dim);
        eos5data->dimsizes_to_dimnames.insert(pair<hsize_t, string>(dim->size, Fakedimname));
        eos5data->dimnames.push_back(Fakedimname);
        dim->name = Fakedimname;
        if (num_groups > 1)
            dim->newname = dim->name;
        else {
            string dname = HDF5CFUtil::obtain_string_after_lastslash(dim->name);
            if ("" == dname)
                throw3("The dimension name ", dim->name, " of the variable  is not right");
            else
                dim->newname = dname;
        }
    }
}

// Check all dim. names to see if this dim. size is used by another dim. name.
template<class T>
bool EOS5File::Check_All_DimNames(T* eos5data, string& dimname, hsize_t dimsize) const
{

    BESDEBUG("h5", "Coming to Check_All_DimNames"<<endl);
    bool ret_flag = false;
    for (map<string, hsize_t>::iterator im = eos5data->dimnames_to_dimsizes.begin();
        im != eos5data->dimnames_to_dimsizes.end(); ++im) {
        // dimname must not be the same one since the same one is rejected.
        if (dimsize == (*im).second && dimname != (*im).first) {
            dimname = (*im).first;
            ret_flag = true;
            break;
        }
    }
    return ret_flag;
}

// Get a unique name.
void EOS5File::Get_Unique_Name(set<string> & nameset, string& dimname_candidate) const 
{

    BESDEBUG("h5", "Coming to Get_Unique_Name"<<endl);
    int clash_index = 1;
    string temp_clashname = dimname_candidate + '_';
    HDF5CFUtil::gen_unique_name(temp_clashname, nameset, clash_index);
    dimname_candidate = temp_clashname;
}

// We may need to generate a unique "fake" dim. name for dimensions that don't have any dimension names.
template<class T>
string EOS5File::Create_Unique_FakeDimName(T*eos5data, EOS5Type eos5type) const
{

    BESDEBUG("h5", "Coming to Create_Unique_FakeDimName"<<endl);
    string fslash_str = "/";
    string eos5typestr;
    if (GRID == eos5type)
        eos5typestr = "/GRIDS/";
    else if (SWATH == eos5type)
        eos5typestr = "/SWATHS/";
    else if (ZA == eos5type)
        eos5typestr = "/ZAS/";
    else
        throw1("Unsupported HDF-EOS5 type, this type is not swath, grid or zonal average");

    stringstream sfakedimindex;
    sfakedimindex << eos5data->addeddimindex;
    string fakedimstr = "FakeDim";
    string added_dimname = eos5typestr + eos5data->name + fslash_str + fakedimstr + sfakedimindex.str();

    pair<set<string>::iterator, bool> setret;
    setret = eos5data->vardimnames.insert(added_dimname);
    if (false == setret.second) Get_Unique_Name(eos5data->vardimnames, added_dimname);
    eos5data->addeddimindex = eos5data->addeddimindex + 1;
    return added_dimname;
}

// Obtain the group name this variable belongs.
string EOS5File::Obtain_Var_EOS5Type_GroupName(const Var*var, EOS5Type eos5type) const 
{

    BESDEBUG("h5", "Coming to Obtain_Var_EOS5Type_GroupName"<<endl);
    string EOS5GRIDPATH = "/HDFEOS/GRIDS";
    string EOS5SWATHPATH = "/HDFEOS/SWATHS";
    string EOS5ZAPATH = "/HDFEOS/ZAS";
    size_t eostypename_start_pos = 0;
    size_t eostypename_end_pos;
    string groupname;

    // The fullpath is like "HDFEOS/GRIDS/Temp/Data Fields/etc."
    // To get "Temp", we obtain the position of "T" and the position of "p"
    // and then generate a substr.

    if (GRID == eos5type)
        eostypename_start_pos = EOS5GRIDPATH.size() + 1;
    else if (SWATH == eos5type)
        eostypename_start_pos = EOS5SWATHPATH.size() + 1;
    else if (ZA == eos5type)
        eostypename_start_pos = EOS5ZAPATH.size() + 1;
    else
        throw2("Non supported eos5 type for var ", var->fullpath);

    eostypename_end_pos = var->fullpath.find('/', eostypename_start_pos) - 1;
    groupname = var->fullpath.substr(eostypename_start_pos, eostypename_end_pos - eostypename_start_pos + 1);

    BESDEBUG("h5", "In Obtain_Var_EOS5Type_GroupName(), the groupname is "<<groupname << endl);

    return groupname;
}

// Check whether this field belongs to "Geolocation Fields" or "Data Fields"
int EOS5File::Check_EOS5Swath_FieldType(const Var*var) const
{

    string geofield_relative_path = "/Geolocation Fields/" + var->name;
    string datafield_relative_path = "/Data Fields/" + var->name;

    int tempflag = -1;

    if (var->fullpath.size() > datafield_relative_path.size()) {
        size_t field_pos_in_full_path = var->fullpath.size() - datafield_relative_path.size();
        if (var->fullpath.rfind(datafield_relative_path, field_pos_in_full_path) != string::npos) tempflag = 0;
    }

    if (tempflag != 0 && (var->fullpath.size() > geofield_relative_path.size())) {
        size_t field_pos_in_full_path = var->fullpath.size() - geofield_relative_path.size();
        if (var->fullpath.rfind(geofield_relative_path, field_pos_in_full_path) != string::npos) tempflag = 1;
    }
    return tempflag;
}

// An error will be thrown if we find a dimension size that doesn't match any dimension name
// in this EOS5 group.
template<class T>
void EOS5File::Set_NonParse_Var_Dims(T*eos5data, const Var* var, const map<hsize_t, string>& /*dimsizes_to_dimnames*/,
    int num_groups, EOS5Type eos5type) 
{

    BESDEBUG("h5", "Coming to Set_NonParse_Var_Dims"<<endl);
    map<hsize_t, string>::iterator itmap;
    set<string> thisvar_dimname_set;

    for (const auto &vdim:var->dims) {
        if ("" == vdim->name)
            Create_Unique_DimName(eos5data, thisvar_dimname_set, vdim, num_groups, eos5type);
        else
            throw5("The dimension name ", vdim->name, " of the variable ", var->name, " is not right");
    }
}

// Aura files don't use the CF attribute names for the bunch of attributes. We need to make it right.
void EOS5File::Check_Aura_Product_Status() 
{

    BESDEBUG("h5", "Coming to Check_Aura_Product_Status"<<endl);
    // Aura files will put an attribute called InStrumentName under /HDFEOS/ADDITIONAL/FILE_ATTRIBUTES
    // We just need to check that attribute.
    string eos5_fattr_group_name = "/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES";
    string instrument_attr_name = "InstrumentName";

    // Check if this file is an aura file
    for (const auto &grp:this->groups) {
        if (eos5_fattr_group_name == grp->path) {
            for (const auto &attr:grp->attrs) {
                if (instrument_attr_name == attr->name) {
                    Retrieve_H5_Attr_Value(attr, grp->path);
                    string attr_value(attr->value.begin(), attr->value.end());
                    if ("OMI" == attr_value) {
                        this->isaura = true;
                        this->aura_name = OMI;
                    }
                    else if ("MLS Aura" == attr_value) {
                        this->isaura = true;
                        this->aura_name = MLS;
                    }
                    else if ("TES" == attr_value) {
                        this->isaura = true;
                        this->aura_name = TES;
                    }
                    else if ("HIRDLS" == attr_value) {
                        this->isaura = true;
                        this->aura_name = HIRDLS;
                    }
                    break;
                }
            }
        }
    }

    // Assign EOS5 to CF MAP values for Aura files
    if (true == this->isaura) {
        eos5_to_cf_attr_map["FillValue"] = "_FillValue";
        eos5_to_cf_attr_map["MissingValue"] = "missing_value";
        eos5_to_cf_attr_map["Units"] = "units";
        eos5_to_cf_attr_map["Offset"] = "add_offset";
        eos5_to_cf_attr_map["ScaleFactor"] = "scale_factor";
        eos5_to_cf_attr_map["ValidRange"] = "valid_range";
        eos5_to_cf_attr_map["Title"] = "title";
    }

}

// Handle Coordinate variables
void EOS5File::Handle_CVar() 
{

    BESDEBUG("h5", "Coming to Handle_CVar()"<<endl);

    // If this file is augmented.
    bool is_augmented = Check_Augmentation_Status();

#if 0
    if(is_augmented) cerr<<"The file is augmented "<<endl;
    else cerr<<"The file is not augmented "<<endl;
#endif

    // Handle coordinate variables for grids.
    if (this->eos5cfgrids.empty() == false)
        Handle_Grid_CVar(is_augmented);
    if (this->eos5cfswaths.empty() == false) 
        Handle_Swath_CVar(is_augmented);
    if (this->eos5cfzas.empty() == false) 
        Handle_Za_CVar(is_augmented);

#if 0
    for (auto irv = this->cvars.begin();
        irv != this->cvars.end(); irv++) {
        cerr<<"EOS5CVar name "<<(*irv)->name <<endl;
        cerr<<"EOS5CVar dimension name "<< (*irv)->cfdimname <<endl;
        cerr<<"EOS5CVar new name "<<(*irv)->newname <<endl;
        cerr<<"EOS5CVar type is  "<<(*irv)->cvartype <<endl;
//cerr<<"EOS5CVar dtype is "<<(*irv)->dtype <<endl;
    }
#endif

}

// Handle Grid Coordinate variables
void EOS5File::Handle_Grid_CVar(bool is_augmented) 
{

    BESDEBUG("h5", "Coming to Handle_Grid_CVar"<<endl);
    if (true == is_augmented) {
        // Create latitude/longitude based on the first XDim and YDim
        Handle_Augmented_Grid_CVar();
    }
    else {
        Remove_MultiDim_LatLon_EOS5CFGrid();
        // If the grid size is 0, it must be a Grid file that cannot be handled
        // with the CF option, simply return with handling any coordinate variables.
        if (this->eos5cfgrids.empty()) return;
        if (1 == this->eos5cfgrids.size())
            Handle_Single_Nonaugment_Grid_CVar((this->eos5cfgrids)[0]);
        else
            Handle_Multi_Nonaugment_Grid_CVar();
    }
}

// Check if this file is augmented. The current augmentation tool will
// add extra variables for every EOS5 object. This function will check
// if that is the case.
bool EOS5File::Check_Augmentation_Status() const
{

    BESDEBUG("h5", "Coming to Check_Augmentation_Status()"<<endl);
    bool aug_status = false;
    int num_aug_eos5grp = 0;

    for (const auto &cfgrid:this->eos5cfgrids) {
        for (const auto &var:this->vars) {
            bool is_augmented = Check_Augmented_Var_Candidate(cfgrid, var, GRID);
            if (true == is_augmented) {
                num_aug_eos5grp++;
                break;
            }
        }
    }

    for (const auto &cfswath:this->eos5cfswaths) {
        for (const auto &var:this->vars) {
            bool is_augmented = Check_Augmented_Var_Candidate(cfswath, var, SWATH);
            if (true == is_augmented) {
                num_aug_eos5grp++;
                break;
            }

        }
    }

    for (const auto &cfza:this->eos5cfzas) {
        for (const auto &var:this->vars) {
            bool is_augmented = Check_Augmented_Var_Candidate(cfza, var, ZA);
            if (true == is_augmented) {
                num_aug_eos5grp++;
                break;
            }
        }
    }

    int total_num_eos5grp = this->eos5cfgrids.size() + this->eos5cfswaths.size() + this->eos5cfzas.size();

#if 0
//cerr<< "total_num_eos5grp "<<total_num_eos5grp <<endl;
//"h5","num_aug_eos5grp "<< num_aug_eos5grp <<endl;
#endif 

    if (num_aug_eos5grp == total_num_eos5grp) aug_status = true;
    return aug_status;

}

// This method is not used. Still keep it now since it may be useful in the future. KY 2012-3-09
// Don't remove the #if 0 #endif block. 
#if 0
bool EOS5File::Check_Augmented_Var_Attrs(Var *var)  {

    // We will check whether the attribute "CLASS" and the attribute "REFERENCE_LIST" exist.
    // For the attribute "CLASS", we would like to check if the value is "DIMENSION_SCALE".
    bool has_dimscale_class = false;
    bool has_reflist = false;
    for (auto ira = var->attrs.begin(); ira != var->attrs.end(); ++ira) {
        if ("CLASS" == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,var->fullpath);
            string class_value((*ira)->value.begin(),(*ira)->value.end());
            if ("DIMENSION_SCALE"==class_value)
            has_dimscale_class = true;
        }

        if ("REFERENCE_LIST" == (*ira)->name)
        has_reflist = true;
        if (true == has_reflist && true == has_dimscale_class)
        break;
    }

    if (true == has_reflist && true == has_dimscale_class)
    return true;
    else
    return false;

}
#endif

// Check if the variable candidate exists for the augmented case.
// The augmented variables have path like /HDFEOS/GRIDS/HIRDLS/nTimes
// The general HDF-EOS5 variables have path like /HDFEOS/GRIDS/HIRDLS/Data Fields/Times.
// So if we find the var name is the same as the string stripped from /HDFEOS/GRIDS/HIRDLS, 
// then this file is augmented.
// Hope that no other hybrid-HDFEOS5 files fall to this category.
template<class T>
bool EOS5File::Check_Augmented_Var_Candidate(T *eos5data, const Var *var, EOS5Type eos5type) const
{

    BESDEBUG("h5", "Coming to Check_Augmented_Var_Candidate"<<endl);
    bool augmented_var = false;

    string EOS5DATAPATH;
    if (GRID == eos5type)
        EOS5DATAPATH = "/HDFEOS/GRIDS/";
    else if (ZA == eos5type)
        EOS5DATAPATH = "/HDFEOS/ZAS/";
    else if (SWATH == eos5type)
        EOS5DATAPATH = "/HDFEOS/SWATHS/";
    else
        throw1("Non supported EOS5 type");

    string fslash_str = "/";
    string THIS_EOS5DATAPATH = EOS5DATAPATH + eos5data->name + fslash_str;

    // Match the EOS5 type
    if (eos5type == Get_Var_EOS5_Type(var)) {
        string var_eos5data_name = Obtain_Var_EOS5Type_GroupName(var, eos5type);
        // Match the EOS5 group name
        if (var_eos5data_name == eos5data->name) {
            if (var->fullpath.size() > THIS_EOS5DATAPATH.size()) {
                // Obtain the var name from the full path
                string var_path_after_eos5dataname = var->fullpath.substr(THIS_EOS5DATAPATH.size());
                // Match the variable name
                if (var_path_after_eos5dataname == var->name) augmented_var = true;
            }
        }
    }

    return augmented_var;

}

// Handle augmented grid coordinate variables.
void EOS5File::Handle_Augmented_Grid_CVar() 
{
    BESDEBUG("h5", "Coming to Handle_Augmented_Grid_CVar()"<<endl);
    for (const auto &cfgrid:this->eos5cfgrids)
        Handle_Single_Augment_CVar(cfgrid, GRID);
}

// Handle the coordinate variables for the single HDF-EOS5 objects(grid,swath,zonal average) for an augmented file
template<class T>
void EOS5File::Handle_Single_Augment_CVar(T* cfeos5data, EOS5Type eos5type) 
{

    BESDEBUG("h5", "Coming to Handle_Single_Augment_CVar()"<<endl);
    set<string> tempvardimnamelist;
    tempvardimnamelist = cfeos5data->vardimnames;
    set<string>::iterator its;
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {
        for (auto irv = this->vars.begin(); irv != this->vars.end();) {

            bool is_augmented = Check_Augmented_Var_Candidate(cfeos5data, *irv, eos5type);

            if (true == is_augmented) {

                // Since we have already checked if this file is augmented or not, we can safely
                // compare the dimension name with the var name now.
                string tempdimname = HDF5CFUtil::obtain_string_after_lastslash(*its);

                // The added variable name is always the same as the dimension name.
                if (tempdimname == (*irv)->name) {

                    //Find it, create a coordinate variable.
                    auto EOS5cvar_unique = make_unique<EOS5CVar>(*irv);
                    auto EOS5cvar = EOS5cvar_unique.release();

                    // Still keep the original dimension name to avoid the name clashing when
                    // one grid and one swath and one za occur in the same file
                    EOS5cvar->cfdimname = *its;
                    EOS5cvar->cvartype = CV_EXIST;
                    EOS5cvar->eos_type = eos5type;

                    // Save this cv to the cv vector
                    this->cvars.push_back(EOS5cvar);

                    // Remove this var from the var vector since it becomes a cv.
                    delete (*irv);
                    irv = this->vars.erase(irv);
                }
                else {
                    ++irv;
                }
            } // "if (true == is_augmented)"
            else {
                ++irv;
            }
        } // end of for (auto irv = this->vars.begin();....
    } // end of for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)

    for (const auto &cvar:this->cvars) {
        its = tempvardimnamelist.find(cvar->cfdimname);
        if (its != tempvardimnamelist.end()) tempvardimnamelist.erase(its);
    }

    if (false == tempvardimnamelist.empty())
        throw1("Augmented files still need to provide more coordinate variables");
}

//Currently we remove HDF-EOS5 grid if we find the latitude/longitude is >2D. This is a big question mark
// given some data producers just don't follow the HDF-EOS5 specification to generate the latitude/longitude.
// KY 2016-07-12
void EOS5File::Remove_MultiDim_LatLon_EOS5CFGrid() 
{

    BESDEBUG("h5", "Coming to Remove_MultiDim_LatLon_EOS5CFGrid()"<<endl);
    for (auto irg = this->eos5cfgrids.begin(); irg != this->eos5cfgrids.end();) {

        // If number of dimension latitude/longitude is >=2, no coordinate variables will be generated.
        // We will simply remove this grid from the vector eos5cfgrids.
        // In the future, we may consider supporting 2D latlon. KY 2012-1-17
        // I just find that new OMI level 3 data provide 2D lat/lon for geographic projection data.
        // The 2D lat/lon can be condensed to 1D lat/lon, which is the same calculated by the calculation of
        // the projection. So I don't remove this OMI grid from the grid list. KY 2012-2-9
        // However, I do remove the "Longitude" and "Latitude" fields since "Latitude" and "Longitude"
        // can be calculated. 

        bool irg_erase = false;

        if (true == (*irg)->has_2dlatlon) {

            if ((true == this->isaura) && (OMI == this->aura_name) && (HE5_GCTP_GEO == (*irg)->eos5_projcode))

            {        // We need to remove the redundant latitude and longitude fields

                string EOS5GRIDPATH = "/HDFEOS/GRIDS/";
                string fslash_str = "/";
                string THIS_EOS5GRIDPATH = EOS5GRIDPATH + (*irg)->name + fslash_str;
                int catch_latlon = 0;

                for (auto irv = this->vars.begin(); (irv != this->vars.end()) && (catch_latlon != 2);
                    ) {
                    if (GRID == Get_Var_EOS5_Type(*irv) && ((*irv)->fullpath.size() > THIS_EOS5GRIDPATH.size())) {

                        string var_grid_name = Obtain_Var_EOS5Type_GroupName(*irv, GRID);
                        if (var_grid_name == (*irg)->name) {
                            if (("Longitude" == (*irv)->name) || ("Latitude" == (*irv)->name)) {
                                catch_latlon++;
                                // Remove this var from the var vector since it becomes a cv.
                                delete (*irv);
                                irv = this->vars.erase(irv);
                            }
                            else {
                                ++irv;
                            }
                        }
                        else {
                            ++irv;
                        }
                    }
                    else {
                        ++irv;
                    }
                } //  "for (auto irv = this->vars.begin() ..."
                if (2 == catch_latlon) {
                    (*irg)->has_nolatlon = true;
                    (*irg)->has_2dlatlon = false;
                }

            } // "if ((true == this->isaura) ..."
            else { // remove this grid from the eos5cfgrids list.
                delete (*irg);
                irg = this->eos5cfgrids.erase(irg);
                irg_erase = true;
            }
        } // "if (true  == (*irg) ..."

        if (false == irg_erase) {
            ++irg;
        }

    } // "for (vector <EOS5CFGrid *>::iterator irg = this->eos5cfgrids.begin() ..."

    // Also remove >2d latlon grids.
    for (auto irg = this->eos5cfgrids.begin(); irg != this->eos5cfgrids.end();) {

        if (true == (*irg)->has_g2dlatlon) {
            delete (*irg);
            irg = this->eos5cfgrids.erase(irg);
        }
        else {
            ++irg;
        }
    } 
}

// Handle single nonaugmented grid coordinate variables.
void EOS5File::Handle_Single_Nonaugment_Grid_CVar(EOS5CFGrid* cfgrid) 
{

    BESDEBUG("h5", "Coming to Handle_Single_Nonaugment_Grid_CVar()"<<endl);
    set<string> tempvardimnamelist;
    tempvardimnamelist = cfgrid->vardimnames;

    // Handle Latitude and longitude
    bool use_own_latlon = false;
    if (true == cfgrid->has_1dlatlon)
        use_own_latlon = Handle_Single_Nonaugment_Grid_CVar_OwnLatLon(cfgrid, tempvardimnamelist);
#if 0
    if(use_own_latlon) "h5","using 1D latlon"<<endl;
    else "h5","use_own_latlon is false "<<endl;
#endif

    if (false == use_own_latlon) {
        bool use_eos5_latlon = false;
        use_eos5_latlon = Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon(cfgrid, tempvardimnamelist);

        // If we cannot obtain lat/lon from the HDF-EOS5 library, no need to create other CVs. Simply return.
        if (false == use_eos5_latlon) return;
    }

    // Else handling non-latlon grids
    Handle_NonLatLon_Grid_CVar(cfgrid, tempvardimnamelist);

}

// Handle single nonaugmented grid coordinate variables with its own lat/lon
bool EOS5File::Handle_Single_Nonaugment_Grid_CVar_OwnLatLon(const EOS5CFGrid *cfgrid, set<string>& tempvardimnamelist)
    
{

    BESDEBUG("h5", "Coming to Handle_Single_Nonaugment_Grid_CVar_OwnLatLon()"<<endl);
    set<string>::iterator its;
    string EOS5GRIDPATH = "/HDFEOS/GRIDS/";
    string fslash_str = "/";
    string THIS_EOS5GRIDPATH = EOS5GRIDPATH + cfgrid->name + fslash_str;

    // Handle latitude and longitude
    bool find_latydim = false;
    bool find_lonxdim = false;

    for (auto irv = this->vars.begin(); irv != this->vars.end(); ++irv) {
        if (GRID == Get_Var_EOS5_Type(*irv) && ((*irv)->fullpath.size() > THIS_EOS5GRIDPATH.size())) {

            string var_grid_name = Obtain_Var_EOS5Type_GroupName(*irv, GRID);
            if ((var_grid_name == cfgrid->name) && ((*irv)->name == "Latitude")) {

                string tempdimname = (((*irv)->dims)[0])->name;

                if ("YDim" == HDF5CFUtil::obtain_string_after_lastslash(tempdimname)) {
                    //Find it, create a coordinate variable.
                    auto EOS5cvar_unique = make_unique<EOS5CVar>(*irv);
                    auto EOS5cvar = EOS5cvar_unique.release();

                    // Still keep the original dimension name to avoid the nameclashing when
                    // one grid and one swath and one za occur in the same file
                    EOS5cvar->cfdimname = tempdimname;
                    EOS5cvar->cvartype = CV_EXIST;
                    EOS5cvar->eos_type = GRID;

                    // Save this cv to the cv vector
                    this->cvars.push_back(EOS5cvar);

                    // Remove this var from the var vector since it becomes a cv.
                    delete (*irv);
                    this->vars.erase(irv);

                    // No need to remove back the iterator since it will go out of the loop.
                    find_latydim = true;
                    break;
                } // <if ("YDim" == HDF5CFUtil::obtain_string_after_lastslash(tempdimname))>
            } // <if ((var_grid_name == cfgrid->name) && ((*irv)->name == "Latitude"))>
        } // <if (GRID == Get_Var_EOS5_Type(*irv) ...>
    } // <for (auto irv = this->vars.begin() ...>

    for (auto irv = this->vars.begin(); irv != this->vars.end(); ++irv) {

        if (GRID == Get_Var_EOS5_Type(*irv) && ((*irv)->fullpath.size() > THIS_EOS5GRIDPATH.size())) {

            string var_grid_name = Obtain_Var_EOS5Type_GroupName(*irv, GRID);

            if ((var_grid_name == cfgrid->name) && ((*irv)->name == "Longitude")) {

                string tempdimname = (((*irv)->dims)[0])->name;

                if ("XDim" == HDF5CFUtil::obtain_string_after_lastslash(tempdimname)) {
                    //Find it, create a coordinate variable.
                    auto EOS5cvar_unique = make_unique<EOS5CVar>(*irv);
                    auto EOS5cvar = EOS5cvar_unique.release();

                    // Still keep the original dimension name to avoid the name clashing when
                    // one grid and one swath and one za occur in the same file
                    EOS5cvar->cfdimname = tempdimname;
                    EOS5cvar->cvartype = CV_EXIST;
                    EOS5cvar->eos_type = GRID;

                    // Save this cv to the cv vector
                    this->cvars.push_back(EOS5cvar);

                    // Remove this var from the var vector since it becomes a cv.
                    delete (*irv);
                    this->vars.erase(irv);
                    find_lonxdim = true;
                    break;
                } // <if ("XDim" == HDF5CFUtil::obtain_string_after_lastslash(tempdimname))>
            } // "if ((var_grid_name == cfgrid->name) && ((*irv)->name == "Longitude"))"
        } // <if (GRID == Get_Var_EOS5_Type(*irv) ...>
    } // for (auto irv = this->vars.begin() ...

    for (const auto &cvar:this->cvars) {

        its = tempvardimnamelist.find(cvar->cfdimname);
        if (its != tempvardimnamelist.end()) tempvardimnamelist.erase(its);

    }

#if 0
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)
    "h5","tempvardim "<<*its <<endl;
#endif

    return (find_latydim == true && find_lonxdim == true);
}

// Handle single non-augmented grid latitude/longitude coordinate variables.
bool EOS5File::Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon(const EOS5CFGrid *cfgrid, set<string>& tempvardimnamelist)
    
{

    BESDEBUG("h5", "Coming to Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon()"<<endl);

    // Handle latitude and longitude
    bool find_ydim = false;
    bool find_xdim = false;
    set<string>::iterator its;

#if 0
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)
    cerr<<"dim names "<<(*its) <<endl;
#endif

    string ydim_full_path;
    string xdim_full_path;

    if (cfgrid->eos5_projcode != HE5_GCTP_GEO) {

        for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {
            if ("YDim" == HDF5CFUtil::obtain_string_after_lastslash(*its))
                ydim_full_path = *its;
            else if ("XDim" == HDF5CFUtil::obtain_string_after_lastslash(*its)) xdim_full_path = *its;
        }
    }


    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end();) {
        if ("YDim" == HDF5CFUtil::obtain_string_after_lastslash(*its)) {

            // Create EOS5 Latitude CV
            auto EOS5cvar_unique = make_unique<EOS5CVar>();
            auto EOS5cvar = EOS5cvar_unique.release();
            EOS5cvar->name = "lat";
            Create_Added_Var_NewName_FullPath(GRID, cfgrid->name, EOS5cvar->name, EOS5cvar->newname,
                EOS5cvar->fullpath);

            if (cfgrid->eos5_projcode != HE5_GCTP_GEO) {
                EOS5cvar->rank = 2;
                EOS5cvar->dtype = H5FLOAT64;
            }
            else {
                EOS5cvar->rank = 1;
                EOS5cvar->dtype = H5FLOAT32;
            }

            auto eos5cvar_dim_unique = make_unique<Dimension>((hsize_t) cfgrid->ydimsize);
            auto eos5cvar_dim = eos5cvar_dim_unique.release();
            eos5cvar_dim->name = *its;
            eos5cvar_dim->newname = (this->eos5cfgrids.size() == 1) ? "YDim" : *its;
            EOS5cvar->dims.push_back(eos5cvar_dim);
            EOS5cvar->cfdimname = eos5cvar_dim->name;

            if (EOS5cvar->rank == 2) {

                auto eos5cvar_dim2_unique = make_unique<Dimension>((hsize_t) cfgrid->xdimsize);
                auto eos5cvar_dim2 = eos5cvar_dim2_unique.release();
                eos5cvar_dim2->name = xdim_full_path;
                eos5cvar_dim2->newname = (this->eos5cfgrids.size() == 1) ? "XDim" : xdim_full_path;
                EOS5cvar->dims.push_back(eos5cvar_dim2);

            }
            EOS5cvar->cvartype = CV_LAT_MISS;
            EOS5cvar->eos_type = GRID;
            EOS5cvar->xdimsize = cfgrid->xdimsize;
            EOS5cvar->ydimsize = cfgrid->ydimsize;

            //Special parameters for EOS5 Grid
            EOS5cvar->point_lower = cfgrid->point_lower;
            EOS5cvar->point_upper = cfgrid->point_upper;
            EOS5cvar->point_left = cfgrid->point_left;
            EOS5cvar->point_right = cfgrid->point_right;
            EOS5cvar->eos5_pixelreg = cfgrid->eos5_pixelreg;
            EOS5cvar->eos5_origin = cfgrid->eos5_origin;
            EOS5cvar->eos5_projcode = cfgrid->eos5_projcode;

            for (unsigned int k = 0; k < 13; k++)
                EOS5cvar->param[k] = cfgrid->param[k];

            EOS5cvar->zone = cfgrid->zone;
            EOS5cvar->sphere = cfgrid->sphere;

            // Save this cv to the cv vector
            this->cvars.push_back(EOS5cvar);
            // erase the dimension name from the dimension name set

            // This is the right way to make its platform-independent.
            tempvardimnamelist.erase(its++);
            find_ydim = true;

        } // <if ("YDim" == HDF5CFUtil::obtain_string_after_lastslash(*its))>
        else if ("XDim" == HDF5CFUtil::obtain_string_after_lastslash(*its)) {

            // Create EOS5 Latitude CV
            auto EOS5cvar_unique = make_unique<EOS5CVar>();
            auto EOS5cvar = EOS5cvar_unique.release();
            EOS5cvar->name = "lon";
            Create_Added_Var_NewName_FullPath(GRID, cfgrid->name, EOS5cvar->name, EOS5cvar->newname,
                EOS5cvar->fullpath);

            if (cfgrid->eos5_projcode != HE5_GCTP_GEO) {
                EOS5cvar->rank = 2;
                EOS5cvar->dtype = H5FLOAT64;
            }
            else {
                EOS5cvar->rank = 1;
                EOS5cvar->dtype = H5FLOAT32;
            }


            if (EOS5cvar->rank == 2) {
                auto eos5cvar_dim_unique = make_unique<Dimension>((hsize_t) cfgrid->ydimsize);
                auto eos5cvar_dim = eos5cvar_dim_unique.release();
                eos5cvar_dim->name = ydim_full_path;
                eos5cvar_dim->newname = (this->eos5cfgrids.size() == 1) ? "YDim" : ydim_full_path;
                EOS5cvar->dims.push_back(eos5cvar_dim);
            }

            auto eos5cvar_dim2_unique = make_unique<Dimension>((hsize_t) cfgrid->xdimsize);
            auto eos5cvar_dim2 = eos5cvar_dim2_unique.release();

            eos5cvar_dim2->name = *its;
            eos5cvar_dim2->newname = (this->eos5cfgrids.size() == 1) ? "XDim" : *its;
            EOS5cvar->dims.push_back(eos5cvar_dim2);
            EOS5cvar->cfdimname = eos5cvar_dim2->name;

            EOS5cvar->cvartype = CV_LON_MISS;
            EOS5cvar->eos_type = GRID;
            EOS5cvar->xdimsize = cfgrid->xdimsize;
            EOS5cvar->ydimsize = cfgrid->ydimsize;

            //Special parameters for EOS5 Grid
            EOS5cvar->point_lower = cfgrid->point_lower;
            EOS5cvar->point_upper = cfgrid->point_upper;
            EOS5cvar->point_left = cfgrid->point_left;
            EOS5cvar->point_right = cfgrid->point_right;
            EOS5cvar->eos5_pixelreg = cfgrid->eos5_pixelreg;
            EOS5cvar->eos5_origin = cfgrid->eos5_origin;
            EOS5cvar->eos5_projcode = cfgrid->eos5_projcode;
            for (unsigned int k = 0; k < 13; k++)
                EOS5cvar->param[k] = cfgrid->param[k];
            EOS5cvar->zone = cfgrid->zone;
            EOS5cvar->sphere = cfgrid->sphere;

            // Save this cv to the cv vector
            this->cvars.push_back(EOS5cvar);

            // erase the dimension name from the dimension name set,platform independent way.
            tempvardimnamelist.erase(its++);
            find_xdim = true;

        } // "else if ("XDim" == HDF5CFUtil::obtain_string_after_lastslash(*its))"
        else
            ++its;
        if (true == find_xdim && true == find_ydim) break;
    } // <for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)>

    return (true == find_xdim && true == find_ydim);
}

// Handle non-latitude/longitude grid coordinate variables.
void EOS5File::Handle_NonLatLon_Grid_CVar(EOS5CFGrid *cfgrid, set<string>& tempvardimnamelist) 
{

    // First check if we have existing coordinate variable
    set<string>::iterator its;
    auto num_dimnames = (int)(tempvardimnamelist.size());
    bool has_dimnames = true;

    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {
        if (cfgrid->dnames_to_1dvnames.find(*its) != cfgrid->dnames_to_1dvnames.end()) {
            for (auto irv = this->vars.begin(); has_dimnames && (irv != this->vars.end());) {
                // We need to check if this var is a grid and use "newname"
                // of var to check the dnames_to_1dvnames since it is 
                // possible to have name clashing for the "name" of a var.
                if (GRID == Get_Var_EOS5_Type(*irv) && (*irv)->newname == (cfgrid->dnames_to_1dvnames)[*its]) {

                    //Find it, create a coordinate variable.
                    auto EOS5cvar_unique = make_unique<EOS5CVar>(*irv);
                    auto EOS5cvar = EOS5cvar_unique.release();

                    // Still keep the original dimension name to avoid the name clashing when
                    // one grid and one swath and one za occur in the same file
                    EOS5cvar->cfdimname = *its;
                    EOS5cvar->cvartype = CV_EXIST;
                    EOS5cvar->eos_type = GRID;

                    // Save this cv to the cv vector
                    this->cvars.push_back(EOS5cvar);

                    // Remove this var from the var vector since it becomes a cv.
                    delete (*irv);
                    irv = this->vars.erase(irv);
                    num_dimnames--;
                    if (0 == num_dimnames) has_dimnames = false;
                } // if (GRID == Get_Var_EOS5_Type(*irv) ...
                else {
                    ++irv;
                }
            } // for (auto irv = this->vars.begin(); ...
        } // if (cfgrid->dnames_to_1dvnames.find(*its) !=cfgrid->dnames_to_1dvnames.end())
    } // for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)

    // Remove the dimension name that finds the cooresponding variables from the tempvardimlist.
    for (auto irv = this->cvars.begin(); irv != this->cvars.end(); ++irv) {
        its = tempvardimnamelist.find((*irv)->cfdimname);
        if (its != tempvardimnamelist.end()) tempvardimnamelist.erase(its);
    }

    // Second: Some dimension names still need to find CVs, create the missing CVs
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {

        auto EOS5cvar_unique = make_unique<EOS5CVar>();
        auto EOS5cvar = EOS5cvar_unique.release();
        Create_Missing_CV(cfgrid, EOS5cvar, *its, GRID, this->eos5cfgrids.size());
        this->cvars.push_back(EOS5cvar);

    }
}

// Handle none-augmented grid coordinate variables for multiple grids.
void EOS5File::Handle_Multi_Nonaugment_Grid_CVar() 
{

    BESDEBUG("h5", "Coming to Handle_Multi_nonaugment_Grid_CVar()"<<endl);

    // If the multiple grids don't share the same lat/lon according to the parameters
    // We then assume that each single grid has its own lat/lon, just loop through each grid.
    if (true == this->grids_multi_latloncvs) {
        for (const auto &cfgrid:this->eos5cfgrids)
            Handle_Single_Nonaugment_Grid_CVar(cfgrid);
    }

    // We would like to check if lat/lon pairs provide for all grids
    // If lat/lon pairs are provided for all grids, then we ASSUME that
    // all grids share the same lat/lon values. This is what happened with
    // Aura grids. We only apply this to Aura files.They provide a lat/lon pair for each grid. We will observe
    // if this assumption is true for the future products.
    // If lat/lon pairs are  not provided for all grids, we assume that each grid
    // may still have its unique lat/lon.
    else {
        int num_1dlatlon_pairs = 0;
        for (auto irv = this->eos5cfgrids.begin(); irv != this->eos5cfgrids.end(); ++irv)
            if (true == (*irv)->has_1dlatlon) num_1dlatlon_pairs++;

        bool use_eos5_latlon = false;
        if ((0 == num_1dlatlon_pairs)
            || ((num_1dlatlon_pairs == (int) (this->eos5cfgrids.size())) && (true == this->isaura))) {
            set<string> tempvardimnamelist = ((this->eos5cfgrids)[0])->vardimnames;
            if (0 == num_1dlatlon_pairs) {
                use_eos5_latlon = Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon((this->eos5cfgrids)[0],
                    tempvardimnamelist);

                if (false == use_eos5_latlon) return;
            }

            else {
                // One lat/lon for all grids
                bool use_own_latlon = false;
                use_own_latlon = Handle_Single_Nonaugment_Grid_CVar_OwnLatLon((this->eos5cfgrids)[0],
                    tempvardimnamelist);
                if (false == use_own_latlon) {
                    use_eos5_latlon = Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon((this->eos5cfgrids)[0],
                        tempvardimnamelist);
                    if (false == use_eos5_latlon) return;
                }
            }

            // We need to handle the first grid differently since it will include "XDim" and "YDim".
            Handle_NonLatLon_Grid_CVar((this->eos5cfgrids)[0], tempvardimnamelist);

            // Updating the dimension name sets for other grids
            for (unsigned j = 1; j < this->eos5cfgrids.size(); j++)
                (this->eos5cfgrids)[j]->Update_Dimnamelist();

            // Adjusting the XDim and YDim dimension names for all vars
            Adjust_EOS5GridDimNames((this->eos5cfgrids)[0]);

            // Now we can safely handle the rest grids
            for (unsigned j = 1; j < this->eos5cfgrids.size(); j++) {
                tempvardimnamelist = (this->eos5cfgrids)[j]->vardimnames;
                Handle_NonLatLon_Grid_CVar((this->eos5cfgrids)[j], tempvardimnamelist);
                tempvardimnamelist.clear();
            }
        }    // if (( 0 == num_1dlatlon_pairs) || .....
             // No unique lat/lon, just loop through.
        else {

            this->grids_multi_latloncvs = true;
            for (const auto &cfgrid:this->eos5cfgrids)
                Handle_Single_Nonaugment_Grid_CVar(cfgrid);
        }
    }
}

// Adjust the HDF-EOS5 grid dimension names for XDim and YDim, we need to remember the grid path  
// Note this function is used under the assumption that only one lat/lon pair is used for all grids.
// This is the case for Aura.
void EOS5File::Adjust_EOS5GridDimNames(const EOS5CFGrid *cfgrid) const
{

    BESDEBUG("h5", "Coming to Adjust_EOS5GridDimNames()"<<endl);
    string xdimname;
    string ydimname;
    bool find_xdim = false;
    bool find_ydim = false;

    for (const auto &vardimname:cfgrid->vardimnames) {
        string xydimname_candidate = HDF5CFUtil::obtain_string_after_lastslash(vardimname);
        if ("XDim" == xydimname_candidate) {
            find_xdim = true;
            xdimname = vardimname; 
        }
        else if ("YDim" == xydimname_candidate) {
            find_ydim = true;
            ydimname = vardimname;
        }
        if (find_xdim && find_ydim) break;
    } // for (auto it = cfgrid->vardimnames.begin() ...

    if (false == find_xdim || false == find_ydim)
    throw2("Cannot find Dimension name that includes XDim or YDim in the grid ", cfgrid->name);

    for (const auto &var:this->vars) {
        if (GRID == Get_Var_EOS5_Type(var)) {
            for (const auto &dim:var->dims) {
                string xydimname_candidate = HDF5CFUtil::obtain_string_after_lastslash(dim->name);
                if ("XDim" == xydimname_candidate)
                    dim->name = xdimname;
                else if ("YDim" == xydimname_candidate) dim->name = ydimname;
            }
        }
    }
}

// Handle Swath Coordinate variables. 
void EOS5File::Handle_Swath_CVar(bool isaugmented) 
{

    BESDEBUG("h5", "Coming to Handle_Swath_CVar()"<<endl);
    // In this version, we will not use the augmented option for coordinate variables of swath
    // since MLS products don't use the recent version of the augmentation tool to allocate their 
    // coordinate variables.
    for (auto irs = this->eos5cfswaths.begin(); irs != this->eos5cfswaths.end();) {
        if ((*irs)->has_1dlatlon) {
            Handle_Single_1DLatLon_Swath_CVar(*irs, isaugmented);
            ++irs;
        }
        else if ((*irs)->has_2dlatlon) {
            Handle_Single_2DLatLon_Swath_CVar(*irs, isaugmented);
            ++irs;
        }
        // If number of dimension latitude/longitude is >2 or no lat/lon, 
        // no cooridnate variables will be generated.
        // We will simply remove this swath from the vector eos5cfswaths.
        // In the future, we may consider supporting non "Latitude", "Longitude" naming swaths. 
        // KY 2011-1-20
        else {
            delete (*irs);
            irs = this->eos5cfswaths.erase(irs);
        }
    } 
}

// Handle single 1D LatLon Swath Coordinate variables.
void EOS5File::Handle_Single_1DLatLon_Swath_CVar(EOS5CFSwath *cfswath, bool is_augmented) 
{

    BESDEBUG("h5", "Coming to Handle_Single_1DLatLon_Swath_CVar"<<endl);
    // For 1DLatLon, we will use latitude as the coordinate variable
    set<string> tempvardimnamelist = cfswath->vardimnames;
    string EOS5SWATHPATH = "/HDFEOS/SWATHS/";
    string fslash_str = "/";
    string THIS_EOS5SWATHPATH = EOS5SWATHPATH + cfswath->name + fslash_str;
#if 0
    for (auto its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)
    cerr<<"Dimension name befor latitude " << *its << endl;
#endif

    // Find latitude and assign to the coordinate variable
    // (*irv)->fullpath.size() > THIS_EOS5SWATHPATH.size() is necessary to handle the augmented variables. 
    for (auto irv = this->vars.begin(); irv != this->vars.end(); ++irv) {
        if (SWATH == Get_Var_EOS5_Type(*irv) && ((*irv)->fullpath.size() > THIS_EOS5SWATHPATH.size())) {

            string var_swath_name = Obtain_Var_EOS5Type_GroupName(*irv, SWATH);
            if ((var_swath_name == cfswath->name) && ((*irv)->name == "Latitude")) {

                //Find it, create a coordinate variable.
                auto EOS5cvar_unique = make_unique<EOS5CVar>(*irv);
                auto EOS5cvar = EOS5cvar_unique.release();

                // Still keep the original dimension name to avoid the name clashing when
                // one grid and one swath and one za occur in the same file
                EOS5cvar->cfdimname = ((*irv)->dims)[0]->name;
                EOS5cvar->cvartype = CV_EXIST;
                EOS5cvar->eos_type = SWATH;

                // Save this cv to the cv vector
                this->cvars.push_back(EOS5cvar);

                // Remove this var from the var vector since it becomes a cv.
                delete (*irv);
                this->vars.erase(irv);
                break;
            } // if ((var_swath_name == cfswath->name) && ...
        } // if SWATH 
    } // for (auto irv = this->vars.begin() ...

    // Finish this variable, remove it from the list.

    bool find_lat_dim = false;
    for (auto its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {

        for (const auto &cvar:this->cvars) {
            if ((cvar->name == "Latitude") && cvar->cfdimname == (*its)) {
                tempvardimnamelist.erase(its);
                find_lat_dim = true;
                break;
            }
        }

        if (true == find_lat_dim) break;
    }

#if 0
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)
    cerr<<"Dimension name afte latitude " << *its << endl;
#endif

    Handle_NonLatLon_Swath_CVar(cfswath, tempvardimnamelist);

    // Remove the added variables during the augmentation process
    if (true == is_augmented) {
        for (auto irv = this->vars.begin(); irv != this->vars.end();) {

            if (SWATH == Get_Var_EOS5_Type(*irv)) {
#if 0
                string my_swath_short_path = (*irv)->fullpath.substr(EOS5SWATHPATH.size());
                size_t first_fslash_pos = my_swath_short_path.find_first_of("/");
                string my_swath_name = my_swath_short_path.substr(0,first_fslash_pos);
#endif
                // Need to find the swath for this variable
                string my_swath_name = Obtain_Var_EOS5Type_GroupName(*irv, SWATH);

                if (my_swath_name == cfswath->name) {
                    string var_path_after_swathname = (*irv)->fullpath.substr(THIS_EOS5SWATHPATH.size());
                    if (var_path_after_swathname == (*irv)->name) {
                        delete (*irv);
                        irv = this->vars.erase(irv);
                    }
                    else {
                        ++irv;
                    }
                }
                else {
                    ++irv;
                }
            }
            else {
                ++irv;
            }
        } // end of for loop
    } // if (true == is_augmented)
}

// Handle Single 2D lat/lon Coordinate variables for Swath
void EOS5File::Handle_Single_2DLatLon_Swath_CVar(EOS5CFSwath *cfswath, bool is_augmented) 
{

    BESDEBUG("h5", "Coming to Handle_Single_2DLatLon_Swath_CVar()"<<endl);
    // For 2DLatLon, we will use both latitude and longitude as the coordinate variables
    set<string>::iterator its;
    set<string> tempvardimnamelist = cfswath->vardimnames;
    string EOS5SWATHPATH = "/HDFEOS/SWATHS/";
    string fslash_str = "/";
    string THIS_EOS5SWATHPATH = EOS5SWATHPATH + cfswath->name + fslash_str;
    bool find_lat = false;
    bool find_lon = false;

#if 0
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)
    cerr<<"Dimension name befor latitude " << *its << endl;
#endif

    // Find latitude and assign to the coordinate variable
    for (auto irv = this->vars.begin(); irv != this->vars.end();) {
        if (SWATH == Get_Var_EOS5_Type(*irv) && ((*irv)->fullpath.size() > THIS_EOS5SWATHPATH.size())) {
            string var_swath_name = Obtain_Var_EOS5Type_GroupName(*irv, SWATH);
            if ((var_swath_name == cfswath->name) && ((*irv)->name == "Latitude")) {

                //Find it, create a coordinate variable.
                auto EOS5cvar_unique = make_unique<EOS5CVar>(*irv);
                auto EOS5cvar = EOS5cvar_unique.release();

                // Still keep the original dimension name to avoid the name clashing when
                // one grid and one swath and one za occur in the same file
                EOS5cvar->cfdimname = ((*irv)->dims)[0]->name;
                EOS5cvar->cvartype = CV_EXIST;
                EOS5cvar->eos_type = SWATH;
                EOS5cvar->is_2dlatlon = true;

                // Save this cv to the cv vector
                this->cvars.push_back(EOS5cvar);

                // Remove this var from the var vector since it becomes a cv.
                delete (*irv);
                irv = this->vars.erase(irv);
                find_lat = true;
            } 
            else if ((var_swath_name == cfswath->name) && ((*irv)->name == "Longitude")) {

                //Find it, create a coordinate variable.
                auto EOS5cvar_unique = make_unique<EOS5CVar>(*irv);
                auto EOS5cvar = EOS5cvar_unique.release();

                // Still keep the original dimension name to avoid the name clashing when
                // one grid and one swath and one za occur in the same file
                EOS5cvar->cfdimname = ((*irv)->dims)[1]->name;
                EOS5cvar->cvartype = CV_EXIST;
                EOS5cvar->eos_type = SWATH;
                EOS5cvar->is_2dlatlon = true;

                // Save this cv to the cv vector
                this->cvars.push_back(EOS5cvar);

                // Remove this var from the var vector since it becomes a cv.
                delete (*irv);
                irv = this->vars.erase(irv);
                find_lon = true;

            } 
            else {
                ++irv;
            }
        } // if (SWATH == Get_Var_EOS5_Type(*irv) && ...
        else {
            ++irv;
        }

        if (true == find_lat && true == find_lon) break;
    } // for (auto irv = this->vars.begin();

    // Remove the dim. of latitude 
    find_lat = false;
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {
        for (const auto &cvar:this->cvars) {
            if ((cvar->name == "Latitude") && cvar->cfdimname == (*its)) {
                tempvardimnamelist.erase(its);
                find_lat = true;
                break;
            }
        }

        if (true == find_lat) break;
    }

    // Remove the dim. of longitude
    find_lon = false;
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {

        for (const auto &cvar:this->cvars) {

            if ((cvar->name == "Longitude") && cvar->cfdimname == (*its)) {
                tempvardimnamelist.erase(its);
                find_lon = true;
                break;
            }
        }

        if (true == find_lon) break;
    }

#if 0
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)
    cerr<<"Dimension name afte latitude " << *its << endl;
#endif

    Handle_NonLatLon_Swath_CVar(cfswath, tempvardimnamelist);

    // Remove the added variables during the augmentation process
    // For Swath, we don't want to keep the augmented files. This is because 
    // some aura files assign the dimensional scale as zero.
    // We will actively check the new NASA HDF-EOS5 products and will 
    // revise the following section as needed. KY 2012-03-09
    if (true == is_augmented) {
        for (auto irv = this->vars.begin(); irv != this->vars.end();) {

            if (SWATH == Get_Var_EOS5_Type(*irv)) {

                string my_swath_name = Obtain_Var_EOS5Type_GroupName(*irv, SWATH);
                if (my_swath_name == cfswath->name) {
                    string var_path_after_swathname = (*irv)->fullpath.substr(THIS_EOS5SWATHPATH.size());
                    if (var_path_after_swathname == (*irv)->name) {
                        delete (*irv);
                        irv = this->vars.erase(irv);
                    }
                    else {
                        ++irv;
                    }
                }
                else {
                    ++irv;
                }
            }
            else {
                ++irv;
            }
        }  
    }
}

// Handle non-lat/lon Swath coordinate variables.
void EOS5File::Handle_NonLatLon_Swath_CVar(EOS5CFSwath *cfswath, set<string>& tempvardimnamelist) 
{

    BESDEBUG("h5", "Coming to Handle_NonLatLon_Swath_CVar()"<<endl);
    // First check if we have existing coordinate variable
    set<string>::iterator its;
    auto num_dimnames = (int)(tempvardimnamelist.size());
    bool has_dimnames = true;
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {
        if (cfswath->dnames_to_geo1dvnames.find(*its) != cfswath->dnames_to_geo1dvnames.end()) {
            for (auto irv = this->vars.begin(); has_dimnames && (irv != this->vars.end());) {

                // We need to check if this var is a swath and use "newname"
                // of var to check the dnames_to_1dvnames since it is 
                // possible to have name clashing for the "name" of a var.
                if (SWATH == Get_Var_EOS5_Type(*irv) && (*irv)->newname == (cfswath->dnames_to_geo1dvnames)[*its]) {

                    //Find it, create a coordinate variable.
                    auto EOS5cvar_unique = make_unique<EOS5CVar>(*irv);
                    auto EOS5cvar = EOS5cvar_unique.release();

                    // Still keep the original dimension name to avoid the name clashing when
                    // one grid and one swath and one za occur in the same file
                    EOS5cvar->cfdimname = *its;
                    EOS5cvar->cvartype = CV_EXIST;
                    EOS5cvar->eos_type = SWATH;

                    // Save this cv to the cv vector
                    this->cvars.push_back(EOS5cvar);

                    // Remove this var from the var vector since it becomes a cv.
                    delete (*irv);
                    irv = this->vars.erase(irv);
                    num_dimnames--;
                    if (0 == num_dimnames) has_dimnames = false;
                } 
                else {
                    ++irv;
                }
            } // for (auto irv = this->vars.begin(); ...
        } // if (cfswath->dnames_to_geo1dvnames.find(*its) ....
    } // for (its = tempvardimnamelist.begin()...

    // Remove the dimension name that finds the cooresponding variables from the tempvardimlist.
    for (const auto &cvar:this->cvars) {
        its = tempvardimnamelist.find(cvar->cfdimname);
        if (its != tempvardimnamelist.end()) tempvardimnamelist.erase(its);
    }

    // Check if some attributes have CV information for some special products
    // Currently TES needs to be handled carefully
    Handle_Special_NonLatLon_Swath_CVar(cfswath, tempvardimnamelist);

    // Remove the dimension name that finds the cooresponding variables from the tempvardimlist.
    for (const auto &cvar:this->cvars) {
        its = tempvardimnamelist.find(cvar->cfdimname);
        if (its != tempvardimnamelist.end()) tempvardimnamelist.erase(its);
    }

    // Second: Some dimension names still need to find CVs, create the missing CVs
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {

        auto EOS5cvar_unique = make_unique<EOS5CVar>();
        auto EOS5cvar = EOS5cvar_unique.release();
        Create_Missing_CV(cfswath, EOS5cvar, *its, SWATH, this->eos5cfswaths.size());
        this->cvars.push_back(EOS5cvar);

    }
}

// Handle special non-lat/lon coordinate variables for swath.
void EOS5File::Handle_Special_NonLatLon_Swath_CVar(EOS5CFSwath *cfswath, const set<string>& tempvardimnamelist)
    
{

    BESDEBUG("h5", "Handle_Special_NonLatLon_Swath_CVar()"<<endl);
    // We have no choice but hard-code this one. 
    // TES swath puts "Pressure" as the VerticalCoordinate but doesn't provide "Pressure" values.
    // Moreover, the number of pressure level(66) is one less than the total number of corresponding dimension size(67)
    // most probably due to the missing pressure level on the ground. To make the handler visualize some
    // TES variables and to follow the general physical sense. We have to add a pressure level by linear interpolation.
    // KY 2012-1-27
    if (true == this->isaura && TES == this->aura_name) {

        string eos5_swath_group_name = "/HDFEOS/SWATHS/" + cfswath->name;
        string eos5_vc_attr_name = "VerticalCoordinate";
        string eos5_pre_attr_name = "Pressure";
        bool has_vc_attr = false;
        const Group *vc_group = nullptr;

        // 1. Check if having the "VerticalCoordinate" attribute in this swath and the attribute is "Pressure".
        for (const auto &grp:this->groups) {
            if (eos5_swath_group_name == grp->path) {
                for (const auto &attr:grp->attrs) {
                    if (eos5_vc_attr_name == attr->name) {
                        Retrieve_H5_Attr_Value(attr, grp->path);
                        string attr_value(attr->value.begin(), attr->value.end());
                        if (eos5_pre_attr_name == attr_value) {
                            has_vc_attr = true;
                            vc_group = grp;
                            break;
                        }
                    }
                } // for (vector<Attribute *>:: iterator ira =grp->attrs.begin(); ...
                if (true == has_vc_attr) break;
            } // if (eos5_swath_group_name ==grp->path)
        } // for (const auto &grp: ...

        // 2. Check if having the "Pressure" attribute and if the attribute size is 1 less than
        // the dimension size of "nLevels". If yes,
        // add one pressure value by using the nearest neighbor value. This value should be the first value
        // of the "Pressure" attribute.
        // Another special part of the TES file is that dimension name nLevels is used twice in some variables
        // float foo[...][nLevels][nLevels]. To make the variable visualized by tools, the dimension name
        // needs to be changed and the coordinate variable needs to separately created. Note this is not
        // against CF conventions. However, the popular tools are not happy with the duplicate dimension names 
        // in a variable.
        // Though may not cover 100% cases, searching the string after the last forward slash and see if
        // it contains nLevels should catch 99% memebers of  the "nLevels" family. We will then create the
        // corresponding coordinate variables.

        // 2.1. Check if we have the dimension name called "nLevels" for this swath
        if (true == has_vc_attr) {
            string dimname_candidate = "/SWATHS/" + cfswath->name + "/nLevels";
            //for (auto it = tempvardimnamelist.begin(); it != tempvardimnamelist.end(); ++it) {
            for (const auto &tvar_dimname:tempvardimnamelist) {
                if (tvar_dimname.find(dimname_candidate) != string::npos) {
                    hsize_t dimsize_candidate = 0;
                    if ((cfswath->dimnames_to_dimsizes).find(tvar_dimname) != (cfswath->dimnames_to_dimsizes).end())
                        dimsize_candidate = cfswath->dimnames_to_dimsizes[tvar_dimname];
                    else
                        throw2("Cannot find the dimension size of the dimension name ", tvar_dimname);

                    // Note: we don't have to use two loops to create the coordinate variables.
                    // However, there are only 3-4 attributes for this group and so far TES has only 
                    // one additional nLevels.
                    // So essentially the following loop doesn't hurt the performance.
                    // KY 2012-2-1
                    for (const auto &attr:vc_group->attrs) {
                        if ((eos5_pre_attr_name == attr->name) && (attr->count == (dimsize_candidate - 1))) {

                            // Should change the attr_value from char type to float type when reading the data
                            // Here just adding a coordinate variable by using this name.
                            string reduced_dimname = HDF5CFUtil::obtain_string_after_lastslash(tvar_dimname);
                            string orig_dimname = "nLevels";
                            auto EOS5cvar_unique = make_unique<EOS5CVar>();
                            auto EOS5cvar = EOS5cvar_unique.release();
                            if ("nLevels" == reduced_dimname)
                                EOS5cvar->name = eos5_pre_attr_name + "_CV";
                            else
                                // the dimname will be ..._CV_1 etc.
                                EOS5cvar->name = eos5_pre_attr_name + "_CV"
                                    + reduced_dimname.substr(orig_dimname.size());
                            Create_Added_Var_NewName_FullPath(SWATH, cfswath->name, EOS5cvar->name, EOS5cvar->newname,
                                EOS5cvar->fullpath);
                            EOS5cvar->rank = 1;
                            EOS5cvar->dtype = attr->dtype;
                            auto eos5cvar_dim_unique = make_unique<Dimension>(dimsize_candidate);
                            auto eos5cvar_dim = eos5cvar_dim_unique.release();
                            eos5cvar_dim->name = tvar_dimname;
                            if (1 == this->eos5cfswaths.size())
                                eos5cvar_dim->newname = reduced_dimname;
                            else
                                eos5cvar_dim->newname = eos5cvar_dim->name;

                            EOS5cvar->dims.push_back(eos5cvar_dim);
                            EOS5cvar->cvartype = CV_SPECIAL;
                            EOS5cvar->cfdimname = eos5cvar_dim->name;
                            EOS5cvar->eos_type = SWATH;

                            // Save this cv to the cv vector
                            this->cvars.push_back(EOS5cvar);
                        } // if ((eos5_pre_attr_name == attr->name) && ...
                    } // for auto attr
                } // if ((*it).find(dimname_candidate) != string::npos)
            } // for (it = tempvardimnamelist.begin(); ...
        } // if (true == has_vc_attr) ...
    } // if (true == this->isaura && ...
}

// Handle Zonal average coordinate variables.    
void EOS5File::Handle_Za_CVar(bool isaugmented) 
{

    BESDEBUG("h5", "Coming to Handle_Za_CVar()"<<endl);
    // We are not supporting non-augmented zonal average HDF-EOS5 product now. KY:2012-1-20
    if (false == isaugmented) return;

    for (const auto &cfza:this->eos5cfzas)
        Handle_Single_Augment_CVar(cfza, ZA);

}

// Adjust the newname(final names appeared at DDS) for variable and dimensions before flattening.
void EOS5File::Adjust_Var_Dim_NewName_Before_Flattening() const
{

    BESDEBUG("h5", "Coming to Adjust_Var_Dim_NewName_Before_Flattening()"<<endl);
    auto num_grids = (int)(this->eos5cfgrids.size());
    auto num_swaths = (int)(this->eos5cfswaths.size());
    auto num_zas = (int)(this->eos5cfzas.size());

    bool mixed_eos5typefile = false;

    // Check if this file mixes grid,swath and zonal average
    if (((num_grids > 0) && (num_swaths > 0)) || ((num_grids > 0) && (num_zas > 0))
        || ((num_swaths > 0) && (num_zas > 0))) mixed_eos5typefile = true;

    // This file doesn't mix swath, grid and zonal average
    for (const auto &var:this->vars)
        Adjust_Per_Var_Dim_NewName_Before_Flattening(var, mixed_eos5typefile, num_grids, num_swaths, num_zas);

    for (const auto &cvar:this->cvars)
        Adjust_Per_Var_Dim_NewName_Before_Flattening(cvar, mixed_eos5typefile, num_grids, num_swaths, num_zas);
#if 0
    for (auto irv = this->cvars.begin();
        irv != this->cvars.end(); ++irv) {
        cerr<<"eos5svar var new name "<<(*irv)->newname <<endl;
        for (auto ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end(); ++ird) {
            cerr<<"eos5svar dimension new name "<<(*ird)->newname <<endl;
        }
    }
#endif
    // If (lat,lon) is shared for grids, more consideration for the names
    Adjust_SharedLatLon_Grid_Var_Dim_Name();

}

// Adjust the final name of one variable or dim. before flattening the names
template<class T>
void EOS5File::Adjust_Per_Var_Dim_NewName_Before_Flattening(T* var, bool mixed_eos5type, int num_grids, int num_swaths,
    int num_zas) const
{

    BESDEBUG("h5", "Coming to Adjust_Per_Var_Dim_NewName_Before_Flattening()"<<endl);

    string eos5typestr;
    EOS5Type vartype = Get_Var_EOS5_Type(var);
    switch (vartype) {

    case GRID: {
        eos5typestr = "/GRIDS/";
        if (false == mixed_eos5type) {
            if (0 == num_grids)
                var->newname = ((1 == this->orig_num_grids) ? var->name : var->newname.substr(eos5typestr.size()));
            else
                var->newname = ((1 == num_grids) ? var->name : var->newname.substr(eos5typestr.size()));
            // Dimension newname is unlike Var newname, when num_grids is equal to 1, the
            // newname is Dimension name already. So we don't need to do anything with
            // the dimension newname when the num_grids is 1. The main reason we handle
            // the var newname and the dimension newname differently is that the variable name is
            // more critical for users to pick up the meanings of that variable. So we would like
            // to work hard to keep the original form. However, the dimension name is mainly used to
            // generate the coordinate variables. So the different usage makes us relax the dimension
            // name a bit. This is an example of end-user priority driven implementation.
            // KY 2012-1-24
            // Just receive a user request: the dimension name is also very important.
            // So a bunch of code has been updated. For number of grid/swath/za = 1, I still maintain
            // the newname to be the same as the last part of the dim name. Hopefully this
            // will handle the current HDF-EOS5 products. Improvement for complicate HDF-EOS5 products
            // will be supported as demanded in the future. KY 2012-1-26
            if (num_grids > 1) {
                for (auto &dim:var->dims) {
                    if (dim->newname.size() <= eos5typestr.size())
                        throw5("The size of the dimension new name ", dim->newname, "of variable ", var->newname,
                            " is too small");
                    dim->newname = dim->newname.substr(eos5typestr.size());
                }
            }
        } // if(false == mixed_eos5type)
        else {
            // No need to set the dimension newname for the reason listed above.
            var->newname = ((1 == num_grids) ? (eos5typestr + var->name) : var->newname);
        }
    }
        break;

    case SWATH: {
        eos5typestr = "/SWATHS/";
        if (false == mixed_eos5type) {
            var->newname = ((1 == num_swaths) ? var->name : var->newname.substr(eos5typestr.size()));
            if (num_swaths > 1) {
                for (auto &dim:var->dims) {
                    if (dim->newname.size() <= eos5typestr.size())
                        throw5("The size of the dimension new name ", dim->newname, "of variable ", var->newname,
                            " is too small");
                    dim->newname = dim->newname.substr(eos5typestr.size());
                }
            }
        }
        else {
            var->newname = ((1 == num_swaths) ? (eos5typestr + var->name) : var->newname);
        }
    }
        break;

    case ZA: {
        eos5typestr = "/ZAS/";
        if (false == mixed_eos5type) {
            var->newname = ((1 == num_zas) ? var->name : var->newname.substr(eos5typestr.size()));
            if (num_zas > 1) {
                for (auto &dim:var->dims) {
                    if (dim->newname.size() <= eos5typestr.size())
                        throw5("The size of the dimension new name ", dim->newname, "of variable ", var->newname,
                            " is too small");
                    dim->newname = dim->newname.substr(eos5typestr.size());
                }
            }
        }
        else {
            var->newname = ((1 == num_zas) ? (eos5typestr + var->name) : var->newname);
        }
    }
        break;
    case OTHERVARS:
        break;
    default:
        throw1("Non-supported EOS type");
    } 

}

// Adjust shared var and dim names for shared lat/lon grid case.
void EOS5File::Adjust_SharedLatLon_Grid_Var_Dim_Name() const
{

    BESDEBUG("h5", "Adjust_SharedLatLon_Grid_Var_Dim_Name()"<<endl);
    // Remove the EOS5 type string("GRIDS") and the GRID Name from 
    // the variable newname and the dimension newname
    // This case won't happen for the current version, but may occur
    // if curviliner grid exists in the file. KY 2012-1-26
    if ((this->eos5cfgrids.size() > 1) && (true == this->eos5cfswaths.empty()) && (true == this->eos5cfzas.empty())
        && (false == this->grids_multi_latloncvs)) {

        // We would like to condense the dimension name and the coordinate variable name for lat/lon.
        string lat_dimname;
        string lat_dimnewname;
        string lon_dimname;
        string lon_dimnewname;
        for (const auto &cvar:this->cvars) {
            if ("lat" == cvar->name || "Latitude" == cvar->name) {
                cvar->newname = cvar->name;
                lat_dimnewname = ((cvar->dims)[0])->newname;
                lat_dimnewname = HDF5CFUtil::obtain_string_after_lastslash(lat_dimnewname);
                if ("" == lat_dimnewname)
                throw2("/ is not included in the dimension new name ", ((cvar->dims)[0])->newname);
                ((cvar->dims)[0])->newname = lat_dimnewname;
                lat_dimname = cvar->cfdimname;
            }
            else if ("lon" == cvar->name || "Longitude" == cvar->name) {
                cvar->newname = cvar->name;
                lon_dimnewname = ((cvar->dims)[0])->newname;
                lon_dimnewname = HDF5CFUtil::obtain_string_after_lastslash(lon_dimnewname);
                if ("" == lon_dimnewname)
                throw2("/ is not included in the dimension new name ", ((cvar->dims)[0])->newname);
                ((cvar->dims)[0])->newname = lon_dimnewname;
                lon_dimname = cvar->cfdimname;
            }
        } // for 

        for (auto &var:this->vars) {
            for (auto &dim:var->dims) {
                if (dim->name == lat_dimname)
                    dim->newname = lat_dimnewname;
                else if (dim->name == lon_dimname) dim->newname = lon_dimnewname;
            }
        }
    } // if ((this->eos5cfgrids.size() > 1) && ...
}

// Flatten the object names.
void EOS5File::Flatten_Obj_Name(bool include_attr) 
{

    BESDEBUG("h5", "Coming to Flatten_Obj_Name()"<<endl);
    File::Flatten_Obj_Name(include_attr);

    for (auto &cvar:this->cvars) {
        cvar->newname = get_CF_string(cvar->newname);

        for (auto &dim:cvar->dims) 
            dim->newname = get_CF_string(dim->newname);

        if (true == include_attr) {
            for (auto &attr:cvar->attrs) {
                attr->newname = File::get_CF_string(attr->newname);
            }
        }
    } // end for 
}

// Handle Object Name clashing
void EOS5File::Handle_Obj_NameClashing(bool include_attr) 
{

    BESDEBUG("h5", "Coming to Handle_Obj_NameClashing()"<<endl);
    // objnameset will be filled with all object names that we are going to check the name clashing.
    // For example, we want to see if there are any name clashing for all variable names in this file.
    // objnameset will include all variable names. If a name clashing occurs, we can figure out from the set operation immediately.
    set<string> objnameset;
    Handle_EOS5CVar_NameClashing(objnameset);
    File::Handle_GeneralObj_NameClashing(include_attr, objnameset);
    if (true == include_attr) {
        Handle_EOS5CVar_AttrNameClashing();
    }
#if 0
    //if (this->cvars.size() >0) 
    //  Handle_DimNameClashing();
#endif
}

// Handle EOS5 coordinate variable name clashing
void EOS5File::Handle_EOS5CVar_NameClashing(set<string> &objnameset) 
{

    BESDEBUG("h5", "Coming to Handle_EOS5CVar_NameClashing()"<<endl);
    EOS5Handle_General_NameClashing(objnameset, this->cvars);
}

// Handle EOS5 coordinate variable attribute name clashing
void EOS5File::Handle_EOS5CVar_AttrNameClashing() 
{

    BESDEBUG("h5", "Coming to Handle_EOS5CVar_AttrNameClashing()"<<endl);
    set<string> objnameset;

    for (const auto &cvar:this->cvars) {
        Handle_General_NameClashing(objnameset, cvar->attrs);
        objnameset.clear();
    }
}
// The routine to handle general name clashing 
//class T must have member string newname
template<class T> void EOS5File::EOS5Handle_General_NameClashing(set<string>&objnameset, vector<T*>& objvec)
    
{

    BESDEBUG("h5", "Coming to EOS5Handle_General_NameClashing()"<<endl);
    pair<set<string>::iterator, bool> setret;
    set<string>::iterator iss;

    vector<string> clashnamelist;

    map<int, int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;

#if 0
    typename vector<T*>::iterator irv;
#endif

    for (auto irv = objvec.begin(); irv != objvec.end(); ++irv) {

        setret = objnameset.insert((*irv)->newname);
        if (!setret.second) {
            clashnamelist.insert(clashnamelist.end(), (*irv)->newname);
            cl_to_ol[cl_index] = ol_index;
            cl_index++;
        }
        ol_index++;
    }

    // Now change the clashed elements to unique elements; 
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

// Handle Dimension name clashing
void EOS5File::Handle_DimNameClashing() 
{

    BESDEBUG("h5", "Coming to Handle_DimNameClashing()"<<endl);
    map<string, string> dimname_to_dimnewname;
    pair<map<string, string>::iterator, bool> mapret;
    set<string> dimnameset;
    vector<Dimension*> vdims;
    set<string> dimnewnameset;
    pair<set<string>::iterator, bool> setret;

    // First: Generate the dimset/dimvar based on coordinate variables.
    for (const auto &cvar:this->cvars) {
        for (const auto &dim:cvar->dims) {
#if 0
            //setret = dimnameset.insert((*ird)->newname);
#endif
            setret = dimnameset.insert(dim->name);
            if (setret.second) vdims.push_back(dim);
        }
    }

    // For some cases, dimension names are provided but there are no corresponding coordinate
    // variables. For now, we will assume no such cases.
    // Actually, we find such a case in our fake testsuite. So we need to fix it.

    for (const auto &var:this->vars) {
        for (const auto &dim:var->dims) {
#if 0
            //setret = dimnameset.insert((*ird)->newname);
#endif
            setret = dimnameset.insert(dim->name);
            if (setret.second) vdims.push_back(dim);
        }
    }

#if 0
    for (auto ird=vdims.begin();ird!=vdims.end();++ird)
    cerr<<"dimension name "<<(*ird)->name <<endl;
#endif

    // For some cases, dimension names are provided but there are no corresponding coordinate
    // variables. For now, we will assume no such cases.
    EOS5Handle_General_NameClashing(dimnewnameset, vdims);

    // Third: Make dimname_to_dimnewname map
    for (const auto &dim:vdims) {
        mapret = dimname_to_dimnewname.insert(pair<string, string>(dim->name, dim->newname));
        if (false == mapret.second)
        throw4("The dimension name ", dim->name, " should map to ", dim->newname);
    }

    // Fourth: Change the original dimension new names to the unique dimension new names
    for (auto &cvar:this->cvars)
        for (auto &dim:cvar->dims)
            dim->newname = dimname_to_dimnewname[dim->name];

    for (auto &var:this->vars)
        for (auto &dim:var->dims)
            dim->newname = dimname_to_dimnewname[dim->name];

}

// Set COARDS Status, if we can follow COARDS, we should follow COARDS.
// http://ferret.wrc.noaa.gov/noaa_coop/coop_cdf_profile.html
void EOS5File::Set_COARDS_Status() 
{

    BESDEBUG("h5", "Coming to Set_COARDS_Status()"<<endl);
    iscoard = true;
    for (const auto &cfgrid:this->eos5cfgrids) {
        if (false == cfgrid->has_1dlatlon) {
            if (false == cfgrid->has_nolatlon || (HE5_GCTP_GEO != cfgrid->eos5_projcode)) iscoard = false;
            break;
        }
    }

    if (true == iscoard) {
        for (const auto &cfswath:this->eos5cfswaths) {
            if (false == cfswath->has_1dlatlon) {
                iscoard = false;
                break;
            }
        }
    }
}

// Adjust attribute info., mostly for CF name correction of Aura files.
void EOS5File::Adjust_Attr_Info() 
{

    BESDEBUG("h5", "Coming to Adjust_Attr_Info()"<<endl);
    if (true == this->isaura) {
        Adjust_Aura_Attr_Name();
        Adjust_Aura_Attr_Value();
    }
    else {
        Handle_EOS5CVar_Unit_Attr();
        Add_EOS5_Grid_CF_Attr();
    }
}

// Adjust Attribute Name, mostly for Aura files.
void EOS5File::Adjust_Aura_Attr_Name() 
{

    BESDEBUG("h5", "Coming to Adjust_Attr_Name() for Aura"<<endl);
    for (auto &var:this->vars) {
        for (auto &attr:var->attrs) {
            if (eos5_to_cf_attr_map.find(attr->name) != eos5_to_cf_attr_map.end()) 
                attr->newname = eos5_to_cf_attr_map[attr->name];
        }
    }

    for (auto &var:this->cvars) {
        for (auto &attr:var->attrs) {
            if (eos5_to_cf_attr_map.find(attr->name) != eos5_to_cf_attr_map.end()) 
                attr->newname = eos5_to_cf_attr_map[attr->name];
        }
    }
}

void EOS5File::Adjust_Aura_Attr_Value() const
{

    BESDEBUG("h5", "Coming to Adjust_Attr_Value() for Aura"<<endl);
    // Handle Units
    Handle_EOS5CVar_Unit_Attr();
    Handle_Aura_Special_Attr();

    // Handle Time. This is just for Aura files. 
    // This is for speical NASA requests only for Aura. 
    // We need to pay attention if things get changed later.
    string time_cf_units_value = "seconds since 1993-01-01";
    for (const auto &var:this->vars) {
        if ((var->name == "Time") || (var->name == "nTimes")) {
            for (const auto &attr:var->attrs) {
                if ("units" == attr->name) {
                    Retrieve_H5_Attr_Value(attr, var->fullpath);
                    string units_value(attr->value.begin(), attr->value.end());
                    if (time_cf_units_value != units_value) {

                        units_value = time_cf_units_value;
                        attr->value.resize(units_value.size());
                        if (H5FSTRING == attr->dtype) attr->fstrsize = units_value.size();
                        // strsize is used by both fixed and variable length strings.
                        attr->strsize.resize(1);
                        attr->strsize[0] = units_value.size();

                        copy(units_value.begin(), units_value.end(), attr->value.begin());
                    }
                    break;
                } // if ("units" == attr->name)
            } // for (const auto &attr
        } // if((var->name == "Time") || (var->name == "nTimes"))
    } // for (auto irv = this->vars.begin()...
}

// Handle EOS5 coordinate variable special attributes. 
void EOS5File::Handle_Aura_Special_Attr() const 
{

    BESDEBUG("h5", "Coming to Handle_Aura_Special_Attr()"<<endl);
    // Need to handle MLS aura file specially.
    if (true == this->isaura && MLS == this->aura_name) {

        const string File_attr_group_path = "/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES";
        const string PCF1_attr_name = "PCF1";
        bool find_group = false;
        bool find_attr = false;
        for (const auto &grp:this->groups) {
            if (File_attr_group_path == grp->path) {
                find_group = true;
                for (const auto &attr:grp->attrs) {
                    if (PCF1_attr_name == attr->name) {
                        Retrieve_H5_Attr_Value(attr, grp->path);
                        string pcf_value(attr->value.begin(), attr->value.end());
                        HDF5CFDAPUtil::replace_double_quote(pcf_value);
                        attr->value.resize(pcf_value.size());
                        if (H5FSTRING == attr->dtype) attr->fstrsize = pcf_value.size();
                        // strsize is used by both fixed and variable length strings.
                        attr->strsize.resize(1);
                        attr->strsize[0] = pcf_value.size();

                        copy(pcf_value.begin(), pcf_value.end(), attr->value.begin());
                        find_attr = true;
                        break;
                    } // if (PCF1_attr_name == attr->name)  
                } // for (auto attr
            } // if (File_attr_group_path == (*it_g)->path)
            if (true == find_group && true == find_attr) break;
        } // end of for it_g ...
    } // if (true == this->isaura && MLS == this->aura_name)
}

// Handle coordinate variable units attribute     
void EOS5File::Handle_EOS5CVar_Unit_Attr() const
{

    BESDEBUG("h5", "Coming to Handle_EOS5CVar_Unit_Attr()"<<endl);
    string unit_attrname = "units";
    string nonll_cf_level_attrvalue = "level";
    string lat_cf_unit_attrvalue = "degrees_north";
    string lon_cf_unit_attrvalue = "degrees_east";
    string tes_cf_pre_attrvalue = "hPa";

    for (const auto &cvar:this->cvars) {
        switch (cvar->cvartype) {
        case CV_EXIST:
        case CV_MODIFY: {
            for (const auto &attr:cvar->attrs) {
                if (attr->newname == unit_attrname) {
                    Retrieve_H5_Attr_Value(attr, cvar->fullpath);
                    string units_value(attr->value.begin(), attr->value.end());
                    if ((lat_cf_unit_attrvalue != units_value)
                        && ((cvar->name == "Latitude") || ((this->eos5cfzas.empty() == false) && (cvar->name == "nLats")))) {
                        units_value = lat_cf_unit_attrvalue;
#if 0
//cerr<<"coming to obtain the correct units_value: "<<units_value <<endl;
//cerr<<"cvar name is "<<cvar->newname <<endl;
#endif
                        attr->value.resize(units_value.size());
                        if (H5FSTRING == attr->dtype) attr->fstrsize = units_value.size();
                        // strsize is used by both fixed and variable length strings.
                        attr->strsize.resize(1);
                        attr->strsize[0] = units_value.size();
                        copy(units_value.begin(), units_value.end(), attr->value.begin());
                    }
                    else if ((lon_cf_unit_attrvalue != units_value) && cvar->name == "Longitude") {
                        units_value = lon_cf_unit_attrvalue;
                        attr->value.resize(units_value.size());
                        if (H5FSTRING == attr->dtype) attr->fstrsize = units_value.size();
                        // strsize is used by both fixed and variable length strings.
                        attr->strsize.resize(1);
                        attr->strsize[0] = units_value.size();

                        copy(units_value.begin(), units_value.end(), attr->value.begin());
                    }
                    break;
                } // if (attr->newname ==unit_attrname)
            }
        }
            break;

        case CV_LAT_MISS: {
            auto attr_unique = make_unique<Attribute>();
            auto attr = attr_unique.release();
            Add_Str_Attr(attr, unit_attrname, lat_cf_unit_attrvalue);
            cvar->attrs.push_back(attr);
        }
            break;

        case CV_LON_MISS: {
            auto attr_unique = make_unique<Attribute>();
            auto attr = attr_unique.release();
            Add_Str_Attr(attr, unit_attrname, lon_cf_unit_attrvalue);
            cvar->attrs.push_back(attr);
        }
            break;

        case CV_NONLATLON_MISS: {
            auto attr_unique = make_unique<Attribute>();
            auto attr = attr_unique.release();
            Add_Str_Attr(attr, unit_attrname, nonll_cf_level_attrvalue);
            cvar->attrs.push_back(attr);
        }
            break;
        case CV_SPECIAL: {
            if (true == this->isaura && TES == this->aura_name) {
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_Str_Attr(attr, unit_attrname, tes_cf_pre_attrvalue);
                cvar->attrs.push_back(attr);
            }
        }
            break;
        default:
            throw1("Non-supported Coordinate Variable Type.");
        } 
    } // for (auto cvar
}

void EOS5File::Add_EOS5_Grid_CF_Attr()  
{
    BESDEBUG("h5", "Coming to Add_EOS5_Grid_CF_Attr()"<<endl);

    bool has_eos5_grid_nongeo_proj = false;

    // Check if we have EOS5 grids that are not using the geographic projection.
    for (const auto &cvar:this->cvars) {
        if (cvar->cvartype == CV_LAT_MISS) {
            if(cvar->eos5_projcode !=HE5_GCTP_GEO) {
                has_eos5_grid_nongeo_proj = true;
                break;
            }
        } 
    } 

    // We would like to add the CF conventions mark if the mark is not there.
    if(true == has_eos5_grid_nongeo_proj)  {
        string conventions_attrname = "Conventions";
        string conventions_attrvalue = "CF-1.7";
        bool has_conventions_attr=false;
        for (const auto &root_attr:this->root_attrs) {
            if(root_attr->name==conventions_attrname){
                has_conventions_attr = true;
                break;
            }

        }
        if(false==has_conventions_attr) {
            auto attr_unique = make_unique<Attribute>();
            auto attr = attr_unique.release();
            Add_Str_Attr(attr,conventions_attrname,conventions_attrvalue);
            this->root_attrs.push_back(attr);
        }
    }

}



// Adjust Dimension name
void EOS5File::Adjust_Dim_Name() 
{

    BESDEBUG("h5", "Coming to Adjust_Dim_Name()"<<endl);
    // No need if this is following COARDS.
    if (false == this->iscoard)
        return;
    else {
        for (const auto &cvar:this->cvars) {
            if (cvar->dims.size() != 1)
                throw3("Coard coordinate variable ", cvar->name, "is not 1D");
            if (cvar->newname != ((cvar->dims)[0]->newname)) {
                (cvar->dims)[0]->newname = cvar->newname;

                // For all variables that have this dimension,the dimension newname should also change.
                for (const auto &var:this->vars) {
                    for (const auto &dim:var->dims) {
                        // This is the key, the dimension name of this dimension 
                        // should be equal to the dimension name of the coordinate variable.
                        // Then the dimension name matches and the dimension name should be changed to
                        // the new dimension name.
                        if (dim->name == (cvar->dims)[0]->name) dim->newname = (cvar->dims)[0]->newname;
                    }
                }
            } // if (cvar->newname != ((cvar->dims)[0]->newname))
        } // for (auto irv = this->cvars.begin();
    } // else
}

// Add supplemental attributes such as origname and fullpath.
void EOS5File::Add_Supplement_Attrs(bool add_path) 
{

    BESDEBUG("h5", "Coming to Add_Supplement_Attrs()"<<endl);
    if (true == add_path) {

        File::Add_Supplement_Attrs(add_path);

        // Adding variable original name(origname) and full path(fullpath)
        for (const auto &cvar:this->cvars) {
            if ((cvar->cvartype == CV_EXIST) || (cvar->cvartype == CV_MODIFY)) {
                const string varname = cvar->name;
                const string attrname = "origname";
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_Str_Attr(attr, attrname, varname);
                cvar->attrs.push_back(attr);
            }
        }

        for (const auto &cvar:this->cvars) {
            if ((cvar->cvartype == CV_EXIST) || (cvar->cvartype == CV_MODIFY)) {
                // Turn off the fullnamepath attribute when zero_storage_size is 0.
                // Use the BES key since quite a few testing cases will be affected.
                // KY 2020-03-23
                if(cvar->zero_storage_size==false 
                    || HDF5RequestHandler::get_no_zero_size_fullnameattr() == false) {
                    const string varname = cvar->fullpath;
                    const string attrname = "fullnamepath";
                    auto attr_unique = make_unique<Attribute>();
                    auto attr = attr_unique.release();
                    Add_Str_Attr(attr, attrname, varname);
                    cvar->attrs.push_back(attr);
                }
            }
        }
    } // if(true == add_path)

    if (true == this->iscoard) {
        for (const auto &cvar:this->cvars) {
            if ((cvar->cvartype == CV_EXIST) || (cvar->cvartype == CV_MODIFY)) {
                const string attrname = "orig_dimname";
                string orig_dimname = ((cvar->dims)[0])->name;
                orig_dimname = HDF5CFUtil::obtain_string_after_lastslash(orig_dimname);
                if ("" == orig_dimname)
                    throw2("wrong dimension name ", orig_dimname);
                if (orig_dimname.find("FakeDim") != string::npos) orig_dimname = "";
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_Str_Attr(attr, attrname, orig_dimname);
                cvar->attrs.push_back(attr);
            }
        } // for (auto irv = this->cvars.begin()

        for (const auto &var:this->vars) {

            if (var->dims.empty() == false) {
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                if (1 == var->dims.size()) {
                    const string attrname = "orig_dimname";
                    string orig_dimname = ((var->dims)[0])->name;
                    if ("" == orig_dimname)
                        orig_dimname = "NoDimName";
                    else
                        orig_dimname = HDF5CFUtil::obtain_string_after_lastslash(orig_dimname);
                    if (orig_dimname.find("FakeDim") != string::npos) orig_dimname = "NoDimName";
                    Add_Str_Attr(attr, attrname, orig_dimname);
                }
                else {
                    const string attrname = "orig_dimname_list";
                    string orig_dimname_list;
                    for (auto ird = var->dims.begin(); ird != var->dims.end(); ++ird) {
                        string orig_dimname = (*ird)->name;
                        if ("" == orig_dimname)
                            orig_dimname = "NoDimName";
                        else
                            orig_dimname = HDF5CFUtil::obtain_string_after_lastslash((*ird)->name);
                        if (orig_dimname.find("FakeDim") != string::npos) orig_dimname = "NoDimName";
                        if ("" == orig_dimname_list)
                            orig_dimname_list = orig_dimname;
                        else
                            orig_dimname_list = orig_dimname_list + " " + orig_dimname;
#if 0
//                        orig_dimname_list = orig_dimname_list + " ";
#endif
                    }
                    Add_Str_Attr(attr, attrname, orig_dimname_list);
                }
                var->attrs.push_back(attr);
            }  
        } // for (auto irv = this->vars.begin();
    } // if(true == this->iscoard )

}

// Handle coordinate attributes.
void EOS5File::Handle_Coor_Attr()
{

    BESDEBUG("h5", "Coming to Handle_Coor_Attr()"<<endl);
    string co_attrname = "coordinates";
    string co_attrvalue;

    if (iscoard) return;

    for (const auto &var:this->vars) {

        for (const auto &dim:var->dims) {
            for (const auto &cvar:this->cvars) {
                if (dim->name == cvar->cfdimname)
                    co_attrvalue = (co_attrvalue.empty()) ? cvar->newname : co_attrvalue + " " + cvar->newname;
            }
        }
        if (false == co_attrvalue.empty()) {
            auto attr_unique = make_unique<Attribute>();
            auto attr = attr_unique.release();
            Add_Str_Attr(attr, co_attrname, co_attrvalue);
            var->attrs.push_back(attr);
        }
        co_attrvalue.clear();
    } // for (auto irv = this->vars.begin(); ...

    // We will check if 2dlatlon coordinate variables exist
    bool has_2dlatlon_cv = false;
    for (const auto &cvar:this->cvars) {
        if (true == cvar->is_2dlatlon) {
            has_2dlatlon_cv = true;
            break;
        }
    }

    if (true == has_2dlatlon_cv) {

        string dimname1;
        string dimname2;
        for (const auto &cvar:this->cvars) {
            if (true == cvar->is_2dlatlon) {
                dimname1 = ((cvar->dims)[0])->name;
                dimname2 = ((cvar->dims)[1])->name;
                break;
            }
        }

        int num_latlondims = 0;

        for (const auto &var:this->vars) {
            for (const auto &dim:var->dims) {
                if (dimname1 == dim->name) num_latlondims++;
                if (dimname2 == dim->name) num_latlondims++;
            }
            if ((num_latlondims != 0) && (num_latlondims != 2)) {
                // need to remove the coordinates attribute.
                for (auto ira = var->attrs.begin(); ira != var->attrs.end(); ++ira) {
                    if (co_attrname == (*ira)->name) {
                        delete (*ira);
                        var->attrs.erase(ira);
                        break;
                    }
                }
            }
            num_latlondims = 0;
        } // for (auto irv = this->vars.begin();
    } //  if (true == has_2dlatlon_cv)
}

// This function is from the original requirement of NASA, then
// NASA changes the requirment. Still leave it here for future usage.
#if 0
void EOS5File::Adjust_Special_EOS5CVar_Name()  {

    int num_grids =this->eos5cfgrids.size();
    int num_swaths = this->eos5cfswaths.size();
    int num_zas = this->eos5cfzas.size();

    bool mixed_eos5typefile = false;

    // Check if this file mixes grid,swath and zonal average
    if (((num_grids > 0) && (num_swaths > 0)) ||
        ((num_grids > 0) && (num_zas > 0)) ||
        ((num_swaths >0) && (num_zas > 0)))
    mixed_eos5typefile = true;

    if (false == mixed_eos5typefile) {

        // Grid is very special since all grids may share the same lat/lon.
        // so we also consider this case.

        if ((1 == num_swaths) || ( 1 == num_zas) ||
            (1 == num_grids) || ((num_grids >1) && (this->grids_multi_latloncvs))) {

            string unit_attrname = "units";
            string nonll_cf_level_attralue ="level";
            string lat_cf_unit_attrvalue ="degrees_north";
            string lon_cf_unit_attrvalue ="degrees_east";

            for (auto irv = this->cvars.begin();
                irv != this->cvars.end(); irv++) {
                switch((*irv)->eos_type) {
                    case CV_EXIST:
                    case CV_MODIFY:
                    case CV_LAT_MISS:
                    case CV_LON_MISS:
                    {
                        for(vector <Attribute*>::iterator ira = (*irv)->attrs.begin();
                            ira != (*irv)->attrs.end(); ira++) {
                            if ((*ira)->name ==unit_attrname) {
                                if ((*ira)->value.size() > 0) {
                                    string units_value((*ira)->value.begin(),(*ira)->value.end());
                                    if (lat_cf_unit_attrvalue ==units_value) (*irv)->newname = "lat";
                                    if (lon_cf_unit_attrvalue ==units_value) (*irv)->newname = "lon";
                                }
                            }
                        }
                    }
                    break;
                    case CV_NONLATLON_MISS:
                    {
                        for(vector <Attribute*>::iterator ira = (*irv)->attrs.begin();
                            ira != (*irv)->attrs.end(); ira++) {
                            if ((*ira)->name ==unit_attrname) {
                                if ((*ira)->value.size() > 0) {
                                    string units_value((*ira)->value.begin(),(*ira)->value.end());
                                    if (nonll_cf_level_attralue ==units_value) {
                                        (*irv)->newname = "lev";
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    break;
                    default:
                    throw1("Non-supported coordinate variable type");
                }
            }
        }
    }
}
#endif

// Create missing coordinate variables. Some NASA files don't provide coordinate
// variables for some dimensions. To make the visualization tools plot the data,
// we provide index number to be the coordinate variable values for these missing coordinate variables.
// These missing coordinate variables are all 1-D.
template<class T>
void EOS5File::Create_Missing_CV(T* eos5data, EOS5CVar *EOS5cvar, const string& dimname, EOS5Type eos5type,
    int num_eos5data) const
{

    BESDEBUG("h5", "Coming to Create_Missing_CV()"<<endl);
    string reduced_dimname = HDF5CFUtil::obtain_string_after_lastslash(dimname);
    if ("" == reduced_dimname) throw2("wrong dimension name ", dimname);
    EOS5cvar->name = reduced_dimname;
    Create_Added_Var_NewName_FullPath(eos5type, eos5data->name, EOS5cvar->name, EOS5cvar->newname, EOS5cvar->fullpath);
    EOS5cvar->rank = 1;
    EOS5cvar->dtype = H5INT32;
    hsize_t eos5cvar_dimsize = (eos5data->dimnames_to_dimsizes)[dimname];
    auto eos5cvar_dim_unique = make_unique<Dimension>(eos5cvar_dimsize);
    auto eos5cvar_dim = eos5cvar_dim_unique.release();
    eos5cvar_dim->name = dimname;
    eos5cvar_dim->unlimited_dim = (eos5data->dimnames_to_unlimited)[dimname];
    if (1 == num_eos5data)
        eos5cvar_dim->newname = reduced_dimname;
    else
        eos5cvar_dim->newname = dimname;

    EOS5cvar->dims.push_back(eos5cvar_dim);
    EOS5cvar->cfdimname = dimname;
    EOS5cvar->cvartype = CV_NONLATLON_MISS;
    EOS5cvar->eos_type = eos5type;
}

// Helper function for Create_Missing_CV
void EOS5File::Create_Added_Var_NewName_FullPath(EOS5Type eos5type, const string& eos5_groupname, const string& varname,
    string &var_newname, string &var_fullpath) const
{

    BESDEBUG("h5", "Coming to Create_Added_Var_NewName_FullPath()"<<endl);
    string fslash_str = "/";
    string eos5typestr;
    string top_eos5_groupname = "/HDFEOS";

    switch (eos5type) {
    case GRID: {
        eos5typestr = "/GRIDS/";
        var_newname = eos5typestr + eos5_groupname + fslash_str + varname;
        var_fullpath = top_eos5_groupname + eos5typestr + eos5_groupname + fslash_str + varname;
    }
        break;

    case SWATH: {
        eos5typestr = "/SWATHS/";
        var_newname = eos5typestr + eos5_groupname + fslash_str + varname;
        var_fullpath = top_eos5_groupname + eos5typestr + eos5_groupname + fslash_str + varname;

    }
        break;

    case ZA: {
        eos5typestr = "/ZAS/";
        var_newname = eos5typestr + eos5_groupname + fslash_str + varname;
        var_fullpath = top_eos5_groupname + eos5typestr + eos5_groupname + fslash_str + varname;

    }
        break;
    case OTHERVARS:
    default:
        throw1("Non-supported EOS type");
    }
}

// Handle special variables, various speical cases are handled here.
void EOS5File::Handle_SpVar() 
{

    BESDEBUG("h5", "Coming to Handle_SpVar()"<<endl);
    if (true == this->isaura && TES == this->aura_name) {
        const string ProHist_full_path = "/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES/ProductionHistory";
        for (auto irv = this->vars.begin(); irv != this->vars.end(); ++irv) {
            if (ProHist_full_path == (*irv)->fullpath) {
                delete (*irv);
                this->vars.erase(irv);
                break;
            }
        }
    }

    // First, if the duplicate dimension exists,
    if (dimname_to_dupdimnamelist.empty() == false) {
        for (auto ircv = this->cvars.begin(); ircv != this->cvars.end(); ircv++) {
            if ((*ircv)->cvartype == CV_EXIST) {
                pair<multimap<string, string>::iterator, multimap<string, string>::iterator> mm_er_ret;
                multimap<string, string>::iterator itmm;
                for (itmm = dimname_to_dupdimnamelist.begin(); itmm != dimname_to_dupdimnamelist.end(); ++itmm) {

                    // Find the original dimension(the coordinate variable)
                    if ((*ircv)->cfdimname == (*itmm).first) {

                        // Loop through the cv again,this time just check CV_NONLATLON_MISS
                        for (auto irv2 = this->cvars.begin(); irv2 != this->cvars.end();
                            irv2++) {
                            if ((*irv2)->cvartype == CV_NONLATLON_MISS) {
#if 0
                                //cerr<<"the duplicate cf dimension name "<<(*irv2)->cfdimname <<endl;
                                //if((*irv2)->cfdimname == (*ircv)->cfdimname) {
#endif
                                // Obtain the fake CV that has the duplicate dimension.
                                if ((*irv2)->cfdimname == (*itmm).second) {

                                    //find the duplicate dimension name 
                                    string dup_var_name = (*irv2)->newname;
                                    Replace_Var_Info_EOS((*ircv), (*irv2));
                                    // The following two lines are key to make sure duplicate CV
                                    //  using a different name but keep all other info.
                                    (*irv2)->newname = dup_var_name;
                                    (*irv2)->getDimensions()[0]->newname = dup_var_name;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // No need to loop through the variables. We just need to loop through the coordinate variables and check cfdimname.
#if 0
    // For the EOS case, Loop through every variable that has a  >=2 rank,
    for (auto irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        // Check if having the duplicate dimensions.
        if((*irv)->rank >=2) {
            // Loop through the dimensions
            for (auto ird = (*irv)->dims.begin();
                ird != (*irv)->dims.end(); ++ ird) {
                pair<multimap<string,string>::iterator,multimap<string,string>::iterator> mm_er_ret;
                multimap<string,string>::iterator itmm;
                for (itmm = dimname_to_dupdimnamelist.begin(); itmm!=dimname_to_dupdimnamelist.end();++itmm) {
//cerr<<"the original dim. name is "<<(*itmm).first <<endl;
//cerr<<"the duplicate dim. name is "<<(*itmm).second <<endl;
//if((*irv)->name == "RetrievalAveragingKernelMatrixDay")
                    cerr<<"duplicate dimension name of a variable is "<<(*ird)->name <<endl;
                    // Find the duplicated dim name in the dimname_to_dupdimnamelist,
                    // Now retrieve the dim. name and loop through all CV_EXIST variable to see if
                    // one CV_EXIST variable has a dimension of which name is the dim. name.
                    // If yes, loop through all CV_NONLLMISS variables and find the CV variable that has the
                    // duplicate dim. name. If found, replace this variable's information(except name and newname) with the
                    // fullpath of the CV_EXIST variable. In this way, the duplicate CV variable will read
                    // correctly the existing CV values and other information. This is the most complicate process.

//                        if((*itmm).second == HDF5CFUtil::obtain_string_after_lastslash((*ird)->name)) {
                    if((*itmm).second == (*ird)->name) {
                        cerr<<"coming to find the duplicate dim. name "<<endl;
                        for (auto ircv = this->cvars.begin(); ircv != this->cvars.end(); ircv++) {
                            if((*ircv)->cvartype == CV_EXIST) {
                                cerr<<"cf dim. name is "<<(*ircv)->cfdimname <<endl;
                                // Find the original dimension(the coordinate variable)
                                if((*ircv)->cfdimname == (*itmm).first) {
                                    // Loop through the cv again,this time just check CV_NONLATLON_MISS
                                    for (auto irv2 = this->cvars.begin();
                                        irv2 != this->cvars.end(); irv2++) {
                                        if((*irv2)->cvartype == CV_NONLATLON_MISS) {
                                            // Obtain the fake CV that has the duplicate dimension.
                                            if((*irv2)->cfdimname == (*itmm).second) {
                                                string dup_var_name = (*irv2)->newname;
                                                Replace_Var_Info_EOS((*ircv),(*irv2));
                                                (*irv2)->newname = dup_var_name;
                                                (*irv2)->getDimensions()[0]->newname = dup_var_name;
                                            }

                                        }
                                    }

                                }

                            }

                        }

                    }

                }

            }

        }
    }

}
#endif
}

// Handle special variable attributes
void EOS5File::Handle_SpVar_Attr() 
{

    BESDEBUG("h5", "Coming to Handle_SpVar_Attr()"<<endl);

    // First, if the duplicate dimension exists,
    if (dimname_to_dupdimnamelist.empty() == false) {

        pair<multimap<string, string>::iterator, multimap<string, string>::iterator> mm_er_ret;
        multimap<string, string>::iterator itmm;
        for (itmm = dimname_to_dupdimnamelist.begin(); itmm != dimname_to_dupdimnamelist.end(); ++itmm) {
            for (auto ircv = this->cvars.begin(); ircv != this->cvars.end(); ircv++) {
                // The duplicated CV must share with an existing coordinate variable
                if ((*ircv)->cvartype == CV_EXIST) {

                    // Find the original dimension(the coordinate variable)
                    if ((*ircv)->cfdimname == (*itmm).first) {

                        // Loop through the cv again,this time just check CV_NONLATLON_MISS
                        // The duplciated CV must be CV_NONLATLON_MISS.
                        for (auto irv2 = this->cvars.begin(); irv2 != this->cvars.end();
                            irv2++) {
                            if ((*irv2)->cvartype == CV_NONLATLON_MISS) {

                                // Obtain the fake CV that has the duplicate dimension.
#if 0
                                //if((*irv2)->cfdimname == (*ircv)->cfdimname) 
#endif
                                if ((*irv2)->cfdimname == (*itmm).second) Replace_Var_Attrs_EOS((*ircv), (*irv2));

                            }
                        }
                    } // if((*ircv)->cfdimname == (*itmm).first) 
                } // if((*ircv)->cvartype == CV_EXIST) 
            } // for (auto ircv = this->cvars.begin()
        } // for (itmm = dimname_to_dupdimnamelist.begin();
    } // if(dimname_to_dupdimnamelist.size() > 0)
}

// Handle special variables, various special cases are handled here.
void EOS5File::Handle_SpVar_DMR() 
{

    BESDEBUG("h5", "Coming to Handle_SpVar_DMR()"<<endl);
    if (true == this->isaura && TES == this->aura_name) {
        const string ProHist_full_path = "/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES/ProductionHistory";
        for (auto irv = this->vars.begin(); irv != this->vars.end(); ++irv) {
            if (ProHist_full_path == (*irv)->fullpath) {
                delete (*irv);
                this->vars.erase(irv);
                break;
            }
        }
    }

    // First, if the duplicate dimension exists,
    if (dimname_to_dupdimnamelist.empty() == false) {
        for (auto ircv = this->cvars.begin(); ircv != this->cvars.end(); ircv++) {
            if ((*ircv)->cvartype == CV_EXIST) {
                pair<multimap<string, string>::iterator, multimap<string, string>::iterator> mm_er_ret;
                multimap<string, string>::iterator itmm;
                for (itmm = dimname_to_dupdimnamelist.begin(); itmm != dimname_to_dupdimnamelist.end(); ++itmm) {

                    // Find the original dimension(the coordinate variable)
                    if ((*ircv)->cfdimname == (*itmm).first) {

                        // Loop through the cv again,this time just check CV_NONLATLON_MISS
                        for (auto irv2 = this->cvars.begin(); irv2 != this->cvars.end();
                            irv2++) {
                            if ((*irv2)->cvartype == CV_NONLATLON_MISS) {
#if 0
                                //cerr<<"the duplicate cf dimension name "<<(*irv2)->cfdimname <<endl;
                                //if((*irv2)->cfdimname == (*ircv)->cfdimname) {
#endif
                                // Obtain the fake CV that has the duplicate dimension.
                                if ((*irv2)->cfdimname == (*itmm).second) {

                                    Replace_Var_Attrs_EOS((*ircv), (*irv2));
                                    //find the duplicate dimension name 
                                    string dup_var_name = (*irv2)->newname;
                                    Replace_Var_Info_EOS((*ircv), (*irv2));
                                    
                                    // The following two lines are key to make sure duplicate CV
                                    //  using a different name but keep all other info.
                                    (*irv2)->newname = dup_var_name;
                                    (*irv2)->getDimensions()[0]->newname = dup_var_name;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

}


void EOS5File::Adjust_Obj_Name() 
{
    //Intentionally unimplemented, may have use cases for the future.
}

bool EOS5File::Have_Grid_Mapping_Attrs() {
    return File::Have_Grid_Mapping_Attrs();
}
void EOS5File::Handle_Grid_Mapping_Vars() {
    File:: Handle_Grid_Mapping_Vars();
}


// Sometimes need to replace information of a variable with the information of another variable.
void EOS5File::Replace_Var_Info_EOS(const EOS5CVar *src, EOS5CVar*target)
{

    BESDEBUG("h5", "Coming to Replace_Var_Info_EOS()"<<endl);
    File::Replace_Var_Info(src, target);
    target->cfdimname = src->cfdimname;
    target->cvartype = src->cvartype;
    target->eos_type = src->eos_type;
    target->total_elems = src->total_elems;

}

//Sometimes the attributes of a variable need to replace with the attribute of another variable.
void EOS5File::Replace_Var_Attrs_EOS(const EOS5CVar *src, EOS5CVar*target)
{

    BESDEBUG("h5", "Coming to Replace_Var_Attrs_EOS()"<<endl);
    File::Replace_Var_Attrs(src, target);

}

#if 0
void
EOS5File:: add_ignored_info_attrs(bool is_grp,bool is_first) {

}
void
EOS5File:: add_ignored_info_objs(bool is_dim_related, bool is_first) {

}
#endif

