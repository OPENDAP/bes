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
#include "HDF5CFStr.h"
#include "HDF5CFArray.h"

using namespace HDF5CF;

// Generate DDS from one variable
void gen_dap_onevar_dds(DDS &dds,const HDF5CF::Var* var, const hid_t file_id, const string & filename) {

    BESDEBUG("h5","Coming to gen_dap_onevar_dds()  "<<endl);

    const vector<HDF5CF::Dimension *>& dims = var->getDimensions();

    if (0 == dims.size()) { 
        if (H5FSTRING == var->getType() || H5VSTRING == var->getType()) {
            HDF5CFStr *sca_str = NULL;
            try {
                sca_str = new HDF5CFStr(var->getNewName(),filename,var->getFullPath());
            }
            catch(...) {
                throw InternalErr(__FILE__,__LINE__,"Cannot allocate the HDF5CFStr.");
            }
            dds.add_var(sca_str);
            delete sca_str;
        }
        else {
            switch(var->getType()) {

                case H5UCHAR:
                {
                    HDF5CFByte * sca_uchar = NULL;
                    try {
                        sca_uchar = new HDF5CFByte(var->getNewName(),var->getFullPath(),filename);
                    }
                    catch(...) {
                        throw InternalErr(__FILE__,__LINE__,"Cannot allocate the HDF5CFByte.");
                    }
                    dds.add_var(sca_uchar);
                    delete sca_uchar;

                }
                break;
                case H5CHAR:
                case H5INT16:
                {
                    HDF5CFInt16 * sca_int16 = NULL;
                    try {
                        sca_int16 = new HDF5CFInt16(var->getNewName(),var->getFullPath(),filename);
                    }
                    catch(...) {
                        throw InternalErr(__FILE__,__LINE__,"Cannot allocate the HDF5CFInt16.");
                    }
                    dds.add_var(sca_int16);
                    delete sca_int16;
                }
                break;
                case H5UINT16:
       	        {
                    HDF5CFUInt16 * sca_uint16 = NULL;
                    try {
                        sca_uint16 = new HDF5CFUInt16(var->getNewName(),var->getFullPath(),filename);
                    }
                    catch(...) {
                        throw InternalErr(__FILE__,__LINE__,"Cannot allocate the HDF5CFUInt16.");
                    }
                    dds.add_var(sca_uint16);
                    delete sca_uint16;
                }
                break;
                case H5INT32:
                {
                    HDF5CFInt32 * sca_int32 = NULL;
                    try {
                        sca_int32 = new HDF5CFInt32(var->getNewName(),var->getFullPath(),filename);
                    }
                    catch(...) {
                        throw InternalErr(__FILE__,__LINE__,"Cannot allocate the HDF5CFInt32.");
                    }
                    dds.add_var(sca_int32);
                    delete sca_int32;
                }
                break;
                case H5UINT32:
                {
                    HDF5CFUInt32 * sca_uint32 = NULL;
                    try {
                        sca_uint32 = new HDF5CFUInt32(var->getNewName(),var->getFullPath(),filename);
                    }
                    catch(...) {
                        throw InternalErr(__FILE__,__LINE__,"Cannot allocate the HDF5CFUInt32.");
                    }
                    dds.add_var(sca_uint32);
                    delete sca_uint32;
                }
                break;
                case H5FLOAT32:
                {
                    HDF5CFFloat32 * sca_float32 = NULL;
                    try {
                        sca_float32 = new HDF5CFFloat32(var->getNewName(),var->getFullPath(),filename);
                    }
                    catch(...) {
                        throw InternalErr(__FILE__,__LINE__,"Cannot allocate the HDF5CFFloat32.");
                    }
                    dds.add_var(sca_float32);
                    delete sca_float32;
                }
                break;
                case H5FLOAT64:
                {
                    HDF5CFFloat64 * sca_float64 = NULL;
                    try {
                        sca_float64 = new HDF5CFFloat64(var->getNewName(),var->getFullPath(),filename);
                    }
                    catch(...) {
                        throw InternalErr(__FILE__,__LINE__,"Cannot allocate the HDF5CFFloat64.");
                    }
                    dds.add_var(sca_float64);
                    delete sca_float64;
 
                }
                break;
                default:
                        throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
            }
        }
    }
 
    else {
        BaseType *bt = NULL;

        switch(var->getType()) {
#define HANDLE_CASE(tid,type)                                  \
            case tid:                                           \
                bt = new (type)(var->getNewName(),var->getFullPath()); \
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

        vector <HDF5CF::Dimension*>:: const_iterator it_d;

        HDF5CFArray *ar = NULL;
        try {
            ar = new HDF5CFArray (
                                  var->getRank(),
                                  file_id,
                                  filename,
                                  var->getType(),
                                  var->getFullPath(),
                                  var->getTotalElems(),
                                  var->getNewName(),
                                  bt);
        }
        catch(...) {
            delete bt;
            throw InternalErr(__FILE__,__LINE__,"Cannot allocate the HDF5CFStr.");
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

// Currently only when the datatype of fillvalue is not the same as the datatype of the variable, 
// special attribute handling is needed.
bool need_special_attribute_handling(const HDF5CF::Attribute* attr,const HDF5CF::Var* var) { 
    return ((("_FillValue" == attr->getNewName()) && (var->getType() != attr->getType()))?true:false);
}
     
// Currently we only handle the case when the datatype of _FillValue is not the same as the variable datatype.
void gen_dap_special_oneobj_das(AttrTable*at, const HDF5CF::Attribute* attr,const HDF5CF::Var* var) {

    BESDEBUG("h5","Coming to gen_dap_special_oneobj_das()  "<<endl);
    if (attr->getCount() != 1) 
        throw InternalErr(__FILE__,__LINE__,"FillValue attribute can only have one element.");

    H5DataType var_dtype = var->getType();
    if(true == HDF5RequestHandler::get_fillvalue_check()) {
        if(false == is_fvalue_valid(var_dtype,attr)) {
            string msg = "The attribute value is out of the range.\n";
            msg += "The variable name: "+var->getNewName()+"\n";
            msg += "The attribute name: " + attr->getNewName()+"\n" ;
            msg += "The error occurs inside the gen_dap_special_oneobj_das function in h5commoncfdap.cc.";
            throw InternalErr(msg);
            //throw InternalErr(__FILE__,__LINE__,msg);
        }
    }
    string print_rep = HDF5CFDAPUtil::print_attr(attr->getType(),0,(void*)(&(attr->getValue()[0])));
    at->append_attr(attr->getNewName(), HDF5CFDAPUtil::print_type(var_dtype), print_rep);
}

// Check if this fillvalue is in the valid datatype range when the fillvalue datatype is changed to follow the CF
bool is_fvalue_valid(H5DataType var_dtype, const HDF5CF::Attribute* attr) {

    BESDEBUG("h5","Coming to is_fvalue_valid()  "<<endl);
    bool ret_value = true;
    // We only check 8-bit and 16-bit integers. 
    switch(attr->getType()) {
        case H5CHAR:
        {
            signed char final_fill_value = *((signed char*)((void*)(&(attr->getValue()[0]))));
            if(var_dtype == H5UCHAR) {
                if(final_fill_value < 0) 
                    ret_value = false;
            }
            return ret_value;

        }
        case H5INT16:
        {
            short final_fill_value = *((short*)((void*)(&(attr->getValue()[0]))));
            if(var_dtype == H5UCHAR) {
                if(final_fill_value >255 || final_fill_value <0) 
                    ret_value = false;
            }
            // No need to check the var_dtype==H5CHAR case since it is mapped to int16.
            else if(var_dtype == H5UINT16) {
                if(final_fill_value <0)
                    ret_value = false;
            }
            return ret_value;
        }
        case H5UINT16:
        {
            unsigned short final_fill_value = *((unsigned short*)((void*)(&(attr->getValue()[0]))));
            if(var_dtype == H5UCHAR) {
                if(final_fill_value >255) 
                    ret_value = false;
            }
            else if(var_dtype == H5INT16) {
                if(final_fill_value >32767)
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
            if(var_dtype == H5CHAR){ 
                if(final_fill_value >127)  
                    ret_value = false; 
            }
            return ret_value;
        }
#endif

        case H5UCHAR:
        case H5INT32:
        case H5UINT32:
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
void gen_dap_oneobj_das(AttrTable*at,const HDF5CF::Attribute* attr, const HDF5CF::Var *var) {

    BESDEBUG("h5","Coming to gen_dap_oneobj_das()  "<<endl);
    if ((H5FSTRING == attr->getType()) ||
        (H5VSTRING == attr->getType())) {
        gen_dap_str_attr(at,attr);
    }
    else {

        if (NULL == var) {

            // HDF5 Native Char maps to DAP INT16(DAP doesn't have the corresponding datatype), so needs to
            // obtain the mem datatype. 
            size_t mem_dtype_size = (attr->getBufSize())/(attr->getCount());
            H5DataType mem_dtype = HDF5CFDAPUtil::get_mem_dtype(attr->getType(),mem_dtype_size);

            for (unsigned int loc=0; loc < attr->getCount() ; loc++) {
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
            bool special_attr_handling = need_special_attribute_handling(attr,var);
            if (true == special_attr_handling) {
                gen_dap_special_oneobj_das(at,attr,var);           
            }

            else {

                // HDF5 Native Char maps to DAP INT16(DAP doesn't have the corresponding datatype), so needs to
                // obtain the mem datatype. 
                size_t mem_dtype_size = (attr->getBufSize())/(attr->getCount());
                H5DataType mem_dtype = HDF5CFDAPUtil::get_mem_dtype(attr->getType(),mem_dtype_size);

                for (unsigned int loc=0; loc < attr->getCount() ; loc++) {
                    string print_rep = HDF5CFDAPUtil::print_attr(mem_dtype, loc, (void*) &(attr->getValue()[0]));
                    at->append_attr(attr->getNewName(), HDF5CFDAPUtil::print_type(attr->getType()), print_rep);
                }
            }
        }
    }
}

void gen_dap_str_attr(AttrTable *at, const HDF5CF::Attribute *attr) {

    BESDEBUG("h5","Coming to gen_dap_str_attr()  "<<endl);
    const vector<size_t>& strsize = attr->getStrSize();
    unsigned int temp_start_pos = 0;
    for (unsigned int loc=0; loc < attr->getCount() ; loc++) {
        if (strsize[loc] !=0) {
            string tempstring(attr->getValue().begin()+temp_start_pos,
                              attr->getValue().begin()+temp_start_pos+strsize[loc]);
            temp_start_pos += strsize[loc];

            // If the string size is longer than the current netCDF JAVA
            // string and the "EnableDropLongString" key is turned on,
            // No string is generated.
            //if (false == is_droplongstr || 
            if (false == HDF5RequestHandler::get_drop_long_string() || 
                tempstring.size() <= NC_JAVA_STR_SIZE_LIMIT) {
                if((attr->getNewName()!="origname")&&(attr->getNewName()!="fullnamepath"))
                    tempstring = HDF5CFDAPUtil::escattr(tempstring);
                at->append_attr(attr->getNewName(),"String",tempstring);
            }
        }
    }
}

