#ifndef _HDFCLASS_H
#define _HDFCLASS_H

// -*- C++ -*-

//////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// Copyright 1996, by the California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.

// U.S. Government Sponsorship under NASA Contract
// NAS7-1260 is acknowledged.

// Author: Todd.K.Karakashian@jpl.nasa.gov
//
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <vector>

using std::vector;
using std::string;

#ifdef NO_BOOL
enum bool { false = 0, true = 1 };
#endif

// Global values -- someday change "struct" to "namespace"
// The enum-inside-of-struct hack is more portable than the previous
// const int-inside-of-struct hack, which didn't work with Sun C++
struct hdfclass {
    enum hdfenum {
        MAXSTR = 32767,         // maximum length of a string
        MAXDIMS = 20,           // maximum dimensions in a dataset
        MAXANNOTS = 20,         // max number of annotations per object
        MAXREFS = 1000          // max number of instances of a tag in a file
    };
};

// generic vector class
class hdf_genvec {
  public:
    hdf_genvec(void);
    hdf_genvec(int32 nt, void *data, int begin, int end, int stride = 1);
    hdf_genvec(int32 nt, void *data, int nelts);
    hdf_genvec(const hdf_genvec & gv);

    virtual ~ hdf_genvec(void);

    hdf_genvec & operator=(const hdf_genvec & gv);

    int32 number_type(void) const {
        return _nt;
    } int size(void) const {
        return _nelts;
    } const char *data(void) const {
        return _data;
    } // use with care!
        void append(int32 nt, const char *new_data, int32 nelts);

    void import(int32 nt, void *data, int nelts) {
        import(nt, data, 0, nelts - 1, 1);
    } 
    
    void import(int32 nt, void *data, int begin, int end, int stride = 1);
    void import(int32 nt) {
        import(nt, 0, 0, 0, 0);
    }
    void import(int32 nt, const vector < string > &sv);

    void print(vector < string > &strv) const;
    void print(vector < string > &strv, int begin, int end,
               int stride) const;

    vector < uchar8 > exportv_uchar8(void) const;
    uchar8 *export_uchar8(void) const;
    uchar8 elt_uchar8(int i) const;
    vector < char8 > exportv_char8(void) const;
    char8 *export_char8(void) const;
    char8 elt_char8(int i) const;
    vector < uint8 > exportv_uint8(void) const;
    uint8 *export_uint8(void) const;
    uint8 elt_uint8(int i) const;
    vector < int8 > exportv_int8(void) const;
    int8 *export_int8(void) const;
    int8 elt_int8(int i) const;
    vector < uint16 > exportv_uint16(void) const;
    uint16 *export_uint16(void) const;
    uint16 elt_uint16(int i) const;
    vector < int16 > exportv_int16(void) const;
    int16 *export_int16(void) const;
    int16 elt_int16(int i) const;
    vector < uint32 > exportv_uint32(void) const;
    uint32 *export_uint32(void) const;
    uint32 elt_uint32(int i) const;
    vector < int32 > exportv_int32(void) const;
    int32 *export_int32(void) const;
    int32 elt_int32(int i) const;
    vector < float32 > exportv_float32(void) const;
    float32 *export_float32(void) const;
    float32 elt_float32(int i) const;
    vector < float64 > exportv_float64(void) const;
    float64 *export_float64(void) const;
    float64 elt_float64(int i) const;
    string export_string(void) const;

  protected:
    void _init(int32 nt, void *data, int begin, int end, int stride = 1);
    void _init(void);
    void _init(const hdf_genvec & gv);
    void _del(void);

    int32 _nt;                  // HDF data type of vector
    int _nelts;                 // number of elements in vector
    char *_data;                // hold data
};

// HDF attribute class
class hdf_attr {
  public:
    string name;                // name of attribute
    hdf_genvec values;          // value(s) of attribute
};

// Structure to hold array-type constraint information. Used within
// hdfistream_sds (See hcstream.h)
struct array_ce {
    string name;
    int start;
    int edge;
    int stride;
    array_ce(const string & n, int s1, int e, int s3)
    :name(n), start(s1), edge(e), stride(s3) {
}};

// HDF dimension class - holds dimension info and scale for an SDS
class hdf_dim {
  public:
    string name;                // name of dimension
    string label;               // label of dimension
    string unit;                // units of this dimension
    string format;              // format to use when displaying
    int32 count;                // size of this dimension
    hdf_genvec scale;           // dimension scale data
    vector < hdf_attr > attrs;  // vector of dimension attributes
};

// HDF SDS class
class hdf_sds {
  public:
    bool operator!(void) const {
        return !_ok();
    } bool has_scale(void) const;       // does this SDS have a dim scale?
    int32 ref;                  // ref number of SDS
    string name;
    vector < hdf_dim > dims;    // dimensions and dimension scales
    hdf_genvec data;            // data stored in SDS
    vector < hdf_attr > attrs;  // vector of attributes
  protected:
    bool _ok(bool * has_scale = 0) const;       // is this hdf_sds correctly initialized?
};

class hdf_field {
  public:
    friend class hdf_vdata;
    bool operator!(void) const {
        return !_ok();
    } string name;
    vector < hdf_genvec > vals; // values for field; vals.size() is order of field
  protected:
    bool _ok(void) const;       // is this hdf_field correctly initialized?
};

class hdf_vdata {
  public:
    bool operator!(void) const {
        return !_ok();
    } int32 ref;                // ref number
    string name;                // name of vdata
    string vclass;              // class name of vdata
    vector < hdf_field > fields;
    vector < hdf_attr > attrs;
  protected:
    bool _ok(void) const;       // is this hdf_vdata correctly initialized?
};

// Vgroup class
class hdf_vgroup {
  public:
    bool operator!(void) const {
        return !_ok();
    } int32 ref;                // ref number of vgroup
    string name;                // name of vgroup
    string vclass;              // class name of vgroup
    vector < int32 > tags;      // vector of tags inside vgroup
    vector < int32 > refs;      // vector of refs inside vgroup
    vector < string > vnames;   // vector of variable name(refs) inside vgroup
    vector < hdf_attr > attrs;
  protected:
    bool _ok(void) const;       // is this hdf_vgroup correctly initialized?
};

// Palette container class 
class hdf_palette {
  public:
    string name;                // palettes name
    hdf_genvec table;           // palette
    int32 ncomp;                // number of components
    int32 num_entries;          // number of entries
};

// Raster container class based on the gernal raster image (GR API)
class hdf_gri {
  public:
    int32 ref;                  // ref number of raster
    string name;
    vector < hdf_palette > palettes;    // vector of associated palettes
    vector < hdf_attr > attrs;  // vector of attributes
    int32 dims[2];              // the image dimensions
    int32 num_comp;             // the number of components
    int32 interlace;            // the interlace mode of the image
    hdf_genvec image;           // the image
    bool operator!(void) const {
        return !_ok();
    } bool has_palette(void) const {
        return (palettes.size() > 0 ? true : false);
  } protected:
    bool _ok(void) const;
};

// misc utility functions

#if 0
vector < string > split(const string & str, const string & delim);
#endif
string join(const vector < string > &sv, const string & delim);
bool SDSExists(const char *filename, const char *sdsname);
bool GRExists(const char *filename, const char *grname);
bool VdataExists(const char *filename, const char *vdname);

#endif                          // _HDFCLASS_H

