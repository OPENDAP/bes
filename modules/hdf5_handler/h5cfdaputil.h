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
/// \file h5cfdaputil.h
/// \brief Helper functions for generating DAS attributes and a function to check BES Key.
///
///  
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#ifndef _HDF5CFDAPUtil_H
#define _HDF5CFDAPUtil_H

#include "HDF5CFUtil.h"
#include <sstream>
#include <iomanip>
#include <TheBESKeys.h>
#include <BESUtil.h>
#include <libdap/D4Attributes.h>

const size_t  NC_JAVA_STR_SIZE_LIMIT=32767;


class HDF5CFDAPUtil {

    public: 
    static H5DataType get_mem_dtype(H5DataType, size_t);
    static string print_type(H5DataType h5type);
    static string print_attr(H5DataType h5type, unsigned int loc, void *vals);
    static void replace_double_quote(string &str);

    /// A customized escaping function to escape special characters following OPeNDAP's escattr function
    /// that can be found at escaping.cc and escaping.h. i
    /// Note: the customized version will not treat
    /// \n(new line),\t(tab),\r(Carriage return) as special characters since NASA HDF files
    /// use this characters to make the attribute easy to read. Escaping these characters in the attributes
    /// will use \012 etc to replace \n etc. in these attributes and make attributes hard to read.
    static string escattr(string s);

    /// Helper function for escattr
    static string octstring(unsigned char val);

    /// Helper function to obtain the DAP4 attribute type. 
    /// We deliberately duplicates the same function for the default option to separate the handling of CF .
    static D4AttributeType daptype_strrep_to_dap4_attrtype(const string &s);
    static D4AttributeType print_type_dap4(H5DataType h5type);
};
#endif
