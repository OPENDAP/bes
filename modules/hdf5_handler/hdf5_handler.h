
// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2009-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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

///////////////////////////////////////////////////////////////////////////////
/// \mainpage
/// 
/// \section Introduction
/// This is the HDF5 OPeNDAP handler that  extracts DAS/DDS/DODS information
/// from an  file. 
///
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// \file hdf5_handler.h
/// \brief The main header of the HDF5 OPeNDAP handler
///////////////////////////////////////////////////////////////////////////////
#ifndef _hdf5_handler_H
#define _hdf5_handler_H

#include "config_hdf5.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <libdap/DAS.h>
#include <libdap/DDS.h>
#include <libdap/parser.h>
#include <libdap/ConstraintEvaluator.h>
#include <libdap/InternalErr.h>
#include <hdf5.h>


/// Maximum number of dimensions in an array(default option only).
const int DODS_MAX_RANK=30;
/// Maximum length of variable or attribute name(default option only).
const int DODS_NAMELEN=1024;
/// The special DAS attribute name for HDF5 path information from the top(root) group.
const std::string HDF5_OBJ_FULLPATH="HDF5_OBJ_FULLPATH";


/// \brief A structure for DDS generation
typedef struct DS {
    /// Name of HDF5 group or dataset
    char name[DODS_NAMELEN];
    /// HDF5 data set id
#if 0
    //hid_t dset;
#endif
    /// HDF5 data type id
    hid_t type;
    /// HDF5 data space id
#if 0
    //hid_t dataspace;
#endif
    /// Number of dimensions
    int ndims;
    /// Size of each dimension
    hsize_t size[DODS_MAX_RANK];
    vector <string> dimnames;
    vector <string> dimnames_path;
    vector <bool> unlimited_dims;
    /// Number of elements 
    hsize_t nelmts;
    /// Space needed 
    hsize_t need;
} DS_t;
/// \brief A structure for DAS generation
typedef struct DSattr {
    /// Name of HDF5 group or dataset
    char name[DODS_NAMELEN];
    /// Memory type
    hid_t type;
    /// Number of dimensions
    int ndims;
    /// Size of each dimension
    /// Note: We don't expect we have a large array>4GB attribute.
    int size[DODS_MAX_RANK];
    /// Number of elements 
    hsize_t nelmts;
    /// Memory space needed to hold nelmts type.
    hsize_t need;
} DSattr_t;

/// An abstract respresntation of DAP String type.
static const char STRING[] = "String";
/// An abstract respresntation of DAP2 Byte type.
static const char BYTE[] = "Byte";
/// An abstract respresntation of DAP4 unsigned 8-bit integer type.
/// Note: DAP2 Byte type is equivalent to DAP4 unsigned 8-bit integer
static const char UINT8[] = "UInt8"; 
/// An abstract respresntation of DAP4 Signed 8-bit integer type.
static const char INT8[] = "Int8"; 
/// An abstract respresntation of DAP Int32 type.
static const char INT32[] = "Int32";
/// An abstract respresntation of DAP Int16 type.
static const char INT16[] = "Int16";
/// An abstract respresntation of DAP Int64 type.
static const char INT64[] = "Int64";

/// An abstract respresntation of DAP Float64 type.
static const char FLOAT64[] = "Float64";
/// An abstract respresntation of DAP Float32 type.
static const char FLOAT32[] = "Float32";
/// An abstract respresntation of DAP Uint16 type.
static const char UINT16[] = "UInt16";
/// An abstract respresntation of DAP UInt32 type.
static const char UINT32[] = "UInt32";
/// An abstract respresntation of DAP UInt64 type.
static const char UINT64[] = "UInt64";

/// For umappable HDF5 integer data types.
/// Note: Int64 and UInt64 are unmappable for DAP2 but not for DAP4.
static const char INT_ELSE[] = "Int_else";
/// For unmappable HDF5 float data types.
static const char FLOAT_ELSE[] = "Float_else";
/// An abstract respresntation of DAP Structure type.
static const char COMPOUND[] = "Structure";
/// An abstract respresntation of DAP Array type.
static const char ARRAY[] = "Array";   
/// An abstract respresntation of DAP Url type.
static const char URL[] = "Url";       

// These header files have to be left in this location. Don't move to the top.
#include "h5das.h"
#include "h5dds.h"
#include "h5dmr.h"
#include "h5get.h"              
#include "HDF5PathFinder.h"

#endif
