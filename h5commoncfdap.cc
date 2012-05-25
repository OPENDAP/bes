// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2011 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
        // cerr<<"number of dimensions "<<dims.size() <<endl;

        vector <HDF5CF::Dimension*>:: const_iterator it_d;
        if (0 == dims.size()) { 
            if (H5FSTRING == var->getType() || H5VSTRING == var->getType()) {
                HDF5CFStr *sca_str = NULL;
                sca_str = new HDF5CFStr(var->getNewName(),filename,var->getFullPath());
                dds.add_var(sca_str);
                delete sca_str;
            }
            else 
                throw InternalErr(__FILE__,__LINE__,"Non string scalar data is not supported");
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
               // cerr<<"dim name "<<(*it_d)->getNewName() <<endl;
               if (""==(*it_d)->getNewName()) 
                    ar->append_dim((*it_d)->getSize());
               else 
                    ar->append_dim((*it_d)->getSize(), (*it_d)->getNewName());
            }

            dds.add_var(ar);
            delete ar;
        }
    }
}

void gen_dap_oneobj_das(AttrTable*at,const HDF5CF::Attribute* attr) {

    if ((H5FSTRING == attr->getType()) ||
        (H5VSTRING == attr->getType())) {
        gen_dap_str_attr(at,attr);
    }
    else {
        size_t mem_dtype_size = (attr->getBufSize())/(attr->getCount());
        H5DataType mem_dtype = HDF5CFDAPUtil::get_mem_dtype(attr->getType(),mem_dtype_size);
        for (unsigned int loc=0; loc < attr->getCount() ; loc++) {
            string print_rep = HDF5CFDAPUtil::print_attr(mem_dtype, loc, (void*) &(attr->getValue()[0]));
            at->append_attr(attr->getNewName(), HDF5CFDAPUtil::print_type(attr->getType()), print_rep);
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

