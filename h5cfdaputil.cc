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
/// \file h5cfdaputil.cc
/// \brief Helper functions for generating DAS attributes and a function to check BES Key.
///
///  
/// \author Muqun Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////


#include "h5cfdaputil.h"
#include <math.h>

using namespace std;
using namespace libdap;

string HDF5CFDAPUtil::escattr(string s)
{
    const string printable = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~`!@#$%^&*()_-+={[}]|\\:;<,>.?/'\"\n\t\r";
    const string ESC = "\\";
    const string DOUBLE_ESC = ESC + ESC;
    const string QUOTE = "\"";
    const string ESCQUOTE = ESC + QUOTE;

    // escape \ with a second backslash
    size_t ind = 0;
    while ((ind = s.find(ESC, ind)) != string::npos) {
        s.replace(ind, 1, DOUBLE_ESC);
        ind += DOUBLE_ESC.length();
    }

    // escape non-printing characters with octal escape
    ind = 0;
    while ((ind = s.find_first_not_of(printable, ind)) != string::npos)
        s.replace(ind, 1, ESC + octstring(s[ind]));


    // escape " with backslash
    ind = 0;
    while ((ind = s.find(QUOTE, ind)) != string::npos) {
        s.replace(ind, 1, ESCQUOTE);
        ind += ESCQUOTE.length();
    }

    return s;
}

// present the string in octal base.
string
HDF5CFDAPUtil::octstring(unsigned char val)
{
    ostringstream buf;
    buf << oct << setw(3) << setfill('0')
    << static_cast<unsigned int>(val);

    return buf.str();
}


void HDF5CFDAPUtil::replace_double_quote(string & str) {

    const string offend_char = "\"";
    const string replace_str = "&quote";
    size_t found_quote = 0;
    size_t start_pos = 0;
    while (found_quote != string::npos) {
        found_quote = str.find(offend_char,start_pos);
        if (found_quote!= string::npos){
            str.replace(found_quote,offend_char.size(),replace_str);
            start_pos = found_quote+1;
        }
    }
}


string HDF5CFDAPUtil::print_type(H5DataType type) {

    // The list is based on libdap/AttrTable.h.
    // We added DAP4 INT64 and UINT64 support.
    string DAPUNSUPPORTED ="Unsupported";
    string DAPBYTE ="Byte";
    string DAPINT16 ="Int16";
    string DAPUINT16 ="Uint16";
    string DAPINT32 ="Int32";
    string DAPUINT32 ="Uint32";
    string DAPFLOAT32 ="Float32";
    string DAPFLOAT64 ="Float64";
    string DAP4INT64  ="Int64";
    string DAP4UINT64 ="UInt64";
    string DAPSTRING = "String";

    switch (type) {

        case H5UCHAR:
            return DAPBYTE;

        case H5CHAR:
            return DAPINT16;

        case H5INT16:
            return DAPINT16;
        
        case H5UINT16:
            return DAPUINT16;

        case H5INT32:
            return DAPINT32;

        case H5UINT32:
            return DAPUINT32;

        case H5FLOAT32:
            return DAPFLOAT32;

        case H5FLOAT64:
            return DAPFLOAT64;

        case H5FSTRING:
        case H5VSTRING:
            return DAPSTRING;
        case H5INT64:
            return DAP4INT64;
        case H5UINT64:
            return DAP4UINT64;
        case H5REFERENCE:
        case H5COMPOUND:
        case H5ARRAY:
             return DAPUNSUPPORTED;
 
        default:
             return DAPUNSUPPORTED;
    }

}

H5DataType
HDF5CFDAPUtil::get_mem_dtype(H5DataType dtype,size_t mem_dtype_size ) {

    // Currently in addition to "char" to "int16", all other memory datatype will be the same as the datatype.
    // So we have a short cut for this function
    return ((H5INT16 == dtype) && (1 == mem_dtype_size))?H5CHAR:dtype;
}

string
HDF5CFDAPUtil:: print_attr(H5DataType type, int loc, void *vals)
{
    ostringstream rep;

    union {
        unsigned char* ucp;
        char *cp;
        short *sp;
        unsigned short *usp;
        int   *ip;
        unsigned int *uip;
        long long *llp;
        unsigned long long *ullp;
        float *fp;
        double *dp;
    } gp;

    switch (type) {

    case H5UCHAR:
        {
            unsigned char uc;
            gp.ucp = (unsigned char *) vals;

            uc = *(gp.ucp+loc);
            rep << (int)uc;

            return rep.str();
        }

    case H5CHAR:
        {
            gp.cp = (char *) vals;
            char c;
            c = *(gp.cp+loc);
            // Since the character may be a special character and DAP may not be able to represent so supposedly we should escape the character
            // by calling the escattr function. However, HDF5 native char maps to DAP Int16. So the mapping assumes that users will never
            // use HDF5 native char or HDF5 unsigned native char to represent characters. Instead HDF5 string should be used to represent characters.
            // So don't do any escaping of H5CHAR for now. KY 2016-10-14
            rep <<(int)c;
            return rep.str();
        }


    case H5INT16:
        {
            gp.sp = (short *) vals;
            rep<< *(gp.sp+loc);
            return rep.str();
        }

    case H5UINT16:

       {
            gp.usp = (unsigned short *) vals;
            rep << *(gp.usp+loc);
            return rep.str();
        }


    case H5INT32:
        {
            gp.ip = (int *) vals;
            rep << *(gp.ip+loc);
            return rep.str();
        }

    case H5UINT32:
        {
            gp.uip = (unsigned int *) vals;
            rep << *(gp.uip+loc);
            return rep.str();
        }
    case H5INT64: // For DAP4 CF support only
        {
            gp.llp = (long long *) vals;
            rep << *(gp.llp+loc);
            return rep.str();
        }

    case H5UINT64: // For DAP4 CF support only
        {
            gp.ullp = (unsigned long long *) vals;
            rep << *(gp.ullp+loc);
            return rep.str();
        }



    case H5FLOAT32:
        {
            float attr_val = *(float*)vals;
            bool is_a_fin = isfinite(attr_val);
            gp.fp = (float *) vals;
            rep << showpoint;
            rep << setprecision(10);
            rep << *(gp.fp+loc);
            string tmp_rep_str = rep.str();
            if (tmp_rep_str.find('.') == string::npos
                && tmp_rep_str.find('e') == string::npos
                && tmp_rep_str.find('E') == string::npos
                && (true == is_a_fin)){
                rep<<".";
            }
            return rep.str();
        }

    case H5FLOAT64:
        {
            double attr_val = *(double*)vals;
            bool is_a_fin = isfinite(attr_val);
            gp.dp = (double *) vals;
            rep << std::showpoint;
            rep << std::setprecision(17);
            rep << *(gp.dp+loc);
            string tmp_rep_str = rep.str();
            if (tmp_rep_str.find('.') == string::npos
                && tmp_rep_str.find('e') == string::npos
                && tmp_rep_str.find('E') == string::npos
                && (true == is_a_fin)) {
                rep << ".";
            }
            return rep.str();
        }
    default:
        return string("UNKNOWN");
    }

}

// This helper function is used for 64-bit integer DAP4 support.
// We need to support the attributes of all types for 64-bit integer variables.
D4AttributeType HDF5CFDAPUtil::daptype_strrep_to_dap4_attrtype(std::string s){
    
    if (s == "Byte")
        return attr_byte_c;
    else if (s == "Int8")
        return attr_int8_c;
    else if (s == "UInt8") // This may never be used.
        return attr_uint8_c;
    else if (s == "Int16")
        return attr_int16_c;
    else if (s == "UInt16")
        return attr_uint16_c;
    else if (s == "Int32")
        return attr_int32_c;
    else if (s == "UInt32")
        return attr_uint32_c;
    else if (s == "Int64")
        return attr_int64_c;
    else if (s == "UInt64")
        return attr_uint64_c;
    else if (s == "Float32")
        return attr_float32_c;
    else if (s == "Float64")
        return attr_float64_c;
    else if (s == "String")
        return attr_str_c;
    else if (s == "Url")
        return attr_url_c;
    else
        return attr_null_c;


}


