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
/// \file h5commoncfdap.cc
/// \brief Functions to generate DDS and DAS for one object(variable). 
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

#include <InternalErr.h>
#include <BESDebug.h>

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
#include "HDF5CFInt64.h"
#include "HDF5CFUInt64.h"
#include "HDF5CFStr.h"
#include "HDF5CFArray.h"
#include "HDF5CFGeoCF1D.h"
#include "HDF5CFGeoCFProj.h"

#include "HDF5Int64.h"

using namespace std;
using namespace libdap;
using namespace HDF5CF;

// Generate DDS from one variable
void gen_dap_onevar_dds(DDS &dds, const HDF5CF::Var* var, const hid_t file_id, const string & filename)
{

    BESDEBUG("h5", "Coming to gen_dap_onevar_dds()  "<<endl);
    const vector<HDF5CF::Dimension *>& dims = var->getDimensions();

    if (0 == dims.size()) {
        // Adding 64-bit integer support for DMR 
        if (H5INT64 == var->getType() || H5UINT64 == var->getType()){
            DMR * dmr = HDF5RequestHandler::get_dmr_64bit_int();
            if(dmr == NULL)
                return;
            else {
                D4Group* root_grp = dmr->root();
                if(H5INT64 == var->getType()) {
                    HDF5CFInt64 *sca_int64 = NULL;
                    try {
                        sca_int64 = new HDF5CFInt64(var->getNewName(), var->getFullPath(), filename);
                    }
                    catch (std::exception &e) {
                        string error_message = e.what();
                        error_message = "Cannot allocate the HDF5CFInt64: " + error_message;
                        throw InternalErr(__FILE__, __LINE__, error_message);
                    }
                    sca_int64->set_is_dap4(true);
                    map_cfh5_attrs_to_dap4(var,sca_int64);
                    root_grp->add_var_nocopy(sca_int64);
 
                }
                else if(H5UINT64 == var->getType()) {
                    HDF5CFUInt64 *sca_uint64 = NULL;
                    try {
                        sca_uint64 = new HDF5CFUInt64(var->getNewName(), var->getFullPath(), filename);
                    }
                    catch (...) {
                        throw InternalErr(__FILE__, __LINE__, "Cannot allocate the HDF5CFInt64.");
                    }
                    sca_uint64->set_is_dap4(true);
                    map_cfh5_attrs_to_dap4(var,sca_uint64);
                    root_grp->add_var_nocopy(sca_uint64);
 
                }

            }
        }
        else if (H5FSTRING == var->getType() || H5VSTRING == var->getType()) {
            HDF5CFStr *sca_str = NULL;
            try {
                sca_str = new HDF5CFStr(var->getNewName(), filename, var->getFullPath());
            }
            catch (...) {
                throw InternalErr(__FILE__, __LINE__, "Cannot allocate the HDF5CFStr.");
            }
            dds.add_var(sca_str);
            delete sca_str;
        }
        else {
            switch (var->getType()) {

            case H5UCHAR: {
                HDF5CFByte * sca_uchar = NULL;
                try {
                    sca_uchar = new HDF5CFByte(var->getNewName(), var->getFullPath(), filename);
                }
                catch (...) {
                    throw InternalErr(__FILE__, __LINE__, "Cannot allocate the HDF5CFByte.");
                }
                dds.add_var(sca_uchar);
                delete sca_uchar;

            }
                break;
            case H5CHAR:
            case H5INT16: {
                HDF5CFInt16 * sca_int16 = NULL;
                try {
                    sca_int16 = new HDF5CFInt16(var->getNewName(), var->getFullPath(), filename);
                }
                catch (...) {
                    throw InternalErr(__FILE__, __LINE__, "Cannot allocate the HDF5CFInt16.");
                }
                dds.add_var(sca_int16);
                delete sca_int16;
            }
                break;
            case H5UINT16: {
                HDF5CFUInt16 * sca_uint16 = NULL;
                try {
                    sca_uint16 = new HDF5CFUInt16(var->getNewName(), var->getFullPath(), filename);
                }
                catch (...) {
                    throw InternalErr(__FILE__, __LINE__, "Cannot allocate the HDF5CFUInt16.");
                }
                dds.add_var(sca_uint16);
                delete sca_uint16;
            }
                break;
            case H5INT32: {
                HDF5CFInt32 * sca_int32 = NULL;
                try {
                    sca_int32 = new HDF5CFInt32(var->getNewName(), var->getFullPath(), filename);
                }
                catch (...) {
                    throw InternalErr(__FILE__, __LINE__, "Cannot allocate the HDF5CFInt32.");
                }
                dds.add_var(sca_int32);
                delete sca_int32;
            }
                break;
            case H5UINT32: {
                HDF5CFUInt32 * sca_uint32 = NULL;
                try {
                    sca_uint32 = new HDF5CFUInt32(var->getNewName(), var->getFullPath(), filename);
                }
                catch (...) {
                    throw InternalErr(__FILE__, __LINE__, "Cannot allocate the HDF5CFUInt32.");
                }
                dds.add_var(sca_uint32);
                delete sca_uint32;
            }
                break;
            case H5FLOAT32: {
                HDF5CFFloat32 * sca_float32 = NULL;
                try {
                    sca_float32 = new HDF5CFFloat32(var->getNewName(), var->getFullPath(), filename);
                }
                catch (...) {
                    throw InternalErr(__FILE__, __LINE__, "Cannot allocate the HDF5CFFloat32.");
                }
                dds.add_var(sca_float32);
                delete sca_float32;
            }
                break;
            case H5FLOAT64: {
                HDF5CFFloat64 * sca_float64 = NULL;
                try {
                    sca_float64 = new HDF5CFFloat64(var->getNewName(), var->getFullPath(), filename);
                }
                catch (...) {
                    throw InternalErr(__FILE__, __LINE__, "Cannot allocate the HDF5CFFloat64.");
                }
                dds.add_var(sca_float64);
                delete sca_float64;

            }
                break;
            default:
                throw InternalErr(__FILE__, __LINE__, "unsupported data type.");
            }
        }
    }

    else {

        // 64-bit integer support
        // DMR CHECK
        bool dap4_int64 = false;
        if(var->getType() == H5INT64 || var->getType()==H5UINT64) { 
            DMR * dmr = HDF5RequestHandler::get_dmr_64bit_int();
            if(dmr == NULL)
                return;
            else 
                dap4_int64 = true;
        }

#if 0
            else {
                D4Group* root_grp = dmr->root();
                BaseType *bt = NULL;
                bt = new(HDF5Int64)(var->getNewName(),var->getFullPath(),filename);
                bt->transform_to_dap4(root_grp,root_grp);
                delete bt;
                return;
            }
#endif
        BaseType *bt = NULL;

        if(true == dap4_int64) {
            if(var->getType() == H5INT64)
                bt = new(HDF5CFInt64)(var->getNewName(),var->getFullPath());
            else if(var->getType() == H5UINT64)
                bt = new(HDF5CFUInt64)(var->getNewName(),var->getFullPath());
        }

        else {
        switch (var->getType()) {
#define HANDLE_CASE(tid,type)                                  \
            case tid:                                           \
                bt = new (type)(var->getNewName(),var->getFullPath()); \
            break;
        HANDLE_CASE(H5FLOAT32, HDF5CFFloat32)
            ;
        HANDLE_CASE(H5FLOAT64, HDF5CFFloat64)
            ;
        HANDLE_CASE(H5CHAR, HDF5CFInt16)
            ;
        HANDLE_CASE(H5UCHAR, HDF5CFByte)
            ;
        HANDLE_CASE(H5INT16, HDF5CFInt16)
            ;
        HANDLE_CASE(H5UINT16, HDF5CFUInt16)
            ;
        HANDLE_CASE(H5INT32, HDF5CFInt32)
            ;
        HANDLE_CASE(H5UINT32, HDF5CFUInt32)
            ;
        HANDLE_CASE(H5FSTRING, Str)
            ;
        HANDLE_CASE(H5VSTRING, Str)
            ;
        default:
            throw InternalErr(__FILE__, __LINE__, "unsupported data type.");
#undef HANDLE_CASE
        }
        }

        vector<HDF5CF::Dimension*>::const_iterator it_d;
        vector<size_t> dimsizes;
        dimsizes.resize(var->getRank());
        for (int i = 0; i < var->getRank(); i++)
            dimsizes[i] = (dims[i])->getSize();

        HDF5CFArray *ar = NULL;
        try {
            ar = new HDF5CFArray(var->getRank(), file_id, filename, var->getType(), dimsizes, var->getFullPath(),
                var->getTotalElems(), CV_UNSUPPORTED, false, var->getCompRatio(), var->getNewName(), bt);
        }
        catch (...) {
            delete bt;
            throw InternalErr(__FILE__, __LINE__, "Cannot allocate the HDF5CFStr.");
        }

        for (it_d = dims.begin(); it_d != dims.end(); ++it_d) {
            if ("" == (*it_d)->getNewName())
                ar->append_dim((*it_d)->getSize());
            else
                ar->append_dim((*it_d)->getSize(), (*it_d)->getNewName());
        }

        // When handling DAP4 CF, we need to generate dmr for 64-bit integer separately.
        if(dap4_int64 == true) {
            DMR * dmr = HDF5RequestHandler::get_dmr_64bit_int();
            D4Group* root_grp = dmr->root();
            // Dimensions need to be translated.
            BaseType* d4_var = ar->h5cfdims_transform_to_dap4(root_grp);
            // Attributes.
            map_cfh5_attrs_to_dap4(var,d4_var);
            root_grp->add_var_nocopy(d4_var);
        }
        else 
            dds.add_var(ar);

        delete bt;
        delete ar;
    }

    return;

}

// Currently only when the datatype of fillvalue is not the same as the datatype of the variable, 
// special attribute handling is needed.
bool need_special_attribute_handling(const HDF5CF::Attribute* attr, const HDF5CF::Var* var)
{
    return ((("_FillValue" == attr->getNewName()) && (var->getType() != attr->getType())) ? true : false);
}

// Currently we only handle the case when the datatype of _FillValue is not the same as the variable datatype.
void gen_dap_special_oneobj_das(AttrTable*at, const HDF5CF::Attribute* attr, const HDF5CF::Var* var)
{

    BESDEBUG("h5", "Coming to gen_dap_special_oneobj_das()  "<<endl);
    if (attr->getCount() != 1) throw InternalErr(__FILE__, __LINE__, "FillValue attribute can only have one element.");

    H5DataType var_dtype = var->getType();
    if ((true == HDF5RequestHandler::get_fillvalue_check()) 
        && (false == is_fvalue_valid(var_dtype, attr))) {
        string msg = "The attribute value is out of the range.\n";
        msg += "The variable name: " + var->getNewName() + "\n";
        msg += "The attribute name: " + attr->getNewName() + "\n";
        msg += "The error occurs inside the gen_dap_special_oneobj_das function in h5commoncfdap.cc.";
        throw InternalErr(msg);
    }
    string print_rep = HDF5CFDAPUtil::print_attr(attr->getType(), 0, (void*) (&(attr->getValue()[0])));
    at->append_attr(attr->getNewName(), HDF5CFDAPUtil::print_type(var_dtype), print_rep);
}

// Check if this fillvalue is in the valid datatype range when the fillvalue datatype is changed to follow the CF
bool is_fvalue_valid(H5DataType var_dtype, const HDF5CF::Attribute* attr)
{

    BESDEBUG("h5", "Coming to is_fvalue_valid()  "<<endl);
    bool ret_value = true;
    // We only check 8-bit and 16-bit integers. 
    switch (attr->getType()) {
    case H5CHAR: {
        signed char final_fill_value = *((signed char*) ((void*) (&(attr->getValue()[0]))));
        if ((var_dtype == H5UCHAR) && (final_fill_value<0)) 
            ret_value = false;
        return ret_value;

    }
    case H5INT16: {
        short final_fill_value = *((short*) ((void*) (&(attr->getValue()[0]))));
        if ((var_dtype == H5UCHAR) &&(final_fill_value > 255 || final_fill_value < 0)) 
            ret_value = false;
        
        // No need to check the var_dtype==H5CHAR case since it is mapped to int16.
        else if ((var_dtype == H5UINT16) && (final_fill_value < 0)) 
            ret_value = false;
        return ret_value;
    }
    case H5UINT16: {
        unsigned short final_fill_value = *((unsigned short*) ((void*) (&(attr->getValue()[0]))));
        if ((var_dtype == H5UCHAR) &&(final_fill_value > 255)) {
            ret_value = false;
        }
        else if ((var_dtype == H5INT16) && (final_fill_value >32767)){
            ret_value = false;
        }
        return ret_value;

    }
        // We are supposed to check the case when the datatype of fillvalue is unsigned char.
        // However, since the variable type signed char is always mapped to int16, so there 
        // will never be an overflow case(the signed char case is the only possible one). 
        // Still the data producer should not do this. We will not check this in the handler.KY 2016-03-04
#if 0
        case H5UCHAR:
        {
            unsigned char final_fill_value = *((unsigned char*)((void*)(&(attr->getValue()[0]))));
            if(var_dtype == H5CHAR) {
                if(final_fill_value >127)
                ret_value = false;
            }
            return ret_value;
        }

    case H5UCHAR:
    case H5INT32:
    case H5UINT32:
#endif

    default:
        return ret_value;
    }

}
// Leave the old code for the time being. KY 2015-05-07
#if 0
void gen_dap_special_oneobj_das(AttrTable*at, const HDF5CF::Attribute* attr,const HDF5CF::Var* var) {

    if (attr->getCount() != 1)
    throw InternalErr(__FILE__,__LINE__,"FillValue attribute can only have one element.");

    H5DataType var_dtype = var->getType();
    switch(var_dtype) {

        case H5UCHAR:
        {
            unsigned char final_fill_value = *((unsigned char*)((void*)(&(attr->getValue()[0]))));
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
        break;

        case H5CHAR:
        {
            // Notice HDF5 native char maps to DAP int16. 
            short final_fill_value = *((short*)((void*)(&(attr->getValue()[0]))));
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
        break;
        case H5INT16:
        {
            short final_fill_value = *((short*)((void*)(&(attr->getValue()[0]))));
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
        break;
        case H5UINT16:
        {
            unsigned short final_fill_value = *((unsigned short*)((void*)(&(attr->getValue()[0]))));
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
        break;

        case H5INT32:
        {
            int final_fill_value = *((int*)((void*)(&(attr->getValue()[0]))));
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
        break;
        case H5UINT32:
        {
            unsigned int final_fill_value = *((unsigned int*)((void*)(&(attr->getValue()[0]))));
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
        break;
        case H5FLOAT32:
        {
            float final_fill_value = *((float*)((void*)(&(attr->getValue()[0]))));
//            memcpy(&(attr->getValue()[0]),(void*)(&final_fill_value),sizeof(float));
//cerr<<"final_fill_value is "<<final_fill_value <<endl;
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
        break;
        case H5FLOAT64:
        {
            double final_fill_value = *((double*)((void*)(&(attr->getValue()[0]))));
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
        break;
        default:
        throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
    }

    at->append_attr(attr->getNewName(), HDF5CFDAPUtil::print_type(var_dtype), print_rep);
}
#endif

// Generate DAS from one variable
void gen_dap_oneobj_das(AttrTable*at, const HDF5CF::Attribute* attr, const HDF5CF::Var *var)
{

    BESDEBUG("h5", "Coming to gen_dap_oneobj_das()  "<<endl);
    // DMR support for 64-bit integer
    if (H5INT64 == attr->getType() || H5UINT64 == attr->getType()) {
        // TODO: Add code to tackle DMR for the variable datatype that is not 64-bit integer. 
        return;

    }
    else if ((H5FSTRING == attr->getType()) || (H5VSTRING == attr->getType())) {
        gen_dap_str_attr(at, attr);
    }
    else {

        if (NULL == var) {

            // HDF5 Native Char maps to DAP INT16(DAP doesn't have the corresponding datatype), so needs to
            // obtain the mem datatype. 
            size_t mem_dtype_size = (attr->getBufSize()) / (attr->getCount());
            H5DataType mem_dtype = HDF5CFDAPUtil::get_mem_dtype(attr->getType(), mem_dtype_size);

            for (unsigned int loc = 0; loc < attr->getCount(); loc++) {
                string print_rep = HDF5CFDAPUtil::print_attr(mem_dtype, loc, (void*) &(attr->getValue()[0]));
                at->append_attr(attr->getNewName(), HDF5CFDAPUtil::print_type(attr->getType()), print_rep);
            }

        }

        else {

            // The datatype of _FillValue attribute needs to be the same as the variable datatype for an netCDF C file.
            // To make OPeNDAP's netCDF file out work, we need to change the attribute datatype of _FillValue to be the
            // same as the variable datatype if they are not the same. An OMI-Aura_L2-OMUVB file has such a case.
            // The datatype of "TerrainHeight" is int32 but the datatype of the fillvalue is int16. 
            // KY 2012-11-20
            bool special_attr_handling = need_special_attribute_handling(attr, var);
            if (true == special_attr_handling) {
                gen_dap_special_oneobj_das(at, attr, var);
            }

            else {

                // HDF5 Native Char maps to DAP INT16(DAP doesn't have the corresponding datatype), so needs to
                // obtain the mem datatype. 
                size_t mem_dtype_size = (attr->getBufSize()) / (attr->getCount());
                H5DataType mem_dtype = HDF5CFDAPUtil::get_mem_dtype(attr->getType(), mem_dtype_size);

                for (unsigned int loc = 0; loc < attr->getCount(); loc++) {
                    string print_rep = HDF5CFDAPUtil::print_attr(mem_dtype, loc, (void*) &(attr->getValue()[0]));
                    at->append_attr(attr->getNewName(), HDF5CFDAPUtil::print_type(attr->getType()), print_rep);
                }
            }
        }
    }
}

void gen_dap_str_attr(AttrTable *at, const HDF5CF::Attribute *attr)
{

    BESDEBUG("h5", "Coming to gen_dap_str_attr()  "<<endl);
    const vector<size_t>& strsize = attr->getStrSize();
    unsigned int temp_start_pos = 0;
    bool is_cset_ascii = attr->getCsetType();
    for (unsigned int loc = 0; loc < attr->getCount(); loc++) {
        if (strsize[loc] != 0) {
            string tempstring(attr->getValue().begin() + temp_start_pos,
                attr->getValue().begin() + temp_start_pos + strsize[loc]);
            temp_start_pos += strsize[loc];

            // If the string size is longer than the current netCDF JAVA
            // string and the "EnableDropLongString" key is turned on,
            // No string is generated.
            // The above statement is no longer true. The netCDF Java can handle long string
            // attributes. The long string can be kept and I do think the
            // performance penalty should be small. KY 2018-02-26
            if ((attr->getNewName() != "origname") && (attr->getNewName() != "fullnamepath") && (true == is_cset_ascii)) 
                tempstring = HDF5CFDAPUtil::escattr(tempstring);
            at->append_attr(attr->getNewName(), "String", tempstring);
        }
    }
}

//#if 0
// This function adds the 1-D horizontal coordinate variables as well as the dummy projection variable to the grid.
//Note: Since we don't add these artifical CF variables to our main engineering at HDFEOS5CF.cc, the information
// to handle DAS won't pass to DDS by the file pointer, we need to re-call the routines to check projection
// and dimension. The time to retrieve these information is trivial compared with the whole translation.
void add_cf_grid_cvs(DDS & dds, EOS5GridPCType cv_proj_code, float cv_point_lower, float cv_point_upper,
    float cv_point_left, float cv_point_right, const vector<HDF5CF::Dimension*>& dims)
{

    //1. Check the projection information: we first just handled the sinusoidal projection. 
    // We also add the LAMAZ and PS support. These 1-D varaibles are the same as the sinusoidal one.
    if (HE5_GCTP_SNSOID == cv_proj_code || HE5_GCTP_LAMAZ == cv_proj_code || HE5_GCTP_PS == cv_proj_code) {

        //2. Obtain the dimension information from latitude and longitude(fieldtype =1 or fieldtype =2)
        vector<HDF5CF::Dimension*>::const_iterator it_d;
        string dim0name = dims[0]->getNewName();
        int dim0size = dims[0]->getSize();
        string dim1name = dims[1]->getNewName();
        int dim1size = dims[1]->getSize();

        //3. Add the 1-D CV variables and the dummy projection variable
        BaseType *bt_dim0 = NULL;
        BaseType *bt_dim1 = NULL;

        HDF5CFGeoCF1D * ar_dim0 = NULL;
        HDF5CFGeoCF1D * ar_dim1 = NULL;

        try {

            bt_dim0 = new (HDF5CFFloat64)(dim0name, dim0name);
            bt_dim1 = new (HDF5CFFloat64)(dim1name, dim1name);

            // Note ar_dim0 is y, ar_dim1 is x.
            ar_dim0 = new HDF5CFGeoCF1D(HE5_GCTP_SNSOID, cv_point_upper, cv_point_lower, dim0size, dim0name, bt_dim0);
            ar_dim0->append_dim(dim0size, dim0name);

            ar_dim1 = new HDF5CFGeoCF1D(HE5_GCTP_SNSOID, cv_point_left, cv_point_right, dim1size, dim1name, bt_dim1);
            ar_dim1->append_dim(dim1size, dim1name);
            dds.add_var(ar_dim0);
            dds.add_var(ar_dim1);

        }
        catch (...) {
            if (bt_dim0) delete bt_dim0;
            if (bt_dim1) delete bt_dim1;
            if (ar_dim0) delete ar_dim0;
            if (ar_dim1) delete ar_dim1;
            throw InternalErr(__FILE__, __LINE__, "Unable to allocate the HDFEOS2GeoCF1D instance.");
        }

        if (bt_dim0) delete bt_dim0;
        if (bt_dim1) delete bt_dim1;
        if (ar_dim0) delete ar_dim0;
        if (ar_dim1) delete ar_dim1;

    }
}

// This function adds the grid mapping variables.
void add_cf_grid_mapinfo_var(DDS & dds, const EOS5GridPCType grid_proj_code, const unsigned short g_suffix)
{

    //Add the dummy projection variable. The attributes of this variable can be used to store the grid mapping info.
    // To handle multi-grid cases, we need to add suffixes to distinguish them.
    string cf_projection_base = "eos_cf_projection";

    HDF5CFGeoCFProj * dummy_proj_cf = NULL;
    if(HE5_GCTP_SNSOID == grid_proj_code)  {
        // AFAWK, one grid_mapping variable is necessary for multi-grids. So we just leave one grid here.
        if(g_suffix == 1) {
            dummy_proj_cf = new HDF5CFGeoCFProj(cf_projection_base, cf_projection_base);
            dds.add_var(dummy_proj_cf);
        }
    }
    else {
        stringstream t_suffix_ss;
        t_suffix_ss << g_suffix;
        string cf_projection_name = cf_projection_base + "_" + t_suffix_ss.str();
        dummy_proj_cf = new HDF5CFGeoCFProj(cf_projection_name, cf_projection_name);
        dds.add_var(dummy_proj_cf);
    }
    if (dummy_proj_cf) delete dummy_proj_cf;

}

// This function adds 1D grid mapping CF attributes to CV and data variables.
#if 0
void add_cf_grid_cv_attrs(DAS & das, const vector<HDF5CF::Var*>& vars, EOS5GridPCType cv_proj_code,
    float /*cv_point_lower*/, float /*cv_point_upper*/, float /*cv_point_left*/, float /*cv_point_right*/,
    const vector<HDF5CF::Dimension*>& dims,const vector<double> &eos5_proj_params,const unsigned short g_suffix)
#endif
void add_cf_grid_cv_attrs(DAS & das, const vector<HDF5CF::Var*>& vars, EOS5GridPCType cv_proj_code,
    const vector<HDF5CF::Dimension*>& dims,const vector<double> &eos5_proj_params,const unsigned short g_suffix)
{


    //1. Check the projection information, now, we handle sinusoidal,PS and LAMAZ projections.
    if (HE5_GCTP_SNSOID == cv_proj_code || HE5_GCTP_PS == cv_proj_code || HE5_GCTP_LAMAZ== cv_proj_code) {

        string dim0name = (dims[0])->getNewName();
        int dim0size = dims[0]->getSize();
        string dim1name = (dims[1])->getNewName();
        int dim1size = dims[1]->getSize();

        //2. Add 1D CF attributes to the 1-D CV variables and the dummy grid_mapping variable
        AttrTable *at = das.get_table(dim0name);
        if (!at) 
            at = das.add_table(dim0name, new AttrTable);
        at->append_attr("standard_name", "String", "projection_y_coordinate");

        string long_name = "y coordinate of projection ";
        at->append_attr("long_name", "String", long_name);

        // Change this to meter.
        at->append_attr("units", "string", "meter");

        at->append_attr("_CoordinateAxisType", "string", "GeoY");

        at = das.get_table(dim1name);
        if (!at) at = das.add_table(dim1name, new AttrTable);

        at->append_attr("standard_name", "String", "projection_x_coordinate");

        long_name = "x coordinate of projection ";
        at->append_attr("long_name", "String", long_name);

        // change this to meter.
        at->append_attr("units", "string", "meter");
        
        // This is for CDM conventions. Adding doesn't do harm. Same as GeoY.
        at->append_attr("_CoordinateAxisType", "string", "GeoX");

        // Add the attributes for the dummy grid_mapping variable.
        string cf_projection_base = "eos_cf_projection";
        string cf_projection;
        if(HE5_GCTP_SNSOID == cv_proj_code)
            cf_projection = cf_projection_base;
        else {
            stringstream t_suffix_ss;
            t_suffix_ss << g_suffix;
            cf_projection = cf_projection_base + "_" + t_suffix_ss.str();
        }
	add_cf_projection_attrs(das,cv_proj_code,eos5_proj_params,cf_projection);

        // Fill in the data fields that contains the dim0name and dim1name dimensions with the grid_mapping
        // We only apply to >=2D data fields.
        add_cf_grid_mapping_attr(das, vars, cf_projection, dim0name, dim0size, dim1name, dim1size);
    }

}

// Add CF projection attribute

void add_cf_projection_attrs(DAS &das,EOS5GridPCType cv_proj_code,const vector<double> &eos5_proj_params,const string& cf_projection) {

    AttrTable* at = das.get_table(cf_projection);
    if (!at) {// Only append attributes when the table is created.
        at = das.add_table(cf_projection, new AttrTable);

        if (HE5_GCTP_SNSOID == cv_proj_code) {
            at->append_attr("grid_mapping_name", "String", "sinusoidal");
            at->append_attr("longitude_of_central_meridian", "Float64", "0.0");
            at->append_attr("earth_radius", "Float64", "6371007.181");
            at->append_attr("_CoordinateAxisTypes", "string", "GeoX GeoY");
        }
        else if (HE5_GCTP_PS == cv_proj_code) {

            // The following information is added according to the HDF-EOS5 user's guide and
            // CF 1.7 grid_mapping requirement.

            // Longitude down below pole of map
            double vert_lon_pole =  HE5_EHconvAng(eos5_proj_params[4],HE5_HDFE_DMS_DEG);

            // Latitude of true scale
            double lat_true_scale = HE5_EHconvAng(eos5_proj_params[5],HE5_HDFE_DMS_DEG);

            // False easting
            double fe = eos5_proj_params[6];

            // False northing 
            double fn = eos5_proj_params[7];

            at->append_attr("grid_mapping_name", "String", "polar_stereographic");

            ostringstream s_vert_lon_pole;
            s_vert_lon_pole << vert_lon_pole;

            // I did this map is based on my best understanding. I cannot be certain about south pole. KY
            // CF: straight_vertical_longitude_from_pole
            at->append_attr("straight_vertical_longitude_from_pole", "Float64", s_vert_lon_pole.str());
            ostringstream s_lat_true_scale;
            s_lat_true_scale << lat_true_scale;

            at->append_attr("standard_parallel", "Float64", s_lat_true_scale.str());

            if(fe == 0.0) 
                at->append_attr("false_easting","Float64","0.0");
            else { 
                ostringstream s_fe;
                s_fe << fe;
                at->append_attr("false_easting","Float64",s_fe.str());
            }


            if(fn == 0.0) 
                at->append_attr("false_northing","Float64","0.0");
            else { 
                ostringstream s_fn;
                s_fn << fn;
                at->append_attr("false_northing","Float64",s_fn.str());
            }

            
            if(lat_true_scale >0) 
                at->append_attr("latitude_of_projection_origin","Float64","+90.0");
            else 
                at->append_attr("latitude_of_projection_origin","Float64","-90.0");


            at->append_attr("_CoordinateAxisTypes", "string", "GeoX GeoY");

            // From CF, PS has another parameter,
            // Either standard_parallel (EPSG 9829) or scale_factor_at_projection_origin (EPSG 9810)
            // I cannot find the corresponding parameter from the EOS5.

        }
        else if(HE5_GCTP_LAMAZ == cv_proj_code) {
            double lon_proj_origin = HE5_EHconvAng(eos5_proj_params[4],HE5_HDFE_DMS_DEG);
            double lat_proj_origin = HE5_EHconvAng(eos5_proj_params[5],HE5_HDFE_DMS_DEG);
            double fe = eos5_proj_params[6];
            double fn = eos5_proj_params[7];

            at->append_attr("grid_mapping_name", "String", "lambert_azimuthal_equal_area");

            ostringstream s_lon_proj_origin;
            s_lon_proj_origin << lon_proj_origin;
            at->append_attr("longitude_of_projection_origin", "Float64", s_lon_proj_origin.str());
            
            ostringstream s_lat_proj_origin;
            s_lat_proj_origin << lat_proj_origin;
 
            at->append_attr("latitude_of_projection_origin", "Float64", s_lat_proj_origin.str());


            if(fe == 0.0) 
                at->append_attr("false_easting","Float64","0.0");
            else { 
                ostringstream s_fe;
                s_fe << fe;
                at->append_attr("false_easting","Float64",s_fe.str());
            }


            if(fn == 0.0) 
                at->append_attr("false_northing","Float64","0.0");
            else { 
                ostringstream s_fn;
                s_fn << fn;
                at->append_attr("false_northing","Float64",s_fn.str());
            }

            at->append_attr("_CoordinateAxisTypes", "string", "GeoX GeoY");


        }
    }

}


// This function adds the 1-D cf grid projection mapping attribute to data variables
// it is called by the function add_cf_grid_attrs. 
void add_cf_grid_mapping_attr(DAS &das, const vector<HDF5CF::Var*>& vars, const string& cf_projection,
    const string & dim0name, hsize_t dim0size, const string &dim1name, hsize_t dim1size)
{

#if 0
    cerr<<"dim0name is "<<dim0name <<endl;
    cerr<<"dim1name is "<<dim1name <<endl;
    cerr<<"dim0size is "<<dim0size <<endl;
    cerr<<"dim1size is "<<dim1size <<endl;
#endif

    // Check >=2-D fields, check if they hold the dim0name,dim0size etc., yes, add the attribute cf_projection.
    vector<HDF5CF::Var *>::const_iterator it_v;
    for (it_v = vars.begin(); it_v != vars.end(); ++it_v) {

        if ((*it_v)->getRank() > 1) {
            bool has_dim0 = false;
            bool has_dim1 = false;
            const vector<HDF5CF::Dimension*>& dims = (*it_v)->getDimensions();
            for (vector<HDF5CF::Dimension *>::const_iterator j = dims.begin(); j != dims.end(); ++j) {
                if ((*j)->getNewName() == dim0name && (*j)->getSize() == dim0size)
                    has_dim0 = true;
                else if ((*j)->getNewName() == dim1name && (*j)->getSize() == dim1size) 
                    has_dim1 = true;

            }
            if (true == has_dim0 && true == has_dim1) {        // Need to add the grid_mapping attribute
                AttrTable *at = das.get_table((*it_v)->getNewName());
                if (!at) at = das.add_table((*it_v)->getNewName(), new AttrTable);

                // The dummy projection name is the value of the grid_mapping attribute
                at->append_attr("grid_mapping", "String", cf_projection);
            }
        }
    }
}
// Now this is specially for LAMAZ where the NSIDC EASE-Grid may have points off the earth. So
// The calculated lat/lon are set to number out of the normal range. The valid_range attributes
// will hopefully constrain the applications not to consider those points.
void add_ll_valid_range(AttrTable* at, bool is_lat) {
    if(true == is_lat) {
        at->append_attr("valid_min", "Float64","-90.0");
        at->append_attr("valid_max", "Float64","90.0");
    }
    else {
        at->append_attr("valid_min", "Float64","-180.0");
        at->append_attr("valid_max", "Float64","180.0");
    }
}

// This routine is for 64-bit DAP4 CF support: when var type is 64-bit integer.
bool need_attr_values_for_dap4(const HDF5CF::Var *var) {
    bool ret_value = false;
    if((HDF5RequestHandler::get_dmr_64bit_int()!=NULL) && 
        ((H5INT64 == var->getType() || H5UINT64 == var->getType())))
        ret_value = true;
    return ret_value;
}

// This routine is for 64-bit DAP4 CF support: map all attributes to DAP4 for 64-bit integers.
void map_cfh5_attrs_to_dap4(const HDF5CF::Var *var,BaseType* d4_var) {

    vector<HDF5CF::Attribute *>::const_iterator it_ra;
    for (it_ra = var->getAttributes().begin();
        it_ra != var->getAttributes().end(); ++it_ra) {
        // HDF5 Native Char maps to DAP INT16(DAP doesn't have the corresponding datatype), so needs to
        // obtain the mem datatype. Keep this in DAP4 mapping.
        size_t mem_dtype_size = ((*it_ra)->getBufSize()) / ((*it_ra)->getCount());
        H5DataType mem_dtype = HDF5CFDAPUtil::get_mem_dtype((*it_ra)->getType(), mem_dtype_size);

        string dap2_attrtype = HDF5CFDAPUtil::print_type(mem_dtype);
        D4AttributeType dap4_attrtype = HDF5CFDAPUtil::daptype_strrep_to_dap4_attrtype(dap2_attrtype);
        D4Attribute *d4_attr = new D4Attribute((*it_ra)->getNewName(),dap4_attrtype);
        if(dap4_attrtype == attr_str_c) {
            const vector<size_t>& strsize = (*it_ra)->getStrSize();
            unsigned int temp_start_pos = 0;
            for (unsigned int loc = 0; loc < (*it_ra)->getCount(); loc++) {
                if (strsize[loc] != 0) {
                    string tempstring((*it_ra)->getValue().begin() + temp_start_pos,
                                      (*it_ra)->getValue().begin() + temp_start_pos + strsize[loc]);
                    temp_start_pos += strsize[loc];
                    if (((*it_ra)->getNewName() != "origname") && ((*it_ra)->getNewName() != "fullnamepath")) 
                        tempstring = HDF5CFDAPUtil::escattr(tempstring);
                    d4_attr->add_value(tempstring);
                }
            }
        }
        else {
            for (unsigned int loc = 0; loc < (*it_ra)->getCount(); loc++) {
                string print_rep = HDF5CFDAPUtil::print_attr(mem_dtype, loc, (void*) &((*it_ra)->getValue()[0]));
                d4_attr->add_value(print_rep);
            }
        }
        d4_var->attributes()->add_attribute_nocopy(d4_attr);
    }

}

void check_update_int64_attr(const string & obj_name, const HDF5CF::Attribute * attr) {
    if(attr->getType() == H5INT64 || attr->getType() == H5UINT64) { 
        DMR * dmr = HDF5RequestHandler::get_dmr_64bit_int();
        if(dmr != NULL) {
            string dap2_attrtype = HDF5CFDAPUtil::print_type(attr->getType());
            D4AttributeType dap4_attrtype = HDF5CFDAPUtil::daptype_strrep_to_dap4_attrtype(dap2_attrtype);
            D4Attribute *d4_attr = new D4Attribute(attr->getNewName(),dap4_attrtype);
            for (unsigned int loc = 0; loc < attr->getCount(); loc++) {
                string print_rep = HDF5CFDAPUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
                d4_attr->add_value(print_rep);
            }
            D4Group * root_grp = dmr->root();
            D4Attribute *d4_hg_container; 
            if(root_grp->attributes()->empty() == true){
#if 0
            //D4Attribute *d4_hg_container = root_grp->attributes()->find("HDF5_GLOBAL");
            //if(d4_hg_container == NULL) {
#endif
                d4_hg_container = new D4Attribute;
                d4_hg_container->set_name("HDF5_GLOBAL_integer_64");
                d4_hg_container->set_type(attr_container_c);
                root_grp->attributes()->add_attribute_nocopy(d4_hg_container);
#if 0
                //root_grp->attributes()->add_attribute(d4_hg_container);
#endif
            }
            //else 
            d4_hg_container = root_grp->attributes()->get("HDF5_GLOBAL_integer_64");
            if(obj_name != "") {
                string test_obj_name = "HDF5_GLOBAL_integer_64."+obj_name;
#if 0
                //D4Attribute *d4_container = root_grp->attributes()->find(obj_name);
                //D4Attribute *d4_container = root_grp->attributes()->get(obj_name);
#endif
                D4Attribute *d4_container = root_grp->attributes()->get(test_obj_name);
                // ISSUES need to search the attributes 
                //
#if 0
                //D4Attribute *d4_container = d4_hg_container->attributes()->find(obj_name);
#endif
                if(d4_container == NULL) {
                    d4_container = new D4Attribute;
                    d4_container->set_name(obj_name);
                    d4_container->set_type(attr_container_c);

#if 0
                    //if(d4_hg_container->attributes()->empty()==true)
                    //    cerr<<"global container is empty"<<endl;
                    //d4_hg_container->attributes()->add_attribute_nocopy(d4_container);
                    //cerr<<"end of d4_container "<<endl;
#endif
                }
                d4_container->attributes()->add_attribute_nocopy(d4_attr);
#if 0
                //root_grp->attributes()->add_attribute_nocopy(d4_container);
#endif
//#if 0
                if(d4_hg_container->attributes()->get(obj_name)==NULL)
                    d4_hg_container->attributes()->add_attribute_nocopy(d4_container);
//#endif
            }
            else 
                d4_hg_container->attributes()->add_attribute_nocopy(d4_attr);
        }
    }
}
