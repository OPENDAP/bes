// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5GMCF.cc
/// \brief Implementation of the mapping of NASA generic HDF5 products to DAP by following CF
///
///  It includes functions to 
///  1) retrieve HDF5 metadata info.
///  2) translate HDF5 objects into DAP DDS and DAS by following CF conventions.
///
///
/// \author Muqun Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.

#include "HDF5CF.h"
#include "HDF5RequestHandler.h"
#include "h5apicompatible.h"
#include <BESDebug.h>
#include <algorithm>

using namespace std;
using namespace libdap;
using namespace HDF5CF;

// Copier function.
GMCVar::GMCVar(Var*var) {

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
    
    for (vector<Attribute*>::iterator ira = var->attrs.begin();
        ira!=var->attrs.end(); ++ira) {
        Attribute* attr= new Attribute();
        attr->name = (*ira)->name;
        attr->newname = (*ira)->newname;
        attr->dtype =(*ira)->dtype;
        attr->count =(*ira)->count;
        attr->strsize = (*ira)->strsize;
        attr->fstrsize = (*ira)->fstrsize;
        attr->value =(*ira)->value;
        attrs.push_back(attr);
    } //for (vector<Attribute*>::iterator ira = var->attrs.begin()

    for (vector<Dimension*>::iterator ird = var->dims.begin();
        ird!=var->dims.end(); ++ird) {
        Dimension *dim = new Dimension((*ird)->size);
        dim->name = (*ird)->name;
        dim->newname = (*ird)->newname;
        dim->unlimited_dim = (*ird)->unlimited_dim;
        dims.push_back(dim);
    } // for (vector<Dimension*>::iterator ird = var->dims.begin()
    product_type = General_Product;

}
#if 0
GMCVar::GMCVar(GMCVar*cvar) {

    newname = cvar->newname;
    name = cvar->name;
    fullpath = cvar->fullpath;
    rank  = cvar->rank;
    dtype = cvar->dtype;
    unsupported_attr_dtype = cvar->unsupported_attr_dtype;
    unsupported_dspace = cvar->unsupported_dspace;
    
    for (vector<Attribute*>::iterator ira = cvar->attrs.begin();
        ira!=cvar->attrs.end(); ++ira) {
        Attribute* attr= new Attribute();
        attr->name = (*ira)->name;
        attr->newname = (*ira)->newname;
        attr->dtype =(*ira)->dtype;
        attr->count =(*ira)->count;
        attr->strsize = (*ira)->strsize;
        attr->fstrsize = (*ira)->fstrsize;
        attr->value =(*ira)->value;
        attrs.push_back(attr);
    }

    for (vector<Dimension*>::iterator ird = cvar->dims.begin();
        ird!=cvar->dims.end(); ++ird) {
        Dimension *dim = new Dimension((*ird)->size);
//"h5","dim->name "<< (*ird)->name <<endl;
//"h5","dim->newname "<< (*ird)->newname <<endl;
        dim->name = (*ird)->name;
        dim->newname = (*ird)->newname;
        dims.push_back(dim);
    }

    GMcvar->cfdimname = latdim0;
    GMcvar->cvartype = CV_EXIST;
                GMcvar->product_type = product_type;

 
}
#endif

//Copier function of a special variable.
GMSPVar::GMSPVar(Var*var) {

    BESDEBUG("h5", "Coming to GMSPVar()"<<endl);
    fullpath = var->fullpath;
    rank  = var->rank;
    total_elems = var->total_elems;
    zero_storage_size = var->zero_storage_size;
    unsupported_attr_dtype = var->unsupported_attr_dtype;
    unsupported_dspace = var->unsupported_dspace;

    // The caller of this function should change the following fields.
    // This is just to make data coverity happy.
    otype = H5UNSUPTYPE;
    sdbit = -1;
    numofdbits = -1;

    for (vector<Attribute*>::iterator ira = var->attrs.begin();
        ira!=var->attrs.end(); ++ira) {
        Attribute* attr= new Attribute();
        attr->name = (*ira)->name;
        attr->newname = (*ira)->newname;
        attr->dtype =(*ira)->dtype;
        attr->count =(*ira)->count;
        attr->strsize = (*ira)->strsize;
        attr->fstrsize = (*ira)->fstrsize;
        attr->value =(*ira)->value;
        attrs.push_back(attr);
    } // "for (vector<Attribute*>::iterator ira = var->attrs.begin()"

    for (vector<Dimension*>::iterator ird = var->dims.begin();
        ird!=var->dims.end(); ++ird) {
        Dimension *dim = new Dimension((*ird)->size);
        dim->name = (*ird)->name;
        dim->newname = (*ird)->newname;
        dim->unlimited_dim = (*ird)->unlimited_dim;
        dims.push_back(dim);
    } 
}


GMFile::GMFile(const char*file_fullpath, hid_t file_id, H5GCFProduct product_type, GMPattern gproduct_pattern):
File(file_fullpath,file_id), product_type(product_type),gproduct_pattern(gproduct_pattern),iscoard(false),have_nc4_non_coord(false)
{

}

// destructor
GMFile::~GMFile() 
{

    if (!this->cvars.empty()){
        for (vector<GMCVar *>:: const_iterator i= this->cvars.begin(); i!=this->cvars.end(); ++i) {
           delete *i;
        }
    }

    if (!this->spvars.empty()){
        for (vector<GMSPVar *>:: const_iterator i= this->spvars.begin(); i!=this->spvars.end(); ++i) {
           delete *i;
        }
    }

}

// Get CF string
string GMFile::get_CF_string(string s) {

    // HDF5 group or variable path always starts with '/'. When CF naming rule is applied,
    // the first '/' is always changes to "_", this is not necessary. However,
    // to keep the backward compatiablity, I use a BES key for people to go back with the original name.

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
// Currently GPM level 3 is changed.
// This routine should be called right after Retrieve_H5_Info.
void GMFile::Update_Product_Type()  {

    BESDEBUG("h5", "Coming to Update_Product_Type()"<<endl);
    if(GPMS_L3 == this->product_type || GPMM_L3 == this->product_type) {

        // Check Dimscale attributes 
        Check_General_Product_Pattern();
        if(GENERAL_DIMSCALE == this->gproduct_pattern){
            if(GPMS_L3 == this->product_type) {
                for (vector<Var *>::iterator irv = this->vars.begin();
                    irv != this->vars.end(); ++irv) 
                        (*irv)->newname = (*irv)->name;
            }
            this->product_type = General_Product;
        }
    }
//#if 0
    else if(General_Product == this->product_type)
        Check_General_Product_Pattern();
//#endif
}

void GMFile::Remove_Unneeded_Objects()  {

    BESDEBUG("h5", "Coming to Remove_Unneeded_Objects()"<<endl);
    if(General_Product == this->product_type) {
        string file_path = this->path;
        if(HDF5CFUtil::obtain_string_after_lastslash(file_path).find("OMPS-NPP")==0) 
            Remove_OMPSNPP_InputPointers();
    }
    if((General_Product == this->product_type) && (GENERAL_DIMSCALE == this->gproduct_pattern)) {
        set<string> nc4_non_coord_set;
        string nc4_non_coord="_nc4_non_coord_";
        size_t nc4_non_coord_size= nc4_non_coord.size();
        for (vector<Var *>::iterator irv = this->vars.begin();
                                    irv != this->vars.end(); ++irv) {
            if((*irv)->name.find(nc4_non_coord)==0) 
                nc4_non_coord_set.insert((*irv)->name.substr(nc4_non_coord_size,(*irv)->name.size()-nc4_non_coord_size));
            
        }

        for (vector<Var *>::iterator irv = this->vars.begin();
                                    irv != this->vars.end();) {
            if(nc4_non_coord_set.find((*irv)->name)!=nc4_non_coord_set.end()){
                delete(*irv);
                irv=this->vars.erase(irv);
            }
            else 
                ++irv;
        }

        if(nc4_non_coord_set.size()!=0)
            this->have_nc4_non_coord = true;
    }
}

void GMFile::Remove_OMPSNPP_InputPointers()  {
    // Here I don't check whether this is a netCDF file by 
    // using Check_Dimscale_General_Product_Pattern() to see if it returns true.
    // We will see if we need this.
    for (vector<Group *>::iterator irg = this->groups.begin();
        irg != this->groups.end(); ) {
        if((*irg)->path.find("/InputPointers")==0) {
            delete(*irg);
            irg = this->groups.erase(irg);

        }
        else 
            ++irg;
    }

    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ) {
         if((*irv)->fullpath.find("/InputPointers")==0) {
            delete(*irv);
            irv = this->vars.erase(irv);

        }
        else 
            ++irv;
    }
}
void GMFile::Retrieve_H5_CVar_Supported_Attr_Values() {

    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
          ircv != this->cvars.end(); ++ircv) {
          
        if ((*ircv)->cvartype != CV_NONLATLON_MISS){
            for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                 ira != (*ircv)->attrs.end(); ++ira) {
                Retrieve_H5_Attr_Value(*ira,(*ircv)->fullpath);
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
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
          ircv != this->cvars.end(); ++ircv) {
          
        if ((*ircv)->cvartype != CV_NONLATLON_MISS){
            for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                 ira != (*ircv)->attrs.end(); ++ira) {
                Retrieve_H5_Attr_Value(*ira,(*ircv)->fullpath);
            }
        }
    }

    // Special variable attributes
    for (vector<GMSPVar *>::iterator irspv = this->spvars.begin();
          irspv != this->spvars.end(); ++irspv) {
          
        for (vector<Attribute *>::iterator ira = (*irspv)->attrs.begin();
              ira != (*irspv)->attrs.end(); ++ira) {
            Retrieve_H5_Attr_Value(*ira,(*irspv)->fullpath);
            Adjust_H5_Attr_Value(*ira);
        }
    }
}

// Adjust attribute values. Currently this is only for ACOS and OCO2.
// Reason: DAP2 doesn't support 64-bit integer and they have 64-bit integer data
// in these files. Chop them to two 32-bit integers following the data producer's information.
void GMFile::Adjust_H5_Attr_Value(Attribute *attr)  {

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
    } // "end if (product_type == ACOS_L2S_OR_OCO2_L1B)"
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
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
                ircv != this->cvars.end(); ) {
        if (true == include_attr) {
            for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                 ira != (*ircv)->attrs.end(); ) {
                H5DataType temp_dtype = (*ira)->getType();
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                    delete (*ira);
                    ira = (*ircv)->attrs.erase(ira);
                }
                else {
                    ++ira;
                }
            }
        }
        H5DataType temp_dtype = (*ircv)->getType();
        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
            
            // This may need to be checked carefully in the future,
            // My current understanding is that the coordinate variable can
            // be ignored if the corresponding variable is ignored. 
            // Currently we don't find any NASA files in this category.
            // KY 2012-5-21
            delete (*ircv);
            ircv = this->cvars.erase(ircv);
        }
        else {
            ++ircv;
        }
       
    } // for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
    for (vector<GMSPVar *>::iterator ircv = this->spvars.begin();
                ircv != this->spvars.end(); ) {

        if (true == include_attr) {
            for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                ira != (*ircv)->attrs.end(); ) {
                H5DataType temp_dtype = (*ira)->getType();
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                    delete (*ira);
                    ira = (*ircv)->attrs.erase(ira);
                }
                else {
                    ++ira;
                }
            }
        }
        H5DataType temp_dtype = (*ircv)->getType();
        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
            delete (*ircv);
            ircv = this->spvars.erase(ircv);
        }
        else {
            ++ircv;
        }
            
    }// "end for (vector<GMSPVar *>::iterator ircv = this->spvars.begin()"
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

// Datatype ignored information for variable ttributes
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

        for (vector<GMCVar *>::iterator irv = this->cvars.begin();
            irv != this->cvars.end(); ++irv) {
            // If the attribute REFERENCE_LIST comes with the attribut CLASS, the
            // attribute REFERENCE_LIST is okay to ignore. No need to report.
            bool is_ignored = ignored_dimscale_ref_list((*irv));
            if (false == (*irv)->attrs.empty()) {
                    for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                        ira != (*irv)->attrs.end(); ++ira) {
                        H5DataType temp_dtype = (*ira)->getType();
                        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype) || temp_dtype == H5INT64 || temp_dtype == H5UINT64) {
                            // "DIMENSION_LIST" is okay to ignore and "REFERENCE_LIST"
                            // is okay to ignore if the variable has another attribute
                            // CLASS="DIMENSION_SCALE"
                            if (("DIMENSION_LIST" !=(*ira)->name) &&
                                ("REFERENCE_LIST" != (*ira)->name || true == is_ignored))
                                this->add_ignored_info_attrs(false,(*irv)->fullpath,(*ira)->name);
                        }
                    }
            } // end "if (false == (*irv)->attrs.empty())" 
        }// end "for(vector<GMCVar*>"    

        for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
            irv != this->spvars.end(); ++irv) {
            // If the attribute REFERENCE_LIST comes with the attribut CLASS, the
            // attribute REFERENCE_LIST is okay to ignore. No need to report.
            bool is_ignored = ignored_dimscale_ref_list((*irv));
            if (false == (*irv)->attrs.empty()) {
                //if (true == (*irv)->unsupported_attr_dtype) {
                    for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                        ira != (*irv)->attrs.end(); ++ira) {
                        H5DataType temp_dtype = (*ira)->getType();
                        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype) || temp_dtype == H5INT64 || temp_dtype == H5UINT64) {
                            // "DIMENSION_LIST" is okay to ignore and "REFERENCE_LIST"
                            // is okay to ignore if the variable has another attribute
                            // CLASS="DIMENSION_SCALE"
                            if (("DIMENSION_LIST" !=(*ira)->name) &&
                                ("REFERENCE_LIST" != (*ira)->name || true == is_ignored))
                                this->add_ignored_info_attrs(false,(*irv)->fullpath,(*ira)->name);
                        }
                    }
            } // "if (false == (*irv)->attrs.empty())"  
        }// "for(vector<GMSPVar*>"    
    }// "if((General_Product == ......)"
    else {
 
        for (vector<GMCVar *>::iterator irv = this->cvars.begin();
            irv != this->cvars.end(); ++irv) {
            if (false == (*irv)->attrs.empty()) {
                //if (true == (*irv)->unsupported_attr_dtype) {
                    for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                        ira != (*irv)->attrs.end(); ++ira) {
                        H5DataType temp_dtype = (*ira)->getType();
                        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype) || temp_dtype == H5INT64 || temp_dtype == H5UINT64) {
                                this->add_ignored_info_attrs(false,(*irv)->fullpath,(*ira)->name);
                        }
                    }
                //}
            }
        }// for (vector<GMCVar *>::iterator irv = this->cvars.begin() STOP adding end logic comments

        for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
            irv != this->spvars.end(); ++irv) {
            if (false == (*irv)->attrs.empty()) {
                //if (true == (*irv)->unsupported_attr_dtype) {
                    for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                        ira != (*irv)->attrs.end(); ++ira) {
                        H5DataType temp_dtype = (*ira)->getType();
                        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype) || temp_dtype == H5INT64 || temp_dtype == H5UINT64) {
                            this->add_ignored_info_attrs(false,(*irv)->fullpath,(*ira)->name);
                        }
                    }
                //}
            }
        }// for(vector<GMSPVar *> 

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
        for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
                ircv != this->cvars.end(); ) {
            if (true  == (*ircv)->unsupported_dspace ) {
            
                // This may need to be checked carefully in the future,
                // My current understanding is that the coordinate variable can
                // be ignored if the corresponding variable is ignored. 
                // Currently we don't find any NASA files in this category.
                // KY 2012-5-21
                delete (*ircv);
                ircv = this->cvars.erase(ircv);
            }
            else {
                ++ircv;
            }
        } // "for (vector<GMCVar *>::iterator ircv = this->cvars.begin();"

        for (vector<GMSPVar *>::iterator ircv = this->spvars.begin();
                ircv != this->spvars.end(); ) {

            if (true == (*ircv)->unsupported_dspace) {
                delete (*ircv);
                ircv = this->spvars.erase(ircv);
            }
            else {
                ++ircv;
            }
            
        }// for (vector<GMSPVar *>::iterator ircv = this->spvars.begin();
    }// if(true == this->unsupported_dspace) 

    if(true == include_attr) {
        if(true == this->unsupported_var_attr_dspace) {
            for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
                ircv != this->cvars.end(); ++ircv) {
                if (false == (*ircv)->attrs.empty()) {
                    if (true == (*ircv)->unsupported_attr_dspace) {
                        for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                            ira != (*ircv)->attrs.end(); ) {
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

            for (vector<GMSPVar *>::iterator ircv = this->spvars.begin();
                ircv != this->spvars.end(); ++ircv) {
                if (false == (*ircv)->attrs.empty()) {
                    if (true == (*ircv)->unsupported_attr_dspace) {
                        for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                            ira != (*ircv)->attrs.end(); ) {
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
        }// if(true == this->unsupported_var_attr_dspace)
    }// if(true == include_attr)

}

// Generate unsupported data space information
void GMFile:: Gen_Unsupported_Dspace_Info() {

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
            for (vector<Attribute *>::iterator ira = this->root_attrs.begin(); ira != this->root_attrs.end();)  {

                if((*ira)->name == "_nc3_strict") {
                    delete(*ira);
                    ira =this->root_attrs.erase(ira);
                    //If we have other root attributes to remove, remove the break statement.
                }
                else if((*ira)->name == "_NCProperties") {
                    delete(*ira);
                    ira =this->root_attrs.erase(ira);
                }
                else {
                    ++ira;
                }
            }
            for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
                for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                     ira != (*irv)->attrs.end();) {
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
                        ira =(*irv)->attrs.erase(ira);
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
                                delete((*ira));
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
#if 0
                    else if((*ira)->name == "_nc3_strict") {
                        delete((*ira));
                        ira =(*irv)->attrs.erase(ira);
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
    } // switch(product_type)

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
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone((*irv));
        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end();++ird) { 
            setret = dimnamelist.insert((*ird)->name);
            if (true == setret.second) 
                Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size,(*ird)->unlimited_dim);
        }
    } // for (vector<Var *>::iterator irv = this->vars.begin();
 
    if (true == dimnamelist.empty()) 
        throw1("This product should have the dimension names, but no dimension names are found");
}    

// Handle Dimension scales for MEasUREs SeaWiFS and OZone.
void GMFile::Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(Var* var)
{

    BESDEBUG("h5", "Coming to Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone()"<<endl);
    Attribute* dimlistattr = NULL;
    bool has_dimlist = false;
    bool has_class = false;
    bool has_reflist = false;

    for(vector<Attribute *>::iterator ira = var->attrs.begin();
          ira != var->attrs.end();ira++) {
        if ("DIMENSION_LIST" == (*ira)->name) {
           dimlistattr = *ira;
           has_dimlist = true;  
        }
        if ("CLASS" == (*ira)->name) 
            has_class = true;
        if ("REFERENCE_LIST" == (*ira)->name) 
            has_reflist = true;
        
        if (true == has_dimlist) 
            break;
        if (true == has_class && true == has_reflist) 
            break; 
    } // for(vector<Attribute *>::iterator ira = var->attrs.begin(); ...

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
    // For Fake dimesnion
    else {

        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        for (vector<Dimension *>::iterator ird= var->dims.begin();
            ird != var->dims.end(); ++ird) {
                Add_One_FakeDim_Name(*ird);
                setsizeret = fakedimsize.insert((*ird)->size);
                if (false == setsizeret.second)   
                    Adjust_Duplicate_FakeDim_Name(*ird);
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
void GMFile::Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(Var *var,Attribute*dimlistattr) 
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


    if(NULL == dimlistattr) 
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


        if (H5Aread(attr_id,amemtype_id,&vlbuf[0]) <0)  
            throw2("Cannot obtain the referenced object for the variable ",var->name);
        

        vector<char> objname;
        int vlbuf_index = 0;

        // The dimension names of variables will be the HDF5 dataset names dereferenced from the DIMENSION_LIST attribute.
        for (vector<Dimension *>::iterator ird = var->dims.begin();
                ird != var->dims.end(); ++ird) {

            if(vlbuf[vlbuf_index].p== NULL) 
                throw4("The dimension doesn't exist. Var name is ",var->name,"; the dimension index is ",vlbuf_index);
            rbuf =((hobj_ref_t*)vlbuf[vlbuf_index].p)[0];
            if ((ref_dset = H5RDEREFERENCE(attr_id, H5R_OBJECT, &rbuf)) < 0) 
                throw2("Cannot dereference from the DIMENSION_LIST attribute  for the variable ",var->name);

            if ((objnamelen= H5Iget_name(ref_dset,NULL,0))<=0) 
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);
            objname.resize(objnamelen+1);
            if ((objnamelen= H5Iget_name(ref_dset,&objname[0],objnamelen+1))<=0) 
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);

            string objname_str = string(objname.begin(),objname.end());
            string trim_objname = objname_str.substr(0,objnamelen);
            (*ird)->name = string(trim_objname.begin(),trim_objname.end());

            pair<set<string>::iterator,bool> setret;
            setret = dimnamelist.insert((*ird)->name);
            if (true == setret.second) 
               Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size,(*ird)->unlimited_dim);
            (*ird)->newname = (*ird)->name;
            H5Dclose(ref_dset);
            ref_dset = -1;
            objname.clear();
            vlbuf_index++;
        }// for (vector<Dimension *>::iterator ird = var->dims.begin()

        if(vlbuf.size()!= 0) {

            if ((aspace_id = H5Aget_space(attr_id)) < 0)
                throw2("Cannot get hdf5 dataspace id for the attribute ",dimlistattr->name);

            if (H5Dvlen_reclaim(amemtype_id,aspace_id,H5P_DEFAULT,(void*)&vlbuf[0])<0) 
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

    for (vector<Group *>::iterator irg = this->groups.begin();
        irg != this->groups.end(); ++ irg) {
        if ("/Dimensions" == (*irg)->path) {
            use_dimscale = true;
            break;
        }
    }

    if (false == use_dimscale) {

        bool has_dimlist = false;
        bool has_class = false;
        bool has_reflist = false;

        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); irv++) {

            for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                ira != (*irv)->attrs.end();ira++) {
                if ("DIMENSION_LIST" == (*ira)->name) 
                    has_dimlist = true;  
            }
            if (true == has_dimlist) 
                break;
        }

        if (true == has_dimlist) {
            for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); irv++) {

                for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end();ira++) {
                    if ("CLASS" == (*ira)->name) 
                        has_class = true;
                    if ("REFERENCE_LIST" == (*ira)->name) 
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
        } // if (true == has_dimlist)
    } // if (false == use_dimscale)

    if (true == use_dimscale) {

        pair<set<string>::iterator,bool> setret;
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone((*irv));
            for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird !=(*irv)->dims.end();++ird) { 
                setret = dimnamelist.insert((*ird)->name);
                if(true == setret.second) 
                    Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size,(*ird)->unlimited_dim);
            }
        }
 
        if (true == dimnamelist.empty()) 
            throw1("This product should have the dimension names, but no dimension names are found");
    } // if (true == use_dimscale)    

    else {

        // Since the dim. size of each dimension of 2D lat/lon may be the same, so use multimap.
        multimap<hsize_t,string> ozonedimsize_to_dimname;
        pair<multimap<hsize_t,string>::iterator,multimap<hsize_t,string>::iterator> mm_er_ret;
        multimap<hsize_t,string>::iterator irmm;
 
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            bool is_cv = check_cv((*irv)->name);
            if (true == is_cv) {
                if ((*irv)->dims.size() != 1)
                    throw3("The coordinate variable", (*irv)->name," must be one dimension for the zonal average product");
                ozonedimsize_to_dimname.insert(pair<hsize_t,string>(((*irv)->dims)[0]->size,(*irv)->fullpath));
            }
        }// for (vector<Var *>::iterator irv = this->vars.begin(); ...

        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        pair<set<string>::iterator,bool> setret;
        pair<set<string>::iterator,bool> tempsetret;
        set<string> tempdimnamelist;
        bool fakedimflag = false;

        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

            for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird != (*irv)->dims.end(); ++ird) {

                fakedimflag = true;
                mm_er_ret = ozonedimsize_to_dimname.equal_range((*ird)->size);
                for (irmm = mm_er_ret.first; irmm!=mm_er_ret.second;irmm++) {
                    setret = tempdimnamelist.insert(irmm->second);
                    if (true == setret.second) {
                       (*ird)->name = irmm->second;
                       (*ird)->newname = (*ird)->name;
                       setret = dimnamelist.insert((*ird)->name);
                       if(setret.second) Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size,(*ird)->unlimited_dim);
                       fakedimflag = false;
                       break;
                    }
                }
                      
                if (true == fakedimflag) {
                     Add_One_FakeDim_Name(*ird);
                     setsizeret = fakedimsize.insert((*ird)->size);
                     if (false == setsizeret.second)  
                        Adjust_Duplicate_FakeDim_Name(*ird);
                }
            
            } // for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
            tempdimnamelist.clear();
            fakedimsize.clear();
        } // for (vector<Var *>::iterator irv = this->vars.begin();
    } // else
}

// This is a special helper function for MeaSURES ozone products
bool GMFile::check_cv(string & varname)  {

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
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); irv++) {

        for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                        ira != (*irv)->attrs.end(); ++ira) {

            if("DimensionNames" == (*ira)->name) {

                Retrieve_H5_Attr_Value(*ira,(*irv)->fullpath);
                string dimname_value((*ira)->value.begin(),(*ira)->value.end());

                vector<string> ind_elems;
                char sep=',';
                HDF5CFUtil::Split(&dimname_value[0],sep,ind_elems);

                if(ind_elems.size() != (size_t)((*irv)->getRank())) {
                    throw2("The number of dims obtained from the <DimensionNames> attribute is not equal to the rank ", 
                           (*irv)->name); 
                }

                for(unsigned int i = 0; i<ind_elems.size(); ++i) {

                    ((*irv)->dims)[i]->name = ind_elems[i];

                    // Generate a dimension name if the dimension name is missing.
                    // The routine will ensure that the fakeDim name is unique.
                    if(((*irv)->dims)[i]->name==""){ 
                        Add_One_FakeDim_Name(((*irv)->dims)[i]);
// For debugging
#if 0
                        string fakedim = "FakeDim";
                        stringstream sdim_count;
                        sdim_count << dim_count;
                        fakedim = fakedim + sdim_count.str();
                        dim_count++;
                        ((*irv)->dims)[i]->name = fakedim;
                        ((*irv)->dims)[i]->newname = fakedim;
                        ind_elems[i] = fakedim;
#endif
                    }
                    
                    else {
                        ((*irv)->dims)[i]->newname = ind_elems[i];
                        setret = dimnamelist.insert(((*irv)->dims)[i]->name);
                   
                        if (true == setret.second) {
                            Insert_One_NameSizeMap_Element(((*irv)->dims)[i]->name,
                                                           ((*irv)->dims)[i]->size,
                                                           ((*irv)->dims)[i]->unlimited_dim);
                        }
                        else {
                            if(dimname_to_dimsize[((*irv)->dims)[i]->name] !=((*irv)->dims)[i]->size)
                                throw5("Dimension ",((*irv)->dims)[i]->name, "has two sizes",   
                                        ((*irv)->dims)[i]->size,dimname_to_dimsize[((*irv)->dims)[i]->name]);

                        }
                    }

                }// for(unsigned int i = 0; i<ind_elems.size(); ++i)
                break;

            } //if("DimensionNames" == (*ira)->name)
        } //for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin()

#if 0
        if(false == has_dim_name_attr) {

            throw4( "The variable ", (*irv)->name, " doesn't have the DimensionNames attribute.",
                    "We currently don't support this case. Please report to the NASA data center.");
        }
            
#endif
    } //for (vector<Var *>::iterator irv = this->vars.begin();
 
}
     
// Add Dimension names for Aquarius level 3 products
void GMFile::Add_Dim_Name_Aqu_L3()
{
    BESDEBUG("h5", "Coming to Add_Dim_Name_Aqu_L3()"<<endl);
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); irv++) {
        if ("l3m_data" == (*irv)->name) {
           ((*irv)->dims)[0]->name = "lat";
           ((*irv)->dims)[0]->newname = "lat";
           ((*irv)->dims)[1]->name = "lon";
           ((*irv)->dims)[1]->newname = "lon";
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

    }// for (vector<Var *>::iterator irv = this->vars.begin()
}

// Add dimension names for OSMAPL2S(note: the SMAP change their structures. The code doesn't not apply to them.)
void GMFile::Add_Dim_Name_OSMAPL2S(){

    BESDEBUG("h5", "Coming to Add_Dim_Name_OSMAPL2S()"<<endl);
    string tempvarname ="";
    string key = "_lat";
    string osmapl2sdim0 ="YDim";
    string osmapl2sdim1 ="XDim";

    // Since the dim. size of each dimension of 2D lat/lon may be the same, so use multimap.
    multimap<hsize_t,string> osmapl2sdimsize_to_dimname;
    pair<multimap<hsize_t,string>::iterator,multimap<hsize_t,string>::iterator> mm_er_ret;
    multimap<hsize_t,string>::iterator irmm; 

    // Generate dimension names based on the size of "???_lat"(one coordinate variable) 
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
        tempvarname = (*irv)->name;
        if ((tempvarname.size() > key.size())&& 
            (key == tempvarname.substr(tempvarname.size()-key.size(),key.size()))){
            if ((*irv)->dims.size() !=2) 
                throw1("Currently only 2D lat/lon is supported for OSMAPL2S");
            osmapl2sdimsize_to_dimname.insert(pair<hsize_t,string>(((*irv)->dims)[0]->size,osmapl2sdim0));
            osmapl2sdimsize_to_dimname.insert(pair<hsize_t,string>(((*irv)->dims)[1]->size,osmapl2sdim1));
            break;
        }
    }

    set<hsize_t> fakedimsize;
    pair<set<hsize_t>::iterator,bool> setsizeret;
    pair<set<string>::iterator,bool> setret;
    pair<set<string>::iterator,bool> tempsetret;
    set<string> tempdimnamelist;
    bool fakedimflag = false;


    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {

            fakedimflag = true;
            mm_er_ret = osmapl2sdimsize_to_dimname.equal_range((*ird)->size);
            for (irmm = mm_er_ret.first; irmm!=mm_er_ret.second;irmm++) {
                setret = tempdimnamelist.insert(irmm->second);
                if (setret.second) {
                   (*ird)->name = irmm->second;
                   (*ird)->newname = (*ird)->name;
                   setret = dimnamelist.insert((*ird)->name);
                   if(setret.second) Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size,(*ird)->unlimited_dim);
                   fakedimflag = false;
                   break;
                }
            }
                      
            if (true == fakedimflag) {
                 Add_One_FakeDim_Name(*ird);
                 setsizeret = fakedimsize.insert((*ird)->size);
                 if (!setsizeret.second)  
                    Adjust_Duplicate_FakeDim_Name(*ird);
            }
        } // for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
        tempdimnamelist.clear();
        fakedimsize.clear();
    } // for (vector<Var *>::iterator irv = this->vars.begin();
}

//Add dimension names for ACOS level2S or OCO2 level1B products
void GMFile::Add_Dim_Name_ACOS_L2S_OCO2_L1B(){
 
    BESDEBUG("h5", "Coming to Add_Dim_Name_ACOS_L2S_OCO2_L1B()"<<endl);
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
                Add_One_FakeDim_Name(*ird);
                setsizeret = fakedimsize.insert((*ird)->size);
                if (false == setsizeret.second)   
                    Adjust_Duplicate_FakeDim_Name(*ird);
        }
    } // for (vector<Var *>::iterator irv = this->vars.begin();
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
    if(false == Check_Dimscale_General_Product_Pattern()) {
        if(false == Check_LatLon2D_General_Product_Pattern()) 
            if(false == Check_LatLon1D_General_Product_Pattern())
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
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
          ira != (*irv)->attrs.end();ira++) {
           if ("DIMENSION_LIST" == (*ira)->name) {
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
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {


        for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
          ira != (*irv)->attrs.end();ira++) {
            if ("CLASS" == (*ira)->name) {

                Retrieve_H5_Attr_Value(*ira,(*irv)->fullpath);
                string class_value;
                class_value.resize((*ira)->value.size());
                copy((*ira)->value.begin(),(*ira)->value.end(),class_value.begin());

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

            for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
                 
                bool has_class_dscale = false;
                bool has_name = false;
                bool has_netcdf4_id = false;

                for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end();ira++) {
                    if ("CLASS" == (*ira)->name) {

                        Retrieve_H5_Attr_Value(*ira,(*irv)->fullpath);
                        string class_value;
                        class_value.resize((*ira)->value.size());
                        copy((*ira)->value.begin(),(*ira)->value.end(),class_value.begin());

                        // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
                        // "DIMENSION_SCALE", which is 15.
                        if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) 
                            has_class_dscale= true;
                    }
                    else if ("NAME" == (*ira)->name)
                        has_name = true;
                    else if ("_Netcdf4Dimid" == (*ira)->name)
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
    // If the first two cases don't exist, we allow to check another group"GeolocationData" and
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
    //if(2 == ll_flag) 
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

    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        if((*irv)->rank == 1) {
            if((*irv)->name == latname)  {

                string lat_path =HDF5CFUtil::obtain_string_before_lastslash((*irv)->fullpath);

                // Tackle only the root group or the name of the group as "/Geolocation"
                // May not generate the correct output. See https://jira.hdfgroup.org/browse/HFVHANDLER-175
                if("/" == lat_path || "/Geolocation/" == lat_path) {
                    ll_flag++;
                    lat_size = (*irv)->getDimensions()[0]->size; 
                }
            }
            else if((*irv)->name == lonname) {
                string lon_path = HDF5CFUtil::obtain_string_before_lastslash((*irv)->fullpath);
                if("/" == lon_path || "/Geolocation/" == lon_path) {
                    ll_flag++;
                    lon_size = (*irv)->getDimensions()[0]->size; 
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
            for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
                if((*irv)->rank >=2) {
                    short ll_size_flag = 0;
                    for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
                       ird != (*irv)->dims.end(); ++ird) {
                        if(lat_size == (*ird)->size) {
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
    string co_attrvalue="";
    string unit_attrname = "units";
    string lat_unit_attrvalue ="degrees_north";
    string lon_unit_attrvalue ="degrees_east";

    bool coor_has_lat_flag = false;
    bool coor_has_lon_flag = false;

    vector<Var*> tempvar_lat;
    vector<Var*> tempvar_lon;

    // Check if having both lat, lon names stored in the coordinate attribute value by looping through rank >1 variables.
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        if((*irv)->rank >=2) {
            for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin();
                ira !=(*irv)->attrs.end();++ira) {

                // If having attribute "coordinates" for this variable, checking the values and 
                // see if having lat/lon,latitude/longitude, Latitude/Longitude pairs.
                if((*ira)->name == co_attrname) {
                    Retrieve_H5_Attr_Value((*ira),(*irv)->fullpath);
                    string orig_attr_value((*ira)->value.begin(),(*ira)->value.end());
                    vector<string> coord_values;
                    char sep=' ';
                    HDF5CFUtil::Split_helper(coord_values,orig_attr_value,sep);
                       
                    for(vector<string>::iterator irs=coord_values.begin();irs!=coord_values.end();++irs) {
                        string coord_value_suffix1;
                        string coord_value_suffix2;
                        string coord_value_suffix3;

                        if((*irs).size() >=3) {

                            // both "lat" and "lon" have 3 characters.
                            coord_value_suffix1 = (*irs).substr((*irs).size()-3,3);

                            // The word "latitude" has 8 characters and the word "longitude" has 9 characters.
                            if((*irs).size() >=8){
                                coord_value_suffix2 = (*irs).substr((*irs).size()-8,8);
                                if((*irs).size() >=9)
                                    coord_value_suffix3 = (*irs).substr((*irs).size()-9,9);
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
                }// end if((*ira)->name
            }// for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin()
            if(true == coor_has_lat_flag && true == coor_has_lon_flag) 
                break;
            else {
                coor_has_lat_flag = false;
                coor_has_lon_flag = false;
            }
        } // if((*irv)->rank >=2) 
    }//  for (vector<Var *>::iterator irv = this->vars.begin()

    // Check the variable names that include latitude and longitude suffixes such as lat,latitude and Latitude. 
    if(true == coor_has_lat_flag && true == coor_has_lon_flag) {

        for (vector<Var *>::iterator irv = this->vars.begin();
            irv != this->vars.end(); ++irv) {
            bool var_is_lat = false;
            bool var_is_lon = false;
         
            string varname = (*irv)->name;
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
                if((*irv)->rank > 0) {
                    Var * lat = new Var(*irv);
                    tempvar_lat.push_back(lat);
                }
            }
            else if(true == var_is_lon) {
                if((*irv)->rank >0) {
                    Var * lon = new Var(*irv);
                    tempvar_lon.push_back(lon);
                }
            }
        }// for (vector<Var *>::iterator

        // Build up latloncv_candidate_pairs,  Name_Size_2Pairs struct, 
        // 1) Compare the rank, dimension sizes and the dimension orders of tempvar_lon against tempvar_lat
        //      rank >=2 the sizes,orders, should be consistent
        //      rank =1, no check issue.
        //   2) If the conditions are fulfilled, save them to the Name_Size struct 
        for(vector<Var*>:: iterator irlat = tempvar_lat.begin(); irlat!=tempvar_lat.end();++irlat) {

            // Check the rank =1 case
            if((*irlat)->rank == 1) 
                Build_lat1D_latlon_candidate(*irlat,tempvar_lon);

            // Check the reank>=2 case
            else if((*irlat)->rank >1)
                Build_latg1D_latlon_candidate(*irlat,tempvar_lon);
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
        if(latloncv_candidate_pairs.size() >0) {
            int num_1d_rank = 0;
            int num_2d_rank = 0;
            int num_g2d_rank = 0;
            vector<struct Name_Size_2Pairs> temp_1d_latlon_pairs;
            for(vector<struct Name_Size_2Pairs>::iterator ivs=latloncv_candidate_pairs.begin(); 
                ivs!=latloncv_candidate_pairs.end();++ivs) {
                if(1 == (*ivs).rank) {
                    num_1d_rank++;
                    temp_1d_latlon_pairs.push_back(*ivs);
                }
                else if(2 == (*ivs).rank)
                    num_2d_rank++;
                else if((*ivs).rank >2) 
                    num_g2d_rank++;
            }
 
            // This is the GENERAL_LATLON_COOR_ATTR case.
            if (num_2d_rank !=0) 
                ret_value = true;
            else if(num_1d_rank!=0) {

                // Check if lat and lon share the same size and the dimension of a variable 
                // that has the "coordinates" only holds one size.
                for(vector<struct Name_Size_2Pairs>::iterator ivs=temp_1d_latlon_pairs.begin();
                    ivs!=temp_1d_latlon_pairs.end();++ivs) {
                    if((*ivs).size1 != (*ivs).size2) {
                        ret_value = true;
                        break;
                    }
                    else {

                    // If 1-D lat and lon share the same size,we need to check if there is a variable
                    // that has both lat and lon as the coordinates but only has one dimension that holds the size.
                    // If this is true, this is not the GENERAL_LATLON_COOR_ATTR case(SMAP level 2 follows into the category).
                    
                        ret_value = true;
                        for (vector<Var *>::iterator irv = this->vars.begin();
                            irv != this->vars.end(); ++irv) {
                            if((*irv)->rank >=2) {
                                for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin();
                                    ira !=(*irv)->attrs.end();++ira) {
                                    // Check if this variable has the "coordinates" attribute
                                    if((*ira)->name == co_attrname) {
                                        Retrieve_H5_Attr_Value((*ira),(*irv)->fullpath);
                                        string orig_attr_value((*ira)->value.begin(),(*ira)->value.end());
                                        vector<string> coord_values;
                                        char sep=' ';
                                        HDF5CFUtil::Split_helper(coord_values,orig_attr_value,sep);
                                        bool has_lat_flag = false;
                                        bool has_lon_flag = false;
                                        for (vector<string>::iterator itcv=coord_values.begin();itcv!=coord_values.end();++itcv) {
                                            if((*ivs).name1 == (*itcv)) 
	                                        has_lat_flag = true;
                                            else if((*ivs).name2 == (*itcv))
                                                has_lon_flag = true;
                                        }
                                        // Find both lat and lon, now check the dim. size 
                                        if(true == has_lat_flag && true == has_lon_flag) {
                                            short has_same_ll_size = 0;
                                            for(vector<Dimension *>::iterator ird = (*irv)->dims.begin();ird!=(*irv)->dims.end();++ird){
                                                if((*ird)->size == (*ivs).size1)
                                                    has_same_ll_size++;
                                            }
                                            if(has_same_ll_size!=2){
                                                ret_value = false;
                                                break;
                                            }
                                        }
                                    }
                                }// for (vector<Attribute *>:: iterator ira 
                                if(false == ret_value)
                                    break;
                            }// if((*irv)->rank >=2) 
                        }// for (vector<Var *>::iterator irv

                        if(true == ret_value) 
                            break;
                    }// else 
                }// for(vector<struct Name_Size_2Pairs>::iterator ivs
            } // else if(num_1d_rank!=0)
        }// if(latloncv_candidate_pairs.size() >0)
        
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
void GMFile::Build_lat1D_latlon_candidate(Var *lat,const vector<Var*> &lon_vec) {

    BESDEBUG("h5", "Coming to Build_lat1D_latlon_candidate()"<<endl);
    set<string> lon_candidate_path;
    vector< pair<string,hsize_t> > lon_path_size_vec;

    // Obtain the path and the size info. from all the potential qualified longitude candidate.
    for(vector<Var *>::const_iterator irlon = lon_vec.begin(); irlon!=lon_vec.end();++irlon) {

        if (lat->rank == (*irlon)->rank) {
            pair<string,hsize_t>lon_path_size;
            lon_path_size.first = (*irlon)->fullpath;
            lon_path_size.second = (*irlon)->getDimensions()[0]->size;
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
        for(vector <pair<string,hsize_t> >::iterator islon =lon_path_size_vec.begin();islon!=lon_path_size_vec.end();++islon) {
            // Search the longitude path and see if it matches with the latitude.
            if(HDF5CFUtil::obtain_string_before_lastslash((*islon).first)==lat_path) {
                num_lon_match++;
                if(1 == num_lon_match)
                    lon_final_path_size = *islon;
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
    for(vector<Var*>:: const_iterator irlon = lon_vec.begin(); irlon!=lon_vec.end();++irlon) {

        if (lat->rank == (*irlon)->rank) {

            // Check the dim order and size.
            bool same_dim = true;
            for(int dim_index = 0; dim_index <lat->rank; dim_index++) {
                if(lat->getDimensions()[dim_index]->size !=
                   (*irlon)->getDimensions()[dim_index]->size){ 
                    same_dim = false;
                    break;
                }
            }
            if(true == same_dim) 
                lon_candidate_path.insert((*irlon)->fullpath);
        }
    }
           
    // Check the size of the lon., if the size is not 1, see if we can find the pair under the same group.
    if(lon_candidate_path.size() > 1) {

        string lat_path = HDF5CFUtil::obtain_string_before_lastslash(lat->fullpath);
        vector <string> lon_final_candidate_path_vec;
        for(set<string>::iterator islon_path =lon_candidate_path.begin();islon_path!=lon_candidate_path.end();++islon_path) {

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

            for(vector<string>::iterator ilon = lon_final_candidate_path_vec.begin(); ilon!=lon_final_candidate_path_vec.end();++ilon) {
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
        }// else if(lon_final_candidate_path_vec.size() >1) 
    }//  if(lon_candidate_path.size() > 1)

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
    for(set<int>::reverse_iterator its= duplicate_index.rbegin();its!=duplicate_index.rend();++its) {
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

    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        if((*irv)->rank == ll_rank) {
            if((*irv)->name == "lat") {
                ll_flag[0]++;
                if(ll_rank == 2) {
                    lat_size[0] = (*irv)->getDimensions()[0]->size; 
                    lat_size[1] = (*irv)->getDimensions()[1]->size; 
                    
                }
                    
            }
            else if((*irv)->name == "lon") {
                ll_flag[0]++;
                if(ll_rank == 2) {
                    lon_size[0] = (*irv)->getDimensions()[0]->size; 
                    lon_size[1] = (*irv)->getDimensions()[1]->size; 
                    
                }
 
            }
            else if((*irv)->name == "latitude"){
                ll_flag[1]++;
                if(ll_rank == 2) {
                    lat_size[2] = (*irv)->getDimensions()[0]->size; 
                    lat_size[3] = (*irv)->getDimensions()[1]->size; 
                    
                }
            }
            else if((*irv)->name == "longitude"){
                ll_flag[1]++;
                if(ll_rank == 2) {
                    lon_size[2] = (*irv)->getDimensions()[0]->size; 
                    lon_size[3] = (*irv)->getDimensions()[1]->size; 
                    
                }
 
            }
            else if((*irv)->name == "Latitude"){
                 ll_flag[2]++;
                if(ll_rank == 2) {
                    lat_size[4] = (*irv)->getDimensions()[0]->size; 
                    lat_size[5] = (*irv)->getDimensions()[1]->size; 
                    
                }
 
            }
            else if((*irv)->name == "Longitude"){
                ll_flag[2]++;
                if(ll_rank == 2) {
                    lon_size[4] = (*irv)->getDimensions()[0]->size; 
                    lon_size[5] = (*irv)->getDimensions()[1]->size; 
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
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
 
        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
                Add_One_FakeDim_Name(*ird);
                setsizeret = fakedimsize.insert((*ird)->size);

                // Avoid the same size dimension sharing the same dimension name.
                if (false == setsizeret.second)   
                    Adjust_Duplicate_FakeDim_Name(*ird);
        }

        // Find variable name that is latitude or lat or Latitude
        // Note that we don't need to check longitude since longitude dim. sizes should be the same as the latitude for this case.
        if((*irv)->name == gp_latname) {
            if((*irv)->rank != 2) {
                throw4("coordinate variables ",gp_latname,
                " must have rank 2 for the 2-D latlon case , the current rank is ",
                (*irv)->rank);
            }
            latdimname0 = (*irv)->getDimensions()[0]->name;
            latdimsize0 = (*irv)->getDimensions()[0]->size;
            
            latdimname1 = (*irv)->getDimensions()[1]->name;
            latdimsize1 = (*irv)->getDimensions()[1]->size;
        }
    }


    // Now we need to change a dimension of a general variable that shares the same size of lat
    // to the dimension name of the lat. 
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        int lat_dim0_index = 0;
        int lat_dim1_index = 0;
        bool has_lat_dims_size = false;

        for (unsigned int dim_index = 0; dim_index <(*irv)->dims.size(); dim_index++) {

            // Find if having the first dimension size of lat
            if(((*irv)->dims[dim_index])->size == latdimsize0) {

                // Find if having the second dimension size of lat
                lat_dim0_index = dim_index;
                for(unsigned int dim_index2 = dim_index+1;dim_index2 < (*irv)->dims.size();dim_index2++) {
                    if(((*irv)->dims[dim_index2])->size == latdimsize1) {
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
            ((*irv)->dims[lat_dim0_index])->name = latdimname0;
            //((*irv)->dims[lat_dim0_index])->newname = latdimname0;

            ((*irv)->dims[lat_dim1_index])->name = latdimname1;
            //((*irv)->dims[lat_dim1_index])->newname = latdimname1;

        }
    }

    //When we generate Fake dimensions, we may encounter discontiguous Fake dimension names such 
    // as FakeDim0, FakeDim9 etc. We would like to make Fake dimension names in contiguous order
    // FakeDim0,FakeDim1,etc.

    // Obtain the tempdimnamelist set.
    set<string>tempdimnamelist;

    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) 
            tempdimnamelist.insert((*ird)->name);

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
        set<string>:: iterator tempit = tempdimnamelist.begin();
        set<string>:: iterator finalit = finaldimnamelist.begin();
        while(tempit != tempdimnamelist.end()) {
            tempdimname_to_finaldimname[*tempit] = *finalit;
            tempit++;
            finalit++;
        } 

        // Change the dimension names of every variable to the final dimension name list.
        for (vector<Var *>::iterator irv = this->vars.begin();
             irv != this->vars.end(); ++irv) {
            for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
                ird != (*irv)->dims.end(); ++ird) {
                if(tempdimname_to_finaldimname.find((*ird)->name) !=tempdimname_to_finaldimname.end()){
                    (*ird)->name = tempdimname_to_finaldimname[(*ird)->name];
                }
                else 
                    throw3("The  dimension names ",(*ird)->name, "cannot be found in the dim. name list.");
            }
        }
    }


    dimnamelist.clear();
    dimnamelist = finaldimnamelist;

    // We need to update dimname_to_dimsize map. This may be used in the future.
    dimname_to_dimsize.clear();
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
            if(finaldimnamelist.find((*ird)->name)!=finaldimnamelist.end()) {
                dimname_to_dimsize[(*ird)->name] = (*ird)->size;
                dimname_to_unlimited[(*ird)->name] = (*ird)->unlimited_dim;
                finaldimnamelist.erase((*ird)->name);
            }
            
        }
        if(true == finaldimnamelist.empty())
            break;
    }
 
    // Finally set dimension newname
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
            (*ird)->newname = (*ird)->name;    
        }
    }
 
}

// Add dimension names for the case that has 1-D lat/lon or CoordAttr..
// 
void GMFile::Add_Dim_Name_LatLon1D_Or_CoordAttr_General_Product()  {

    BESDEBUG("h5", "Coming to Add_Dim_Name_LatLon1D_Or_CoordAttr_General_Product()"<<endl);
    // Only need to add the fake dimension names
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
 
        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
                Add_One_FakeDim_Name(*ird);
                setsizeret = fakedimsize.insert((*ird)->size);
                // Avoid the same size dimension sharing the same dimension name.
                if (false == setsizeret.second)   
                    Adjust_Duplicate_FakeDim_Name(*ird);
        }
    }
}

// For netCDF-4-like HDF5 products, we need to add the dimension scales.
void GMFile::Add_Dim_Name_Dimscale_General_Product()  {

    BESDEBUG("h5", "Coming to Add_Dim_Name_Dimscale_General_Product()"<<endl);
    //cerr<<"coming to Add_Dim_Name_Dimscale_General_Product"<<endl;
    pair<set<string>::iterator,bool> setret;
    this->iscoard = true;

    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        // Obtain all the dimension names for this variable
        Handle_UseDimscale_Var_Dim_Names_General_Product((*irv));

        // Need to update dimenamelist and dimname_to_dimsize and dimname_to_unlimited maps for future use.
        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end();++ird) { 
            setret = dimnamelist.insert((*ird)->name);
            if (true == setret.second) 
                Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size,(*ird)->unlimited_dim);
        }
    } // for (vector<Var *>::iterator irv = this->vars.begin();
 
    if (true == dimnamelist.empty()) 
        throw1("This product should have the dimension names, but no dimension names are found");

}

// Obtain dimension names for this variable when netCDF-4 model(using dimension scales) is followed.
void GMFile::Handle_UseDimscale_Var_Dim_Names_General_Product(Var *var)  {

    BESDEBUG("h5", "Coming to Handle_UseDimscale_Var_Dim_Names_General_Product()"<<endl);
    Attribute* dimlistattr = NULL;
    bool has_dimlist = false;
    bool has_dimclass   = false;

    for(vector<Attribute *>::iterator ira = var->attrs.begin();
          ira != var->attrs.end();ira++) {
        if ("DIMENSION_LIST" == (*ira)->name) {
            dimlistattr = *ira;
            has_dimlist = true;  
        }
        if ("CLASS" == (*ira)->name) {

            Retrieve_H5_Attr_Value(*ira,var->fullpath);
            string class_value;
            class_value.resize((*ira)->value.size());
            copy((*ira)->value.begin(),(*ira)->value.end(),class_value.begin());

            // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
            // "DIMENSION_SCALE", which is 15.
            if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                has_dimclass = true;
                break;
            }
        }

    } // for(vector<Attribute *>::iterator ira = var->attrs.begin(); ...

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
        for (vector<Dimension *>::iterator ird= var->dims.begin();
            ird != var->dims.end(); ++ird) {
                Add_One_FakeDim_Name(*ird);
                setsizeret = fakedimsize.insert((*ird)->size);
                // Avoid the same size dimension sharing the same dimension name.
                if (false == setsizeret.second)   
                    Adjust_Duplicate_FakeDim_Name(*ird);
        }
    }

}

// Add dimension names for the case when HDF5 dimension scale is followed(netCDF4-like)
void GMFile::Add_UseDimscale_Var_Dim_Names_General_Product(Var *var,Attribute*dimlistattr) 
{
    
    BESDEBUG("h5", "Coming to Add_UseDimscale_Var_Dim_Names_General_Product()"<<endl);
    ssize_t objnamelen = -1;
    hobj_ref_t rbuf;
    //hvl_t *vlbuf = NULL;
    vector<hvl_t> vlbuf;
    
    hid_t dset_id = -1;
    hid_t attr_id = -1;
    hid_t atype_id = -1;
    hid_t amemtype_id = -1;
    hid_t aspace_id = -1;
    hid_t ref_dset = -1;

    if(NULL == dimlistattr) 
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


        if (H5Aread(attr_id,amemtype_id,&vlbuf[0]) <0)  
            throw2("Cannot obtain the referenced object for the variable ",var->name);
        

        vector<char> objname;
        int vlbuf_index = 0;

        // The dimension names of variables will be the HDF5 dataset names dereferenced from the DIMENSION_LIST attribute.
        for (vector<Dimension *>::iterator ird = var->dims.begin();
                ird != var->dims.end(); ++ird) {

            if(vlbuf[vlbuf_index].p== NULL) 
                throw4("The dimension doesn't exist. Var name is ",var->name,"; the dimension index is ",vlbuf_index);
            rbuf =((hobj_ref_t*)vlbuf[vlbuf_index].p)[0];
            if ((ref_dset = H5RDEREFERENCE(attr_id, H5R_OBJECT, &rbuf)) < 0) 
                throw2("Cannot dereference from the DIMENSION_LIST attribute  for the variable ",var->name);

            if ((objnamelen= H5Iget_name(ref_dset,NULL,0))<=0) 
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);
            objname.resize(objnamelen+1);
            if ((objnamelen= H5Iget_name(ref_dset,&objname[0],objnamelen+1))<=0) 
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);

            string objname_str = string(objname.begin(),objname.end());

            // We need to remove the first character of the object name since the first character
            // of the object full path is always "/" and this will be changed to "_".
            // The convention of handling the dimension-scale general product is to remove the first "_".
            // Check the get_CF_string function of HDF5GMCF.cc.
            string trim_objname = objname_str.substr(0,objnamelen);
            (*ird)->name = string(trim_objname.begin(),trim_objname.end());

            pair<set<string>::iterator,bool> setret;
            setret = dimnamelist.insert((*ird)->name);
            if (true == setret.second) 
               Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size,(*ird)->unlimited_dim);
            (*ird)->newname = (*ird)->name;
            H5Dclose(ref_dset);
#if 0
            ref_dset = -1;
#endif
            objname.clear();
            vlbuf_index++;
        }// for (vector<Dimension *>::iterator ird = var->dims.begin()
        if(vlbuf.size()!= 0) {

            if ((aspace_id = H5Aget_space(attr_id)) < 0)
                throw2("Cannot get hdf5 dataspace id for the attribute ",dimlistattr->name);

            if (H5Dvlen_reclaim(amemtype_id,aspace_id,H5P_DEFAULT,(void*)&vlbuf[0])<0) 
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
    // 3) Dimensions don't follow HDF5 dimension scale specification bu have 2D lat/lon
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
    else if (GPMS_L3 == this->product_type || GPMM_L3 == this->product_type) 
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
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ) {
        if((*irv)->rank == 2 && (*irv)->name == "Latitude") {
            GMCVar* GMcvar = new GMCVar(*irv);
            size_t lat_pos = (*irv)->fullpath.rfind("Latitude");
            string lat_path = (*irv)->fullpath.substr(0,lat_pos);
            GMcvar->cfdimname = lat_path + ((*irv)->dims)[0]->name;    
            ll_dim_set.insert(((*irv)->dims)[0]->name);
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar);
            delete(*irv);
            irv = this->vars.erase(irv);
        }
       
        if((*irv)->rank == 2 && (*irv)->name == "Longitude") {
            GMCVar* GMcvar = new GMCVar(*irv);
            size_t lon_pos = (*irv)->fullpath.rfind("Longitude");
            string lon_path = (*irv)->fullpath.substr(0,lon_pos);
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
    }// for (vector<Var *>::iterator irv = this->vars.begin();...

#if 0
    // Loop through all variables and create a dim set.
    set<string> cvdimset;
    pair<set<string>::iterator,bool> setret;
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        for(vector<Dimension *>::iterator ird = (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
            setret = cvdimset.insert((*ird)->name);
cerr<<"var name is "<<(*irv)->fullpath <<endl;
            if (true == setret.second) {
cerr<<"dim name is "<<(*ird)->name <<endl;
                Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size);
            }
        }
    }// for (vector<Var *>::iterator irv = this->vars.begin();...
#endif

    // For each dimension, create a coordinate variable.
    // Here we just need to loop through the map dimname_to_dimsize,
    // use the name and the size to create coordinate variables.
    for (map<string,hsize_t>::const_iterator itd = dimname_to_dimsize.begin();
                                            itd!=dimname_to_dimsize.end();++itd) {
        // We will not create fake coordinate variables for the
        // dimensions of latitude and longitude.
        if((ll_dim_set.find(itd->first)) == ll_dim_set.end()) {
            GMCVar*GMcvar = new GMCVar();
            Create_Missing_CV(GMcvar,itd->first);
            this->cvars.push_back(GMcvar);
        }
    }//for (map<string,hsize_t>::iterator itd = dimname_to_dimsize.begin(); ...

}

// Handle coordinate variables for GPM level 3
void GMFile::Handle_CVar_GPM_L3() {

    BESDEBUG("h5", "Coming to Handle_CVar_GPM_L3()"<<endl);
    iscoard = true;
    
    // Here we just need to loop through the map dimname_to_dimsize,
    // use the name and the size to create coordinate variables.
    for (map<string,hsize_t>::const_iterator itd = dimname_to_dimsize.begin();
                                            itd!=dimname_to_dimsize.end();++itd) {

        GMCVar*GMcvar = new GMCVar();
        if("nlon" == itd->first || "nlat" == itd->first
           || "lnH" == itd->first || "ltH" == itd->first
           || "lnL" == itd->first || "ltL" == itd->first) {
            GMcvar->name = itd->first;
            GMcvar->newname = GMcvar->name;
            GMcvar->fullpath = GMcvar->name;
            GMcvar->rank = 1;
            GMcvar->dtype = H5FLOAT32; 
            Dimension* gmcvar_dim = new Dimension(itd->second);
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
            Dimension* gmcvar_dim = new Dimension(itd->second);
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
    }//for (map<string,hsize_t>::iterator itd = dimname_to_dimsize.begin(); ...
}

// Handle Coordinate variables for MeaSuRES SeaWiFS
void GMFile::Handle_CVar_Mea_SeaWiFS() {

    BESDEBUG("h5", "Coming to Handle_CVar_Mea_SeaWiFS()"<<endl);
    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;

    for (set<string>::iterator irs = dimnamelist.begin();
            irs != dimnamelist.end();++irs) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ) {
            if ((*irs)== (*irv)->fullpath) {

                if (!iscoard && (("/natrack" == (*irs)) 
                                 || "/nxtrack" == (*irs))) {
                    ++irv;
                    continue;
                 }

                if((*irv)->dims.size()!=1) 
                    throw3("Coard coordinate variable ",(*irv)->name, "is not 1D");

                // Create Coordinate variables.
                tempdimnamelist.erase(*irs);
                GMCVar* GMcvar = new GMCVar(*irv);
                GMcvar->cfdimname = *irs;
                GMcvar->cvartype = CV_EXIST;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar); 
                delete(*irv);
                irv = this->vars.erase(irv);
                   //irv--;
            } // if ((*irs)== (*irv)->fullpath)
            else if(false == iscoard) { 
            // 2-D lat/lon, natrack maps to lat, nxtrack maps to lon.
                 
                if ((((*irs) =="/natrack") && ((*irv)->fullpath == "/latitude"))
                  ||(((*irs) =="/nxtrack") && ((*irv)->fullpath == "/longitude"))) {
                    tempdimnamelist.erase(*irs);
                    GMCVar* GMcvar = new GMCVar(*irv);
                    GMcvar->cfdimname = *irs;    
                    GMcvar->cvartype = CV_EXIST;
                    GMcvar->product_type = product_type;
                    this->cvars.push_back(GMcvar);
                    delete(*irv);
                    irv = this->vars.erase(irv);
                }
                else {
                    ++irv;
                }

            }// else if(false == iscoard)
            else {
                ++irv;
            }
        } // for (vector<Var *>::iterator irv = this->vars.begin() ... 
    } // for (set<string>::iterator irs = dimnamelist.begin() ...

    // Creating the missing "third-dimension" according to the dimension names.
    // This may never happen for the current MeaSure SeaWiFS, but put it here for code coherence and completeness.
    // KY 12-30-2011
    for (set<string>::iterator irs = tempdimnamelist.begin();
        irs != tempdimnamelist.end();++irs) {
        GMCVar*GMcvar = new GMCVar();
        Create_Missing_CV(GMcvar,*irs);
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

    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ) {

        tempvarname = (*irv)->name;

        if ((tempvarname.size() > key0.size())&&
                (key0 == tempvarname.substr(tempvarname.size()-key0.size(),key0.size()))){

            foundkey0 = true;

            if (dimnamelist.find(osmapl2sdim0)== dimnamelist.end()) 
                throw5("variable ",tempvarname," must have dimension ",osmapl2sdim0," , but not found ");

            tempdimnamelist.erase(osmapl2sdim0);
            GMCVar* GMcvar = new GMCVar(*irv);
            GMcvar->newname = GMcvar->name; // Remove the path, just use the variable name
            GMcvar->cfdimname = osmapl2sdim0;    
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar);
            delete(*irv);
            irv = this->vars.erase(irv);
        }// if ((tempvarname.size() > key0.size())&& ...
                    
        else if ((tempvarname.size() > key1.size())&& 
                (key1 == tempvarname.substr(tempvarname.size()-key1.size(),key1.size()))){

            foundkey1 = true;

            if (dimnamelist.find(osmapl2sdim1)== dimnamelist.end()) 
                throw5("variable ",tempvarname," must have dimension ",osmapl2sdim1," , but not found ");

            tempdimnamelist.erase(osmapl2sdim1);

            GMCVar* GMcvar = new GMCVar(*irv);
            GMcvar->newname = GMcvar->name;
            GMcvar->cfdimname = osmapl2sdim1;    
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar);
            delete(*irv);
            irv = this->vars.erase(irv);
        }// else if ((tempvarname.size() > key1.size())&& ...
        else {
            ++irv;
        }
        if (true == foundkey0 && true == foundkey1) 
            break;
    } // for (vector<Var *>::iterator irv = this->vars.begin(); ...

    for (set<string>::iterator irs = tempdimnamelist.begin();
        irs != tempdimnamelist.end();++irs) {

        GMCVar*GMcvar = new GMCVar();
        Create_Missing_CV(GMcvar,*irs);
        this->cvars.push_back(GMcvar);
    }

}

// Handle coordinate variables for Aquarius level 3 products
void GMFile::Handle_CVar_Aqu_L3()  {

    BESDEBUG("h5", "Coming to Handle_CVar_Aqu_L3()"<<endl);
    iscoard = true;
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

        if ( "l3m_data" == (*irv)->name) { 
            for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                        ird != (*irv)->dims.end(); ++ird) {
                GMCVar*GMcvar = new GMCVar();
                GMcvar->name = (*ird)->name;
                GMcvar->newname = GMcvar->name;
                GMcvar->fullpath = GMcvar->name;
                GMcvar->rank = 1;
                GMcvar->dtype = H5FLOAT32; 
                Dimension* gmcvar_dim = new Dimension((*ird)->size);
                gmcvar_dim->name = GMcvar->name;
                gmcvar_dim->newname = gmcvar_dim->name;
                GMcvar->dims.push_back(gmcvar_dim); 
                GMcvar->cfdimname = gmcvar_dim->name;
                if ("lat" ==GMcvar->name ) GMcvar->cvartype = CV_LAT_MISS;
                if ("lon" == GMcvar->name ) GMcvar->cvartype = CV_LON_MISS;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar);
            } // for (vector<Dimension *>::iterator ird = (*irv)->dims.begin(); ...
        } // if ( "l3m_data" == (*irv)->name) 
    }//for (vector<Var *>::iterator irv = this->vars.begin(); ...
 
}

//Handle coordinate variables for MeaSuRES Ozone products
void GMFile::Handle_CVar_Mea_Ozone() {

    BESDEBUG("h5", "Coming to Handle_CVar_Mea_Ozone()"<<endl);
    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;

    if(false == iscoard) 
        throw1("Measure Ozone level 3 zonal average product must follow COARDS conventions");

    for (set<string>::iterator irs = dimnamelist.begin();
            irs != dimnamelist.end();++irs) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ) {
            if ((*irs)== (*irv)->fullpath) {

                if((*irv)->dims.size()!=1) 
                    throw3("Coard coordinate variable",(*irv)->name, "is not 1D");

                // Create Coordinate variables.
                tempdimnamelist.erase(*irs);
                GMCVar* GMcvar = new GMCVar(*irv);
                GMcvar->cfdimname = *irs;
                GMcvar->cvartype = CV_EXIST;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar); 
                delete(*irv);
                irv = this->vars.erase(irv);
            } // if ((*irs)== (*irv)->fullpath)
            else {
                ++irv;
            }
        } // for (vector<Var *>::iterator irv = this->vars.begin();
    } // for (set<string>::iterator irs = dimnamelist.begin();

    for (set<string>::iterator irs = tempdimnamelist.begin();
        irs != tempdimnamelist.end();irs++) {

        GMCVar*GMcvar = new GMCVar();
        Create_Missing_CV(GMcvar,*irs);
        this->cvars.push_back(GMcvar);
    }
}

// Handle coordinate variables for general products that use HDF5 dimension scales.
void GMFile::Handle_CVar_Dimscale_General_Product()  {

    BESDEBUG("h5", "Coming to Handle_CVar_Dimscale_General_Product"<<endl);
    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;

    for (set<string>::iterator irs = dimnamelist.begin();
            irs != dimnamelist.end();++irs) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ) {

            // This is the dimension scale dataset; it should be changed to a coordinate variable.
            if ((*irs)== (*irv)->fullpath) {
                if((*irv)->dims.size()!=1) 
                    throw3("COARDS coordinate variable",(*irv)->name, "is not 1D");

                // Create Coordinate variables.
                tempdimnamelist.erase(*irs);
                GMCVar* GMcvar = new GMCVar(*irv);
                GMcvar->cfdimname = *irs;

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
            } // if ((*irs)== (*irv)->fullpath)
            else {
                ++irv;
            }
       } // for (vector<Var *>::iterator irv = this->vars.begin();
    } // for (set<string>::iterator irs = dimnamelist.begin();

    // Check if we have 2-D lat/lon CVs, and if yes, add those to the CV list.
    Update_M2DLatLon_Dimscale_CVs();

    // Add other missing coordinate variables.
    for (set<string>::iterator irs = tempdimnamelist.begin();
        irs != tempdimnamelist.end();irs++) {
        GMCVar*GMcvar = new GMCVar();
        Create_Missing_CV(GMcvar,*irs);
        this->cvars.push_back(GMcvar);
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
        if(tempcvar_2dlat.size()>0)
            iscoard = false;

        // 7. Add the CVs based on the final 2-D lat/lon CV candidates.
        // We need to remember the dim names that the 2-D lat/lon CVs are associated with.
        set<string>dim_names_2d_cvs;

        for(vector<Var *>::iterator irv = tempcvar_2dlat.begin();irv != tempcvar_2dlat.end();++irv){
            GMCVar *lat = new GMCVar(*irv);
            // Latitude is always corresponding to the first dimension.
            lat->cfdimname = (*irv)->getDimensions()[0]->name;
            dim_names_2d_cvs.insert(lat->cfdimname);
            lat->cvartype = CV_EXIST;
            lat->product_type = product_type;
            this->cvars.push_back(lat);
        }
        for(vector<Var *>::iterator irv = tempcvar_2dlon.begin();irv != tempcvar_2dlon.end();++irv){
            GMCVar *lon = new GMCVar(*irv);
            // Longitude is always corresponding to the second dimension.
            lon->cfdimname = (*irv)->getDimensions()[1]->name;
            dim_names_2d_cvs.insert(lon->cfdimname);
            lon->cvartype = CV_EXIST;
            lon->product_type = product_type;
            this->cvars.push_back(lon);
        }
     
        // 8. Move the originally assigned 1-D CVs that are replaced by 2-D CVs back to the general variable list. 
        //    Also remove the CV created by the pure dimensions. 
        //    Dimension names are used to identify those 1-D CVs.
        for(vector<GMCVar*>::iterator ircv= this->cvars.begin();ircv !=this->cvars.end();) {
            if(1 == (*ircv)->rank) {
                if(dim_names_2d_cvs.find((*ircv)->cfdimname)!=dim_names_2d_cvs.end()) {
                    if(CV_FILLINDEX == (*ircv)->cvartype) {// This is pure dimension
                        delete(*ircv);
                        ircv = this->cvars.erase(ircv);
                    }
                    else if(CV_EXIST == (*ircv)->cvartype) {// This var exists already 

                        // Add this var. to the var list.
                        Var *var = new Var(*ircv);
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
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
            ircv != this->cvars.end(); ++ircv) {

        if((*ircv)->cvartype == CV_EXIST) {
            string attr_name ="units";
            string lat_unit_value = "degrees_north";
            string lon_unit_value = "degrees_east";

           for(vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                     ira != (*ircv)->attrs.end();ira++) {

                if(true == Is_Str_Attr(*ira,(*ircv)->fullpath,attr_name,lat_unit_value)) {
                    lat_size = (*ircv)->getDimensions()[0]->size;
                    lat_dimname = (*ircv)->getDimensions()[0]->name;
                    has_1d_lat_cv_flag = true;
                    break;
                }
                else if(true == Is_Str_Attr(*ira,(*ircv)->fullpath,attr_name,lon_unit_value)){ 
                    lon_size = (*ircv)->getDimensions()[0]->size;
                    lon_dimname = (*ircv)->getDimensions()[0]->name;
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
            if(0 == this->groups.size()) {

                // Rarely happens when lat_size is the same as the lon_size.
                // However, still want to make sure there is a 2-D variable that uses both lat and lon dims.
                if(lat_size == lon_size) { 
                    bool var_has_latdim = false;
                    bool var_has_londim = false;
                    for (vector<Var *>::iterator irv = this->vars.begin();
                        irv != this->vars.end(); ++irv) {
                        if((*irv)->rank >= 2) {
                            for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                                ird !=(*irv)->dims.end();++ird) {
                                if((*ird)->name == lat_dimname)
                                    var_has_latdim = true;
                                else if((*ird)->name == lon_dimname)
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
                for (vector<Var *>::iterator irv = this->vars.begin();
                    irv != this->vars.end(); ++irv) {
                    if((*irv)->rank == 2) { 
                        
                        //Note: When the 2nd parameter is true in the function Is_geolatlon, it checks the lat/latitude/Latitude 
                        if(true == Is_geolatlon((*irv)->name,true))
                            has_2d_latname_flag = true;

                        //Note: When the 2nd parameter is false in the function Is_geolatlon, it checks the lon/longitude/Longitude
                        else if(true == Is_geolatlon((*irv)->name,false))
                            has_2d_lonname_flag = true;

                        if((true == has_2d_latname_flag) && (true == has_2d_lonname_flag))
                            break;
                    }
                }

                if(has_2d_latname_flag != true || has_2d_lonname_flag != true) {

                    //Check if having the 2-D lat/lon by checking if having lat/lon CF units(lon's units: degrees_east  lat's units: degrees_north) 
                    has_2d_latname_flag = false;
                    has_2d_lonname_flag = false;

                    for (vector<Var *>::iterator irv = this->vars.begin();
                        irv != this->vars.end(); ++irv) {
                        if((*irv)->rank == 2) {
                            for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                                ira != (*irv)->attrs.end(); ++ira) {

                                if (false == has_2d_latname_flag) {

                                    // When the third parameter of the function has_latlon_cf_units is set to true, it checks latitude
                                    has_2d_latname_flag = has_latlon_cf_units((*ira),(*irv)->fullpath,true);
                                    if(true == has_2d_latname_flag)
                                        break;
                                    else if(false == has_2d_lonname_flag) {

                                        // When the third parameter of the function has_latlon_cf_units is set to false, it checks longitude 
                                        has_2d_lonname_flag = has_latlon_cf_units((*ira),(*irv)->fullpath,false);
                                        if(true == has_2d_lonname_flag)
                                            break;
                                    }
                                }
                                else if(false == has_2d_lonname_flag) {

                                    // Now has_2d_latname_flag is true, just need to check the has_2d_lonname_flag
                                    // When the third parameter of has_latlon_cf_units is set to false, it checks longitude 
                                    has_2d_lonname_flag = has_latlon_cf_units((*ira),(*irv)->fullpath,false);
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
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
            ircv != this->cvars.end(); ++ircv) {

        if((*ircv)->cvartype == CV_EXIST) {

            string attr_name ="units";
            string lat_unit_value = "degrees_north";
            string lon_unit_value = "degrees_east";

           for(vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                     ira != (*ircv)->attrs.end();ira++) {

                // 1-D latitude
                if(true == Is_Str_Attr(*ira,(*ircv)->fullpath,attr_name,lat_unit_value)) {
                    GMCVar *lat = new GMCVar(*ircv);
                    lat->cfdimname = (*ircv)->getDimensions()[0]->name;
                    lat->cvartype = (*ircv)->cvartype;
                    lat->product_type = (*ircv)->product_type;
                    cvar_1dlat.push_back(lat);
                }
                // 1-D longitude
                else if(true == Is_Str_Attr(*ira,(*ircv)->fullpath,attr_name,lon_unit_value)){ 
                    GMCVar *lon = new GMCVar(*ircv);
                    lon->cfdimname = (*ircv)->getDimensions()[0]->name;
                    lon->cvartype = (*ircv)->cvartype;
                    lon->product_type = (*ircv)->product_type;
                    cvar_1dlon.push_back(lon);
                }
            }
        }// if((*ircv)->cvartype == CV_EXIST)
    }// for (vector<GMCVar *>::iterator ircv = this->cvars.begin();

}

// Obtain all 2-D lat/lon variables. We first check the lat/latitude/Latitude names, if not found, we check if CF lat/lon units are present.
// Latitude variables are saved in the vector var_2dlat. Longitude variables are saved in the vector var_2dlon.
// We also remember the index of these lat/lon in the original var vector.
void GMFile::Obtain_2DLatLon_Vars(vector<Var*> &var_2dlat,vector<Var*> &var_2dlon,map<string,int> & latlon2d_path_to_index) {

    BESDEBUG("h5", "Coming to Obtain_2DLatLon_Vars()"<<endl);
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        if((*irv)->rank == 2) { 
                        
            //Note: When the 2nd parameter is true in the function Is_geolatlon, it checks the lat/latitude/Latitude 
            if(true == Is_geolatlon((*irv)->name,true)) {
                Var *lat = new Var(*irv);
                var_2dlat.push_back(lat);
                latlon2d_path_to_index[(*irv)->fullpath]= distance(this->vars.begin(),irv);
                continue;
            }
            else {

                bool has_2dlat = false;
                for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end(); ++ira) {

                    // When the third parameter of has_latlon_cf_units is set to true, it checks latitude
                    if(true == has_latlon_cf_units((*ira),(*irv)->fullpath,true)) {
                        Var *lat = new Var(*irv);
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
            if(true == Is_geolatlon((*irv)->name,false)) {
                Var *lon = new Var(*irv);
                latlon2d_path_to_index[(*irv)->fullpath] = distance(this->vars.begin(),irv);
                var_2dlon.push_back(lon);
            }
            else {
                for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end(); ++ira) {

                    // When the third parameter of has_latlon_cf_units is set to false, it checks longitude
                    if(true == has_latlon_cf_units((*ira),(*irv)->fullpath,false)) {
                        Var *lon = new Var(*irv);
                        latlon2d_path_to_index[(*irv)->fullpath] = distance(this->vars.begin(),irv);
                        var_2dlon.push_back(lon);
                        break;
                    }
                }
            }
        } // if((*irv)->rank == 2)
    } // for (vector<Var *>::iterator irv
}

//  Sequeeze the 2-D lat/lon vectors by removing the ones that share the same dims with 1-D lat/lon CVs.
//  The latlon2d_path_to_index map also needs to be updated.
void GMFile::Obtain_2DLLVars_With_Dims_not_1DLLCVars(vector<Var*> &var_2dlat,
                                                     vector<Var*> &var_2dlon, 
                                                     vector<GMCVar*> &cvar_1dlat,
                                                     vector<GMCVar*> &cvar_1dlon,
                                                     map<string,int> &latlon2d_path_to_index) {

    BESDEBUG("h5", "Coming to Obtain_2DLLVars_With_Dims_not_1DLLCVars()"<<endl);
    // First latitude at var_2dlat
    for(vector<Var *>::iterator irv = var_2dlat.begin();irv != var_2dlat.end();) {
        bool remove_2dlat = false;
        for(vector<GMCVar *>::iterator ircv = cvar_1dlat.begin();ircv != cvar_1dlat.end();++ircv) {
            for (vector<Dimension*>::iterator ird = (*irv)->dims.begin();
                ird!=(*irv)->dims.end(); ++ird) {
                if((*ird)->name == (*ircv)->getDimensions()[0]->name &&
                   (*ird)->size == (*ircv)->getDimensions()[0]->size) {
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
    }// for(vector<Var *>::iterator irv = var_2dlat.begin()

    // Second longitude
    for(vector<Var *>::iterator irv = var_2dlon.begin();irv != var_2dlon.end();) {
        bool remove_2dlon = false;
        for(vector<GMCVar *>::iterator ircv = cvar_1dlon.begin();ircv != cvar_1dlon.end();++ircv) {
            for (vector<Dimension*>::iterator ird = (*irv)->dims.begin();
                ird!=(*irv)->dims.end(); ++ird) {
                if((*ird)->name == (*ircv)->getDimensions()[0]->name &&
                   (*ird)->size == (*ircv)->getDimensions()[0]->size) {
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
    } // for(vector<Var *>::iterator irv = var_2dlon.begin()

}

//Out of the collected 2-D lat/lon variables, we will select the final qualified 2-D lat/lon as CVs.
void GMFile::Obtain_2DLLCVar_Candidate(vector<Var*> &var_2dlat,
                                       vector<Var*> &var_2dlon,
                                       map<string,int>& latlon2d_path_to_index) {
    BESDEBUG("h5", "Coming to Obtain_2DLLCVar_Candidate()"<<endl);
    // First check 2-D lat, see if we have the corresponding 2-D lon(same dims, under the same group).
    // If no, remove that lat from the vector.
    vector<string> lon2d_group_paths;

    for(vector<Var *>::iterator irv_2dlat = var_2dlat.begin();irv_2dlat !=var_2dlat.end();) {
        for(vector<Var *>::iterator irv_2dlon = var_2dlon.begin();irv_2dlon != var_2dlon.end();++irv_2dlon) {
            if(((*irv_2dlat)->getDimensions()[0]->name == (*irv_2dlon)->getDimensions()[0]->name) &&
               ((*irv_2dlat)->getDimensions()[0]->size == (*irv_2dlon)->getDimensions()[0]->size) &&
               ((*irv_2dlat)->getDimensions()[1]->name == (*irv_2dlon)->getDimensions()[1]->name) &&
               ((*irv_2dlat)->getDimensions()[1]->size == (*irv_2dlon)->getDimensions()[1]->size)) 
                lon2d_group_paths.push_back(HDF5CFUtil::obtain_string_before_lastslash((*irv_2dlon)->fullpath));
        }
        // Doesn't find any lons that shares the same dims,remove this lat from the 2dlat vector,
        // also update the latlon2d_path_to_index map
        if(0 == lon2d_group_paths.size()) {
            latlon2d_path_to_index.erase((*irv_2dlat)->fullpath);         
            delete(*irv_2dlat);
            irv_2dlat = var_2dlat.erase(irv_2dlat);
        }
        else {// Find lons,check if they are under the same group
            //string lat2d_group_path = (*irv_2dlat)->fullpath.substr(0,(*irv_2dlat)->fullpath.find_last_of("/"));
            string lat2d_group_path = HDF5CFUtil::obtain_string_before_lastslash((*irv_2dlat)->fullpath);

            // Check how many lon2d shares the same group with the lat2d
            short lon2d_has_lat2d_group_path_flag = 0;
            for(vector<string>::iterator ivs = lon2d_group_paths.begin();ivs!=lon2d_group_paths.end();++ivs) {
                if((*ivs)==lat2d_group_path) 
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
for(vector<Var *>::iterator irv_2dlat = var_2dlat.begin();irv_2dlat !=var_2dlat.end();++irv_2dlat) 
cerr<<"2 left 2-D lat variable full path is: "<<(*irv_2dlat)->fullpath <<endl;
#endif
 

    // Second check 2-D lon, see if we have the corresponding 2-D lat(same dims, under the same group).
    // If no, remove that lon from the vector.
    vector<string> lat2d_group_paths;

    // Check the longitude 
    for(vector<Var *>::iterator irv_2dlon = var_2dlon.begin();irv_2dlon !=var_2dlon.end();) {
        for(vector<Var *>::iterator irv_2dlat = var_2dlat.begin();irv_2dlat != var_2dlat.end();++irv_2dlat) {
            if(((*irv_2dlat)->getDimensions()[0]->name == (*irv_2dlon)->getDimensions()[0]->name) &&
               ((*irv_2dlat)->getDimensions()[0]->size == (*irv_2dlon)->getDimensions()[0]->size) &&
               ((*irv_2dlat)->getDimensions()[1]->name == (*irv_2dlon)->getDimensions()[1]->name) &&
               ((*irv_2dlat)->getDimensions()[1]->size == (*irv_2dlon)->getDimensions()[1]->size)) 
               lat2d_group_paths.push_back(HDF5CFUtil::obtain_string_before_lastslash((*irv_2dlat)->fullpath)); 
                //lat2d_group_paths.push_back((*irv_2dlat)->fullpath.substr(0,(*irv_2dlat)->fullpath.find_last_of("/")));
        }
        // Doesn't find any lats that shares the same dims,remove this lon from this vector
        if(0 == lat2d_group_paths.size()) {
            latlon2d_path_to_index.erase((*irv_2dlon)->fullpath);
            delete(*irv_2dlon);
            irv_2dlon = var_2dlon.erase(irv_2dlon);
        }
        else {
            string lon2d_group_path = HDF5CFUtil::obtain_string_before_lastslash((*irv_2dlon)->fullpath);

            // Check how many lat2d shares the same group with the lon2d
            short lat2d_has_lon2d_group_path_flag = 0;
            for(vector<string>::iterator ivs = lat2d_group_paths.begin();ivs!=lat2d_group_paths.end();++ivs) {
                if((*ivs)==lon2d_group_path) 
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

// If two vars in the 2-D lat or 2-D lon CV candidate vector share the same dim. , these two vars cannot be CVs. 
// The group they belong to is the group candidate that the coordinates attribute of the variable under that group may be modified..
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
                // compare the string size, long.compare(0,shortlength,short)==0, 
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
            }// if((var_ll[i]->getDimensions()[0]->name == var_ll[j]->getDimensions()[0]->name)
        }// for(int j = i+1; j<var_ll.size();j++)
    }// for( int i = 0; i <var_ll.size();i++)

    // Remove the shared 2-D lat/lon CVs from the 2-D lat/lon CV candidates. 
    int var_index = 0;
    for(vector<Var*>::iterator itv = var_ll.begin(); itv!= var_ll.end();) {
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
    vector<Var *>::iterator it = this->vars.begin();

    // This is a performance optimiziation operation.
    // We find it is typical for swath files that have many many general variables but only have very few lat/lon CVs.
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
bool GMFile::Check_Var_2D_CVars(Var *var)  {

    BESDEBUG("h5", "Coming to Check_Var_2D_CVars()"<<endl);
    bool ret_value = true;
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
        ircv != this->cvars.end(); ++ircv) {
        if((*ircv)->rank==2) {
            short first_dim_index = 0;
            short first_dim_times = 0;
            short second_dim_index = 0;
            short second_dim_times = 0;
            for (vector<Dimension *>::iterator ird = var->dims.begin();
                ird != var->dims.end(); ++ird) {
                if((*ird)->name == ((*ircv)->getDimensions()[0])->name) {
                    first_dim_index = distance(var->dims.begin(),ird);  
                    first_dim_times++;
                }
                else if((*ird)->name == ((*ircv)->getDimensions()[1])->name) {
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
    bool has_coor_attr = false;
    string orig_coor_value;
    string flatten_coor_value;
    // Assume the separator is always a space.
    char sc = ' ';

    for (vector<Attribute *>:: iterator ira =var->attrs.begin(); ira !=var->attrs.end();) {

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
        Attribute *attr = new Attribute();
        Add_Str_Attr(attr,co_attrname,flatten_coor_value);
        var->attrs.push_back(attr);
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

    for (vector<Attribute *>:: iterator ira =var->attrs.begin(); ira !=var->attrs.end();) {

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
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
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
        for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
            ircv != this->cvars.end(); ++ircv) {       
            if((*ircv)->cvartype == CV_EXIST) {
                for(vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
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
            
        for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
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

        for (vector<Var *>::iterator irv = this->vars.begin();
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
    for (vector<Var *>::iterator irv = this->vars.begin();
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
    for (vector<Var *>::iterator irv = this->vars.begin();
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

    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        // This is the dimension scale dataset; it should be changed to a coordinate variable.
        if (gp_latname== (*irv)->name) {

            // For latitude, regardless 1D or 2D, the first dimension needs to be updated.
            // Create Coordinate variables.
            tempdimnamelist.erase(((*irv)->dims[0])->name);
            GMCVar* GMcvar = new GMCVar(*irv);
            GMcvar->cfdimname = ((*irv)->dims[0])->name;
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar); 
            delete(*irv);
            this->vars.erase(irv);
            break;
        } // if ((*irs)== (*irv)->fullpath)
    } // for (vector<Var *>::iterator irv = this->vars.begin();

    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

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
            GMCVar* GMcvar = new GMCVar(*irv);
            GMcvar->cfdimname = londimname;
            GMcvar->cvartype = CV_EXIST;
            GMcvar->product_type = product_type;
            this->cvars.push_back(GMcvar); 
            delete(*irv);
            this->vars.erase(irv);
            break;
        } // if ((*irs)== (*irv)->fullpath)
    } // for (vector<Var *>::iterator irv = this->vars.begin();

    //
    // Add other missing coordinate variables.
    for (set<string>::iterator irs = tempdimnamelist.begin();
        irs != tempdimnamelist.end();irs++) {
        GMCVar*GMcvar = new GMCVar();
        Create_Missing_CV(GMcvar,*irs);
        this->cvars.push_back(GMcvar);
    }

}

// Handle coordinate variables for OBPG level 3 
void GMFile::Handle_CVar_OBPG_L3()  {

    BESDEBUG("h5", "Coming to Handle_CVar_OBPG_L3()"<<endl);
    if (GENERAL_DIMSCALE == this->gproduct_pattern)
            Handle_CVar_Dimscale_General_Product();

    // Change the CV Type of the corresponding CVs of lat and lon from CV_FILLINDEX to CV_LATMISS or CV_LONMISS
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        // Here I try to avoid using the dimension name row and column to find the lat/lon dimension size.
        // So I am looking for a 2-D floating-point array or a 2-D array under the group geophsical_data.
        // This may be subject to change if OBPG level 3 change its arrangement of variables.
        // KY 2014-09-29
 
        if((*irv)->rank == 2) {

            if(((*irv)->fullpath.find("/geophsical_data") == 0) || ((*irv)->dtype == H5FLOAT32)) {

                size_t lat_size = (*irv)->getDimensions()[0]->size;
                string lat_name = (*irv)->getDimensions()[0]->name;
                size_t lon_size = (*irv)->getDimensions()[1]->size;
                string lon_name = (*irv)->getDimensions()[1]->name;
                size_t temp_size = 0;
                string temp_name;
                H5DataType ll_dtype = (*irv)->dtype;

                // We always assume that longitude size is greater than latitude size.
                if(lat_size >lon_size) {
                    temp_size = lon_size;
                    temp_name = lon_name;
                    lon_size = lat_size;
                    lon_name = lat_name;
                    lat_size = temp_size;
                    lat_name = temp_name;
                }
                for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
                    ircv != this->cvars.end(); ++ircv) {
                    if((*ircv)->cvartype == CV_FILLINDEX) {
                        if((*ircv)->getDimensions()[0]->size == lat_size &&
                           (*ircv)->getDimensions()[0]->name == lat_name) {
                            (*ircv)->cvartype = CV_LAT_MISS;
                            (*ircv)->dtype = ll_dtype;
                            for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                                ira != (*ircv)->attrs.end(); ++ira) {
                                if ((*ira)->name == "NAME") {
                                    delete (*ira);
                                    (*ircv)->attrs.erase(ira);
                                    break;
                                }
                            }
                        }
                        else if((*ircv)->getDimensions()[0]->size == lon_size &&
                           (*ircv)->getDimensions()[0]->name == lon_name) {
                            (*ircv)->cvartype = CV_LON_MISS;
                            (*ircv)->dtype = ll_dtype;
                            for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                                ira != (*ircv)->attrs.end(); ++ira) {
                                if ((*ira)->name == "NAME") {
                                    delete (*ira);
                                    (*ircv)->attrs.erase(ira);
                                    break;
                                }
                            }
                        }

                    }
                }
                break;

            } // if(((*irv)->fullpath.find("/geophsical_data") == 0) || ((*irv)->dtype == H5FLOAT32))
        } // if((*irv)->rank == 2)
    } // for (vector<Var *>::iterator irv = this->vars.begin();

}

// Handle some special variables. Currently only GPM  and ACOS have these variables.
void GMFile::Handle_SpVar() {

    BESDEBUG("h5", "Coming to Handle_SpVar()"<<endl);
    if (ACOS_L2S_OR_OCO2_L1B == product_type) 
        Handle_SpVar_ACOS_OCO2();
    else if(GPM_L1 == product_type) {
        // Loop through the variable list to build the coordinates.
        // These variables need to be removed.
        for (vector<Var *>::iterator irv = this->vars.begin();
                    irv != this->vars.end(); ++irv) {
            if((*irv)->name=="AlgorithmRuntimeInfo") {
                delete(*irv);
                this->vars.erase(irv); 
                break;
            }
        }
    }

    // GPM level-3 These variables need to be removed.
    else if(GPMM_L3 == product_type || GPMS_L3 == product_type) {

        for (vector<Var *>::iterator irv = this->vars.begin();
                    irv != this->vars.end(); ) {
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
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ) {
        if (H5INT64 == (*irv)->getType()) {
            
            // First: Time Part of soundingid
            GMSPVar * spvar = new GMSPVar(*irv);
            spvar->name = (*irv)->name +"_Time";
            spvar->newname = (*irv)->newname+"_Time";
            spvar->dtype = H5INT32;
            spvar->otype = (*irv)->getType();
            spvar->sdbit = 1;

            // 2 digit hour, 2 digit min, 2 digit seconds
            spvar->numofdbits = 6;
            this->spvars.push_back(spvar);

            // Second: Date Part of soundingid
            GMSPVar * spvar2 = new GMSPVar(*irv);
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
        } // if (H5INT64 == (*irv)->getType())
        else {
            ++irv;
        }
    } // for (vector<Var *>::iterator irv = this->vars.begin(); ...
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
for (vector<Var*>::iterator irv2 = this->vars.begin();
    irv2 != this->vars.end(); irv2++) {
    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
         ird !=(*irv2)->dims.end(); ird++) {
         cerr<<"Dimension name afet Adjust_Obj_Name "<<(*ird)->newname <<endl;
    }
}
#endif

}

// Adjust object names for GPM level 3 products
void GMFile:: Adjust_GPM_L3_Obj_Name()  {

    BESDEBUG("h5", "Coming to Adjust_GPM_L3_Obj_Name()"<<endl);
    string objnewname;
    // In this definition, root group is not considered as a group.
    if(this->groups.size() <= 1) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                   irv != this->vars.end(); ++irv) {
            objnewname =  HDF5CFUtil::obtain_string_after_lastslash((*irv)->newname);
            if (objnewname !="") 
                (*irv)->newname = objnewname;
        }
    }
    else {
        for (vector<Var *>::iterator irv = this->vars.begin();
                   irv != this->vars.end(); ++irv) {
            size_t grid_group_path_pos = ((*irv)->newname.substr(1)).find_first_of("/");
            objnewname =  ((*irv)->newname).substr(grid_group_path_pos+2);
            (*irv)->newname = objnewname;
        }
    }
}

// Adjust object names for MeaSUREs OZone
void GMFile:: Adjust_Mea_Ozone_Obj_Name()  {

    BESDEBUG("h5", "Coming to Adjust_Mea_Ozone_Obj_Name()"<<endl);
    string objnewname;
    for (vector<Var *>::iterator irv = this->vars.begin();
                   irv != this->vars.end(); ++irv) {
        objnewname =  HDF5CFUtil::obtain_string_after_lastslash((*irv)->newname);
        if (objnewname !="") 
           (*irv)->newname = objnewname;

#if 0
//Just for debugging
for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird !=(*irv)->dims.end();++ird) {
 cerr<<"Ozone dim. name "<<(*ird)->name <<endl;
 cerr<<"Ozone dim. new name "<<(*ird)->newname <<endl;
}
#endif

    }

    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
        objnewname =  HDF5CFUtil::obtain_string_after_lastslash((*irv)->newname);
        if (objnewname !="")
           (*irv)->newname = objnewname;
#if 0
 //Just for debugging
for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
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
    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
             irv != this->cvars.end(); ++irv) {
        (*irv)->newname = get_CF_string((*irv)->newname);

        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                        ird != (*irv)->dims.end(); ++ird) { 
            (*ird)->newname = get_CF_string((*ird)->newname);
        }

        if (true == include_attr) {
            for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                        ira != (*irv)->attrs.end(); ++ira) 
                (*ira)->newname = File::get_CF_string((*ira)->newname);
                
        }

    }

    // Special variables
    for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
                irv != this->spvars.end(); ++irv) {
        (*irv)->newname = get_CF_string((*irv)->newname);

        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                     ird != (*irv)->dims.end(); ++ird) 
            (*ird)->newname = get_CF_string((*ird)->newname);

        if (true == include_attr) {
            for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                        ira != (*irv)->attrs.end(); ++ira) 
                  (*ira)->newname = File::get_CF_string((*ira)->newname);
                
        }
    }

// Just for debugging
#if 0
for (vector<Var*>::iterator irv2 = this->vars.begin();
    irv2 != this->vars.end(); irv2++) {
    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
         ird !=(*irv2)->dims.end(); ird++) {
         cerr<<"Dimension name afet Flatten_Obj_Name "<<(*ird)->newname <<endl;
    }
}
#endif


}

// Rarely object name clashings may occur. This routine makes sure
// all object names are unique.
void GMFile::Handle_Obj_NameClashing(bool include_attr)  {

    BESDEBUG("h5", "GMFile::Coming to Handle_Obj_NameClashing()"<<endl);
    // objnameset will be filled with all object names that we are going to check the name clashing.
    // For example, we want to see if there are any name clashings for all variable names in this file.
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

// Name clashings for coordinate variables
void GMFile::Handle_GMCVar_NameClashing(set<string> &objnameset )  {

    GMHandle_General_NameClashing(objnameset,this->cvars);
}

// Name clashings for special variables(like 64-bit integer variables)
void GMFile::Handle_GMSPVar_NameClashing(set<string> &objnameset )  {

    GMHandle_General_NameClashing(objnameset,this->spvars);
}

// This routine handles attribute name clashings for coordinate variables.
void GMFile::Handle_GMCVar_AttrNameClashing()  {

    BESDEBUG("h5", "Coming to Handle_GMCVar_AttrNameClashing()"<<endl);
    set<string> objnameset;

    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
        irv != this->cvars.end(); ++irv) {
        Handle_General_NameClashing(objnameset,(*irv)->attrs);
        objnameset.clear();
    }
}

// Attribute name clashings for special variables
void GMFile::Handle_GMSPVar_AttrNameClashing()  {

    BESDEBUG("h5", "Coming to Handle_GMSPVar_AttrNameClashing()"<<endl);
    set<string> objnameset;

    for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
        irv != this->spvars.end(); ++irv) {
        Handle_General_NameClashing(objnameset,(*irv)->attrs);
        objnameset.clear();
    }
}

//class T must have member string newname
// The subroutine to handle name clashings,
// it builds up a map from original object names to clashing-free object names.
template<class T> void
GMFile::GMHandle_General_NameClashing(set <string>&objnameset, vector<T*>& objvec) {

    BESDEBUG("h5", "Coming to GMHandle_General_NameClashing()"<<endl);
    pair<set<string>::iterator,bool> setret;
    set<string>::iterator iss;

    vector<string> clashnamelist;
    vector<string>::iterator ivs;

    map<int,int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;

    typename vector<T*>::iterator irv;

    for (irv = objvec.begin();
                irv != objvec.end(); ++irv) {

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
    for (ivs=clashnamelist.begin(); ivs!=clashnamelist.end(); ++ivs) {
        int clash_index = 1;
        string temp_clashname = *ivs +'_';
        HDF5CFUtil::gen_unique_name(temp_clashname,objnameset,clash_index);
        *ivs = temp_clashname;
    }


    // Now go back to the original vector, make it unique.
    for (unsigned int i =0; i <clashnamelist.size(); i++)
        objvec[cl_to_ol[i]]->newname = clashnamelist[i];
     
}

// Handle dimension name clashings
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
    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
             irv !=this->cvars.end(); ++irv) {
        for (vector <Dimension *>:: iterator ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end();++ird) {
            setret = dimnameset.insert((*ird)->name);
            if (true == setret.second) 
                vdims.push_back(*ird); 
        }
    }
    
    // For some cases, dimension names are provided but there are no corresponding coordinate
    // variables. For now, we will assume no such cases.
    // Actually, we find such a case in our fake testsuite. So we need to fix it.
    for(vector<Var *>::iterator irv= this->vars.begin();
        irv != this->vars.end();++irv) {
        for (vector <Dimension *>:: iterator ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end();++ird) {
            //setret = dimnameset.insert((*ird)->newname);
            setret = dimnameset.insert((*ird)->name);
            if (setret.second) vdims.push_back(*ird);
        }
    }

    GMHandle_General_NameClashing(dimnewnameset,vdims);
   
    // Third: Make dimname_to_dimnewname map
    for (vector<Dimension*>::iterator ird = vdims.begin();ird!=vdims.end();++ird) {
        mapret = dimname_to_dimnewname.insert(pair<string,string>((*ird)->name,(*ird)->newname));
        if (false == mapret.second) 
            throw4("The dimension name ",(*ird)->name," should map to ",
                                      (*ird)->newname);
    }

    // Fourth: Change the original dimension new names to the unique dimension new names
    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
        irv !=this->cvars.end(); ++irv)
        for (vector <Dimension *>:: iterator ird = (*irv)->dims.begin();
            ird!=(*irv)->dims.end();++ird) 
           (*ird)->newname = dimname_to_dimnewname[(*ird)->name];

    for (vector<Var *>::iterator irv = this->vars.begin();
         irv != this->vars.end(); ++irv)
        for (vector <Dimension *>:: iterator ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end();++ird) 
           (*ird)->newname = dimname_to_dimnewname[(*ird)->name];

}
    
// For COARDS, dim. names need to be the same as obj. names.
void GMFile::Adjust_Dim_Name() {

    BESDEBUG("h5", "GMFile:Coming to Adjust_Dim_Name()"<<endl);
#if 0
    // Just for debugging
for (vector<Var*>::iterator irv2 = this->vars.begin();
       irv2 != this->vars.end(); irv2++) {
    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
           ird !=(*irv2)->dims.end(); ird++) {
         cerr<<"Dimension new name "<<(*ird)->newname <<endl;
       }
}
#endif

    // Only need for COARD conventions.
    if( true == iscoard) {
        for (vector<GMCVar *>::iterator irv = this->cvars.begin();
             irv !=this->cvars.end(); ++irv) {
#if 0
cerr<<"1D Cvariable name is "<<(*irv)->name <<endl;
cerr<<"1D Cvariable new name is "<<(*irv)->newname <<endl;
cerr<<"1D Cvariable dim name is "<<((*irv)->dims)[0]->name <<endl;
cerr<<"1D Cvariable dim new name is "<<((*irv)->dims)[0]->newname <<endl;
#endif
            if ((*irv)->dims.size()!=1) 
                throw3("Coard coordinate variable ",(*irv)->name, "is not 1D");
            if ((*irv)->newname != (((*irv)->dims)[0]->newname)) {
                ((*irv)->dims)[0]->newname = (*irv)->newname;

                // For all variables that have this dimension,the dimension newname should also change.
                for (vector<Var*>::iterator irv2 = this->vars.begin();
                    irv2 != this->vars.end(); ++irv2) {
                    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
                        ird !=(*irv2)->dims.end(); ++ird) {
                        // This is the key, the dimension name of this dimension 
                        // should be equal to the dimension name of the coordinate variable.
                        // Then the dimension name matches and the dimension name should be changed to
                        // the new dimension name.
                        if ((*ird)->name == ((*irv)->dims)[0]->name) 
                            (*ird)->newname = ((*irv)->dims)[0]->newname;
                    }
                }
            } // if ((*irv)->newname != (((*irv)->dims)[0]->newname))
        }// for (vector<GMCVar *>::iterator irv = this->cvars.begin(); ...
   } // if( true == iscoard) 

// Just for debugging
#if 0
for (vector<Var*>::iterator irv2 = this->vars.begin();
    irv2 != this->vars.end(); irv2++) {
    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
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

       if(true == add_path) {
            // Adding variable original name(origname) and full path(fullpath)
            for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
                if (((*irv)->cvartype == CV_EXIST) || ((*irv)->cvartype == CV_MODIFY)) {
                    Attribute * attr = new Attribute();
                    const string varname = (*irv)->name;
                    const string attrname = "origname";
                    Add_Str_Attr(attr,attrname,varname);
                    (*irv)->attrs.push_back(attr);
                }
            }

            for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                    irv != this->cvars.end(); ++irv) {
                if (((*irv)->cvartype == CV_EXIST) || ((*irv)->cvartype == CV_MODIFY)) {
                    Attribute * attr = new Attribute();
                    const string varname = (*irv)->fullpath;
                    const string attrname = "fullnamepath";
                    Add_Str_Attr(attr,attrname,varname);
                    (*irv)->attrs.push_back(attr);
                }
            }

            for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
                    irv != this->spvars.end(); ++irv) {
                Attribute * attr = new Attribute();
                const string varname = (*irv)->name;
                const string attrname = "origname";
                Add_Str_Attr(attr,attrname,varname);
                (*irv)->attrs.push_back(attr);
            }

            for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
                irv != this->spvars.end(); ++irv) {
                Attribute * attr = new Attribute();
                const string varname = (*irv)->fullpath;
                const string attrname = "fullnamepath";
                Add_Str_Attr(attr,attrname,varname);
                (*irv)->attrs.push_back(attr);
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
    vector<HDF5CF::Var *>::const_iterator it_v;
    vector<HDF5CF::Attribute *>::const_iterator ira;
    const string attr_name_be_replaced = "CodeMissingValue";
    const string attr_new_name = "_FillValue";
    const string attr2_name_be_replaced = "Units";
    const string attr2_new_name ="units";

    // Need to convert String type CodeMissingValue to the corresponding _FilLValue
    // Create a function at HDF5CF.cc. use strtod,strtof,strtol etc. function to convert
    // string to the corresponding type.
    for (it_v = vars.begin(); it_v != vars.end(); ++it_v) {
        bool has_fvalue_attr = false;
        for(ira = (*it_v)->attrs.begin(); ira!= (*it_v)->attrs.end();ira++) {
            if(attr_new_name == (*ira)->name) { 
                has_fvalue_attr = true;
                break;
            }
        }

        if(false == has_fvalue_attr) {
            for(ira = (*it_v)->attrs.begin(); ira!= (*it_v)->attrs.end();ira++) {
                if(attr_name_be_replaced == (*ira)->name) { 
                    if((*ira)->dtype == H5FSTRING) 
                        Change_Attr_One_Str_to_Others((*ira),(*it_v));
                    (*ira)->name = attr_new_name;
                    (*ira)->newname = attr_new_name;
                }
            }
        }

    }
     
    
    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
        bool has_fvalue_attr = false;

        for(ira = (*irv)->attrs.begin(); ira!= (*irv)->attrs.end();ira++) {

            if(attr_new_name == (*ira)->name) {
                has_fvalue_attr = true;
                break;
            }
        }
        if(false == has_fvalue_attr) {
            for(ira = (*irv)->attrs.begin(); ira!= (*irv)->attrs.end();ira++) {

                if(attr_name_be_replaced == (*ira)->name) {
                    if((*ira)->dtype == H5FSTRING) 
                        Change_Attr_One_Str_to_Others((*ira),(*irv));
                    (*ira)->name = attr_new_name;
                    (*ira)->newname = attr_new_name;
                    break;
                }
            }
        }
        
    
        if(product_type == GPM_L1) {

            if ((*irv)->cvartype == CV_EXIST) {
                if((*irv)->name.find("Latitude") !=string::npos) {
                    string unit_value = "degrees_north";
                    Correct_GPM_L1_LatLon_units(*irv,unit_value);

                }
                else if((*irv)->name.find("Longitude") !=string::npos) {
                    string unit_value = "degrees_east";
                    Correct_GPM_L1_LatLon_units(*irv,unit_value);
                }
            } 
        

            else if ((*irv)->cvartype == CV_NONLATLON_MISS) {
            
            string comment;
            const string attrname = "comment";
            Attribute*attr = new Attribute();

            {
            if((*irv)->name == "nchannel1") 
                comment = "Number of Swath S1 channels (10V 10H 19V 19H 23V 37V 37H 89V 89H).";
            else if((*irv)->name == "nchannel2") 
                comment = "Number of Swath S2 channels (166V 166H 183+/-3V 183+/-8V).";
            else if((*irv)->name == "nchan1")
                comment = "Number of channels in Swath 1.";
            else if((*irv)->name == "nchan2")
                comment = "Number of channels in Swath 2.";
            else if((*irv)->name == "VH") 
                comment = "Number of polarizations.";
            else if((*irv)->name == "GMIxyz") 
                comment = "x, y, z components in GMI instrument coordinate system.";
            else if((*irv)->name == "LNL")
                comment = "Linear and non-linear.";
            else if((*irv)->name == "nscan")
                comment = "Number of scans in the granule.";
            else if((*irv)->name == "nscan1")
                comment = "Typical number of Swath S1 scans in the granule.";
            else if((*irv)->name == "nscan2")
                comment = "Typical number of Swath S2 scans in the granule.";
            else if((*irv)->name == "npixelev")
                comment = "Number of earth view pixels in one scan.";
            else if((*irv)->name == "npixelht")
                comment = "Number of hot load pixels in one scan.";
            else if((*irv)->name == "npixelcs")
                comment = "Number of cold sky pixels in one scan.";
            else if((*irv)->name == "npixelfr")
                comment = "Number of full rotation earth view pixels in one scan.";
            else if((*irv)->name == "nfreq1")
                comment = "Number of frequencies in Swath 1.";
            else if((*irv)->name == "nfreq2")
                comment = "Number of frequencies in Swath 2.";
            else if((*irv)->name == "npix1")
                comment = "Number of pixels in Swath 1.";
            else if((*irv)->name == "npix2")
                comment = "Number of pixels in Swath 2.";
            else if((*irv)->name == "npix3")
                comment = "Number of pixels in Swath 3.";
            else if((*irv)->name == "npix4")
                comment = "Number of pixels in Swath 4.";
            else if((*irv)->name == "ncolds1")
                comment = "Maximum number of cold samples in Swath 1.";
            else if((*irv)->name == "ncolds2")
                comment = "Maximum number of cold samples in Swath 2.";
            else if((*irv)->name == "nhots1")
                comment = "Maximum number of hot samples in Swath 1.";
            else if((*irv)->name == "nhots2")
                comment = "Maximum number of hot samples in Swath 2.";
            else if((*irv)->name == "ntherm")
                comment = "Number of hot load thermisters.";
            else if((*irv)->name == "ntach")
                comment = "Number of tachometer readings.";
            else if((*irv)->name == "nsamt"){
                comment = "Number of sample types. ";
                comment = +"The types are: total science GSDR, earthview,hot load, cold sky.";
            }
            else if((*irv)->name == "nndiode")
                comment = "Number of noise diodes.";
            else if((*irv)->name == "n7")
                comment = "Number seven.";
            else if((*irv)->name == "nray")
                comment = "Number of angle bins in each NS scan."; 
            else if((*irv)->name == "nrayMS")
                comment = "Number of angle bins in each MS scan."; 
            else if((*irv)->name == "nrayHS")
                comment = "Number of angle bins in each HS scan."; 
            else if((*irv)->name == "nbin")
                comment = "Number of range bins in each NS and MS ray. Bin interval is 125m."; 
            else if((*irv)->name == "nbinHS")
                comment = "Number of range bins in each HS ray. Bin interval is 250m."; 
            else if((*irv)->name == "nbinSZP")
                comment = "Number of range bins for sigmaZeroProfile."; 
            else if((*irv)->name == "nbinSZPHS")
                comment = "Number of range bins for sigmaZeroProfile in each HS scan."; 
            else if((*irv)->name == "nNP")
                comment = "Number of NP kinds."; 
            else if((*irv)->name == "nearFar")
                comment = "Near reference, Far reference."; 
            else if((*irv)->name == "foreBack")
                comment = "Forward, Backward."; 
            else if((*irv)->name == "method")
                comment = "Number of SRT methods."; 
             else if((*irv)->name == "nNode")
                comment = "Number of binNode."; 
              else if((*irv)->name == "nDSD")
                comment = "Number of DSD parameters. Parameters are N0 and D0"; 
           else if((*irv)->name == "LS")
                comment = "Liquid, solid."; 
            }

            if(""==comment)
                delete attr;
            else {
                Add_Str_Attr(attr,attrname,comment);
                (*irv)->attrs.push_back(attr);
            }

        }
      }

      if(product_type == GPMS_L3 || product_type == GPMM_L3) {
        if ((*irv)->cvartype == CV_NONLATLON_MISS) {
            
            string comment;
            const string attrname = "comment";
            Attribute*attr = new Attribute();

            {
            if((*irv)->name == "chn") 
                comment = "Number of channels:Ku,Ka,KaHS,DPR.";
            else if((*irv)->name == "inst") 
                comment = "Number of instruments:Ku,Ka,KaHS.";
            else if((*irv)->name == "tim")
                comment = "Number of hours(local time).";
            else if((*irv)->name == "ang"){
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
            else if((*irv)->name == "rt") 
                comment = "Number of rain types: stratiform, convective,all.";
            else if((*irv)->name == "st") 
                comment = "Number of surface types:ocean,land,all.";
            else if((*irv)->name == "bin"){
                comment = "Number of bins in histogram. The thresholds are different for different";
                comment +=" variables. see the file specification for this algorithm.";
            }
            else if((*irv)->name == "nvar") {
                comment = "Number of phase bins. Bins are counts of phase less than 100, ";
                comment +="counts of phase greater than or equal to 100 and less than 200, ";
                comment +="counts of phase greater than or equal to 200.";
            }
            else if((*irv)->name == "AD")
                comment = "Ascending or descending half of the orbit.";
            }

            if(""==comment)
                delete attr;
            else {
                Add_Str_Attr(attr,attrname,comment);
                (*irv)->attrs.push_back(attr);
            }

        }
      }


      if ((*irv)->cvartype == CV_SPECIAL) {
            if((*irv)->name == "nlayer" || (*irv)->name == "hgt"
               || (*irv)->name == "nalt") {
                Attribute*attr = new Attribute();
                string unit_value = "km";
                Add_Str_Attr(attr,attr2_new_name,unit_value);
                (*irv)->attrs.push_back(attr);

                Attribute*attr1 = new Attribute();
                string attr1_axis="axis";
                string attr1_value = "Z";
                Add_Str_Attr(attr1,attr1_axis,attr1_value);
                (*irv)->attrs.push_back(attr1);

                Attribute*attr2 = new Attribute();
                string attr2_positive="positive";
                string attr2_value = "up";
                Add_Str_Attr(attr2,attr2_positive,attr2_value);
                (*irv)->attrs.push_back(attr2);

            }
            if((*irv)->name == "hgt" || (*irv)->name == "nalt"){
                Attribute*attr1 = new Attribute();
                string comment ="Number of heights above the earth ellipsoid";
                Add_Str_Attr(attr1,"comment",comment);
                (*irv)->attrs.push_back(attr1);
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
                    float _FillValue = -9999.9;
                    Add_One_Float_Attr(attr,fill_value_attr_name,_FillValue);
               (*it_v)->attrs.push_back(attr);
            }
        }
    }// for (it_v = vars.begin(); ...
#endif

}

// For GPM level 1 data, var must have names that contains either "Latitude" nor "Longitude".
void 
GMFile:: Correct_GPM_L1_LatLon_units(Var *var, const string unit_value)  {

    BESDEBUG("h5", "Coming to Correct_GPM_L1_LatLon_units()"<<endl);
    const string Unit_name = "Units";
    const string unit_name = "units";

    vector<HDF5CF::Attribute *>::iterator ira;
    
    // Delete "units"  and "Units"
    for(ira = var->attrs.begin(); ira!= var->attrs.end();) {
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
    // rather than degrees_north.. So units also needs to be corrected to follow CF.
    Attribute *attr = new Attribute();
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
    string longname_value;
    

    const string orig_units_attr_name = "Units";
    const string units_attr_name = "units";
    string units_value;
    
    const string orig_valid_min_attr_name = "Data Minimum";
    const string valid_min_attr_name = "valid_min";
    float valid_min_value = 0;

    const string orig_valid_max_attr_name = "Data Maximum";
    const string valid_max_attr_name = "valid_max";
    float valid_max_value = 0;

    // The fill value is -32767.0. However, No _FillValue attribute is added.
    // So add it here. KY 2012-2-16
 
    const string fill_value_attr_name = "_FillValue";
    float _FillValue = -32767.0;

    for (ira = this->root_attrs.begin(); ira != this->root_attrs.end(); ++ira) {
        if (orig_longname_attr_name == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            longname_value.resize((*ira)->value.size());
            copy((*ira)->value.begin(),(*ira)->value.end(),longname_value.begin());

        }
        else if (orig_units_attr_name == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            units_value.resize((*ira)->value.size());
            copy((*ira)->value.begin(),(*ira)->value.end(),units_value.begin());

        }
        else if (orig_valid_min_attr_name == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            memcpy(&valid_min_value,(void*)(&((*ira)->value[0])),(*ira)->value.size());
        }

        else if (orig_valid_max_attr_name == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            memcpy(&valid_max_value,(void*)(&((*ira)->value[0])),(*ira)->value.size());
        }
        
    }// for (ira = this->root_attrs.begin(); ira != this->root_attrs.end(); ++ira)

    // New version Aqu(Q20112132011243.L3m_MO_SCI_V3.0_SSS_1deg.bz2) files seem to have CF attributes added. 
    // In this case, we should not add extra CF attributes, or duplicate values may appear. KY 2015-06-20
    bool has_long_name = false;
    bool has_units = false;
    bool has_valid_min = false;
    bool has_valid_max = false;
    bool has_fillvalue = false;

    for (it_v = vars.begin();
                it_v != vars.end(); ++it_v) {
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
    } // for (it_v = vars.begin(); ...


    // Level 3 variable name is l3m_data
    for (it_v = vars.begin();
                it_v != vars.end(); ++it_v) {
        if ("l3m_data" == (*it_v)->name) {

            Attribute *attr = NULL;
            // 1. Add the long_name attribute if no
            if(false == has_long_name) {
                attr = new Attribute();
                Add_Str_Attr(attr,longname_attr_name,longname_value);
                (*it_v)->attrs.push_back(attr);
            }

            // 2. Add the units attribute
            if(false == has_units) {
                attr = new Attribute();
                Add_Str_Attr(attr,units_attr_name,units_value);
                (*it_v)->attrs.push_back(attr);
            }

            // 3. Add the valid_min attribute
            if(false == has_valid_min) {
                attr = new Attribute();
                Add_One_Float_Attr(attr,valid_min_attr_name,valid_min_value);
                (*it_v)->attrs.push_back(attr);
            }

            // 4. Add the valid_max attribute
            if(false == has_valid_max) {
                attr = new Attribute();
                Add_One_Float_Attr(attr,valid_max_attr_name,valid_max_value);
                (*it_v)->attrs.push_back(attr);
            }

            // 5. Add the _FillValue attribute
            if(false == has_fillvalue) {
               attr = new Attribute();
               Add_One_Float_Attr(attr,fill_value_attr_name,_FillValue);
               (*it_v)->attrs.push_back(attr);
            }

            break;
        }
    } // for (it_v = vars.begin(); ...
}

// Add SeaWiFS attributes
void 
GMFile:: Add_SeaWiFS_Attrs()  {

    BESDEBUG("h5", "Coming to Add_SeaWiFS_Attrs()"<<endl);
    // The fill value is -999.0. However, No _FillValue attribute is added.
    // So add it here. KY 2012-2-16
    const string fill_value_attr_name = "_FillValue";
    float _FillValue = -999.0;
    const string valid_range_attr_name = "valid_range";
    vector<HDF5CF::Var *>::const_iterator it_v;
    vector<HDF5CF::Attribute *>::const_iterator ira;


    for (it_v = vars.begin();
                it_v != vars.end(); ++it_v) {
        if (H5FLOAT32 == (*it_v)->dtype) {
            bool has_fillvalue = false;
            bool has_validrange = false;
            for(ira = (*it_v)->attrs.begin(); ira!= (*it_v)->attrs.end();ira++) {
                if (fill_value_attr_name == (*ira)->name){
                    has_fillvalue = true;
                    break;
                }

                else if(valid_range_attr_name == (*ira)->name) {
                    has_validrange = true;
                    break;
                }

            }
            // Add the fill value
            if (has_fillvalue != true && has_validrange != true ) {
                Attribute* attr = new Attribute();
                Add_One_Float_Attr(attr,fill_value_attr_name,_FillValue);
                (*it_v)->attrs.push_back(attr);
            }
        }// if (H5FLOAT32 == (*it_v)->dtype)
    }// for (it_v = vars.begin(); ...
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

    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
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
    } // for (vector<GMCVar *>::iterator ircv = this->cvars.begin(); ...
   
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
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
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
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
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
 
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

            bool coor_attr_keep_exist = false;

            // May need to delete only the "coordinates" with both 2-D lat/lon dim. KY 2015-12-07
            if(((*irv)->rank >=2)) { 
                
                short ll2dim_flag = 0;
                for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
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
                    for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin();
                        ira !=(*irv)->attrs.end();) {
                        if ((*ira)->newname == co_attrname) {
                            delete (*ira);
                            ira = (*irv)->attrs.erase(ira);
                        }
                        else {
                            ++ira;
                        }
                    }// for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin(); ...
                
                    // Generate the "coordinates" attribute only for variables that have both 2-D lat/lon dim. names.
                    for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                        ird != (*irv)->dims.end(); ++ ird) {
                        for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
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
                } // for (vector<Var *>::iterator irv = this->vars.begin(); ...
            }
        }
    }
}
#endif


// Handle the "coordinates" and "units" attributes of coordinate variables.
void GMFile:: Handle_Coor_Attr() {

    BESDEBUG("h5", "GMFile::Coming to Handle_Coor_Attr()"<<endl);
    string co_attrname = "coordinates";
    string co_attrvalue="";
    string unit_attrname = "units";
    string nonll_unit_attrvalue ="level";
    string lat_unit_attrvalue ="degrees_north";
    string lon_unit_attrvalue ="degrees_east";

    // Attribute units should be added for coordinate variables that
    // have the type CV_NONLATLON_MISS,CV_LAT_MISS and CV_LON_MISS.
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
        ircv != this->cvars.end(); ++ircv) {

        if ((*ircv)->cvartype == CV_NONLATLON_MISS) {
           Attribute * attr = new Attribute();
           Add_Str_Attr(attr,unit_attrname,nonll_unit_attrvalue);
           (*ircv)->attrs.push_back(attr);
        }
        else if ((*ircv)->cvartype == CV_LAT_MISS) {
           Attribute * attr = new Attribute();
           Add_Str_Attr(attr,unit_attrname,lat_unit_attrvalue);
           (*ircv)->attrs.push_back(attr);
        }
        else if ((*ircv)->cvartype == CV_LON_MISS) {
           Attribute * attr = new Attribute();
           Add_Str_Attr(attr,unit_attrname,lon_unit_attrvalue);
           (*ircv)->attrs.push_back(attr);
        }
    } // for (vector<GMCVar *>::iterator ircv = this->cvars.begin(); ...
   
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
        if(grp_cv_paths.size() >0) {
            for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
                if(grp_cv_paths.find(HDF5CFUtil::obtain_string_before_lastslash((*irv)->fullpath)) != grp_cv_paths.end()){ 

                    // Check the "coordinates" attribute and flatten the values. 
                    Flatten_VarPath_In_Coordinates_Attr(*irv);
                }
            }
        }
        return;
    }
   
    // Now handle the 2-D lat/lon case
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
        ircv != this->cvars.end(); ++ircv) {

        if((*ircv)->rank == 2 && (*ircv)->cvartype == CV_EXIST) {

            //Note: When the 2nd parameter is true in the function Is_geolatlon, it checks the lat/latitude/Latitude 
            //      When the 2nd parameter is false in the function Is_geolatlon, it checks the lon/longitude/Longitude
            // The following code makes sure that the replacement only happens with the general 2-D lat/lon case.
            // The following code is commented out since we find an OMPS-NPP case that has the no-CF unit for
            // "Latitude". So just to check the latitude and longitude and if the units are not CF-compliant, 
            // change them. KY 2020-02-27
#if 0
            if(gp_latname == (*ircv)->name) { 
                // Only if gp_latname is not lat/latitude/Latitude, change the units
                if(false == Is_geolatlon(gp_latname,true)) 
                    Replace_Var_Str_Attr((*ircv),unit_attrname,lat_unit_attrvalue);
            }
            else if(gp_lonname ==(*ircv)->name) { 
                // Only if gp_lonname is not lon/longitude/Longitude, change the units
                if(false == Is_geolatlon(gp_lonname,false))
                    Replace_Var_Str_Attr((*ircv),unit_attrname,lon_unit_attrvalue);
            }
#endif

            // We meet several products that miss the 2-D latitude and longitude CF units although they
            // have the CV names like latitude/longitude, we should double check this case,
            // and add the correct CF units if possible. We will watch if this is the right way.
            //else if(true == Is_geolatlon((*ircv)->name,true))
            if(true == Is_geolatlon((*ircv)->name,true))
                Replace_Var_Str_Attr((*ircv),unit_attrname,lat_unit_attrvalue);

            else if(true == Is_geolatlon((*ircv)->name,false))
                Replace_Var_Str_Attr((*ircv),unit_attrname,lon_unit_attrvalue);
        }
    } // for (vector<GMCVar *>::iterator ircv = this->cvars.begin()
    
    // If we find that there are groups that we should check the coordinates attribute of the variable under, 
    // we should flatten the path inside the coordinates. Note this is for 2D-latlon CV netCDF-4-like case.
    if(grp_cv_paths.size() >0) {
        for (vector<Var *>::iterator irv = this->vars.begin();
            irv != this->vars.end(); ++irv) {
            if(grp_cv_paths.find(HDF5CFUtil::obtain_string_before_lastslash((*irv)->fullpath)) != grp_cv_paths.end()){ 

                // Check the "coordinates" attribute and flatten the values. 
                Flatten_VarPath_In_Coordinates_Attr(*irv);
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
        for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
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
        for (vector<Var *>::iterator irv = this->vars.begin();
                                irv != this->vars.end(); ++irv) {
            if((*irv)->rank >=2){
                for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin(); ira !=(*irv)->attrs.end();++ira) {
                    // We will check if we have the coordinate attribute 
                    if((*ira)->name == co_attrname) {
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
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

            bool has_coor = false;
            for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin(); ira !=(*irv)->attrs.end();++ira) {
                // We will check if we have the coordinate attribute 
                if((*ira)->name == co_attrname) {
                    has_coor = true;
                    break;
                }
            }
 
            // The coordinates attribute is flattened by force.
            if(true == force_flatten_coor_attr && true == has_coor) { 
#if 0
                if(is_hybrid_eos5 == true) {
                    Flatten_VarPath_In_Coordinates_Attr_EOS5((*irv));
                }
                else 
#endif
                    Flatten_VarPath_In_Coordinates_Attr((*irv));
            }
            
            else if(((*irv)->rank >=2) && (has_coor_attr_ge_2d_vars == false || false == force_flatten_coor_attr)) { 
               
                bool coor_attr_keep_exist = false;

                // Check if this var is under group_cv_paths, no, then check if this var's dims are the same as the dims of 2-D CVars 
                if(grp_cv_paths.find(HDF5CFUtil::obtain_string_before_lastslash((*irv)->fullpath)) == grp_cv_paths.end()) 

                    // If finding this var is associated with 2-D lat/lon CVs, not keep the original "coordinates" attribute.
                    coor_attr_keep_exist = Check_Var_2D_CVars(*irv);
                else {
                    coor_attr_keep_exist = true;
                }
                
                // The following two lines are just for old smap level 2 case. 
                if(product_type == OSMAPL2S)
                    coor_attr_keep_exist = true;

                // Need to delete the original "coordinates" and rebuild the "coordinates" if this var is associated with the 2-D lat/lon CVs.
                if (false == coor_attr_keep_exist) {
                    for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin();
                        ira !=(*irv)->attrs.end();) {
                        if ((*ira)->newname == co_attrname) {
                            delete (*ira);
                            ira = (*irv)->attrs.erase(ira);
                        }
                        else {
                            ++ira;
                        }
                    }// for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin(); ...
                
                    // Generate the new "coordinates" attribute. 
                    for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                        ird != (*irv)->dims.end(); ++ ird) {
                        for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
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
                } // for (vector<Var *>::iterator irv = this->vars.begin(); ...
            }
        }
    }
}

// Handle GPM level 1 coordiantes attributes.
void GMFile:: Handle_GPM_l1_Coor_Attr() {

    BESDEBUG("h5", "Coming to Handle_GPM_l1_Coor_Attr()"<<endl);
    // Build a map from CFdimname to 2-D lat/lon variable name, should be something like: aa_list[cfdimname]=s1_latitude .
    // Loop all variables
    // Inner loop: for all dims of a var
    // if(dimname matches the dim(not cfdim) name of one of 2-D lat/lon,
    // check if the variable's full path contains the path of one of 2-D lat/lon,
    // yes, build its cfdimname = path+ dimname, check this cfdimname with the cfdimname of the corresponding 2-D lat/lon
    //      If matched, save this latitude variable name as one of the coordinate variable. 
    //      else this is a 3rd-dimension cv, just use the dimension name(or the corresponding cv name maybe through a map).

    // Prepare 1) 2-D CVar(lat,lon) corresponding dimension name set.
    //         2) cfdim name to cvar name map(don't need to use a map, just a holder. It should be fine. 

    // "coordinates" attribute name and value.  We only need to provide this atttribute for variables that have 2-D lat/lon 
    string co_attrname = "coordinates";
    string co_attrvalue="";

    // 2-D cv dimname set.
    set<string> cvar_2d_dimset;

    pair<map<string,string>::iterator,bool>mapret;

    // Hold the mapping from cfdimname to 2-D cvar name. Something like nscan->lat, npixel->lon 
    map<string,string>cfdimname_to_cvar2dname;

    // Loop through cv variables to build 2-D cv dimname set and the mapping from cfdimname to 2-D cvar name.
    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {

        //This CVar must be 2-D array.
        if((*irv)->rank == 2) { 

//cerr<<"2-D cv name is "<<(*irv)->name <<endl;
//cerr<<"2-D cv new name is "<<(*irv)->newname <<endl;
//cerr<<"(*irv)->cfdimname is "<<(*irv)->cfdimname <<endl;

            for(vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                  ird != (*irv)->dims.end(); ++ird) {
                cvar_2d_dimset.insert((*ird)->name);
            }

            // Generate cfdimname to cvar2d map
            mapret = cfdimname_to_cvar2dname.insert(pair<string,string>((*irv)->cfdimname,(*irv)->newname));      
            if (false == mapret.second)
                throw4("The cf dimension name ",(*irv)->cfdimname," should map to 2-D coordinate variable",
                                      (*irv)->newname);
        }
    }

    // Loop through the variable list to build the coordinates.
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        // Only apply to >2D variables.
        if((*irv)->rank >=2) {

            // The variable dimension names must be found in the 2D cvar dim. nameset.
            // The flag must be at least 2.
            short have_2d_dimnames_flag = 0;
            for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird !=(*irv)->dims.end();++ird) {
                if (cvar_2d_dimset.find((*ird)->name)!=cvar_2d_dimset.end())    
                    have_2d_dimnames_flag++;
            }

            // Final candidates to have 2-D CVar coordinates. 
            if(have_2d_dimnames_flag >=2) {

                // Obtain the variable path
                string var_path;
                if((*irv)->fullpath.size() > (*irv)->name.size()) 
                    var_path=(*irv)->fullpath.substr(0,(*irv)->fullpath.size()-(*irv)->name.size());
                else 
                    throw4("The variable full path ",(*irv)->fullpath," doesn't contain the variable name ",
                                      (*irv)->name);

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
                    // cfdim_path will not be NULL only when the cfdim name is for the 2-D cv var.

                    // Find the correct path,
                    // Note: 
                    // var_path doesn't have to be the same as cfdim_path
                    // consider the variable /a1/a2/foo and the latitude /a1/latitude(cfdimpath is /a1)
                    // If there is no /a1/a2/latitude, the /a1/latitude can be used as the coordinate of /a1/a2/foo.
                    // But we want to check if var_path is the same as cfdim_path first. So we check cfdimname_to_cvarname again.
                    if(var_path == cfdim_path) {
                        for (vector<Dimension*>::iterator ird = (*irv)->dims.begin();
                            ird!=(*irv)->dims.end();++ird) {
                            if(reduced_dimname == (*ird)->name) {
                               cv_2d_flag++;
                               cv_2d_names.push_back(itm->second);
                               cv_2d_dimnames.insert((*ird)->name);
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
                        // cfdim_path will not be NULL only when the cfdim name is for the 2-D cv var.

                        // Find the correct path,
                        // Note: 
                        // var_path doesn't have to be the same as cfdim_path
                        // consider the variable /a1/a2/foo and the latitude /a1/latitude(cfdimpath is /a1)
                        // If there is no /a1/a2/latitude, the /a1/latitude can be used as the coordinate of /a1/a2/foo.
                        // 
                        if(var_path.find(cfdim_path)!=string::npos) {
                            for (vector<Dimension*>::iterator ird = (*irv)->dims.begin();
                                ird!=(*irv)->dims.end();++ird) {
                                if(reduced_dimname == (*ird)->name) {
                                   cv_2d_flag++;
                                   cv_2d_names.push_back(itm->second);
                                   cv_2d_dimnames.insert((*ird)->name);
                                }
                            }
                        }
                    
                    }
                }

                // Now we got all cases.
                if(2 == cv_2d_flag) {

                    // Add latitude and longitude to the 'coordinates' attribute.
                    co_attrvalue = cv_2d_names[0] + " " + cv_2d_names[1];
                    if((*irv)->rank >2) {
                        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                            ird !=(*irv)->dims.end();++ird) {

                            // Add 3rd-dimension to the 'coordinates' attribute.
                            if(cv_2d_dimnames.find((*ird)->name) == cv_2d_dimnames.end())
                                co_attrvalue = co_attrvalue + " " +(*ird)->newname;
                        }
                    }
                    Attribute * attr = new Attribute();
                    Add_Str_Attr(attr,co_attrname,co_attrvalue);
                    (*irv)->attrs.push_back(attr);
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
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        if((*irv)->rank >= 2) {
            for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin(); ira !=(*irv)->attrs.end(); ++ira) { 
                if((*ira)->name == co_attrname) {
                    // If having the coordinates attribute, check if the "coordinates" variables match 2-D lat/lon CV condition,
                    // if yes, flatten the coordinates attribute.
                    string coor_value = Retrieve_Str_Attr_Value(*ira,(*irv)->fullpath);
                    if(Coord_Match_LatLon_NameSize(coor_value) == true) 
                        Flatten_VarPath_In_Coordinates_Attr(*irv);
                    // If the "coordinates" variables don't match the first condition, we can still check
                    // if we can find the corresponding "coordinates" variables that match the names under the same group,
                    // if yes, we add the path to the attribute "coordinates".
                    else if(Coord_Match_LatLon_NameSize_Same_Group(coor_value,HDF5CFUtil::obtain_string_before_lastslash((*irv)->fullpath)) == true) 
                        Add_VarPath_In_Coordinates_Attr(*irv,coor_value);
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
        for(vector<string>::iterator irs=coord_values_vec.begin();irs!=coord_values_vec.end();++irs){
            if(((*irs).find_first_of('/'))!=string::npos) {
                *irs = '/' + (*irs);
            }
        }
    }

    //Loop through all coordinate path stored in the coordinate patch vector,
    for(vector<string>::iterator irs=coord_values_vec.begin();irs!=coord_values_vec.end();++irs){

        // Loop through all the lat/lon pairs generated in the Check_LatLon_With_Coordinate_Attr routine
        // Remember the index and number appeared for both lat and lon.
        for(vector<struct Name_Size_2Pairs>::iterator ivs=latloncv_candidate_pairs.begin(); ivs!=latloncv_candidate_pairs.end();++ivs) {
            if((*irs) == (*ivs).name1){
                match_lat_name_pair_index = distance(latloncv_candidate_pairs.begin(),ivs);
                num_match_lat++;
            }
            else if ((*irs) == (*ivs).name2) {
                match_lon_name_pair_index = distance(latloncv_candidate_pairs.begin(),ivs);
                num_match_lon++;
            }
        }
    }
    //Only when both index and the number of appearence match, we can set this be true.
    if((match_lat_name_pair_index == match_lon_name_pair_index) && (num_match_lat ==1) && (num_match_lon ==1))
        ret_value = true;

    return ret_value;
    
}

//Some products only store the coordinate name(not full path) in the attribute coordinates, as
//long as it is valid, we should add the path to this coordinates.
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
    for(vector<string>::iterator irs=coord_values_vec.begin();irs!=coord_values_vec.end();++irs){
//cerr<<"coordinate values are "<<*irs <<endl;
        for(vector<struct Name_Size_2Pairs>::iterator ivs=latloncv_candidate_pairs.begin(); ivs!=latloncv_candidate_pairs.end();++ivs) {
            string lat_name = HDF5CFUtil::obtain_string_after_lastslash((*ivs).name1);
            string lat_path = HDF5CFUtil::obtain_string_before_lastslash((*ivs).name1);
            string lon_name = HDF5CFUtil::obtain_string_after_lastslash((*ivs).name2);
            string lon_path = HDF5CFUtil::obtain_string_before_lastslash((*ivs).name2);
            if((*irs) == lat_name && lat_path == var_path){
                match_lat_name_pair_index = distance(latloncv_candidate_pairs.begin(),ivs);
                num_match_lat++;
            }
            else if ((*irs) == lon_name && lon_path == var_path) {
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
    Dimension* gmcvar_dim = new Dimension(gmcvar_dimsize);
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
bool GMFile::Is_netCDF_Dimension(Var *var)  {
    
    string netcdf_dim_mark = "This is a netCDF dimension but not a netCDF variable";

    bool is_only_dimension = false;

    for(vector<Attribute *>::iterator ira = var->attrs.begin();
          ira != var->attrs.end();ira++) {

        if ("NAME" == (*ira)->name) {

             Retrieve_H5_Attr_Value(*ira,var->fullpath);
             string name_value;
             name_value.resize((*ira)->value.size());
             copy((*ira)->value.begin(),(*ira)->value.end(),name_value.begin());

             // Compare the attribute "NAME" value with the string netcdf_dim_mark. We only compare the string with the size of netcdf_dim_mark
             if (0 == name_value.compare(0,netcdf_dim_mark.size(),netcdf_dim_mark))
                is_only_dimension = true;
           
            break;
        }
    } // for(vector<Attribute *>::iterator ira = var->attrs.begin(); ...

    return is_only_dimension;
}

// Handle attributes for special variables.
void 
GMFile::Handle_SpVar_Attr()  {

}

bool
GMFile::Is_Hybrid_EOS5() {

    bool has_group_hdfeos  = false;
    bool has_group_hdfeos_info = false;

    // Too costly to check the dataset. 
    // We will just check the attribute under /HDFEOS INFORMATION.

    // First check if the HDFEOS groups are included
    for (vector<Group *>::iterator irg = this->groups.begin();
        irg != this->groups.end(); ++ irg) {
        if ("/HDFEOS" == (*irg)->path) 
            has_group_hdfeos = true;
        else if("/HDFEOS INFORMATION" == (*irg)->path) {
            for(vector<Attribute *>::iterator ira = (*irg)->attrs.begin();
                ira != (*irg)->attrs.end();ira++) {
                if("HDFEOSVersion" == (*ira)->name)
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
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); irv++) {
        string temp_var_name = (*irv)->newname;

        bool remove_eos = Remove_EOS5_Strings(temp_var_name);
        
        if(true == remove_eos)
            (*irv)->newname = get_CF_string(temp_var_name);
        else {//HDFEOS info and extra fields
            string::size_type eos_info_pos = temp_var_name.find(eos_info_str);
            if(eos_info_pos !=string::npos) 
                (*irv)->newname = temp_var_name.erase(eos_info_pos,eos_info_str.size());
            else {// Check the extra fields
                if(Remove_EOS5_Strings_NonEOS_Fields(temp_var_name)==true)
                    (*irv)->newname = get_CF_string(temp_var_name);
            }
        }
    }

    // Now we need to handle the dimension names.
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); irv++) {
        for (vector<Dimension*>::iterator ird = (*irv)->dims.begin();
            ird!=(*irv)->dims.end(); ++ird) {
            string temp_dim_name = (*ird)->newname;
            bool remove_eos = Remove_EOS5_Strings(temp_dim_name);
            
            if(true == remove_eos)
                (*ird)->newname = get_CF_string(temp_dim_name);
            else {//HDFEOS info and extra fields
                string::size_type eos_info_pos = temp_dim_name.find(eos_info_str);
                if(eos_info_pos !=string::npos) 
                    (*ird)->newname = temp_dim_name.erase(eos_info_pos,eos_info_str.size());
                else {// Check the extra fields
                    if(Remove_EOS5_Strings_NonEOS_Fields(temp_dim_name)==true)
                        (*ird)->newname = get_CF_string(temp_dim_name);
                }
            }
        }
    }

    // We have to loop through all CVs.
    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
        string temp_var_name = (*irv)->newname;

        bool remove_eos = Remove_EOS5_Strings(temp_var_name);
        
        if(true == remove_eos)
            (*irv)->newname = get_CF_string(temp_var_name);
        else {//HDFEOS info and extra "fields"
            string::size_type eos_info_pos = temp_var_name.find(eos_info_str);
            if(eos_info_pos !=string::npos) 
                (*irv)->newname = temp_var_name.erase(eos_info_pos,eos_info_str.size());
            else {// Check the extra fields
                if(Remove_EOS5_Strings_NonEOS_Fields(temp_var_name)==true)
                    (*irv)->newname = get_CF_string(temp_var_name);
            }
        }
    }
    // Now we need to handle the dimension names.
    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
        irv != this->cvars.end(); irv++) {
        for (vector<Dimension*>::iterator ird = (*irv)->dims.begin();
            ird!=(*irv)->dims.end(); ++ird) {
            string temp_dim_name = (*ird)->newname;
            bool remove_eos = Remove_EOS5_Strings(temp_dim_name);
            
            if(true == remove_eos)
                (*ird)->newname = get_CF_string(temp_dim_name);
            else {// HDFEOS info and extra "fields"
                string::size_type eos_info_pos = temp_dim_name.find(eos_info_str);
                if(eos_info_pos !=string::npos) 
                    (*ird)->newname = temp_dim_name.erase(eos_info_pos,eos_info_str.size());
                else {// Check the extra "fields"
                    if(Remove_EOS5_Strings_NonEOS_Fields(temp_dim_name)==true)
                        (*ird)->newname = get_CF_string(temp_dim_name);
                }
            }
        }
    }

    // Update the coordinate attribute values
    // We need to remove the HDFEOS special information from the coordinates attributes 
    // since the variable names in the DAP output are already updated.
    for (vector<Var *>::iterator irv = this->vars.begin();
                     irv != this->vars.end(); irv++) {
        for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                     ira != (*irv)->attrs.end();ira++) {
            // We cannot use Retrieve_Str_Attr_value for "coordinates" since "coordinates" may be added by the handler.
            // KY 2017-11-3
            if((*ira)->name == "coordinates") {
                string cor_values((*ira)->value.begin(),(*ira)->value.end()) ;
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
                    (*ira)->value.resize(cor_values.size());
                    (*ira)->fstrsize=cor_values.size();
                    (*ira)->strsize[0] = cor_values.size();
                    copy(cor_values.begin(), cor_values.end(), (*ira)->value.begin());
                }
                
                break;
             }
        }
 
    }
}

// This routine is for handling the hybrid-HDFEOS5 products that have to be treated as "general products"
bool GMFile:: Remove_EOS5_Strings(string &var_name) {

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

bool GMFile:: Remove_EOS5_Strings_NonEOS_Fields(string &var_name) {

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
        for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
                 ircv != this->cvars.end();) {
            if((*ircv)->newname.find("FakeDim")==0) {
                bool var_has_fakedim = false;
                for (vector<Var*>::iterator irv2 = this->vars.begin();
                    irv2 != this->vars.end(); irv2++) {
                    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
                         ird !=(*irv2)->dims.end(); ird++) {
                        if((*ird)->newname == (*ircv)->newname){
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
void GMFile::Rename_NC4_NonCoordVars() {

    if(true == this->have_nc4_non_coord) {
        string nc4_non_coord="_nc4_non_coord_";
        size_t nc4_non_coord_size= nc4_non_coord.size();
        for (vector<Var*>::iterator irv = this->vars.begin();
                 irv != this->vars.end(); irv++) {
            if((*irv)->name.find(nc4_non_coord)==0)
                (*irv)->newname = (*irv)->newname.substr(nc4_non_coord_size,(*irv)->newname.size()-nc4_non_coord_size);
        }
 
        for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
                 ircv != this->cvars.end();++ircv) {
            if((*ircv)->name.find(nc4_non_coord)==0)
                (*ircv)->newname = (*ircv)->newname.substr(nc4_non_coord_size,(*ircv)->newname.size()-nc4_non_coord_size);
        }
    }
 
}
// We will create some temporary coordinate variables. The resource allocoated
// for these variables need to be released.
void 
GMFile::release_standalone_GMCVar_vector(vector<GMCVar*>&tempgc_vars){

    for (vector<GMCVar *>::iterator i = tempgc_vars.begin();
            i != tempgc_vars.end(); ) {
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
        for(vector<Attribute *>::iterator ira = var->attrs.begin();
                     ira != var->attrs.end();ira++) {
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
