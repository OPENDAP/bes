// This file is part of the hdf5_handler implementing for the CF-compliant
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

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5CFUtil.h
/// \brief This file includes several helper functions for translating HDF5 to CF-compliant
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2013 The HDF Group
///
/// All rights reserved.

#ifndef _HDF5CFUtil_H
#define _HDF5CFUtil_H
#include <string.h>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include "hdf5.h"

// We create this intermediate enum H5DataType in order to totally separate the 
// creating of DAS and DDS from any HDF5 API calls. When mapping to DAP, only this
// intermediate H5DataType will be used to map to the corresponding DAP datatype.
// Here H5UNSUPTYPE includes H5T_TIME, H5T_BITFIELD, H5T_OPAQUE,H5T_ENUM, H5T_VLEN.
// For CF option, H5REFERENCE, H5COMPOUND, H5ARRAY will not be supported. We leave them
// here for future merging of default option and CF option. Currently DAP2 doesn't 
// support 64-bit integer. We still list int64 bit types since we find ACOSL2S has this
// datatype and we need to provide a special mapping for this datatype. 
// H5UCHAR also needs a special mapping. Similiarly other unsupported types may need to 
// have special mappings in the future. So the following enum type may be extended 
// according to the future need. The idea is that all the necessary special mappings should
// be handled in the HDF5CF name space. 
// The DDS and DAS generation modules should not use any HDF5 APIs.
enum H5DataType
{H5FSTRING, H5FLOAT32,H5CHAR,H5UCHAR,H5INT16,H5UINT16,
 H5INT32,H5UINT32,H5INT64,H5UINT64,H5FLOAT64,H5VSTRING,
 H5REFERENCE,H5COMPOUND,H5ARRAY,H5UNSUPTYPE};

using namespace std;

struct HDF5CFUtil {


               /// Map HDF5 Datatype to the intermediate H5DAPtype for the future use.
               static H5DataType H5type_to_H5DAPtype(hid_t h5_type_id);

               /// Trim the string with many NULL terms or garbage characters to simply a string
               /// with a NULL terminator. This method will not handle the NULL PAD case.
               static string trim_string(hid_t dtypeid,const string s, int num_sect, size_t section_size, vector<size_t>& sect_newsize);

               static string obtain_string_after_lastslash(const string s);
               static bool cf_strict_support_type(H5DataType dtype); 

               // Obtain the unique name for the clashed names and save it to set namelist.
               static void gen_unique_name(string &str, set<string>&namelist,int&clash_index);

               static void ClearMem(int* offset, int*count,int*step,hsize_t*hoffset,hsize_t*hcount,hsize_t*hstep);

               

};

#endif
