// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c)  2011-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file heos5cfdap.cc
/// \brief Map and generate DDS and DAS for the CF option for HDF-EOS5 products 
///
/// This file also includes a function to retrieve ECS metadata in C++ string forms.
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

#include "parser.h"
#include "heos5cfdap.h"
#include "h5cfdaputil.h"
#include "HDF5CFByte.h"
#include "HDF5CFUInt16.h"
#include "HDF5CFInt16.h"
#include "HDF5CFUInt32.h"
#include "HDF5CFInt32.h"
#include "HDF5CFFloat32.h"
#include "HDF5CFFloat64.h"
#include "HDF5CFStr.h"
#include "HDF5CFArray.h"
#include "HDFEOS5CFMissLLArray.h"
#include "HDFEOS5CFMissNonLLCVArray.h"
#include "HDFEOS5CFSpecialCVArray.h"

#include "he5dds.tab.hh"
#include "HE5Parser.h"
#include "HE5Checker.h"
#include "he5das.tab.hh"

struct yy_buffer_state;

yy_buffer_state *he5dds_scan_string(const char *str);
int he5ddsparse(HE5Parser *he5parser);
int he5dasparse(libdap::parser_arg *arg);

/// Buffer state for NASA EOS metadata scanner
yy_buffer_state *he5das_scan_string(const char *str);

using namespace HDF5CF;

void map_eos5_cfdds(DDS &dds, hid_t file_id, const string & filename) {

    string st_str ="";
    string core_str="";
    string arch_str="";
    string xml_str ="";
    string subset_str="";
    string product_str="";
    string other_str ="";
    bool st_only = true;

    // Read ECS metadata: merge them into one C++ string
    read_ecs_metadata(file_id,st_str,core_str,arch_str,xml_str, subset_str,product_str,other_str,st_only); 
    if(""==st_str) {
        H5Fclose(file_id);
        string msg =
            "unable to obtain the HDF-EOS5 struct metadata ";
        throw InternalErr(__FILE__, __LINE__, msg);
    }
     
    string check_objnameclashing_key ="H5.EnableCheckNameClashing";
    bool is_check_nameclashing = false;
    is_check_nameclashing = HDF5CFDAPUtil::check_beskeys(check_objnameclashing_key);

    EOS5File *f = new EOS5File(filename.c_str(),file_id);
    bool include_attr = false;

    // This first "try-catch" block will use the parsed info
    try {

        // Parse the structmetadata
        HE5Parser p;
        HE5Checker c;
        he5dds_scan_string(st_str.c_str());
        he5ddsparse(&p);
//        p.print();
// cerr<<"main loop  p.za_list.size() = "<<p.za_list.size() <<endl;

        // Check if the HDF-EOS5 grid has the valid parameters, projection codes.
        if (c.check_grids_unknown_parameters(&p)) {
            delete f;
            throw InternalErr("Unknown HDF-EOS5 grid paramters found in the file");
        }

        if (c.check_grids_missing_projcode(&p)) {
            delete f;
            throw InternalErr("The HDF-EOS5 is missing project code ");
        }

        if (c.check_grids_support_projcode(&p)) {
            delete f;
            throw InternalErr("The current project code is not supported");
        }
       
        // HDF-EOS5 provides default pixel and origin values if they are not defined.
        c.set_grids_missing_pixreg_orig(&p);

        // cerr<<"after unknown parameters "<<endl;
        // Check if this multi-grid file shares the same grid.
        bool grids_mllcv = c.check_grids_multi_latlon_coord_vars(&p);

        // Retrieve all HDF5 info(Not the values)
        f->Retrieve_H5_Info(filename.c_str(),file_id,include_attr);

        // Adjust EOS5 Dimension names/sizes based on the parsed results
        f->Adjust_EOS5Dim_Info(&p);

        // Translate the parsed output to HDF-EOS5 grids/swaths/zonal.
        f->Add_EOS5File_Info(&p, grids_mllcv);

        // Add the dimension names
        f->Add_Dim_Name(&p);
    }
    catch (HDF5CF::Exception &e){
        delete f;
        throw InternalErr(e.what());
    }


    // The parsed struct will no longer be in this "try-catch" block.
    try {

        // NASA Aura files need special handlings. So first check if this file is an Aura file.
        f->Check_Aura_Product_Status();

        // Adjust the variable name
        f->Adjust_Var_NewName_After_Parsing();

        // Handle coordinate variables
        f->Handle_CVar();

        // Adjust variable and dimension names again based on the handling coordinate variables.
        f->Adjust_Var_Dim_NewName_Before_Flattening();

        // Remove unsupported datatype 
        f->Handle_Unsupported_Dtype(include_attr);

        // Remove unsupported dataspace 
        f->Handle_Unsupported_Dspace();
        
        // May need to adjust the object names for special objects. Currently no operations
        // are done in this routine.
        f->Adjust_Obj_Name();

        // Flatten the object name
        f->Flatten_Obj_Name(include_attr);
     
        // Handle name clashing     
        if(true == is_check_nameclashing)
            f->Handle_Obj_NameClashing(include_attr);

        // Check if this should follow COARDS, yes, set the COARDS flag.
        f->Set_COARDS_Status();

        // For COARDS, the dimension name needs to be changed.
        f->Adjust_Dim_Name();
        // We need to turn off the very long string in the TES file to avoid
        // the choking of netCDF Java tools. So this special variable routine
        // is listed at last. We may need to turn off this if netCDF can handle
        // long string better.
        f->Handle_SpVar();
    }
    catch (HDF5CF::Exception &e){
        delete f;
        throw InternalErr(e.what());
    }

    gen_eos5_cfdds(dds,f);

    delete f;
}

void map_eos5_cfdas(DAS &das, hid_t file_id, const string &filename) {

    string st_str ="";
    string core_str="";
    string arch_str="";
    string xml_str ="";
    string subset_str="";
    string product_str="";
    string other_str ="";
    bool st_only = true;

    read_ecs_metadata(file_id,st_str,core_str,arch_str,xml_str, subset_str,product_str,other_str,st_only); 
    if(""==st_str) {
        H5Fclose(file_id);
        string msg =
            "unable to obtain the HDF-EOS5 struct metadata ";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    string check_objnameclashing_key ="H5.EnableCheckNameClashing";
    bool is_check_nameclashing = false;
    is_check_nameclashing = HDF5CFDAPUtil::check_beskeys(check_objnameclashing_key);

    string add_path_attrs_key = "H5.EnableAddPathAttrs";
    bool is_add_path_attrs = false;
    is_add_path_attrs = HDF5CFDAPUtil::check_beskeys(add_path_attrs_key);

    EOS5File *f = new EOS5File(filename.c_str(),file_id);
    bool include_attr = true;

    // The first "try-catch" block will use the parsed info.
    try {

        HE5Parser p;
        HE5Checker c;
        he5dds_scan_string(st_str.c_str());
        he5ddsparse(&p);
//      p.print();
// cerr<<"main loop  p.za_list.size() = "<<p.za_list.size() <<endl;

        if (c.check_grids_unknown_parameters(&p)) {
            delete f;
            throw InternalErr("Unknown HDF-EOS5 grid paramters found in the file");
        }

        if (c.check_grids_missing_projcode(&p)) {
            delete f;
            throw InternalErr("The HDF-EOS5 is missing project code ");
        }

        if (c.check_grids_support_projcode(&p)) {
            delete f;
            throw InternalErr("The current project code is not supported");
        }
        c.set_grids_missing_pixreg_orig(&p);
        // cerr<<"after unknown parameters "<<endl;

        bool grids_mllcv = c.check_grids_multi_latlon_coord_vars(&p);

        f->Retrieve_H5_Info(filename.c_str(),file_id,include_attr);
        f->Adjust_EOS5Dim_Info(&p);
        f->Add_EOS5File_Info(&p, grids_mllcv);
        f->Add_Dim_Name(&p);
    }
    catch (HDF5CF::Exception &e){
        delete f;
        throw InternalErr(e.what());
    }

    try {
        f->Check_Aura_Product_Status();
        f->Adjust_Var_NewName_After_Parsing();
        f->Handle_CVar();
        f->Adjust_Var_Dim_NewName_Before_Flattening();
        f->Handle_Unsupported_Dtype(include_attr);

        // Remove unsupported dataspace 
        f->Handle_Unsupported_Dspace();


        // Need to retrieve the attribute values.
        f->Retrieve_H5_Supported_Attr_Values();

        // Add/adjust CF attributes
        f->Adjust_Attr_Info();
        f->Adjust_Obj_Name();
        f->Flatten_Obj_Name(include_attr);
        if (true == is_check_nameclashing) 
           f->Handle_Obj_NameClashing(include_attr);
        f->Set_COARDS_Status();

        // Add supplemental attributes
        f->Add_Supplement_Attrs(is_add_path_attrs);

        // Handle coordinate attributes
        f->Handle_Coor_Attr();
    }
    catch (HDF5CF::Exception &e){
        delete f;
        throw InternalErr(e.what());
    }


    gen_eos5_cfdas(das,file_id,f);

    delete f;

}

void gen_eos5_cfdds(DDS &dds,  HDF5CF::EOS5File *f) {

    const vector<HDF5CF::Var *>& vars       = f->getVars();
    const vector<HDF5CF::EOS5CVar *>& cvars = f->getCVars();
    const string filename = f->getPath();

    // Read Variable info.
    vector<HDF5CF::Var *>::const_iterator it_v;
    vector<HDF5CF::EOS5CVar *>::const_iterator it_cv;

    for (it_v = vars.begin(); it_v !=vars.end();++it_v) {
        // cerr <<"variable full path= "<< (*it_v)->getFullPath() <<endl;
        gen_dap_onevar_dds(dds,*it_v,filename);
    }

    for (it_cv = cvars.begin(); it_cv !=cvars.end();++it_cv) {
        // cerr <<"variable full path= "<< (*it_cv)->getFullPath() <<endl;
//gen_dap_onevar_dds(dds,*it_v,filename);
        gen_dap_oneeos5cvar_dds(dds,*it_cv,filename);

    }

}

void gen_dap_oneeos5cvar_dds(DDS &dds,const HDF5CF::EOS5CVar* cvar, const string & filename) {

    BaseType *bt = NULL;

    switch(cvar->getType()) {
#define HANDLE_CASE(tid,type)                                  \
        case tid:                                           \
            bt = new (type)(cvar->getNewName(),cvar->getFullPath());  \
        break;
    // FIXME bt leaked by throw
    // James, I don't know why bt is leaked below.  Since here we basically
    // follow the netCDF handler(ncdds.cc), could you give us some advice?
    // If it is still causing potential leaks, we can fix this in the next release.
    // KY 2012-09-28

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

        switch(cvar->getCVType()) {

            case CV_EXIST:
            {
                HDF5CFArray *ar = NULL;
                ar = new HDF5CFArray (
                                    cvar->getRank(),
                                    filename,
                                    cvar->getType(),
                                    cvar->getFullPath(),
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

            case CV_LAT_MISS:
            case CV_LON_MISS:
            {

                HDFEOS5CFMissLLArray *ar = NULL;
                ar = new HDFEOS5CFMissLLArray (
                                    cvar->getRank(),
                                    filename,
                                    cvar->getFullPath(),
                                    cvar->getCVType(),
                                    cvar->getPointLower(),
                                    cvar->getPointUpper(),
                                    cvar->getPointLeft(),
                                    cvar->getPointRight(),
                                    cvar->getPixelReg(),
                                    cvar->getOrigin(),
                                    cvar->getProjCode(),
                                    cvar->getXDimSize(),
                                    cvar->getYDimSize(),
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

            case CV_NONLATLON_MISS:
            {

                if (cvar->getRank() !=1) {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                }
                int nelem = (cvar->getDimensions()[0])->getSize();

                HDFEOS5CFMissNonLLCVArray *ar = NULL;
                ar = new HDFEOS5CFMissNonLLCVArray(
                                                    cvar->getRank(),
                                                    nelem,
                                                    cvar->getNewName(),
                                                    bt);

                for(it_d = dims.begin(); it_d != dims.end(); it_d++) {
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
                // Currently only support Aura TES files. May need to revise when having more
                // special products KY 2012-2-3
            {
                if (cvar->getRank() !=1) {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                }
                int nelem = (cvar->getDimensions()[0])->getSize();
                HDFEOS5CFSpecialCVArray *ar = NULL;
                ar = new HDFEOS5CFSpecialCVArray(
                                                      cvar->getRank(),
                                                      filename,
                                                      cvar->getType(),
                                                      nelem,
                                                      cvar->getFullPath(),
                                                      cvar->getNewName(),
                                                      bt);

                for(it_d = dims.begin(); it_d != dims.end(); ++it_d){
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
                    throw InternalErr(__FILE__,__LINE__,"Unsupported coordinate variable type.");
        }

    }
        
}


void gen_eos5_cfdas(DAS &das, hid_t file_id, HDF5CF::EOS5File *f) {

    // cerr<<"read_cfdas()"<<endl;

    const vector<HDF5CF::Var *>& vars             = f->getVars();
    const vector<HDF5CF::EOS5CVar *>& cvars       = f->getCVars();
    const vector<HDF5CF::Group *>& grps           = f->getGroups();
    const vector<HDF5CF::Attribute *>& root_attrs = f->getAttributes();

    vector<HDF5CF::Var *>::const_iterator it_v;
    vector<HDF5CF::EOS5CVar *>::const_iterator it_cv;
    vector<HDF5CF::Group *>::const_iterator it_g;
    vector<HDF5CF::Attribute *>::const_iterator it_ra;

    // Handling the file attributes(attributes under the root group)
    // The table name is "HDF_GLOBAL".
    if (false == root_attrs.empty()) {
        AttrTable *at = das.get_table(FILE_ATTR_TABLE_NAME);
        if (NULL == at)
            at = das.add_table(FILE_ATTR_TABLE_NAME, new AttrTable);

        for (it_ra = root_attrs.begin(); it_ra != root_attrs.end(); it_ra++) {
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
                gen_dap_oneobj_das(at,*it_ra,NULL);
            }
        }
    }

    for (it_v = vars.begin();
         it_v != vars.end(); ++it_v) {
        if (false == ((*it_v)->getAttributes().empty())) {

            AttrTable *at = das.get_table((*it_v)->getNewName());
            if (NULL == at)
                at = das.add_table((*it_v)->getNewName(), new AttrTable);

            for (it_ra = (*it_v)->getAttributes().begin();
                 it_ra != (*it_v)->getAttributes().end(); ++it_ra) {
                gen_dap_oneobj_das(at,*it_ra,*it_v);
            }
        }
    }
 
    for (it_cv = cvars.begin(); it_cv !=cvars.end();it_cv++) {
        // cerr <<"variable full path= "<< (*it_cv)->getFullPath() <<endl;

        if (false == ((*it_cv)->getAttributes().empty())) {

            AttrTable *at = das.get_table((*it_cv)->getNewName());
            if (NULL == at)
                at = das.add_table((*it_cv)->getNewName(), new AttrTable);

            for (it_ra = (*it_cv)->getAttributes().begin();
                 it_ra != (*it_cv)->getAttributes().end(); ++it_ra) {
                 gen_dap_oneobj_das(at,*it_ra,*it_cv);
            }
        }
    }


    // To keep the backward compatiablity with the old handler,
    // we parse the special ECS metadata to DAP attributes

    string st_str ="";
    string core_str="";
    string arch_str="";
    string xml_str ="";
    string subset_str="";
    string product_str="";
    string other_str ="";
    bool st_only = false;

    read_ecs_metadata(file_id, st_str, core_str, arch_str, xml_str,
                          subset_str, product_str, other_str, st_only);

#if 0
if(st_str!="") cerr <<"Final structmetadata "<<st_str <<endl;
if(core_str!="") cerr <<"Final coremetadata "<<core_str <<endl;
if(arch_str!="") cerr <<"Final archivedmetadata "<<arch_str <<endl;
if(xml_str!="") cerr <<"Final xmlmetadata "<<xml_str <<endl;
if(subset_str!="") cerr <<"Final subsetmetadata "<<subset_str <<endl;
if(product_str!="") cerr <<"Final productmetadata "<<product_str <<endl;
if(other_str!="") cerr <<"Final othermetadata "<<other_str <<endl;

#endif
    if(st_str != ""){

        string check_disable_smetadata_key ="H5.DisableStructMetaAttr";
        bool is_check_disable_smetadata = false;
        is_check_disable_smetadata = HDF5CFDAPUtil::check_beskeys(check_disable_smetadata_key);

        if (false == is_check_disable_smetadata) {

            AttrTable *at = das.get_table("StructMetadata");
            if (NULL == at)
                at = das.add_table("StructMetadata", new AttrTable);
            parser_arg arg(at);

            he5das_scan_string((const char*) st_str.c_str());
            if (he5dasparse(&arg) != 0
                || false == arg.status()){
                cerr << "HDF-EOS StructMetdata parse error!\n";
            }

        }
    }

    if(core_str != ""){
        AttrTable *at = das.get_table("CoreMetadata");
        if (NULL == at)
            at = das.add_table("CoreMetadata", new AttrTable);
        parser_arg arg(at);
        he5das_scan_string((const char*) core_str.c_str());
        if (he5dasparse(&arg) != 0
                || false == arg.status()){
            cerr << "HDF-EOS CoreMetadata parse error!\n";
        }
    }
    if(arch_str != ""){
        AttrTable *at = das.get_table("ArchiveMetadata");
        if (NULL == at)
            at = das.add_table("ArchiveMetadata", new AttrTable);
        parser_arg arg(at);
        he5das_scan_string((const char*) arch_str.c_str());
        if (he5dasparse(&arg) != 0
            || false == arg.status()){
                cerr << "HDF-EOS ArchiveMetdata parse error!\n";
        }
    }

    // XML attribute includes double quote("), this will choke netCDF Java library.
    // So we replace double_quote(") with &quote.This is currently the OPeNDAP way.
    // XML attribute cannot parsed. So just pass the string.
    if(xml_str != ""){
        AttrTable *at = das.get_table("XMLMetadata");
        if (NULL == at)
            at = das.add_table("XMLMetadata", new AttrTable);
        HDF5CFDAPUtil::replace_double_quote(xml_str);
        at->append_attr("Contents","String",xml_str);
    }

    // SubsetMetadata and ProductMetadata exist in HDF-EOS2 files.
    // So far we haven't found any metadata in NASA HDF-EOS5 files,
    // but will keep an eye on it. KY 2012-3-6
    if(subset_str != ""){
        AttrTable *at = das.get_table("SubsetMetadata");
        if (NULL == at)
            at = das.add_table("SubsetMetadata", new AttrTable);
        parser_arg arg(at);
        he5das_scan_string((const char*) subset_str.c_str());
        if (he5dasparse(&arg) != 0
                || false == arg.status()) {
            cerr << "HDF-EOS SubsetMetadata parse error!\n";
        }
    }
    if(product_str != ""){
        AttrTable *at = das.get_table("ProductMetadata");
        if (NULL == at)
            at = das.add_table("ProductMetadata", new AttrTable);
        parser_arg arg(at);
        he5das_scan_string((const char*) product_str.c_str());
        if (he5dasparse(&arg) != 0
                || false == arg.status()){
            cerr << "HDF-EOS ProductMetadata parse error!\n";
        }
    }

    // All other metadata under "HDF-EOS Information" will not be
    // parsed since we don't know how to parse them.
    // We will simply pass a string to the DAS.
    if (other_str != ""){
        AttrTable *at = das.get_table("OtherMetadata");
        if (NULL == at)
            at = das.add_table("OtherMetadata", new AttrTable);
        at->append_attr("Contents","String",other_str);
    }

}


void read_ecs_metadata(hid_t s_file_id, 
                       string &total_strmeta_value,
                       string &total_coremeta_value,
                       string &total_archmeta_value, 
                       string &total_xmlmeta_value, 
                       string &total_submeta_value, 
                       string &total_prometa_value,
                       string &total_othermeta_value,
                       bool s_st_only) {

    string ecs_group = "/HDFEOS INFORMATION";
    hid_t ecs_grp_id = -1;
    if ((ecs_grp_id = H5Gopen(s_file_id, ecs_group.c_str(),H5P_DEFAULT))<0) {
        // cerr<<"cannot open the group "<<endl;
        string msg =
            "h5_ecs_meta: unable to open the HDF5 group  ";
        msg +=ecs_group;
        H5Fclose(s_file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    H5G_info_t g_info;
    hsize_t nelems = 0;

    if (H5Gget_info(ecs_grp_id,&g_info) <0) {
       string msg =
            "h5_ecs_meta: unable to obtain the HDF5 group info. for ";
        msg +=ecs_group;
        H5Gclose(ecs_grp_id);
        H5Fclose(s_file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    nelems = g_info.nlinks;

    ssize_t oname_size      = 0;
#if 0
    int cur_archmeta_suffix = 0;
    int cur_coremeta_suffix = 0;
    int cur_strmeta_suffix  = 0;
    int cur_xmlmeta_suffix  = 0;
#endif

    int archmeta_num        = -1;
    int coremeta_num        = -1;
    int xmlmeta_num         = -1;
    int strmeta_num         = -1;
    int submeta_num         = -1;
    int prometa_num         = -1;

    // Initalize the total number for different metadata.
    int archmeta_num_total  = 0;
    int coremeta_num_total  = 0;
    int xmlmeta_num_total   = 0;
    int strmeta_num_total   = 0;
    int submeta_num_total   = 0;
    int prometa_num_total   = 0;
    int othermeta_num_total = 0;
        
    bool archmeta_no_suffix = true;
    bool coremeta_no_suffix = true;
    bool strmeta_no_suffix  = true;
    bool xmlmeta_no_suffix  = true;
    bool submeta_no_suffix  = true;
    bool prometa_no_suffix  = true;

    // Define a vector of string to hold all dataset names.
    vector<string> s_oname(nelems);

    // Define an EOSMetadata array that can describe the metadata type for each object
    // We initialize the value to OtherMeta.
    EOS5Metadata metatype[nelems];
    for (unsigned int i =0; i<nelems; i++) 
        metatype[i] = OtherMeta;

    for (hsize_t i = 0; i < nelems; i++) {

        // Query the length of the object name.
        oname_size =
            H5Lget_name_by_idx(ecs_grp_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,NULL,
                0, H5P_DEFAULT); 
        if (oname_size <= 0) {
            string msg = "hdf5 object name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the name of the object.
        vector<char> oname(oname_size + 1);
        if (H5Lget_name_by_idx(ecs_grp_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,&oname[0],
                (size_t)(oname_size+1), H5P_DEFAULT)<0){
            string msg = "hdf5 object name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Check if this object is an HDF5 dataset, not, throw an error.
        // First, check if it is the hard link or the soft link
        H5L_info_t linfo;
        if (H5Lget_info(ecs_grp_id,&oname[0],&linfo,H5P_DEFAULT)<0) {
            string msg = "hdf5 link name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // This is the soft link.
        if (linfo.type == H5L_TYPE_SOFT){
            string msg = "hdf5 link name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the object type
        H5O_info_t oinfo;
        if (H5Oget_info_by_idx(ecs_grp_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
            string msg = "Cannot obtain the object info ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if(oinfo.type != H5O_TYPE_DATASET) {
            string msg = "hdf5 link name error from: ";
            msg += ecs_group;
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }
 
        // We want to remove the last '\0' character added by C .
        string s_one_oname(oname.begin(),oname.end()-1);
        // cerr <<"full object name "<<s_one_oname <<endl;
        s_oname[i] = s_one_oname;

        // Calculate how many elements we have for each category(StructMetadata, CoreMetadata, etc.)
        if (((s_one_oname.find("StructMetadata"))==0) ||
           ((s_one_oname.find("structmetadata"))==0)){

            metatype[i] = StructMeta;

            // Do we have suffix for the metadata?
            // If this metadata doesn't have any suffix, it should only come to this loop once.
            // That's why, when checking the first time, no_suffix is always true.
            // If we have already found that it doesn't have any suffix,
            // it should not go into this loop. throw an error.
            if (false == strmeta_no_suffix) {
                string msg = "StructMetadata/structmetadata without suffix should only appear once. ";
                H5Gclose(ecs_grp_id);
                H5Fclose(s_file_id);
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
cerr <<"strmeta_num_total= "<<strmeta_num_total <<endl;
if(strmeta_no_suffix) cerr <<"structmeta data has the suffix" <<endl;
else cerr <<"structmeta data doesn't have the suffix" <<endl;
#endif
        }

        if(false == s_st_only) {

            if ((0 == (s_one_oname.find("CoreMetadata"))) ||
                (0 == (s_one_oname.find("coremetadata")))){

                metatype[i] = CoreMeta;
                // Do we have suffix for the metadata?
                // When checking the first time, no_suffix is always true.
                // If we have already found that it doesn't have any suffix,
                // it should not go into this loop anyway. throw an error.
                if (false == coremeta_no_suffix) {
                    string msg = "CoreMetadata/coremetadata without suffix should only appear once. ";
                    H5Gclose(ecs_grp_id);
                    H5Fclose(s_file_id);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                else if(coremeta_num_total >0) coremeta_num_total++;
                else { // either no suffix or the first time to loop the one having the suffix.   
                   // If no suffix is true, it should be out of the loop. In case it comes 
                   // to the loop again,   we set  "coremeta_no_suffix" be false so an error
                   // can be thrown. This is counter-intutitive. Hopefully people can understand it.
                   if ((0 == s_one_oname.compare("CoreMetadata")) ||
                       (0 == s_one_oname.compare("coremetadata")))
                       coremeta_no_suffix = false;
                   else coremeta_num_total++;
                }
#if 0
cerr <<"coremeta_num_total= "<<coremeta_num_total <<endl;
if(coremeta_no_suffix) cerr <<"coreuctmeta data has the suffix" <<endl;
else cerr <<"coremeta data doesn't have the suffix" <<endl;
#endif
            }

            // OMI has the metadata name as "ArchiveMetadata.0"
            else if ((0 == (s_one_oname.find("ArchivedMetadata"))) ||
                     (0 == (s_one_oname.find("archivedmetadata"))) ||
                     (0 == (s_one_oname.find("ArchiveMetadata"))) || 
                     (0 == (s_one_oname.find("archivemetadata")))){

                metatype[i] = ArchivedMeta;
                // Do we have suffix for the metadata?
                // When checking the first time, no_suffix is always true.
                // If we have already found that it doesn't have any suffix,
                // it should not go into this loop anyway. throw an error.
                if (false == archmeta_no_suffix) {
                    string msg = "archivedmetadata/ArchivedMetadata without suffix should only appear once. ";
                    H5Gclose(ecs_grp_id);
                    H5Fclose(s_file_id);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                else if(archmeta_num_total >0) archmeta_num_total++;
                else { // either no suffix or the first time to loop the one having the suffix.   
                    if ((0 == s_one_oname.compare("ArchivedMetadata"))||
                       (0 == s_one_oname.compare("archivedmetadata")) || 
                       (0 == s_one_oname.compare("archivemetadata")) ||
                       (0 == s_one_oname.compare("ArchiveMetadata")))
                        archmeta_no_suffix = false;
                    else archmeta_num_total++;
                }
#if 0
cerr <<"archmeta_num_total= "<<archmeta_num_total <<endl;
if(archmeta_no_suffix) cerr <<"archuctmeta data has the suffix" <<endl;
else cerr <<"archmeta data doesn't have the suffix" <<endl;
#endif

            }

            else if (((s_one_oname.find("SubsetMetadata"))==0) ||
                     ((s_one_oname.find("subsetmetadata"))==0)){

                metatype[i] = SubsetMeta;
                // Do we have suffix for the metadata?
                // When checking the first time, no_suffix is always true.
                // If we have already found that it doesn't have any suffix,
                // it should not go into this loop anyway. throw an error.
                if (false == submeta_no_suffix) {
                    H5Gclose(ecs_grp_id);
                    H5Fclose(s_file_id);
                    string msg = "submetadata/SubMetadata without suffix should only appear once. ";
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                else if(submeta_num_total >0) 
                    submeta_num_total++;
                else { // either no suffix or the first time to loop the one having the suffix.   
                    if ((0 == s_one_oname.compare("SubsetMetadata"))||
                        (0 == s_one_oname.compare("subsetmetadata")))
                        submeta_no_suffix = false;
                    else submeta_num_total++;
                }
#if 0
cerr <<"submeta_num_total= "<<submeta_num_total <<endl;
if(submeta_no_suffix) cerr <<"subuctmeta data has the suffix" <<endl;
else cerr <<"submeta data doesn't have the suffix" <<endl;
#endif

            }

            else if ((0 == (s_one_oname.find("XmlMetadata"))) ||
                     (0 == (s_one_oname.find("xmlmetadata")))){

                metatype[i] = XMLMeta;

                // Do we have suffix for the metadata?
                // When checking the first time, no_suffix is always true.
                // If we have already found that it doesn't have any suffix,
                // it should not go into this loop anyway. throw an error.
                if (false == xmlmeta_no_suffix) {
                    H5Gclose(ecs_grp_id);
                    H5Fclose(s_file_id);
                    string msg = "xmlmetadata/Xmlmetadata without suffix should only appear once. ";
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                else if(xmlmeta_num_total >0) 
                    xmlmeta_num_total++;
                else { // either no suffix or the first time to loop the one having the suffix.   
                    if ((0 == s_one_oname.compare("XmlMetadata"))||
                        (0 == s_one_oname.compare("xmlmetadata")))
                        xmlmeta_no_suffix = false;
                    else xmlmeta_num_total++;
                }
#if 0
cerr <<"xmlmeta_num_total= "<<xmlmeta_num_total <<endl;
if(xmlmeta_no_suffix) cerr <<"xmluctmeta data doesn't have the suffix" <<endl;
else cerr <<"xmlmeta data has the suffix" <<endl;
#endif

            }

            else if ((0 == (s_one_oname.find("ProductMetadata"))) ||
                     (0 == (s_one_oname.find("productmetadata")))){

                metatype[i] = ProductMeta;
                // Do we have suffix for the metadata?
                // When checking the first time, no_suffix is always true.
                // If we have already found that it doesn't have any suffix,
                // it should not go into this loop anyway. throw an error.
                if (!prometa_no_suffix) {
                    H5Gclose(ecs_grp_id);
                    H5Fclose(s_file_id);
                    string msg = "productmetadata/ProductMetadata without suffix should only appear once. ";
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                else if(prometa_num_total >0) prometa_num_total++;
                else { // either no suffix or the first time to loop the one having the suffix.   
                   if ((0 == s_one_oname.compare("ProductMetadata"))||
                       (0 == s_one_oname.compare("productmetadata")))
                       prometa_no_suffix = false;
                   else prometa_num_total++;
                }

            }

        // All other metadata will be merged to one string, no need to check the name.
            else othermeta_num_total++;
        }

        // cerr <<"object name using s_oname is " <<s_oname[i] <<endl;
        oname.clear();
        s_one_oname.clear();
    }

    // Define a vector of string to hold StructMetadata.
    // StructMetadata must exist for a valid HDF-EOS5 file.
    vector<string> strmeta_value;
    if (strmeta_num_total <= 0) {
        string msg = "hdf5 object name error from: ";
        H5Gclose(ecs_grp_id);
        H5Fclose(s_file_id);
        throw InternalErr(__FILE__, __LINE__, msg);

        //     cerr <<"Struct Metadata doesn't exist."<<endl;
    }
    else {
        strmeta_value.resize(strmeta_num_total);
        for (int i = 0; i < strmeta_num_total; i++) 
            strmeta_value[i]="";
    }

    // All other metadata are optional.
    // Define a vector of string to hold archivedmetadata.
    vector<string> archmeta_value;
    if (archmeta_num_total >0) {
        archmeta_value.resize(archmeta_num_total);
        for (int i = 0; i < archmeta_num_total; i++) 
            archmeta_value[i]="";
    }
    
    // Define a vector of string to hold coremetadata.
    vector<string> coremeta_value;
    // cerr<<"coremeta_num_total "<<coremeta_num_total <<endl;
    if (coremeta_num_total >0) {
        coremeta_value.resize(coremeta_num_total);
        for (int i = 0; i < coremeta_num_total; i++) 
            coremeta_value[i]="";
    }

    // Define a vector of string to hold xmlmetadata.
    vector<string> xmlmeta_value;
    if (xmlmeta_num_total >0) {
        xmlmeta_value.resize(xmlmeta_num_total);
        for (int i = 0; i < xmlmeta_num_total; i++) 
            xmlmeta_value[i]="";
    }

    // Define a vector of string to hold subsetmetadata.
    vector<string> submeta_value;
    if (submeta_num_total >0) {
        submeta_value.resize(submeta_num_total);
        for (int i = 0; i < submeta_num_total; i++) 
            submeta_value[i]="";
    }

    // Define a vector of string to hold productmetadata.
    vector<string> prometa_value;
    if (prometa_num_total >0) {
        prometa_value.resize(prometa_num_total);
        for (int i = 0; i < prometa_num_total; i++) 
            prometa_value[i]="";
    }

    // For all other metadata, we don't need to calculate the value, just append them.
//    string total_othermeta_value ="";

    // Now we want to retrieve the metadata value and combine them into one string.
    // Here we have to remember the location of every element of the metadata if
    // this metadata has a suffix.
    for (hsize_t i = 0; i < nelems; i++) {

        // cerr <<"object name "<<s_oname[i] <<endl;
         // DDS parser only needs to parse the struct Metadata. So check
        // if st_only flag is true, will only read StructMetadata string.
        // Struct Metadata is generated by the HDF-EOS5 library, so the
        // name "StructMetadata.??" won't change for real struct metadata. 
        //However, we still assume that somebody may not use the HDF-EOS5
        // library to add StructMetadata, the name may be "structmetadata".
        if (true == s_st_only &&
           (((s_oname[i].find("StructMetadata"))!=0) && 
            ((s_oname[i].find("structmetadata"))!=0))){
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
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if ((s_space_id = H5Dget_space(s_dset_id))<0) {
            string msg = "Cannot open the data space of HDF5 dataset  ";
            msg += s_oname[i];
            H5Dclose(s_dset_id);
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if ((s_ty_id = H5Dget_type(s_dset_id)) < 0) {
            string msg = "Cannot get the data type of HDF5 dataset  ";
            msg += s_oname[i];
            H5Sclose(s_space_id);
            H5Dclose(s_dset_id);
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        if ((s_nelms = H5Sget_simple_extent_npoints(s_space_id))<0) {
            string msg = "Cannot get the number of points of HDF5 dataset  ";
            msg += s_oname[i];
            H5Tclose(s_ty_id);
            H5Sclose(s_space_id);
            H5Dclose(s_dset_id);
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        if ((dtype_size = H5Tget_size(s_ty_id))<0) {

            string msg = "Cannot get the data type size of HDF5 dataset  ";
            msg += s_oname[i];
            H5Tclose(s_ty_id);
            H5Sclose(s_space_id);
            H5Dclose(s_dset_id);
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Obtain the real value of the metadata
        vector<char> s_buf(dtype_size*s_nelms +1);

        if ((H5Dread(s_dset_id,s_ty_id,H5S_ALL,H5S_ALL,H5P_DEFAULT,&s_buf[0]))<0) {

            string msg = "Cannot read HDF5 dataset  ";
            msg += s_oname[i];
            H5Tclose(s_ty_id);
            H5Sclose(s_space_id);
            H5Dclose(s_dset_id);
            H5Gclose(ecs_grp_id);
            H5Fclose(s_file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Convert from the vector<char> to a C++ string.
        string tempstr(s_buf.begin(),s_buf.end());
        s_buf.clear();
        size_t temp_null_pos = tempstr.find_first_of('\0');

        // temp_null_pos returns the position of NULL,which is the last character of the string. 
        // so the length of string before null is EQUAL to
        // temp_null_pos since pos starts at 0.
        string finstr = tempstr.substr(0,temp_null_pos);

        // For the DDS parser, only return StructMetadata
        if (StructMeta == metatype[i]) {

            // Now obtain the corresponding value in integer type for the suffix. '0' to 0 etc. 
            strmeta_num = get_metadata_num(s_oname[i]);

            // This is probably not necessary, since structmetadata may always have a suffix.           
            // Leave here just in case the rules change or a special non-HDF-EOS5 library generated file.
            // when strmeta_num is -1, it means no suffix for this metadata. So the total structmetadata
            // is this string only.
            if (-1 == strmeta_num) 
                total_strmeta_value = finstr;
            // strmeta_value at this point should be empty before assigning any values.
            else if (strmeta_value[strmeta_num]!="") {
                string msg = "The structmeta value array at this index should be empty string  ";
                H5Tclose(s_ty_id);
                H5Sclose(s_space_id);
                H5Dclose(s_dset_id);
                H5Gclose(ecs_grp_id);
                H5Fclose(s_file_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        // assign the string vector to this value.
            else 
                strmeta_value[strmeta_num] = finstr;
        }

        // DAS parser needs all metadata.
        if (false == s_st_only && 
            (metatype[i] != StructMeta)) {

            switch (metatype[i]) {
                    
                case CoreMeta:
                {
                    if (coremeta_num_total < 0) {
                        string msg = "There may be no coremetadata or coremetadata is not counted ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);

                    }
                    coremeta_num = get_metadata_num(s_oname[i]);

                    // when coremeta_num is -1, it means no suffix for this metadata. So the total coremetadata
                    // is this string only. Similar cases apply for the rest metadata.
                    if ( -1 == coremeta_num ) 
                        total_coremeta_value = finstr;    
                    else if (coremeta_value[coremeta_num]!="") {
                        string msg = "The coremeta value array at this index should be empty string  ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);
                    }
 
                    // assign the string vector to this value.
                    else 
                        coremeta_value[coremeta_num] = finstr;
                }
                    break;
 
                case ArchivedMeta:
                {
                    if (archmeta_num_total < 0) {
                        string msg = "There may be no archivemetadata or archivemetadata is not counted ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);
                    }
                    archmeta_num = get_metadata_num(s_oname[i]);
                    if (-1 == archmeta_num ) 
                        total_archmeta_value = finstr;    
                    else if (archmeta_value[archmeta_num]!="") {
                        string msg = "The archivemeta value array at this index should be empty string  ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);

                    }
                         // assign the string vector to this value.
                    else 
                        archmeta_value[archmeta_num] = finstr;
                }
                    break;
                case SubsetMeta:
                {
                    if (submeta_num_total < 0) {
                        string msg = "The subsetemeta value array at this index should be empty string  ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);
                    }
                    submeta_num = get_metadata_num(s_oname[i]);
                    if (-1 == submeta_num ) 
                        total_submeta_value = finstr;     
                    else if (submeta_value[submeta_num]!="") {
                        string msg = "The submeta value array at this index should be empty string  ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);
                    }
                         // assign the string vector to this value.
                    else 
                        submeta_value[submeta_num] = finstr;
                }
                    break;
                case ProductMeta:
                {
                    if (prometa_num_total < 0) {
                        string msg = "There may be no productmetadata or productmetadata is not counted ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);
                    }
                    prometa_num = get_metadata_num(s_oname[i]);
                    if (prometa_num == -1) 
                        total_prometa_value = finstr;
                    else if (prometa_value[prometa_num]!="") {
                        string msg = "The productmeta value array at this index should be empty string  ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);
                    }
                    // assign the string vector to this value.
                    else 
                        prometa_value[prometa_num] = finstr;
                }
                    break;
                case XMLMeta:
                {
                    if (xmlmeta_num_total < 0) {
                        string msg = "There may be no xmlmetadata or xmlmetadata is not counted ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);
                    }
                    xmlmeta_num = get_metadata_num(s_oname[i]);
                    if (-1 == xmlmeta_num ) 
                        total_xmlmeta_value = finstr;
                    else if (xmlmeta_value[xmlmeta_num]!="") {
                        string msg = "The xmlmeta value array at this index should be empty string  ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);
                    }
                    // assign the string vector to this value.
                    else 
                        xmlmeta_value[xmlmeta_num] = finstr;
                }
                    break;
                case OtherMeta:
                {
                    if (othermeta_num_total < 0) {
                        string msg = "There may be no coremetadata or coremetadata is not counted ";
                        H5Tclose(s_ty_id);
                        H5Sclose(s_space_id);
                        H5Dclose(s_dset_id);
                        H5Gclose(ecs_grp_id);
                        H5Fclose(s_file_id);
                        throw InternalErr(__FILE__, __LINE__, msg);
                    }
                    total_othermeta_value = total_othermeta_value + finstr;
                }
                    break;
                default :
                {
                     string msg = "Unsupported metadata type ";
                     H5Tclose(s_ty_id);
                     H5Sclose(s_space_id);
                     H5Dclose(s_dset_id);
                     H5Gclose(ecs_grp_id);
                     H5Fclose(s_file_id); 
                     throw InternalErr(__FILE__, __LINE__, msg);
                }
            }
        }
        tempstr.clear();
        finstr.clear();
        H5Sclose(s_space_id);
        H5Tclose(s_ty_id);
        H5Dclose(s_dset_id);
    }

    // Now we need to handle the concatenation of the metadata
    // first StructMetadata
    if (strmeta_num_total > 0) {
        // The no suffix one has been taken care.
        if (strmeta_num != -1) {
            for (int i = 0; i <strmeta_num_total; i++) 
                total_strmeta_value +=strmeta_value[i];
        }
    }

    // For the DAS handler
    if ( false == s_st_only) {

        if (coremeta_num_total >0) {
            if (coremeta_num != -1) {
                for(int i = 0; i <coremeta_num_total; i++) 
                    total_coremeta_value +=coremeta_value[i];
            }
        }
       
        if (archmeta_num_total >0) {
            if (archmeta_num != -1) { 
                for(int i = 0; i <archmeta_num_total; i++) 
                    total_archmeta_value +=archmeta_value[i];
            }
        }

        if (submeta_num_total >0) {
            if (submeta_num != -1) {
                for(int i = 0; i <submeta_num_total; i++) 
                    total_submeta_value +=submeta_value[i];
            }
        }
       
        if (xmlmeta_num_total >0) {
            if (xmlmeta_num != -1) { 
                for(int i = 0; i <xmlmeta_num_total; i++) 
                    total_xmlmeta_value +=xmlmeta_value[i];
            }
        }

        if (prometa_num_total >0) {
            if (prometa_num != -1) {
                for(int i = 0; i <prometa_num_total; i++) 
                    total_prometa_value +=prometa_value[i];
            }
        }
    }
    H5Gclose(ecs_grp_id);
}

int get_metadata_num(const string & meta_str) {

    // The normal metadata names should be like coremetadata.0, coremetadata.1 etc.
    // We just find a not so nice coremetadata names such as coremetadata.0, coremetadata.0.1 for a HIRDLS-MLS-Aura-L3
    // We need to handle them. Here we assume no more than two dots in a name series. KY 2012-11-08
    size_t dot_pos = meta_str.find(".");
    if (dot_pos == string::npos) // No dot
        return -1;
    else if (meta_str.find_first_of(".") == meta_str.find_last_of(".")) { // One dot
        string num_str = meta_str.substr(dot_pos+1);
        stringstream ssnum(num_str);
        int num;
        ssnum >> num;
        if (false == ssnum) 
            throw InternalErr(__FILE__,__LINE__,"Suffix after dots is not a number.");
        return num;
    }
    else { // Two dots
        string str_after_first_dot = meta_str.substr(dot_pos+1);
        if (str_after_first_dot.find_first_of(".") != str_after_first_dot.find_last_of("."))    
            throw InternalErr(__FILE__,__LINE__,"Currently don't support metadata names containing more than two dots.");
        // Here we don't check if names are like coremetadata.0 coremetadata.0.0 etc., Having ".0 .0.0" is,if not mistaken,
        // is insane. 
        // Instead we hope that the data producers will produce data like coremetadata.0 coremetadata.0.1 coremeatadata.0.2
        // KY 2012-11-08
        size_t second_dot_pos = str_after_first_dot.find(".");
        string num_str = str_after_first_dot.substr(second_dot_pos+1);
        stringstream ssnum(num_str);
        int num;
        ssnum >> num;
        return num;
    }
        
}
       
       



