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
/// \file HDF5GMCF.cc
/// \brief Implementation of the mapping of NASA generic HDF5 products to DAP by following CF
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
#include "h5apicompatible.h"
#include <BESDebug.h>
#include <algorithm>
#include <memory>

using namespace std;
using namespace libdap;
using namespace HDF5CF;

// Copier function.
GMCVar::GMCVar(const Var*var) {

    BESDEBUG("h5", "Coming to GMCVar()"<<endl);
    newname = var->newname;
    name = var->name;
    fullpath = var->fullpath;
    rank  = var->rank;
    total_elems = var->total_elems;
    zero_storage_size = var->zero_storage_size;
    dtype = var->dtype;
    unsupported_attr_dtype = var->unsupported_attr_dtype;
    unsupported_dspace = var->unsupported_dspace;
    coord_attr_add_path = false;
    
    for (const auto& vattr:var->attrs) {
        auto attr_unique = make_unique<Attribute>();
        auto attr = attr_unique.release();
        attr->name = vattr->name;
        attr->newname = vattr->newname;
        attr->dtype =vattr->dtype;
        attr->count =vattr->count;
        attr->strsize = vattr->strsize;
        attr->fstrsize = vattr->fstrsize;
        attr->value =vattr->value;
        attrs.push_back(attr);
    } 

    for (const auto& vdim:var->dims) {
        auto dim_unique = make_unique<Dimension>(vdim->size);
        auto dim = dim_unique.release();
        dim->name = vdim->name;
        dim->newname = vdim->newname;
        dim->unlimited_dim = vdim->unlimited_dim;
        dims.push_back(dim);
    } 

    product_type = General_Product;

}

//Copy function of a special variable.
GMSPVar::GMSPVar(const Var*var) {

    BESDEBUG("h5", "Coming to GMSPVar()"<<endl);
    fullpath = var->fullpath;
    rank  = var->rank;
    total_elems = var->total_elems;
    zero_storage_size = var->zero_storage_size;
    unsupported_attr_dtype = var->unsupported_attr_dtype;
    unsupported_dspace = var->unsupported_dspace;
    coord_attr_add_path = var->coord_attr_add_path;
    // The caller of this function should change the following fields.
    // This is just to make data coverity happy.
    otype = H5UNSUPTYPE;
    sdbit = -1;
    numofdbits = -1;

    for (const auto &vattr:var->attrs) {
        auto attr_unique = make_unique<Attribute>();
        auto attr = attr_unique.release();
        attr->name = vattr->name;
        attr->newname = vattr->newname;
        attr->dtype =vattr->dtype;
        attr->count =vattr->count;
        attr->strsize = vattr->strsize;
        attr->fstrsize = vattr->fstrsize;
        attr->value =vattr->value;
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


GMFile::GMFile(const char*file_fullpath, hid_t file_id, H5GCFProduct product_type, GMPattern gproduct_pattern):
File(file_fullpath,file_id), product_type(product_type),gproduct_pattern(gproduct_pattern)
{

}

// destructor
GMFile::~GMFile() 
{

    if (!this->cvars.empty()){
        for (auto i= this->cvars.begin(); i!=this->cvars.end(); ++i) {
           delete *i;
        }
    }

    if (!this->spvars.empty()){
        for (auto i= this->spvars.begin(); i!=this->spvars.end(); ++i) {
           delete *i;
        }
    }

}

// Get CF string
string GMFile::get_CF_string(string s) {

    // HDF5 group or variable path always starts with '/'. When CF naming rule is applied,
    // the first '/' is always changes to "_", this is not necessary. However,
    // to keep the backward compatability, I use a BES key for people to go back with the original name.

    if(s[0] !='/') 
        return File::get_CF_string(s);
    else if (General_Product == product_type &&  OTHERGMS == gproduct_pattern)  { 

        if(true == HDF5RequestHandler::get_keep_var_leading_underscore())
            return File::get_CF_string(s);
        else {
            s.erase(0,1);
            return  File::get_CF_string(s);
        }
    }
    else {
        // The leading underscore should be removed from all supported products
        s.erase(0,1);
        return File::get_CF_string(s);
    }
}

// Retrieve all the HDF5 information.
void GMFile::Retrieve_H5_Info(const char *file_fullpath,
                              hid_t file_id, bool include_attr) {

    BESDEBUG("h5", "Coming to Retrieve_H5_Info()"<<endl);
    // GPM needs the attribute info. to obtain the lat/lon.
    // So set the include_attr to be true for these products.
    if (product_type == Mea_SeaWiFS_L2 || product_type == Mea_SeaWiFS_L3
        || GPMS_L3 == product_type  || GPMM_L3 == product_type || GPM_L1 == product_type || OBPG_L3 == product_type
        || Mea_Ozone == product_type || General_Product == product_type)  
        File::Retrieve_H5_Info(file_fullpath,file_id,true);
    else 
        File::Retrieve_H5_Info(file_fullpath,file_id,include_attr);

}

// Update the product type. This is because the file structure may change across different versions of products
// I need to handle them differently and still support different versions. The goal is to support two versions in a row.
// Currently, GPM level 3 is changed.
// This routine should be called right after Retrieve_H5_Info.
void GMFile::Update_Product_Type()  {

    BESDEBUG("h5", "Coming to Update_Product_Type()"<<endl);
    if(GPMS_L3 == this->product_type || GPMM_L3 == this->product_type) {

        // Check Dimscale attributes 
#if 0
        //Check_General_Product_Pattern();
#endif
        Check_Dimscale_General_Product_Pattern();
        if(GENERAL_DIMSCALE == this->gproduct_pattern){
            if(GPMS_L3 == this->product_type) {
                for (auto &var:this->vars) 
                    var->newname = var->name;
            }
            this->product_type = General_Product;
        }
    }
    else if(General_Product == this->product_type)
        Check_General_Product_Pattern();
}

void GMFile::Remove_Unneeded_Objects()  {

    BESDEBUG("h5", "Coming to Remove_Unneeded_Objects()"<<endl);
    if(General_Product == this->product_type) {
        string file_path = this->path;
        if(HDF5CFUtil::obtain_string_after_lastslash(file_path).find("OMPS-NPP")==0) 
            Remove_OMPSNPP_InputPointers();
    }
    if((General_Product == this->product_type) && (GENERAL_DIMSCALE == this->gproduct_pattern)) {
        string nc4_non_coord="_nc4_non_coord_";
        size_t nc4_non_coord_size= nc4_non_coord.size();
        for (const auto &var:this->vars) {
            if(var->name.find(nc4_non_coord)==0) {
                // Here we need to store the path
                string temp_vname = HDF5CFUtil::obtain_string_after_lastslash(var->fullpath);
                string temp_vpath = HDF5CFUtil::obtain_string_before_lastslash(var->fullpath);
                string temp_vname_removed = temp_vname.substr(nc4_non_coord_size,temp_vname.size()-nc4_non_coord_size);
                string temp_v_fullpath_removed = temp_vpath + temp_vname_removed;
                nc4_sdimv_dv_path.insert(temp_v_fullpath_removed);
            }
        }

        if(nc4_sdimv_dv_path.empty()==false)
            this->have_nc4_non_coord = true;
 

        if (true == this->have_nc4_non_coord) {
            for (auto irv = this->vars.begin(); irv != this->vars.end();) {
                if(nc4_sdimv_dv_path.find((*irv)->fullpath)!=nc4_sdimv_dv_path.end()){
                    delete(*irv);
                    irv=this->vars.erase(irv);
                }
                else 
                    ++irv;
            }
        }

    }
    else if(GPM_L3_New == this->product_type) {
        for (auto irg = this->groups.begin();  irg != this->groups.end(); ) {
            if((*irg)->attrs.empty()) {
                delete(*irg);
                irg = this->groups.erase(irg);

            }
            else 
                ++irg;
        }
    }
}

void GMFile::Remove_OMPSNPP_InputPointers()  {
    // Here I don't check whether this is a netCDF file by 
    // using Check_Dimscale_General_Product_Pattern() to see if it returns true.
    // We will see if we need this.
    for (auto irg = this->groups.begin(); irg != this->groups.end(); ) {
        if((*irg)->path.find("/InputPointers")==0) {
            delete(*irg);
            irg = this->groups.erase(irg);

        }
        else 
            ++irg;
    }

    for (auto irv = this->vars.begin(); irv != this->vars.end(); ) {
         if((*irv)->fullpath.find("/InputPointers")==0) {
            delete(*irv);
            irv = this->vars.erase(irv);

        }
        else 
            ++irv;
    }
}
void GMFile::Retrieve_H5_CVar_Supported_Attr_Values() {

    for (const auto &cvar:this->cvars) {
          
        if (cvar->cvartype != CV_NONLATLON_MISS){
            for (auto & attr:cvar->attrs) {
                Retrieve_H5_Attr_Value(attr,cvar->fullpath);
            }
        }
    }
}

// Retrieve HDF5 supported attribute values.
void GMFile::Retrieve_H5_Supported_Attr_Values()  {

    BESDEBUG("h5", "Coming to Retrieve_H5_Supported_Attr_Values()"<<endl);

    // General attributes
    File::Retrieve_H5_Supported_Attr_Values();

    //Coordinate variable attributes 
    Retrieve_H5_CVar_Supported_Attr_Values();

    // Special variable attributes
    for (const auto &spv:this->spvars) {
        for (auto &attr:spv->attrs) {
            Retrieve_H5_Attr_Value(attr,spv->fullpath);
            Adjust_H5_Attr_Value(attr);
        }
    }
}

// Adjust attribute values. Currently, this is only for ACOS and OCO2.
// Reason: DAP2 doesn't support 64-bit integer, and they have 64-bit integer data
// in these files. Chop them to two 32-bit integers following the data producer's information.
void GMFile::Adjust_H5_Attr_Value(Attribute *attr)  const{

    BESDEBUG("h5", "Coming to Adjust_H5_Attr_Value()"<<endl);
    if (product_type == ACOS_L2S_OR_OCO2_L1B) {
        if (("Type" == attr->name) && (H5VSTRING == attr->dtype)) {
            string orig_attrvalues(attr->value.begin(),attr->value.end());
            if (orig_attrvalues != "Signed64") return;
            string new_attrvalues = "Signed32";
            // Since the new_attrvalues size is the same as the orig_attrvalues size
            // No need to adjust the strsize and fstrsize. KY 2011-2-1
            attr->value.clear();
            attr->value.resize(new_attrvalues.size());
            copy(new_attrvalues.begin(),new_attrvalues.end(),attr->value.begin()); 
        }
    } 
}

// Unsupported datatype
void GMFile:: Handle_Unsupported_Dtype(bool include_attr) {

    BESDEBUG("h5", "Coming to Handle_Unsupported_Dtype()"<<endl);
    if(true == check_ignored) {
        Gen_Unsupported_Dtype_Info(include_attr);
    }
    File::Handle_Unsupported_Dtype(include_attr);
    Handle_GM_Unsupported_Dtype(include_attr);
}

// Unsupported datatype for general data products.
void GMFile:: Handle_GM_Unsupported_Dtype(bool include_attr)  {

    BESDEBUG("h5", "Coming to Handle_GM_Unsupported_Dtype()"<<endl);
    for (auto ircv = this->cvars.begin(); ircv != this->cvars.end(); ) {
        if (true == include_attr) {
            for (auto ira = (*ircv)->attrs.begin(); ira != (*ircv)->attrs.end(); ) {
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
        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4)) {
            
            // This may need to be checked carefully in the future,
            // My current understanding is that the coordinate variable can
            // be ignored if the corresponding variable is ignored. 
            // Currently, we don't find any NASA files in this category.
            // KY 2012-5-21
            delete (*ircv);
            ircv = this->cvars.erase(ircv);
        }
        else {
            ++ircv;
        }
       
    } 

    for (auto ircv = this->spvars.begin(); ircv != this->spvars.end(); ) {
        if (true == include_attr) {
            for (auto ira = (*ircv)->attrs.begin(); ira != (*ircv)->attrs.end(); ) {
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
        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4)) {
            delete (*ircv);
            ircv = this->spvars.erase(ircv);
        }
        else {
            ++ircv;
        }
            
    }
}

// Datatype ignore information.
void GMFile:: Gen_Unsupported_Dtype_Info(bool include_attr) {

    BESDEBUG("h5", "GMFile::Coming to Gen_Unsupported_Dtype_Info()"<<endl);
    if(true == include_attr) {

        File::Gen_Group_Unsupported_Dtype_Info();
        File::Gen_Var_Unsupported_Dtype_Info();
        Gen_VarAttr_Unsupported_Dtype_Info();
    }

}

// Datatype ignored information for variable attributes
void GMFile:: Gen_VarAttr_Unsupported_Dtype_Info() {

     BESDEBUG("h5", "GMFile::Coming to Gen_Unsupported_Dtype_Info()"<<endl);
    // First general variables(non-CV and non-special variable) that use dimension scales.
    if((General_Product == this->product_type && GENERAL_DIMSCALE== this->gproduct_pattern) 
           || (Mea_Ozone == this->product_type)  || (Mea_SeaWiFS_L2 == this->product_type) || (Mea_SeaWiFS_L3 == this->product_type)
          || (OBPG_L3 == this->product_type)) {
        Gen_DimScale_VarAttr_Unsupported_Dtype_Info();
    }
         
    else 
        File::Gen_VarAttr_Unsupported_Dtype_Info();

    // CV and special variables
    Gen_GM_VarAttr_Unsupported_Dtype_Info();
 
}

// Generate ignored object,attribute information for the CVs and special variables of general supported products.
void GMFile:: Gen_GM_VarAttr_Unsupported_Dtype_Info(){

     BESDEBUG("h5", "GMFile::Coming to Gen_GM_VarAttr_Unsupported_Dtype_Info()"<<endl);
    if((General_Product == this->product_type && GENERAL_DIMSCALE== this->gproduct_pattern)
        || (Mea_Ozone == this->product_type)  || (Mea_SeaWiFS_L2 == this->product_type) || (Mea_SeaWiFS_L3 == this->product_type)
        || (OBPG_L3 == this->product_type)) {

        for (const auto &cvar:this->cvars) {
            // If the attribute REFERENCE_LIST comes with the attribute CLASS, the
            // attribute REFERENCE_LIST is okay to ignore. No need to report.
            bool is_ignored = ignored_dimscale_ref_list(cvar);
            if (false == cvar->attrs.empty()) {
                for (const auto &attr:cvar->attrs) {
                    H5DataType temp_dtype = attr->getType();
                    // TODO: check why 64-bit integer is included here.
                    if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4) || temp_dtype == H5INT64 || temp_dtype == H5UINT64) {
                        // "DIMENSION_LIST" is okay to ignore and "REFERENCE_LIST"
                        // is okay to ignore if the variable has another attribute
                        // CLASS="DIMENSION_SCALE"
                        if (("DIMENSION_LIST" !=attr->name) &&
                            ("REFERENCE_LIST" != attr->name || true == is_ignored))
                            this->add_ignored_info_attrs(false,cvar->fullpath,attr->name);
                    }
                }
            }  
        } 

        for (const auto&spvar:this->spvars) {
            // If the attribute REFERENCE_LIST comes with the attribute CLASS, the
            // attribute REFERENCE_LIST is okay to ignore. No need to report.
            bool is_ignored = ignored_dimscale_ref_list(spvar);
            if (false == spvar->attrs.empty()) {
#if 0
                //if (true == spvar->unsupported_attr_dtype) 
#endif
                for (const auto &attr:spvar->attrs) {
                    H5DataType temp_dtype = attr->getType();
                    //TODO; check why 64-bit integer is included here.
                    if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4) || temp_dtype == H5INT64 || temp_dtype == H5UINT64) {
                        // "DIMENSION_LIST" is okay to ignore and "REFERENCE_LIST"
                        // is okay to ignore if the variable has another attribute
                        // CLASS="DIMENSION_SCALE"
                        if (("DIMENSION_LIST" !=attr->name) &&
                            ("REFERENCE_LIST" != attr->name || true == is_ignored))
                            this->add_ignored_info_attrs(false,spvar->fullpath,attr->name);
                    }
                }
            }   
        }    
    }// "if((General_Product == ......)"
    else {
        for (const auto &cvar:this->cvars) {
            if (false == cvar->attrs.empty()) {
#if 0
                //if (true == cvar->unsupported_attr_dtype) 
#endif
                for (const auto &attr:cvar->attrs) {
                    H5DataType temp_dtype = attr->getType();
                    // TODO: check why 64-bit integer is included here.
                    if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4) || temp_dtype == H5INT64 || temp_dtype == H5UINT64) 
                          this->add_ignored_info_attrs(false,cvar->fullpath,attr->name);
                }
            }
        }

        for (const auto &spvar:this->spvars) {
            if (false == spvar->attrs.empty()) {
#if 0
                //if (true == spvar->unsupported_attr_dtype) 
#endif
                for (const auto &attr:spvar->attrs) {
                    H5DataType temp_dtype = attr->getType();
                    //TODO: check why 64-bit integer is included here.
                    if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype,_is_dap4) || temp_dtype == H5INT64 || temp_dtype == H5UINT64) {
                        this->add_ignored_info_attrs(false,spvar->fullpath,attr->name);
                    }
                }
            }
        }

    }// else

}

// Unsupported data space
void GMFile:: Handle_Unsupported_Dspace(bool include_attr)  {

    BESDEBUG("h5", "Coming to GMFile:Handle_Unsupported_Dspace()"<<endl);
    if(true == check_ignored)
        Gen_Unsupported_Dspace_Info();

    File::Handle_Unsupported_Dspace(include_attr);
    Handle_GM_Unsupported_Dspace(include_attr);
    
}

// Unsupported data space for coordinate variables and special variables of general products
void GMFile:: Handle_GM_Unsupported_Dspace(bool include_attr)  {

    BESDEBUG("h5", "Coming to GMFile:Handle_GM_Unsupported_Dspace()"<<endl);
    if(true == this->unsupported_var_dspace) {
        for (auto ircv = this->cvars.begin(); ircv != this->cvars.end(); ) {
            if (true  == (*ircv)->unsupported_dspace ) {
            
                // This may need to be checked carefully in the future,
                // My current understanding is that the coordinate variable can
                // be ignored if the corresponding variable is ignored. 
                // Currently, we don't find any NASA files in this category.
                // KY 2012-5-21
                delete (*ircv);
                ircv = this->cvars.erase(ircv);
            }
            else {
                ++ircv;
            }
        } 

        for (auto ircv = this->spvars.begin(); ircv != this->spvars.end(); ) {
            if (true == (*ircv)->unsupported_dspace) {
                delete (*ircv);
                ircv = this->spvars.erase(ircv);
            }
            else {
                ++ircv;
            }
            
        }
    }// if

    if(true == include_attr) {
        if(true == this->unsupported_var_attr_dspace) {
            for (auto &cvar:this->cvars) {
                if (false == cvar->attrs.empty()) {
                    if (true == cvar->unsupported_attr_dspace) {
                        for (auto ira = cvar->attrs.begin(); ira != cvar->attrs.end(); ) {
                            if (0 == (*ira)->count) {
                                delete (*ira);
                                ira = cvar->attrs.erase(ira);
                            }
                            else {
                                ++ira;
                            }
                        }
                    }
                }
            }

            for (auto &spvar:this->spvars) {
                if (false == spvar->attrs.empty()) {
                    if (true == spvar->unsupported_attr_dspace) {
                        for (auto ira = spvar->attrs.begin(); ira != spvar->attrs.end(); ) {
                            if (0 == (*ira)->count) {
                                delete (*ira);
                                ira = spvar->attrs.erase(ira);
                            }
                            else {
                                ++ira;
                            }
                        }
                    }
                }
            } // for 
        }// if
    }// if

}

// Generate unsupported data space information
void GMFile:: Gen_Unsupported_Dspace_Info() {

    // Leave like this since we may add more info. in this method later. KY 2022-12-08
    File::Gen_Unsupported_Dspace_Info();

}

// Handle other unsupported objects
void GMFile:: Handle_Unsupported_Others(bool include_attr)  {

    BESDEBUG("h5", "Coming to GMFile:Handle_Unsupported_Others()"<<endl);
    File::Handle_Unsupported_Others(include_attr);

    // Add the removal of CLASS=DIM_SCALE attribute if this is a netCDF-4-like attribute.
    //
    if(General_Product != this->product_type 
        || (General_Product == this->product_type && OTHERGMS != this->gproduct_pattern)){
   //
#if 0
    if((General_Product == this->product_type && GENERAL_DIMSCALE== this->gproduct_pattern)
        || (Mea_Ozone == this->product_type)  || (Mea_SeaWiFS_L2 == this->product_type) 
        || (Mea_SeaWiFS_L3 == this->product_type)
        || (OBPG_L3 == this->product_type)) 
#endif
      remove_netCDF_internal_attributes(include_attr);
      if(include_attr == true) {
            // We also need to remove the _nc3_strict from the root attributes
            for (auto ira = this->root_attrs.begin(); ira != this->root_attrs.end();)  {

                if((*ira)->name == "_nc3_strict") {
                    delete(*ira);
                    ira =this->root_attrs.erase(ira);
                    //If we have other root attributes to remove, remove the break statement.
                }
                else if((*ira)->name == "_NCProperties") {
                    delete(*ira);
                    ira =this->root_attrs.erase(ira);
                }
                else if((*ira)->name == "_Netcdf4Coordinates") {
                        delete(*ira);
                        ira =this->root_attrs.erase(ira);
                }

                else {
                    ++ira;
                }
            }
            for (const auto &cvar:this->cvars) {
                for(auto ira = cvar->attrs.begin(); ira != cvar->attrs.end();) {
                    if((*ira)->name == "CLASS") {
                        string class_value = Retrieve_Str_Attr_Value(*ira,cvar->fullpath);

                        // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
                        // "DIMENSION_SCALE", which is 15.
                        if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                            delete(*ira);
                            ira = cvar->attrs.erase(ira);
                            // Add another block to set a key
                        }
                        else {
                            ++ira;
                        }
                    }
                    else if((*ira)->name == "NAME") {// Add a BES Key later
                        delete(*ira);
                        ira =cvar->attrs.erase(ira);
                        //"NAME" attribute causes the file netCDF-4 failed.
#if 0

                        string name_value = Retrieve_Str_Attr_Value(*ira,cvar->fullpath);
                        if( 0 == name_value.compare(0,cvar->name.size(),cvar->name)) {
                            delete(*ira);
                            ira =cvar->attrs.erase(ira);
                        }
                        else {
                            string netcdf_dim_mark= "This is a netCDF dimension but not a netCDF variable";
                            if( 0 == name_value.compare(0,netcdf_dim_mark.size(),netcdf_dim_mark)) {
                                delete((*ira));
                                ira =cvar->attrs.erase(ira);
                            }
                            else {
                                ++ira;
                            }
                        }
#endif
                    }
                    else if((*ira)->name == "_Netcdf4Dimid") {
                        delete(*ira);
                        ira =cvar->attrs.erase(ira);
                    }
                    else if((*ira)->name == "_Netcdf4Coordinates") {
                        delete(*ira);
                        ira =cvar->attrs.erase(ira);
                    }

#if 0
                    else if((*ira)->name == "_nc3_strict") {
                        delete((*ira));
                        ira =cvar->attrs.erase(ira);
                    }
#endif
                    else {
                        ++ira;
                    }
                }
            }
        }
    }
    // netCDF Java lifts the string size limitation. All the string attributes can be 
    // represented by netCDF Java. So comment out the code. KY 2018/08/10
#if 0
    if(true == this->check_ignored && true == include_attr) {
        if(true == HDF5RequestHandler::get_drop_long_string()){
            for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                 irv != this->cvars.end(); ++irv) {
                for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end();++ira) {
                    if(true == Check_DropLongStr((*irv),(*ira))) {
                        this->add_ignored_droplongstr_hdr();
                        this->add_ignored_var_longstr_info((*irv),(*ira));
                    }
                }
            }

            for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
                irv != this->spvars.end(); ++irv) {
                for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end();++ira) {
                    if(true == Check_DropLongStr((*irv),(*ira))) {
                        this->add_ignored_droplongstr_hdr();
                        this->add_ignored_var_longstr_info((*irv),(*ira));
                    }
                }
 
            }
        }
    }
#endif

    if(false == this->have_ignored)
        this->add_no_ignored_info();

}

// Add dimension names
void GMFile::Add_Dim_Name() {
    
    BESDEBUG("h5", "Coming to GMFile:Add_Dim_Name()"<<endl);
    switch(product_type) {
        case Mea_SeaWiFS_L2:
        case Mea_SeaWiFS_L3:
            Add_Dim_Name_Mea_SeaWiFS();
            break;
        case Aqu_L3:
            Add_Dim_Name_Aqu_L3();
            break;
        case OSMAPL2S:
            Add_Dim_Name_OSMAPL2S();
            break;
        case ACOS_L2S_OR_OCO2_L1B:
            Add_Dim_Name_ACOS_L2S_OCO2_L1B();
            break;
        case Mea_Ozone:
            Add_Dim_Name_Mea_Ozonel3z();
            break;
        case GPMS_L3:
        case GPMM_L3:
        case GPM_L1:
        case GPM_L3_New:
            Add_Dim_Name_GPM();
            break;
        case OBPG_L3:
            Add_Dim_Name_OBPG_L3();
            break;
        case General_Product:
            Add_Dim_Name_General_Product();
            break;
        default:
           throw1("Cannot generate dim. names for unsupported datatype");
    } 

// Just for debugging
#if 0
for (vector<Var*>::iterator irv2 = this->vars.begin();
    irv2 != this->vars.end(); irv2++) {
    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
         ird !=(*irv2)->dims.end(); ird++) {
         cerr<<"Dimension name afet Add_Dim_Name "<<(*ird)->newname <<endl;
    }
}
#endif

}

//Add Dim. Names for OBPG level 3 product
void GMFile::Add_Dim_Name_OBPG_L3()  {

    BESDEBUG("h5", "Coming to Add_Dim_Name_OBPG_L3()"<<endl);
    // netCDF-4 like structure
    // Note: We need to change the product type to netCDF-4 like product type and pattern.
    Check_General_Product_Pattern();
    Add_Dim_Name_General_Product();
}

//Add Dim. Names for MeaSures SeaWiFS. Future: May combine with the handling of netCDF-4 products  
void GMFile::Add_Dim_Name_Mea_SeaWiFS() {

    BESDEBUG("h5", "Coming to Add_Dim_Name_Mea_SeaWiFS()"<<endl);
    pair<set<string>::iterator,bool> setret;
    if (Mea_SeaWiFS_L3 == product_type) 
        iscoard = true;
    for (const auto &var:this->vars) {
        Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(var);
        for (const auto &dim:var->dims) {
            setret = dimnamelist.insert(dim->name);
            if (true == setret.second) 
                Insert_One_NameSizeMap_Element(dim->name,dim->size,dim->unlimited_dim);
        }
    } 
 
    if (true == dimnamelist.empty()) 
        throw1("This product should have the dimension names, but no dimension names are found");
}    

// Handle Dimension scales for MEasUREs SeaWiFS and OZone.
void GMFile::Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(Var* var)
{

    BESDEBUG("h5", "Coming to Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone()"<<endl);
    Attribute* dimlistattr = nullptr;
    bool has_dimlist = false;
    bool has_class = false;
    bool has_reflist = false;

    for (const auto &attr:var->attrs) {
        if ("DIMENSION_LIST" == attr->name) {
           dimlistattr = attr;
           has_dimlist = true;  
        }
        if ("CLASS" == attr->name) 
            has_class = true;
        if ("REFERENCE_LIST" == attr->name) 
            has_reflist = true;
        
        if (true == has_dimlist) 
            break;
        if (true == has_class && true == has_reflist) 
            break; 
    } 

    if (true == has_dimlist) 
        Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(var,dimlistattr);

    // Dim name is the same as the variable name for dimscale variable
    else if(true == has_class && true == has_reflist) {
        if (var->dims.size() !=1) 
           throw2("dimension scale dataset must be 1 dimension, this is not true for variable ",
                  var->name);

        // The var name is the object name, however, we would like the dimension name to be full path.
        // so that the dim name can be served as the key for future handling.
        (var->dims)[0]->name = var->fullpath;
        (var->dims)[0]->newname = var->fullpath;
        pair<set<string>::iterator,bool> setret;
        setret = dimnamelist.insert((var->dims)[0]->name);
        if (true == setret.second) 
            Insert_One_NameSizeMap_Element((var->dims)[0]->name,(var->dims)[0]->size,(var->dims)[0]->unlimited_dim);
    }

    // No dimension, add fake dim names, this may never happen for MeaSure
    // but just for coherence and completeness.
    // For Fake dimension
    else {

        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        for (auto &dim:var->dims) {
            Add_One_FakeDim_Name(dim);
            setsizeret = fakedimsize.insert(dim->size);
            if (false == setsizeret.second)   
                Adjust_Duplicate_FakeDim_Name(dim);
        }
// Just for debugging
#if 0
        for (int i = 0; i < var->dims.size(); ++i) {
            Add_One_FakeDim_Name((var->dims)[i]);
            bool gotoMainLoop = false;
                for (int j =i-1; j>=0 && !gotoMainLoop; --j) {
                    if (((var->dims)[i])->size == ((var->dims)[j])->size){
                        Adjust_Duplicate_FakeDim_Name((var->dims)[i]);
                        gotoMainLoop = true;
                    }
                }
        }
#endif
        
    }//end of else
}

// Helper function to support dimensions of MeaSUrES SeaWiFS and OZone products
void GMFile::Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(const Var *var,const Attribute*dimlistattr) 
{
    
    BESDEBUG("h5", "Coming to Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone()"<<endl);
    ssize_t objnamelen = -1;
    hobj_ref_t rbuf;

    vector<hvl_t> vlbuf;
    
    hid_t dset_id = -1;
    hid_t attr_id = -1;
    hid_t atype_id = -1;
    hid_t amemtype_id = -1;
    hid_t aspace_id = -1;
    hid_t ref_dset = -1;


    if(nullptr == dimlistattr) 
        throw2("Cannot obtain the dimension list attribute for variable ",var->name);

    if (0==var->rank) 
        throw2("The number of dimension should NOT be 0 for the variable ",var->name);
    
    try {

        vlbuf.resize(var->rank);
    
        dset_id = H5Dopen(this->fileid,(var->fullpath).c_str(),H5P_DEFAULT);
        if (dset_id < 0) 
            throw2("Cannot open the dataset ",var->fullpath);

        attr_id = H5Aopen(dset_id,(dimlistattr->name).c_str(),H5P_DEFAULT);
        if (attr_id <0 ) 
            throw4("Cannot open the attribute ",dimlistattr->name," of HDF5 dataset ",var->fullpath);

        atype_id = H5Aget_type(attr_id);
        if (atype_id <0) 
            throw4("Cannot obtain the datatype of the attribute ",dimlistattr->name," of HDF5 dataset ",var->fullpath);

        amemtype_id = H5Tget_native_type(atype_id, H5T_DIR_ASCEND);

        if (amemtype_id < 0) 
            throw2("Cannot obtain the memory datatype for the attribute ",dimlistattr->name);


        if (H5Aread(attr_id,amemtype_id,vlbuf.data()) <0)
            throw2("Cannot obtain the referenced object for the variable ",var->name);
        

        vector<char> objname;
        int vlbuf_index = 0;

        // The dimension names of variables will be the HDF5 dataset names dereferenced from the DIMENSION_LIST attribute.
        for (auto &vdim:var->dims) {

            if(vlbuf[vlbuf_index].p== nullptr) 
                throw4("The dimension doesn't exist. Var name is ",var->name,"; the dimension index is ",vlbuf_index);

            rbuf =((hobj_ref_t*)vlbuf[vlbuf_index].p)[0];
            if ((ref_dset = H5RDEREFERENCE(attr_id, H5R_OBJECT, &rbuf)) < 0) 
                throw2("Cannot dereference from the DIMENSION_LIST attribute  for the variable ",var->name);
            // The above code works with dereferencing references generated with new H5R(H5R_ref_t) APIs with 1.12 and 1.13.
            // However, in case this h5rdeference2 API stops working with new APIs, the following #if 0 #endif block is a 
            // way to handle this issue.
#if 0
     
            rbuf =((hobj_ref_t*)vl_ref[0].p)[0];
            H5E_BEGIN_TRY {
                dset1 =  H5Rdereference2(attr_id,H5P_DEFAULT,H5R_OBJECT,&ds_ref_buf);
            } H5E_END_TRY;

            H5R_ref_t new_rbuf =((H5R_ref_t*)vlbuf[vlbuf_index].p)[0];
            if ((ref_dset = H5Ropen_object((H5R_ref_t *)&new_rbuf, H5P_DEFAULT, H5P_DEFAULT))<0)
                throw2("Cannot dereference from the DIMENSION_LIST attribute  for the variable ",var->name);
            H5Rdestroy(&new_rbuf);

#endif
            if ((objnamelen= H5Iget_name(ref_dset,nullptr,0))<=0) 
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);
            objname.resize(objnamelen+1);
            if ((objnamelen= H5Iget_name(ref_dset,objname.data(),objnamelen+1))<=0)
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);

            auto objname_str = string(objname.begin(),objname.end());
            string trim_objname = objname_str.substr(0,objnamelen);
            vdim->name = string(trim_objname.begin(),trim_objname.end());

            pair<set<string>::iterator,bool> setret;
            setret = dimnamelist.insert(vdim->name);
            if (true == setret.second) 
               Insert_One_NameSizeMap_Element(vdim->name,vdim->size,vdim->unlimited_dim);
            vdim->newname = vdim->name;
            H5Dclose(ref_dset);
            objname.clear();
            vlbuf_index++;
        }// end of for 

        if(vlbuf.empty()==false) {

            if ((aspace_id = H5Aget_space(attr_id)) < 0)
                throw2("Cannot get hdf5 dataspace id for the attribute ",dimlistattr->name);

            if (H5Dvlen_reclaim(amemtype_id,aspace_id,H5P_DEFAULT,(void*)vlbuf.data())<0)
                throw2("Cannot successfully clean up the variable length memory for the variable ",var->name);

            H5Sclose(aspace_id);
           
        }

        H5Tclose(atype_id);
        H5Tclose(amemtype_id);
        H5Aclose(attr_id);
        H5Dclose(dset_id);
    
    }

    catch(...) {

        if(atype_id != -1)
            H5Tclose(atype_id);

        if(amemtype_id != -1)
            H5Tclose(amemtype_id);

        if(aspace_id != -1)
            H5Sclose(aspace_id);

        if(attr_id != -1)
            H5Aclose(attr_id);

        if(dset_id != -1)
            H5Dclose(dset_id);

        throw;
    }
 
}

// Add MeaSURES OZone level 3Z dimension names
void GMFile::Add_Dim_Name_Mea_Ozonel3z() {

    BESDEBUG("h5", "Coming to Add_Dim_Name_Mea_Ozonel3z()"<<endl);
    iscoard = true;
    bool use_dimscale = false;

    for (const auto &grp:this->groups) {
        if ("/Dimensions" == grp->path) {
            use_dimscale = true;
            break;
        }
    }

    if (false == use_dimscale) {

        bool has_dimlist = false;
        bool has_class = false;
        bool has_reflist = false;

        for (const auto &var:this->vars) {
            for(const auto &attr:var->attrs) {
                if ("DIMENSION_LIST" == attr->name) 
                    has_dimlist = true;  
            }
            if (true == has_dimlist) 
                break;
        }

        if (true == has_dimlist) {

            for (const auto &var:this->vars) {

                for (const auto &attr:var->attrs) {
                    if ("CLASS" == attr->name) 
                        has_class = true;
                    if ("REFERENCE_LIST" == attr->name) 
                        has_reflist = true;
                    if (true == has_class && true == has_reflist) 
                        break; 
                } 
                if (true == has_class && 
                    true == has_reflist) 
                    break;
            
            } 
            if (true == has_class && true == has_reflist) 
                use_dimscale = true;
        } // end of if (true == has_dimlist)
    } // end of if (false == use_dimscale)

    if (true == use_dimscale) {

        pair<set<string>::iterator,bool> setret;
        for (const auto &var:this->vars) {
            Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(var);
            for (const auto &dim:var->dims) {  
                setret = dimnamelist.insert(dim->name);
                if(true == setret.second) 
                    Insert_One_NameSizeMap_Element(dim->name,dim->size,dim->unlimited_dim);
            }
        }
 
        if (true == dimnamelist.empty()) 
            throw1("This product should have the dimension names, but no dimension names are found");
    } // end of if (true == use_dimscale)    

    else {

        // Since the dim. size of each dimension of 2D lat/lon may be the same, so use multimap.
        multimap<hsize_t,string> ozonedimsize_to_dimname;
        pair<multimap<hsize_t,string>::iterator,multimap<hsize_t,string>::iterator> mm_er_ret;
        multimap<hsize_t,string>::iterator irmm;
 
        for (const auto &var:this->vars) {
            bool is_cv = check_cv(var->name);
            if (true == is_cv) {
                if (var->dims.size() != 1)
                    throw3("The coordinate variable", var->name," must be one dimension for the zonal average product");
                ozonedimsize_to_dimname.insert(pair<hsize_t,string>((var->dims)[0]->size,var->fullpath));
            }
        }// end of for 

        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        pair<set<string>::iterator,bool> setret;
        pair<set<string>::iterator,bool> tempsetret;
        set<string> tempdimnamelist;
        bool fakedimflag = false;

        for (const auto &var:this->vars) {

            for (auto &dim:var->dims) {

                fakedimflag = true;
                mm_er_ret = ozonedimsize_to_dimname.equal_range(dim->size);
                for (irmm = mm_er_ret.first; irmm!=mm_er_ret.second;irmm++) {
                    setret = tempdimnamelist.insert(irmm->second);
                    if (true == setret.second) {
                       dim->name = irmm->second;
                       dim->newname = dim->name;
                       setret = dimnamelist.insert(dim->name);
                       if(setret.second) Insert_One_NameSizeMap_Element(dim->name,dim->size,dim->unlimited_dim);
                       fakedimflag = false;
                       break;
                    }
                }
                      
                if (true == fakedimflag) {
                     Add_One_FakeDim_Name(dim);
                     setsizeret = fakedimsize.insert(dim->size);
                     if (false == setsizeret.second)  
                        Adjust_Duplicate_FakeDim_Name(dim);
                }
            
            } // end of for 
            tempdimnamelist.clear();
            fakedimsize.clear();
        } // end of for 
    } // end of else
}

// This is a special helper function for MeaSURES ozone products
bool GMFile::check_cv(const string & varname) const {

     BESDEBUG("h5", "Coming to check_cv()"<<endl);
     const string lat_name ="Latitude";
     const string time_name ="Time";
     const string ratio_pressure_name ="MixingRatioPressureLevels";
     const string profile_pressure_name ="ProfilePressureLevels";
     const string wave_length_name ="Wavelength";

     if (lat_name == varname) 
        return true;
     else if (time_name == varname) 
        return true;
     else if (ratio_pressure_name == varname) 
        return true;
     else if (profile_pressure_name == varname) 
        return true;
     else if (wave_length_name == varname)
        return true;
     else 
        return false;
}

// Add Dimension names for GPM products
void GMFile::Add_Dim_Name_GPM()
{

    BESDEBUG("h5", "Coming to Add_Dim_Name_GPM()"<<endl);
    // This is used to create a dimension name set.
    pair<set<string>::iterator,bool> setret;

    // The commented code is for an old version of GPM products. May remove them later. KY 2015-06-16
    // We need to create a fakedim name to fill in. To make the dimension name unique, we use a counter.
#if 0
    // int dim_count = 0;
    // map<string,string> varname_to_fakedim;
    // map<int,string> gpm_dimsize_to_fakedimname;
#endif

    // We find that GPM has an attribute DimensionNames(nlon,nlat) in this case.
    // We will use this attribute to specify the dimension names.
    for (const auto &var:this->vars) {

        for (auto &attr:var->attrs) {

            if("DimensionNames" == attr->name) {

                Retrieve_H5_Attr_Value(attr,var->fullpath);
                string dimname_value(attr->value.begin(),attr->value.end());

                vector<string> ind_elems;
                char sep=',';
                HDF5CFUtil::Split(dimname_value.data(),sep,ind_elems);

                if(ind_elems.size() != (size_t)(var->getRank())) {
                    throw2("The number of dims obtained from the <DimensionNames> attribute is not equal to the rank ", 
                           var->name); 
                }

                for(unsigned int i = 0; i<ind_elems.size(); ++i) {

                    (var->dims)[i]->name = ind_elems[i];

                    // Generate a dimension name if the dimension name is missing.
                    // The routine will ensure that the fakeDim name is unique.
                    if((var->dims)[i]->name==""){ 
                        Add_One_FakeDim_Name((var->dims)[i]);
// For debugging
#if 0
                        string fakedim = "FakeDim";
                        stringstream sdim_count;
                        sdim_count << dim_count;
                        fakedim = fakedim + sdim_count.str();
                        dim_count++;
                        (var->dims)[i]->name = fakedim;
                        (var->dims)[i]->newname = fakedim;
                        ind_elems[i] = fakedim;
#endif
                    }
                    
                    else {
                        (var->dims)[i]->newname = ind_elems[i];
                        setret = dimnamelist.insert((var->dims)[i]->name);
                   
                        if (true == setret.second) {
                            Insert_One_NameSizeMap_Element((var->dims)[i]->name,
                                                           (var->dims)[i]->size,
                                                           (var->dims)[i]->unlimited_dim);
                        }
                        else {
                            if(dimname_to_dimsize[(var->dims)[i]->name] !=(var->dims)[i]->size)
                                throw5("Dimension ",(var->dims)[i]->name, "has two sizes",   
                                        (var->dims)[i]->size,dimname_to_dimsize[(var->dims)[i]->name]);

                        }
                    }

                }// end of for
                break;

            } //end of if
        } //end of for 

#if 0
        if(false == has_dim_name_attr) {

            throw4( "The variable ", var->name, " doesn't have the DimensionNames attribute.",
                    "We currently don't support this case. Please report to the NASA data center.");
        }
            
#endif
    } //end of for 
 
}
     
// Add Dimension names for Aquarius level 3 products
void GMFile::Add_Dim_Name_Aqu_L3() const
{
    BESDEBUG("h5", "Coming to Add_Dim_Name_Aqu_L3()"<<endl);
    for (auto &var:this->vars) {
        if ("l3m_data" == var->name) {
           (var->dims)[0]->name = "lat";
           (var->dims)[0]->newname = "lat";
           (var->dims)[1]->name = "lon";
           (var->dims)[1]->newname = "lon";
           break;
        }
       
// For the time being, don't assign dimension names to palette,
// we will see if tools can pick up l3m and then make decisions.
#if 0
        if ("palette" == (*irv)->name) {
//"h5","coming to palette" <<endl;
          ((*irv)->dims)[0]->name = "paldim0";
           ((*irv)->dims)[0]->newname = "paldim0";
           ((*irv)->dims)[1]->name = "paldim1";
           ((*irv)->dims)[1]->newname = "paldim1";
        }
#endif

    }// end of for 
}

// Add dimension names for OSMAPL2S(note: the SMAP change their structures. The code doesn't apply to them.)
void GMFile::Add_Dim_Name_OSMAPL2S(){

    BESDEBUG("h5", "Coming to Add_Dim_Name_OSMAPL2S()"<<endl);
    string tempvarname;
    string key = "_lat";
    string osmapl2sdim0 ="YDim";
    string osmapl2sdim1 ="XDim";

    // Since the dim. size of each dimension of 2D lat/lon may be the same, so use multimap.
    multimap<hsize_t,string> osmapl2sdimsize_to_dimname;
    pair<multimap<hsize_t,string>::iterator,multimap<hsize_t,string>::iterator> mm_er_ret;
    multimap<hsize_t,string>::iterator irmm; 

    // Generate dimension names based on the size of "???_lat"(one coordinate variable) 
    for (const auto &var:this->vars) {
        tempvarname = var->name;
        if ((tempvarname.size() > key.size())&& 
            (key == tempvarname.substr(tempvarname.size()-key.size(),key.size()))){
            if (var->dims.size() !=2) 
                throw1("Currently only 2D lat/lon is supported for OSMAPL2S");
            osmapl2sdimsize_to_dimname.insert(pair<hsize_t,string>((var->dims)[0]->size,osmapl2sdim0));
            osmapl2sdimsize_to_dimname.insert(pair<hsize_t,string>((var->dims)[1]->size,osmapl2sdim1));
            break;
        }
    }

    set<hsize_t> fakedimsize;
    pair<set<hsize_t>::iterator,bool> setsizeret;
    pair<set<string>::iterator,bool> setret;
    pair<set<string>::iterator,bool> tempsetret;
    set<string> tempdimnamelist;
    bool fakedimflag = false;


    for (const auto &var:this->vars) {
        for (auto &dim:var->dims) {

            fakedimflag = true;
            mm_er_ret = osmapl2sdimsize_to_dimname.equal_range(dim->size);
            for (irmm = mm_er_ret.first; irmm!=mm_er_ret.second;irmm++) {
                setret = tempdimnamelist.insert(irmm->second);
                if (setret.second) {
                   dim->name = irmm->second;
                   dim->newname = dim->name;
                   setret = dimnamelist.insert(dim->name);
                   if(setret.second) Insert_One_NameSizeMap_Element(dim->name,dim->size,dim->unlimited_dim);
                   fakedimflag = false;
                   break;
                }
            }
                      
            if (true == fakedimflag) {
                 Add_One_FakeDim_Name(dim);
                 setsizeret = fakedimsize.insert(dim->size);
                 if (!setsizeret.second)  
                    Adjust_Duplicate_FakeDim_Name(dim);
            }
        } // end of for dim
        tempdimnamelist.clear();
        fakedimsize.clear();
    } // end of for var
}

//Add dimension names for ACOS level2S or OCO2 level1B products
void GMFile::Add_Dim_Name_ACOS_L2S_OCO2_L1B(){
 
    BESDEBUG("h5", "Coming to Add_Dim_Name_ACOS_L2S_OCO2_L1B()"<<endl);
    for (const auto &var:this->vars) {
        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        for (auto dim:var->dims) {
                Add_One_FakeDim_Name(dim);
                setsizeret = fakedimsize.insert(dim->size);
                if (false == setsizeret.second)   
                    Adjust_Duplicate_FakeDim_Name(dim);
        }
    } // end of for var
}

// Add dimension names for general products. Read the descrption of Check_General_Product_Pattern() for different patterns we support.
void GMFile::Add_Dim_Name_General_Product(){

    BESDEBUG("h5", "Coming to Add_Dim_Name_General_Product()"<<endl);

    // This general product should follow the HDF5 dimension scale model. 
    if (GENERAL_DIMSCALE == this->gproduct_pattern){
        Add_Dim_Name_Dimscale_General_Product();
}
    // This general product has 2-D latitude,longitude
    else if (GENERAL_LATLON2D == this->gproduct_pattern)
        Add_Dim_Name_LatLon2D_General_Product();
    // This general product has 1-D latitude,longitude
    else if (GENERAL_LATLON1D == this->gproduct_pattern || GENERAL_LATLON_COOR_ATTR == this->gproduct_pattern)
        Add_Dim_Name_LatLon1D_Or_CoordAttr_General_Product();

        
}

// We check four patterns under the General_Product category
// 1. General products that uses HDF5 dimension scales following netCDF-4 data model
// 2. General products that have 2-D lat/lon variables(lat/lon variable names are used to identify the case) under the root group or
//    a special geolocation group
// 3. General products that have 1-D lat/lon variables(lat/lon variable names are used to identify the case) under the root group or
//    a special geolocation group
// 4. General products that have some variables containing CF "coordinates" attributes. We can support some products if the "coordinates"
//    attribute contains CF lat/lon units and the variable ranks are 2 or 1.  
void GMFile::Check_General_Product_Pattern()  {

    BESDEBUG("h5", "Coming to Check_General_Product_Pattern()"<<endl);
    if (false == Check_Dimscale_General_Product_Pattern()) {
        //HERE add a check for the GPM. (choose 5 variables equally distance for the attribute)
        if (false == Check_And_Update_New_GPM_L3()) 
            if (false == Check_LatLon2D_General_Product_Pattern()) 
                if (false == Check_LatLon1D_General_Product_Pattern())
                    Check_LatLon_With_Coordinate_Attr_General_Product_Pattern();
    }

}

// Check if this general product is netCDF4-like HDF5 file.
// We only need to check "DIMENSION_LIST","CLASS" and CLASS values.
bool GMFile::Check_Dimscale_General_Product_Pattern()  {

    BESDEBUG("h5", "Coming to Check_Dimscale_General_Product_Pattern()"<<endl);
    bool ret_value = false;
    bool has_dimlist = false;
    bool has_dimscalelist  = false;

    // Check if containing the "DIMENSION_LIST" attribute;
    for (const auto &var:this->vars) {
        for(const auto &attr:var->attrs) {
            if ("DIMENSION_LIST" == attr->name) {
                has_dimlist = true;
                break;
            }
        }
        if (true == has_dimlist)
            break;
    }

    // Check if containing both the attribute "CLASS" and the attribute "REFERENCE_LIST" for the same variable.
    // This is the dimension scale. 
    // Actually "REFERENCE_LIST" is not necessary for a dimension scale dataset. If a dimension scale doesn't
    // have a "REFERENCE_LIST", it is still valid. But no other variables use this dimension scale. We found
    // such a case in a matched_airs_aqua product. KY 2012-12-03
    for (const auto &var:this->vars) {
        for(const auto &attr:var->attrs) {
            if ("CLASS" == attr->name) {

                Retrieve_H5_Attr_Value(attr,var->fullpath);
                string class_value;
                class_value.resize(attr->value.size());
                copy(attr->value.begin(),attr->value.end(),class_value.begin());

                // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
                // "DIMENSION_SCALE", which is 15.
                if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                    has_dimscalelist = true;
                    break;
                }
            }
        }
        if (true == has_dimscalelist)
            break;
    }

    if (true == has_dimscalelist) {
        if (true == has_dimlist ) {
            this->gproduct_pattern = GENERAL_DIMSCALE;
            ret_value = true;
        }
        else {
            //May fall into the single dimension scale case. 
            //This is really, really rare,but we do have to check.
            // Check if NAME and _Netcdf4Dimid exists for this variable.
            
            bool is_general_dimscale = false;

            for (const auto &var:this->vars) {
                 
                bool has_class_dscale = false;
                bool has_name = false;
                bool has_netcdf4_id = false;

                for (const auto &attr:var->attrs) {

                    if ("CLASS" == attr->name) {
                        Retrieve_H5_Attr_Value(attr,var->fullpath);
                        string class_value;
                        class_value.resize(attr->value.size());
                        copy(attr->value.begin(),attr->value.end(),class_value.begin());

                        // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
                        // "DIMENSION_SCALE", which is 15.
                        if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) 
                            has_class_dscale= true;
                    }
                    else if ("NAME" == attr->name)
                        has_name = true;
                    else if ("_Netcdf4Dimid" == attr->name)
                        has_netcdf4_id = true;
                    if(true == has_class_dscale && true == has_name && true == has_netcdf4_id)
                        is_general_dimscale = true;
                }

                if(true == is_general_dimscale)
                    break;
            }

            if (true == is_general_dimscale) {
                this->gproduct_pattern = GENERAL_DIMSCALE;
                ret_value = true;
            }
        }
    }

    return ret_value;
}

bool GMFile::Check_And_Update_New_GPM_L3() {

    bool is_new_gpm_l3 = false;
    unsigned num_vars = this->vars.size();
    unsigned sel_steps = num_vars/5;
    string dim_name="DimensionNames";
    bool has_dim_name = false;
    if(sel_steps == 0)
        sel_steps = 1;

    // Given DimensionNames exists in almost every variable in the new GPM product,
    // We will check the existence of this attribute for at most 5 variables. 

    vector<Var *>::iterator it_var_end;

    if(sel_steps ==1)
        it_var_end = this->vars.end();
    else 
        it_var_end = this->vars.begin()+5*sel_steps;
        
    for (auto irv = this->vars.begin(); irv != it_var_end; irv+=sel_steps) {
        for (const auto &attr:(*irv)->attrs) {

            if(H5FSTRING == attr->getType()) {
                if(attr->name == dim_name){
                    has_dim_name = true;
                    break;
                }
            }
        }
        if(true == has_dim_name) 
            break;
    }

    // Files that can go to this step should be a small subset, now
    // we will check the "??GridHeader" for all the groups. 
    if(true == has_dim_name) {
        string attr_name_subset = "GridHeader";
        BESDEBUG("h5", "GMFile::Check_And_Update_New_GPM_L3() has attribute <DimensionNames>. "<<endl);
        for (const auto &grp:this->groups) {
            for (const auto &attr:grp->attrs) {

                string attr_name = attr->name;

                // We identify this as a new GPM level 3 product.
                if (attr_name.find(attr_name_subset)!=string::npos) {
                    this->product_type = GPM_L3_New;
                    is_new_gpm_l3 = true;
                    break;
                }
            }
            if (true == is_new_gpm_l3)
                break;
        }
    }
    return is_new_gpm_l3;
}

// If having 2-D latitude/longitude,set the general product pattern.
// In this version, we only check if we have "latitude,longitude","Latitude,Longitude","lat,lon" and "cell_lat,cell_lon"names.
// The "cell_lat" and "cell_lon" come from SMAP. KY 2015-12-2
bool GMFile::Check_LatLon2D_General_Product_Pattern()  {

    BESDEBUG("h5", "Coming to Check_LatLon2D_General_Product_Pattern()"<<endl);
    bool ret_value = false;
 
    ret_value =  Check_LatLon2D_General_Product_Pattern_Name_Size("latitude","longitude");
    if(false == ret_value) {
        ret_value =  Check_LatLon2D_General_Product_Pattern_Name_Size("Latitude","Longitude");
        if(false == ret_value) {
            ret_value =  Check_LatLon2D_General_Product_Pattern_Name_Size("lat","lon");
            if(false == ret_value)
                ret_value =  Check_LatLon2D_General_Product_Pattern_Name_Size("cell_lat","cell_lon");
        }
    }

    // Make sure set the general product pattern flag for this case.
    if(true == ret_value)
        this->gproduct_pattern = GENERAL_LATLON2D;
    return ret_value;

}

// Helper function for Check_LatLon2D_General_Product_Pattern,we assume the lat and lon only present either under the root or
// a specific group Geolocation.
bool GMFile::Check_LatLon2D_General_Product_Pattern_Name_Size(const string & latname,const string & lonname)  {

    BESDEBUG("h5", "Coming to Check_LatLon2D_General_Product_Pattern_Name_Size()"<<endl);
    bool ret_value = false;
    bool ll_flag =  false;

    vector<size_t>lat_size(2,0);
    vector<size_t>lon_size(2,0);

    const string designed_group1 = "/";
    const string designed_group2 = "/Geolocation/";

    bool lat_flag_g1 = false;
    bool lon_flag_g1 = false;
    bool lat_flag_g2 = false;
    bool lon_flag_g2 = false;


    // This case allows to have both "lat and lon" under either group 1 or group 2 but on not both group 1 and 2.
    // This case doesn't allow "lat" and "lon" under separate groups.
    // Check if we have lat and lon at the only designated group,group 1 "/"
    lat_flag_g1 = is_var_under_group(latname,designed_group1,2,lat_size);
    lon_flag_g1 = is_var_under_group(lonname,designed_group1,2,lon_size);
    if(lat_flag_g1 == true && lon_flag_g1 == true) {

        // Make sure the group 2 "/Geolocation"  doesn't have the lat/lon
        lat_flag_g2 = is_var_under_group(latname,designed_group2,2,lat_size);
        if(lat_flag_g2 == false) {
            lon_flag_g2 = is_var_under_group(lonname,designed_group2,2,lon_size);
            if(lon_flag_g2 == false)
                ll_flag = true;
        }
    }// If the root doesn't have lat/lon, check the group 2 "/Geolocation".
    else if(lat_flag_g1 == false && lon_flag_g1 == false) {
        lat_flag_g2 = is_var_under_group(latname,designed_group2,2,lat_size);
        if(lat_flag_g2 == true) {
            lon_flag_g2 = is_var_under_group(lonname,designed_group2,2,lon_size);
            if(lon_flag_g2 == true)
                ll_flag = true;
        }
    }

    // We are loose here since this is just to support some NASA products in a customized way.
    // If the first two cases don't exist, we allow to check another group "GeolocationData" and
    // see if Latitude and Longitude are present. (4 years ? from the first implementation, we got this case.)
    // KY 2020-02-27
    if(false == ll_flag) {

        const string designed_group3 = "/GeolocationData/";
        if(is_var_under_group(latname,designed_group3,2,lat_size) && 
           is_var_under_group(lonname,designed_group3,2,lon_size))
           ll_flag = true;
    }
   
#if 0
    
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        if((*irv)->rank == 2) {
            if((*irv)->name == latname) {

                // Obtain the variable path
                string lat_path =HDF5CFUtil::obtain_string_before_lastslash((*irv)->fullpath);

                // Tackle only the root group or the name of the group as "/Geolocation"
                // By doing this, we assume that the file has lat/lon either under the root or under the "Geolocation
                // but not BOTH. The following code may generate wrong results if the file contains lat/lon under
                // both the root and /Geolocation. This is documented in https://jira.hdfgroup.org/browse/HFVHANDLER-175
                bool has_right_lat = false;
                if("/" == lat_path || "/Geolocation/" == lat_path) 
                if("/" == lat_path || "/Geolocation/" == lat_path) {
                    ll_flag++;
                    lat_size[0] = (*irv)->getDimensions()[0]->size; 
                    lat_size[1] = (*irv)->getDimensions()[1]->size; 
                }

            }
            else if((*irv)->name == lonname) {
                string lon_path = HDF5CFUtil::obtain_string_before_lastslash((*irv)->fullpath);
                if("/" == lon_path || "/Geolocation/" == lon_path) {
                    ll_flag++;
                    lon_size[0] = (*irv)->getDimensions()[0]->size; 
                    lon_size[1] = (*irv)->getDimensions()[1]->size; 
                }
            }
            if(2 == ll_flag)
                break;
        } // if((*irv)->rank == 2)
    } // for (vector<Var *>::iterator irv = this->vars.begin();
 
#endif

    // Only when both lat/lon are found can we support this case.
    // Before that, we also need to check if the lat/lon shares the same dimensions.
    if(true == ll_flag) {

        bool latlon_size_match = true;
        for (unsigned int size_index = 0; size_index <lat_size.size();size_index++) {
            if(lat_size[size_index] != lon_size[size_index]){
                latlon_size_match = false;
                break;
            }
        }
        if (true == latlon_size_match) {
            // If we do find the lat/lon pair, save them for later use.
            gp_latname = latname;
            gp_lonname = lonname;
            ret_value = true;
        }

    }

    return ret_value;

}

// If having 1-D latitude/longitude,set the general product pattern.
// In this version, we only check if we have "latitude,longitude","Latitude,Longitude","lat,lon" and "cell_lat,cell_lon"names.
// The "cell_lat" and "cell_lon" come from SMAP. KY 2015-12-2
bool GMFile::Check_LatLon1D_General_Product_Pattern()  {

    BESDEBUG("h5", "Coming to Check_LatLon1D_General_Product_Pattern()"<<endl);
    bool ret_value = false;
 
    ret_value =  Check_LatLon1D_General_Product_Pattern_Name_Size("latitude","longitude");
    if(false == ret_value) {
        ret_value =  Check_LatLon1D_General_Product_Pattern_Name_Size("Latitude","Longitude");
        if(false == ret_value) {
            ret_value =  Check_LatLon1D_General_Product_Pattern_Name_Size("lat","lon");
            if(false == ret_value)
                ret_value =  Check_LatLon1D_General_Product_Pattern_Name_Size("cell_lat","cell_lon");
        }
    }

    if(true == ret_value)
        this->gproduct_pattern = GENERAL_LATLON1D;
    return ret_value;

}

// Helper function for Check_LatLon1D_General_Product_Pattern.
// We only check if the lat/lon etc. pairs are under "/" or "/Geolocation". Other cases can be easily added.
bool GMFile::Check_LatLon1D_General_Product_Pattern_Name_Size(const string & latname,const string & lonname)  {

    BESDEBUG("h5", "Coming to Check_LatLon1D_General_Product_Pattern_Name_Size()"<<endl);
    bool ret_value = false;
    short ll_flag   = 0;
    size_t lat_size = 0;
    size_t lon_size = 0;

    for (const auto &var:this->vars) {

        if(var->rank == 1) {
            if(var->name == latname)  {

                string lat_path =HDF5CFUtil::obtain_string_before_lastslash(var->fullpath);

                // Tackle only the root group or the name of the group as "/Geolocation"
                // May not generate the correct output. See https://jira.hdfgroup.org/browse/HFVHANDLER-175
                if("/" == lat_path || "/Geolocation/" == lat_path) {
                    ll_flag++;
                    lat_size = var->getDimensions()[0]->size; 
                }
            }
            else if(var->name == lonname) {
                string lon_path = HDF5CFUtil::obtain_string_before_lastslash(var->fullpath);
                if("/" == lon_path || "/Geolocation/" == lon_path) {
                    ll_flag++;
                    lon_size = var->getDimensions()[0]->size; 
                }
            }
            if(2 == ll_flag)
                break;
        }
    }
 
    if(2 == ll_flag) {

        bool latlon_size_match_grid = true;

        // When the size of latitude is equal to the size of longitude for a 1-D lat/lon, it is very possible
        // that this is not a regular grid but rather a profile with the lat,lon recorded as the function of time.
        // Adding the coordinate/dimension as the normal grid is wrong, so check out this case.
        // KY 2015-12-2
        if(lat_size == lon_size) {

            // It is very unusual that lat_size = lon_size for a grid.
            latlon_size_match_grid = false;

            // For a normal grid, a >2D variable should exist to have both lat and lon size, 
            // if such a variable that has the same size exists, we will treat it as a normal grid.
            for (const auto &var:this->vars) {
                if(var->rank >=2) {
                    short ll_size_flag = 0;
                    for (const auto &dim:var->dims) {
                        if(lat_size == dim->size) {
                            ll_size_flag++;
                            if(2 == ll_size_flag){
                                break;
                            }
                        }
                    }
                    if(2 == ll_size_flag) {
                        latlon_size_match_grid = true;
                        break;
                    }
                }
            }
        }

        // If the sizes of lat and lon match the grid, this is the lat/lon candidate.
        // Save the latitude and longitude names for later use.
        if (true == latlon_size_match_grid) {
            gp_latname = latname;
            gp_lonname = lonname;
            ret_value = true;
        }
    }
 
    return ret_value;
}

// This function checks if this general product contains "coordinates" attributes in some variables
// that can be used to handle CF friendly.
bool GMFile::Check_LatLon_With_Coordinate_Attr_General_Product_Pattern()  {

    BESDEBUG("h5", "Coming to Check_LatLon_With_Coordinate_Attr_General_Product_Pattern()"<<endl);
    bool ret_value = false;
    string co_attrname = "coordinates";
    string co_attrvalue;
    string unit_attrname = "units";
    string lat_unit_attrvalue ="degrees_north";
    string lon_unit_attrvalue ="degrees_east";

    bool coor_has_lat_flag = false;
    bool coor_has_lon_flag = false;

    vector<Var*> tempvar_lat;
    vector<Var*> tempvar_lon;

    // Check if having both lat, lon names stored in the coordinate attribute value by looping through rank >1 variables.
    for (const auto &var:this->vars) {

        if(var->rank >=2) {
            for (const auto &attr:var->attrs) {

                // If having attribute "coordinates" for this variable, checking the values and 
                // see if having lat/lon,latitude/longitude, Latitude/Longitude pairs.
                if(attr->name == co_attrname) {
                    Retrieve_H5_Attr_Value(attr,var->fullpath);
                    string orig_attr_value(attr->value.begin(),attr->value.end());
                    vector<string> coord_values;
                    char sep=' ';
                    HDF5CFUtil::Split_helper(coord_values,orig_attr_value,sep);
                       
                    for (const auto &coord_value:coord_values) {
                        string coord_value_suffix1;
                        string coord_value_suffix2;
                        string coord_value_suffix3;

                        if(coord_value.size() >=3) {

                            // both "lat" and "lon" have 3 characters.
                            coord_value_suffix1 = coord_value.substr(coord_value.size()-3,3);

                            // The word "latitude" has 8 characters and the word "longitude" has 9 characters.
                            if(coord_value.size() >=8){
                                coord_value_suffix2 = coord_value.substr(coord_value.size()-8,8);
                                if(coord_value.size() >=9)
                                    coord_value_suffix3 = coord_value.substr(coord_value.size()-9,9);
                            }
                        }
                        
                        // lat/longitude or latitude/lon pairs in theory are fine.
                        if(coord_value_suffix1=="lat" || coord_value_suffix2 =="latitude" || coord_value_suffix2 == "Latitude")
                            coor_has_lat_flag = true;
                        else if(coord_value_suffix1=="lon" || coord_value_suffix3 =="longitude" || coord_value_suffix3 == "Longitude")
                            coor_has_lon_flag = true;
                    }

                    if(true == coor_has_lat_flag && true == coor_has_lon_flag)
                        break;
                }// end of if(attr->name
            }// end of for (ira)

            if(true == coor_has_lat_flag && true == coor_has_lon_flag) 
                break;
            else {
                coor_has_lat_flag = false;
                coor_has_lon_flag = false;
            }
        } // end of if 
    }//  end of for 

    // Check the variable names that include latitude and longitude suffixes such as lat,latitude and Latitude. 
    if(true == coor_has_lat_flag && true == coor_has_lon_flag) {

        for (const auto &var:this->vars) {

            bool var_is_lat = false;
            bool var_is_lon = false;
         
            string varname = var->name;
            string ll_ssuffix;
            string ll_lsuffix1;
            string ll_lsuffix2;
            if(varname.size() >=3) {//lat/lon
                ll_ssuffix = varname.substr(varname.size()-3,3);
                if(varname.size() >=8) {//latitude/Latitude
                    ll_lsuffix1 = varname.substr(varname.size()-8,8);
                    if(varname.size() >=9)//Longitude/longitude
                        ll_lsuffix2 = varname.substr(varname.size()-9,9);
                }
            }
            if(ll_ssuffix=="lat" || ll_lsuffix1 =="latitude" || ll_lsuffix1 == "Latitude")
                var_is_lat = true;
            else if(ll_ssuffix=="lon" || ll_lsuffix2 =="longitude" || ll_lsuffix2 == "Longitude")
                var_is_lon = true;
 
            // Find the lat/lon candidate, save them to  temporary vectors
            if(true == var_is_lat) {
                if(var->rank > 0) {
                    auto lat_unique = make_unique<Var>(var);
                    auto lat = lat_unique.release();
                    tempvar_lat.push_back(lat);
                }
            }
            else if(true == var_is_lon) {
                if(var->rank >0) {
                    auto lon_unique = make_unique<Var>(var);
                    auto lon = lon_unique.release();
                    tempvar_lon.push_back(lon);
                }
            }
        }// for of irv

        // Build up latloncv_candidate_pairs,  Name_Size_2Pairs struct, 
        // 1) Compare the rank, dimension sizes and the dimension orders of tempvar_lon against tempvar_lat
        //      rank >=2 the sizes,orders, should be consistent
        //      rank =1, no check issue.
        //   2) If the conditions are fulfilled, save them to the Name_Size struct 
        for (auto &t_lat:tempvar_lat) {

            // Check the rank =1 case
            if(t_lat->rank == 1) 
                Build_lat1D_latlon_candidate(t_lat,tempvar_lon);

            // Check the reank>=2 case
            else if(t_lat->rank >1)
                Build_latg1D_latlon_candidate(t_lat,tempvar_lon);
        }

#if 0
for(vector<struct Name_Size_2Pairs>::iterator ivs=latloncv_candidate_pairs.begin(); ivs!=latloncv_candidate_pairs.end();++ivs) {
cerr<<"struct lat lon names are " <<(*ivs).name1 <<" and " << (*ivs).name2 <<endl;
}
#endif

        // Check if there is duplicate latitude variables for one longitude variable in the latloncv_candidate_pairs.
        // if yes, remove the ones that have duplicate latitude variables. 
        // This will assure that the latloncv_candidate_pairs is one-to-one mapping between latitude and longitude.
        Build_unique_latlon_candidate();
        
        
        // Even if we find that there are qualified geo-location coordinate pairs, we still need to check
        // the geo-location variable rank. 
        // If the rank of any one-pair is 2, this case is qualified for the category GENERAL_LATLON_COOR_ATTR.
        // If the rank of any one-pair is 1, 
        //    we will check if the sizes of the lat and the lon in a pair are the same.
        //    If they are not the same, this case is qualified for the category GENERAL_LATLON_COOR_ATTR
        //    else check if there is any variable that has the "coordinates" attribute and the "coordinates" attribute includes
        //    the paths of this lat/lon pair. If the dimensions of such a variable have two sizes that are equal to the size of the lat,
        //    this case is still qualfied for the category GENERAL_LATLON_COOR_ATTR.
        // NOTE: here we deliberately ignore the case when the rank of lat/lon is >2. In some recent developments, we find that
        // there are 3D lat/lon and some tools like Panoply can visualize those data. So maybe we need to accept some 3D lat/lon in the futurei(KY 2016-07-07).        
        if(latloncv_candidate_pairs.empty() == false) {
            int num_1d_rank = 0;
            int num_2d_rank = 0;
            int num_g2d_rank = 0;
            vector<struct Name_Size_2Pairs> temp_1d_latlon_pairs;
            for(const auto &llcv_p:latloncv_candidate_pairs) {
                if(1 == llcv_p.rank) {
                    num_1d_rank++;
                    temp_1d_latlon_pairs.push_back(llcv_p);
                }
                else if(2 == llcv_p.rank)
                    num_2d_rank++;
                else if(llcv_p.rank >2) 
                    num_g2d_rank++;
            }
 
            // This is the GENERAL_LATLON_COOR_ATTR case.
            if (num_2d_rank !=0) 
                ret_value = true;
            else if(num_1d_rank!=0) {

                // Check if lat and lon share the same size and the dimension of a variable 
                // that has the "coordinates" only holds one size.
                for (const auto &t_1dll_p:temp_1d_latlon_pairs) {
                    if(t_1dll_p.size1 != t_1dll_p.size2) {
                        ret_value = true;
                        break;
                    }
                    else {

                        // If 1-D lat and lon share the same size,we need to check if there is a variable
                        // that has both lat and lon as the coordinates but only has one dimension that holds the size.
                        // If this is true, this is not the GENERAL_LATLON_COOR_ATTR case(SMAP level 2 follows into the category).
                    
                        ret_value = true;
                        for (const auto &var:this->vars) {

                            if(var->rank >=2) {
                                for (const auto &attr:var->attrs) {

                                    // Check if this variable has the "coordinates" attribute
                                    if(attr->name == co_attrname) {
                                        Retrieve_H5_Attr_Value(attr,var->fullpath);
                                        string orig_attr_value(attr->value.begin(),attr->value.end());
                                        vector<string> coord_values;
                                        char sep=' ';
                                        HDF5CFUtil::Split_helper(coord_values,orig_attr_value,sep);
                                        bool has_lat_flag = false;
                                        bool has_lon_flag = false;
                                        for (const auto &c_value:coord_values) {
                                            if(t_1dll_p.name1 == c_value) 
	                                        has_lat_flag = true;
                                            else if(t_1dll_p.name2 == c_value)
                                                has_lon_flag = true;
                                        }
                                        // Find both lat and lon, now check the dim. size 
                                        if(true == has_lat_flag && true == has_lon_flag) {
                                            short has_same_ll_size = 0;
                                            for(const auto &dim:var->dims){
                                                if(dim->size == t_1dll_p.size1)
                                                    has_same_ll_size++;
                                            }
                                            if(has_same_ll_size!=2){
                                                ret_value = false;
                                                break;
                                            }
                                        }
                                    }
                                }// end of for ira 

                                if(false == ret_value)
                                    break;
                            }// end of if irv
                        }// end of for irv

                        if(true == ret_value) 
                            break;
                    }// else 
                }// end of for ivs
            } // else of if num_1d_rank
        }// end of if of latloncv_candidate_pairs
        
        release_standalone_var_vector(tempvar_lat);
        release_standalone_var_vector(tempvar_lon);

    }
#if 0
if(true == ret_value) 
cerr<<"This product is the coordinate type "<<endl;
#endif
    // Don't forget to set the flag for this general product pattern.
    if(true == ret_value)
        this->gproduct_pattern = GENERAL_LATLON_COOR_ATTR;

    return ret_value;
}

// Build 1-D latlon coordinate variables candidate for GENERAL_LATLON_COOR_ATTR.
void GMFile::Build_lat1D_latlon_candidate(const Var *lat,const vector<Var*> &lon_vec) {

    BESDEBUG("h5", "Coming to Build_lat1D_latlon_candidate()"<<endl);
    set<string> lon_candidate_path;
    vector< pair<string,hsize_t> > lon_path_size_vec;

    // Obtain the path and the size info. from all the potential qualified longitude candidate.
    for(const auto &lon:lon_vec) {

        if (lat->rank == lon->rank) {
            pair<string,hsize_t>lon_path_size;
            lon_path_size.first = lon->fullpath;
            lon_path_size.second = lon->getDimensions()[0]->size;
            lon_path_size_vec.push_back(lon_path_size);
        }
    }

    // If there is only one potential qualified longitude for this latitude, just save this pair.
    if(lon_path_size_vec.size() == 1) {

        Name_Size_2Pairs latlon_pair;
        latlon_pair.name1 = lat->fullpath;
        latlon_pair.name2 = lon_path_size_vec[0].first;
        latlon_pair.size1 = lat->getDimensions()[0]->size;
        latlon_pair.size2 = lon_path_size_vec[0].second;
        latlon_pair.rank = lat->rank;
        latloncv_candidate_pairs.push_back(latlon_pair);
 
    }
    else if(lon_path_size_vec.size() >1) {

        // For more than one potential qualified longitude, we can still find a qualified one
        // if we find there is only one longitude under the same group of this latitude.
        string lat_path = HDF5CFUtil::obtain_string_before_lastslash(lat->fullpath);
        pair<string,hsize_t> lon_final_path_size;
        short num_lon_match = 0;
        for (const auto &lon_size:lon_path_size_vec) {
            // Search the longitude path and see if it matches with the latitude.
            if(HDF5CFUtil::obtain_string_before_lastslash(lon_size.first)==lat_path) {
                num_lon_match++;
                if(1 == num_lon_match)
                    lon_final_path_size = lon_size;
                else if(num_lon_match > 1) 
                    break;
            }
        }
        if(num_lon_match ==1) {// insert this lat/lon pair to the struct
            Name_Size_2Pairs latlon_pair;
            latlon_pair.name1 = lat->fullpath;
            latlon_pair.name2 = lon_final_path_size.first;
            latlon_pair.size1 = lat->getDimensions()[0]->size;
            latlon_pair.size2 = lon_final_path_size.second;
            latlon_pair.rank = lat->rank;
            latloncv_candidate_pairs.push_back(latlon_pair);
        }
    }

}

// Build >1D latlon coordinate variables candidate for GENERAL_LATLON_COOR_ATTR.
void GMFile::Build_latg1D_latlon_candidate(Var *lat,const vector<Var*> & lon_vec) {

    BESDEBUG("h5", "Coming to Build_latg1D_latlon_candidate()"<<endl);
    set<string> lon_candidate_path;

    // We will check if the longitude shares the same dimensions of the latitude
    for(const auto &lon:lon_vec) {

        if (lat->rank == lon->rank) {

            // Check the dim order and size.
            bool same_dim = true;
            for(int dim_index = 0; dim_index <lat->rank; dim_index++) {
                if(lat->getDimensions()[dim_index]->size !=
                   lon->getDimensions()[dim_index]->size){ 
                    same_dim = false;
                    break;
                }
            }
            if(true == same_dim) 
                lon_candidate_path.insert(lon->fullpath);
        }
    }
           
    // Check the size of the lon., if the size is not 1, see if we can find the pair under the same group.
    if(lon_candidate_path.size() > 1) {

        string lat_path = HDF5CFUtil::obtain_string_before_lastslash(lat->fullpath);
        vector <string> lon_final_candidate_path_vec;
        for(auto islon_path =lon_candidate_path.begin();islon_path!=lon_candidate_path.end();++islon_path) {

            // Search the path.
            if(HDF5CFUtil::obtain_string_before_lastslash(*islon_path)==lat_path) 
                lon_final_candidate_path_vec.push_back(*islon_path);
        }

        if(lon_final_candidate_path_vec.size() == 1) {// insert this lat/lon pair to the struct

            Name_Size_2Pairs latlon_pair;
                    
            latlon_pair.name1 = lat->fullpath;
            latlon_pair.name2 = lon_final_candidate_path_vec[0];
            latlon_pair.size1 = lat->getDimensions()[0]->size;
            latlon_pair.size2 = lat->getDimensions()[1]->size;
            latlon_pair.rank = lat->rank;
            latloncv_candidate_pairs.push_back(latlon_pair);
        }
        else if(lon_final_candidate_path_vec.size() >1) {

            // Under the same group, if we have two pairs lat/lon such as foo1_lat,foo1_lon, foo2_lat,foo2_lon, we will
            // treat {foo1_lat,foo1_lon} and {foo2_lat,foo2_lon} as two lat,lon coordinate candidates. This is essentially the SMAP L1B case.
            // We only compare three potential suffixes, lat/lon, latitude/longitude,Latitude/Longitude. We will treat the pair
            // latitude/Longitude and Latitude/longitude as a valid one. 
          
            string lat_name = HDF5CFUtil::obtain_string_after_lastslash(lat->fullpath);
            string lat_name_prefix1;
            string lat_name_prefix2;
             
            // name prefix before the pair lat,note: no need to check if the last 3 characters are lat or lon. We've checked already.
            if(lat_name.size() >3) {
                lat_name_prefix1 = lat_name.substr(0,lat_name.size()-3);
                if(lat_name.size() >8)
                    lat_name_prefix2 = lat_name.substr(0,lat_name.size()-8);
            }
            string lon_name_prefix1;
            string lon_name_prefix2;

            for(auto ilon = lon_final_candidate_path_vec.begin(); ilon!=lon_final_candidate_path_vec.end();++ilon) {
                string lon_name = HDF5CFUtil::obtain_string_after_lastslash(*ilon);
                if(lon_name.size() >3) {
                    lon_name_prefix1 = lon_name.substr(0,lon_name.size()-3);
                    if(lon_name.size() >9)
                        lon_name_prefix2 = lon_name.substr(0,lon_name.size()-9);
                }
                if((lat_name_prefix1 !="" && lat_name_prefix1 == lon_name_prefix1) ||
                   (lat_name_prefix2 !="" && lat_name_prefix2 == lon_name_prefix2)) {// match lat,lon this one is the candidate

                    Name_Size_2Pairs latlon_pair;
                    latlon_pair.name1 = lat->fullpath;
                    latlon_pair.name2 = *ilon;
                    latlon_pair.size1 = lat->getDimensions()[0]->size;
                    latlon_pair.size2 = lat->getDimensions()[1]->size;
                    latlon_pair.rank = lat->rank;
                    latloncv_candidate_pairs.push_back(latlon_pair);

                }
            }
        }// end else of if lon_final_candidate_path_vec 
    }//  end of if(lon_candidate_path.size() > 1)

    else if(lon_candidate_path.size() == 1) {//insert this lat/lon pair to the struct

        Name_Size_2Pairs latlon_pair;
                    
        latlon_pair.name1 = lat->fullpath;
        latlon_pair.name2 = *(lon_candidate_path.begin());
        latlon_pair.size1 = lat->getDimensions()[0]->size;
        latlon_pair.size2 = lat->getDimensions()[1]->size;
        latlon_pair.rank = lat->rank;
        latloncv_candidate_pairs.push_back(latlon_pair);
 
    }

}

// We need to make sure that one lat maps to one lon in the lat/lon pairs.
// This routine removes the duplicate ones like (lat1,lon1) and (lat2,lon1).
void GMFile::Build_unique_latlon_candidate() {

    BESDEBUG("h5", "Coming to Build_unique_latlon_candidate()"<<endl);
    set<int> duplicate_index;
    for(unsigned int i= 0; i<latloncv_candidate_pairs.size();i++) {
        for(unsigned int j=i+1;j<latloncv_candidate_pairs.size();j++) {
            if(latloncv_candidate_pairs[i].name2 == latloncv_candidate_pairs[j].name2) {
                duplicate_index.insert(i);
                duplicate_index.insert(j);
            }
        }       
    }

    // set is pre-sorted. we used a quick way to remove multiple elements.
    for(auto its= duplicate_index.rbegin();its!=duplicate_index.rend();++its) {
        latloncv_candidate_pairs[*its] = latloncv_candidate_pairs.back();
        latloncv_candidate_pairs.pop_back();
    }
}
// Leave the following code for the time being.
#if 0
// In this version, we only check if we have "latitude,longitude","Latitude,Longitude","lat,lon" names.
// This routine will check this case.
bool GMFile::Check_LatLonName_General_Product(int ll_rank)  {

    if(ll_rank <1 || ll_rank >2) 
        throw2("Only support rank = 1 or 2 lat/lon case for the general product. The current rank is ",ll_rank);
    bool ret_value = false;
    size_t lat2D_dimsize0 = 0;
    size_t lat2D_dimsize1 = 0;
    size_t lon2D_dimsize0 = 0;
    size_t lon2D_dimsize1 = 0;
    
    // The element order is latlon_flag,latilong_flag and LatLon_flag. 
    vector<short>ll_flag(3,0);

    vector<size_t>lat_size;
    vector<size_t>lon_size;

    // We only need to check  2-D latlon 
    if(2 == ll_rank) {
        //lat/lon is 2-D array, so the size is doubled.
        lat_size.assign(6,0);
        lon_size.assign(6,0);
    }

    for (const auto &var:this->vars) {

        if(var->rank == ll_rank) {
            if(var->name == "lat") {
                ll_flag[0]++;
                if(ll_rank == 2) {
                    lat_size[0] = var->getDimensions()[0]->size; 
                    lat_size[1] = var->getDimensions()[1]->size; 
                    
                }
                    
            }
            else if(var->name == "lon") {
                ll_flag[0]++;
                if(ll_rank == 2) {
                    lon_size[0] = var->getDimensions()[0]->size; 
                    lon_size[1] = var->getDimensions()[1]->size; 
                    
                }
 
            }
            else if(var->name == "latitude"){
                ll_flag[1]++;
                if(ll_rank == 2) {
                    lat_size[2] = var->getDimensions()[0]->size; 
                    lat_size[3] = var->getDimensions()[1]->size; 
                    
                }
            }
            else if(var->name == "longitude"){
                ll_flag[1]++;
                if(ll_rank == 2) {
                    lon_size[2] = var->getDimensions()[0]->size; 
                    lon_size[3] = var->getDimensions()[1]->size; 
                    
                }
 
            }
            else if(var->name == "Latitude"){
                 ll_flag[2]++;
                if(ll_rank == 2) {
                    lat_size[4] = var->getDimensions()[0]->size; 
                    lat_size[5] = var->getDimensions()[1]->size; 
                    
                }
 
            }
            else if(var->name == "Longitude"){
                ll_flag[2]++;
                if(ll_rank == 2) {
                    lon_size[4] = var->getDimensions()[0]->size; 
                    lon_size[5] = var->getDimensions()[1]->size; 
                }
            }
        }
    }

    int total_llflag = 0;
    for (int i = 0; i < ll_flag.size();i++)
        if(2 == ll_flag[i])  
            total_llflag ++;

    // We only support 1 (L)lat(i)/(L)lon(g) pair.
    if(1 == total_llflag) {
        bool latlon_size_match = true;
        if(2 == ll_rank) {
            for (int size_index = 0; size_index <lat_size.size();size_index++) {
                if(lat_size[size_index] != lon_size[size_index]){
                    latlon_size_match = false;
                    break;
                }
            }
        }

        if(true ==  latlon_size_match) {
            ret_value = true;
            if(2 == ll_flag[0]) {
                gp_latname = "lat";
                gp_lonname = "lon";
            }
            else if ( 2 == ll_flag[1]) {
                gp_latname = "latitude";
                gp_lonname = "longitude";
            }
        
            else if (2 == ll_flag[2]){
                gp_latname = "Latitude";
                gp_lonname = "Longitude";
            }
        }
    }
 
    return ret_value;
}
#endif

// Add dimension names for the case that has 2-D lat/lon.
void GMFile::Add_Dim_Name_LatLon2D_General_Product()  {

    BESDEBUG("h5", "Coming to Add_Dim_Name_LatLon2D_General_Product()"<<endl);
    string latdimname0;
    string latdimname1;
    size_t latdimsize0 = 0;
    size_t latdimsize1 = 0;

    // Need to generate fake dimensions.
    for (const auto &var:this->vars) {
 
        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        int num_dup_dim_size = 0;
        for (auto &dim:var->dims) {
            Add_One_FakeDim_Name(dim);
            setsizeret = fakedimsize.insert(dim->size);

            // Avoid the same size dimension sharing the same dimension name.
            if (false == setsizeret.second){   
                num_dup_dim_size++;
                Adjust_Duplicate_FakeDim_Name2(dim,num_dup_dim_size);
            }
 
#if 0
                if (false == setsizeret.second)   
                    Adjust_Duplicate_FakeDim_Name(dim);
#endif
        }

        // Find variable name that is latitude or lat or Latitude
        // Note that we don't need to check longitude since longitude dim. sizes should be the same as the latitude for this case.
        if(var->name == gp_latname) {
            if(var->rank != 2) {
                throw4("coordinate variables ",gp_latname,
                " must have rank 2 for the 2-D latlon case , the current rank is ",
                var->rank);
            }
            latdimname0 = var->getDimensions()[0]->name;
            latdimsize0 = var->getDimensions()[0]->size;
            
            latdimname1 = var->getDimensions()[1]->name;
            latdimsize1 = var->getDimensions()[1]->size;
        }
    }


    // Now we need to change a dimension of a general variable that shares the same size of lat
    // to the dimension name of the lat. 
    for (auto &var:this->vars) {
        int lat_dim0_index = 0;
        int lat_dim1_index = 0;
        bool has_lat_dims_size = false;

        for (unsigned int dim_index = 0; dim_index <var->dims.size(); dim_index++) {

            // Find if having the first dimension size of lat
            if((var->dims[dim_index])->size == latdimsize0) {

                // Find if having the second dimension size of lat
                lat_dim0_index = dim_index;
                for(unsigned int dim_index2 = dim_index+1;dim_index2 < var->dims.size();dim_index2++) {
                    if((var->dims[dim_index2])->size == latdimsize1) {
                        lat_dim1_index = dim_index2;
                        has_lat_dims_size = true;
                        break;
                    }
                }
            }
            if(true == has_lat_dims_size) 
                break;
        }
        // Find the lat's dimension sizes, change the (fake) dimension names.
        if(true == has_lat_dims_size) {

            (var->dims[lat_dim0_index])->name = latdimname0;
#if 0
            //(var->dims[lat_dim0_index])->newname = latdimname0;
#endif
            (var->dims[lat_dim1_index])->name = latdimname1;
#if 0
            //(var->dims[lat_dim1_index])->newname = latdimname1;
#endif

        }
    }

    //When we generate Fake dimensions, we may encounter discontiguous Fake dimension names such 
    // as FakeDim0, FakeDim9 etc. We would like to make Fake dimension names in contiguous order
    // FakeDim0,FakeDim1,etc.

    // Obtain the tempdimnamelist set.
    set<string>tempdimnamelist;

    for (const auto &var:this->vars) {
        for (const auto &dim:var->dims) 
            tempdimnamelist.insert(dim->name);
    }

    // Generate the final dimnamelist,it is a contiguous order: FakeDim0,FakeDim1 etc.
    set<string>finaldimnamelist;
    string finaldimname_base = "FakeDim";

    for(unsigned int i = 0; i<tempdimnamelist.size();i++) {
        stringstream sfakedimindex;
        sfakedimindex  << i;
        string finaldimname = finaldimname_base + sfakedimindex.str();
        finaldimnamelist.insert(finaldimname);
    }
    
    // If the original tempdimnamelist is not the same as the finaldimnamelist,
    // we need to generate a map from original name to the final name.
    if(finaldimnamelist != tempdimnamelist) {
        map<string,string> tempdimname_to_finaldimname;
        auto tempit = tempdimnamelist.begin();
        auto finalit = finaldimnamelist.begin();
        while(tempit != tempdimnamelist.end()) {
            tempdimname_to_finaldimname[*tempit] = *finalit;
            tempit++;
            finalit++;
        } 

        // Change the dimension names of every variable to the final dimension name list.
        for (auto &var:this->vars) {
            for (auto &dim:var->dims) {
                if(tempdimname_to_finaldimname.find(dim->name) !=tempdimname_to_finaldimname.end())
                    dim->name = tempdimname_to_finaldimname[dim->name];
                else 
                    throw3("The  dimension names ",dim->name, "cannot be found in the dim. name list.");
            }
        }
    }


    dimnamelist.clear();
    dimnamelist = finaldimnamelist;

    // We need to update dimname_to_dimsize map. This may be used in the future.
    dimname_to_dimsize.clear();
    for (const auto &var:this->vars) {
        for (const auto &dim:var->dims) {
            if(finaldimnamelist.find(dim->name)!=finaldimnamelist.end()) {
                dimname_to_dimsize[dim->name] = dim->size;
                dimname_to_unlimited[dim->name] = dim->unlimited_dim;
                finaldimnamelist.erase(dim->name);
            }
            
        }
        if(true == finaldimnamelist.empty())
            break;
    }
 
    // Finally set dimension newname
    for (auto &var:this->vars) {
        for (auto &dim:var->dims) 
            dim->newname = dim->name;    
    }
 
}

// Add dimension names for the case that has 1-D lat/lon or CoordAttr..
// 
void GMFile::Add_Dim_Name_LatLon1D_Or_CoordAttr_General_Product()  {

    BESDEBUG("h5", "Coming to Add_Dim_Name_LatLon1D_Or_CoordAttr_General_Product()"<<endl);
    // Only need to add the fake dimension names
    for (auto &var:this->vars) {
        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        int num_dup_dim_size = 0;
        for (auto &dim:var->dims) {
            Add_One_FakeDim_Name(dim);
            setsizeret = fakedimsize.insert(dim->size);
            // Avoid the same size dimension sharing the same dimension name.
            // HYRAX-629, we not only want to have the unique dimension name for
            // the same dimension size of different dimensions in a var, we also
            // want to reduce the number of dimension names.
            // So foo[100][100], foo2[100][100][100] should be something like
            // foo[FakeDim0][FakeDim1] foo2[FakeDim0][FakeDim1][FakeDim2]
            if (false == setsizeret.second){   
                num_dup_dim_size++;
                Adjust_Duplicate_FakeDim_Name2(dim,num_dup_dim_size);
            }
            // Comment out the original code for the time being.
#if 0
                if (false == setsizeret.second){   
                    num_dup_dim_size++;
                    Adjust_Duplicate_FakeDim_Name(dim,num_dup_dim_size);
                }
#endif
        }
    }

// leave it here for debugging purpose
#if 0
for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
cerr<<"Var name is "<<(*irv)->newname<<endl;
         for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) 
cerr<<"Dimension name is "<<(*ird)->newname <<endl;
}
#endif

}

// For netCDF-4-like HDF5 products, we need to add the dimension scales.
void GMFile::Add_Dim_Name_Dimscale_General_Product()  {

    BESDEBUG("h5", "Coming to Add_Dim_Name_Dimscale_General_Product()"<<endl);

    pair<set<string>::iterator,bool> setret;
    this->iscoard = true;

    for (const auto &var:this->vars) {

        // Obtain all the dimension names for this variable
        Handle_UseDimscale_Var_Dim_Names_General_Product(var);

        // Need to update dimenamelist and dimname_to_dimsize and dimname_to_unlimited maps for future use.
        for (const auto &dim:var->dims) { 
            setret = dimnamelist.insert(dim->name);
            if (true == setret.second) 
                Insert_One_NameSizeMap_Element(dim->name,dim->size,dim->unlimited_dim);
        }
    } // end of for 
 
    if (true == dimnamelist.empty()) 
        throw1("This product should have the dimension names, but no dimension names are found");

}

// Obtain dimension names for this variable when netCDF-4 model(using dimension scales) is followed.
void GMFile::Handle_UseDimscale_Var_Dim_Names_General_Product(Var *var)  {

    BESDEBUG("h5", "Coming to Handle_UseDimscale_Var_Dim_Names_General_Product()"<<endl);
    const Attribute* dimlistattr = nullptr;
    bool has_dimlist = false;
    bool has_dimclass   = false;

    for(const auto &attr:var->attrs) {
        if ("DIMENSION_LIST" == attr->name) {
            dimlistattr = attr;
            has_dimlist = true;  
        }
        if ("CLASS" == attr->name) {

            Retrieve_H5_Attr_Value(attr,var->fullpath);
            string class_value;
            class_value.resize(attr->value.size());
            copy(attr->value.begin(),attr->value.end(),class_value.begin());

            // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
            // "DIMENSION_SCALE", which is 15.
            if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                has_dimclass = true;
                break;
            }
        }

    } // end of for

    // This is a general variable, we need to find the corresponding coordinate variables.
    if (true == has_dimlist) 
        Add_UseDimscale_Var_Dim_Names_General_Product(var,dimlistattr);

    // Dim name is the same as the variable name for dimscale variable
    else if(true == has_dimclass) {
        if (var->dims.size() !=1) 
           throw2("Currently dimension scale dataset must be 1 dimension, this is not true for the dataset ",
                  var->name);

        // The var name is the object name, however, we would like the dimension name to be the full path.
        // so that the dim name can be served as the key for future handling.
        (var->dims)[0]->name = var->fullpath;
        (var->dims)[0]->newname = var->fullpath;
        pair<set<string>::iterator,bool> setret;
        setret = dimnamelist.insert((var->dims)[0]->name);
        if (true == setret.second) 
            Insert_One_NameSizeMap_Element((var->dims)[0]->name,(var->dims)[0]->size,(var->dims)[0]->unlimited_dim);
    }

    // No dimension, add fake dim names, this will rarely happen.
    else {

        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        for (auto &dim:var->dims) {
            Add_One_FakeDim_Name(dim);
            setsizeret = fakedimsize.insert(dim->size);
            // Avoid the same size dimension sharing the same dimension name.
            if (false == setsizeret.second)   
                Adjust_Duplicate_FakeDim_Name(dim);
        }
    }

}

// Add dimension names for the case when HDF5 dimension scale is followed(netCDF4-like)
void GMFile::Add_UseDimscale_Var_Dim_Names_General_Product(const Var *var,const Attribute*dimlistattr) 
{
    
    BESDEBUG("h5", "Coming to Add_UseDimscale_Var_Dim_Names_General_Product()"<<endl);
    ssize_t objnamelen = -1;
    hobj_ref_t rbuf;

#if 0
    //hvl_t *vlbuf = nullptr;
#endif 
    vector<hvl_t> vlbuf;
    
    hid_t dset_id = -1;
    hid_t attr_id = -1;
    hid_t atype_id = -1;
    hid_t amemtype_id = -1;
    hid_t aspace_id = -1;
    hid_t ref_dset = -1;

    if(nullptr == dimlistattr) 
        throw2("Cannot obtain the dimension list attribute for variable ",var->name);

    else if (0==var->rank) 
        throw2("The number of dimension should NOT be 0 for the variable ",var->name);
    
    else {
    try {

        vlbuf.resize(var->rank);
    
        dset_id = H5Dopen(this->fileid,(var->fullpath).c_str(),H5P_DEFAULT);
        if (dset_id < 0) 
            throw2("Cannot open the dataset ",var->fullpath);

        attr_id = H5Aopen(dset_id,(dimlistattr->name).c_str(),H5P_DEFAULT);
        if (attr_id <0 ) 
            throw4("Cannot open the attribute ",dimlistattr->name," of HDF5 dataset ",var->fullpath);

        atype_id = H5Aget_type(attr_id);
        if (atype_id <0) 
            throw4("Cannot obtain the datatype of the attribute ",dimlistattr->name," of HDF5 dataset ",var->fullpath);

        amemtype_id = H5Tget_native_type(atype_id, H5T_DIR_ASCEND);

        if (amemtype_id < 0) 
            throw2("Cannot obtain the memory datatype for the attribute ",dimlistattr->name);


        if (H5Aread(attr_id,amemtype_id,vlbuf.data()) <0)
            throw2("Cannot obtain the referenced object for the variable ",var->name);
        

        vector<char> objname;
        int vlbuf_index = 0;

        // The dimension names of variables will be the HDF5 dataset names dereferenced from the DIMENSION_LIST attribute.
        for (auto &dim:var->dims) {

            if(vlbuf[vlbuf_index].p== nullptr) 
                throw4("The dimension doesn't exist. Var name is ",var->name,"; the dimension index is ",vlbuf_index);
            rbuf =((hobj_ref_t*)vlbuf[vlbuf_index].p)[0];
            if ((ref_dset = H5RDEREFERENCE(attr_id, H5R_OBJECT, &rbuf)) < 0) 
                throw2("Cannot dereference from the DIMENSION_LIST attribute  for the variable ",var->name);

            if ((objnamelen= H5Iget_name(ref_dset,nullptr,0))<=0) 
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);
            objname.resize(objnamelen+1);
            if ((objnamelen= H5Iget_name(ref_dset,objname.data(),objnamelen+1))<=0)
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);

            auto objname_str = string(objname.begin(),objname.end());

            // We need to remove the first character of the object name since the first character
            // of the object full path is always "/" and this will be changed to "_".
            // The convention of handling the dimension-scale general product is to remove the first "_".
            // Check the get_CF_string function of HDF5GMCF.cc.
            string trim_objname = objname_str.substr(0,objnamelen);
            dim->name = string(trim_objname.begin(),trim_objname.end());

            pair<set<string>::iterator,bool> setret;
            setret = dimnamelist.insert(dim->name);
            if (true == setret.second) 
               Insert_One_NameSizeMap_Element(dim->name,dim->size,dim->unlimited_dim);
            dim->newname = dim->name;
            H5Dclose(ref_dset);
#if 0
            ref_dset = -1;
#endif
            objname.clear();
            vlbuf_index++;
        }// end of for ird 

        if(vlbuf.empty() == false) {

            if ((aspace_id = H5Aget_space(attr_id)) < 0)
                throw2("Cannot get hdf5 dataspace id for the attribute ",dimlistattr->name);

            if (H5Dvlen_reclaim(amemtype_id,aspace_id,H5P_DEFAULT,(void*)vlbuf.data())<0)
                throw2("Cannot successfully clean up the variable length memory for the variable ",var->name);

            H5Sclose(aspace_id);
           
        }

        H5Tclose(atype_id);
        H5Tclose(amemtype_id);
        H5Aclose(attr_id);
        H5Dclose(dset_id);
    }

    catch(...) {

        if(atype_id != -1)
            H5Tclose(atype_id);

        if(amemtype_id != -1)
            H5Tclose(amemtype_id);

        if(aspace_id != -1)
            H5Sclose(aspace_id);

        if(attr_id != -1)
            H5Aclose(attr_id);

        if(dset_id != -1)
            H5Dclose(dset_id);

        throw;
    }
    }
 
}

// Handle coordinate variables
void GMFile::Handle_CVar() {

    BESDEBUG("h5", "GMFile:: Coming to Handle_CVar()"<<endl);
    // No coordinate variables are generated for ACOS_L2S or OCO2_L1B
    // Currently we support the three patterns for the general products:
    // 1) Dimensions follow HDF5 dimension scale specification
    // 2) Dimensions don't follow HDF5 dimension scale specification but have 1D lat/lon
    // 3) Dimensions don't follow HDF5 dimension scale specification but have 2D lat/lon
    if (General_Product == this->product_type ||
        ACOS_L2S_OR_OCO2_L1B == this->product_type) {
        if (GENERAL_DIMSCALE == this->gproduct_pattern)
            Handle_CVar_Dimscale_General_Product();
        else if (GENERAL_LATLON1D == this->gproduct_pattern) 
            Handle_CVar_LatLon1D_General_Product();
        else if (GENERAL_LATLON2D == this->gproduct_pattern)
            Handle_CVar_LatLon2D_General_Product();
        return;
    } 

    else if (Mea_SeaWiFS_L2 == this->product_type ||
        Mea_SeaWiFS_L3 == this->product_type) 
        Handle_CVar_Mea_SeaWiFS();
    else if (Aqu_L3 == this->product_type) 
        Handle_CVar_Aqu_L3(); 
    else if (OBPG_L3 == this->product_type)
        Handle_CVar_OBPG_L3();
    else if (OSMAPL2S == this->product_type) 
        Handle_CVar_OSMAPL2S();
    else if (Mea_Ozone == this->product_type) 
        Handle_CVar_Mea_Ozone();
    else if (GPMS_L3 == this->product_type || GPMM_L3 == this->product_type 
            || GPM_L3_New == this->product_type ) 
        Handle_CVar_GPM_L3();
    else if (GPM_L1 == this->product_type)
        Handle_CVar_GPM_L1();
}

// Handle GPM level 1 coordinate variables
void GMFile::Handle_CVar_GPM_L1()  {

    BESDEBUG("h5", "Coming to Handle_CVar_GPM_L1()"<<endl);
#if 0
    // Loop through the variable list to build the coordinates.
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
        if((*irv)->name=="AlgorithmRuntimeInfo") {
            delete(*irv);
            this->vars.erase(irv); 
            break;
        }
    }
#endif
 
    // Loop through all variables to check 2-D "Latitude" and "Longitude". 
    // Create coordinate variables based on 2-D "Latitude" and  "Longitude".
    // Latitude[Xdim][YDim] Longitude[Xdim][YDim], Latitude <->Xdim, Longitude <->YDim.
    // Make sure to build cf dimension names cfdimname = latpath+ the lat dimension name.
    // We want to save dimension names of Latitude and Longitude since 
    // the fake coordinate variables of these two dimensions should not be generated.
    // So we need to remember these dimension names.
    set<string> ll_dim_set;
    for (auto irv = this->vars.begin(); irv != this->vars.end(); ) {
        if((*irv)->rank == 2 && (*irv)->name == "Latitude") {
            size_t lat_pos = (*irv)->fullpath.rfind("Latitude");
            string lat_path = (*irv)->fullpath.substr(0,lat_pos);
            auto GMcvar_unique = make_unique<GMCVar>(*irv);
            auto GMcvar = GMcvar_unique.release();
            GMcvar->cfdimname = lat_path + ((*irv)->dims)[0]->name;    
            ll_dim_set.insert(((*irv)->dims)[0]->name);
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar);
            delete(*irv);
            irv = this->vars.erase(irv);
        }
       
        if((*irv)->rank == 2 && (*irv)->name == "Longitude") {
            size_t lon_pos = (*irv)->fullpath.rfind("Longitude");
            string lon_path = (*irv)->fullpath.substr(0,lon_pos);
            auto GMcvar_unique = make_unique<GMCVar>(*irv);
            auto GMcvar = GMcvar_unique.release();
            GMcvar->cfdimname = lon_path + ((*irv)->dims)[1]->name;    
            ll_dim_set.insert(((*irv)->dims)[1]->name);
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar);
            delete(*irv);
            irv = this->vars.erase(irv);
        }
        else {
            ++irv;
        }
    }// end of for irv 


    // For each dimension, create a coordinate variable.
    // Here we just need to loop through the map dimname_to_dimsize,
    // use the name and the size to create coordinate variables.
    for (map<string,hsize_t>::const_iterator itd = dimname_to_dimsize.begin();
                                            itd!=dimname_to_dimsize.end();++itd) {
        // We will not create fake coordinate variables for the
        // dimensions of latitude and longitude.
        if((ll_dim_set.find(itd->first)) == ll_dim_set.end()) {
            auto GMcvar_unique = make_unique<GMCVar>();
            auto GMcvar = GMcvar_unique.release();
            Create_Missing_CV(GMcvar,itd->first);
            this->cvars.push_back(GMcvar);
        }
    }//end of for (map<string,hsize_t>.

}

// Handle coordinate variables for GPM level 3
void GMFile::Handle_CVar_GPM_L3() {

    BESDEBUG("h5", "Coming to Handle_CVar_GPM_L3()"<<endl);
    iscoard = true;
    
    // Here we just need to loop through the map dimname_to_dimsize,
    // use the name and the size to create coordinate variables.
    for (map<string,hsize_t>::const_iterator itd = dimname_to_dimsize.begin();
                                            itd!=dimname_to_dimsize.end();++itd) {

        auto GMcvar_unique = make_unique<GMCVar>();
        auto GMcvar = GMcvar_unique.release();
        if("nlon" == itd->first || "nlat" == itd->first
           || "lnH" == itd->first || "ltH" == itd->first
           || "lnL" == itd->first || "ltL" == itd->first) {
            GMcvar->name = itd->first;
            GMcvar->newname = GMcvar->name;
            GMcvar->fullpath = GMcvar->name;
            GMcvar->rank = 1;
            GMcvar->dtype = H5FLOAT32;
            auto gmcvar_dim_unique = make_unique<Dimension>(itd->second);
            auto gmcvar_dim = gmcvar_dim_unique.release();
            gmcvar_dim->name = GMcvar->name;
            gmcvar_dim->newname = gmcvar_dim->name;
            GMcvar->dims.push_back(gmcvar_dim); 
            GMcvar->cfdimname = gmcvar_dim->name;
            if ("nlat" ==GMcvar->name || "ltH" == GMcvar->name 
                 || "ltL" == GMcvar->name) 
                GMcvar->cvartype = CV_LAT_MISS;
            else if ("nlon" == GMcvar->name || "lnH" == GMcvar->name
                 || "lnL" == GMcvar->name) 
                GMcvar->cvartype = CV_LON_MISS;
            GMcvar->product_type = product_type;
        }   
        else if (("nlayer" == itd->first && (28 == itd->second || 19 == itd->second)) ||
                 ("hgt" == itd->first && 5 == itd->second) ||
                 ("nalt" == itd->first && 5 == itd->second)){
            GMcvar->name = itd->first;
            GMcvar->newname = GMcvar->name;
            GMcvar->fullpath = GMcvar->name;
            GMcvar->rank = 1;
            GMcvar->dtype = H5FLOAT32;
            auto gmcvar_dim_unique = make_unique<Dimension>(itd->second);
            auto gmcvar_dim = gmcvar_dim_unique.release();
            gmcvar_dim->name = GMcvar->name;
            gmcvar_dim->newname = gmcvar_dim->name;
            GMcvar->dims.push_back(gmcvar_dim);
            GMcvar->cfdimname = gmcvar_dim->name;
            GMcvar->cvartype  = CV_SPECIAL;
            GMcvar->product_type = product_type;
        }
        else 
            Create_Missing_CV(GMcvar,itd->first);
        this->cvars.push_back(GMcvar);
    }//end of for
}

// Handle Coordinate variables for MeaSuRES SeaWiFS
void GMFile::Handle_CVar_Mea_SeaWiFS() {

    BESDEBUG("h5", "Coming to Handle_CVar_Mea_SeaWiFS()"<<endl);
    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;

    for (const auto &dimname:dimnamelist) {
        for (auto irv = this->vars.begin(); irv != this->vars.end(); ) {
            if (dimname== (*irv)->fullpath) {

                if (!iscoard && (("/natrack" == dimname) 
                                 || "/nxtrack" == dimname)) {
                    ++irv;
                    continue;
                 }

                if((*irv)->dims.size()!=1) 
                    throw3("Coard coordinate variable ",(*irv)->name, "is not 1D");

                // Create Coordinate variables.
                tempdimnamelist.erase(dimname);
                auto GMcvar_unique = make_unique<GMCVar>(*irv);
                auto GMcvar = GMcvar_unique.release();
                GMcvar->cfdimname = dimname;
                GMcvar->cvartype = CV_EXIST;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar); 
                delete(*irv);
                irv = this->vars.erase(irv);
            } // end of if irs
            else if(false == iscoard) { 
                // 2-D lat/lon, natrack maps to lat, nxtrack maps to lon.
                if (((dimname =="/natrack") && ((*irv)->fullpath == "/latitude"))
                  ||((dimname =="/nxtrack") && ((*irv)->fullpath == "/longitude"))) {
                    tempdimnamelist.erase(dimname);
                    auto GMcvar_unique = make_unique<GMCVar>(*irv);
                    auto GMcvar = GMcvar_unique.release();
                    GMcvar->cfdimname = dimname;    
                    GMcvar->cvartype = CV_EXIST;
                    GMcvar->product_type = product_type;
                    this->cvars.push_back(GMcvar);
                    delete(*irv);
                    irv = this->vars.erase(irv);
                }
                else {
                    ++irv;
                }

            }// end of else if(false == iscoard)
            else {
                ++irv;
            }
        } // end of for irv  ... 
    } // end of for irs  ...

    // Creating the missing "third-dimension" according to the dimension names.
    // This may never happen for the current MeaSure SeaWiFS, but put it here for code coherence and completeness.
    // KY 12-30-2011
    for (auto &dimname:tempdimnamelist) {
        auto GMcvar_unique = make_unique<GMCVar>();
        auto GMcvar = GMcvar_unique.release();
        Create_Missing_CV(GMcvar,dimname);
        this->cvars.push_back(GMcvar);
    }
}

// Handle Coordinate varibles for OSMAPL2S(Note: this function doesn't apply to SMAP)
void GMFile::Handle_CVar_OSMAPL2S()  {

    BESDEBUG("h5", "Coming to Handle_CVar_OSMAPL2S()"<<endl);
    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;
    string tempvarname;
    string key0 = "_lat";
    string key1 = "_lon";
    string osmapl2sdim0 ="YDim";
    string osmapl2sdim1 ="XDim";

    bool foundkey0 = false;
    bool foundkey1 = false;

    set<string> itset;

    for (auto irv = this->vars.begin(); irv != this->vars.end(); ) {

        tempvarname = (*irv)->name;

        if ((tempvarname.size() > key0.size())&&
                (key0 == tempvarname.substr(tempvarname.size()-key0.size(),key0.size()))){

            foundkey0 = true;

            if (dimnamelist.find(osmapl2sdim0)== dimnamelist.end()) 
                throw5("variable ",tempvarname," must have dimension ",osmapl2sdim0," , but not found ");

            tempdimnamelist.erase(osmapl2sdim0);
            auto GMcvar_unique = make_unique<GMCVar>(*irv);
            auto GMcvar = GMcvar_unique.release();
            GMcvar->newname = GMcvar->name; // Remove the path, just use the variable name
            GMcvar->cfdimname = osmapl2sdim0;    
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar);
            delete(*irv);
            irv = this->vars.erase(irv);
        }// end of if 
                    
        else if ((tempvarname.size() > key1.size())&& 
                (key1 == tempvarname.substr(tempvarname.size()-key1.size(),key1.size()))){

            foundkey1 = true;

            if (dimnamelist.find(osmapl2sdim1)== dimnamelist.end()) 
                throw5("variable ",tempvarname," must have dimension ",osmapl2sdim1," , but not found ");

            tempdimnamelist.erase(osmapl2sdim1);

            auto GMcvar_unique = make_unique<GMCVar>(*irv);
            auto GMcvar = GMcvar_unique.release();
            GMcvar->newname = GMcvar->name;
            GMcvar->cfdimname = osmapl2sdim1;    
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar);
            delete(*irv);
            irv = this->vars.erase(irv);
        }// end of else if ((tempvarname.size() > key1.size())&& ...
        else {
            ++irv;
        }
        if (true == foundkey0 && true == foundkey1) 
            break;
    } // end of for 

    for (auto irs = tempdimnamelist.begin(); irs != tempdimnamelist.end();++irs) {

        auto GMcvar_unique = make_unique<GMCVar>();
        auto GMcvar = GMcvar_unique.release();
        Create_Missing_CV(GMcvar,*irs);
        this->cvars.push_back(GMcvar);
    }

}

// Handle coordinate variables for Aquarius level 3 products
void GMFile::Handle_CVar_Aqu_L3()  {

    BESDEBUG("h5", "Coming to Handle_CVar_Aqu_L3()"<<endl);
    iscoard = true;
    for (const auto &var:this->vars) {

        if ( "l3m_data" == var->name) { 
            for (const auto &dim:var->dims) {

                auto GMcvar_unique = make_unique<GMCVar>();
                auto GMcvar = GMcvar_unique.release();
                GMcvar->name = dim->name;
                GMcvar->newname = GMcvar->name;
                GMcvar->fullpath = GMcvar->name;
                GMcvar->rank = 1;
                GMcvar->dtype = H5FLOAT32;
                auto gmcvar_dim_unique = make_unique<Dimension>(dim->size);
                auto gmcvar_dim = gmcvar_dim_unique.release();
                gmcvar_dim->name = GMcvar->name;
                gmcvar_dim->newname = gmcvar_dim->name;
                GMcvar->dims.push_back(gmcvar_dim); 
                GMcvar->cfdimname = gmcvar_dim->name;
                if ("lat" ==GMcvar->name ) GMcvar->cvartype = CV_LAT_MISS;
                if ("lon" == GMcvar->name ) GMcvar->cvartype = CV_LON_MISS;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar);
            } // end of for 
        } // end of if  
    }//end of for 
 
}

//Handle coordinate variables for MeaSuRES Ozone products
void GMFile::Handle_CVar_Mea_Ozone() {

    BESDEBUG("h5", "Coming to Handle_CVar_Mea_Ozone()"<<endl);
    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;

    if(false == iscoard) 
        throw1("Measure Ozone level 3 zonal average product must follow COARDS conventions");

    for (const auto &dimname:dimnamelist) {
        for (auto irv = this->vars.begin(); irv != this->vars.end(); ) {
            if (dimname== (*irv)->fullpath) {

                if((*irv)->dims.size()!=1) 
                    throw3("Coard coordinate variable",(*irv)->name, "is not 1D");

                // Create Coordinate variables.
                tempdimnamelist.erase(dimname);
                auto GMcvar_unique = make_unique<GMCVar>(*irv);
                auto GMcvar = GMcvar_unique.release();
                GMcvar->cfdimname = dimname;
                GMcvar->cvartype = CV_EXIST;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar); 
                delete(*irv);
                irv = this->vars.erase(irv);
            } // end of if 
            else {
                ++irv;
            }
        } // end of for irv 
    } // end of for irs 

    for (auto irs = tempdimnamelist.begin(); irs != tempdimnamelist.end();irs++) {

        auto GMcvar_unique = make_unique<GMCVar>();
        auto GMcvar = GMcvar_unique.release();
        Create_Missing_CV(GMcvar,*irs);
        this->cvars.push_back(GMcvar);
    }
}

// Handle coordinate variables for general products that use HDF5 dimension scales.
void GMFile::Handle_CVar_Dimscale_General_Product()  {

    BESDEBUG("h5", "Coming to Handle_CVar_Dimscale_General_Product"<<endl);
    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;

    for (const auto &dimname:dimnamelist) {
        for (auto irv = this->vars.begin(); irv != this->vars.end(); ) {

            // This is the dimension scale dataset; it should be changed to a coordinate variable.
            if (dimname== (*irv)->fullpath) {
                if((*irv)->dims.size()!=1) 
                    throw3("COARDS coordinate variable",(*irv)->name, "is not 1D");

                // Create Coordinate variables.
                tempdimnamelist.erase(dimname);

                auto GMcvar_unique = make_unique<GMCVar>(*irv);
                auto GMcvar = GMcvar_unique.release();
                GMcvar->cfdimname = dimname;

                // Check if this is just a netCDF-4 dimension. 
                bool is_netcdf_dimension = Is_netCDF_Dimension(*irv);

                // If this is just the netcdf dimension, we
                // will fill in the index numbers.
                if (true == is_netcdf_dimension)
                   GMcvar->cvartype = CV_FILLINDEX;
                else 
                    GMcvar->cvartype = CV_EXIST;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar); 
                delete(*irv);
                irv = this->vars.erase(irv);
            } // end of if irs
            else {
                ++irv;
            }
       } // end of for  irv 
    } // end of for irs 

    // Check if we have 2-D lat/lon CVs, and if yes, add those to the CV list.
    Update_M2DLatLon_Dimscale_CVs();

    // Add other missing coordinate variables.
    for (auto &dimname:tempdimnamelist) {
        if ((this->have_nc4_non_coord == false) ||
            (nc4_sdimv_dv_path.find(dimname) == nc4_sdimv_dv_path.end())) { 
            
            auto GMcvar_unique = make_unique<GMCVar>();
            auto GMcvar = GMcvar_unique.release();
            Create_Missing_CV(GMcvar,dimname);
            this->cvars.push_back(GMcvar);
        }
    }


//Debugging
#if 0
for (set<string>::iterator irs = dimnamelist.begin();
        irs != dimnamelist.end();irs++) {
cerr<<"dimension name is "<<(*irs)<<endl;
}
#endif

}


// Check if we have 2-D lat/lon CVs in a netCDF-4-like file, and if yes, add those to the CV list.
// This routine is a really complicate one. There are 9 steps to generate right 2-D lat/lon CVs.  
void GMFile::Update_M2DLatLon_Dimscale_CVs()  {

    BESDEBUG("h5", "Coming to Update_M2DLatLon_Dimscale_CVs()"<<endl);
    // If this is not a file that only includes 1-D lat/lon CVs
    if(false == Check_1DGeolocation_Dimscale()) {

        // Define temporary vectors to store 1-D lat/lon CVs
        vector<GMCVar*> tempcvar_1dlat;
        vector<GMCVar*> tempcvar_1dlon;

        // 1. Obtain 1-D lat/lon CVs(only search the CF units and the reserved lat/lon names)
        Obtain_1DLatLon_CVs(tempcvar_1dlat,tempcvar_1dlon);

        //  Define temporary vectors to store 2-D lat/lon Vars
        vector<Var*> tempcvar_2dlat;
        vector<Var*> tempcvar_2dlon;
        
        // This map remembers the positions of the latlon vars in the vector var. 
        // Remembering the positions avoids the searching of these lat and lon again when
        // deleting them for the var vector and adding them(only the CVs) to the CV vector.
        // KY 2015-12-23 
        map<string,int> latlon2d_path_to_index;

        // 2. Obtain 2-D lat/lon variables(only search the CF units and the reserved names)
        Obtain_2DLatLon_Vars(tempcvar_2dlat,tempcvar_2dlon,latlon2d_path_to_index);

#if 0
for(vector<GMCVar *>::iterator irv = tempcvar_1dlat.begin();irv != tempcvar_1dlat.end();++irv)
cerr<<"1-D lat variable full path is "<<(*irv)->fullpath <<endl;
for(vector<GMCVar *>::iterator irv = tempcvar_1dlon.begin();irv != tempcvar_1dlon.end();++irv)
cerr<<"1-D lon variable full path is "<<(*irv)->fullpath <<endl;

for(vector<Var *>::iterator irv = tempcvar_2dlat.begin();irv != tempcvar_2dlat.end();++irv)
cerr<<"2-D lat variable full path is "<<(*irv)->fullpath <<endl;
for(vector<Var *>::iterator irv = tempcvar_2dlon.begin();irv != tempcvar_2dlon.end();++irv)
cerr<<"2-D lon variable full path is "<<(*irv)->fullpath <<endl;
#endif

        // 3. Sequeeze the 2-D lat/lon vectors by removing the ones that share the same dims with 1-D lat/lon CVs.
        Obtain_2DLLVars_With_Dims_not_1DLLCVars(tempcvar_2dlat,tempcvar_2dlon,tempcvar_1dlat,tempcvar_1dlon,latlon2d_path_to_index);

#if 0
for(vector<Var *>::iterator irv = tempcvar_2dlat.begin();irv != tempcvar_2dlat.end();++irv)
cerr<<"2-D Left lat variable full path is "<<(*irv)->fullpath <<endl;
for(vector<Var *>::iterator irv = tempcvar_2dlon.begin();irv != tempcvar_2dlon.end();++irv)
cerr<<"2-D Left lon variable full path is "<<(*irv)->fullpath <<endl;
#endif

        // 4. Assemble the final 2-D lat/lon CV candidate vectors by checking if the corresponding 2-D lon of a 2-D lat shares
        // the same dimension and under the same group and if there is another pair of 2-D lat/lon under the same group.
        Obtain_2DLLCVar_Candidate(tempcvar_2dlat,tempcvar_2dlon,latlon2d_path_to_index);

#if 0
for(vector<Var *>::iterator irv = tempcvar_2dlat.begin();irv != tempcvar_2dlat.end();++irv)
cerr<<"Final candidate 2-D Left lat variable full path is "<<(*irv)->fullpath <<endl;
for(vector<Var *>::iterator irv = tempcvar_2dlon.begin();irv != tempcvar_2dlon.end();++irv)
cerr<<"Final candidate 2-D Left lon variable full path is "<<(*irv)->fullpath <<endl;
#endif

        // 5. Remove the 2-D lat/lon variables that are to be used as CVs from the vector that stores general variables
        // var2d_index, remembers the index of the 2-D lat/lon CVs in the original vector of vars.
        vector<int> var2d_index;
        for (map<string,int>::const_iterator it= latlon2d_path_to_index.begin();it!=latlon2d_path_to_index.end();++it)
            var2d_index.push_back(it->second);

        Remove_2DLLCVar_Final_Candidate_from_Vars(var2d_index);

        // 6. If we have 2-D CVs, COARDS should be turned off.
        if(tempcvar_2dlat.empty()==false)
            iscoard = false;

        // 7. Add the CVs based on the final 2-D lat/lon CV candidates.
        // We need to remember the dim names that the 2-D lat/lon CVs are associated with.
        set<string>dim_names_2d_cvs;

        for (auto &cvar_lat:tempcvar_2dlat){

            auto lat_unique = make_unique<GMCVar>(cvar_lat);
            auto lat = lat_unique.release();
            // Latitude is always corresponding to the first dimension.
            lat->cfdimname = cvar_lat->getDimensions()[0]->name;
            dim_names_2d_cvs.insert(lat->cfdimname);
            lat->cvartype = CV_EXIST;
            lat->product_type = product_type;
            this->cvars.push_back(lat);
        }
        for (auto &cvar_lon:tempcvar_2dlon) {

            auto lon_unique = make_unique<GMCVar>(cvar_lon);
            auto lon = lon_unique.release();
            // Longitude is always corresponding to the second dimension.
            lon->cfdimname = cvar_lon->getDimensions()[1]->name;
            dim_names_2d_cvs.insert(lon->cfdimname);
            lon->cvartype = CV_EXIST;
            lon->product_type = product_type;
            this->cvars.push_back(lon);
        }
     
        // 8. Move the originally assigned 1-D CVs that are replaced by 2-D CVs back to the general variable list. 
        //    Also remove the CV created by the pure dimensions. 
        //    Dimension names are used to identify those 1-D CVs.
        for(auto ircv= this->cvars.begin();ircv !=this->cvars.end();) {
            if(1 == (*ircv)->rank) {
                if(dim_names_2d_cvs.find((*ircv)->cfdimname)!=dim_names_2d_cvs.end()) {
                    if(CV_FILLINDEX == (*ircv)->cvartype) {// This is pure dimension
                        delete(*ircv);
                        ircv = this->cvars.erase(ircv);
                    }
                    else if(CV_EXIST == (*ircv)->cvartype) {// This var exists already 

                        // Add this var. to the var list.
                        auto var_unique = make_unique<Var>(*ircv);
                        auto var = var_unique.release();
                        this->vars.push_back(var);

                        // Remove this var. from the GMCVar list.
                        delete(*ircv);
                        ircv = this->cvars.erase(ircv);

                    }
                    else {// the removed 1-D coordinate variable  should be either the CV_FILLINDEX or CV_EXIST.
                        if(CV_LAT_MISS == (*ircv)->cvartype) 
                            throw3("For the 2-D lat/lon case, the latitude dimension name ",(*ircv)->cfdimname, "is a coordinate variable of type CV_LAT_MISS");
                        else if(CV_LON_MISS == (*ircv)->cvartype)
                            throw3("For the 2-D lat/lon case, the latitude dimension name ",(*ircv)->cfdimname, "is a coordinate variable of type CV_LON_MISS");
                        else if(CV_NONLATLON_MISS == (*ircv)->cvartype)
                            throw3("For the 2-D lat/lon case, the latitude dimension name ",(*ircv)->cfdimname, "is a coordinate variable of type CV_NONLATLON_MISS");
                        else if(CV_MODIFY == (*ircv)->cvartype)
                            throw3("For the 2-D lat/lon case, the latitude dimension name ",(*ircv)->cfdimname, "is a coordinate variable of type CV_MODIFY");
                        else if(CV_SPECIAL == (*ircv)->cvartype)
                            throw3("For the 2-D lat/lon case, the latitude dimension name ",(*ircv)->cfdimname, "is a coordinate variable of type CV_SPECIAL");
                        else 
                            throw3("For the 2-D lat/lon case, the latitude dimension name ",(*ircv)->cfdimname, "is a coordinate variable of type CV_UNSUPPORTED");
                    }

                }
                else 
                    ++ircv;

            }
            else
                ++ircv;

        }
        

#if 0
//if(iscoard == true)
//cerr<<"COARD is true"<<endl;
for(set<string>::iterator irs = grp_cv_paths.begin();irs != grp_cv_paths.end();++irs) {
cerr<<"group path is "<< (*irs)<<endl;

}
#endif

#if 0
//Print CVs
cerr<<"File name is "<< this->path <<endl;
cerr<<"CV names are the following "<<endl;
for (vector<GMCVar *>:: iterator i= this->cvars.begin(); i!=this->cvars.end(); ++i) 
cerr<<(*i)->fullpath <<endl;
#endif
       

        // 9. release the resources allocated by the temporary vectors.
        release_standalone_GMCVar_vector(tempcvar_1dlat);
        release_standalone_GMCVar_vector(tempcvar_1dlon);
        release_standalone_var_vector(tempcvar_2dlat);
        release_standalone_var_vector(tempcvar_2dlon);
    }// if(false == Check_1DGeolocation_Dimscale())
#if 0
for (vector<GMCVar *>:: iterator i= this->cvars.begin(); i!=this->cvars.end(); ++i) 
cerr<<(*i)->fullpath <<endl;
#endif

}
    
// If Check_1DGeolocation_Dimscale() is true, no need to build 2-D lat/lon coordinate variables.
// This function is introduced to avoid the performance penalty caused by handling the general 2-D lat/lon case.
bool GMFile::Check_1DGeolocation_Dimscale()  {

    BESDEBUG("h5", "Coming to Check_1DGeolocation_Dimscale()"<<endl);
    bool has_only_1d_geolocation_cv = false;
    bool has_1d_lat_cv_flag = false;
    bool has_1d_lon_cv_flag = false;

    string  lat_dimname;
    hsize_t lat_size = 0;

    string  lon_dimname;
    hsize_t lon_size = 0;

    // We need to consider both 1-D lat/lon and the 1-D zonal average case(1-D lat only). 
    for (const auto &cvar:this->cvars) {

        if(cvar->cvartype == CV_EXIST) {

            string attr_name ="units";
            string lat_unit_value = "degrees_north";
            string lon_unit_value = "degrees_east";

           for (const auto &attr:cvar->attrs) {

                if(true == Is_Str_Attr(attr,cvar->fullpath,attr_name,lat_unit_value)) {
                    lat_size = cvar->getDimensions()[0]->size;
                    lat_dimname = cvar->getDimensions()[0]->name;
                    has_1d_lat_cv_flag = true;
                    break;
                }
                else if(true == Is_Str_Attr(attr,cvar->fullpath,attr_name,lon_unit_value)){ 
                    lon_size = cvar->getDimensions()[0]->size;
                    lon_dimname = cvar->getDimensions()[0]->name;
                    has_1d_lon_cv_flag = true;
                    break;
                }
            }
        }
    }

    // If having 1-D lat/lon CVs, this is a good sign for only 1-D lat/lon CVs , 
    // just need to have a couple of checks.
    if(true == has_1d_lat_cv_flag ) {           

        if(true == has_1d_lon_cv_flag) {

            // Come to the possible classic netCDF-4 case,
            if(true == this->groups.empty()) {

                // Rarely happens when lat_size is the same as the lon_size.
                // However, still want to make sure there is a 2-D variable that uses both lat and lon dims.
                if(lat_size == lon_size) { 
                    bool var_has_latdim = false;
                    bool var_has_londim = false;
                    for (const auto &var:this->vars) {
                        if(var->rank >= 2) {
                            for (const auto &dim:var->dims) {
                                if(dim->name == lat_dimname)
                                    var_has_latdim = true;
                                else if(dim->name == lon_dimname)
                                    var_has_londim = true;
                            }
                            if(true == var_has_latdim && true == var_has_londim) {
                                has_only_1d_geolocation_cv = true;
                                break;
                            }
                            else {
                                var_has_latdim = false;
                                var_has_londim = false;
                            }
                        }
                    }
                }
                else 
                    has_only_1d_geolocation_cv = true;
            }// if(0 == this->groups.size())
            else {
                // Multiple groups, need to check if having 2-D lat/lon pairs 
                bool has_2d_latname_flag = false;
                bool has_2d_lonname_flag = false;
                for (const auto &var:this->vars) {
                    if(var->rank == 2) { 
                        
                        //Note: When the 2nd parameter is true in the function Is_geolatlon, it checks the lat/latitude/Latitude 
                        if(true == Is_geolatlon(var->name,true))
                            has_2d_latname_flag = true;

                        //Note: When the 2nd parameter is false in the function Is_geolatlon, it checks the lon/longitude/Longitude
                        else if(true == Is_geolatlon(var->name,false))
                            has_2d_lonname_flag = true;

                        if((true == has_2d_latname_flag) && (true == has_2d_lonname_flag))
                            break;
                    }
                }

                if(has_2d_latname_flag != true || has_2d_lonname_flag != true) {

                    //Check if having the 2-D lat/lon by checking if having lat/lon CF units(lon's units: degrees_east  lat's units: degrees_north) 
                    has_2d_latname_flag = false;
                    has_2d_lonname_flag = false;

                    for (const auto &var:this->vars) { 
                        if(var->rank == 2) {
                            for (const auto &attr:var->attrs) {

                                if (false == has_2d_latname_flag) {

                                    // When the third parameter of the function has_latlon_cf_units is set to true, it checks latitude
                                    has_2d_latname_flag = has_latlon_cf_units(attr,var->fullpath,true);
                                    if(true == has_2d_latname_flag)
                                        break;
                                    else if(false == has_2d_lonname_flag) {

                                        // When the third parameter of the function has_latlon_cf_units is set to false, it checks longitude 
                                        has_2d_lonname_flag = has_latlon_cf_units(attr,var->fullpath,false);
                                        if(true == has_2d_lonname_flag)
                                            break;
                                    }
                                }
                                else if(false == has_2d_lonname_flag) {

                                    // Now has_2d_latname_flag is true, just need to check the has_2d_lonname_flag
                                    // When the third parameter of has_latlon_cf_units is set to false, it checks longitude 
                                    has_2d_lonname_flag = has_latlon_cf_units(attr,var->fullpath,false);
                                    if(true == has_2d_lonname_flag)
                                        break;
                                }
                            }
                            if(true == has_2d_latname_flag && true == has_2d_lonname_flag)
                                break;
                        }
                    }
                }// if(has_2d_latname_flag != true || has_2d_lonname_flag != true)

                // If we cannot find either of 2-D any lat/lon variables, this file is treated as having only 1-D lat/lon. 
                if(has_2d_latname_flag != true  || has_2d_lonname_flag != true)
                    has_only_1d_geolocation_cv = true;
            }
        
        }// 
        else {//Zonal average case, we do not need to find 2-D lat/lon CVs.
            has_only_1d_geolocation_cv = true;
        }
            
    }

#if 0
if(has_only_1d_geolocation_cv == true) 
cerr <<"has only 1D lat/lon CVs. "<<endl;
else
cerr<<"Possibly has 2D lat/lon CVs. "<<endl;
#endif

    return has_only_1d_geolocation_cv;

}

// Obtain the originally assigned 1-D lat/lon coordinate variables.
// This function should be used before generating any 2-D lat/lon CVs.
void GMFile::Obtain_1DLatLon_CVs(vector<GMCVar*> &cvar_1dlat,vector<GMCVar*> &cvar_1dlon) {

    BESDEBUG("h5", "Coming to Obtain_1DLatLon_CVs()"<<endl);
    for (const auto &cvar:this->cvars) {

        if (cvar->cvartype == CV_EXIST) {

            string attr_name ="units";
            string lat_unit_value = "degrees_north";
            string lon_unit_value = "degrees_east";

           for (const auto &attr:cvar->attrs) {

                // 1-D latitude
                if(true == Is_Str_Attr(attr,cvar->fullpath,attr_name,lat_unit_value)) {
                    auto lat_unique = make_unique<GMCVar>(cvar);
                    auto lat = lat_unique.release();
                    lat->cfdimname = cvar->getDimensions()[0]->name;
                    lat->cvartype = cvar->cvartype;
                    lat->product_type = cvar->product_type;
                    cvar_1dlat.push_back(lat);
                }
                // 1-D longitude
                else if(true == Is_Str_Attr(attr,cvar->fullpath,attr_name,lon_unit_value)){ 
                    auto lon_unique = make_unique<GMCVar>(cvar);
                    auto lon = lon_unique.release();
                    lon->cfdimname = cvar->getDimensions()[0]->name;
                    lon->cvartype = cvar->cvartype;
                    lon->product_type = cvar->product_type;
                    cvar_1dlon.push_back(lon);
                }
            }
        }// end of if(cvar->cvartype == CV_EXIST)
    }// end of for (cvar = this->cvars

}

// Obtain all 2-D lat/lon variables. We first check the lat/latitude/Latitude names, if not found, we check if CF lat/lon units are present.
// Latitude variables are saved in the vector var_2dlat. Longitude variables are saved in the vector var_2dlon.
// We also remember the index of these lat/lon in the original var vector.
void GMFile::Obtain_2DLatLon_Vars(vector<Var*> &var_2dlat,vector<Var*> &var_2dlon,map<string,int> & latlon2d_path_to_index) {

    BESDEBUG("h5", "Coming to Obtain_2DLatLon_Vars()"<<endl);
    for (auto irv = this->vars.begin(); irv != this->vars.end(); ++irv) {
        if((*irv)->rank == 2) { 
                        
            //Note: When the 2nd parameter is true in the function Is_geolatlon, it checks the lat/latitude/Latitude 
            if(true == Is_geolatlon((*irv)->name,true)) {
                auto lat_unique = make_unique<Var>(*irv);
                auto lat = lat_unique.release();
                var_2dlat.push_back(lat);
                latlon2d_path_to_index[(*irv)->fullpath]= distance(this->vars.begin(),irv);
                continue;
            }
            else {

                bool has_2dlat = false;
                for (const auto &attr:(*irv)->attrs) {

                    // When the third parameter of has_latlon_cf_units is set to true, it checks latitude
                    if(true == has_latlon_cf_units(attr,(*irv)->fullpath,true)) {
                        auto lat_unique = make_unique<Var>(*irv);
                        auto lat = lat_unique.release();
                        var_2dlat.push_back(lat);
                        latlon2d_path_to_index[(*irv)->fullpath] = distance(this->vars.begin(),irv);
                        has_2dlat = true;
                        break;
                    }
                }

                if(true == has_2dlat)
                    continue;
            }

            //Note: When the 2nd parameter is false in the function Is_geolatlon, it checks the lon/longitude/Longitude
            if (true == Is_geolatlon((*irv)->name,false)) {
                latlon2d_path_to_index[(*irv)->fullpath] = distance(this->vars.begin(),irv);
                auto lon_unique = make_unique<Var>(*irv);
                auto lon = lon_unique.release();
                var_2dlon.push_back(lon);
            }
            else {
                for (const auto &attr:(*irv)->attrs) {

                    // When the third parameter of has_latlon_cf_units is set to false, it checks longitude
                    if(true == has_latlon_cf_units(attr,(*irv)->fullpath,false)) {
                        latlon2d_path_to_index[(*irv)->fullpath] = distance(this->vars.begin(),irv);
                        auto lon_unique = make_unique<Var>(*irv);
                        auto lon = lon_unique.release();
                        var_2dlon.push_back(lon);
                        break;
                    }
                }
            }
        } // if((*irv)->rank == 2)
    } // for (auto irv
}

//  Squeeze the 2-D lat/lon vectors by removing the ones that share the same dims with 1-D lat/lon CVs.
//  The latlon2d_path_to_index map also needs to be updated.
void GMFile::Obtain_2DLLVars_With_Dims_not_1DLLCVars(vector<Var*> &var_2dlat,
                                                     vector<Var*> &var_2dlon, 
                                                     const vector<GMCVar*> &cvar_1dlat,
                                                     const vector<GMCVar*> &cvar_1dlon,
                                                     map<string,int> &latlon2d_path_to_index) {

    BESDEBUG("h5", "Coming to Obtain_2DLLVars_With_Dims_not_1DLLCVars()"<<endl);
    // First latitude at var_2dlat
    for(auto irv = var_2dlat.begin();irv != var_2dlat.end();) {
        bool remove_2dlat = false;
        for(const auto &lat:cvar_1dlat) {
            for (auto &dim:(*irv)->dims) {
                if(dim->name == lat->getDimensions()[0]->name &&
                   dim->size == lat->getDimensions()[0]->size) {
                    latlon2d_path_to_index.erase((*irv)->fullpath);
                    delete(*irv);
                    irv = var_2dlat.erase(irv);
                    remove_2dlat = true;
                    break;
                }
            }
            if(true == remove_2dlat)
                break; 
        }

        if(false == remove_2dlat)
            ++irv;
    }// end of for(auto irv = var_2dlat.begin()

    // Second longitude
    for(auto irv = var_2dlon.begin();irv != var_2dlon.end();) {
        bool remove_2dlon = false;
        for(const auto &lon:cvar_1dlon) {
            for (auto &dim:(*irv)->dims) {
                if(dim->name == lon->getDimensions()[0]->name &&
                   dim->size == lon->getDimensions()[0]->size) {
                    latlon2d_path_to_index.erase((*irv)->fullpath);
                    delete(*irv);
                    irv = var_2dlon.erase(irv);
                    remove_2dlon = true;
                    break;
                }
            }
            if(true == remove_2dlon)
                break; 
        }

        if(false == remove_2dlon)
            ++irv;
    } // end of for(auto irv = var_2dlon.begin()

}

//Out of the collected 2-D lat/lon variables, we will select the final qualified 2-D lat/lon as CVs.
void GMFile::Obtain_2DLLCVar_Candidate(vector<Var*> &var_2dlat,
                                       vector<Var*> &var_2dlon,
                                       map<string,int>& latlon2d_path_to_index) {
    BESDEBUG("h5", "Coming to Obtain_2DLLCVar_Candidate()"<<endl);
    // First check 2-D lat, see if we have the corresponding 2-D lon(same dims, under the same group).
    // If no, remove that lat from the vector.
    vector<string> lon2d_group_paths;

    for (auto irv_2dlat = var_2dlat.begin();irv_2dlat !=var_2dlat.end();) {
        for (const auto &lon:var_2dlon) {
            if(((*irv_2dlat)->getDimensions()[0]->name == lon->getDimensions()[0]->name) &&
               ((*irv_2dlat)->getDimensions()[0]->size == lon->getDimensions()[0]->size) &&
               ((*irv_2dlat)->getDimensions()[1]->name == lon->getDimensions()[1]->name) &&
               ((*irv_2dlat)->getDimensions()[1]->size == lon->getDimensions()[1]->size)) 
                lon2d_group_paths.push_back(HDF5CFUtil::obtain_string_before_lastslash(lon->fullpath));
        }
        // Doesn't find any lons that shares the same dims,remove this lat from the 2dlat vector,
        // also update the latlon2d_path_to_index map
        if (true == lon2d_group_paths.empty()) {
            latlon2d_path_to_index.erase((*irv_2dlat)->fullpath);         
            delete(*irv_2dlat);
            irv_2dlat = var_2dlat.erase(irv_2dlat);
        }
        else {// Find lons,check if they are under the same group
#if 0
            //string lat2d_group_path = (*irv_2dlat)->fullpath.substr(0,(*irv_2dlat)->fullpath.find_last_of("/"));
#endif
            string lat2d_group_path = HDF5CFUtil::obtain_string_before_lastslash((*irv_2dlat)->fullpath);

            // Check how many lon2d shares the same group with the lat2d
            short lon2d_has_lat2d_group_path_flag = 0;
            for(const auto &lon2d_group_path:lon2d_group_paths) {
                if(lon2d_group_path==lat2d_group_path) 
                    lon2d_has_lat2d_group_path_flag++;
            }

            // No lon2d shares the same group with the lat2d, remove this lat2d
            if(0 == lon2d_has_lat2d_group_path_flag) {
                latlon2d_path_to_index.erase((*irv_2dlat)->fullpath);
                delete(*irv_2dlat);
                irv_2dlat = var_2dlat.erase(irv_2dlat);
            }
            // Only one lon2d, yes, keep it.
            else if (1== lon2d_has_lat2d_group_path_flag) {
                ++irv_2dlat;
            }
            // More than 1 lon2d, we will remove the lat2d, but save the group path so that we may  
            // flatten the variable path stored in the coordinates attribute under this group.
            else {
                // Save the group path for the future use.
                grp_cv_paths.insert(lat2d_group_path);
                latlon2d_path_to_index.erase((*irv_2dlat)->fullpath);
                delete(*irv_2dlat);
                irv_2dlat = var_2dlat.erase(irv_2dlat);
            }
        }

        //Clear the vector that stores the same dim. since it is only applied to this lat, 
        lon2d_group_paths.clear();
    }

#if 0
for(auto irv_2dlat = var_2dlat.begin();irv_2dlat !=var_2dlat.end();++irv_2dlat) 
cerr<<"2 left 2-D lat variable full path is: "<<(*irv_2dlat)->fullpath <<endl;
#endif
 

    // Second check 2-D lon, see if we have the corresponding 2-D lat(same dims, under the same group).
    // If no, remove that lon from the vector.
    vector<string> lat2d_group_paths;

    // Check the longitude 
    for(auto irv_2dlon = var_2dlon.begin();irv_2dlon !=var_2dlon.end();) {
        for(const auto &lat:var_2dlat) {
            if((lat->getDimensions()[0]->name == (*irv_2dlon)->getDimensions()[0]->name) &&
               (lat->getDimensions()[0]->size == (*irv_2dlon)->getDimensions()[0]->size) &&
               (lat->getDimensions()[1]->name == (*irv_2dlon)->getDimensions()[1]->name) &&
               (lat->getDimensions()[1]->size == (*irv_2dlon)->getDimensions()[1]->size)) 
               lat2d_group_paths.push_back(HDF5CFUtil::obtain_string_before_lastslash(lat->fullpath)); 
#if 0
                //lat2d_group_paths.push_back(lat->fullpath.substr(0,lat->fullpath.find_last_of("/")));
#endif
        }
        // Doesn't find any lats that shares the same dims,remove this lon from this vector
        if(lat2d_group_paths.empty()) {
            latlon2d_path_to_index.erase((*irv_2dlon)->fullpath);
            delete(*irv_2dlon);
            irv_2dlon = var_2dlon.erase(irv_2dlon);
        }
        else {
            string lon2d_group_path = HDF5CFUtil::obtain_string_before_lastslash((*irv_2dlon)->fullpath);

            // Check how many lat2d shares the same group with the lon2d
            short lat2d_has_lon2d_group_path_flag = 0;
            for(const auto &lat2d_group_path:lat2d_group_paths) {
                if(lat2d_group_path == lon2d_group_path) 
                    lat2d_has_lon2d_group_path_flag++;
            }

            // No lat2d shares the same group with the lon2d, remove this lon2d
            if(0 == lat2d_has_lon2d_group_path_flag) {
                latlon2d_path_to_index.erase((*irv_2dlon)->fullpath);
                delete(*irv_2dlon);
                irv_2dlon = var_2dlon.erase(irv_2dlon);
            }
            // Only one lat2d shares the same group with the lon2d, yes, keep it.
            else if (1== lat2d_has_lon2d_group_path_flag) {
                ++irv_2dlon;
            }
            // more than 1 lat2d, we will remove the lon2d, but save the group path so that we can
            // change the coordinates attribute for variables under this group later.
            else {
                // Save the group path for future "coordinates" modification.
                grp_cv_paths.insert(lon2d_group_path);
                latlon2d_path_to_index.erase((*irv_2dlon)->fullpath);
                delete(*irv_2dlon);
                irv_2dlon = var_2dlon.erase(irv_2dlon);
            }
        }
        //Clear the vector that stores the same dim. since it is only applied to this lon, 
        lat2d_group_paths.clear();
    }
#if 0
for(vector<Var*>::iterator itv = var_2dlat.begin(); itv!= var_2dlat.end();++itv) {
cerr<<"Before unique, 2-D CV latitude name is "<<(*itv)->fullpath <<endl;
}
for(vector<Var*>::iterator itv = var_2dlon.begin(); itv!= var_2dlon.end();++itv) {
cerr<<"Before unique, 2-D CV longitude name is "<<(*itv)->fullpath <<endl;
}
#endif
 
    // Final check var_2dlat and var_2dlon to remove non-qualified CVs.
    Obtain_unique_2dCV(var_2dlat,latlon2d_path_to_index);
    Obtain_unique_2dCV(var_2dlon,latlon2d_path_to_index);
#if 0
for(vector<Var*>::iterator itv = var_2dlat.begin(); itv!= var_2dlat.end();++itv) {
cerr<<"2-D CV latitude name is "<<(*itv)->fullpath <<endl;
}
for(vector<Var*>::iterator itv = var_2dlon.begin(); itv!= var_2dlon.end();++itv) {
cerr<<"2-D CV longitude name is "<<(*itv)->fullpath <<endl;
}
#endif
 
    // This is to serve as a sanity check. This can help us find bugs in the first place.
    if(var_2dlat.size() != var_2dlon.size()) {
        throw1("Error in generating 2-D lat/lon CVs. The size of 2d-lat should be the same as that of 2d-lon.");
    }
}

// If two vars in the 2-D lat or 2-D lon CV candidate vector share the same dim, these two vars cannot be CVs.
// The group they belong to is the group candidate that the coordinates attribute of the variable under that group
// may be modified.
void GMFile::Obtain_unique_2dCV(vector<Var*> &var_ll,map<string,int>&latlon2d_path_to_index){

    BESDEBUG("h5", "Coming to Obtain_unique_2dCV()"<<endl);
    vector<bool> var_share_dims(var_ll.size(),false);
    
    for(unsigned int i = 0; i <var_ll.size();i++) {

        // obtain the path of var_ll
        string var_ll_i_path = HDF5CFUtil::obtain_string_before_lastslash(var_ll[i]->fullpath);
        
        // Check if two vars share the same dims.
	for(unsigned int j = i+1; j<var_ll.size();j++)   {
            if((var_ll[i]->getDimensions()[0]->name == var_ll[j]->getDimensions()[0]->name)
              ||(var_ll[i]->getDimensions()[0]->name == var_ll[j]->getDimensions()[1]->name)
              ||(var_ll[i]->getDimensions()[1]->name == var_ll[j]->getDimensions()[0]->name)
              ||(var_ll[i]->getDimensions()[1]->name == var_ll[j]->getDimensions()[1]->name)){
                string var_ll_j_path = HDF5CFUtil::obtain_string_before_lastslash(var_ll[j]->fullpath);

                // Compare var_ll_i_path and var_ll_j_path,only set the child group path be true and remember the path.
                // The variable at the parent group can be the coordinate variable.
                // Obtain the string size, 
                // compare the string size, long.compare(0,short-length,short)==0,
                // yes, save the long path(child group path), set the long path one true. Else save two paths, set both true
                if(var_ll_i_path.size() > var_ll_j_path.size()) {

                    // If var_ll_j_path is the parent group of var_ll_i_path,
                    // set the shared dim. be true for the child group only,remember the path.
                    if(var_ll_i_path.compare(0,var_ll_j_path.size(),var_ll_j_path)==0) {
                        var_share_dims[i] = true;
                        grp_cv_paths.insert(var_ll_i_path);
                    }
                    else {// Save both as shared, they cannot be CVs.
                        var_share_dims[i] = true;
                        var_share_dims[j] = true;
                  
                        grp_cv_paths.insert(var_ll_i_path);
                        grp_cv_paths.insert(var_ll_j_path);
                    }
                }
                else if (var_ll_i_path.size() == var_ll_j_path.size()) {// Share the same group, remember both group paths.
                    var_share_dims[i] = true;      
                    var_share_dims[j] = true;
                   if(var_ll_i_path == var_ll_j_path) 
                      grp_cv_paths.insert(var_ll_i_path);
                   else {
                      grp_cv_paths.insert(var_ll_i_path);
                      grp_cv_paths.insert(var_ll_j_path);
                   }
                }
                else {
                    // var_ll_i_path is the parent group of var_ll_j_path,
                    // set the shared dim. be true for the child group,remember the path.
                    if(var_ll_j_path.compare(0,var_ll_i_path.size(),var_ll_i_path)==0) {
                        var_share_dims[j] = true;
                        grp_cv_paths.insert(var_ll_j_path);
                    }
                    else {// Save both as shared, they cannot be CVs.
                        var_share_dims[i] = true;
                        var_share_dims[j] = true;
                  
                        grp_cv_paths.insert(var_ll_i_path);
                        grp_cv_paths.insert(var_ll_j_path);
 
                    }
                }
            }// end of if((var_ll[i]->getDimensions()[0]->name == var_ll[j]->getDimensions()[0]->name)
        }// end of for(int j = i+1; j<var_ll.size();j++)
    }// end of for( int i = 0; i <var_ll.size();i++)

    // Remove the shared 2-D lat/lon CVs from the 2-D lat/lon CV candidates. 
    int var_index = 0;
    for(auto itv = var_ll.begin(); itv!= var_ll.end();) {
        if(true == var_share_dims[var_index]) {
            latlon2d_path_to_index.erase((*itv)->fullpath);        
            delete(*itv);
            itv = var_ll.erase(itv);
        }
        else {
            ++itv;
        }
        ++var_index;
    }

}

// When promoting a 2-D lat or lon to a coordinate variable, we need to remove them from the general variable vector.
void GMFile::Remove_2DLLCVar_Final_Candidate_from_Vars(vector<int> &var2d_index)  {

    BESDEBUG("h5", "Coming to Remove_2DLLCVar_Final_Candidate_from_Vars()"<<endl);
    //Sort the 2-D lat/lon var index according to the ascending order before removing the 2-D lat/lon vars
    sort(var2d_index.begin(),var2d_index.end());
    auto it = this->vars.begin();

    // This is a performance optimization operation.
    // We find it is typical for swath files that have many general variables but only have very few lat/lon CVs.
    // To reduce the looping through all variables and comparing the fullpath(string), we use index and remember
    // the position of 2-D CVs in the iterator. In this way, only a few operations are needed.
    for (unsigned int i = 0; i <var2d_index.size();i++) {
       if ( i == 0)
           advance(it,var2d_index[i]);
       else
           advance(it,var2d_index[i]-var2d_index[i-1]-1);

       if(it == this->vars.end())
          throw1("Out of range to obtain 2D lat/lon variables");
       else {
          delete(*it);
          it = this->vars.erase(it);
       }
    }
}

//This function is for generating the coordinates attribute for the 2-D lat/lon.
//It will check if this var can keep its "coordinates" attribute rather than rebuilding it.
//This function is used by Handle_Coor_Attr(). 
bool GMFile::Check_Var_2D_CVars(Var *var)  const {

    BESDEBUG("h5", "Coming to Check_Var_2D_CVars()"<<endl);
    bool ret_value = true;
    for (const auto &cvar:this->cvars) {
        if(cvar->rank==2) {
            short first_dim_index = 0;
            short first_dim_times = 0;
            short second_dim_index = 0;
            short second_dim_times = 0;
            for (auto ird = var->dims.begin(); ird != var->dims.end(); ++ird) {
                if((*ird)->name == (cvar->getDimensions()[0])->name) {
                    first_dim_index = distance(var->dims.begin(),ird);  
                    first_dim_times++;
                }
                else if((*ird)->name == (cvar->getDimensions()[1])->name) {
                    second_dim_index = distance(var->dims.begin(),ird);
                    second_dim_times++;
                }
            }
            // The 2-D CV dimensions must only appear once as the dimension of the variable
            // It also must follow the dimension order of the 2-D lat/lon dimensions.
            if(first_dim_times == 1 &&  second_dim_times == 1) { 
                if(first_dim_index < second_dim_index) {
                    ret_value = false;
                    break;
                }
            }
        }
    }
    return ret_value;
    
}
                
// This function flattens the variable path in the "coordinates" attribute.
// It is also used by Handle_Coor_Attr().
bool GMFile::Flatten_VarPath_In_Coordinates_Attr(Var *var)  {

    BESDEBUG("h5", "Coming to Flatten_VarPath_In_Coordinates_Attr()"<<endl);
    string co_attrname = "coordinates";
    bool need_flatten_coor_attr = false;
    string orig_coor_value;
    string flatten_coor_value;
    // Assume the separator is always a space.
    char sc = ' ';
    char backslash = '/';

    for (auto ira =var->attrs.begin(); ira !=var->attrs.end();) {

        // We only check the original attribute name
        // Remove the original "coordinates" attribute.
        // There is a case that the "coordinates" attribute doesn't need to be flattened.
        // We will skip this case. This is necessary since the "coordinates" may be revised
        // to add a path to follow the CF. Without skipping the case, the "coordinates" is
        // actually not handled but "makes the impression" it is handled. That makes
        // this case ignored when the EnableCoorattrAddPath BES key is supposed to work on.
        // https://bugs.earthdata.nasa.gov/browse/HDFFEATURE-45
        if((*ira)->name == co_attrname) {
            Retrieve_H5_Attr_Value((*ira),var->fullpath);
            string orig_attr_value((*ira)->value.begin(),(*ira)->value.end());
            if(orig_attr_value.find_first_of(backslash)!=string::npos){ 
                orig_coor_value = orig_attr_value;
                need_flatten_coor_attr = true;
                delete(*ira);
                ira = var->attrs.erase(ira);
            }
            break;
        }
        else 
            ++ira;
    }

    if(true == need_flatten_coor_attr) {

        // We need to loop through each element in the "coordinates".
        size_t ele_start_pos = 0;
        size_t cur_pos = orig_coor_value.find_first_of(sc);
        while(cur_pos !=string::npos) {
            string tempstr = orig_coor_value.substr(ele_start_pos,cur_pos-ele_start_pos);
            tempstr = get_CF_string(tempstr);
            flatten_coor_value += tempstr + sc;
            ele_start_pos = cur_pos+1;
            cur_pos = orig_coor_value.find_first_of(sc,cur_pos+1);
        }
        // Only one element
        if(ele_start_pos == 0)
            flatten_coor_value = get_CF_string(orig_coor_value);
        else // Add the last element
            flatten_coor_value += get_CF_string(orig_coor_value.substr(ele_start_pos));

        // Generate the new "coordinates" attribute.
        auto attr_unique = make_unique<Attribute>();
        auto attr = attr_unique.release();
        Add_Str_Attr(attr,co_attrname,flatten_coor_value);
        var->attrs.push_back(attr);
        var->coord_attr_add_path = false;
        
    }

    return true;
 
} 
// This function flattens the variable path in the "coordinates" attribute for hybrid EOS5.
// Will implement it later. 
// It is also used by Handle_Coor_Attr().
#if 0
bool GMFile::Flatten_VarPath_In_Coordinates_Attr_EOS5(Var *var)  {

    BESDEBUG("h5", "Coming to Flatten_VarPath_In_Coordinates_Attr_EOS5()"<<endl);
    string co_attrname = "coordinates";
    bool has_coor_attr = false;
    string orig_coor_value;
    string flatten_coor_value;
    // Assume the separator is always a space.
    char sc = ' ';

    for (auto ira =var->attrs.begin(); ira !=var->attrs.end();) {

        // We only check the original attribute name
        // Remove the original "coordinates" attribute.
        if((*ira)->name == co_attrname) {
            Retrieve_H5_Attr_Value((*ira),var->fullpath);

            string orig_attr_value((*ira)->value.begin(),(*ira)->value.end());
            orig_coor_value = orig_attr_value;
            has_coor_attr = true;
            delete(*ira);
            ira = var->attrs.erase(ira);
            break;
        }
        else 
            ++ira;
    }

    if(true == has_coor_attr) {

        // We need to loop through each element in the "coordinates".
        // For EOS5: we need to  find the swath name.
        size_t ele_start_pos = 0;
// cerr<<"orig_coor_value is "<<orig_coor_value <<endl;
        size_t cur_pos = orig_coor_value.find_first_of(sc);
        while(cur_pos !=string::npos) {
            string tempstr = orig_coor_value.substr(ele_start_pos,cur_pos-ele_start_pos);
            // Find the swath name
            // tempstr = "swath name" +"_"+tempstr;
            tempstr = get_CF_string(tempstr);
            flatten_coor_value += tempstr + sc;
            ele_start_pos = cur_pos+1;
            cur_pos = orig_coor_value.find_first_of(sc,cur_pos+1);
        }
        // Only one element
        if(ele_start_pos == 0) {
            // Find the swath name
            // tempstr = "swath name" +"_"+tempstr;
            flatten_coor_value = get_CF_string(tempstr);
        }
        else // Add the last element
            flatten_coor_value += get_CF_string(orig_coor_value.substr(ele_start_pos));

        // Generate the new "coordinates" attribute.
        Attribute *attr = new Attribute();
        Add_Str_Attr(attr,co_attrname,flatten_coor_value);
        var->attrs.push_back(attr);
    }

    return true;
 
} 
#endif


// The following two routines only handle one 2-D lat/lon CVs. It is replaced by the more general 
// multi 2-D lat/lon CV routines. Leave it here just for references.
#if 0
bool  GMFile::Check_2DLatLon_Dimscale(string & latname, string &lonname)  {

    // New code to support 2-D lat/lon, still in development.
    // Need to handle 2-D latitude and longitude cases.
    // 1. Searching only the coordinate variables and if getting either of the following, keep the current way,
    // (A) GMcvar no CV_FILLINDEX:(The 2-D latlon case should have fake CVs)
    // (B) CV_EXIST: Attributes contain units and units value is degrees_east or degrees_north(have lat/lon) 
    // (B) CV_EXIST: variables have name pair{lat/latitude/Latitude,lon/longitude/Longitude}(have lat/lon)
    //
    // 2. if not 1),  searching all the variables and see if finding variables {lat/latitude/Latitude,lon/longitude/Longitude}; 
    // If finding {lat/lon},{latitude,longitude},{latitude,Longitude} pair, 
    //     if the number of dimension of either variable is not 2, keep the current way.
    //     else check if the dimension name of latitude and longitude are the same, not, keep the current way
    //          check the units of this CV pair, if units of the latitude is not degrees_north, 
    //          change it to degrees_north.
    //          if units of the longitude is not degrees_east, change it to degrees_east.
    //          make iscoard false.                
      
    bool latlon_2d_cv_check1 = false;

    // Some products(TOM MEaSURE) provide the true dimension scales for 2-D lat,lon. So relax this check.
    latlon_2d_cv_check1 = true;
#if 0
    // If having 2-D lat/lon, the corresponding dimension must be pure and the CV type must be FILLINDEX.
    for (auto ircv = this->cvars.begin();
        ircv != this->cvars.end(); ++ircv) {
        if((*ircv)->cvartype == CV_FILLINDEX){ 
            latlon_2d_cv_check1 = true;
            break;
        }
    }
#endif

    bool latlon_2d_cv_check2 = true;

    // There may still not be 2-D lat/lon. Check the units attributes and lat/lon pairs.
    if(true == latlon_2d_cv_check1) {
        BESDEBUG("h5","Coming to check if having 2d latlon coordinates for a netCDF-4 like product. "<<endl);

        // check if units attribute values have CF lat/lon units "degrees_north" or "degrees_east".
        for (auto ircv = this->cvars.begin();
            ircv != this->cvars.end(); ++ircv) {       
            if((*ircv)->cvartype == CV_EXIST) {
                for(auto ira = (*ircv)->attrs.begin();
                     ira != (*ircv)->attrs.end();ira++) {
                    string attr_name ="units";
                    string lat_unit_value = "degrees_north";
                    string lon_unit_value = "degrees_east";

                    // Considering the cross-section case, either is fine.
                    if((true == Is_Str_Attr(*ira,(*ircv)->fullpath,attr_name,lat_unit_value)) ||
                       (true == Is_Str_Attr(*ira,(*ircv)->fullpath,attr_name,lon_unit_value))) {
                        latlon_2d_cv_check2= false;
                        break;
                    }
                }
            }

            if(false == latlon_2d_cv_check2) 
                break;
        }
    }

    bool latlon_2d_cv_check3 = true;

    // Even we cannot find the CF lat/lon attributes, we may still find lat/lon etc pairs.
    if(true == latlon_2d_cv_check1 && true == latlon_2d_cv_check2) {

        short latlon_flag   = 0;
        short LatLon_flag   = 0;
        short latilong_flag = 0;
            
        for (auto ircv = this->cvars.begin();
            ircv != this->cvars.end(); ++ircv) {       
            if((*ircv)->cvartype == CV_EXIST) {
                if((*ircv)->name == "lat")
                    latlon_flag++;
                else if((*ircv)->name == "lon")
                    latlon_flag++;
                else if((*ircv)->name == "latitude")
                    latilong_flag++;
                else if((*ircv)->name == "longitude")
                    latilong_flag++;
                else if((*ircv)->name == "Latitude")
                    LatLon_flag++;
                else if((*ircv)->name == "Longitude")
                    LatLon_flag++;
            }

        }
        if((2== latlon_flag) || (2 == latilong_flag) || (2 == LatLon_flag ))
            latlon_2d_cv_check3 = false;
    }

    bool latlon_2d = false;
    short latlon_flag   = 0;
    string latdim1,latdim2,londim1,londim2;

    short LatLon_flag   = 0;
    string Latdim1,Latdim2,Londim1,Londim2;

    short latilong_flag = 0;
    string latidim1,latidim2,longdim1,longdim2;


    // Final check, we need to check if we have 2-D {lat/latitude/Latitude, lon/longitude/Longitude}
    // in the general variable list.
    // Here, depending on the future support, lat/lon pairs with other names(cell_lat,cell_lon etc) may be supported.
    // KY 2015-12-03
    if(true == latlon_2d_cv_check1 && true == latlon_2d_cv_check2 && true == latlon_2d_cv_check3) {

        for (auto irv = this->vars.begin();
            irv != this->vars.end(); ++irv) {

            // 
            if((*irv)->rank == 2) {
                if((*irv)->name == "lat") {
                    latlon_flag++;
                    latdim1 = (*irv)->getDimensions()[0]->name;
                    latdim2 = (*irv)->getDimensions()[1]->name;
                    
                }
                else if((*irv)->name == "lon") {
                    latlon_flag++;
                    londim1 = (*irv)->getDimensions()[0]->name;
                    londim2 = (*irv)->getDimensions()[1]->name;
                }
                else if((*irv)->name == "latitude"){
                    latilong_flag++;
                    latidim1 = (*irv)->getDimensions()[0]->name;
                    latidim2 = (*irv)->getDimensions()[1]->name;
                }
                else if((*irv)->name == "longitude"){
                    latilong_flag++;
                    longdim1 = (*irv)->getDimensions()[0]->name;
                    longdim2 = (*irv)->getDimensions()[1]->name;
 
                }
                else if((*irv)->name == "Latitude"){
                    LatLon_flag++;
                    Latdim1 = (*irv)->getDimensions()[0]->name;
                    Latdim2 = (*irv)->getDimensions()[1]->name;
 
                }
                else if((*irv)->name == "Longitude"){
                    LatLon_flag++;
                    Londim1 = (*irv)->getDimensions()[0]->name;
                    Londim2 = (*irv)->getDimensions()[1]->name;
                }
      
            }
        }

        // Here we ensure that only one lat/lon(lati/long,Lati/Long) is in the file.
        // If we find >=2 pairs lat/lon and latitude/longitude, or Latitude/Longitude,
        // we will not treat this as a 2-D latlon Dimscale case. The data producer
        // should correct their mistakes.    
        if(2 == latlon_flag) {
            if((2 == latilong_flag) || ( 2 == LatLon_flag))
                latlon_2d = false;
            else if((latdim1 == londim1) && (latdim2 == londim2)) {
                latname = "lat";
                lonname = "lon";
                latlon_2d = true;
            }
        }
        else if ( 2 == latilong_flag) {
            if( 2 == LatLon_flag)
                latlon_2d = false;
            else if ((latidim1 == longdim1) ||(latidim2 == longdim2)) {
                latname = "latitude";
                lonname = "longitude";
                latlon_2d = true;
            }
        }
        else if (2 == LatLon_flag){
            if ((Latdim1 == Londim1) ||(Latdim2 == Londim2)) {
                latname = "Latitude";
                lonname = "Longitude";
                latlon_2d = true;
            }
        }
    }
    
    return latlon_2d;
}


// Update the coordinate variables for files that use HDF5 dimension scales and have 2-D lat/lon.
void GMFile::Update_2DLatLon_Dimscale_CV(const string &latname,const string &lonname)  {

    iscoard = false;

    // Update latitude.
    for (auto irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        if((*irv)->rank == 2) {

            // Find 2-D latitude
            if((*irv)->name == latname) {

                // Obtain the first dimension of this variable
                string latdim0 = (*irv)->getDimensions()[0]->name;
//cerr<<"latdim0 is "<<latdim0 <<endl;

                // Remove the CV corresponding to latdim0
                for (vector<GMCVar *>:: iterator i= this->cvars.begin(); i!=this->cvars.end(); ) {
                    if((*i)->cfdimname == latdim0)  {
                        if(CV_FILLINDEX == (*i)->cvartype) {
                            delete(*i);
                            i = this->cvars.erase(i);
                        }
                        else if(CV_EXIST == (*i)->cvartype) { 
                            // Add this var. to the var list.
                            Var *var = new Var(*i);
                            this->vars.push_back(var);
                            // Remove this var. from the GMCVar list.
                            delete(*i);
                            i = this->cvars.erase(i);

                        }
                        else {// the latdimname should be either the CV_FILLINDEX or CV_EXIST.
                            if(CV_LAT_MISS == (*i)->cvartype) 
                                throw3("For the 2-D lat/lon case, the latitude dimension name ",latdim0, "is a coordinate variable of type CV_LAT_MISS");
                            else if(CV_LON_MISS == (*i)->cvartype)
                                throw3("For the 2-D lat/lon case, the latitude dimension name ",latdim0, "is a coordinate variable of type CV_LON_MISS");
                            else if(CV_NONLATLON_MISS == (*i)->cvartype)
                                throw3("For the 2-D lat/lon case, the latitude dimension name ",latdim0, "is a coordinate variable of type CV_NONLATLON_MISS");
                            else if(CV_MODIFY == (*i)->cvartype)
                                throw3("For the 2-D lat/lon case, the latitude dimension name ",latdim0, "is a coordinate variable of type CV_MODIFY");
                            else if(CV_SPECIAL == (*i)->cvartype)
                                throw3("For the 2-D lat/lon case, the latitude dimension name ",latdim0, "is a coordinate variable of type CV_SPECIAL");
                            else 
                                throw3("For the 2-D lat/lon case, the latitude dimension name ",latdim0, "is a coordinate variable of type CV_UNSUPPORTED");
 
                        }
                    }
                    else 
                        ++i;
                }
                // Add the 2-D latitude(latname) to the CV list.
                GMCVar* GMcvar = new GMCVar(*irv);
                GMcvar->cfdimname = latdim0;
                GMcvar->cvartype = CV_EXIST;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar);
                delete(*irv);
                this->vars.erase(irv);
                break;
            }
        }
    }

    // Update longitude.
    for (auto irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        if((*irv)->rank == 2) {

            // Find 2-D longitude
            if((*irv)->name == lonname) {

                // Obtain the second dimension of this variable
                string londim0 = (*irv)->getDimensions()[1]->name;

                // Remove the CV corresponding to londim0
                for (vector<GMCVar *>:: iterator i= this->cvars.begin(); i!=this->cvars.end(); ) {
                    // NEED more work!!! should also remove ntime from the GMCVar list but add it to the cvar list.Same for Lon.
                    if((*i)->cfdimname == londim0) {
                        if(CV_FILLINDEX == (*i)->cvartype) {
                            delete(*i);
                            i= this->cvars.erase(i);
                        }
                        else if(CV_EXIST == (*i)->cvartype) { 
                            // Add this var. to the var list.
                            Var *var = new Var(*i);
                            this->vars.push_back(var);
                            // Remove this var. from the GMCVar list.
                            delete(*i);
                            i = this->cvars.erase(i);
                        }
                        else {// the latdimname should be either the CV_FILLINDEX or CV_EXIST.
                            if(CV_LAT_MISS == (*i)->cvartype) 
                                throw3("For the 2-D lat/lon case, the longitude dimension name ",londim0, "is a coordinate variable of type CV_LAT_MISS");
                            else if(CV_LON_MISS == (*i)->cvartype)
                                throw3("For the 2-D lat/lon case, the longitude dimension name ",londim0, "is a coordinate variable of type CV_LON_MISS");
                            else if(CV_NONLATLON_MISS == (*i)->cvartype)
                                throw3("For the 2-D lat/lon case, the longitude dimension name ",londim0, "is a coordinate variable of type CV_NONLATLON_MISS");
                            else if(CV_MODIFY == (*i)->cvartype)
                                throw3("For the 2-D lat/lon case, the longitude dimension name ",londim0, "is a coordinate variable of type CV_MODIFY");
                            else if(CV_SPECIAL == (*i)->cvartype)
                                throw3("For the 2-D lat/lon case, the longitude dimension name ",londim0, "is a coordinate variable of type CV_SPECIAL");
                            else 
                                throw3("For the 2-D lat/lon case, the longitude dimension name ",londim0, "is a coordinate variable of type CV_UNSUPPORTED");
                        }
                    }
                    else
                        ++i;
                }

                // Add the 2-D longitude(lonname) to the CV list.
                GMCVar* GMcvar = new GMCVar(*irv);
                GMcvar->cfdimname = londim0;
                GMcvar->cvartype = CV_EXIST;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar);
                delete(*irv);
                this->vars.erase(irv);
                break;
            }
        }
    }
}
#endif

// Handle coordinate variables for general HDF5 products that have 1-D lat/lon
void GMFile::Handle_CVar_LatLon1D_General_Product()  {
 
    BESDEBUG("h5", "Coming to Handle_CVar_LatLon1D_General_Product()"<<endl);
    this->iscoard = true;
    Handle_CVar_LatLon_General_Product();
    
}

// Handle coordinate variables for general HDF5 products that have 2-D lat/lon
void GMFile::Handle_CVar_LatLon2D_General_Product()  {
   
    BESDEBUG("h5", "Coming to Handle_CVar_LatLon2D_General_Product()"<<endl);
    Handle_CVar_LatLon_General_Product();

}

// Routine to handle coordinate variables for general HDF5 product 
// that have either 1-D or 2-D lat/lon
void GMFile::Handle_CVar_LatLon_General_Product()  {

    BESDEBUG("h5", "Coming to Handle_CVar_LatLon_General_Product()"<<endl);
    if((GENERAL_LATLON2D != this->gproduct_pattern) 
       && GENERAL_LATLON1D != this->gproduct_pattern)
        throw1("This function only supports latlon 1D or latlon 2D general products");

    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;

    for (auto irv = this->vars.begin(); irv != this->vars.end(); ++irv) {

        // This is the dimension scale dataset; it should be changed to a coordinate variable.
        if (gp_latname== (*irv)->name) {

            // For latitude, regardless 1D or 2D, the first dimension needs to be updated.
            // Create Coordinate variables.
            tempdimnamelist.erase(((*irv)->dims[0])->name);
            auto GMcvar_unique = make_unique<GMCVar>(*irv);
            auto GMcvar = GMcvar_unique.release();
            GMcvar->cfdimname = ((*irv)->dims[0])->name;
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar); 
            delete(*irv);
            this->vars.erase(irv);
            break;
        } // end of if 
    } // end of for 

    for (auto irv = this->vars.begin(); irv != this->vars.end(); ++irv) {

        // This is the dimension scale dataset; it should be changed to a coordinate variable.
        if (gp_lonname== (*irv)->name) {

            // For 2-D lat/lon, the londimname should be the second dimension of the longitude
            // For 1-D lat/lon, the londimname should be the first dimension of the longitude
            // Create Coordinate variables.
            string londimname;
            if(GENERAL_LATLON2D == this->gproduct_pattern)
                londimname = ((*irv)->dims[1])->name;
            else
                londimname = ((*irv)->dims[0])->name;
               
            tempdimnamelist.erase(londimname);
            auto GMcvar_unique = make_unique<GMCVar>(*irv);
            auto GMcvar = GMcvar_unique.release();
            GMcvar->cfdimname = londimname;
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar); 
            delete(*irv);
            this->vars.erase(irv);
            break;
        } // end of if 
    } // end of for 

    //
    // Add other missing coordinate variables.
    for (const auto &dimname:tempdimnamelist) {
        auto GMcvar_unique = make_unique<GMCVar>();
        auto GMcvar = GMcvar_unique.release();
        Create_Missing_CV(GMcvar,dimname);
        this->cvars.push_back(GMcvar);
    }

}

// Handle coordinate variables for OBPG level 3 
void GMFile::Handle_CVar_OBPG_L3()  {

    BESDEBUG("h5", "Coming to Handle_CVar_OBPG_L3()"<<endl);
    if (GENERAL_DIMSCALE == this->gproduct_pattern)
            Handle_CVar_Dimscale_General_Product();

    // Change the CV Type of the corresponding CVs of lat and lon from CV_FILLINDEX to CV_LATMISS or CV_LONMISS
    for (const auto &var:this->vars) {

        // Here I try to avoid using the dimension name row and column to find the lat/lon dimension size.
        // So I am looking for a 2-D floating-point array or a 2-D array under the group geophysical_data.
        // This may be subject to change if OBPG level 3 change its arrangement of variables.
        // KY 2014-09-29
 
        if(var->rank == 2) {

            if((var->fullpath.find("/geophysical_data") == 0) || (var->dtype == H5FLOAT32)) {

                size_t lat_size = var->getDimensions()[0]->size;
                string lat_name = var->getDimensions()[0]->name;
                size_t lon_size = var->getDimensions()[1]->size;
                string lon_name = var->getDimensions()[1]->name;
                size_t temp_size = 0;
                string temp_name;
                H5DataType ll_dtype = var->dtype;

                // We always assume that longitude size is greater than latitude size.
                if(lat_size >lon_size) {
                    temp_size = lon_size;
                    temp_name = lon_name;
                    lon_size = lat_size;
                    lon_name = lat_name;
                    lat_size = temp_size;
                    lat_name = temp_name;
                }
                for (auto &cvar:this->cvars) {
                    if(cvar->cvartype == CV_FILLINDEX) {
                        if(cvar->getDimensions()[0]->size == lat_size &&
                           cvar->getDimensions()[0]->name == lat_name) {
                            cvar->cvartype = CV_LAT_MISS;
                            cvar->dtype = ll_dtype;
                            for (auto ira = cvar->attrs.begin(); ira != cvar->attrs.end(); ++ira) {
                                if ((*ira)->name == "NAME") {
                                    delete (*ira);
                                    cvar->attrs.erase(ira);
                                    break;
                                }
                            }
                        }
                        else if(cvar->getDimensions()[0]->size == lon_size &&
                           cvar->getDimensions()[0]->name == lon_name) {
                            cvar->cvartype = CV_LON_MISS;
                            cvar->dtype = ll_dtype;
                            for (auto ira = cvar->attrs.begin(); ira != cvar->attrs.end(); ++ira) {
                                if ((*ira)->name == "NAME") {
                                    delete (*ira);
                                    cvar->attrs.erase(ira);
                                    break;
                                }
                            }
                        }
                    }
                }
                break;

            } // end of if((var->fullpath.find("/geophysical_data") == 0) || (var->dtype == H5FLOAT32))
        } // end of if(var->rank == 2)
    } // end of for (auto irv = this->vars.begin();

}

// Handle some special variables. Currently only GPM  and ACOS have these variables.
void GMFile::Handle_SpVar() {

    BESDEBUG("h5", "Coming to Handle_SpVar()"<<endl);
    if (ACOS_L2S_OR_OCO2_L1B == product_type) 
        Handle_SpVar_ACOS_OCO2();
    else if(GPM_L1 == product_type) {
        // Loop through the variable list to build the coordinates.
        // These variables need to be removed.
        for (auto irv = this->vars.begin(); irv != this->vars.end(); ++irv) {
            if((*irv)->name=="AlgorithmRuntimeInfo") {
                delete(*irv);
                this->vars.erase(irv); 
                break;
            }
        }
    }

    // GPM level-3 These variables need to be removed.
    else if(GPMM_L3 == product_type || GPMS_L3 == product_type || GPM_L3_New==product_type) {

        for (auto irv = this->vars.begin(); irv != this->vars.end(); ) {
            if((*irv)->name=="InputFileNames") {
                delete(*irv);
                irv = this->vars.erase(irv);
            }
            else if((*irv)->name=="InputAlgorithmVersions") {
                delete(*irv);
                irv = this->vars.erase(irv);
            }
            else if((*irv)->name=="InputGenerationDateTimes") {
                delete(*irv);
                irv = this->vars.erase(irv);
            }
            else {
                ++irv;
            }
        }
   }

}

// Handle special variables for ACOS.
void GMFile::Handle_SpVar_ACOS_OCO2()  {

    BESDEBUG("h5", "Coming to Handle_SpVar_ACOS_OCO2()"<<endl);
    //The ACOS or OCO2 have 64-bit variables. DAP2 doesn't support 64-bit variables.
    // So we will not handle attributes yet.
    for (auto irv = this->vars.begin(); irv != this->vars.end(); ) {

        if (H5INT64 == (*irv)->getType()) {
            
            // First: Time Part of soundingid
            auto spvar_unique = make_unique<GMSPVar>(*irv);
            auto spvar = spvar_unique.release();
            spvar->name = (*irv)->name +"_Time";
            spvar->newname = (*irv)->newname+"_Time";
            spvar->dtype = H5INT32;
            spvar->otype = (*irv)->getType();
            spvar->sdbit = 1;

            // 2 digit hour, 2 digit min, 2 digit seconds
            spvar->numofdbits = 6;
            this->spvars.push_back(spvar);

            // Second: Date Part of soundingid
            auto spvar2_unique = make_unique<GMSPVar>(*irv);
            auto spvar2 = spvar2_unique.release();
            spvar2->name = (*irv)->name +"_Date";
            spvar2->newname = (*irv)->newname+"_Date";
            spvar2->dtype = H5INT32;
            spvar2->otype = (*irv)->getType();
            spvar2->sdbit = 7;

            // 4 digit year, 2 digit month, 2 digit day
            spvar2->numofdbits = 8;
            this->spvars.push_back(spvar2);

            delete(*irv);
            irv = this->vars.erase(irv);
        } // end of if (H5INT64 == (*irv)->getType())
        else {
            ++irv;
        }
    } // end of for (auto irv = this->vars.begin(); ...
}

// Adjust Object names, For some products, NASA data centers don't need 
// the fullpath of objects.            
void GMFile::Adjust_Obj_Name()  {

    BESDEBUG("h5", "Coming to Adjust_Obj_Name()"<<endl);
    if(Mea_Ozone == product_type) 
        Adjust_Mea_Ozone_Obj_Name();

    if(GPMS_L3 == product_type || GPMM_L3 == product_type)
        Adjust_GPM_L3_Obj_Name();

// Just for debugging
#if 0
for (auto irv2 = this->vars.begin();
    irv2 != this->vars.end(); irv2++) {
    for (auto ird = (*irv2)->dims.begin();
         ird !=(*irv2)->dims.end(); ird++) {
         cerr<<"Dimension name afet Adjust_Obj_Name "<<(*ird)->newname <<endl;
    }
}
#endif

}

// Adjust object names for GPM level 3 products
void GMFile:: Adjust_GPM_L3_Obj_Name()  const {

    BESDEBUG("h5", "Coming to Adjust_GPM_L3_Obj_Name()"<<endl);
    string objnewname;
    // In this definition, root group is not considered as a group.
    if(this->groups.size() <= 1) {
        for (auto &var:this->vars) {
            objnewname =  HDF5CFUtil::obtain_string_after_lastslash(var->newname);
            if (objnewname !="") 
                var->newname = objnewname;
        }
    }
    else {
        for (auto &var:this->vars) {
            size_t grid_group_path_pos = (var->newname.substr(1)).find_first_of("/");
            objnewname =  (var->newname).substr(grid_group_path_pos+2);
            var->newname = objnewname;
        }
    }
}

// Adjust object names for MeaSUREs OZone
void GMFile:: Adjust_Mea_Ozone_Obj_Name()  const {

    BESDEBUG("h5", "Coming to Adjust_Mea_Ozone_Obj_Name()"<<endl);
    string objnewname;
    for (auto &var:this->vars) {
        objnewname =  HDF5CFUtil::obtain_string_after_lastslash(var->newname);
        if (objnewname !="") 
           var->newname = objnewname;

#if 0
//Just for debugging
for (auto ird = (*irv)->dims.begin();
                ird !=(*irv)->dims.end();++ird) {
 cerr<<"Ozone dim. name "<<(*ird)->name <<endl;
 cerr<<"Ozone dim. new name "<<(*ird)->newname <<endl;
}
#endif

    }

    for (auto &cvar:this->cvars) {
        objnewname =  HDF5CFUtil::obtain_string_after_lastslash(cvar->newname);
        if (objnewname !="")
           cvar->newname = objnewname;
#if 0
 //Just for debugging
for (auto ird = (*irv)->dims.begin();
                ird !=(*irv)->dims.end();++ird) {
 cerr<<"Ozone CV dim. name "<<(*ird)->name <<endl;
 cerr<<"Ozone CV dim. new name "<<(*ird)->newname <<endl;
}   
#endif
    }
}

// Flatten object names. 
void GMFile::Flatten_Obj_Name(bool include_attr) {
     
    BESDEBUG("h5", "GMFile::Coming to Flatten_Obj_Name()"<<endl);
    // General variables
    File::Flatten_Obj_Name(include_attr);

    // Coordinate variables
    for (auto &cvar:this->cvars) {
        cvar->newname = get_CF_string(cvar->newname);

        for (auto &dim:cvar->dims) 
            dim->newname = get_CF_string(dim->newname);

        if (true == include_attr) {
            for (auto &attr: cvar->attrs)
                attr->newname = File::get_CF_string(attr->newname);
        }

    }

    // Special variables
    for (auto &spvar:this->spvars) {

        spvar->newname = get_CF_string(spvar->newname);
        for (auto &dim:spvar->dims)
            dim->newname = get_CF_string(dim->newname);

        if (true == include_attr) {
            for (auto &attr:spvar->attrs)
                  attr->newname = File::get_CF_string(attr->newname);
        }
    }

// Just for debugging
#if 0
for (auto irv2 = this->vars.begin();
    irv2 != this->vars.end(); irv2++) {
    for (auto ird = (*irv2)->dims.begin();
         ird !=(*irv2)->dims.end(); ird++) {
         cerr<<"Dimension name afet Flatten_Obj_Name "<<(*ird)->newname <<endl;
    }
}
#endif


}

// Rarely object name clashing may occur. This routine makes sure
// all object names are unique.
void GMFile::Handle_Obj_NameClashing(bool include_attr)  {

    BESDEBUG("h5", "GMFile::Coming to Handle_Obj_NameClashing()"<<endl);
    // objnameset will be filled with all object names that we are going to check the name clashing.
    // For example, we want to see if there are any name clashing for all variable names in this file.
    // objnameset will include all variable names. If a name clashing occurs, we can figure out from the set operation immediately.
    set<string>objnameset;
    Handle_GMCVar_NameClashing(objnameset);
    Handle_GMSPVar_NameClashing(objnameset);
    File::Handle_GeneralObj_NameClashing(include_attr,objnameset);
    if (true == include_attr) {
        Handle_GMCVar_AttrNameClashing();
        Handle_GMSPVar_AttrNameClashing();
    }
    // Moving to h5gmcfdap.cc, right after Adjust_Dim_Name
    //Handle_DimNameClashing();
}

// Name clashing for coordinate variables
void GMFile::Handle_GMCVar_NameClashing(set<string> &objnameset )  {

    GMHandle_General_NameClashing(objnameset,this->cvars);
}

// Name clashing for special variables(like 64-bit integer variables)
void GMFile::Handle_GMSPVar_NameClashing(set<string> &objnameset )  {

    GMHandle_General_NameClashing(objnameset,this->spvars);
}

// This routine handles attribute name clashing for coordinate variables.
void GMFile::Handle_GMCVar_AttrNameClashing()  {

    BESDEBUG("h5", "Coming to Handle_GMCVar_AttrNameClashing()"<<endl);
    set<string> objnameset;

    for (auto &cvar:this->cvars) {
        Handle_General_NameClashing(objnameset,cvar->attrs);
        objnameset.clear();
    }
}

// Attribute name clashing for special variables
void GMFile::Handle_GMSPVar_AttrNameClashing()  {

    BESDEBUG("h5", "Coming to Handle_GMSPVar_AttrNameClashing()"<<endl);
    set<string> objnameset;

    for (auto &spvar:this->spvars) {
        Handle_General_NameClashing(objnameset,spvar->attrs);
        objnameset.clear();
    }
}

//class T must have member string newname
// The subroutine to handle name clashing,
// it builds up a map from original object names to clashing-free object names.
template<class T> void
GMFile::GMHandle_General_NameClashing(set <string>&objnameset, vector<T*>& objvec) {

    BESDEBUG("h5", "Coming to GMHandle_General_NameClashing()"<<endl);
    pair<set<string>::iterator,bool> setret;
    set<string>::iterator iss;

    vector<string> clashnamelist;

    map<int,int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;

    typename vector<T*>::iterator irv;

    for (irv = objvec.begin(); irv != objvec.end(); ++irv) {

        setret = objnameset.insert((*irv)->newname);
        if (false == setret.second ) {
            clashnamelist.insert(clashnamelist.end(),(*irv)->newname);
            cl_to_ol[cl_index] = ol_index;
            cl_index++;
        }
        ol_index++;
    }


    // Now change the clashed elements to unique elements; 
    // Generate the set which has the same size as the original vector.
    for (auto &clashname:clashnamelist) {
        int clash_index = 1;
        string temp_clashname = clashname +'_';
        HDF5CFUtil::gen_unique_name(temp_clashname,objnameset,clash_index);
        clashname = temp_clashname;
    }


    // Now go back to the original vector, make it unique.
    for (unsigned int i =0; i <clashnamelist.size(); i++)
        objvec[cl_to_ol[i]]->newname = clashnamelist[i];
     
}

// Handle dimension name clashing
void GMFile::Handle_DimNameClashing() {


    BESDEBUG("h5", "GMFile: Coming to Handle_DimNameClashing()"<<endl);
    // ACOS L2S or OCO2 L1B products doesn't need the dimension name clashing check based on our current understanding. KY 2012-5-16
    if (ACOS_L2S_OR_OCO2_L1B == product_type) 
        return;

    map<string,string>dimname_to_dimnewname;
    pair<map<string,string>::iterator,bool>mapret;
    set<string> dimnameset;
    vector<Dimension*>vdims;
    set<string> dimnewnameset;
    pair<set<string>::iterator,bool> setret;

    // First: Generate the dimset/dimvar based on coordinate variables.
    for (const auto &cvar:this->cvars) {
        for (const auto &dim:cvar->dims) {
            setret = dimnameset.insert(dim->name);
            if (true == setret.second) 
                vdims.push_back(dim); 
        }
    }
    
    // For some cases, dimension names are provided but there are no corresponding coordinate
    // variables. For now, we will assume no such a case.
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

    GMHandle_General_NameClashing(dimnewnameset,vdims);
   
    // Third: Make dimname_to_dimnewname map
    for (const auto& dim:vdims) {
        mapret = dimname_to_dimnewname.insert(pair<string,string>(dim->name,dim->newname));
        if (false == mapret.second) 
            throw4("The dimension name ",dim->name," should map to ",
                                      dim->newname);
    }

    // Fourth: Change the original dimension new names to the unique dimension new names
    for (auto &cvar:this->cvars)
        for (auto &dim:cvar->dims)
            dim->newname = dimname_to_dimnewname[dim->name];

    for (auto &var:this->vars)
        for (auto &dim:var->dims)
            dim->newname = dimname_to_dimnewname[dim->name];

}
    
// For COARDS, dim. names need to be the same as obj. names.
void GMFile::Adjust_Dim_Name() {

    BESDEBUG("h5", "GMFile:Coming to Adjust_Dim_Name()"<<endl);
#if 0
    // Just for debugging
for (auto irv2 = this->vars.begin();
       irv2 != this->vars.end(); irv2++) {
    for (auto ird = (*irv2)->dims.begin();
           ird !=(*irv2)->dims.end(); ird++) {
         cerr<<"Dimension new name "<<(*ird)->newname <<endl;
       }
}
#endif

    // Only need for COARD conventions.
    if( true == iscoard) {
        for (const auto &cvar:this->cvars) {
#if 0
cerr<<"1D Cvariable name is "<<cvar->name <<endl;
cerr<<"1D Cvariable new name is "<<cvar->newname <<endl;
cerr<<"1D Cvariable dim name is "<<(cvar->dims)[0]->name <<endl;
cerr<<"1D Cvariable dim new name is "<<(cvar->dims)[0]->newname <<endl;
#endif
            if (cvar->dims.size()!=1) 
                throw3("Coard coordinate variable ",cvar->name, "is not 1D");
            if (cvar->newname != ((cvar->dims)[0]->newname)) {
                (cvar->dims)[0]->newname = cvar->newname;

                // For all variables that have this dimension,the dimension newname should also change.
                for (auto &var:this->vars) {
                    for (auto &dim:var->dims) {
                        // This is the key, the dimension name of this dimension 
                        // should be equal to the dimension name of the coordinate variable.
                        // Then the dimension name matches and the dimension name should be changed to
                        // the new dimension name.
                        if (dim->name == (cvar->dims)[0]->name) 
                            dim->newname = (cvar->dims)[0]->newname;
                    }
                }
            } // end of if (cvar->newname != ((cvar->dims)[0]->newname))
        }// end of for (auto irv = this->cvars.begin(); ...
   } // end of if( true == iscoard) 

// Just for debugging
#if 0
for (auto irv2 = this->vars.begin();
    irv2 != this->vars.end(); irv2++) {
    for (auto ird = (*irv2)->dims.begin();
         ird !=(*irv2)->dims.end(); ird++) {
         cerr<<"Dimension name afet Adjust_Dim_Name "<<(*ird)->newname <<endl;
    }
}
#endif


}

// Add supplemental CF attributes for some products.
void 
GMFile:: Add_Supplement_Attrs(bool add_path)  {

    BESDEBUG("h5", "GMFile::Coming to Add_Supplement_Attrs()"<<endl);
    if (General_Product == product_type || true == add_path) {
        File::Add_Supplement_Attrs(add_path);   

       if (true == add_path) {
            // Adding variable original name(origname) and full path(fullpath)
            for (auto &cvar:this->cvars) {
                if ((cvar->cvartype == CV_EXIST) || (cvar->cvartype == CV_MODIFY)) {
                    const string varname = cvar->name;
                    const string attrname = "origname";
                    auto attr_unique = make_unique<Attribute>();
                    auto attr = attr_unique.release();
                    Add_Str_Attr(attr,attrname,varname);
                    cvar->attrs.push_back(attr);
                }
            }

            for (auto &cvar:this->cvars) {
                // Turn off the fullnamepath attribute when zero_storage_size is 0.
                // Use the BES key since quite a few testing cases will be affected.
                // KY 2020-03-23
                if(cvar->zero_storage_size == false
                   || HDF5RequestHandler::get_no_zero_size_fullnameattr() == false) {
                    if ((cvar->cvartype == CV_EXIST) || (cvar->cvartype == CV_MODIFY)) {
                        const string varname = cvar->fullpath;
                        const string attrname = "fullnamepath";
                        auto attr_unique = make_unique<Attribute>();
                        auto attr = attr_unique.release();
                        Add_Str_Attr(attr,attrname,varname);
                        cvar->attrs.push_back(attr);
                    }
                }
            }

            for (auto &spvar:this->spvars) {
                const string varname = spvar->name;
                const string attrname = "origname";
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_Str_Attr(attr,attrname,varname);
                spvar->attrs.push_back(attr);
            }

            for (auto &spvar:this->spvars) {
                // Turn off the fullnamepath attribute when zero_storage_size is 0.
                // Use the BES key since quite a few testing cases will be affected.
                // KY 2020-03-23
                if(spvar->zero_storage_size == false
                   || HDF5RequestHandler::get_no_zero_size_fullnameattr() == false) {
                    const string varname = spvar->fullpath;
                    const string attrname = "fullnamepath";
                    auto attr_unique = make_unique<Attribute>();
                    auto attr = attr_unique.release();
                    Add_Str_Attr(attr,attrname,varname);
                    spvar->attrs.push_back(attr);
                }
            }
        }
    } // if (General_Product == product_type || true == add_path)

    if(GPM_L1 == product_type || GPMS_L3 == product_type || GPMM_L3 == product_type)
        Add_GPM_Attrs();
    else if (Aqu_L3 == product_type) 
        Add_Aqu_Attrs();
    else if (Mea_SeaWiFS_L2 == product_type || Mea_SeaWiFS_L3 == product_type) 
        Add_SeaWiFS_Attrs();
        
}

// Add CF attributes for GPM products
void 
GMFile:: Add_GPM_Attrs()  {

    BESDEBUG("h5", "Coming to Add_GPM_Attrs()"<<endl);
    const string attr_name_be_replaced = "CodeMissingValue";
    const string attr_new_name = "_FillValue";
    const string attr2_name_be_replaced = "Units";
    const string attr2_new_name ="units";

    // Need to convert String type CodeMissingValue to the corresponding _FilLValue
    // Create a function at HDF5CF.cc. use strtod,strtof,strtol etc. function to convert
    // string to the corresponding type.
    for (const auto &var:this->vars) {
        bool has_fvalue_attr = false;
        for (const auto &attr:var->attrs) {
            if(attr_new_name == attr->name) { 
                has_fvalue_attr = true;
                break;
            }
        }

        if(false == has_fvalue_attr) {
            for(auto &attr:var->attrs) {
                if(attr_name_be_replaced == attr->name) { 
                    if(attr->dtype == H5FSTRING) 
                        Change_Attr_One_Str_to_Others(attr,var);
                    attr->name = attr_new_name;
                    attr->newname = attr_new_name;
                }
            }
        }

    }
     
    
    for (auto &cvar: this->cvars) {

        bool has_fvalue_attr = false;

        for(const auto &attr:cvar->attrs) {

            if(attr_new_name == attr->name) {
                has_fvalue_attr = true;
                break;
            }
        }

        if(false == has_fvalue_attr) {

            for(auto &attr:cvar->attrs) {

                if(attr_name_be_replaced == attr->name) {
                    if(attr->dtype == H5FSTRING) 
                        Change_Attr_One_Str_to_Others(attr,cvar);
                    attr->name = attr_new_name;
                    attr->newname = attr_new_name;
                    break;
                }
            }
        }
        
    
        if(product_type == GPM_L1) {

            if (cvar->cvartype == CV_EXIST) {
                if(cvar->name.find("Latitude") !=string::npos) {
                    string unit_value = "degrees_north";
                    Correct_GPM_L1_LatLon_units(cvar,unit_value);

                }
                else if(cvar->name.find("Longitude") !=string::npos) {
                    string unit_value = "degrees_east";
                    Correct_GPM_L1_LatLon_units(cvar,unit_value);
                }
            } 
        

            else if (cvar->cvartype == CV_NONLATLON_MISS) {
            
            string comment;
            const string attrname = "comment";

            {
            if(cvar->name == "nchannel1") 
                comment = "Number of Swath S1 channels (10V 10H 19V 19H 23V 37V 37H 89V 89H).";
            else if(cvar->name == "nchannel2") 
                comment = "Number of Swath S2 channels (166V 166H 183+/-3V 183+/-8V).";
            else if(cvar->name == "nchan1")
                comment = "Number of channels in Swath 1.";
            else if(cvar->name == "nchan2")
                comment = "Number of channels in Swath 2.";
            else if(cvar->name == "VH") 
                comment = "Number of polarizations.";
            else if(cvar->name == "GMIxyz") 
                comment = "x, y, z components in GMI instrument coordinate system.";
            else if(cvar->name == "LNL")
                comment = "Linear and non-linear.";
            else if(cvar->name == "nscan")
                comment = "Number of scans in the granule.";
            else if(cvar->name == "nscan1")
                comment = "Typical number of Swath S1 scans in the granule.";
            else if(cvar->name == "nscan2")
                comment = "Typical number of Swath S2 scans in the granule.";
            else if(cvar->name == "npixelev")
                comment = "Number of earth view pixels in one scan.";
            else if(cvar->name == "npixelht")
                comment = "Number of hot load pixels in one scan.";
            else if(cvar->name == "npixelcs")
                comment = "Number of cold sky pixels in one scan.";
            else if(cvar->name == "npixelfr")
                comment = "Number of full rotation earth view pixels in one scan.";
            else if(cvar->name == "nfreq1")
                comment = "Number of frequencies in Swath 1.";
            else if(cvar->name == "nfreq2")
                comment = "Number of frequencies in Swath 2.";
            else if(cvar->name == "npix1")
                comment = "Number of pixels in Swath 1.";
            else if(cvar->name == "npix2")
                comment = "Number of pixels in Swath 2.";
            else if(cvar->name == "npix3")
                comment = "Number of pixels in Swath 3.";
            else if(cvar->name == "npix4")
                comment = "Number of pixels in Swath 4.";
            else if(cvar->name == "ncolds1")
                comment = "Maximum number of cold samples in Swath 1.";
            else if(cvar->name == "ncolds2")
                comment = "Maximum number of cold samples in Swath 2.";
            else if(cvar->name == "nhots1")
                comment = "Maximum number of hot samples in Swath 1.";
            else if(cvar->name == "nhots2")
                comment = "Maximum number of hot samples in Swath 2.";
            else if(cvar->name == "ntherm")
                comment = "Number of hot load thermisters.";
            else if(cvar->name == "ntach")
                comment = "Number of tachometer readings.";
            else if(cvar->name == "nsamt"){
                comment = "Number of sample types. ";
                comment = +"The types are: total science GSDR, earthview,hot load, cold sky.";
            }
            else if(cvar->name == "nndiode")
                comment = "Number of noise diodes.";
            else if(cvar->name == "n7")
                comment = "Number seven.";
            else if(cvar->name == "nray")
                comment = "Number of angle bins in each NS scan."; 
            else if(cvar->name == "nrayMS")
                comment = "Number of angle bins in each MS scan."; 
            else if(cvar->name == "nrayHS")
                comment = "Number of angle bins in each HS scan."; 
            else if(cvar->name == "nbin")
                comment = "Number of range bins in each NS and MS ray. Bin interval is 125m."; 
            else if(cvar->name == "nbinHS")
                comment = "Number of range bins in each HS ray. Bin interval is 250m."; 
            else if(cvar->name == "nbinSZP")
                comment = "Number of range bins for sigmaZeroProfile."; 
            else if(cvar->name == "nbinSZPHS")
                comment = "Number of range bins for sigmaZeroProfile in each HS scan."; 
            else if(cvar->name == "nNP")
                comment = "Number of NP kinds."; 
            else if(cvar->name == "nearFar")
                comment = "Near reference, Far reference."; 
            else if(cvar->name == "foreBack")
                comment = "Forward, Backward."; 
            else if(cvar->name == "method")
                comment = "Number of SRT methods."; 
            else if(cvar->name == "nNode")
                comment = "Number of binNode."; 
            else if(cvar->name == "nDSD")
                comment = "Number of DSD parameters. Parameters are N0 and D0"; 
            else if(cvar->name == "LS")
                comment = "Liquid, solid."; 
            }

            if (!comment.empty()) { 
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_Str_Attr(attr,attrname,comment);
                cvar->attrs.push_back(attr);
            }
         }
      }

      if(product_type == GPMS_L3 || product_type == GPMM_L3) {
        if (cvar->cvartype == CV_NONLATLON_MISS) {
            
            string comment;
            const string attrname = "comment";
            {
            if(cvar->name == "chn") 
                comment = "Number of channels:Ku,Ka,KaHS,DPR.";
            else if(cvar->name == "inst") 
                comment = "Number of instruments:Ku,Ka,KaHS.";
            else if(cvar->name == "tim")
                comment = "Number of hours(local time).";
            else if(cvar->name == "ang"){
                comment = "Number of angles.The meaning of ang is different for each channel.";
                comment +=  
                "For Ku channel all indices are used with the meaning 0,1,2,..6 =angle bins 24,";
                comment +=
                "(20,28),(16,32),(12,36),(8,40),(3,44),and (0,48).";
                comment +=
                "For Ka channel 4 indices are used with the meaning 0,1,2,3 = angle bins 12,(8,16),";
                comment +=
                "(4,20),and (0,24). For KaHS channel 4 indices are used with the meaning 0,1,2,3 =";
                comment += "angle bins(11,2),(7,16),(3,20),and (0.23).";
    
            }
            else if(cvar->name == "rt") 
                comment = "Number of rain types: stratiform, convective,all.";
            else if(cvar->name == "st") 
                comment = "Number of surface types:ocean,land,all.";
            else if(cvar->name == "bin"){
                comment = "Number of bins in histogram. The thresholds are different for different";
                comment +=" variables. see the file specification for this algorithm.";
            }
            else if(cvar->name == "nvar") {
                comment = "Number of phase bins. Bins are counts of phase less than 100, ";
                comment +="counts of phase greater than or equal to 100 and less than 200, ";
                comment +="counts of phase greater than or equal to 200.";
            }
            else if(cvar->name == "AD")
                comment = "Ascending or descending half of the orbit.";
            }

            if (comment.empty() == false) {
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_Str_Attr(attr,attrname,comment);
                cvar->attrs.push_back(attr);
            }
         }
      }

      if (cvar->cvartype == CV_SPECIAL) {
            if(cvar->name == "nlayer" || cvar->name == "hgt"
               || cvar->name == "nalt") {
                string unit_value = "km";
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_Str_Attr(attr,attr2_new_name,unit_value);
                cvar->attrs.push_back(attr);

                string attr1_axis="axis";
                string attr1_value = "Z";
                auto attr1_unique = make_unique<Attribute>();
                auto attr1 = attr1_unique.release();
                Add_Str_Attr(attr1,attr1_axis,attr1_value);
                cvar->attrs.push_back(attr1);

                string attr2_positive="positive";
                string attr2_value = "up";
                auto attr2_unique = make_unique<Attribute>();
                auto attr2 = attr2_unique.release();
                Add_Str_Attr(attr2,attr2_positive,attr2_value);
                cvar->attrs.push_back(attr2);

            }
            if(cvar->name == "hgt" || cvar->name == "nalt"){
                string comment ="Number of heights above the earth ellipsoid";
                auto attr1_unique = make_unique<Attribute>();
                auto attr1 = attr1_unique.release();
                Add_Str_Attr(attr1,"comment",comment);
                cvar->attrs.push_back(attr1);
            }

        }

    }

    
// Old code, leave it for the time being
#if 0
    const string fill_value_attr_name = "_FillValue";
    vector<HDF5CF::Var *>::const_iterator it_v;
    vector<HDF5CF::Attribute *>::const_iterator ira;

    for (it_v = vars.begin();
                it_v != vars.end(); ++it_v) {

        bool has_fillvalue = false;
        for(ira = (*it_v)->attrs.begin(); ira!= (*it_v)->attrs.end();ira++) {
            if (fill_value_attr_name == (*ira)->name){
                has_fillvalue = true;
                break;
            }

        }

        // Add the fill value
        if (has_fillvalue != true ) {
            
            if(H5FLOAT32 == (*it_v)->dtype) {
                Attribute* attr = new Attribute();
                    float _FV = -9999.9;
                    Add_One_Float_Attr(attr,fill_value_attr_name,_FV);
               (*it_v)->attrs.push_back(attr);
            }
        }
    }// for (it_v = vars.begin(); ...
#endif

}

// For GPM level 1 data, var must have names that contains either "Latitude" nor "Longitude".
void 
GMFile:: Correct_GPM_L1_LatLon_units(Var *var, const string & unit_value)  {

    BESDEBUG("h5", "Coming to Correct_GPM_L1_LatLon_units()"<<endl);
    const string Unit_name = "Units";
    const string unit_name = "units";

    // Delete "units"  and "Units"
    for(auto ira = var->attrs.begin(); ira!= var->attrs.end();) {
        if(unit_name == (*ira)->name) {
            delete(*ira);
            ira = var->attrs.erase(ira); 
        }
        else if(Unit_name == (*ira)->name) {
            delete(*ira);
            ira = var->attrs.erase(ira);
        }
        else 
            ++ira;
    }
    // Add the correct units for Latitude and Longitude 
    // Note: the reason we do this way, for some versions of GPM, units is degrees,
    // rather than degrees_north. So units also needs to be corrected to follow CF.
    auto attr_unique = make_unique<Attribute>();
    auto attr = attr_unique.release();
    Add_Str_Attr(attr,unit_name,unit_value);
    var->attrs.push_back(attr);
}



// Add attributes for Aquarius products
void 
GMFile:: Add_Aqu_Attrs()  {

    BESDEBUG("h5", "Coming to Add_Aqu_Attrs()"<<endl);
    vector<HDF5CF::Var *>::const_iterator it_v;
    vector<HDF5CF::Attribute *>::const_iterator ira;

    const string orig_longname_attr_name = "Parameter";
    const string longname_attr_name ="long_name";
    bool has_orig_longname = false;
    string longname_value;
    

    const string orig_units_attr_name = "Units";
    const string units_attr_name = "units";
    bool has_orig_units = false;
    string units_value;
    
    const string orig_valid_min_attr_name = "Data Minimum";
    const string orig_valid_min_attr_name2 = "data_minimum";
    const string valid_min_attr_name = "valid_min";
    bool has_orig_valid_min = false;
    float valid_min_value = 0;

    const string orig_valid_max_attr_name = "Data Maximum";
    const string orig_valid_max_attr_name2 = "data_maximum";
    const string valid_max_attr_name = "valid_max";
    bool has_orig_valid_max = false;
    float valid_max_value = 0;

    // The fill value is -32767.0. However, No _FillValue attribute is added.
    // So add it here. KY 2012-2-16
 
    const string fill_value_attr_name = "_FillValue";
    float _FV = -32767.0;

    for (ira = this->root_attrs.begin(); ira != this->root_attrs.end(); ++ira) {
        if (orig_longname_attr_name == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            longname_value.resize((*ira)->value.size());
            copy((*ira)->value.begin(),(*ira)->value.end(),longname_value.begin());
            has_orig_longname = true;
        }
        else if (orig_units_attr_name == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            units_value.resize((*ira)->value.size());
            copy((*ira)->value.begin(),(*ira)->value.end(),units_value.begin());
            has_orig_units = true;
        }
        else if (orig_valid_min_attr_name == (*ira)->name || orig_valid_min_attr_name2 == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            memcpy(&valid_min_value,(void*)(&((*ira)->value[0])),(*ira)->value.size());
            has_orig_valid_min = true;
        }

        else if (orig_valid_max_attr_name == (*ira)->name || orig_valid_max_attr_name2 == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            memcpy(&valid_max_value,(void*)(&((*ira)->value[0])),(*ira)->value.size());
            has_orig_valid_max = true;
        }
        
    }// end of for (ira = this->root_attrs.begin(); ira != this->root_attrs.end(); ++ira)

    // New version Aqu(Q20112132011243.L3m_MO_SCI_V3.0_SSS_1deg.bz2) files seem to have CF attributes added. 
    // In this case, we should not add extra CF attributes, or duplicate values may appear. KY 2015-06-20
    bool has_long_name = false;
    bool has_units = false;
    bool has_valid_min = false;
    bool has_valid_max = false;
    bool has_fillvalue = false;

    for (it_v = vars.begin(); it_v != vars.end(); ++it_v) {
        if ("l3m_data" == (*it_v)->name) {
            for (ira = (*it_v)->attrs.begin(); ira != (*it_v)->attrs.end(); ++ira) {
                if (longname_attr_name == (*ira)->name) 
                    has_long_name = true;
                else if(units_attr_name == (*ira)->name)
                    has_units = true;
                else if(valid_min_attr_name == (*ira)->name)
                    has_valid_min = true;
                else if(valid_max_attr_name == (*ira)->name)
                    has_valid_max = true;
                else if(fill_value_attr_name == (*ira)->name)
                    has_fillvalue = true;
            }
            break;
        }
    } // end of for (it_v = vars.begin(); ...


    // Level 3 variable name is l3m_data
    for (it_v = vars.begin(); it_v != vars.end(); ++it_v) {
        if ("l3m_data" == (*it_v)->name) {

            // 1. Add the long_name attribute if no
            if(false == has_long_name && true == has_orig_longname) {
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_Str_Attr(attr,longname_attr_name,longname_value);
                (*it_v)->attrs.push_back(attr);
            }

            // 2. Add the units attribute
            if(false == has_units && true == has_orig_units) {
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_Str_Attr(attr,units_attr_name,units_value);
                (*it_v)->attrs.push_back(attr);
            }

            // 3. Add the valid_min attribute
            if (false == has_valid_min && has_orig_valid_min == true) {
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_One_Float_Attr(attr,valid_min_attr_name,valid_min_value);
                (*it_v)->attrs.push_back(attr);
            }

            // 4. Add the valid_max attribute
            if(false == has_valid_max && has_orig_valid_max == true) {
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_One_Float_Attr(attr,valid_max_attr_name,valid_max_value);
                (*it_v)->attrs.push_back(attr);
            }

            // 5. Add the _FillValue attribute
            if(false == has_fillvalue) {
               auto attr_unique = make_unique<Attribute>();
               auto attr = attr_unique.release();
               Add_One_Float_Attr(attr,fill_value_attr_name,_FV);
               (*it_v)->attrs.push_back(attr);
            }

            break;
        }
    } // for (it_v = vars.begin(); ...
}

// Add SeaWiFS attributes
void 
GMFile:: Add_SeaWiFS_Attrs() const {

    BESDEBUG("h5", "Coming to Add_SeaWiFS_Attrs()"<<endl);
    // The fill value is -999.0. However, No _FillValue attribute is added.
    // So add it here. KY 2012-2-16
    const string fill_value_attr_name = "_FillValue";
    float _FV = -999.0;
    const string valid_range_attr_name = "valid_range";

    for (auto &var:this->vars) {
        if (H5FLOAT32 == var->dtype) {
            bool has_fillvalue = false;
            bool has_validrange = false;
            for(const auto &attr:var->attrs) {
                if (fill_value_attr_name == attr->name){
                    has_fillvalue = true;
                    break;
                }
                else if(valid_range_attr_name == attr->name) {
                    has_validrange = true;
                    break;
                }
            }
            // Add the fill value
            if (has_fillvalue != true && has_validrange != true ) {
                auto attr_unique = make_unique<Attribute>();
                auto attr = attr_unique.release();
                Add_One_Float_Attr(attr,fill_value_attr_name,_FV);
                var->attrs.push_back(attr);
            }
        }
    }
}

// Leave the following code for the time being
#if 0
// Handle the "coordinates" and "units" attributes of coordinate variables.
void GMFile:: Handle_Coor_Attr() {

    string co_attrname = "coordinates";
    string co_attrvalue="";
    string unit_attrname = "units";
    string nonll_unit_attrvalue ="level";
    string lat_unit_attrvalue ="degrees_north";
    string lon_unit_attrvalue ="degrees_east";

    for (auto ircv = this->cvars.begin();
        ircv != this->cvars.end(); ++ircv) {
//cerr<<"CV name is "<<(*ircv)->name << " cv type is "<<(*ircv)->cvartype <<endl;

        if ((*ircv)->cvartype == CV_NONLATLON_MISS) {
           Attribute * attr = new Attribute();
           Add_Str_Attr(attr,unit_attrname,nonll_unit_attrvalue);
           (*ircv)->attrs.push_back(attr);
        }

        else if ((*ircv)->cvartype == CV_LAT_MISS) {
//cerr<<"Should add new attribute "<<endl;
           Attribute * attr = new Attribute();
//           float temp = -999.9;
//           Add_One_Float_Attr(attr,unit_attrname,temp);
           Add_Str_Attr(attr,unit_attrname,lat_unit_attrvalue);
           (*ircv)->attrs.push_back(attr);
//cerr<<"After adding new attribute "<<endl;
        }

        else if ((*ircv)->cvartype == CV_LON_MISS) {
           Attribute * attr = new Attribute();
           Add_Str_Attr(attr,unit_attrname,lon_unit_attrvalue);
           (*ircv)->attrs.push_back(attr);
        }
    } // for (auto ircv = this->cvars.begin(); ...
   
    // No need to handle MeaSUREs SeaWiFS level 2 products
    if(product_type == Mea_SeaWiFS_L2) 
        return; 

    // GPM level 1 needs to be handled separately
    else if(product_type == GPM_L1) {
        Handle_GPM_l1_Coor_Attr();
        return;
    }
    // No need to handle products that follow COARDS.
    else if (true == iscoard) {
        // May need to check coordinates for 2-D lat/lon but cannot treat those lat/lon as CV case. KY 2015-12-10-TEMPPP
        return;
    }
   

    // Now handle the 2-D lat/lon case(note: this only applies to the one that dim. scale doesn't apply)
    for (auto ircv = this->cvars.begin();
        ircv != this->cvars.end(); ++ircv) {
        if((*ircv)->rank == 2) {

            // The following code makes sure that the replacement only happens with the general 2-D lat/lon case.
            if(gp_latname == (*ircv)->name) 
                Replace_Var_Str_Attr((*ircv),unit_attrname,lat_unit_attrvalue);
            else if(gp_lonname ==(*ircv)->name) 
                Replace_Var_Str_Attr((*ircv),unit_attrname,lon_unit_attrvalue);
        }
    }
    
    // Check the dimension names of 2-D lat/lon CVs
    string ll2d_dimname0,ll2d_dimname1;
    bool has_ll2d_coords = false;
    for (auto ircv = this->cvars.begin();
        ircv != this->cvars.end(); ++ircv) {
        if((*ircv)->rank == 2) {
            // Note: we should still use the original dim. name to match the general variables. 
            ll2d_dimname0 = (*ircv)->getDimensions()[0]->name;
            ll2d_dimname1 = (*ircv)->getDimensions()[1]->name;
            if(ll2d_dimname0 !="" && ll2d_dimname1 !="")
                has_ll2d_coords = true;
            break;
        }
    }
    
    if(true == has_ll2d_coords) {
 
        for (auto irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

            bool coor_attr_keep_exist = false;

            // May need to delete only the "coordinates" with both 2-D lat/lon dim. KY 2015-12-07
            if(((*irv)->rank >=2)) { 
                
                short ll2dim_flag = 0;
                for (auto ird = (*irv)->dims.begin();
                    ird != (*irv)->dims.end(); ++ ird) {
                    if((*ird)->name == ll2d_dimname0)
                        ll2dim_flag++;
                    else if((*ird)->name == ll2d_dimname1)
                        ll2dim_flag++;
                }

                if(ll2dim_flag != 2) 
                    coor_attr_keep_exist = true;

                // The following line doesn't apply to SMAP,it applies to Old SMAP Level 2 Simulation files. 
                if(product_type == OSMAPL2S)
                    coor_attr_keep_exist = true;

                if (false == coor_attr_keep_exist) {
                    for (auto ira =(*irv)->attrs.begin();
                        ira !=(*irv)->attrs.end();) {
                        if ((*ira)->newname == co_attrname) {
                            delete (*ira);
                            ira = (*irv)->attrs.erase(ira);
                        }
                        else {
                            ++ira;
                        }
                    }// for (auto ira =(*irv)->attrs.begin(); ...
                
                    // Generate the "coordinates" attribute only for variables that have both 2-D lat/lon dim. names.
                    for (auto ird = (*irv)->dims.begin();
                        ird != (*irv)->dims.end(); ++ ird) {
                        for (auto ircv = this->cvars.begin();
                            ircv != this->cvars.end(); ++ircv) {
                            if ((*ird)->name == (*ircv)->cfdimname) 
                                co_attrvalue = (co_attrvalue.empty())
                                    ?(*ircv)->newname:co_attrvalue + " "+(*ircv)->newname;
                        }
                    }

                    if (false == co_attrvalue.empty()) {
                        Attribute * attr = new Attribute();
                        Add_Str_Attr(attr,co_attrname,co_attrvalue);
                       (*irv)->attrs.push_back(attr);
                    }

                    co_attrvalue.clear();
                } // for (auto irv = this->vars.begin(); ...
            }
        }
    }
}
#endif


// Handle the "coordinates" and "units" attributes of coordinate variables.
void GMFile:: Handle_Coor_Attr() {

    BESDEBUG("h5", "GMFile::Coming to Handle_Coor_Attr()"<<endl);
    string co_attrname = "coordinates";
    string co_attrvalue;
    string unit_attrname = "units";
    string nonll_unit_attrvalue ="level";
    string lat_unit_attrvalue ="degrees_north";
    string lon_unit_attrvalue ="degrees_east";

    // Attribute units should be added for coordinate variables that
    // have the type CV_NONLATLON_MISS,CV_LAT_MISS and CV_LON_MISS.
    for (auto &cvar:this->cvars) {

        if (cvar->cvartype == CV_NONLATLON_MISS) {
           auto attr_unique = make_unique<Attribute>();
           auto attr = attr_unique.release();
           Add_Str_Attr(attr,unit_attrname,nonll_unit_attrvalue);
           cvar->attrs.push_back(attr);
        }
        else if (cvar->cvartype == CV_LAT_MISS) {
           auto attr_unique = make_unique<Attribute>();
           auto attr = attr_unique.release();
           Add_Str_Attr(attr,unit_attrname,lat_unit_attrvalue);
           cvar->attrs.push_back(attr);
        }
        else if (cvar->cvartype == CV_LON_MISS) {
           auto attr_unique = make_unique<Attribute>();
           auto attr = attr_unique.release();
           Add_Str_Attr(attr,unit_attrname,lon_unit_attrvalue);
           cvar->attrs.push_back(attr);
        }
    } // end of for 
   
    // No need to handle MeaSUREs SeaWiFS level 2 products
    if(product_type == Mea_SeaWiFS_L2) 
        return; 

    // GPM level 1 needs to be handled separately
    else if(product_type == GPM_L1) {
        Handle_GPM_l1_Coor_Attr();
        return;
    }

    // Handle Lat/Lon with "coordinates" attribute.
    else if(product_type == General_Product && gproduct_pattern == GENERAL_LATLON_COOR_ATTR){
        Handle_LatLon_With_CoordinateAttr_Coor_Attr();
        return;
    }
    // No need to handle products that follow COARDS.
    else if (true == iscoard) {

        // If we find that there are groups that should check the coordinates attribute of the variable. 
        // We should flatten the path inside the coordinates.(this is the case mainly for netcdf-4 2D lat/lon case)
        if(grp_cv_paths.empty() == false) {
            for (auto &var:this->vars) {
                if(grp_cv_paths.find(HDF5CFUtil::obtain_string_before_lastslash(var->fullpath)) != grp_cv_paths.end()){ 
                    // Check the "coordinates" attribute and flatten the values. 
                    Flatten_VarPath_In_Coordinates_Attr(var);
                }
            }
        }
        return;
    }
   
    // Now handle the 2-D lat/lon case
    for (auto &cvar:this->cvars) {

        if(cvar->rank == 2 && cvar->cvartype == CV_EXIST) {
            //Note: When the 2nd parameter is true in the function Is_geolatlon, it checks the lat/latitude/Latitude 
            //      When the 2nd parameter is false in the function Is_geolatlon, it checks the lon/longitude/Longitude
            // The following code makes sure that the replacement only happens with the general 2-D lat/lon case.
            // The following code is commented out since we find an OMPS-NPP case that has the no-CF unit for
            // "Latitude". So just to check the latitude and longitude and if the units are not CF-compliant, 
            // change them. KY 2020-02-27
#if 0
            if(gp_latname == cvar->name) { 
                // Only if gp_latname is not lat/latitude/Latitude, change the units
                if(false == Is_geolatlon(gp_latname,true)) 
                    Replace_Var_Str_Attr(cvar,unit_attrname,lat_unit_attrvalue);
            }
            else if(gp_lonname ==cvar->name) { 
                // Only if gp_lonname is not lon/longitude/Longitude, change the units
                if(false == Is_geolatlon(gp_lonname,false))
                    Replace_Var_Str_Attr(cvar,unit_attrname,lon_unit_attrvalue);
            }
#endif

            // We meet several products that miss the 2-D latitude and longitude CF units, although they
            // have the CV names like latitude/longitude, we should double-check this case,
            // and add the correct CF units if possible. We will watch if this is the right way.
            //else if(true == Is_geolatlon(cvar->name,true))
            if(true == Is_geolatlon(cvar->name,true))
                Replace_Var_Str_Attr(cvar,unit_attrname,lat_unit_attrvalue);
            else if(true == Is_geolatlon(cvar->name,false))
                Replace_Var_Str_Attr(cvar,unit_attrname,lon_unit_attrvalue);
        }
    } // for (auto ircv = this->cvars.begin()
    
    // If we find that there are groups that we should check the coordinates attribute of the variable under, 
    // we should flatten the path inside the coordinates. Note this is for 2D-latlon CV netCDF-4-like case.
    if (grp_cv_paths.empty() == false) {
        for (auto &var:this->vars) {
            if(grp_cv_paths.find(HDF5CFUtil::obtain_string_before_lastslash(var->fullpath)) != grp_cv_paths.end()){ 
                // Check the "coordinates" attribute and flatten the values. 
                Flatten_VarPath_In_Coordinates_Attr(var);
            }
        }
    }
    
    // Check if having 2-D lat/lon CVs
    bool has_ll2d_coords = false;

    // Since iscoard is false up to this point, So the netCDF-4 like 2-D lat/lon case must fulfill if the program comes here.
    if(General_Product == this->product_type && GENERAL_DIMSCALE == this->gproduct_pattern) 
        has_ll2d_coords = true; 
    else {// For other cases. Need to see if there is a case. KY 2016-07-07
        string ll2d_dimname0;
        string ll2d_dimname1;
        for (const auto &cvar:this->cvars) {
            if(cvar->rank == 2) {
                // Note: we should still use the original dim. name to match the general variables. 
                ll2d_dimname0 = cvar->getDimensions()[0]->name;
                ll2d_dimname1 = cvar->getDimensions()[1]->name;
                if(ll2d_dimname0 !="" && ll2d_dimname1 !="")
                    has_ll2d_coords = true;
                break;
            }
        }
    }
    
    // We now walk through all the >=2 vars and flatten the "coordinates"
    if(true == has_ll2d_coords) {
 
        // For some netCDF-4-like 2-D lat/lon cases, we may need to forcely flatten the coordinates.
        // This case usually happens when the data producers follow the CF and the NASA DIWG guideline to
        // provide the absolute path of the coordinates as the value of the "coordinates" attribute. 
        // The handler doesn't need to figure out the contents of the coordinates attribute but to
        // flatten the path inside. 
        // However, the BES Key FORCENDCoorAttr must be set.
        bool force_flatten_coor_attr = HDF5RequestHandler::get_force_flatten_coor_attr();

        // We also need to find if we have coordinates attribute for >=2D variables.
        // If not, the handler has to figure out the coordinates. 
        bool has_coor_attr_ge_2d_vars = false;
        for (const auto &var:this->vars) {
            if(var->rank >=2){
                for (const auto &attr:var->attrs) {
                    // We will check if we have the coordinate attribute 
                    if(attr->name == co_attrname) {
                        has_coor_attr_ge_2d_vars = true;
                        break;
                    }
                }
                if(has_coor_attr_ge_2d_vars == true)
                    break;
            }
        }
#if 0
        // Here we may need to consider the special case for HDF-EOS5. The "Data Fields" etc should not be 
        // in the group path. May need to let DIWG provide a guideline for this issue. 
        bool is_hybrid_eos5= false;
        if(force_flatten_coor_attr == true && has_coor_attr_ge_2d_vars == true)
            is_hybrid_eos5 = Is_Hybrid_EOS5();
#endif
        for (auto &var:this->vars) {

            bool has_coor = false;
            for (const auto &attr:var->attrs) {
                // We will check if we have the coordinate attribute 
                if(attr->name == co_attrname) {
                    has_coor = true;
                    break;
                }
            }
 
            // The coordinates attribute is flattened by force.
            if(true == force_flatten_coor_attr && true == has_coor) { 
#if 0
                if(is_hybrid_eos5 == true) {
                    Flatten_VarPath_In_Coordinates_Attr_EOS5(var);
                }
                else 
#endif
                    Flatten_VarPath_In_Coordinates_Attr(var);
            }
            
            else if((var->rank >=2) && (has_coor_attr_ge_2d_vars == false || false == force_flatten_coor_attr)) { 
               
                bool coor_attr_keep_exist = false;

                // Check if this var is under group_cv_paths, no, then check if this var's dims are the same as the dims of 2-D CVars 
                if(grp_cv_paths.find(HDF5CFUtil::obtain_string_before_lastslash(var->fullpath)) == grp_cv_paths.end()) 

                    // If finding this var is associated with 2-D lat/lon CVs, not keep the original "coordinates" attribute.
                    coor_attr_keep_exist = Check_Var_2D_CVars(var);
                else {
                    coor_attr_keep_exist = true;
                }
                
                // The following two lines are just for old smap level 2 case. 
                if(product_type == OSMAPL2S)
                    coor_attr_keep_exist = true;

                // Need to delete the original "coordinates" and rebuild the "coordinates" if this var is associated with the 2-D lat/lon CVs.
                if (false == coor_attr_keep_exist) {
                    for (auto ira =var->attrs.begin(); ira !=var->attrs.end();) {
                        if ((*ira)->newname == co_attrname) {
                            delete (*ira);
                            ira = var->attrs.erase(ira);
                        }
                        else {
                            ++ira;
                        }
                    }// for (auto ira =var->attrs.begin(); ...
                
                    // Generate the new "coordinates" attribute. 
                    for (const auto &dim:var->dims) {
                        for (const auto &cvar:this->cvars) {
                            if (dim->name == cvar->cfdimname) 
                                co_attrvalue = (co_attrvalue.empty())
                                    ?cvar->newname:co_attrvalue + " "+cvar->newname;
                        }
                    }

                    if (false == co_attrvalue.empty()) {
                        auto attr_unique = make_unique<Attribute>();
                        auto attr = attr_unique.release();
                        Add_Str_Attr(attr,co_attrname,co_attrvalue);
                        var->attrs.push_back(attr);
                    }

                    co_attrvalue.clear();
                    var->coord_attr_add_path = false;
                } // for (auto irv = this->vars.begin(); ...
            }
        }
    }
}

// Handle GPM level 1 coordinates attributes.
void GMFile:: Handle_GPM_l1_Coor_Attr() const {

    BESDEBUG("h5", "Coming to Handle_GPM_l1_Coor_Attr()"<<endl);
    // Build a map from CFdimname to 2-D lat/lon variable name, should be something like: aa_list[cfdimname]=s1_latitude .
    // Loop all variables
    // Inner loop: for all dims of a var
    // if (dimname matches the dim(not cfdim) name) of one of 2-D lat/lon,
    // check if the variable's full path contains the path of one of 2-D lat/lon,
    // yes, build its cfdimname = path+ dimname, check this cfdimname with the cfdimname of the corresponding 2-D lat/lon
    //      If matched, save this latitude variable name as one of the coordinate variable. 
    //      else this is a 3rd-dimension cv, just use the dimension name(or the corresponding cv name maybe through a map).

    // Prepare 1) 2-D CVar(lat,lon) corresponding dimension name set.
    //         2) cfdim name to cvar name map(don't need to use a map, just a holder). It should be fine.

    // "coordinates" attribute name and value.  We only need to provide this attribute for variables that have 2-D lat/lon
    string co_attrname = "coordinates";
    string co_attrvalue;

    // 2-D cv dimname set.
    set<string> cvar_2d_dimset;

    pair<map<string,string>::iterator,bool>mapret;

    // Hold the mapping from cfdimname to 2-D cvar name. Something like nscan->lat, npixel->lon 
    map<string,string>cfdimname_to_cvar2dname;

    // Loop through cv variables to build 2-D cv dimname set and the mapping from cfdimname to 2-D cvar name.
    for (const auto &cvar:this->cvars) {

        //This CVar must be 2-D array.
        if(cvar->rank == 2) { 

#if 0
//cerr<<"2-D cv name is "<<cvar->name <<endl;
//cerr<<"2-D cv new name is "<<cvar->newname <<endl;
//cerr<<"cvar->cfdimname is "<<cvar->cfdimname <<endl;
#endif

            for (const auto &dim:cvar->dims) 
                cvar_2d_dimset.insert(dim->name);

            // Generate cfdimname to cvar2d map
            mapret = cfdimname_to_cvar2dname.insert(pair<string,string>(cvar->cfdimname,cvar->newname));      
            if (false == mapret.second)
                throw4("The cf dimension name ",cvar->cfdimname," should map to 2-D coordinate variable",
                                      cvar->newname);
        }
    }

    // Loop through the variable list to build the coordinates.
    for (auto &var:this->vars) {

        // Only apply to >2D variables.
        if(var->rank >=2) {

            // The variable dimension names must be found in the 2D cvar dim. nameset.
            // The flag must be at least 2.
            short have_2d_dimnames_flag = 0;
            for (const auto &dim:var->dims) {
                if (cvar_2d_dimset.find(dim->name)!=cvar_2d_dimset.end())    
                    have_2d_dimnames_flag++;
            }

            // Final candidates to have 2-D CVar coordinates. 
            if(have_2d_dimnames_flag >=2) {

                // Obtain the variable path
                string var_path;
                if(var->fullpath.size() > var->name.size()) 
                    var_path=var->fullpath.substr(0,var->fullpath.size()-var->name.size());
                else 
                    throw4("The variable full path ",var->fullpath," doesn't contain the variable name ",
                                      var->name);

                // A flag to identify if this variable really needs the 2-D coordinate variables.
                short cv_2d_flag = 0;

                // 2-D coordinate variable names for the potential variable candidate
                vector<string> cv_2d_names;

                // Dimension names of the 2-D coordinate variables.
                set<string> cv_2d_dimnames;
                 
                // Loop through the map from dim. name to coordinate name.
                for(map<string,string>::const_iterator itm = cfdimname_to_cvar2dname.begin();
                    itm != cfdimname_to_cvar2dname.end();++itm) {

                    // Obtain the dimension name from the cfdimname.
                    string reduced_dimname = HDF5CFUtil::obtain_string_after_lastslash(itm->first);
                    string cfdim_path;
                    if(itm->first.size() <= reduced_dimname.size()) 
                        throw2("The cf dim. name of this dimension is not right.",itm->first);
                    else
                        cfdim_path= itm->first.substr(0,itm->first.size() - reduced_dimname.size());
                    // cfdim_path will not be nullptr only when the cfdim name is for the 2-D cv var.

                    // Find the correct path,
                    // Note: 
                    // var_path doesn't have to be the same as cfdim_path
                    // consider the variable /a1/a2/foo and the latitude /a1/latitude(cfdimpath is /a1)
                    // If there is no /a1/a2/latitude, the /a1/latitude can be used as the coordinate of /a1/a2/foo.
                    // But we want to check if var_path is the same as cfdim_path first. So we check cfdimname_to_cvarname again.
                    if(var_path == cfdim_path) {
                        for (const auto &dim:var->dims) {
                            if(reduced_dimname == dim->name) {
                               cv_2d_flag++;
                               cv_2d_names.push_back(itm->second);
                               cv_2d_dimnames.insert(dim->name);
                            }
                        }
                    }
                }

                // Note: 
                // var_path doesn't have to be the same as cfdim_path
                // consider the variable /a1/a2/foo and the latitude /a1/latitude(cfdimpath is /a1)
                // If there is no /a1/a2/latitude, the /a1/latitude can be used as the coordinate of /a1/a2/foo.
                // The variable  has 2 coordinates(dimensions) if the flag is 2
                // If the flag is not 2, we will see if the above case stands.
                if(cv_2d_flag != 2) {
                    cv_2d_flag = 0;
                    // Loop through the map from dim. name to coordinate name.
                    for(map<string,string>::const_iterator itm = cfdimname_to_cvar2dname.begin();
                        itm != cfdimname_to_cvar2dname.end();++itm) {
                        // Obtain the dimension name from the cfdimname.
                        string reduced_dimname = HDF5CFUtil::obtain_string_after_lastslash(itm->first);
                        string cfdim_path;
                        if(itm->first.size() <= reduced_dimname.size()) 
                            throw2("The cf dim. name of this dimension is not right.",itm->first);
                        else
                            cfdim_path= itm->first.substr(0,itm->first.size() - reduced_dimname.size());
                        // cfdim_path will not be nullptr only when the cfdim name is for the 2-D cv var.

                        // Find the correct path,
                        // Note: 
                        // var_path doesn't have to be the same as cfdim_path
                        // consider the variable /a1/a2/foo and the latitude /a1/latitude(cfdimpath is /a1)
                        // If there is no /a1/a2/latitude, the /a1/latitude can be used as the coordinate of /a1/a2/foo.
                        // 
                        if(var_path.find(cfdim_path)!=string::npos) {
                            for (const auto &dim:var->dims) {
                                if(reduced_dimname == dim->name) {
                                   cv_2d_flag++;
                                   cv_2d_names.push_back(itm->second);
                                   cv_2d_dimnames.insert(dim->name);
                                }
                            }
                        }
                    
                    }
                }

                // Now we got all cases.
                if(2 == cv_2d_flag) {

                    // Add latitude and longitude to the 'coordinates' attribute.
                    co_attrvalue = cv_2d_names[0] + " " + cv_2d_names[1];
                    if(var->rank >2) {
                        for (const auto &dim:var->dims) {

                            // Add 3rd-dimension to the 'coordinates' attribute.
                            if(cv_2d_dimnames.find(dim->name) == cv_2d_dimnames.end())
                                co_attrvalue = co_attrvalue + " " +dim->newname;
                        }
                    }
                    auto attr_unique = make_unique<Attribute>();
                    auto attr = attr_unique.release();
                    Add_Str_Attr(attr,co_attrname,co_attrvalue);
                    var->attrs.push_back(attr);
                    var->coord_attr_add_path = false;
                }
            }
        }
    }
}

// This routine is for handling "coordinates" for the GENERAL_LATLON_COOR_ATTR pattern of General_Product.
void GMFile::Handle_LatLon_With_CoordinateAttr_Coor_Attr()  {

    BESDEBUG("h5", "Coming to Handle_LatLon_With_CoordinateAttr_Coor_Attr()"<<endl);
    string co_attrname = "coordinates";

    // Loop through all rank >1 variables 
    for (auto &var:this->vars) {
        if(var->rank >= 2) {
            for (auto &attr:var->attrs) { 
                if(attr->name == co_attrname) {
                    // If having the coordinates attribute, check if the "coordinates" variables match 2-D lat/lon CV condition,
                    // if yes, flatten the coordinates attribute.
                    string coor_value = Retrieve_Str_Attr_Value(attr,var->fullpath);
                    if(Coord_Match_LatLon_NameSize(coor_value) == true) 
                        Flatten_VarPath_In_Coordinates_Attr(var);
                    // If the "coordinates" variables don't match the first condition, we can still check
                    // if we can find the corresponding "coordinates" variables that match the names under the same group,
                    // if yes, we add the path to the attribute "coordinates".
                    else if(Coord_Match_LatLon_NameSize_Same_Group(coor_value,HDF5CFUtil::obtain_string_before_lastslash(var->fullpath)) == true) 
                        Add_VarPath_In_Coordinates_Attr(var,coor_value);
                    // For other cases, we don't do anything with the "coordinates".
                    break;
                }
            }
        }
    }

}

// We will check the "coordinates variables" stored in the coordinate attribute match the
// checked latlon_name_pairs for the GENERAL_LATLON_COOR_ATTR case.
bool GMFile::Coord_Match_LatLon_NameSize(const string & coord_values)  {

    BESDEBUG("h5", "Coming to Coord_Match_LatLon_NameSize()"<<endl);
    bool ret_value =false;
    vector<string> coord_values_vec;
    char sep=' ';
    int match_lat_name_pair_index = -1;
    int match_lon_name_pair_index = -2;
    int num_match_lat = 0;
    int num_match_lon = 0;


    // Decompose the coordinates attribute into a string vector.
    HDF5CFUtil::Split_helper(coord_values_vec,coord_values,sep);

    // Some products ignore the first "/" of the coordinate path in the coordinate attribute, we will correct this 
    if((coord_values_vec[0])[0] !='/') {
        for(auto &coor_value:coord_values_vec){
            if((coor_value.find_first_of('/'))!=string::npos) {
                coor_value = '/' + coor_value;
            }
        }
    }

    //Loop through all coordinate path stored in the coordinate patch vector,
    for (const auto &coor_value:coord_values_vec){

        // Loop through all the lat/lon pairs generated in the Check_LatLon_With_Coordinate_Attr routine
        // Remember the index and number appeared for both lat and lon.
        for(auto ivs=latloncv_candidate_pairs.begin(); ivs!=latloncv_candidate_pairs.end();++ivs) {
            if(coor_value == (*ivs).name1){
                match_lat_name_pair_index = distance(latloncv_candidate_pairs.begin(),ivs);
                num_match_lat++;
            }
            else if (coor_value == (*ivs).name2) {
                match_lon_name_pair_index = distance(latloncv_candidate_pairs.begin(),ivs);
                num_match_lon++;
            }
        }
    }
    //Only when both index and the number of appearance match, we can set this be true.
    if((match_lat_name_pair_index == match_lon_name_pair_index) && (num_match_lat ==1) && (num_match_lon ==1))
        ret_value = true;

    return ret_value;
    
}

//Some products only store the coordinate name(not full path) in the attribute coordinates, as
//long as it is valid, we should add the path to these coordinates.
bool GMFile::Coord_Match_LatLon_NameSize_Same_Group(const string &coord_values,const string &var_path) {

    BESDEBUG("h5", "Coming to Coord_Match_LatLon_NameSize_Same_Group()"<<endl);
    bool ret_value =false;
    vector<string> coord_values_vec;
    char sep=' ';
    int match_lat_name_pair_index = -1;
    int match_lon_name_pair_index = -2;
    int num_match_lat = 0;
    int num_match_lon = 0;

    HDF5CFUtil::Split_helper(coord_values_vec,coord_values,sep);
 
    // Assume the 3rd-variable is also located under the same group if rank >=2
    for (const auto &coord_value:coord_values_vec){
#if 0
//cerr<<"coordinate values are "<<*irs <<endl;
#endif
        for(auto ivs=latloncv_candidate_pairs.begin(); ivs!=latloncv_candidate_pairs.end();++ivs) {
            string lat_name = HDF5CFUtil::obtain_string_after_lastslash((*ivs).name1);
            string lat_path = HDF5CFUtil::obtain_string_before_lastslash((*ivs).name1);
            string lon_name = HDF5CFUtil::obtain_string_after_lastslash((*ivs).name2);
            string lon_path = HDF5CFUtil::obtain_string_before_lastslash((*ivs).name2);
            if(coord_value == lat_name && lat_path == var_path){
                match_lat_name_pair_index = distance(latloncv_candidate_pairs.begin(),ivs);
                num_match_lat++;
            }
            else if (coord_value == lon_name && lon_path == var_path) {
                match_lon_name_pair_index = distance(latloncv_candidate_pairs.begin(),ivs);
                num_match_lon++;
            }
        }
    }

    if((match_lat_name_pair_index == match_lon_name_pair_index) && (num_match_lat ==1) && (num_match_lon ==1))
        ret_value = true;

    return ret_value;
}

// This is for the GENERAL_LATLON_COOR_ATTR pattern of General_Product.
void GMFile::Add_VarPath_In_Coordinates_Attr(Var *var, const string &coor_value) {

    BESDEBUG("h5", "Coming to Add_VarPath_In_Coordinates_Attr()"<<endl);
    string new_coor_value;
    char sep =' ';
    string var_path = HDF5CFUtil::obtain_string_before_lastslash(var->fullpath) ;
    string var_flatten_path = get_CF_string(var_path);

    // We need to loop through each element in the "coor_value".
    size_t ele_start_pos = 0;
    size_t cur_pos = coor_value.find_first_of(sep);
    while(cur_pos !=string::npos) {
        string tempstr = coor_value.substr(ele_start_pos,cur_pos-ele_start_pos);
        tempstr = var_flatten_path + tempstr;
        new_coor_value += tempstr + sep;
        ele_start_pos = cur_pos+1;
        cur_pos = coor_value.find_first_of(sep,cur_pos+1);
    }

    if(ele_start_pos == 0)
        new_coor_value = var_flatten_path + coor_value;
    else
        new_coor_value += var_flatten_path + coor_value.substr(ele_start_pos);

    string coor_attr_name = "coordinates";
    Replace_Var_Str_Attr(var,coor_attr_name,new_coor_value);
    var->coord_attr_add_path = false;

}

// Create Missing coordinate variables. Index numbers are used for these variables.
void GMFile:: Create_Missing_CV(GMCVar *GMcvar, const string& dimname)  {

    BESDEBUG("h5", "GMFile::Coming to Create_Missing_CV()"<<endl);

    GMcvar->name = dimname;
    GMcvar->newname = GMcvar->name;
    GMcvar->fullpath = GMcvar->name;
    GMcvar->rank = 1;
    GMcvar->dtype = H5INT32;
    hsize_t gmcvar_dimsize = dimname_to_dimsize[dimname];
    bool unlimited_flag = dimname_to_unlimited[dimname];

    auto gmcvar_dim_unique = make_unique<Dimension>(gmcvar_dimsize);
    auto gmcvar_dim = gmcvar_dim_unique.release();

    gmcvar_dim->unlimited_dim = unlimited_flag;
    gmcvar_dim->name = dimname;
    gmcvar_dim->newname = dimname;
    GMcvar->dims.push_back(gmcvar_dim);
    GMcvar->cfdimname = dimname;
    GMcvar->cvartype = CV_NONLATLON_MISS;
    GMcvar->product_type = product_type;
}

 // Check if this is just a netCDF-4 dimension. We need to check the dimension scale dataset attribute "NAME",
 // the value should start with "This is a netCDF dimension but not a netCDF variable".
bool GMFile::Is_netCDF_Dimension(const Var *var) const {
    
    string netcdf_dim_mark = "This is a netCDF dimension but not a netCDF variable";

    bool is_only_dimension = false;

    for(const auto &attr:var->attrs) {

        if ("NAME" == attr->name) {

             Retrieve_H5_Attr_Value(attr,var->fullpath);
             string name_value;
             name_value.resize(attr->value.size());
             copy(attr->value.begin(),attr->value.end(),name_value.begin());

             // Compare the attribute "NAME" value with the string netcdf_dim_mark. We only compare the string with the size of netcdf_dim_mark
             if (0 == name_value.compare(0,netcdf_dim_mark.size(),netcdf_dim_mark))
                is_only_dimension = true;
           
            break;
        }
    } // for(auto ira = var->attrs.begin(); ...

    return is_only_dimension;
}

// Handle attributes for special variables.
void 
GMFile::Handle_SpVar_Attr()  {

}

bool
GMFile::Is_Hybrid_EOS5() const {

    bool has_group_hdfeos  = false;
    bool has_group_hdfeos_info = false;

    // Too costly to check the dataset. 
    // We will just check the attribute under /HDFEOS INFORMATION.

    // First check if the HDFEOS groups are included
    for (const auto &grp:this->groups) {
        if ("/HDFEOS" == grp->path) 
            has_group_hdfeos = true;
        else if("/HDFEOS INFORMATION" == grp->path) {
            for(const auto &attr:grp->attrs) {
                if("HDFEOSVersion" == attr->name)
                    has_group_hdfeos_info = true;
            }
        }
        if(true == has_group_hdfeos && true == has_group_hdfeos_info)
            break;
    }

    
    if(true == has_group_hdfeos && true == has_group_hdfeos_info) 
        return true;
    else 
        return false;
}

void GMFile::Handle_Hybrid_EOS5() {

    string eos_str="HDFEOS_";
    string eos_info_str="HDFEOS_INFORMATION_";
    string grid_str="GRIDS_";
    string swath_str="SWATHS_";
    string zas_str="ZAS_";
    string df_str="Data_Fields_";
    string gf_str="Geolocation_Fields_";
    for (auto &var:this->vars) {
        string temp_var_name = var->newname;

        bool remove_eos = Remove_EOS5_Strings(temp_var_name);
        
        if(true == remove_eos)
            var->newname = get_CF_string(temp_var_name);
        else {//HDFEOS info and extra fields
            string::size_type eos_info_pos = temp_var_name.find(eos_info_str);
            if(eos_info_pos !=string::npos) 
                var->newname = temp_var_name.erase(eos_info_pos,eos_info_str.size());
            else {// Check the extra fields
                if(Remove_EOS5_Strings_NonEOS_Fields(temp_var_name)==true)
                    var->newname = get_CF_string(temp_var_name);
            }
        }
    }

    // Now we need to handle the dimension names.
    for (auto &var:this->vars) {
        for (auto &dim:var->dims) {
            string temp_dim_name = dim->newname;
            bool remove_eos = Remove_EOS5_Strings(temp_dim_name);
            
            if(true == remove_eos)
                dim->newname = get_CF_string(temp_dim_name);
            else {//HDFEOS info and extra fields
                string::size_type eos_info_pos = temp_dim_name.find(eos_info_str);
                if(eos_info_pos !=string::npos) 
                    dim->newname = temp_dim_name.erase(eos_info_pos,eos_info_str.size());
                else {// Check the extra fields
                    if(Remove_EOS5_Strings_NonEOS_Fields(temp_dim_name)==true)
                        dim->newname = get_CF_string(temp_dim_name);
                }
            }
        }
    }

    // We have to loop through all CVs.
    for (auto &cvar:this->cvars) {
        string temp_var_name = cvar->newname;

        bool remove_eos = Remove_EOS5_Strings(temp_var_name);
        
        if(true == remove_eos)
            cvar->newname = get_CF_string(temp_var_name);
        else {//HDFEOS info and extra "fields"
            string::size_type eos_info_pos = temp_var_name.find(eos_info_str);
            if(eos_info_pos !=string::npos) 
                cvar->newname = temp_var_name.erase(eos_info_pos,eos_info_str.size());
            else {// Check the extra fields
                if(Remove_EOS5_Strings_NonEOS_Fields(temp_var_name)==true)
                    cvar->newname = get_CF_string(temp_var_name);
            }
        }
    }
    // Now we need to handle the dimension names.
    for (const auto &cvar:this->cvars) {
        for (const auto &dim:cvar->dims) {
            string temp_dim_name = dim->newname;
            bool remove_eos = Remove_EOS5_Strings(temp_dim_name);
            
            if(true == remove_eos)
                dim->newname = get_CF_string(temp_dim_name);
            else {// HDFEOS info and extra "fields"
                string::size_type eos_info_pos = temp_dim_name.find(eos_info_str);
                if(eos_info_pos !=string::npos) 
                    dim->newname = temp_dim_name.erase(eos_info_pos,eos_info_str.size());
                else {// Check the extra "fields"
                    if(Remove_EOS5_Strings_NonEOS_Fields(temp_dim_name)==true)
                        dim->newname = get_CF_string(temp_dim_name);
                }
            }
        }
    }

    // Update the coordinate attribute values
    // We need to remove the HDFEOS special information from the coordinates attributes 
    // since the variable names in the DAP output are already updated.
    for (const auto &var:this->vars) {
        for (const auto &attr:var->attrs) {
            // We cannot use Retrieve_Str_Attr_value for "coordinates" since "coordinates" may be added by the handler.
            // KY 2017-11-3
            if(attr->name == "coordinates") {
                string cor_values(attr->value.begin(),attr->value.end()) ;
                bool change_cor_values = false;
                // Find the HDFEOS string
                if(cor_values.find(eos_str)==0) {
                    if(cor_values.find(grid_str)!=string::npos) {// Grid
                        cor_values = HDF5CFUtil::remove_substrings(cor_values,eos_str);
                        cor_values = HDF5CFUtil::remove_substrings(cor_values,grid_str);
                        string new_cor_values = HDF5CFUtil::remove_substrings(cor_values,df_str);
                        if(new_cor_values.size() < cor_values.size()){//df_str is also removed.
                            change_cor_values = true;
                            cor_values = new_cor_values;
                        }
                    }
                    else if(cor_values.find(zas_str)!=string::npos) {//ZA 
                        cor_values = HDF5CFUtil::remove_substrings(cor_values,eos_str);
                        cor_values = HDF5CFUtil::remove_substrings(cor_values,zas_str);
                        string new_cor_values = HDF5CFUtil::remove_substrings(cor_values,df_str);
                        if(new_cor_values.size() < cor_values.size()){//df_str is also removed.
                            change_cor_values = true;
                            cor_values = new_cor_values;
                        }
                    }
                    else if(cor_values.find(swath_str)!=string::npos) {//Swath 
                        cor_values = HDF5CFUtil::remove_substrings(cor_values,eos_str);
                        cor_values = HDF5CFUtil::remove_substrings(cor_values,swath_str);
                        string new_cor_values = HDF5CFUtil::remove_substrings(cor_values,df_str);
                        if(new_cor_values.size() < cor_values.size()){//df_str is also removed.
                            change_cor_values = true;
                            cor_values = new_cor_values;
                        }
                        else {
                            new_cor_values = HDF5CFUtil::remove_substrings(cor_values,gf_str);
                            if(new_cor_values.size() < cor_values.size()){//gf_str is also removed.
                                change_cor_values = true;
                                cor_values = new_cor_values;
                            }
                        }
                    }
                }
                if(true == change_cor_values) {//Update the coordinate values
                    attr->value.resize(cor_values.size());
                    attr->fstrsize=cor_values.size();
                    attr->strsize[0] = cor_values.size();
                    copy(cor_values.begin(), cor_values.end(), attr->value.begin());
                    var->coord_attr_add_path = false;
                }
                
                break;
            }
        }
    }
}

// This routine is for handling the hybrid-HDFEOS5 products that have to be treated as "general products"
bool GMFile:: Remove_EOS5_Strings(string &var_name) const {

    string eos_str="HDFEOS_";
    string grid_str="GRIDS_";
    string swath_str="SWATHS_";
    string zas_str="ZAS_";
    string df_str="Data_Fields_";
    string gf_str="Geolocation_Fields_";
    string temp_var_name = var_name;

    bool remove_eos = false;

    string::size_type eos_pos = temp_var_name.find(eos_str);
    if(eos_pos!=string::npos) {
        temp_var_name.erase(eos_pos,eos_str.size());
        // Check grid,swath and zonal 
        string::size_type grid_pos=temp_var_name.find(grid_str);
        string::size_type grid_df_pos=string::npos;
        if(grid_pos!=string::npos)
            grid_df_pos = temp_var_name.find(df_str,grid_pos);
        string::size_type zas_pos = string::npos;
        string::size_type zas_df_pos=string::npos;
        if(grid_pos==string::npos || grid_df_pos ==string::npos) 
            zas_pos=temp_var_name.find(zas_str);
        if(zas_pos!=string::npos)
            zas_df_pos=temp_var_name.find(df_str,zas_pos);
        
        if(grid_pos !=string::npos && grid_df_pos!=string::npos) {
            temp_var_name.erase(grid_pos,grid_str.size());
            grid_df_pos = temp_var_name.find(df_str);              
            temp_var_name.erase(grid_df_pos,df_str.size());
            remove_eos = true;
        }
        else if(zas_pos!=string::npos && zas_df_pos!=string::npos){
            temp_var_name.erase(zas_pos,zas_str.size());
            zas_df_pos = temp_var_name.find(df_str);              
            temp_var_name.erase(zas_df_pos,df_str.size());
            remove_eos = true;
        }
        else {//Check both Geolocation and Data for Swath
            
            string::size_type swath_pos=temp_var_name.find(swath_str);
            string::size_type swath_df_pos=string::npos;
            if(swath_pos!=string::npos)
                swath_df_pos=temp_var_name.find(df_str,swath_pos);

            string::size_type swath_gf_pos=string::npos;
            if(swath_pos!=string::npos && swath_df_pos == string::npos)
                swath_gf_pos=temp_var_name.find(gf_str,swath_pos);

            if(swath_pos !=string::npos) {

                if(swath_df_pos!=string::npos) {
                    temp_var_name.erase(swath_pos,swath_str.size());
                    swath_df_pos = temp_var_name.find(df_str);              
                    temp_var_name.erase(swath_df_pos,df_str.size());
                    remove_eos = true;
                }
                else if(swath_gf_pos!=string::npos) {
                    temp_var_name.erase(swath_pos,swath_str.size());
                    swath_gf_pos = temp_var_name.find(gf_str);              
                    temp_var_name.erase(swath_gf_pos,gf_str.size());
                    remove_eos = true;
                }
            }
        }
    }
    if(true == remove_eos)
        var_name = temp_var_name;
        
    return remove_eos;
}

bool GMFile:: Remove_EOS5_Strings_NonEOS_Fields(string &var_name) const{

    string eos_str="HDFEOS_";
    string grid_str="GRIDS_";
    string swath_str="SWATHS_";
    string zas_str="ZAS_";
    string temp_var_name = var_name;

    bool remove_eos = false;

    string::size_type eos_pos = temp_var_name.find(eos_str);
    if(eos_pos!=string::npos) {
        temp_var_name.erase(eos_pos,eos_str.size());
        remove_eos = true;

        // See if we need to further remove some fields 
        if(temp_var_name.find(grid_str)==0)
            temp_var_name.erase(0,grid_str.size());
        else if(temp_var_name.find(swath_str)==0)
            temp_var_name.erase(0,swath_str.size());
        else if(temp_var_name.find(zas_str)==0)
            temp_var_name.erase(0,zas_str.size());
    }
    if(true == remove_eos)
        var_name = temp_var_name;

        
    return remove_eos;
}

// We do have an AirMSPI HDF-EOS5 hybrid UTM product that has grid_mapping attribute.
bool GMFile:: Have_Grid_Mapping_Attrs(){
    return File::Have_Grid_Mapping_Attrs();
}

void GMFile:: Handle_Grid_Mapping_Vars(){
    File:: Handle_Grid_Mapping_Vars();
}

void GMFile:: Remove_Unused_FakeDimVars() {

    // We need to remove the FakeDim added for the unsupported variables.
    // We found such a case in the AirMSPR product. A compound dataype array
    // is assigned to a FakeDim. We need to remove them.
    // KY 2017-11-2: no need to even check the unsupported_var_dspace now. 
    if(this->unsupported_var_dtype == true) {

        // Need to check if we have coordinate variables such as FakeDim?
        for (auto ircv = this->cvars.begin(); ircv != this->cvars.end();) {
            if((*ircv)->newname.find("FakeDim")==0) {
                bool var_has_fakedim = false;
                for (const auto &var:this->vars) {
                    for (const auto &dim:var->dims) {
                        if(dim->newname == (*ircv)->newname){
                            var_has_fakedim = true;
                            break;
                        }
                    }
                    if(var_has_fakedim == true) 
                        break;
                }
                if(var_has_fakedim == false) {
                    // Remove this cv, the variable is unsupported.
                    delete(*ircv);
                    ircv = this->cvars.erase(ircv);
                }
                else 
                    ++ircv;
            }
            else 
                ++ircv;
        }
#if 0
        // We need to handle unlimited dimensions
        //if(removed_fakedim_vars.size()!=0) {
        //}
#endif
    }

}

//Rename NC4 NonCoordVars back to the original name. This is detected by CAR_ARCTAS files.
//By handling this way, the output will be the same as the netCDF handler output.
//Check HFVHANDLER-254 for more information.
void GMFile::Rename_NC4_NonCoordVars() const {

    if(true == this->have_nc4_non_coord) {
        string nc4_non_coord="_nc4_non_coord_";
        size_t nc4_non_coord_size= nc4_non_coord.size();
        for (const auto &var:this->vars) {
            if(var->name.find(nc4_non_coord)==0) {
                size_t rm_str_pos = var->newname.find(nc4_non_coord);
                if (rm_str_pos == string::npos) 
                    throw4(var->name," variable's new name ",var->newname, "doesn't contain nc4_non_coord");
                else {
                    string var_new_first_part;
                    if (rm_str_pos !=0) 
                        var_new_first_part = var->newname.substr(0,rm_str_pos);
                    string var_new_second_part = var->newname.substr(rm_str_pos+nc4_non_coord_size);
                    var->newname = var_new_first_part + var_new_second_part;
                }
            }
        }
 
        // Coordinate variables should never contain "_nc4_non_coord_"
#if 0
        for (auto ircv = this->cvars.begin(); ircv != this->cvars.end();++ircv) {
            if((*ircv)->name.find(nc4_non_coord)==0)
                (*ircv)->newname = (*ircv)->newname.substr(nc4_non_coord_size,(*ircv)->newname.size()-nc4_non_coord_size);
        }
#endif
    }
 
}

void GMFile::Add_Path_Coord_Attr() {

    BESDEBUG("h5", "GMFile::Coming to Add_Path_Coor_Attr()"<<endl);
    string co_attrname = "coordinates";
    for (const auto &var:this->vars) {
        if(var->coord_attr_add_path == true) {
            for (const auto &attr:var->attrs) {
                // We will check if we have the coordinate attribute 
                if(attr->name == co_attrname) {
                    string coor_value = Retrieve_Str_Attr_Value(attr,var->fullpath);
                    char sep=' ';
                    vector<string>cvalue_vec;
                    HDF5CFUtil::Split_helper(cvalue_vec,coor_value,sep);
                    string new_coor_value;
                    for (int i = 0; i<cvalue_vec.size();i++) {
                        HDF5CFUtil::cha_co(cvalue_vec[i],var->fullpath);
                        cvalue_vec[i] = get_CF_string(cvalue_vec[i]);
                        if(i == 0) 
                            new_coor_value = cvalue_vec[i];
                        else 
                            new_coor_value += sep+cvalue_vec[i];
                        
#if 0
//cout<<"co1["<<i<<"]= "<<cvalue_vec[i]<<endl;
#endif
                    }
#if 0
//cout<<"new_coor_value is "<<new_coor_value<<endl;
#endif
                        Replace_Var_Str_Attr(var,co_attrname,new_coor_value);
                        break;
                }
            }
        }
    }
}

void GMFile::Update_Bounds_Attr() {

    BESDEBUG("h5", "GMFile::Coming to Add_Path_Coor_Attr()"<<endl);
    for (auto &var:this->vars) {
        for (auto &attr:var->attrs) {
            if (attr->name == "bounds") {
                string bnd_values = Retrieve_Str_Attr_Value(attr,var->fullpath);
                HDF5CFUtil::cha_co(bnd_values,var->fullpath);
                bnd_values = get_CF_string(bnd_values);
                Replace_Var_Str_Attr(var,"bounds",bnd_values);
                break;
            }
        }
    }

    for (auto &var:this->cvars) {
        for (auto &attr:var->attrs) {
            if (attr->name == "bounds") {
                string bnd_values = Retrieve_Str_Attr_Value(attr,var->fullpath);
                HDF5CFUtil::cha_co(bnd_values,var->fullpath);
                bnd_values = get_CF_string(bnd_values);
                Replace_Var_Str_Attr(var,"bounds",bnd_values);
                break;
            }
        }
    }


}

// We will create some temporary coordinate variables. The resource allocated
// for these variables need to be released.
void 
GMFile::release_standalone_GMCVar_vector(vector<GMCVar*>&tempgc_vars){

    for (auto i = tempgc_vars.begin(); i != tempgc_vars.end(); ) {
        delete(*i);
        i = tempgc_vars.erase(i);
    }

}

#if 0
void 
GMFile::add_ignored_info_attrs(bool is_grp,bool is_first){

}
void 
GMFile::add_ignored_info_objs(bool is_dim_related, bool is_first) {

}
#endif

#if 0
bool
GMFile::ignored_dimscale_ref_list(Var *var) {

    bool ignored_dimscale = true;
    if(General_Product == this->product_type && GENERAL_DIMSCALE== this->gproduct_pattern) {

        bool has_dimscale = false;
        bool has_reference_list = false;

        for(auto ira = var->attrs.begin(); ira != var->attrs.end();ira++) {

             if((*ira)->name == "REFERENCE_LIST" && 
                false == HDF5CFUtil::cf_strict_support_type((*ira)->getType()))
                has_reference_list = true;
             if((*ira)->name == "CLASS") {
                Retrieve_H5_Attr_Value(*ira,var->fullpath);
                string class_value;
                class_value.resize((*ira)->value.size());
                copy((*ira)->value.begin(),(*ira)->value.end(),class_value.begin());

                // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
                // "DIMENSION_SCALE", which is 15.
                if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                    has_dimscale = true;
                }
            }
 
            if(true == has_dimscale && true == has_reference_list) {
                ignored_dimscale= false;
                break;
            }
           
        }
    }
    return ignored_dimscale;
}
    
#endif
