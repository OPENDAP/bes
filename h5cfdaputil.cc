// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2011-2012 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file h5cfdaputil.cc
/// \brief Helper functions for generating DAS attributes and a function to check BES Key.
///
///  
/// \author Muqun Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2012 The HDF Group
///
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////


#include "h5cfdaputil.h"

bool HDF5CFDAPUtil::check_beskeys(const string key) {

    bool found = false;
    string doset ="";
    const string dosettrue ="true";
    const string dosetyes = "yes";

    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( true == found ) {
        doset = BESUtil::lowercase( doset ) ;
        if( dosettrue == doset  || dosetyes == doset )
            return true;
    }
    return false;

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
    string DAPUNSUPPORTED ="Unsupported";
    string DAPBYTE ="Byte";
    string DAPINT16 ="Int16";
    string DAPUINT16 ="Uint16";
    string DAPINT32 ="Int32";
    string DAPUINT32 ="Uint32";
    string DAPFLOAT32 ="Float32";
    string DAPFLOAT64 ="Float64";
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
        case H5UINT64:
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


    case H5FLOAT32:
        {
            gp.fp = (float *) vals;
            rep << showpoint;
            rep << setprecision(10);
            rep << *(gp.fp+loc);
            if (rep.str().find('.') == string::npos
                && rep.str().find('e') == string::npos)
                rep << ".";
            return rep.str();
        }

    case H5FLOAT64:
        {
            gp.dp = (double *) vals;
            rep << std::showpoint;
            rep << std::setprecision(17);
            rep << *(gp.dp+loc);
            if (rep.str().find('.') == string::npos
                && rep.str().find('e') == string::npos)
                rep << ".";
            return rep.str();
            break;
        }
    default:
        return string("UNKNOWN");
    }

}

