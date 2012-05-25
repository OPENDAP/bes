
// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  

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
/// Maximum number of dimensions in an array(default option only).
#define DODS_MAX_RANK 30
/// Maximum length of variable or attribute name(default option only).
#define DODS_NAMELEN    1024
/// The special DAS attribute name for HDF5 path information from the top(root) group.
#define HDF5_OBJ_FULLPATH "HDF5_OBJ_FULLPATH"

#include "config_hdf5.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <iostream>
#include <string>
#include <sstream>

#include <debug.h>
#include <DAS.h>
#include <DDS.h>
#include <parser.h>
#include <ConstraintEvaluator.h>
#include <InternalErr.h>
#include <hdf5.h>

/// \brief A structure for DDS generation
typedef struct DS {
    /// Name of HDF5 group or dataset
    char name[DODS_NAMELEN];
    /// HDF5 data set id
    hid_t dset;
    /// HDF5 data type id
    hid_t type;
    /// HDF5 data space id
    hid_t dataspace;
    /// Number of dimensions
    int ndims;
    /// Size of each dimension
    int size[DODS_MAX_RANK];
    /// Number of elements 
    hsize_t nelmts;
    /// Space needed 
    size_t need;
} DS_t;
/// \brief A structure for DAS generation
typedef struct DSattr {
    /// Name of HDF5 group or dataset
    char name[DODS_NAMELEN];
    /// Memory type
    int type;
    /// Number of dimensions
    int ndims;
    /// Size of each dimension
    int size[DODS_MAX_RANK];
    /// Number of elements 
    hsize_t nelmts;
    /// Memory space needed to hold nelmts type.
    size_t need;
} DSattr_t;

/// An abstract respresntation of DAP String type.
static const char STRING[] = "String";
/// An abstract respresntation of DAP Byte type.
static const char BYTE[] = "Byte";
/// An abstract respresntation of DAP Signed Byte type.
static const char INT8[] = "Int8"; 
/// An abstract respresntation of DAP Int32 type.
static const char INT32[] = "Int32";
/// An abstract respresntation of DAP Int16 type.
static const char INT16[] = "Int16";
/// An abstract respresntation of DAP Float64 type.
static const char FLOAT64[] = "Float64";
/// An abstract respresntation of DAP Float32 type.
static const char FLOAT32[] = "Float32";
/// An abstract respresntation of DAP Uint16 type.
static const char UINT16[] = "UInt16";
/// An abstract respresntation of DAP UInt32 type.
static const char UINT32[] = "UInt32";
/// For umappable HDF5 integer data types.
static const char INT_ELSE[] = "Int_else";
/// For unmappable HDF5 float data types.
static const char FLOAT_ELSE[] = "Float_else";
/// An abstract respresntation of DAP Structure type.
static const char COMPOUND[] = "Structure";
/// An abstract respresntation of DAP Array type.
static const char ARRAY[] = "Array";   
/// An abstract respresntation of DAP Url type.
static const char URL[] = "Url";       


#include "h5das.h"
#include "h5dds.h"
#include "h5get.h"              
#include "HDF5PathFinder.h"

/// Adding CF options

//#include "h5cfdds.h"

using namespace libdap;
#endif
