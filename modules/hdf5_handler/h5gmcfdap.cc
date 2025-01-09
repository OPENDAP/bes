// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

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

////////////////////////////////////////////////////////////////////////////////
/// \file h5gmcfdap.cc
/// \brief Map and generate DDS and DAS for the CF option for generic HDF5 products 
///
/// 
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>
#include <iostream>

#include <BESDebug.h>
#include <libdap/InternalErr.h>

#include "HDF5RequestHandler.h"
#include "h5cfdaputil.h"
#include "h5gmcfdap.h"
#include "HDF5CFInt8.h"
#include "HDF5CFByte.h"
#include "HDF5CFUInt16.h"
#include "HDF5CFInt16.h"
#include "HDF5CFUInt32.h"
#include "HDF5CFInt32.h"
#include "HDF5CFInt64.h"
#include "HDF5CFUInt64.h"
#include "HDF5CFFloat32.h"
#include "HDF5CFFloat64.h"
#include "HDF5CFStr.h"
#include "HDF5CFArray.h"
#include "HDF5GMCFMissLLArray.h"
#include "HDF5GMCFFillIndexArray.h"
#include "HDF5GMCFMissNonLLCVArray.h"
#include "HDF5GMCFSpecialCVArray.h"
#include "HDF5GMSPCFArray.h"

using namespace std;
using namespace libdap;
using namespace HDF5CF;

// Map general HDF5 products to DAP DDS
void map_gmh5_cfdds(DDS &dds, hid_t file_id, const string& filename){

    BESDEBUG("h5","Coming to GM products DDS mapping function map_gmh5_cfdds()  "<<endl);

    H5GCFProduct product_type = check_product(file_id);

    GMPattern  gproduct_pattern = OTHERGMS;


    auto f_unique = make_unique<GMFile>(filename.c_str(),file_id,product_type,gproduct_pattern);
    auto f = f_unique.get();

    // Generally don't need to handle attributes when handling DDS. 
    bool include_attr = false;
    try {
        // Retrieve all HDF5 info(Not the values)
        f->Retrieve_H5_Info(filename.c_str(),file_id,include_attr);

        // Update product type
        // Newer version of a product may have different layout and the 
        // product type needs to be changed to reflect it. We also want
        // to support the older version in case people still use them. 
        // This routine will check if newer layout can be applied. If yes,
        // update the product type.
        f->Update_Product_Type();

        f->Remove_Unneeded_Objects();

        // Need to add dimension names.
        f->Add_Dim_Name();

        // Handle coordinate variables
        f->Handle_CVar();
#if 0
        // We need to retrieve  coordinate variable attributes for memory cache use.
        //f->Retrieve_H5_CVar_Supported_Attr_Values(); 
        //if((HDF5RequestHandler::get_lrdata_mem_cache() != nullptr) || 
        //   (HDF5RequestHandler::get_srdata_mem_cache() != nullptr)){
        //    f->Retrieve_H5_Supported_Attr_Values();
#endif

        // Handle special variables
        f->Handle_SpVar();

        // When cv memory cache is on, the unit attributes are needed to
        // distinguish whether this is lat/lon. Generally, memory cache 
        // is not used. This snipnet will not be accessed.
        if ((HDF5RequestHandler::get_lrdata_mem_cache() != nullptr) ||
           (HDF5RequestHandler::get_srdata_mem_cache() != nullptr)){

            // Handle unsupported datatypes including the attributes
            f->Handle_Unsupported_Dtype(true);

            // Handle unsupported dataspaces including the attributes
            f->Handle_Unsupported_Dspace(true);

            // We need to retrieve  coordinate variable attributes for memory cache use.
            f->Retrieve_H5_CVar_Supported_Attr_Values(); 

        }
        else {

            // Handle unsupported datatypes
            f->Handle_Unsupported_Dtype(include_attr);

            // Handle unsupported dataspaces
            f->Handle_Unsupported_Dspace(include_attr);

        }

        // Need to handle the "coordinate" attributes when memory cache is turned on.
        if((HDF5RequestHandler::get_lrdata_mem_cache() != nullptr) || 
           (HDF5RequestHandler::get_srdata_mem_cache() != nullptr))
            f->Add_Supplement_Attrs(HDF5RequestHandler::get_add_path_attrs());

        // Adjust object names(may remove redundant paths)
        f->Adjust_Obj_Name();

        // Flatten the object names
        f->Flatten_Obj_Name(include_attr);

        // Handle Object name clashings
        // Only when the check_nameclashing key is turned on or
        // general product.
        if(General_Product == product_type ||
           true == HDF5RequestHandler::get_check_name_clashing()) 
           f->Handle_Obj_NameClashing(include_attr);

        // Adjust Dimension name 
        f->Adjust_Dim_Name();
        if(General_Product == product_type ||
            true == HDF5RequestHandler::get_check_name_clashing()) 
            f->Handle_DimNameClashing();

        f->Handle_Hybrid_EOS5();
        if(true == f->Have_Grid_Mapping_Attrs()) 
            f->Handle_Grid_Mapping_Vars();
        // Need to handle the "coordinate" attributes when memory cache is turned on.
        if((HDF5RequestHandler::get_lrdata_mem_cache() != nullptr) || 
           (HDF5RequestHandler::get_srdata_mem_cache() != nullptr))
            f->Handle_Coor_Attr();
 
        f->Remove_Unused_FakeDimVars();
        f->Rename_NC4_NonCoordVars();
#if 0
        if (f->HaveUnlimitedDim())
            f->Update_NC4_PureDimSize();
#endif
    }
    catch (HDF5CF::Exception &e){
        throw InternalErr(e.what());
    }

    // generate DDS.
    try {
        gen_gmh5_cfdds(dds,f);
    }
    catch(...) {
        throw;
    }

}

// Map general HDF5 products to DAP DAS
void map_gmh5_cfdas(DAS &das, hid_t file_id, const string& filename){

    BESDEBUG("h5","Coming to GM products DAS mapping function map_gmh5_cfdas()  "<<endl);

    H5GCFProduct product_type = check_product(file_id);
    GMPattern gproduct_pattern = OTHERGMS;

    auto f_unique = make_unique<GMFile>(filename.c_str(),file_id,product_type,gproduct_pattern);
    auto f = f_unique.get();

    bool include_attr = true;
    try {
        f->Retrieve_H5_Info(filename.c_str(),file_id,include_attr);

        // Update product type(check comments of map_gmh5_cfdds)
        f->Update_Product_Type();

        f->Remove_Unneeded_Objects();

        f->Add_Dim_Name();
        f->Handle_CVar();
        f->Handle_SpVar();
        f->Handle_Unsupported_Dtype(include_attr);

        // Remove unsupported dataspace 
        f->Handle_Unsupported_Dspace(include_attr);

        // Need to retrieve the attribute values to feed DAS
        f->Retrieve_H5_Supported_Attr_Values();

        // Handle other unsupported objects,
        // currently it mainly generates the info. for the
        // unsupported objects other than datatype, dataspace,links and named datatype
        // One area is maybe very long string. So we retrieve the attribute
        // values before calling this function.
        f->Handle_Unsupported_Others(include_attr);


        // Need to add original variable name and path
        // and other special attributes
        // Can be turned on/off by using the check_path_attrs keys.
        f->Add_Supplement_Attrs(HDF5RequestHandler::get_add_path_attrs());
        f->Adjust_Obj_Name();
        f->Flatten_Obj_Name(include_attr);
        if(General_Product == product_type ||
           true == HDF5RequestHandler::get_check_name_clashing()) 
            f->Handle_Obj_NameClashing(include_attr);
        if(f->HaveUnlimitedDim() == true) 
            f->Adjust_Dim_Name();
        // Handle the "coordinate" attributes.
        if (f->is_special_gpm_l3()==false)
            f->Handle_Coor_Attr();

        f->Handle_Hybrid_EOS5();
        if(true == f->Have_Grid_Mapping_Attrs()) 
            f->Handle_Grid_Mapping_Vars();

        f->Remove_Unused_FakeDimVars();

        f->Rename_NC4_NonCoordVars();

        if(true == HDF5RequestHandler::get_enable_coord_attr_add_path() && f->is_special_gpm_l3()==false)
            f->Add_Path_Coord_Attr();

        f->Update_Bounds_Attr();
    }
    catch (HDF5CF::Exception &e){
        throw InternalErr(e.what());
    }

    // Generate the DAS attributes.
    try {
        gen_gmh5_cfdas(das,f);
    }   
    catch (...) {
        throw;
    }

}


void map_gmh5_cfdmr(D4Group *d4_root, hid_t file_id, const string& filename){

    BESDEBUG("h5","Coming to GM products DMR mapping function map_gmh5_cfdmr()  "<<endl);

    H5GCFProduct product_type = check_product(file_id);

    GMPattern  gproduct_pattern = OTHERGMS;

    auto f_unique = make_unique<GMFile>(filename.c_str(),file_id,product_type,gproduct_pattern);
    auto f = f_unique.get();

    //  Both variables and attributes are in DMR.
    bool include_attr = true;
    try {

        // Set the is_dap4 flag be true.
        f->setDap4(true);

        // Retrieve all HDF5 info(Not the values)
        f->Retrieve_H5_Info(filename.c_str(),file_id,include_attr);

        // Update product type
        // Newer version of a product may have different layout and the 
        // product type needs to be changed to reflect it. We also want
        // to support the older version in case people still use them. 
        // This routine will check if newer layout can be applied. If yes,
        // update the product type.
        f->Update_Product_Type();

        f->Remove_Unneeded_Objects();

        // Need to add dimension names.
        f->Add_Dim_Name();

        // Handle coordinate variables
        f->Handle_CVar();
#if 0
        // We need to retrieve  coordinate variable attributes for memory cache use.
        //f->Retrieve_H5_CVar_Supported_Attr_Values(); 
        //if((HDF5RequestHandler::get_lrdata_mem_cache() != nullptr) || 
        //   (HDF5RequestHandler::get_srdata_mem_cache() != nullptr)){
        //    f->Retrieve_H5_Supported_Attr_Values();
#endif

        // Handle special variables
        f->Handle_SpVar();

        // Handle unsupported datatypes including the attributes
        f->Handle_Unsupported_Dtype(true);

        // Handle unsupported dataspaces including the attributes
        f->Handle_Unsupported_Dspace(true);

        // We need to retrieve variable attributes.
        f->Retrieve_H5_Supported_Attr_Values(); 

        // Include handling internal netCDF-4 attributes
        f->Handle_Unsupported_Others(include_attr);

        // Need to handle the "coordinate" attributes 
        f->Add_Supplement_Attrs(HDF5RequestHandler::get_add_path_attrs());

        // Adjust object names(may remove redundant paths)
        f->Adjust_Obj_Name();

        // Flatten the object names
        f->Flatten_Obj_Name(include_attr);

        // Handle Object name clashings
        // Only when the check_nameclashing key is turned on or
        // general product.
        if (General_Product == product_type ||
           true == HDF5RequestHandler::get_check_name_clashing()) 
           f->Handle_Obj_NameClashing(include_attr);

        // Adjust Dimension name, CHECK: the das generation has a f->HaveUnlimitedDim() condition
        f->Adjust_Dim_Name();
        if (General_Product == product_type ||
            true == HDF5RequestHandler::get_check_name_clashing()) 
            f->Handle_DimNameClashing();

        // Handle the "coordinate" attributes.
        f->Handle_Coor_Attr();
       
        f->Handle_Hybrid_EOS5();
        if(true == f->Have_Grid_Mapping_Attrs()) 
            f->Handle_Grid_Mapping_Vars();
#if 0
        // Need to handle the "coordinate" attributes when memory cache is turned on.
        if((HDF5RequestHandler::get_lrdata_mem_cache() != nullptr) || 
           (HDF5RequestHandler::get_srdata_mem_cache() != nullptr))
            f->Handle_Coor_Attr();
#endif
 
        f->Remove_Unused_FakeDimVars();
        f->Rename_NC4_NonCoordVars();

        if(true == HDF5RequestHandler::get_enable_coord_attr_add_path())
            f->Add_Path_Coord_Attr();

        f->Update_Bounds_Attr();

        if (f->HaveUnlimitedDim()) 
            f->Update_NC4_PureDimSize();

    }
    catch (HDF5CF::Exception &e){
        throw InternalErr(e.what());
    }
    
    // generate DMR.
    try {
        gen_gmh5_cfdmr(d4_root,f);
    }
    catch(...) {
        throw;
    }

}

// Generate DDS mapped from general HDF5 products
void gen_gmh5_cfdds( DDS & dds, HDF5CF:: GMFile *f) {

    BESDEBUG("h5","Coming to GM DDS generation function gen_gmh5_cfdds()  "<<endl);

    const vector<HDF5CF::Var *>&      vars  = f->getVars();
    const vector<HDF5CF::GMCVar *>&  cvars  = f->getCVars();
    const vector<HDF5CF::GMSPVar *>& spvars = f->getSPVars();
    const string filename                   = f->getPath();
    const hid_t fileid                      = f->getFileID();

    // Read Variable info.
    // Since we need to use dds to add das for 64-bit dmr,we need to check if
    // this case includes 64-bit integer variables and this is for dmr response.
    bool dmr_64bit_support = false;
    if (HDF5RequestHandler::get_dmr_long_int()==true &&
        HDF5RequestHandler::get_dmr_64bit_int()!=nullptr) {
        for (const auto &var:vars) {
            if (H5INT64 == var->getType() || H5UINT64 == var->getType()){
                dmr_64bit_support = true;
                break;
            }
        }
    }

    // We need to remove the unsupported attributes.
    if(true == dmr_64bit_support) {
        //STOP: add non-support stuff
        f->Handle_Unsupported_Dtype(true);

        // Remove unsupported dataspace 
        f->Handle_Unsupported_Dspace(true);

    }

    for (auto &var:vars) {
        BESDEBUG("h5","variable full path= "<< var->getFullPath() <<endl);
        // Handle 64-integer DAP4 CF mapping
        if(need_attr_values_for_dap4(var) == true) 
            f->Retrieve_H5_Var_Attr_Values(var);
        gen_dap_onevar_dds(dds,var,fileid, filename);
    }
    for (const auto &cvar:cvars) {
        BESDEBUG("h5","variable full path= "<< cvar->getFullPath() <<endl);
        gen_dap_onegmcvar_dds(dds,cvar,fileid, filename);
    }

    for (const auto &spvar:spvars) {
        BESDEBUG("h5","variable full path= "<< spvar->getFullPath() <<endl);
        gen_dap_onegmspvar_dds(dds,spvar,fileid, filename);
    }

}

// Generate DAS mapped from general HDF5 products
void gen_gmh5_cfdas( DAS & das, HDF5CF:: GMFile *f) {

    BESDEBUG("h5","Coming to GM DAS generation function gen_gmh5_cfdas()  "<<endl);

    // First check if this is for generating the ignored object info.
    if(true == f->Get_IgnoredInfo_Flag()) {
        gen_gmh5_cf_ignored_obj_info(das, f);
        return;
    }

    const vector<HDF5CF::Var *>& vars             = f->getVars();
    const vector<HDF5CF::GMCVar *>& cvars         = f->getCVars();
    const vector<HDF5CF::GMSPVar *>& spvars       = f->getSPVars();
    const vector<HDF5CF::Group *>& grps           = f->getGroups();
    const vector<HDF5CF::Attribute *>& root_attrs = f->getAttributes();


    // Handling the file attributes(attributes under the root group)
    // The table name is "HDF_GLOBAL".

    if (false == root_attrs.empty()) {

        AttrTable *at = das.get_table(FILE_ATTR_TABLE_NAME);
        if (nullptr == at) {
            at = das.add_table(FILE_ATTR_TABLE_NAME, obtain_new_attr_table());
        }
        for (const auto &root_attr:root_attrs) {
            // Check and may update the 64-bit integer attributes in DAP4.
            check_update_int64_attr("",root_attr);
            gen_dap_oneobj_das(at,root_attr,nullptr);
        }
    }

    if (false == grps.empty()) {
        for (const auto &grp:grps) {
            AttrTable *at = das.get_table(grp->getNewName());
            if (nullptr == at) {
                at = das.add_table(grp->getNewName(), obtain_new_attr_table());
            }
            for (const auto &grp_attr:grp->getAttributes()) {
                check_update_int64_attr(grp->getNewName(),grp_attr);
                gen_dap_oneobj_das(at,grp_attr,nullptr);
            }
        }
    }

    for (const auto &var:vars) {

        if (false == (var->getAttributes().empty())) {

            // Skip the 64-bit integer variables. The attribute mapping of 
            // DAP4 CF 64-bit integer variable support
            // has been taken care at the routine gen_dap_onevar_dds() 
            // defined at h5commoncfdap.cc
            if(H5INT64 == var->getType() || H5UINT64 == var->getType()){
               continue;
            }

            AttrTable *at = das.get_table(var->getNewName());
            if (nullptr == at) {
                at = das.add_table(var->getNewName(), obtain_new_attr_table());
            }
            for (const auto &attr:var->getAttributes())
                gen_dap_oneobj_das(at,attr,var);

            // TODO: If a var has integer-64 bit datatype attributes, maybe 
            // we can just keep that attributes(not consistent but 
            // easy to implement) or we have to duplicate all 
            // the var in dmr and delete this var from dds. 
                    
        }

        // GPM needs to be handled in a special way(mostly _FillValue)
        if (GPMS_L3 == f->getProductType() || GPMM_L3 == f->getProductType()
                                          || GPM_L1 == f->getProductType()) 
            update_GPM_special_attrs(das,var,false);
        
    }

    for (const auto &cvar:cvars) {
        if (false == (cvar->getAttributes().empty())) {

            // TODO: Add 64-bit int support for coordinates, this has not been tackled.
            if(H5INT64 == cvar->getType() || H5UINT64 == cvar->getType()){
               continue;
            }

            AttrTable *at = das.get_table(cvar->getNewName());
            if (nullptr == at) {
                at = das.add_table(cvar->getNewName(), obtain_new_attr_table());
            }
            for (const auto &attr:cvar->getAttributes())
                gen_dap_oneobj_das(at,attr,cvar);
                    
        }
        // Though CF doesn't allow _FillValue, still keep it to keep the original form.
        if(GPMS_L3 == f->getProductType() || GPMM_L3 == f->getProductType() 
                                          || GPM_L1 == f->getProductType()) 
            update_GPM_special_attrs(das,cvar,true);

    }

    // Currently the special variables are only limited to the ACOS/OCO2 64-bit integer variables
    for (const auto &spvar:spvars) {
        if (false == (spvar->getAttributes().empty())) {

            AttrTable *at = das.get_table(spvar->getNewName());
            if (nullptr == at)
                at = das.add_table(spvar->getNewName(), obtain_new_attr_table());

            for (const auto &attr:spvar->getAttributes())
                gen_dap_oneobj_das(at,attr,spvar);
        }
    }
       
    // CHECK ALL UNLIMITED DIMENSIONS from the coordinate variables based on the names. 
    if(f->HaveUnlimitedDim() == true) {

        BESDEBUG("h5","Find unlimited dimension in the GM DAS generation function gen_gmh5_cfdas()  "<<endl);

        // Currently there is no way for DAP to present the unlimited dimension info.
        // when there are no dimension names. So don't create DODS_EXTRA even if
        // there is an unlimited dimension in the file. KY 2016-02-18
        if(cvars.empty()==false ){

            // First check if we do have unlimited dimension in the coordinate variables.
            // Since unsupported fakedims are removed, we may not have unlimited dimensions.
            bool still_has_unlimited = false;
            for (const auto &cvar:cvars) {

                // Check unlimited dimension names.
                for (const auto &dim:cvar->getDimensions()) {

                    // Currently we only check one unlimited dimension, which is the most
                    // common case. When receiving the conventions from JG, will add
                    // the support of multi-unlimited dimension. KY 2016-02-09
                    if(dim->HaveUnlimitedDim() == true) {
                        still_has_unlimited = true;
                        break;
                    }
                }
                if (true == still_has_unlimited)
                    break;
            }
 
            if (true == still_has_unlimited) {
                AttrTable* at = das.get_table("DODS_EXTRA");
                if (nullptr == at)
                    at = das.add_table("DODS_EXTRA", obtain_new_attr_table());
             
                string unlimited_names;
    
                for (const auto &cvar:cvars) {

                    // Check unlimited dimension names.
                    for (const auto &dim:cvar->getDimensions()) {
    
                        // Currently we only check one unlimited dimension, which is the most
                        // common case. When receiving the conventions from JG, will add
                        // the support of multi-unlimited dimension. KY 2016-02-09
                        if (dim->HaveUnlimitedDim() == true) {
                            if (unlimited_names=="") {
                               unlimited_names = dim->getNewName();
                               if (at !=nullptr)
                                    at->append_attr("Unlimited_Dimension","String",unlimited_names);
                            }
                            else {
                                if(unlimited_names.rfind(dim->getNewName()) == string::npos) {
                                    unlimited_names = unlimited_names+" "+dim->getNewName();
                                    if(at !=nullptr) 
                                        at->append_attr("Unlimited_Dimension","String",dim->getNewName());
                                }
                            }
                        }// if(dim->HaveUnlimitedDim()
                    }// for (vector<Dimension*>::
                }// for (cvars
            }// if(true == still_has_unlimited)
            
        }//if(cvars.size()>0)
#if 0
        // The following line will generate the string like "Band1 str1 str2".
        //if(unlimited_names!="") 
        //         //   at->append_attr("Unlimited_Dimension","String",unlimited_names);
#endif
    }
}

void gen_gmh5_cfdmr(D4Group* d4_root,const HDF5CF::GMFile *f) {

    BESDEBUG("h5","Coming to GM DDS generation function gen_gmh5_cfdmr()  "<<endl);

    const vector<HDF5CF::Var *>&      vars  = f->getVars();
    const vector<HDF5CF::GMCVar *>&  cvars  = f->getCVars();
    const vector<HDF5CF::GMSPVar *>& spvars = f->getSPVars();
    const string filename                   = f->getPath();
    const hid_t fileid                      = f->getFileID();
    const vector<HDF5CF::Group *>& grps           = f->getGroups();
    const vector<HDF5CF::Attribute *>& root_attrs = f->getAttributes();

    vector<HDF5CF::Var *>::const_iterator       it_v;
    vector<HDF5CF::GMCVar *>::const_iterator   it_cv;

    // Root and low-level group attributes.
    if (false == root_attrs.empty()) {
        for (const auto &root_attr:root_attrs) 
            map_cfh5_grp_attr_to_dap4(d4_root,root_attr);
    }

    // When the DAP4 coverage is turned on, the coordinate variables should be before the variables.
    if (HDF5RequestHandler::get_add_dap4_coverage() == true) {

        for (it_cv = cvars.begin(); it_cv !=cvars.end();++it_cv) {
            BESDEBUG("h5","variable full path= "<< (*it_cv)->getFullPath() <<endl);
            gen_dap_onegmcvar_dmr(d4_root,*it_cv,fileid, filename);
        }
    
        for (it_v = vars.begin(); it_v !=vars.end();++it_v) {
            BESDEBUG("h5","variable full path= "<< (*it_v)->getFullPath() <<endl);
            gen_dap_onevar_dmr(d4_root,*it_v,fileid, filename);
        }
    }
    else {

        // Read Variable info.
        for (it_v = vars.begin(); it_v !=vars.end();++it_v) {
            BESDEBUG("h5","variable full path= "<< (*it_v)->getFullPath() <<endl);
            gen_dap_onevar_dmr(d4_root,*it_v,fileid, filename);
    
        }
        for (it_cv = cvars.begin(); it_cv !=cvars.end();++it_cv) {
            BESDEBUG("h5","variable full path= "<< (*it_cv)->getFullPath() <<endl);
            gen_dap_onegmcvar_dmr(d4_root,*it_cv,fileid, filename);
        }
    }

    // GPM needs to be handled in a special way(mostly _FillValue)
    if(GPMS_L3 == f->getProductType() || GPMM_L3 == f->getProductType() 
                                      || GPM_L1 == f->getProductType())
        update_GPM_special_attrs_cfdmr(d4_root,cvars);

    for (auto it_spv = spvars.begin(); it_spv !=spvars.end();it_spv++) {
        BESDEBUG("h5","variable full path= "<< (*it_spv)->getFullPath() <<endl);
        gen_dap_onegmspvar_dmr(d4_root,*it_spv,fileid, filename);
    }

    // We use the attribute container to store the group attributes.
    if (false == grps.empty()) {

        for (const auto &grp:grps) {

            auto tmp_grp_unique = make_unique<D4Attribute>();
            auto tmp_grp = tmp_grp_unique.release();
            tmp_grp->set_name(grp->getNewName());

            // Make the type as a container
            tmp_grp->set_type(attr_container_c);

            for (const auto &attr: grp->getAttributes()) 
                map_cfh5_attr_container_to_dap4(tmp_grp,attr);

            d4_root->attributes()->add_attribute_nocopy(tmp_grp);
        }
    }

    // CHECK ALL UNLIMITED DIMENSIONS from the coordinate variables based on the names. 
    if(f->HaveUnlimitedDim() == true) {

        BESDEBUG("h5","Find unlimited dimension in the GM DMR generation function gen_gmh5_cfdmr()  "<<endl);

        // Currently there is no way for DAP to present the unlimited dimension info.
        // when there are no dimension names. So don't create DODS_EXTRA even if
        // there is an unlimited dimension in the file. KY 2016-02-18
        if(cvars.empty()==false ){

            // First check if we do have unlimited dimension in the coordinate variables.
            // Since unsupported fakedims are removed, we may not have unlimited dimensions.
            bool still_has_unlimited = false;
            for (const auto &cvar:cvars) {

                // Check unlimited dimension names.
                for (const auto &dim:cvar->getDimensions()) {

                    // Currently we only check one unlimited dimension, which is the most
                    // common case. When receiving the conventions from JG, will add
                    // the support of multi-unlimited dimension. KY 2016-02-09
                    if(dim->HaveUnlimitedDim() == true) {
                        still_has_unlimited = true;
                        break;
                    }
                }
                if(true == still_has_unlimited) 
                    break;
            }
 
            if(true == still_has_unlimited) {
 
                string dods_extra = "DODS_EXTRA";

                // If DODS_EXTRA exists, we will not create the unlimited dimensions. 
                if(d4_root->attributes() != nullptr) {

                // TODO: The following lines cause seg. fault in libdap4, needs to investigate
                //if((d4_root->attributes()->find(dods_extra))==nullptr) 
        
                    string unlimited_dim_names;
        
                    for (const auto &cvar:cvars) {
            
                        // Check unlimited dimension names.
                        for (const auto& dim:cvar->getDimensions()) {
            
                            // Currently we only check one unlimited dimension, which is the most
                            // common case. When receiving the conventions from JG, will add
                            // the support of multi-unlimited dimension. KY 2016-02-09
                            if(dim->HaveUnlimitedDim() == true) {
                                
                                string unlimited_dim_name = dim->getNewName();
                                if(unlimited_dim_names=="") 
                                   unlimited_dim_names = unlimited_dim_name;
                                else {
                                    if(unlimited_dim_names.rfind(unlimited_dim_name) == string::npos) 
                                        unlimited_dim_names = unlimited_dim_names+" "+unlimited_dim_name;
                                }
                            }
                        }
                    }
        
                    if(unlimited_dim_names.empty() == false) {
                        auto unlimited_dim_attr_unique = make_unique<D4Attribute>("Unlimited_Dimension",attr_str_c);
                        auto unlimited_dim_attr = unlimited_dim_attr_unique.release();
                        unlimited_dim_attr->add_value(unlimited_dim_names);
                        auto dods_extra_attr_unique = make_unique<D4Attribute>(dods_extra,attr_container_c);
                        auto dods_extra_attr = dods_extra_attr_unique.release();
                        dods_extra_attr->attributes()->add_attribute_nocopy(unlimited_dim_attr);
                        d4_root->attributes()->add_attribute_nocopy(dods_extra_attr);
                        
                    }
                    else 
                        throw InternalErr(__FILE__, __LINE__, "Unlimited dimension should exist.");  
               }
        
            }
        }
    }

    
    // Add DAP4 Map for coverage 
    if (HDF5RequestHandler::get_add_dap4_coverage() == true) {

        // Obtain the coordinate variable names, these are mapped variables.
        vector <string> cvar_name;
        for (it_cv = cvars.begin(); it_cv !=cvars.end();++it_cv) 
            cvar_name.emplace_back((*it_cv)->getNewName()); 
           
        add_dap4_coverage(d4_root,cvar_name,f->getIsCOARD());
    }

}

// Generate the ignored object info. for the CF option of the general products
void gen_gmh5_cf_ignored_obj_info(DAS &das, HDF5CF::GMFile *f) {

    BESDEBUG("h5","Coming to gen_gmh5_cf_ignored_obj_info()  "<<endl);
    AttrTable *at = das.get_table("Ignored_Object_Info");
    if (nullptr == at)
        at = das.add_table("Ignored_Object_Info", obtain_new_attr_table());

    at->append_attr("Message","String",f->Get_Ignored_Msg());

}

// Generate the DDS for a coordinate variable of the General products
void gen_dap_onegmcvar_dds(DDS &dds,const HDF5CF::GMCVar* cvar, const hid_t file_id, const string & filename) {

    BESDEBUG("h5","Coming to gen_dap_onegmcvar_dds()  "<<endl);

    if(cvar->getType() == H5INT64 || cvar->getType() == H5UINT64)
        return;

    BaseType *bt = nullptr;

    switch(cvar->getType()) {
#define HANDLE_CASE(tid,type)                                  \
        case tid:                                           \
            bt = new (type)(cvar->getNewName(),cvar->getFullPath());  \
            break;

        HANDLE_CASE(H5FLOAT32, HDF5CFFloat32)
        HANDLE_CASE(H5FLOAT64, HDF5CFFloat64)
        HANDLE_CASE(H5CHAR,HDF5CFInt16)
        HANDLE_CASE(H5UCHAR, HDF5CFByte)
        HANDLE_CASE(H5INT16, HDF5CFInt16)
        HANDLE_CASE(H5UINT16, HDF5CFUInt16)
        HANDLE_CASE(H5INT32, HDF5CFInt32)
        HANDLE_CASE(H5UINT32, HDF5CFUInt32)
        HANDLE_CASE(H5FSTRING, Str)
        HANDLE_CASE(H5VSTRING, Str)

        default:
            throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
    }

    if (bt) {

        const vector<HDF5CF::Dimension *>& dims = cvar->getDimensions();
        vector <HDF5CF::Dimension*>:: const_iterator it_d;
        vector <size_t> dimsizes;
        dimsizes.resize(cvar->getRank());
        for(int i = 0; i <cvar->getRank();i++)
            dimsizes[i] = (dims[i])->getSize();

        if (dims.empty()) {
            delete bt;
            throw InternalErr(__FILE__, __LINE__, "the coordinate variable cannot be a scalar");
        }
        switch(cvar->getCVType()) {
            
            case CV_EXIST: 
            {
                HDF5CFArray *ar = nullptr;

                // Need to check if this CV is lat/lon. This is necessary when data memory cache is turned on.
                bool is_latlon = cvar->isLatLon();

                auto ar_unique = make_unique< HDF5CFArray>
                                (cvar->getRank(),
                                file_id,
                                filename,
                                cvar->getType(),
                                dimsizes,
                                cvar->getFullPath(),
                                cvar->getTotalElems(),
                                CV_EXIST,
                                is_latlon,
                                cvar->getCompRatio(),
                                false,
                                cvar->getNewName(),
                                bt);
                ar = ar_unique.get();

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (((*it_d)->getNewName()).empty())
                        ar->append_dim((int)((*it_d)->getSize()));
                    else 
                        ar->append_dim((int)((*it_d)->getSize()), (*it_d)->getNewName());
                }

                dds.add_var(ar);
            }
            break;

            case CV_LAT_MISS:
            case CV_LON_MISS:
            {
                // Using HDF5GMCFMissLLArray
                HDF5GMCFMissLLArray *ar = nullptr;
                auto ar_unique = make_unique<HDF5GMCFMissLLArray>
                                (cvar->getRank(),
                                filename,
                                file_id,
                                cvar->getType(),
                                cvar->getFullPath(),
                                cvar->getPtType(),
                                cvar->getCVType(),
                                cvar->getNewName(),
                                bt);
                ar = ar_unique.get();

                for (it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (((*it_d)->getNewName()).empty())
                        ar->append_dim((int)((*it_d)->getSize()));
                    else 
                        ar->append_dim((int)((*it_d)->getSize()), (*it_d)->getNewName());
                }
                dds.add_var(ar);
            }
            break;

            case CV_NONLATLON_MISS:
            {

                if (cvar->getRank() !=1) {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                }
                auto nelem = (int)((cvar->getDimensions()[0])->getSize());

                HDF5GMCFMissNonLLCVArray *ar = nullptr;

                auto ar_unique = make_unique<HDF5GMCFMissNonLLCVArray>
                                                (cvar->getRank(),
                                                  nelem,
                                                  cvar->getNewName(),
                                                  bt);
                ar = ar_unique.get();

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (((*it_d)->getNewName()).empty())
                        ar->append_dim((int)((*it_d)->getSize()));
                    else 
                        ar->append_dim((int)((*it_d)->getSize()), (*it_d)->getNewName());
                }
                dds.add_var(ar);
            }
            break;

            case CV_FILLINDEX:
            {
                if (cvar->getRank() !=1) {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                }

                HDF5GMCFFillIndexArray *ar = nullptr;

                auto ar_unique = make_unique<HDF5GMCFFillIndexArray>
                                                    (cvar->getRank(),
                                                  cvar->getType(),
                                                  false,
                                                  cvar->getNewName(),
                                                  bt);
                ar = ar_unique.get();

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (((*it_d)->getNewName()).empty())
                        ar->append_dim((int)((*it_d)->getSize()));
                    else 
                        ar->append_dim((int)((*it_d)->getSize()), (*it_d)->getNewName());
                }
                dds.add_var(ar);
            }
            break;

            case CV_SPECIAL:
             {
                // Currently only handle 1-D special CV.
                if (cvar->getRank() !=1) {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of special coordinate variable  must be 1");
                }
                auto nelem = (int)((cvar->getDimensions()[0])->getSize());

                auto ar_unique = make_unique<HDF5GMCFSpecialCVArray>(
                                                cvar->getType(),
                                                nelem,
                                                cvar->getFullPath(),
                                                cvar->getPtType(),
                                                cvar->getNewName(),
                                                bt);
                auto ar = ar_unique.get();

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (((*it_d)->getNewName()).empty())
                        ar->append_dim((int)((*it_d)->getSize()));
                    else
                        ar->append_dim((int)((*it_d)->getSize()), (*it_d)->getNewName());
                }

                dds.add_var(ar);
            }
            break;
            case CV_MODIFY:
            default: 
                delete bt;
                throw InternalErr(__FILE__,__LINE__,"Coordinate variable type is not supported.");
        }
    }
    delete bt;
}

// Generate DDS for special variable in a general product
void gen_dap_onegmspvar_dds(DDS &dds,const HDF5CF::GMSPVar* spvar, const hid_t fileid, const string & filename) {

    BESDEBUG("h5","Coming to gen_dap_onegmspvar_dds()  "<<endl);
    BaseType *bt = nullptr;

    switch(spvar->getType()) {
#define HANDLE_CASE(tid,type)                                  \
        case tid:                                           \
            bt = new (type)(spvar->getNewName(),spvar->getFullPath());  \
        break;

        HANDLE_CASE(H5FLOAT32, HDF5CFFloat32)
        HANDLE_CASE(H5FLOAT64, HDF5CFFloat64)
        HANDLE_CASE(H5CHAR,HDF5CFInt16)
        HANDLE_CASE(H5UCHAR, HDF5CFByte)
        HANDLE_CASE(H5INT16, HDF5CFInt16)
        HANDLE_CASE(H5UINT16, HDF5CFUInt16)
        HANDLE_CASE(H5INT32, HDF5CFInt32)
        HANDLE_CASE(H5UINT32, HDF5CFUInt32)
        HANDLE_CASE(H5FSTRING, Str)
        HANDLE_CASE(H5VSTRING, Str)
        default:
            throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
    }

    if (bt) {

        const vector<HDF5CF::Dimension *>& dims = spvar->getDimensions();

        if (dims.empty()) {
            delete bt;
            throw InternalErr(__FILE__, __LINE__, "Currently don't support scalar special variables. ");
        }
        HDF5GMSPCFArray *ar = nullptr;

        auto ar_unique = make_unique<HDF5GMSPCFArray> (
                             spvar->getRank(),
                             filename,
                             fileid,
                             spvar->getType(),
                             spvar->getFullPath(),
                             spvar->getOriginalType(),
                             spvar->getStartBit(),
                             spvar->getBitNum(),
                             spvar->getNewName(),
                             bt);
        ar = ar_unique.get();

        for (auto const &dim:dims) {
            if ((dim->getNewName()).empty())
                ar->append_dim((int)(dim->getSize()));
            else 
                ar->append_dim((int)(dim->getSize()), dim->getNewName());
        }

        dds.add_var(ar);
        delete bt;
    }

}

// When we add floating point fill value at HDF5CF.cc, the value will be changed
// a little bit when it changes to string representation. 
// For example, -9999.9 becomes -9999.9000123. To reduce the misunderstanding,we
// just add fillvalue in the string type here. KY 2014-04-02
void update_GPM_special_attrs(DAS& das, const HDF5CF::Var *var,bool is_cvar) {

    BESDEBUG("h5","Coming to update_GPM_special_attrs()  "<<endl);
    if(H5FLOAT64 == var->getType() ||
       H5FLOAT32 == var->getType() ||
       H5INT16 == var->getType() ||
       H5CHAR == var->getType()) {

        AttrTable *at = das.get_table(var->getNewName());
        if (nullptr == at)
            at = das.add_table(var->getNewName(), obtain_new_attr_table());
        bool has_fillvalue = false;
        AttrTable::Attr_iter it = at->attr_begin();
        while (it!=at->attr_end() && false==has_fillvalue) {
            if (at->get_name(it) =="_FillValue")
            {
                has_fillvalue = true;
                string fillvalue ="";
                if(H5FLOAT32 == var->getType()) {
                    const string cor_fill_value = "-9999.9";
                    fillvalue =  (*at->get_attr_vector(it)->begin());
                    if((fillvalue.find(cor_fill_value) == 0) && (fillvalue!= cor_fill_value)) {
                        at->del_attr("_FillValue");
                        at->append_attr("_FillValue","Float32",cor_fill_value);
                    }
                }
                else if(H5FLOAT64 == var->getType()) {
                    const string cor_fill_value = "-9999.9";
                    const string exist_fill_value_substr = "-9999.8999";
                    fillvalue =  (*at->get_attr_vector(it)->begin());
                    if((fillvalue.find(exist_fill_value_substr) == 0) && (fillvalue!= cor_fill_value)) {
                        at->del_attr("_FillValue");
                        at->append_attr("_FillValue","Float64",cor_fill_value);
                    }
             
                }
            }
            it++;
        }

        // Add the fill value
        if(false == is_cvar ) {

            // Current versions of GPM don't add fillvalues. We add the fillvalue according to the document.
            if (has_fillvalue != true ) {
            
                if(H5FLOAT32 == var->getType()) 
                    at->append_attr("_FillValue","Float32","-9999.9");
                else if(H5FLOAT64 == var->getType())
                    at->append_attr("_FillValue","Float64","-9999.9");
                else if (H5INT16 == var->getType()) 
                    at->append_attr("_FillValue","Int16","-9999");
                else if (H5CHAR == var->getType())// H5CHAR maps to DAP int16
                    at->append_attr("_FillValue","Int16","-99");
        
            }
        }
    }
}

// This routine is following the DAP2 way,except we map HDF5 8-bit integer to DAP4 8-bit integer. 
// When we add floating point fill value at HDF5CF.cc, the value will be changed
// a little bit when it changes to string representation. 
// For example, -9999.9 becomes -9999.9000123. To reduce the misunderstanding,we
// just add fillvalue in the string type here. KY 2014-04-02
void update_GPM_special_attrs_cfdmr(libdap::D4Group* d4_root, const vector<HDF5CF::GMCVar *>& cvars) {

    BESDEBUG("h5","Coming to update_GPM_special_attrs_cfdmr()  "<<endl);

    // We need to loop through all the variables.
    Constructor::Vars_iter vi = d4_root->var_begin();
    Constructor::Vars_iter ve = d4_root->var_end();

    for (; vi != ve; vi++) {
        
        // The _FillValue datatype only applies to 
        // 32-bit and 64-bit floating-point data and
        // 8-bit and 16-bit integer. 

        Type var_type = (*vi)->type();
        if ((*vi)->type() == dods_array_c) 
            var_type = (*vi)->var()->type();
        if (dods_float64_c == var_type ||
            dods_float32_c == var_type ||
            dods_int16_c == var_type ||
            dods_int8_c == var_type) {
 
            const D4Attribute *d4_attr = (*vi)->attributes()->find("_FillValue");

            // If we don't find the _FillValue, according to DAP2 implementation,
            // we need to add the corresponding fill values.
            if (!d4_attr) {
                bool is_cvar = false;
                for (const auto &cvar:cvars) {
                    if (cvar->getNewName() == (*vi)->name()) {
                        is_cvar = true;
                        break;
                    }
                }

                // Add fillvalue for real variables not for the coordinate variables.
                if(false == is_cvar) {
                    // Add a DAP4 attribute
                    D4Attribute *d4_fv = nullptr;
                    if (dods_float64_c == var_type ) {
                        auto d4_fv_unique = make_unique<D4Attribute>("_FillValue",attr_float64_c);
                        d4_fv = d4_fv_unique.release();
                        d4_fv->add_value("-9999.9");
                    }
                    else if (dods_float32_c == var_type) {
                        auto d4_fv_unique = make_unique<D4Attribute>("_FillValue",attr_float32_c);
                        d4_fv = d4_fv_unique.release();
                        d4_fv->add_value("-9999.9");
                    }
                    else if (dods_int16_c == var_type) {
                        auto d4_fv_unique = make_unique<D4Attribute>("_FillValue",attr_int16_c);
                        d4_fv = d4_fv_unique.release();
                        d4_fv->add_value("-9999");
                    } 
                    else if (dods_int8_c == var_type) {
                        auto d4_fv_unique = make_unique<D4Attribute>("_FillValue",attr_int8_c);
                        d4_fv = d4_fv_unique.release();
                        d4_fv->add_value("-99");
                    }
                    (*vi)->attributes()->add_attribute_nocopy(d4_fv);
                }
            }
            else {
                D4Attribute *d4_fv = nullptr;
                if (dods_float64_c == var_type ) {
                    const string cor_fill_value = "-9999.9";
                    const string exist_fill_value_substr = "-9999.8999";
                    string fillvalue = d4_attr->value(0);
                    if((fillvalue.find(exist_fill_value_substr) == 0) && (fillvalue!= cor_fill_value)) {
                        (*vi)->attributes()->erase("_FillValue");
                        auto d4_fv_unique = make_unique<D4Attribute>("_FillValue",attr_float64_c);
                        d4_fv = d4_fv_unique.release();
                        d4_fv->add_value(cor_fill_value);
                        (*vi)->attributes()->add_attribute_nocopy(d4_fv);
                    }
                }
                else if (dods_float32_c == var_type) {
                    const string cor_fill_value = "-9999.9";
                    string fillvalue = d4_attr->value(0);
                    // Somehow the fillvalue changes to "-9999.90??", we want to turn it back.
                    if((fillvalue.find(cor_fill_value) == 0) && (fillvalue!= cor_fill_value)) {
                        (*vi)->attributes()->erase("_FillValue");
                        auto d4_fv_unique = make_unique<D4Attribute>("_FillValue",attr_float32_c);
                        d4_fv = d4_fv_unique.release();
                        d4_fv->add_value(cor_fill_value);
                        (*vi)->attributes()->add_attribute_nocopy(d4_fv);
                    }
                }
            }
        }
    }
}

void gen_dap_onegmcvar_dmr(D4Group*d4_root,const GMCVar* cvar,const hid_t fileid, const string &filename) {              

    BESDEBUG("h5","Coming to gen_dap_onegmcvar_dds()  "<<endl);

    BaseType *bt = nullptr;

    switch(cvar->getType()) {
#define HANDLE_CASE(tid,type)                                  \
        case tid:                                           \
            bt = new (type)(cvar->getNewName(),cvar->getFullPath());  \
            break;

        HANDLE_CASE(H5FLOAT32, HDF5CFFloat32)
        HANDLE_CASE(H5FLOAT64, HDF5CFFloat64)
        HANDLE_CASE(H5CHAR,HDF5CFInt8)
        HANDLE_CASE(H5UCHAR, HDF5CFByte)
        HANDLE_CASE(H5INT16, HDF5CFInt16)
        HANDLE_CASE(H5UINT16, HDF5CFUInt16)
        HANDLE_CASE(H5INT32, HDF5CFInt32)
        HANDLE_CASE(H5UINT32, HDF5CFUInt32)
        HANDLE_CASE(H5INT64, HDF5CFInt64)
        HANDLE_CASE(H5UINT64, HDF5CFUInt64)
        HANDLE_CASE(H5FSTRING, Str)
        HANDLE_CASE(H5VSTRING, Str)

        default:
            throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
    }

    if (bt) {

        const vector<HDF5CF::Dimension *>& dims = cvar->getDimensions();
        vector <HDF5CF::Dimension*>:: const_iterator it_d;
        vector <size_t> dimsizes;
        dimsizes.resize(cvar->getRank());

        for(int i = 0; i <cvar->getRank();i++)
            dimsizes[i] = (dims[i])->getSize();

        if(dims.empty()) {
            delete bt;
            throw InternalErr(__FILE__, __LINE__, "the coordinate variable cannot be a scalar");
        }
        switch(cvar->getCVType()) {
            
            case CV_EXIST: 
            {
                HDF5CFArray *ar = nullptr;

                // Need to check if this CV is lat/lon. This is necessary when data memory cache is turned on.
                bool is_latlon = cvar->isLatLon();

                bool is_dap4 = true;
                auto ar_unique = make_unique<HDF5CFArray> (
                                cvar->getRank(),
                                fileid,
                                filename,
                                cvar->getType(),
                                dimsizes,
                                cvar->getFullPath(),
                                cvar->getTotalElems(),
                                CV_EXIST,
                                is_latlon,
                                cvar->getCompRatio(),
                                is_dap4,
                                cvar->getNewName(),
                                bt);
                ar = ar_unique.get();

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (((*it_d)->getNewName()).empty())
                        ar->append_dim_ll((int)((*it_d)->getSize()));
                    else 
                        ar->append_dim_ll((int)((*it_d)->getSize()), (*it_d)->getNewName());
                }

                ar->set_is_dap4(true);
                BaseType* d4_var=ar->h5cfdims_transform_to_dap4(d4_root);
                map_cfh5_var_attrs_to_dap4(cvar,d4_var);
                d4_root->add_var_nocopy(d4_var);
            }
            break;

            case CV_LAT_MISS:
            case CV_LON_MISS:
            {
                // Using HDF5GMCFMissLLArray
                HDF5GMCFMissLLArray *ar = nullptr;
                auto ar_unique = make_unique<HDF5GMCFMissLLArray> (
                                cvar->getRank(),
                                filename,
                                fileid,
                                cvar->getType(),
                                cvar->getFullPath(),
                                cvar->getPtType(),
                                cvar->getCVType(),
                                cvar->getNewName(),
                                bt);
                ar = ar_unique.get();

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (((*it_d)->getNewName()).empty())
                        ar->append_dim_ll((*it_d)->getSize());
                    else 
                        ar->append_dim_ll((*it_d)->getSize(), (*it_d)->getNewName());
                }

                ar->set_is_dap4(true);
                BaseType* d4_var=ar->h5cfdims_transform_to_dap4(d4_root);
                map_cfh5_var_attrs_to_dap4(cvar,d4_var);
                d4_root->add_var_nocopy(d4_var);
            }
            break;

            case CV_NONLATLON_MISS:
            {

                if (cvar->getRank() !=1) {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                }
                auto nelem = (int)((cvar->getDimensions()[0])->getSize());

                HDF5GMCFMissNonLLCVArray *ar = nullptr;

                auto ar_unique = make_unique<HDF5GMCFMissNonLLCVArray>(
                                                  cvar->getRank(),
                                                  nelem,
                                                  cvar->getNewName(),
                                                  bt);
                ar = ar_unique.get();

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (((*it_d)->getNewName()).empty())
                        ar->append_dim_ll((*it_d)->getSize());
                    else 
                        ar->append_dim_ll((*it_d)->getSize(), (*it_d)->getNewName());
                }
                ar->set_is_dap4(true);
                BaseType* d4_var=ar->h5cfdims_transform_to_dap4(d4_root);
                map_cfh5_var_attrs_to_dap4(cvar,d4_var);
                d4_root->add_var_nocopy(d4_var);
            }
            break;

            case CV_FILLINDEX:
            {

                if (cvar->getRank() !=1) {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                }

                HDF5GMCFFillIndexArray *ar = nullptr;

                auto ar_unique = make_unique<HDF5GMCFFillIndexArray>(
                                                 cvar->getRank(),
                                                 cvar->getType(),
                                                 true,
                                                 cvar->getNewName(),
                                                 bt);
                ar = ar_unique.get();

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (((*it_d)->getNewName()).empty())
                        ar->append_dim_ll((*it_d)->getSize());
                    else 
                        ar->append_dim_ll((*it_d)->getSize(), (*it_d)->getNewName());
                }
                ar->set_is_dap4(true);
                BaseType* d4_var=ar->h5cfdims_transform_to_dap4(d4_root);
                map_cfh5_var_attrs_to_dap4(cvar,d4_var);
                d4_root->add_var_nocopy(d4_var);
            }
            break;

            case CV_SPECIAL:
             {
                // Currently only handle 1-D special CV.
                if (cvar->getRank() !=1) {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of special coordinate variable  must be 1");
                }
                int nelem = (cvar->getDimensions()[0])->getSize();

                auto ar_unique = make_unique<HDF5GMCFSpecialCVArray>(cvar->getType(),
                                                nelem,
                                                cvar->getFullPath(),
                                                cvar->getPtType(),
                                                cvar->getNewName(),
                                                bt);
                auto ar = ar_unique.get();
                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (((*it_d)->getNewName()).empty())
                        ar->append_dim_ll((*it_d)->getSize());
                    else
                        ar->append_dim_ll((*it_d)->getSize(), (*it_d)->getNewName());
                }

                ar->set_is_dap4(true);
                BaseType* d4_var=ar->h5cfdims_transform_to_dap4(d4_root);
                map_cfh5_var_attrs_to_dap4(cvar,d4_var);
                d4_root->add_var_nocopy(d4_var);
            }
            break;
            case CV_MODIFY:
            default: 
                delete bt;
                throw InternalErr(__FILE__,__LINE__,"Coordinate variable type is not supported.");
        }
    }
    delete bt;
}

void gen_dap_onegmspvar_dmr(D4Group*d4_root,const GMSPVar*spvar,const hid_t fileid, const string &filename) {

    BESDEBUG("h5","Coming to gen_dap_onegmspvar_dmr()  "<<endl);
    BaseType *bt = nullptr;

    // Note: The special variable is actually an ACOS_OCO2 64-bit integer variable.
    // We decompose 64-bit to two integer variables according to the specification.
    // This product has been served in this way for years. For backward compatibility,
    // we will not change this in the CF DMR implementation. So Int64/UInt64 are not added.
    // KY 2021-03-09
    switch(spvar->getType()) {
#define HANDLE_CASE(tid,type)                                  \
        case tid:                                           \
            bt = new (type)(spvar->getNewName(),spvar->getFullPath());  \
        break;

        HANDLE_CASE(H5FLOAT32, HDF5CFFloat32)
        HANDLE_CASE(H5FLOAT64, HDF5CFFloat64)
        HANDLE_CASE(H5CHAR,HDF5CFByte)
        HANDLE_CASE(H5UCHAR, HDF5CFByte)
        HANDLE_CASE(H5INT16, HDF5CFInt16)
        HANDLE_CASE(H5UINT16, HDF5CFUInt16)
        HANDLE_CASE(H5INT32, HDF5CFInt32)
        HANDLE_CASE(H5UINT32, HDF5CFUInt32)
        HANDLE_CASE(H5FSTRING, Str)
        HANDLE_CASE(H5VSTRING, Str)
        default:
            throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
    }

    if (bt) {

        const vector<HDF5CF::Dimension *> &dims = spvar->getDimensions();

        if (dims.empty()) {
            delete bt;
            throw InternalErr(__FILE__, __LINE__, "Currently don't support scalar special variables. ");
        }
        
        HDF5GMSPCFArray *ar = nullptr;

        auto ar_unique = make_unique<HDF5GMSPCFArray>(spvar->getRank(),
                             filename,
                             fileid,
                             spvar->getType(),
                             spvar->getFullPath(),
                             spvar->getOriginalType(),
                             spvar->getStartBit(),
                             spvar->getBitNum(),
                             spvar->getNewName(),
                             bt);
        ar = ar_unique.get();

        for (const auto &dim:dims) {
            if (""==dim->getNewName()) 
                ar->append_dim_ll(dim->getSize());
            else 
                ar->append_dim_ll(dim->getSize(), dim->getNewName());
        }

        ar->set_is_dap4(true);
        BaseType* d4_var=ar->h5cfdims_transform_to_dap4(d4_root);
        map_cfh5_var_attrs_to_dap4(spvar,d4_var);
        d4_root->add_var_nocopy(d4_var);
 
        delete bt;
    }

}
