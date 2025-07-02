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
/// \file h5commoncfdap.cc
/// \brief Functions to generate DDS and DAS for one object(variable). 
///
/// 
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>
#include <iostream>

#include <unordered_map>
#include <unordered_set>

#include <libdap/InternalErr.h>
#include <BESInternalError.h>
#include <BESDebug.h>

#include "HDF5RequestHandler.h"
#include "h5cfdaputil.h"
#include "h5gmcfdap.h"
#include "HDF5CFByte.h"
#include "HDF5CFInt8.h"
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

#include "HDF5CFUtil.h"

using namespace std;
using namespace libdap;
using namespace HDF5CF;


// Generate DDS from one variable
void gen_dap_onevar_dds(DDS &dds, const HDF5CF::Var* var, hid_t file_id, const string & filename)
{
    BESDEBUG("h5", "Coming to gen_dap_onevar_dds()  "<<endl);
    const vector<HDF5CF::Dimension *>& dims = var->getDimensions();

    if (dims.empty()) {

        // Adding 64-bit integer support for DMR 
        if (H5INT64 == var->getType() || H5UINT64 == var->getType())
            gen_dap_onevar_dds_sca_64bit_int(var, filename);
        else if (H5FSTRING == var->getType() || H5VSTRING == var->getType()) {

            auto sca_str_unique =
                    make_unique<HDF5CFStr>(var->getNewName(), filename, var->getFullPath());
            auto sca_str = sca_str_unique.get();
            dds.add_var(sca_str);
        }
        else
            gen_dap_onevar_dds_sca_atomic(dds, var, filename) ;
    }
    else
        gen_dap_onevar_dds_array(dds, var, file_id, filename,dims);

}

void gen_dap_onevar_dds_sca_64bit_int(const HDF5CF::Var *var, const string &filename) {

    DMR * dmr = HDF5RequestHandler::get_dmr_64bit_int();
    if(dmr == nullptr)
        return;
    else {
        D4Group* root_grp = dmr->root();
        if (H5INT64 == var->getType()) {
            auto sca_int64_unique =
                    make_unique<HDF5CFInt64>(var->getNewName(), var->getFullPath(), filename);

            auto sca_int64 = sca_int64_unique.release();
            sca_int64->set_is_dap4(true);
            map_cfh5_var_attrs_to_dap4_int64(var,sca_int64);
            root_grp->add_var_nocopy(sca_int64);
        }
        else if(H5UINT64 == var->getType()) {
            auto sca_uint64_unique =
                    make_unique<HDF5CFUInt64>(var->getNewName(), var->getFullPath(), filename);
            auto sca_uint64 = sca_uint64_unique.release();
            sca_uint64->set_is_dap4(true);
            map_cfh5_var_attrs_to_dap4_int64(var,sca_uint64);
            root_grp->add_var_nocopy(sca_uint64);

        }
    }
}

void gen_dap_onevar_dds_sca_atomic(DDS &dds, const HDF5CF::Var *var, const string &filename) {

    switch (var->getType()) {

        case H5UCHAR: {
            auto sca_uchar_unique =
                    make_unique<HDF5CFByte>(var->getNewName(), var->getFullPath(), filename);
            auto sca_uchar = sca_uchar_unique.get();
            dds.add_var(sca_uchar);
        }
            break;
        case H5CHAR:
        case H5INT16: {
            auto sca_int16_unique =
                    make_unique<HDF5CFInt16>(var->getNewName(), var->getFullPath(), filename);
            auto sca_int16 = sca_int16_unique.get();
            dds.add_var(sca_int16);
        }
            break;
        case H5UINT16: {
            auto sca_uint16_unique =
                    make_unique<HDF5CFUInt16>(var->getNewName(), var->getFullPath(), filename);
            auto sca_uint16 = sca_uint16_unique.get();
            dds.add_var(sca_uint16);
        }
            break;
        case H5INT32: {
            auto sca_int32_unique =
                    make_unique<HDF5CFInt32>(var->getNewName(), var->getFullPath(), filename);
            auto sca_int32 = sca_int32_unique.get();
            dds.add_var(sca_int32);
        }
            break;
        case H5UINT32: {
            
            auto sca_uint32_unique =
                    make_unique<HDF5CFUInt32>(var->getNewName(), var->getFullPath(), filename);
            auto sca_uint32 = sca_uint32_unique.get();
            dds.add_var(sca_uint32);
        }
            break;
        case H5FLOAT32: {
            auto sca_float32_unique =
                    make_unique<HDF5CFFloat32>(var->getNewName(), var->getFullPath(), filename);
            auto sca_float32 = sca_float32_unique.get();
            dds.add_var(sca_float32);
        }
            break;
        case H5FLOAT64: {
            auto sca_float64_unique =
                    make_unique<HDF5CFFloat64>(var->getNewName(), var->getFullPath(), filename);
            auto sca_float64 = sca_float64_unique.get();
            dds.add_var(sca_float64);
        }
            break;
        default:{
            string msg = "unsupported data type.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }
}

void gen_dap_onevar_dds_array(DDS &dds, const HDF5CF::Var *var, hid_t file_id, const string &filename,
                              const vector<HDF5CF::Dimension *>& dims) {

    // 64-bit integer support
    // DMR CHECK
    bool dap4_int64 = false;
    if (var->getType() == H5INT64 || var->getType() == H5UINT64) {
        const DMR *dmr = HDF5RequestHandler::get_dmr_64bit_int();
        if (dmr == nullptr)
            return;
        else
            dap4_int64 = true;
    }

    BaseType *bt = nullptr;

    if (true == dap4_int64) {
        if (var->getType() == H5INT64) {
            auto int64_unique = make_unique<HDF5CFInt64>(var->getNewName(), var->getFullPath());
            bt = int64_unique.release();
        } else if (var->getType() == H5UINT64) {
            auto uint64_unique = make_unique<HDF5CFUInt64>(var->getNewName(), var->getFullPath());
            bt = uint64_unique.release();
        }
    } else {
        switch (var->getType()) {
            // cannot use unique_ptr in the macro.
#if 0
#define HANDLE_CASE(tid, type)                                  \
            case tid:                                           \
                auto type_unique = make_unique<type>(var->getNewName(),var->getFullPath()); bt = type_unique.release(); \
            break;
#endif
#define HANDLE_CASE(tid, type)                                  \
            case tid:                                           \
                bt = new (type)(var->getNewName(),var->getFullPath()); \
            break;
            HANDLE_CASE(H5FLOAT32, HDF5CFFloat32)
            HANDLE_CASE(H5FLOAT64, HDF5CFFloat64)
            HANDLE_CASE(H5CHAR, HDF5CFInt16)
            HANDLE_CASE(H5UCHAR, HDF5CFByte)
            HANDLE_CASE(H5INT16, HDF5CFInt16)
            HANDLE_CASE(H5UINT16, HDF5CFUInt16)
            HANDLE_CASE(H5INT32, HDF5CFInt32)
            HANDLE_CASE(H5UINT32, HDF5CFUInt32)
            HANDLE_CASE(H5FSTRING, Str)
            HANDLE_CASE(H5VSTRING, Str)
            default:
                throw BESInternalError("Unsupported data type.", __FILE__, __LINE__);
#undef HANDLE_CASE
        }
    }

    vector<size_t> dimsizes;
    dimsizes.resize(var->getRank());
    for (int i = 0; i < var->getRank(); i++)
        dimsizes[i] = (dims[i])->getSize();

    auto ar_unique = make_unique<HDF5CFArray>(var->getRank(), file_id,
                                              filename, var->getType(), dimsizes,
                                              var->getFullPath(),var->getTotalElems(),
                                              CV_UNSUPPORTED, false, var->getCompRatio(),
                                              false,var->getNewName(),bt);
    auto ar = ar_unique.get();

    for (const auto &dim: dims) {
        if ((dim->getNewName()).empty())
            ar->append_dim((int) (dim->getSize()));
        else
            ar->append_dim((int) (dim->getSize()), dim->getNewName());
    }

    // When handling DAP4 CF, we need to generate dmr for 64-bit integer separately.
    if (dap4_int64 == true) {

        DMR *dmr = HDF5RequestHandler::get_dmr_64bit_int();
        D4Group *root_grp = dmr->root();
        // Dimensions need to be translated.
        BaseType *d4_var = ar->h5cfdims_transform_to_dap4_int64(root_grp);
        // Attributes.
        map_cfh5_var_attrs_to_dap4_int64(var, d4_var);
        root_grp->add_var_nocopy(d4_var);
    } else
        dds.add_var(ar);

    delete bt;
}
// Currently, only when the datatype of fillvalue is not the same as the datatype of the variable,
// special attribute handling is needed.
bool need_special_attribute_handling(const HDF5CF::Attribute* attr, const HDF5CF::Var* var)
{
    return ((("_FillValue" == attr->getNewName()) && (var->getType() != attr->getType())) ? true : false);
}

// Currently, we only handle the case when the datatype of _FillValue is not the same as the variable datatype.
void gen_dap_special_oneobj_das(AttrTable*at, const HDF5CF::Attribute* attr, const HDF5CF::Var* var)
{

    BESDEBUG("h5", "Coming to gen_dap_special_oneobj_das()  "<<endl);
    if (attr->getCount() != 1) {
        string msg = "FillValue attribute can only have one element.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    H5DataType var_dtype = var->getType();
    if ((true == HDF5RequestHandler::get_fillvalue_check()) 
        && (false == is_fvalue_valid(var_dtype, attr))) {
        string msg = "The attribute value is out of the range.\n";
        msg += "The variable name: " + var->getNewName() + "\n";
        msg += "The attribute name: " + attr->getNewName() + "\n";
        msg += "The error occurs inside the gen_dap_special_oneobj_das function in h5commoncfdap.cc.";
        throw BESInternalError(msg, __FILE__,__LINE__);
    }
    string print_rep = HDF5CFDAPUtil::print_attr(attr->getType(), 0, (void*) (&(attr->getValue()[0])));
    at->append_attr(attr->getNewName(), HDF5CFDAPUtil::print_type(var_dtype), print_rep);
}

// Check if this fill value is in the valid datatype range when the fillvalue datatype is changed to follow the CF
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

#endif

    default:
        return ret_value;
    }

}

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

        if (nullptr == var) {

            // HDF5 Native Char maps to DAP INT16(DAP2 doesn't have the corresponding datatype), so needs to
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
            if (true == special_attr_handling)
                gen_dap_special_oneobj_das(at, attr, var);
            else {

                // HDF5 Native Char maps to DAP INT16(DAP2 doesn't have the corresponding datatype), so needs to
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

// TODO Use a template function in the switch() below. jhrg 3/9/22
// Generate DMR from one variable                                                                           
void gen_dap_onevar_dmr(libdap::D4Group* d4_grp, const HDF5CF::Var* var, hid_t file_id, const string & filename) {

    BESDEBUG("h5", "Coming to gen_dap_onevar_dmr()  "<<endl);

    const vector<HDF5CF::Dimension *>& dims = var->getDimensions();

    if (dims.empty())
        gen_dap_onevar_dmr_sca(d4_grp, var, filename);
    else
        gen_dap_onevar_dmr_array(d4_grp, var, file_id, filename, dims);

}

void gen_dap_onevar_dmr_sca(D4Group* d4_grp, const HDF5CF::Var* var, const string & filename) {
    
    if (H5FSTRING == var->getType() || H5VSTRING == var->getType()) {
        
        auto sca_str_unique =
                make_unique<HDF5CFStr>(var->getNewName(), filename, var->getFullPath());
        auto sca_str = sca_str_unique.release();
        sca_str->set_is_dap4(true);
        map_cfh5_var_attrs_to_dap4(var, sca_str);
        d4_grp->add_var_nocopy(sca_str);
    }
    else {
        switch (var->getType()) {

            case H5UCHAR: {
                auto sca_uchar_unique = 
                        make_unique<HDF5CFByte>(var->getNewName(), var->getFullPath(), filename);
                auto sca_uchar = sca_uchar_unique.release();
                sca_uchar->set_is_dap4(true);
                map_cfh5_var_attrs_to_dap4(var, sca_uchar);
                d4_grp->add_var_nocopy(sca_uchar);
            }
                break;
            case H5CHAR: {
                
                auto sca_char_unique = 
                        make_unique<HDF5CFInt8>(var->getNewName(), var->getFullPath(), filename);
                auto sca_char = sca_char_unique.release();
                sca_char->set_is_dap4(true);
                map_cfh5_var_attrs_to_dap4(var, sca_char);
                d4_grp->add_var_nocopy(sca_char);
            }
                break;

            case H5INT16: {

                auto sca_int16_unique = 
                        make_unique<HDF5CFInt16>(var->getNewName(), var->getFullPath(), filename);
                auto sca_int16 = sca_int16_unique.release();
                sca_int16->set_is_dap4(true);
                map_cfh5_var_attrs_to_dap4(var, sca_int16);
                d4_grp->add_var_nocopy(sca_int16);
            }
                break;
            case H5UINT16: {

                auto sca_uint16_unique = 
                        make_unique<HDF5CFUInt16>(var->getNewName(), var->getFullPath(), filename);
                auto sca_uint16 = sca_uint16_unique.release();
                sca_uint16->set_is_dap4(true);
                map_cfh5_var_attrs_to_dap4(var, sca_uint16);
                d4_grp->add_var_nocopy(sca_uint16);
            }
                break;
            case H5INT32: {

                auto sca_int32_unique = 
                        make_unique<HDF5CFInt32>(var->getNewName(), var->getFullPath(), filename);
                auto sca_int32 = sca_int32_unique.release();
                sca_int32->set_is_dap4(true);
                map_cfh5_var_attrs_to_dap4(var, sca_int32);
                d4_grp->add_var_nocopy(sca_int32);
            }
                break;
            case H5UINT32: {

                auto sca_uint32_unique = 
                        make_unique<HDF5CFUInt32>(var->getNewName(), var->getFullPath(), filename);
                auto sca_uint32 = sca_uint32_unique.release();
                sca_uint32->set_is_dap4(true);
                map_cfh5_var_attrs_to_dap4(var, sca_uint32);
                d4_grp->add_var_nocopy(sca_uint32);
            }
                break;
            case H5INT64: {
                
                auto sca_int64_unique = 
                        make_unique<HDF5CFInt64>(var->getNewName(), var->getFullPath(), filename);
                auto sca_int64 = sca_int64_unique.release();
                sca_int64->set_is_dap4(true);
                map_cfh5_var_attrs_to_dap4(var, sca_int64);
                d4_grp->add_var_nocopy(sca_int64);
            }
                break;
            case H5UINT64: {

                auto sca_uint64_unique = 
                        make_unique<HDF5CFUInt64>(var->getNewName(), var->getFullPath(), filename);
                auto sca_uint64 = sca_uint64_unique.release();
                sca_uint64->set_is_dap4(true);
                map_cfh5_var_attrs_to_dap4(var, sca_uint64);
                d4_grp->add_var_nocopy(sca_uint64);
            }
                break;
            case H5FLOAT32: {

                auto sca_float32_unique = 
                        make_unique<HDF5CFFloat32>(var->getNewName(), var->getFullPath(), filename);
                auto sca_float32 = sca_float32_unique.release();
                sca_float32->set_is_dap4(true);
                map_cfh5_var_attrs_to_dap4(var, sca_float32);
                d4_grp->add_var_nocopy(sca_float32);
            }
                break;
            case H5FLOAT64: {

                auto sca_float64_unique = 
                        make_unique<HDF5CFFloat64>(var->getNewName(), var->getFullPath(), filename);
                auto sca_float64 = sca_float64_unique.release();
                sca_float64->set_is_dap4(true);
                map_cfh5_var_attrs_to_dap4(var, sca_float64);
                d4_grp->add_var_nocopy(sca_float64);
            }
                break;
            default:
                throw BESInternalError("Unsupported data type.", __FILE__, __LINE__);
        }
    }
}

void gen_dap_onevar_dmr_array(D4Group* d4_grp, const HDF5CF::Var* var, hid_t file_id, const string &filename,
                              const vector<HDF5CF::Dimension *>& dims ) {

    BaseType *bt = nullptr;

    switch (var->getType()) {
#define HANDLE_CASE(tid, type)                                  \
            case tid:                                           \
                bt = new (type)(var->getNewName(),var->getFullPath()); \
            break;
        HANDLE_CASE(H5FLOAT32, HDF5CFFloat32)
        HANDLE_CASE(H5FLOAT64, HDF5CFFloat64)
        HANDLE_CASE(H5CHAR, HDF5CFInt8)
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
            throw BESInternalError("Unsupported data type.", __FILE__, __LINE__);
#undef HANDLE_CASE
    }

    vector<size_t> dimsizes;
    dimsizes.resize(var->getRank());
    for (int i = 0; i < var->getRank(); i++)
        dimsizes[i] = (dims[i])->getSize();

    auto ar_unique = make_unique<HDF5CFArray>(var->getRank(), file_id, filename,
                                              var->getType(),dimsizes, var->getFullPath(),
                                              var->getTotalElems(),CV_UNSUPPORTED, false,
                                              var->getCompRatio(), true, var->getNewName(),bt);
    auto ar = ar_unique.get();

    for (const auto &dim: dims) {
        if ((dim->getNewName()).empty())
            ar->append_dim_ll(dim->getSize());
        else
            ar->append_dim_ll(dim->getSize(), dim->getNewName());
    }

    delete bt;
    ar->set_is_dap4(true);
    BaseType *d4_var = ar->h5cfdims_transform_to_dap4(d4_grp);
    map_cfh5_var_attrs_to_dap4(var, d4_var);
    d4_grp->add_var_nocopy(d4_var);

}

/**
 * @brief Transfer string attributes to a DAP2 AttrTable
 *
 * For an HDF5 string attribute stored in a HDF5CF::Attribute, transfer the
 * value(s) of that attribute to a DAP2 AttrTable object. This function
 * handles both single and multiple attribute value cases. It should
 * only be called for string attributes. Except as noted below, the
 * attribute values are escaped so that non-printable ASCII characters
 * are represented by a backslash and an octal code (e.g. \005).The operation
 * occurrs in the libdap4 rather than in the handler level.
 *
 * @note If the given HDF5CF::Attribute uses UTF-8, it will still be
 * escaped as if it were an ASCII string (or used iso-8859-1 encoding)
 * _unless_ the BES key H5.EscapeUTF8Attr is false (default is true).
 *
 * @param at Put the string attribute in this DAP2 AttrTable
 * @param attr The source attribute information
 */
void gen_dap_str_attr(AttrTable *at, const HDF5CF::Attribute *attr)
{
    BESDEBUG("h5", "Coming to gen_dap_str_attr()  " << endl);
    const vector<size_t>& strsize = attr->getStrSize();

    unsigned int temp_start_pos = 0;
    for (unsigned int loc = 0; loc < attr->getCount(); loc++) {
        if (strsize[loc] != 0) {
            string tempstring(attr->getValue().begin() + temp_start_pos, attr->getValue().begin() + temp_start_pos + strsize[loc]);
            temp_start_pos += strsize[loc];

            // Don't escape the special characters. these will be handled in the libdap4. KY 2022-08-25
            // The following comments are for people to understand how utf8 string is escaped.
            // Attributes with values that use the UTF-8 character set _are_ encoded unless the
            // H5.EscapeUTF8Attr is set to false. If the attribute values use ASCII
            // (i.e., attr->getCsetType() is true), they are always escaped. jhrg 3/9/22
            // Note: Attributes named 'origname' or 'fullnamepath' will not be treated as special
            // because the user can set H5.EscapeUTF8Attr false if the utf8 string occurs inside a variable name.
            // KY 2023-07-13

            if (HDF5RequestHandler::get_escape_utf8_attr() == false && (false == attr->getCsetType())) 
                at->append_attr(attr->getNewName(), "String", tempstring,true);
            else 
                at->append_attr(attr->getNewName(), "String", tempstring);
        }
    }
}

// This function adds the 1-D horizontal coordinate variables as well as the dummy projection variable to the grid.
// Note: Since we don't add these artificial CF variables to our main engineering at HDFEOS5CF.cc, so we need to
// re-call the routines to check projections and dimensions.
// The time to retrieve this information is trivial compared with the whole translation.
void add_cf_grid_cvs(DDS & dds, EOS5GridPCType cv_proj_code, float cv_point_lower, float cv_point_upper,
                        float cv_point_left, float cv_point_right, const vector<HDF5CF::Dimension*>& dims)
{

    //1. Check the projection information: we first just handled the sinusoidal projection. 
    // We also add the LAMAZ and PS support. These 1-D variables are the same as the sinusoidal one.
    if (HE5_GCTP_SNSOID == cv_proj_code || HE5_GCTP_LAMAZ == cv_proj_code || HE5_GCTP_PS == cv_proj_code) {

        //2. Obtain the dimension information from latitude and longitude(fieldtype =1 or fieldtype =2)
        string dim0name = dims[0]->getNewName();
        auto dim0size = (int)(dims[0]->getSize());
        string dim1name = dims[1]->getNewName();
        auto dim1size = (int)(dims[1]->getSize());

        //3. Add the 1-D CV variables and the dummy projection variable
        auto bt_dim0_unique = make_unique<HDF5CFFloat64>(dim0name, dim0name);
        auto bt_dim0 = bt_dim0_unique.get();
        auto bt_dim1_unique = make_unique<HDF5CFFloat64>(dim1name, dim1name);
        auto bt_dim1 = bt_dim1_unique.get();

        // Note ar_dim0 is y, ar_dim1 is x.
        auto ar_dim0_unique = make_unique<HDF5CFGeoCF1D>
                (HE5_GCTP_SNSOID, cv_point_upper, cv_point_lower, dim0size, dim0name, bt_dim0);
        auto ar_dim0 = ar_dim0_unique.get();
        ar_dim0->append_dim(dim0size, dim0name);

        auto ar_dim1_unique = make_unique<HDF5CFGeoCF1D>
                (HE5_GCTP_SNSOID, cv_point_left, cv_point_right, dim1size, dim1name, bt_dim1);
        auto ar_dim1 = ar_dim1_unique.get();
        ar_dim1->append_dim(dim1size, dim1name);
        dds.add_var(ar_dim0);
        dds.add_var(ar_dim1);
    }
}

// This function adds the grid mapping variables.
void add_cf_grid_mapinfo_var(DDS & dds, EOS5GridPCType grid_proj_code, unsigned short g_suffix)
{

    //Add the dummy projection variable. The attributes of this variable can be used to store the grid mapping info.
    // To handle multi-grid cases, we need to add suffixes to distinguish them.
    string cf_projection_base = "eos_cf_projection";

    if (HE5_GCTP_SNSOID == grid_proj_code)  {
        // AFAIK, one grid_mapping variable is necessary for multi-grids. So we just leave one grid here.
        if (g_suffix == 1) {
            auto dummy_proj_cf_unique =
                    make_unique<HDF5CFGeoCFProj>(cf_projection_base, cf_projection_base);
            auto dummy_proj_cf = dummy_proj_cf_unique.get();
            dds.add_var(dummy_proj_cf);
        }
    }
    else {
        stringstream t_suffix_ss;
        t_suffix_ss << g_suffix;
        string cf_projection_name = cf_projection_base + "_" + t_suffix_ss.str();
         auto dummy_proj_cf_unique =
                 make_unique<HDF5CFGeoCFProj>(cf_projection_name, cf_projection_name);
         auto dummy_proj_cf = dummy_proj_cf_unique.get();
         dds.add_var(dummy_proj_cf);
    }

}

// This function adds 1D grid mapping CF attributes to CV and data variables.
void add_cf_grid_cv_attrs(DAS & das, const vector<HDF5CF::Var*>& vars, EOS5GridPCType cv_proj_code,
    const vector<HDF5CF::Dimension*>& dims,const vector<double> &eos5_proj_params, unsigned short g_suffix)
{

    //1. Check the projection information, now, we handle sinusoidal,PS and LAMAZ projections.
    if (HE5_GCTP_SNSOID == cv_proj_code || HE5_GCTP_PS == cv_proj_code || HE5_GCTP_LAMAZ== cv_proj_code) {

        string dim0name = (dims[0])->getNewName();
        auto dim0size = (int)(dims[0]->getSize());
        string dim1name = (dims[1])->getNewName();
        auto dim1size = (int)(dims[1]->getSize());

        //2. Add 1D CF attributes to the 1-D CV variables and the dummy grid_mapping variable
        AttrTable *at = das.get_table(dim0name);
        if (!at)
            at = das.add_table(dim0name, obtain_new_attr_table());
        at->append_attr("standard_name", "String", "projection_y_coordinate");

        string long_name = "y coordinate of projection ";
        at->append_attr("long_name", "String", long_name);

        // Change this to meter.
        at->append_attr("units", "string", "meter");

        at->append_attr("_CoordinateAxisType", "string", "GeoY");

        at = das.get_table(dim1name);
        if (!at)
            at = das.add_table(dim1name, obtain_new_attr_table());

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
        if (HE5_GCTP_SNSOID == cv_proj_code)
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
void add_cf_projection_attrs(DAS &das,EOS5GridPCType cv_proj_code,const vector<double> &eos5_proj_params,
                             const string& cf_projection) {

    AttrTable* at = das.get_table(cf_projection);
    if (!at) {// Only append attributes when the table is created.
        at = das.add_table(cf_projection, obtain_new_attr_table());

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

            // I did this map is based on my best understanding. I cannot be certain about value for the South Pole. KY
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

    // Check >=2-D fields, check if they hold the dim0name,dim0size etc., yes, add the attribute cf_projection.
    for (const auto &var:vars) {

        if (var->getRank() > 1) {
            bool has_dim0 = false;
            bool has_dim1 = false;
            const vector<HDF5CF::Dimension*>& dims = var->getDimensions();
            for (const auto &dim:dims) {
                if (dim->getNewName() == dim0name && dim->getSize() == dim0size)
                    has_dim0 = true;
                else if (dim->getNewName() == dim1name && dim->getSize() == dim1size) 
                    has_dim1 = true;

            }
            if (true == has_dim0 && true == has_dim1) {        // Need to add the grid_mapping attribute
                AttrTable *at = das.get_table(var->getNewName());
                if (!at)
                    at = das.add_table(var->getNewName(), obtain_new_attr_table());

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
// Note: the main part of DMR still comes from DDS and DAS.
bool need_attr_values_for_dap4(const HDF5CF::Var *var) {
    bool ret_value = false;
    if((HDF5RequestHandler::get_dmr_64bit_int()!=nullptr) && 
        (H5INT64 == var->getType() || H5UINT64 == var->getType()))
        ret_value = true;
    return ret_value;
}

// This routine is for 64-bit DAP4 CF support: map all attributes to DAP4 for 64-bit integers.
// Note: the main part of DMR still comes from DDS and DAS.
void map_cfh5_var_attrs_to_dap4_int64(const HDF5CF::Var *var,BaseType* d4_var) {

    for (const auto & attr:var->getAttributes()) {
        // HDF5 Native Char maps to DAP INT16(DAP doesn't have the corresponding datatype), so needs to
        // obtain the mem datatype. Keep this in DAP4 mapping.
        size_t mem_dtype_size = (attr->getBufSize()) / (attr->getCount());
        H5DataType mem_dtype = HDF5CFDAPUtil::get_mem_dtype(attr->getType(), mem_dtype_size);

        string dap2_attrtype = HDF5CFDAPUtil::print_type(mem_dtype);
        D4AttributeType dap4_attrtype = HDF5CFDAPUtil::daptype_strrep_to_dap4_attrtype(dap2_attrtype);
        auto d4_attr_unique = make_unique<D4Attribute>(attr->getNewName(),dap4_attrtype);
        auto d4_attr = d4_attr_unique.release();
        if (dap4_attrtype == attr_str_c) {
            if ("coordinates" == attr->getNewName()) {
                bool chg_coor_value = false;
                if ((true == HDF5RequestHandler::get_enable_coord_attr_add_path())
                   &&(true == var->getCoorAttrAddPath()))
                    chg_coor_value = true;
                string tempstring;
                handle_coor_attr_for_int64_var(attr,var->getFullPath(),tempstring,chg_coor_value);
                d4_attr->add_value(tempstring);
            }
            else {
                const vector<size_t>& strsize = attr->getStrSize();
                unsigned int temp_start_pos = 0;
                for (unsigned int loc = 0; loc < attr->getCount(); loc++) {
                    if (strsize[loc] != 0) {
                        string tempstring(attr->getValue().begin() + temp_start_pos,
                                      attr->getValue().begin() + temp_start_pos + strsize[loc]);
                        temp_start_pos += strsize[loc];
                        d4_attr->add_value(tempstring);
                    }
                }
            }
        }
        else {
            for (unsigned int loc = 0; loc < attr->getCount(); loc++) {
                string print_rep = HDF5CFDAPUtil::print_attr(mem_dtype, loc, (void*) &(attr->getValue()[0]));
                d4_attr->add_value(print_rep);
            }
        }
        d4_var->attributes()->add_attribute_nocopy(d4_attr);
    }

    // Here we add the "origname" and "fullnamepath" attributes since they are crucial to DMRPP generation.
    auto d4_attro_unique = make_unique<D4Attribute>("origname",attr_str_c);
    auto d4_attro = d4_attro_unique.release();
    d4_attro->add_value(var->getName());
    d4_var->attributes()->add_attribute_nocopy(d4_attro);

    auto d4_attrf_unique = make_unique<D4Attribute>("fullnamepath",attr_str_c);
    auto d4_attrf = d4_attrf_unique.release();
    d4_attrf->add_value(var->getFullPath());
    d4_var->attributes()->add_attribute_nocopy(d4_attrf);
}

// A helper function for 64-bit DAP4 CF support that converts DAP2 to DAP4
// Note: the main part of DMR still comes from DDS and DAS.
void check_update_int64_attr(const string & obj_name, const HDF5CF::Attribute * attr) {

    if (attr->getType() == H5INT64 || attr->getType() == H5UINT64) {

        DMR * dmr = HDF5RequestHandler::get_dmr_64bit_int();
        if (dmr != nullptr) {

            string dap2_attrtype = HDF5CFDAPUtil::print_type(attr->getType());
            D4AttributeType dap4_attrtype = HDF5CFDAPUtil::daptype_strrep_to_dap4_attrtype(dap2_attrtype);

            auto d4_attr_unique = make_unique<D4Attribute>(attr->getNewName(),dap4_attrtype);
            auto d4_attr = d4_attr_unique.release();
            for (unsigned int loc = 0; loc < attr->getCount(); loc++) {
                string print_rep = HDF5CFDAPUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
                d4_attr->add_value(print_rep);
            }
            D4Group * root_grp = dmr->root();
            if (root_grp->attributes()->empty() == true) {
                auto d4_hg_container_unique = make_unique<D4Attribute>();
                auto d4_hg_container = d4_hg_container_unique.release();
                d4_hg_container->set_name("HDF5_GLOBAL_integer_64");
                d4_hg_container->set_type(attr_container_c);
                root_grp->attributes()->add_attribute_nocopy(d4_hg_container);
            }

            D4Attribute *d4_hg_container = root_grp->attributes()->get("HDF5_GLOBAL_integer_64");
            if (obj_name.empty() == false) {
                string test_obj_name = "HDF5_GLOBAL_integer_64."+obj_name;
                D4Attribute *d4_container = root_grp->attributes()->get(test_obj_name);
                if (d4_container == nullptr) {
                    auto d4_container_unique = make_unique<D4Attribute>();
                    d4_container = d4_container_unique.release();
                    d4_container->set_name(obj_name);
                    d4_container->set_type(attr_container_c);
                }
                d4_container->attributes()->add_attribute_nocopy(d4_attr);
                if (d4_hg_container->attributes()->get(obj_name)==nullptr)
                    d4_hg_container->attributes()->add_attribute_nocopy(d4_container);
            }
            else 
                d4_hg_container->attributes()->add_attribute_nocopy(d4_attr);
        }
    }
}

// Another helper function for 64-bit DAP4 CF support
// Note: the main part of DMR still comes from DDS and DAS.
void handle_coor_attr_for_int64_var(const HDF5CF::Attribute *attr,const string &var_path,string &tempstring,
                                    bool chg_coor_value) {

    string tempstring2(attr->getValue().begin(),attr->getValue().end()); 
    if(true == chg_coor_value) {
        char sep=' ';
        vector<string>cvalue_vec;
        HDF5CFUtil::Split_helper(cvalue_vec,tempstring2,sep);
        for (unsigned int i = 0; i<cvalue_vec.size();i++) {
            HDF5CFUtil::cha_co(cvalue_vec[i],var_path);
            string t_str = get_cf_string(cvalue_vec[i]);
            if(i == 0) 
                tempstring = t_str;
            else 
                tempstring += sep+t_str;
        }
    }
    else 
        tempstring = tempstring2;

}

// This routine is for direct mapping from CF to DAP4. We build DMR not from DDS and DAS. 
// Hopefully this will be eventually used to build DMR. 
void map_cfh5_var_attrs_to_dap4(const HDF5CF::Var *var,BaseType* d4_var) {

    for (const auto &attr:var->getAttributes()) {
        D4Attribute *d4_attr = gen_dap4_attr(attr);
        d4_var->attributes()->add_attribute_nocopy(d4_attr);
    }
}

// This routine is for direct mapping from CF to DAP4. We build DMR not from DDS and DAS. 
void map_cfh5_grp_attr_to_dap4(libdap::D4Group *d4_grp,const HDF5CF::Attribute *attr) {

    D4Attribute *d4_attr = gen_dap4_attr(attr);
    d4_grp->attributes()->add_attribute_nocopy(d4_attr);

}

// This routine is for direct mapping from CF to DAP4. We build DMR not from DDS and DAS. 
void map_cfh5_attr_container_to_dap4(libdap::D4Attribute *d4_con,const HDF5CF::Attribute *attr) {

    D4Attribute *d4_attr = gen_dap4_attr(attr);
    d4_con->attributes()->add_attribute_nocopy(d4_attr);

}

// Helper function to generate a DAP4 attribute.
D4Attribute *gen_dap4_attr(const HDF5CF::Attribute *attr) {

    D4AttributeType dap4_attrtype = HDF5CFDAPUtil::print_type_dap4(attr->getType());
    auto d4_attr_unique = make_unique<D4Attribute>(attr->getNewName(),dap4_attrtype);
    auto d4_attr = d4_attr_unique.release();

    if (dap4_attrtype == attr_str_c) {
            
        const vector<size_t>& strsize = attr->getStrSize();

        unsigned int temp_start_pos = 0;
        for (unsigned int loc = 0; loc < attr->getCount(); loc++) {
            if (strsize[loc] != 0) {
                string tempstring(attr->getValue().begin() + temp_start_pos,
                                  attr->getValue().begin() + temp_start_pos + strsize[loc]);
                temp_start_pos += strsize[loc];
                d4_attr->add_value(tempstring);
            }
        }
        if (HDF5RequestHandler::get_escape_utf8_attr() == false && (false == attr->getCsetType()))
                    d4_attr->set_utf8_str_flag(true);
    }
    else {
        for (unsigned int loc = 0; loc < attr->getCount(); loc++) {
            string print_rep = HDF5CFDAPUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
            d4_attr->add_value(print_rep);
        }
    }

    return d4_attr;
}

// Direct CF to DAP4, add CF grid_mapping attributes of the projection variable to DAP4.  
// we support sinusodial, polar stereographic and lambert azimuthal equal-area(LAMAZ) projections.
void add_gm_oneproj_var_dap4_attrs(BaseType *var,EOS5GridPCType cv_proj_code,const vector<double> &eos5_proj_params) {

    if (HE5_GCTP_SNSOID == cv_proj_code) {

        add_var_dap4_attr(var,"grid_mapping_name",attr_str_c,"sinusoidal"); 
        add_var_dap4_attr(var,"longitude_of_central_meridian",attr_float64_c,"0.0"); 
        add_var_dap4_attr(var,"earth_radius", attr_float64_c, "6371007.181");
        add_var_dap4_attr(var,"_CoordinateAxisTypes", attr_str_c, "GeoX GeoY");

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

        add_var_dap4_attr(var,"grid_mapping_name",attr_str_c,"polar_stereographic"); 

        ostringstream s_vert_lon_pole;
        s_vert_lon_pole << vert_lon_pole;

        // I did this map is based on my best understanding. I cannot be certain about value for the South Pole. KY
        // CF: straight_vertical_longitude_from_pole
        add_var_dap4_attr(var,"straight_vertical_longitude_from_pole", attr_float64_c, s_vert_lon_pole.str());

        ostringstream s_lat_true_scale;
        s_lat_true_scale << lat_true_scale;
        add_var_dap4_attr(var,"standard_parallel", attr_float64_c, s_lat_true_scale.str());

        if(fe == 0.0) 
            add_var_dap4_attr(var,"false_easting",attr_float64_c,"0.0"); 
        else { 
            ostringstream s_fe;
            s_fe << fe;
            add_var_dap4_attr(var,"false_easting",attr_float64_c,s_fe.str());
        }

        if(fn == 0.0) 
            add_var_dap4_attr(var,"false_northing",attr_float64_c,"0.0");
        else { 
            ostringstream s_fn;
            s_fn << fn;
            add_var_dap4_attr(var,"false_northing",attr_float64_c,s_fn.str());
        }
        
        if (lat_true_scale >0)
            add_var_dap4_attr(var,"latitude_of_projection_origin",attr_float64_c,"+90.0");
        else 
            add_var_dap4_attr(var, "latitude_of_projection_origin",attr_float64_c,"-90.0");

        add_var_dap4_attr(var, "_CoordinateAxisTypes", attr_str_c, "GeoX GeoY");

        // From CF, PS has another parameter,
        // Either standard_parallel (EPSG 9829) or scale_factor_at_projection_origin (EPSG 9810)
        // I cannot find the corresponding parameter from the EOS5.

    }
    else if(HE5_GCTP_LAMAZ == cv_proj_code) {

        double lon_proj_origin = HE5_EHconvAng(eos5_proj_params[4],HE5_HDFE_DMS_DEG);
        double lat_proj_origin = HE5_EHconvAng(eos5_proj_params[5],HE5_HDFE_DMS_DEG);
        double fe = eos5_proj_params[6];
        double fn = eos5_proj_params[7];

        add_var_dap4_attr(var,"grid_mapping_name", attr_str_c, "lambert_azimuthal_equal_area");

        ostringstream s_lon_proj_origin;
        s_lon_proj_origin << lon_proj_origin;
        add_var_dap4_attr(var,"longitude_of_projection_origin", attr_float64_c, s_lon_proj_origin.str());
        
        ostringstream s_lat_proj_origin;
        s_lat_proj_origin << lat_proj_origin;

        add_var_dap4_attr(var,"latitude_of_projection_origin", attr_float64_c, s_lat_proj_origin.str());

        if(fe == 0.0) 
            add_var_dap4_attr(var,"false_easting",attr_float64_c,"0.0");
        else { 
            ostringstream s_fe;
            s_fe << fe;
            add_var_dap4_attr(var,"false_easting",attr_float64_c,s_fe.str());
        }

        if(fn == 0.0) 
            add_var_dap4_attr(var,"false_northing",attr_float64_c,"0.0");
        else { 
            ostringstream s_fn;
            s_fn << fn;
            add_var_dap4_attr(var,"false_northing",attr_float64_c,s_fn.str());
        }
        add_var_dap4_attr(var,"_CoordinateAxisTypes", attr_str_c, "GeoX GeoY");
    }
}

// Direct CF to DAP4, add the CF "grid_mapping_name" attribute to every variable that uses the grid.
void add_cf_grid_cv_dap4_attrs(D4Group *d4_root, const string& cf_projection,      
                               const vector<HDF5CF::Dimension*>& dims, const vector<string> & cvar_name)                    
{
    // dims are dimensions for a grid. It is always 2-D for the projections we support.t
    string dim0name = (dims[0])->getNewName();
    hsize_t dim0size = dims[0]->getSize();
    string dim1name = (dims[1])->getNewName();
    hsize_t dim1size = dims[1]->getSize();

    // We only add the attribute to the variables that match the grid dimensions.
    Constructor::Vars_iter vi = d4_root->var_begin();
    Constructor::Vars_iter ve = d4_root->var_end();
    for (; vi != ve; vi++) {
        // Should not add grid_mapping info for the coordinate variables. 
        if ((*vi)->is_vector_type() && (cvar_name.end() == find(cvar_name.begin(), cvar_name.end(),(*vi)->name()))) {
            auto t_a = dynamic_cast<Array*>(*vi);
            if (t_a->dimensions() >1) {

                bool has_dim0 = false;
                bool has_dim1 = false;
                add_cf_grid_cv_dap4_attrs_helper(t_a, dim0name, dim0size, has_dim0, dim1name, dim1size, has_dim1);
                if (true == has_dim0 && true == has_dim1)
                    add_var_dap4_attr((*vi),"grid_mapping",attr_str_c,cf_projection);
            }
        }
    }
}

void add_cf_grid_cv_dap4_attrs_helper(Array *t_a, const string &dim0name, hsize_t dim0size, bool &has_dim0,
                                      const string &dim1name, hsize_t dim1size, bool &has_dim1){

    Array::Dim_iter dim_i = t_a->dim_begin();
    Array::Dim_iter dim_e = t_a->dim_end();

    for(;dim_i !=dim_e;dim_i++) {
        if ((*dim_i).name == dim0name && (*dim_i).size == (int64_t)dim0size)
            has_dim0 = true;
        else if ((*dim_i).name == dim1name && (*dim_i).size == (int64_t)dim1size)
            has_dim1 = true;
    }

}

// Direct CF to DAP4, add special CF grid_mapping variable to DAP4.  
// These variables are dimension variables.
void add_gm_spcvs(libdap::D4Group *d4_root, EOS5GridPCType cv_proj_code, float cv_point_lower, float cv_point_upper,
                  float cv_point_left, float cv_point_right, const std::vector<HDF5CF::Dimension*>& dims) {

    //1. Check the projection information: we first just handled the sinusoidal projection. 
    // We also add the LAMAZ and PS support. These 1-D varaibles are the same as the sinusoidal one.
    if (HE5_GCTP_SNSOID == cv_proj_code || HE5_GCTP_LAMAZ == cv_proj_code || HE5_GCTP_PS == cv_proj_code) {

        //2. Obtain the dimension information from latitude and longitude(fieldtype =1 or fieldtype =2)
        string dim0name = dims[0]->getNewName();
        auto dim0size = (int)(dims[0]->getSize());
        string dim1name = dims[1]->getNewName();
        auto dim1size = (int)(dims[1]->getSize());

        auto bt_dim0_unique = make_unique<HDF5CFFloat64>(dim0name, dim0name);
        auto bt_dim0 = bt_dim0_unique.get();
        auto bt_dim1_unique = make_unique<HDF5CFFloat64>(dim1name, dim1name);
        auto bt_dim1 = bt_dim1_unique.get();

        auto ar_dim0_unique = make_unique<HDF5CFGeoCF1D>
                (HE5_GCTP_SNSOID, cv_point_upper, cv_point_lower, dim0size, dim0name, bt_dim0);
        auto ar_dim0 = ar_dim0_unique.release();
        ar_dim0->append_dim(dim0size, dim0name);
        ar_dim0->set_is_dap4(true);
        add_gm_spcvs_attrs(ar_dim0,true);
        d4_root->add_var_nocopy(ar_dim0);

        auto ar_dim1_unique = make_unique<HDF5CFGeoCF1D>
                (HE5_GCTP_SNSOID, cv_point_left, cv_point_right, dim1size, dim1name, bt_dim1);
        auto ar_dim1 = ar_dim1_unique.release();
        ar_dim1->append_dim(dim1size, dim1name);
        ar_dim1->set_is_dap4(true);
        add_gm_spcvs_attrs(ar_dim1,false);
        d4_root->add_var_nocopy(ar_dim1);
    }
}

// Direct CF to DAP4, 
// add CF grid_mapping $attributes for the special dimension variables.  
void add_gm_spcvs_attrs(libdap::BaseType *var, bool is_dim0) {

    string standard_name;
    string long_name;
    string COORAxisTypes;

    if (true == is_dim0) {
        standard_name = "projection_y_coordinate";
        long_name = "y coordinate of projection ";
        COORAxisTypes = "GeoY";
    }
    else {
        standard_name = "projection_x_coordinate";
        long_name = "x coordinate of projection ";
        COORAxisTypes = "GeoX";
    }
       
    add_var_dap4_attr(var,"standard_name", attr_str_c, standard_name);
    add_var_dap4_attr(var,"long_name", attr_str_c, long_name);
    add_var_dap4_attr(var,"units", attr_str_c, "meter");
    add_var_dap4_attr(var,"_CoordinateAxisType", attr_str_c, COORAxisTypes);

}

// Direct CF to DAP4, helper function to DAP4 group  attributes.  
void add_grp_dap4_attr(D4Group *d4_grp,const string& attr_name, D4AttributeType attr_type, const string& attr_value){

    auto d4_attr_unique = make_unique<D4Attribute>(attr_name,attr_type);
    auto d4_attr = d4_attr_unique.release();
    d4_attr->add_value(attr_value);
    d4_grp->attributes()->add_attribute_nocopy(d4_attr);

}
// Direct CF to DAP4, helper function to DAP4 variable  attributes.  
void add_var_dap4_attr(BaseType *var,const string& attr_name, D4AttributeType attr_type, const string& attr_value){

    auto d4_attr_unique = make_unique<D4Attribute>(attr_name,attr_type);
    auto d4_attr = d4_attr_unique.release();
    d4_attr->add_value(attr_value);
    var->attributes()->add_attribute_nocopy(d4_attr);

}

// Add DAP4 coverage 
void add_dap4_coverage(libdap::D4Group* d4_root, const vector<string>& coord_var_names, bool is_coard) {

    // We need to construct the var name to Array map,using unordered_map for quick search.
    unordered_map<string, Array*> d4map_array_maps;

    // This vector holds all variables that can have coverage maps.
    vector<Array*> has_map_arrays;
  
    Constructor::Vars_iter vi = d4_root->var_begin();
    Constructor::Vars_iter ve = d4_root->var_end();

    for (; vi != ve; vi++) {

        const BaseType *v = *vi;

        // Only Array can have maps. Construct the d4map_array_maps  as well as the has_map_arrays.
        if (libdap::dods_array_c == v->type()) {
            auto t_a = dynamic_cast<Array *>(*vi);
            add_dap4_coverage_set_up(d4map_array_maps, has_map_arrays, t_a, coord_var_names, v->name());
        }
    }

    // loop through the has_map_arrays to add the maps.
    if (is_coard) {// The grid case.
        add_dap4_coverage_grid(d4map_array_maps,has_map_arrays);
    }
    else { // A Swath case, need to find coordinates and retrieve the values.
        add_dap4_coverage_swath(d4map_array_maps,has_map_arrays);
    }

    // We need to set the second element of the d4map_array_maps to 0 to avoid the calling of the ~Array()
    // when this map goes out of loop.
    for (auto& d4map_array_map:d4map_array_maps) 
        d4map_array_map.second = nullptr;
 
}

void add_dap4_coverage_set_up(unordered_map<string, Array*> &d4map_array_maps, vector<Array *> &has_map_arrays,
                              Array *t_a, const vector<string>& coord_var_names, const string & vname) {

    // The maps are essentially coordinate variables.
    // We've sorted the coordinate variables already, so
    // just save them to the vector.
    // Note: the coordinate variables we collect here are
    // based on the understanding of the handler. We will
    // watch if there are complicated cases down the road. KY 04-15-2022
    bool is_cv = false;
    for (const auto &cvar_name: coord_var_names) {
        if (cvar_name == vname) {
            is_cv = true;
            d4map_array_maps.emplace(vname, t_a);
            break;
        }
    }

    // If this is not a map variable, it has a good chance to hold maps.
    if (is_cv == false)
        has_map_arrays.emplace_back(t_a);

}

void add_dap4_coverage_grid(unordered_map<string, Array*> &d4map_array_maps, vector<Array *> &has_map_arrays) {

    for (auto &has_map_array: has_map_arrays) {

        Array::Dim_iter dim_i = has_map_array->dim_begin();
        Array::Dim_iter dim_e = has_map_array->dim_end();
        for (; dim_i != dim_e; dim_i++) {

            // The dimension name is the same as a map name(A Grid case)
            // Need to ensure the map array can be found.
            unordered_map<string, Array *>::const_iterator it_ma = d4map_array_maps.find(dim_i->name);
            if (it_ma != d4map_array_maps.end()) {
                auto d4_map_unique = make_unique<D4Map>((it_ma->second)->FQN(), it_ma->second);
                auto d4_map = d4_map_unique.release();
                has_map_array->maps()->add_map(d4_map);
            }
        }
        // Need to set the has_map_arrays to 0 to avoid calling ~Array() when the vector goes out of loop.
        has_map_array = nullptr;
    }
}

void add_dap4_coverage_swath(unordered_map<std::string, libdap::Array*> &d4map_array_maps,
                            const std::vector<libdap::Array *> &has_map_arrays) {

    for (auto has_map_array: has_map_arrays) {

        // Find the "coordinates".
        vector<string> coord_names;

        // The coord dimension names cannot be the third dimension.
        unordered_set<string> coord_dim_names;

        D4Attributes *d4_attrs = has_map_array->attributes();
        const D4Attribute *d4_attr = d4_attrs->find("coordinates");
        if (d4_attr != nullptr) {
            // For all the coordinates the CF option can handle,
            // the attribute is a one-element string.
            if (d4_attr->type() == attr_str_c && d4_attr->num_values() == 1) {
                string tempstring = d4_attr->value(0);
                char sep = ' ';
                HDF5CFUtil::Split_helper(coord_names, tempstring, sep);
            }
        }

        add_dap4_coverage_swath_coords(d4map_array_maps, has_map_array, coord_names, coord_dim_names);

        // Need to set the has_map_arrays to 0 to avoid calling ~Array() when the vector goes out of loop.
        has_map_array = nullptr;
    }
}

void add_dap4_coverage_swath_coords(unordered_map<string, Array*> &d4map_array_maps, Array *has_map_array,
                                    const vector<string> &coord_names, unordered_set<string> &coord_dim_names) {

    // Search if these coordinates can be found in the coordinate variable list.
    // If yes, add maps.
    for (const auto &cname: coord_names) {

        unordered_map<string, Array *>::const_iterator it_ma = d4map_array_maps.find(cname);
        if (it_ma != d4map_array_maps.end()) {

            auto d4_map_unique = make_unique<D4Map>((it_ma->second)->FQN(), it_ma->second);
            auto d4_map = d4_map_unique.release();
            has_map_array->maps()->add_map(d4_map);

            // We need to find the dimension names of these coordinates.
            Array::Dim_iter dim_i = it_ma->second->dim_begin();
            Array::Dim_iter dim_e = it_ma->second->dim_end();
            for (; dim_i != dim_e; dim_i++)
                coord_dim_names.insert(dim_i->name);
        }
    }

    // Some variables have the third or the fourth dimensions that don't belong to dimensions of the cvs from coordinate attribute.
    // For these dimensions, we need to check if any real CV(like dimension scale) exists. If yes, add them to it.
    Array::Dim_iter dim_i = has_map_array->dim_begin();
    Array::Dim_iter dim_e = has_map_array->dim_end();
    for (; dim_i != dim_e; dim_i++) {

        // This dimension is not found among dimensions of coordinate variables. Check if it is a valid map candidate.If yes, add it.
        if (coord_dim_names.find(dim_i->name) == coord_dim_names.end()) {
            unordered_map<string, Array *>::const_iterator it_ma = d4map_array_maps.find(dim_i->name);
            if (it_ma != d4map_array_maps.end()) {
                auto d4_map_unique = make_unique<D4Map>((it_ma->second)->FQN(), it_ma->second);
                auto d4_map = d4_map_unique.release();
                has_map_array->maps()->add_map(d4_map);
            }
        }
    }
}

// Mainly copy from HDF5CF::get_CF_string. Should be 
// removed if we can generate DMR independently.
string get_cf_string(string & s) {

    if(s[0] !='/') 
        return get_cf_string_helper(s);
    else {
        // The leading underscore should be removed 
        s.erase(0,1);
        return get_cf_string_helper(s);
    }

}
string get_cf_string_helper(string & s) {

    if (s.empty())
        return s;
    string insertString(1, '_');

    // Always start with _ if the first character is not a letter
    if (true == isdigit(s[0])) s.insert(0, insertString);

    for (unsigned int i = 0; i < s.size(); i++)
        if ((false == isalnum(s[i])) && (s[i] != '_')) s[i] = '_';
    return s;
}
