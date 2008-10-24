///////////////////////////////////////////////////////////////////////////////
/// \file common.h
/// \brief a header for common constants and global structures
/// 
/// This file is a header file that includes common constants and global
/// structures of the server. 
///
/// \author  Muqun Yang (myang6@hdfgroup.org)
/// 
/// Copyright (C) 2007   The HDF Group
///
/// Copyright (C) 2001   National Center for Supercomputing Applications.
///                      All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _common_H
#define _common_H

#include <H5Ipublic.h>
/// Maximum number of dimensions in an array.
#define DODS_MAX_RANK 30
/// Maximum length of variable or attribute name
#define DODS_NAMELEN    1024
/// Maximum length of a dimension name in DIMENSION_LIST attribute.
#define HDF5_DIMVARLEN  24
/// The name of dimension list attribute used in HDF5.
#define HDF5_DIMENSIONLIST "DIMENSIONLIST"
/// The name of dimension name list attribute used in HDF5.
#define HDF5_DIMENSIONNAMELIST "DIMENSION_NAMELIST"
/// The name of dimension list attribute used in HDF4.
#define OLD_HDF5_DIMENSIONLIST "DIMSCALE"
/// The name of dimension name list attribute used in HDF4.
#define OLD_HDF5_DIMENSIONNAMELIST "HDF4_DIMENSION_LIST"
/// This enables generation of Grid from HDF5 array with dimension scales.
/// If this is commented out, no Grid will be generated from HDF5.
#define DODSGRID
/// The special DAS attribute name for HDF5 soft link.
#define HDF5_softlink "HDF5_softlink"
/// The special DAS attribute name for HDF5 hard link.
#define HDF5_hardlink "HDF5_hardlink"
/// The special DAS attribute name for HDF5 path information from the top(root) group.
#define HDF5_OBJ_FULLPATH "HDF5_OBJ_FULLPATH"

/// A flag to indicate whether an array can be mapped to grid or not.
/// This also indicates whether the Grid-mappable HDF5 array is in old or new format.
enum H5GridFlag_t {
    NotGrid,
    OldH4H5Grid,
    NewH4H5Grid
};

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

#endif
