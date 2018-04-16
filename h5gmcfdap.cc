// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

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

////////////////////////////////////////////////////////////////////////////////
/// \file h5gmcfdap.cc
/// \brief Map and generate DDS and DAS for the CF option for generic HDF5 products 
///
/// 
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

#include <BESDebug.h>
#include <InternalErr.h>

#include "HDF5RequestHandler.h"
#include "h5cfdaputil.h"
#include "h5gmcfdap.h"
#include "HDF5CFByte.h"
#include "HDF5CFUInt16.h"
#include "HDF5CFInt16.h"
#include "HDF5CFUInt32.h"
#include "HDF5CFInt32.h"
#include "HDF5CFFloat32.h"
#include "HDF5CFFloat64.h"
#include "HDF5CFStr.h"
#include "HDF5CFArray.h"
#include "HDF5GMCFMissLLArray.h"
#include "HDF5GMCFFillIndexArray.h"
#include "HDF5GMCFMissNonLLCVArray.h"
#include "HDF5GMCFSpecialCVArray.h"
#include "HDF5GMSPCFArray.h"

using namespace HDF5CF;

// Map general HDF5 products to DAP DDS
void map_gmh5_cfdds(DDS &dds, hid_t file_id, const string& filename){

    BESDEBUG("h5","Coming to GM products DDS mapping function map_gmh5_cfdds()  "<<endl);

    H5GCFProduct product_type = check_product(file_id);

    GMPattern  gproduct_pattern = OTHERGMS;

    GMFile * f = NULL;

    try {
        f = new GMFile(filename.c_str(),file_id,product_type,gproduct_pattern);
    }
    catch(...) {
        throw InternalErr(__FILE__,__LINE__,"Cannot allocate memory for GMFile ");
    }
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

        // We need to retrieve  coordinate variable attributes for memory cache use.
        //f->Retrieve_H5_CVar_Supported_Attr_Values(); 
        //if((HDF5RequestHandler::get_lrdata_mem_cache() != NULL) || 
        //   (HDF5RequestHandler::get_srdata_mem_cache() != NULL)){
        //    f->Retrieve_H5_Supported_Attr_Values();


        // Handle special variables
        f->Handle_SpVar();

        // When cv memory cache is on, the unit attributes are needed to
        // distinguish whether this is lat/lon. Generally, memory cache 
        // is not used. This snipnet will not be accessed.
        if((HDF5RequestHandler::get_lrdata_mem_cache() != NULL) ||
           (HDF5RequestHandler::get_srdata_mem_cache() != NULL)){

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
        if((HDF5RequestHandler::get_lrdata_mem_cache() != NULL) || 
           (HDF5RequestHandler::get_srdata_mem_cache() != NULL))
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
        if((HDF5RequestHandler::get_lrdata_mem_cache() != NULL) || 
           (HDF5RequestHandler::get_srdata_mem_cache() != NULL))
            f->Handle_Coor_Attr();
 
        f->Remove_Unused_FakeDimVars();
        f->Rename_NC4_NonCoordVars();
    }
    catch (HDF5CF::Exception &e){
        if (f != NULL)
            delete f;
        throw InternalErr(e.what());
    }
    
    // generate DDS.
    try {
        gen_gmh5_cfdds(dds,f);
    }
    catch(...) {
        if (f != NULL)
            delete f;
        throw;
    }
    
    if (f != NULL)
        delete f;
}

// Map general HDF5 products to DAP DAS
void map_gmh5_cfdas(DAS &das, hid_t file_id, const string& filename){

    BESDEBUG("h5","Coming to GM products DAS mapping function map_gmh5_cfdas()  "<<endl);

    H5GCFProduct product_type = check_product(file_id);
    GMPattern gproduct_pattern = OTHERGMS;

    GMFile *f = NULL;

    try {
        f = new GMFile(filename.c_str(),file_id,product_type,gproduct_pattern);
    }
    catch(...) {
        throw InternalErr(__FILE__,__LINE__,"Cannot allocate memory for GMFile ");
    }

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

        // Handle the "coordinate" attributes.
        f->Handle_Coor_Attr();

        f->Handle_Hybrid_EOS5();
        if(true == f->Have_Grid_Mapping_Attrs()) 
            f->Handle_Grid_Mapping_Vars();

        f->Remove_Unused_FakeDimVars();

        f->Rename_NC4_NonCoordVars();
    }
    catch (HDF5CF::Exception &e){
        if (f!= NULL)
            delete f;
        throw InternalErr(e.what());
    }

    // Generate the DAS attributes.
    try {
        gen_gmh5_cfdas(das,f);
    }   
    catch (...) {
        if (f!= NULL)
            delete f;
        throw;
 
    }

    if (f != NULL)
        delete f;
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

    vector<HDF5CF::Var *>::const_iterator       it_v;
    vector<HDF5CF::GMCVar *>::const_iterator   it_cv;
    vector<HDF5CF::GMSPVar *>::const_iterator it_spv;

    for (it_v = vars.begin(); it_v !=vars.end();++it_v) {
        BESDEBUG("h5","variable full path= "<< (*it_v)->getFullPath() <<endl);
        if(need_attr_values_for_dap4(*it_v) == true) 
            f->Retrieve_H5_Var_Attr_Values(*it_v);
        gen_dap_onevar_dds(dds,*it_v,fileid, filename);
    }
    for (it_cv = cvars.begin(); it_cv !=cvars.end();++it_cv) {
        BESDEBUG("h5","variable full path= "<< (*it_cv)->getFullPath() <<endl);
        gen_dap_onegmcvar_dds(dds,*it_cv,fileid, filename);
    }

    for (it_spv = spvars.begin(); it_spv !=spvars.end();it_spv++) {
        BESDEBUG("h5","variable full path= "<< (*it_spv)->getFullPath() <<endl);
        gen_dap_onegmspvar_dds(dds,*it_spv,fileid, filename);
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


    vector<HDF5CF::Var *>::const_iterator it_v;
    vector<HDF5CF::GMCVar *>::const_iterator it_cv;
    vector<HDF5CF::GMSPVar *>::const_iterator it_spv;
    vector<HDF5CF::Group *>::const_iterator it_g;
    vector<HDF5CF::Attribute *>::const_iterator it_ra;

    // Handling the file attributes(attributes under the root group)
    // The table name is "HDF_GLOBAL".

    if (false == root_attrs.empty()) {

        AttrTable *at = das.get_table(FILE_ATTR_TABLE_NAME);
        if (NULL == at) 
            at = das.add_table(FILE_ATTR_TABLE_NAME, new AttrTable);

        for (it_ra = root_attrs.begin(); it_ra != root_attrs.end(); ++it_ra) {
            check_update_int64_attr("",*it_ra);
            gen_dap_oneobj_das(at,*it_ra,NULL);
        }
    }

    if (false == grps.empty()) {
        for (it_g = grps.begin();
             it_g != grps.end(); ++it_g) {
            AttrTable *at = das.get_table((*it_g)->getNewName());
            if (NULL == at)
                at = das.add_table((*it_g)->getNewName(), new AttrTable);

            for (it_ra = (*it_g)->getAttributes().begin();
                 it_ra != (*it_g)->getAttributes().end(); ++it_ra) {
                check_update_int64_attr((*it_g)->getNewName(),*it_ra);
                gen_dap_oneobj_das(at,*it_ra,NULL);
            }
        }
    }

    for (it_v = vars.begin();
         it_v != vars.end(); ++it_v) {
        if (false == ((*it_v)->getAttributes().empty())) {

            // Add 64-bit int support.
            if(H5INT64 == (*it_v)->getType() || H5UINT64 == (*it_v)->getType()){
               continue;
            }

            AttrTable *at = das.get_table((*it_v)->getNewName());
            if (NULL == at)
                at = das.add_table((*it_v)->getNewName(), new AttrTable);

            for (it_ra = (*it_v)->getAttributes().begin();
                 it_ra != (*it_v)->getAttributes().end(); ++it_ra) {
                gen_dap_oneobj_das(at,*it_ra,*it_v);
            }
            // If a var has integer-64 bit datatype attributes, we have to duplicate all 
            // the var in dmr and delete this var from dds. 
                    
        }

        // GPM needs to be handled in a special way(mostly _FillValue)
        if(GPMS_L3 == f->getProductType() || GPMM_L3 == f->getProductType() 
                                          || GPM_L1 == f->getProductType()) 
            update_GPM_special_attrs(das,*it_v,false);
        
    }

    for (it_cv = cvars.begin();
         it_cv != cvars.end(); ++it_cv) {
        if (false == ((*it_cv)->getAttributes().empty())) {

            // Add 64-bit int support.
            if(H5INT64 == (*it_cv)->getType() || H5UINT64 == (*it_cv)->getType()){
               continue;
            }

            AttrTable *at = das.get_table((*it_cv)->getNewName());
            if (NULL == at)
                at = das.add_table((*it_cv)->getNewName(), new AttrTable);

            for (it_ra = (*it_cv)->getAttributes().begin();
                 it_ra != (*it_cv)->getAttributes().end(); ++it_ra){ 
                gen_dap_oneobj_das(at,*it_ra,*it_cv);
            }
                    
        }
        // Though CF doesn't allow _FillValue, still keep it to keep the original form.
        if(GPMS_L3 == f->getProductType() || GPMM_L3 == f->getProductType() 
                                          || GPM_L1 == f->getProductType()) 
            update_GPM_special_attrs(das,*it_cv,true);

    }

    // Currently the special variables are only limited to the ACOS/OCO2 64-bit integer variables
    for (it_spv = spvars.begin();
         it_spv != spvars.end(); ++it_spv) {
        if (false == ((*it_spv)->getAttributes().empty())) {

            AttrTable *at = das.get_table((*it_spv)->getNewName());
            if (NULL == at)
                at = das.add_table((*it_spv)->getNewName(), new AttrTable);
            // cerr<<"spv coordinate variable name "<<(*it_spv)->getNewName() <<endl;

            for (it_ra = (*it_spv)->getAttributes().begin();
                 it_ra != (*it_spv)->getAttributes().end(); ++it_ra) 
                gen_dap_oneobj_das(at,*it_ra,*it_spv);
        }
    }
       
    // CHECK ALL UNLIMITED DIMENSIONS from the coordinate variables based on the names. 
    if(f->HaveUnlimitedDim() == true) {

        BESDEBUG("h5","Find unlimited dimension in the GM DAS generation function gen_gmh5_cfdas()  "<<endl);

        // Currently there is no way for DAP to present the unlimited dimension info.
        // when there are no dimension names. So don't create DODS_EXTRA even if
        // there is an unlimited dimension in the file. KY 2016-02-18
        if(cvars.size() >0){

            // First check if we do have unlimited dimension in the coordinate variables.
            // Since unsupported fakedims are removed, we may not have unlimited dimensions.
            bool still_has_unlimited = false;
            for (it_cv = cvars.begin();
                it_cv != cvars.end(); ++it_cv) {
                // Check unlimited dimension names.
                for (vector<Dimension*>::const_iterator ird = (*it_cv)->getDimensions().begin();
                     ird != (*it_cv)->getDimensions().end(); ++ird) {

                    // Currently we only check one unlimited dimension, which is the most
                    // common case. When receiving the conventions from JG, will add
                    // the support of multi-unlimited dimension. KY 2016-02-09
                    if((*ird)->HaveUnlimitedDim() == true) {
                        still_has_unlimited = true;
                        break;
                    }// if((*ird)->HaveUnlimitedDim()
                }// for (vector<Dimension*>::
                if(true == still_has_unlimited) 
                    break;
            }// for (it_cv=cvars.begin();
 
           if(true == still_has_unlimited) {
            AttrTable* at = das.get_table("DODS_EXTRA");
            if (NULL == at)
                at = das.add_table("DODS_EXTRA", new AttrTable);
         
            string unlimited_names;

            for (it_cv = cvars.begin();
                it_cv != cvars.end(); ++it_cv) {
#if 0
                bool has_unlimited_dim = false;
#endif
                // Check unlimited dimension names.
                for (vector<Dimension*>::const_iterator ird = (*it_cv)->getDimensions().begin();
                     ird != (*it_cv)->getDimensions().end(); ++ird) {

                    // Currently we only check one unlimited dimension, which is the most
                    // common case. When receiving the conventions from JG, will add
                    // the support of multi-unlimited dimension. KY 2016-02-09
                    if((*ird)->HaveUnlimitedDim() == true) {
                        if(unlimited_names=="") {
                           unlimited_names = (*ird)->getNewName();
                           if(at !=NULL) 
                                at->append_attr("Unlimited_Dimension","String",unlimited_names);
                        }
                        else {
                            if(unlimited_names.rfind((*ird)->getNewName()) == string::npos) {
                                unlimited_names = unlimited_names+" "+(*ird)->getNewName();
                                if(at !=NULL) 
                                    at->append_attr("Unlimited_Dimension","String",(*ird)->getNewName());
                            }
                        }
                    }// if((*ird)->HaveUnlimitedDim()
                }// for (vector<Dimension*>::
            }// for (it_cv=cvars.begin();
          }// if(true == still_has_unlimited)
            
        }//if(cvars.size()>0)
        // The following line will generate the string like "Band1 str1 str2".
        //if(unlimited_names!="") 
        //         //   at->append_attr("Unlimited_Dimension","String",unlimited_names);
    }
}

// Generate the ignored object info. for the CF option of the general products
void gen_gmh5_cf_ignored_obj_info(DAS &das, HDF5CF::GMFile *f) {

    BESDEBUG("h5","Coming to gen_gmh5_cf_ignored_obj_info()  "<<endl);
    AttrTable *at = das.get_table("Ignored_Object_Info");
    if (NULL == at)
        at = das.add_table("Ignored_Object_Info", new AttrTable);

    at->append_attr("Message","String",f->Get_Ignored_Msg());

}

// Generate the DDS for a coordinate variable of the General products
void gen_dap_onegmcvar_dds(DDS &dds,const HDF5CF::GMCVar* cvar, const hid_t file_id, const string & filename) {

    BESDEBUG("h5","Coming to gen_dap_onegmcvar_dds()  "<<endl);

    if(cvar->getType() == H5INT64 || cvar->getType() == H5UINT64)
        return;
    BaseType *bt = NULL;

    switch(cvar->getType()) {
#define HANDLE_CASE(tid,type)                                  \
        case tid:                                           \
            bt = new (type)(cvar->getNewName(),cvar->getFullPath());  \
            break;

        HANDLE_CASE(H5FLOAT32, HDF5CFFloat32);
        HANDLE_CASE(H5FLOAT64, HDF5CFFloat64);
        HANDLE_CASE(H5CHAR,HDF5CFInt16);
        HANDLE_CASE(H5UCHAR, HDF5CFByte);
        HANDLE_CASE(H5INT16, HDF5CFInt16);
        HANDLE_CASE(H5UINT16, HDF5CFUInt16);
        HANDLE_CASE(H5INT32, HDF5CFInt32);
        HANDLE_CASE(H5UINT32, HDF5CFUInt32);
        HANDLE_CASE(H5FSTRING, Str);
        HANDLE_CASE(H5VSTRING, Str);

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


        if(dims.size() == 0) 
            throw InternalErr(__FILE__,__LINE__,"the coordinate variable cannot be a scalar");
            
        switch(cvar->getCVType()) {
            
            case CV_EXIST: 
            {
                HDF5CFArray *ar = NULL;

                // Need to check if this CV is lat/lon. This is necessary when data memory cache is turned on.
                bool is_latlon = cvar->isLatLon();
                
                try {
                    ar = new HDF5CFArray (
                                    cvar->getRank(),
                                    file_id,
                                    filename,
                                    cvar->getType(),
                                    dimsizes,
                                    cvar->getFullPath(),
                                    cvar->getTotalElems(),
                                    CV_EXIST,
                                    is_latlon,
                                    cvar->getCompRatio(),
                                    cvar->getNewName(),
                                    bt);
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,"Unable to allocate HDF5CFArray. ");
                }

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (""==(*it_d)->getNewName()) 
                        ar->append_dim((*it_d)->getSize());
                    else 
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getNewName());
                }

                dds.add_var(ar);
                delete bt;
                delete ar;
            }
            break;

            case CV_LAT_MISS:
            case CV_LON_MISS:
            {
                // Using HDF5GMCFMissLLArray
                HDF5GMCFMissLLArray *ar = NULL;
                try {
                    ar = new HDF5GMCFMissLLArray (
                                    cvar->getRank(),
                                    filename,
                                    file_id,
                                    cvar->getType(),
                                    cvar->getFullPath(),
                                    cvar->getPtType(),
                                    cvar->getCVType(),
                                    cvar->getNewName(),
                                    bt);
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,"Unable to allocate HDF5GMCFMissLLArray. ");
                }

           
                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (""==(*it_d)->getNewName()) 
                        ar->append_dim((*it_d)->getSize());
                    else 
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getNewName());
                }

                dds.add_var(ar);
                delete bt;
                delete ar;
            }
            break;

            case CV_NONLATLON_MISS:
            {

                if (cvar->getRank() !=1) {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                }
                int nelem = (cvar->getDimensions()[0])->getSize();

                HDF5GMCFMissNonLLCVArray *ar = NULL;

                try {
                    ar = new HDF5GMCFMissNonLLCVArray(
                                                      cvar->getRank(),
                                                      nelem,
                                                      cvar->getNewName(),
                                                      bt);
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,"Unable to allocate HDF5GMCFMissNonLLCVArray. ");
                }


                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (""==(*it_d)->getNewName()) 
                        ar->append_dim((*it_d)->getSize());
                    else 
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getNewName());
                }
                dds.add_var(ar);
                delete bt;
                delete ar;
            }
            break;

            case CV_FILLINDEX:
            {

                if (cvar->getRank() !=1) {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                }

                HDF5GMCFFillIndexArray *ar = NULL;
  
                try {
                    ar = new HDF5GMCFFillIndexArray(
                                                      cvar->getRank(),
                                                      cvar->getType(),
                                                      cvar->getNewName(),
                                                      bt);
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,"Unable to allocate HDF5GMCFMissNonLLCVArray. ");
                }


                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (""==(*it_d)->getNewName()) 
                        ar->append_dim((*it_d)->getSize());
                    else 
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getNewName());
                }
                dds.add_var(ar);
                delete bt;
                delete ar;
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

                HDF5GMCFSpecialCVArray * ar = NULL;
                ar = new HDF5GMCFSpecialCVArray(
                                                cvar->getType(),
                                                nelem,
                                                cvar->getFullPath(),
                                                cvar->getPtType(),
                                                cvar->getNewName(),
                                                bt);

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
                    if (""==(*it_d)->getNewName())
                        ar->append_dim((*it_d)->getSize());
                    else
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getNewName());
                }

                dds.add_var(ar);
                delete bt;
                delete ar;

            }
            break;
            case CV_MODIFY:
            default: 
                delete bt;
                throw InternalErr(__FILE__,__LINE__,"Coordinate variable type is not supported.");
        }
    }
}

// Generate DDS for special variable in a general product
void gen_dap_onegmspvar_dds(DDS &dds,const HDF5CF::GMSPVar* spvar, const hid_t fileid, const string & filename) {

    BESDEBUG("h5","Coming to gen_dap_onegmspvar_dds()  "<<endl);
    BaseType *bt = NULL;

    switch(spvar->getType()) {
#define HANDLE_CASE(tid,type)                                  \
        case tid:                                           \
            bt = new (type)(spvar->getNewName(),spvar->getFullPath());  \
        break;

        HANDLE_CASE(H5FLOAT32, HDF5CFFloat32);
        HANDLE_CASE(H5FLOAT64, HDF5CFFloat64);
        HANDLE_CASE(H5CHAR,HDF5CFInt16);
        HANDLE_CASE(H5UCHAR, HDF5CFByte);
        HANDLE_CASE(H5INT16, HDF5CFInt16);
        HANDLE_CASE(H5UINT16, HDF5CFUInt16);
        HANDLE_CASE(H5INT32, HDF5CFInt32);
        HANDLE_CASE(H5UINT32, HDF5CFUInt32);
        HANDLE_CASE(H5FSTRING, Str);
        HANDLE_CASE(H5VSTRING, Str);
        default:
            throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
    }

    if (bt) {

        const vector<HDF5CF::Dimension *>& dims = spvar->getDimensions();
        vector <HDF5CF::Dimension*>:: const_iterator it_d;

        if(dims.size() == 0)
            throw InternalErr(__FILE__,__LINE__,"Currently don't support scalar special variables. ");

        HDF5GMSPCFArray *ar = NULL;
 
        try {
            ar = new HDF5GMSPCFArray (
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
        }
        catch(...) {
            delete bt;
            throw InternalErr(__FILE__,__LINE__,"Unable to allocate HDF5GMCFMissNonLLCVArray. ");
        }


        for(it_d = dims.begin(); it_d != dims.end(); ++it_d) {
            if (""==(*it_d)->getNewName()) 
                ar->append_dim((*it_d)->getSize());
            else 
                ar->append_dim((*it_d)->getSize(), (*it_d)->getNewName());
        }

        dds.add_var(ar);
        delete bt;
        delete ar;
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
        if (NULL == at)
            at = das.add_table(var->getNewName(), new AttrTable);
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

               
