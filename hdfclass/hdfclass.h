#ifndef _HDFCLASS_H
#define _HDFCLASS_H

//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1996, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//		       NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: hdfclass.h,v $ - primary include file for HDFclass library
// 
// $Log: hdfclass.h,v $
// Revision 1.2  1998/04/03 18:34:18  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.1  1996/10/31 18:43:03  jimg
// Added.
//
// Revision 1.16  1996/10/14 23:07:53  todd
// Added a new import function to hdf_genvec to allow import from a vector
// of String.
//
// Revision 1.15  1996/09/20 17:56:02  ike
// Added interlace data member to GR class.
//
// Revision 1.14  1996/08/22  20:53:46  todd
// Added export functions for char8 type.
//
// Revision 1.13  1996/08/14  17:57:28  todd
// Added convenience functions to check for existence of GR's, SDS's, Vdata's.
//
// Revision 1.12  1996/08/13  18:45:51  todd
// Added declarations of element access mfuncs for hdf_genvec.
//
// Revision 1.11  1996/07/22  17:14:32  todd
// Changed export_*() mfuncs so they return a C++ array.  Added exportv_*() mfuncs
// which return STL vectors.
//
// Revision 1.10  1996/06/18  01:24:30  ike
// Fixed typo in hdf_gri.
//
// Revision 1.9  1996/06/18  01:11:28  ike
// Added _ok(), !operator, and has_palette to hdf_gri.
//
// Revision 1.8  1996/06/18  00:52:38  todd
// Fixed order of hdf_palette and hdf_gri.
//
// Revision 1.7  1996/06/17  15:45:39  ike
// Fixed typo in hdf_palette.
//
// Revision 1.6  1996/06/14  23:14:57  ike
// Added hdf_gri, hdf_palette container classes.
//
// Revision 1.5  1996/06/14  23:08:45  todd
// Added copyright statement.
// Modified hdf_genvec interface(revised constructor, added new import() mfuncs, added
// print() mfunc).
// Added operator! to hdf_sds.
// Added hdf_vdata, hdf_field container classes.
// Added split(), join() utility functions.
//
// Revision 1.4  1996/05/07  23:41:53  todd
// Made hdf_genvec::print mfunc const.
// Added has_scale mfunc to hdf_sds.
//
// Revision 1.3  1996/04/18  19:06:59  todd
// Added new mfuncs data() and print() to hdf_genvec.
//
// Revision 1.2  1996/04/04  01:12:23  todd
// Added import() public member function to hdf_genvec class declaration.
//
// Revision 1.1  1996/04/02  20:28:23  todd
// Initial revision
//
//////////////////////////////////////////////////////////////////////////////

#include <iostream.h>
#ifdef __GNUG__
#include <String.h>
//#ifndef NO_EXCEPTIONS
//#include <typeinfo>
//#endif // NO_EXCEPTIONS
#else
#include <bstring.h>
typedef string String;
#endif // __GNUG__
#include <vector.h>

#ifdef NO_BOOL
enum bool {false=0,true=1};
#endif

// Global values -- someday change "struct" to "namespace"
struct hdfclass {		
    const int MAXSTR = 32767;	// maximum length of a string
    const int MAXDIMS = 20;	// maximum dimensions in a dataset
    const int MAXANNOTS = 20;	// max number of annotations per object
    const int MAXREFS=1000;	// max number of instances of a tag in a file
};

// generic vector class
class hdf_genvec {
public:
    hdf_genvec(void);
    hdf_genvec(int32 nt, void *data, int begin, int end, int stride=1);
    hdf_genvec(int32 nt, void *data, int nelts);	 
    hdf_genvec(const hdf_genvec& gv);
    virtual ~hdf_genvec(void);
    hdf_genvec& operator=(const hdf_genvec& gv);
    int32 number_type(void) const { return _nt; }
    int size(void) const { return _nelts; }
    const void *data(void) { return _data; }     // use with care!
    void import(int32 nt, void *data, int nelts) { import(nt, data, 0, nelts-1, 1); }
    void import(int32 nt, void *data, int begin, int end, int stride=1);
    void import(int32 nt) { import(nt, 0, 0, 0, 0); }
    void import(int32 nt, const vector<String>& sv);
    vector<uchar8> exportv_uchar8(void) const; 
    uchar8 *export_uchar8(void) const; 
    uchar8 elt_uchar8(int i) const; 
    vector<char8> exportv_char8(void) const; 
    char8 *export_char8(void) const; 
    char8 elt_char8(int i) const; 
    vector<uint8> exportv_uint8(void) const; 
    uint8 *export_uint8(void) const; 
    uint8 elt_uint8(int i) const; 
    vector<int8> exportv_int8(void) const; 
    int8 *export_int8(void) const; 
    int8 elt_int8(int i) const; 
    vector<uint16> exportv_uint16(void) const; 
    uint16 *export_uint16(void) const; 
    uint16 elt_uint16(int i) const; 
    vector<int16> exportv_int16(void) const; 
    int16 *export_int16(void) const; 
    int16 elt_int16(int i) const; 
    vector<uint32> exportv_uint32(void) const; 
    uint32 *export_uint32(void) const; 
    uint32 elt_uint32(int i) const; 
    vector<int32> exportv_int32(void) const; 
    int32 *export_int32(void) const; 
    int32 elt_int32(int i) const; 
    vector<float32> exportv_float32(void) const; 
    float32 *export_float32(void) const; 
    float32 elt_float32(int i) const; 
    vector<float64> exportv_float64(void) const; 
    float64 *export_float64(void) const; 
    float64 elt_float64(int i) const; 
    String export_string(void) const; 
    void print(vector<String>& strv) const;
    void print(vector<String>& strv, int begin, int end, int stride) const;
protected:
    void _init(int32 nt, void *data, int begin, int end, int stride=1);
    void _init(void);
    void _init(const hdf_genvec& gv);
    void _del(void);
    int32 _nt;			// HDF data type of vector
    int _nelts;			// number of elements in vector
    void *_data;		// hold data
};

// HDF attribute class
class hdf_attr {
public:
    String name;		// name of attribute
    hdf_genvec values;		// value(s) of attribute
};

// HDF dimension class - holds dimension info and scale for an SDS
class hdf_dim {
public:
    String name;		// name of dimension
    String label;		// label of dimension
    String unit;		// units of this dimension
    String format;		// format to use when displaying
    int32 count;		// size of this dimension
    hdf_genvec scale;		// dimension scale data
    vector<hdf_attr> attrs;	// vector of dimension attributes
};

// HDF SDS class
class hdf_sds {
public:
    bool operator!(void) const { return !_ok(); }
    bool has_scale(void) const;	// does this SDS have a dim scale?
    int32 ref;                // ref number of SDS
    String name;
    vector<hdf_dim> dims;	// dimensions and dimension scales
    hdf_genvec data;		// data stored in SDS
    vector<hdf_attr> attrs;	// vector of attributes
protected:
    bool _ok(bool *has_scale=0) const;// is this hdf_sds correctly initialized?
};

class hdf_field {
public:
    friend class hdf_vdata;
    bool operator!(void) const { return !_ok(); }
    String name;
    vector<hdf_genvec> vals; // values for field; vals.size() is order of field
protected:
    bool _ok(void) const;	// is this hdf_field correctly initialized?
};

class hdf_vdata {
public:
    bool operator!(void) const { return !_ok(); }
    int32 ref;                  // ref number
    String name;		// name of vdata
    String vclass;		// class name of vdata
    vector<hdf_field> fields;
protected:
    bool _ok(void) const;	// is this hdf_vdata correctly initialized?
};

// Vgroup class
class hdf_vgroup {
public:
  bool operator!(void) const { return !_ok(); }
  int32 ref;                    // ref number of vgroup
  String name;                  // name of vgroup
  String vclass;                // class name of vgroup
  vector<int32> tags;           // vector of tags inside vgroup
  vector<int32> refs;           // vector of refs inside vgroup
protected:
  bool _ok(void) const;         // is this hdf_vgroup correctly initialized?
};

// Palette container class 
class hdf_palette {
public:
    String name;        // palettes name
    hdf_genvec table;   // palette
    int32 ncomp;        // number of components
    int32 num_entries;  // number of entries
};

// Raster container class based on the gernal raster image (GR API)
class hdf_gri {
public:
    int32 ref;                    // ref number of raster
    String name;
    vector<hdf_palette> palettes; // vector of associated palettes
    vector<hdf_attr> attrs;       // vector of attributes
    int32 dims[2];                // the image dimensions
    int32 num_comp;               // the number of components
    int32 interlace;              // the interlace mode of the image
    hdf_genvec image;             // the image
    bool operator!(void) const { return !_ok(); }
    bool has_palette(void) const { return (palettes.size()>0 ? true : false); }   
protected:
    bool _ok( void ) const;
};

// misc utility functions

vector<String> split(const String& str, const String& delim);
String join(const vector<String>& sv, const String& delim);
bool SDSExists(const char *filename, const char *sdsname);
bool GRExists(const char *filename, const char *grname);
bool VdataExists(const char *filename, const char *vdname);

#endif // _HDFCLASS_H
