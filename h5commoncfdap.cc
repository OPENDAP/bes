// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2011-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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


void gen_dap_onevar_dds(DDS &dds,const HDF5CF::Var* var, const string & filename) {

    BaseType *bt = NULL;

    switch(var->getType()) {
#define HANDLE_CASE(tid,type)                                  \
        case tid:                                           \
            bt = new (type)(var->getNewName(),var->getFullPath());  \
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

    if (bt != NULL) {

        const vector<HDF5CF::Dimension *>& dims = var->getDimensions();

        vector <HDF5CF::Dimension*>:: const_iterator it_d;
        if (0 == dims.size()) { 
            if (H5FSTRING == var->getType() || H5VSTRING == var->getType()) {
                HDF5CFStr *sca_str = NULL;
                sca_str = new HDF5CFStr(var->getNewName(),filename,var->getFullPath());
                dds.add_var(sca_str);
                delete bt;
                delete sca_str;
            }
            else {
                delete bt;
                throw InternalErr(__FILE__,__LINE__,"Non string scalar data is not supported");
            }
        }
        else {
            HDF5CFArray *ar = NULL;
            ar = new HDF5CFArray (
                                    var->getRank(),
                                    filename,
                                    var->getType(),
                                    var->getFullPath(),
                                    var->getNewName(),
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
    }
}

// Currently only when the datatype of fillvalue is not the same as the datatype of the variable, 
// special attribute handling is needed.
bool need_special_attribute_handling(const HDF5CF::Attribute* attr,const HDF5CF::Var* var) { 
    return ((("_FillValue" == attr->getNewName()) && (var->getType() != attr->getType()))?true:false);
}

     
// Currently we only handle the case when the datatype of _FillValue is not the same as the variable datatype.
void gen_dap_special_oneobj_das(AttrTable*at, const HDF5CF::Attribute* attr,const HDF5CF::Var* var) {

    if (attr->getCount() != 1) 
        throw InternalErr(__FILE__,__LINE__,"FillValue attribute can only have one element.");

    H5DataType var_dtype = var->getType();
    string print_rep;

    switch(var_dtype) {

        case H5UCHAR:
        {
            unsigned char final_fill_value = (unsigned char)(attr->getValue()[0]);
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
            break;

        case H5CHAR:
        {
            // Notice HDF5 native char maps to DAP int16. 
            short final_fill_value = (short)(attr->getValue()[0]);
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
            break;
        case H5INT16:
        {
            short final_fill_value = (short)(attr->getValue()[0]);
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
            break;
        case H5UINT16:
        {
            unsigned short final_fill_value = (unsigned short)(attr->getValue()[0]);
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
            break;

        case H5INT32:
        {
            int final_fill_value = (int)(attr->getValue()[0]);
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
            break;
        case H5UINT32:
        {
            unsigned int final_fill_value = (unsigned int)(attr->getValue()[0]);
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
            break;
        case H5FLOAT32:
        {
            float final_fill_value = (float)(attr->getValue()[0]);
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
            break;
        case H5FLOAT64:
        {
            double final_fill_value = (double)(attr->getValue()[0]);
            print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);
        }
            break;
        default:
            throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
    }

    at->append_attr(attr->getNewName(), HDF5CFDAPUtil::print_type(var_dtype), print_rep);
}

#if 0
    switch(var_dtype) {

        case H5UCHAR:
        {
            unsigned char final_fill_value = (unsigned char)(attr->getValue()[0]);
            string print_rep = HDF5CFDAPUtil::print_attr(var_dtype,0,(void*)&final_fill_value);

        }
            break;

#endif

    




void gen_dap_oneobj_das(AttrTable*at,const HDF5CF::Attribute* attr, const HDF5CF::Var *var) {

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

    string check_droplongstr_key ="H5.EnableDropLongString";
    bool is_droplongstr = false;
    is_droplongstr = HDF5CFDAPUtil::check_beskeys(check_droplongstr_key);

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
            if (false == is_droplongstr || 
                tempstring.size() <= NC_JAVA_STR_SIZE_LIMIT) 
                at->append_attr(attr->getNewName(),"String",tempstring);
        }
    }
}

