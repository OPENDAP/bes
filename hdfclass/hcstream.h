#ifndef _HCSTREAM_H
#define _HCSTREAM_H

//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1996, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//		       NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: hcstream.h,v $ - stream class declarations for HDFClass
// 
//////////////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>

#include <hcerr.h>
#include <hdfclass.h>

class hdfistream_obj {		// base class for streams reading HDF objects
public:
    hdfistream_obj(const string filename="") { _init(filename); }
    hdfistream_obj(const hdfistream_obj&) { THROW(hcerr_copystream); }
    virtual ~hdfistream_obj(void) {}
    void operator=(const hdfistream_obj&) { THROW(hcerr_copystream); }
    virtual void open(const char *filename=0) = 0; // open stream
    virtual void close(void) = 0;		   // close stream
    virtual void seek(int index=0) = 0; // seek to index'th object
    virtual void seek_next(void) = 0;      // seek to next object in stream
    virtual void rewind(void) = 0;	   // rewind to beginning of stream
    virtual bool bos(void) const = 0;	   // are we at beginning of stream?
    virtual bool eos(void) const = 0;	   // are we at end of stream?
    virtual int index(void) const { return _index; } // return current position
protected:
    void _init(const string filename="") {
	if (filename.length()) _filename = filename;
	_file_id = _index = 0;  
    }
    string _filename;
    int32 _file_id;
    int _index;
};

class hdfistream_sds : public hdfistream_obj {
public:
    hdfistream_sds(const string filename="");
    hdfistream_sds(const hdfistream_sds&) { THROW(hcerr_copystream); }
    virtual ~hdfistream_sds(void) { _del(); }
    void operator=(const hdfistream_sds&) { THROW(hcerr_copystream); }
    virtual void open(const char *filename=0); // open stream, seek to BOS
    virtual void close(void);	       // close stream
    virtual void seek(int index=0);    // seek the index'th SDS array
    virtual void seek(const char *name); // seek the SDS array by name
    virtual void seek_next(void);	 // seek the next SDS array
    virtual void seek_ref(int ref);      // seek the SDS array by ref
    virtual void rewind(void);      // position in front of first SDS
    virtual bool bos(void) const;      // positioned in front of the first SDS?
    virtual bool eos(void) const;      // positioned past the last SDS?
    virtual bool eo_attr(void) const;  // positioned past the last attribute?
    virtual bool eo_dim(void) const;    // positioned past the last dimension?
    void setmeta(bool val) { _meta = val; }  // set metadata loading
    void setslab(vector<int> start, vector<int> edge, vector<int> stride,
		 bool reduce_rank = false);
    void setslab(int *start, int *edge, int *stride, bool reduce_rank = false);
    void unsetslab(void) { _slab.set = _slab.reduce_rank = false; }
    hdfistream_sds& operator>>(hdf_attr& ha); // read an attribute
    hdfistream_sds& operator>>(vector<hdf_attr>& hav); // read all atributes
    hdfistream_sds& operator>>(hdf_sds& hs);	       // read an SDS
    hdfistream_sds& operator>>(vector<hdf_sds>& hsv);  // read all SDS's
    hdfistream_sds& operator>>(hdf_dim& hd);	       // read a dimension
    hdfistream_sds& operator>>(vector<hdf_dim>& hdv);  // read all dims
protected:
    void _init(void);
    void _del(void) { close(); }
    void _get_fileinfo(void);	// get SDS info for the current file
    void _get_sdsinfo(void);	// get info about the current SDS
    void _close_sds(void);	// close the open SDS
    void _seek_next_arr(void);	// find the next SDS array in the stream
    void _seek_arr(int index);	// find the index'th SDS array in the stream
    void _seek_arr(const string& name);	// find the SDS array w/ specified name
    void _seek_arr_ref(int ref); // find the SDS array in the stream by ref
    void _rewind(void) { _index = -1; _attr_index = _dim_index = 0; }
				// position to the beginning of the stream
    static const string long_name; // label
    static const string units;
    static const string format;
    int32 _sds_id;	     // handle of open object in annotation interface
    int32 _attr_index;	     // index of current attribute
    int32 _dim_index;	     // index of current dimension
    int32 _rank;	     // number of dimensions in SDS
    int32 _nattrs;	     // number of attributes for this SDS
    int32 _nsds;	     // number of SDS's in stream
    int32 _nfattrs;	     // number of file attributes in this SDS's file
    bool _meta;
    struct {
	bool set;
	bool reduce_rank;
	int32 start[hdfclass::MAXDIMS];
	int32 edge[hdfclass::MAXDIMS];
	int32 stride[hdfclass::MAXDIMS];
    } _slab;
};

// class for a stream reading annotations
class hdfistream_annot : public hdfistream_obj {	
public:
    hdfistream_annot(const string filename="");
    hdfistream_annot(const string filename, int32 tag, int32 ref); 
    hdfistream_annot(const hdfistream_annot&) { THROW(hcerr_copystream); }
    virtual ~hdfistream_annot(void) { _del(); }
    void operator=(const hdfistream_annot&) { THROW(hcerr_copystream); }
    virtual void open(const char *filename); // open file annotations
    virtual void open(const char *filename, int32 tag, int32 ref); 
                                        // open tag/ref in file, seek to BOS
    virtual void close(void);	        // close open file
    virtual void seek(int index)        // seek to index'th annot in stream
	{ _index = index; }
    virtual void seek_next(void) { _index++; } // position to next annot
    virtual bool eos(void) const {return (_index >= (int)_an_ids.size()); }
    virtual bool bos(void) const {return (_index <= 0); }
    virtual void rewind(void) { _index = 0; }
    virtual void set_annot_type(bool label, bool desc)  // specify types of
	{ _lab = label; _desc = desc; }		// annots to read
    hdfistream_annot& operator>>(string& an);    // read current annotation
    hdfistream_annot& operator>>(vector<string>& anv); // read all annots
protected:
    void _init(const string filename="");
    void _init(const string filename, int32 tag, int32 ref);
    void _del(void) { close(); }
    void _rewind(void) { _index = 0; }
    void _get_anninfo(void);
    void _get_file_anninfo(void);
    void _get_obj_anninfo(void);
    void _open(const char *filename);
    int32 _an_id;	     // handle of open annotation interface
    int32 _tag, _ref;		// tag, ref currently pointed to
    bool _lab;		// if true, read labels
    bool _desc;	// if true, read descriptions
    vector<int32> _an_ids; // list of id's of anns in stream
};

class hdfistream_vdata : public hdfistream_obj {
public:
    hdfistream_vdata(const string filename="");
    hdfistream_vdata(const hdfistream_vdata&) { THROW(hcerr_copystream); }
    virtual ~hdfistream_vdata(void) { _del(); }
    void operator=(const hdfistream_vdata&) { THROW(hcerr_copystream); }
    virtual void open(const char *filename); // open stream, seek to BOS
    virtual void open(const string& filename); // open stream, seek to BOS
    virtual void close(void);	       // close stream
    virtual void seek(int index=0);    // seek the index'th Vdata
    virtual void seek(const char *name); // seek the Vdata by name
    virtual void seek(const string& name); // seek the Vdata by name
    virtual void seek_next(void) { _seek_next(); } // seek the next Vdata in file
    virtual void seek_ref(int ref);        // seek the Vdata by ref
    virtual void rewind(void) { _rewind(); } // position in front of first Vdata
    virtual bool bos(void) const    // positioned in front of the first Vdata?
	{ return (_index <= 0); }
    virtual bool eos(void) const      // positioned past the last Vdata?
	{ return (_index >= (int)_vdata_refs.size()); }
    virtual bool eo_attr(void) const;  // positioned past the last attribute?
    void setmeta(bool val) { _meta = val; }  // set metadata loading
    bool setrecs(int32 begin, int32 end);
    hdfistream_vdata& operator>>(hdf_vdata& hs);	       // read a Vdata
    hdfistream_vdata& operator>>(vector<hdf_vdata>& hsv);  // read all Vdata's
    hdfistream_vdata& operator>>(hdf_attr& ha);          // read an attribute
    hdfistream_vdata& operator>>(vector<hdf_attr>& hav); // read all attributes
    virtual bool isInternalVdata(int ref) const;  // check reference against internal type
protected:
    void _init(void);
    void _del(void) { close(); }
    void _get_fileinfo(void);	// get Vdata info for the current file
    void _seek_next(void); // find the next Vdata in the stream
    void _seek(const char *name); // find the Vdata w/ specified name
    void _seek(int32 ref); // find the index'th Vdata in the stream
    void _rewind(void)  // position to beginning of the stream
	{ _index = _attr_index = 0; if (_vdata_refs.size() > 0) _seek(_vdata_refs[0]); }
    int32 _vdata_id;	     // handle of open object in annotation interface
    int32 _attr_index;       // index of current attribute
    int32 _nattrs;           // number of attributes for this Vdata
    hdfistream_vdata& operator>>(hdf_field& hf);	   // read a field
    hdfistream_vdata& operator>>(vector<hdf_field>& hfv);  // read all fields
    bool _meta;
    vector<int32> _vdata_refs;	// list of refs for Vdata's in the file
    struct {
      bool set;
      int32 begin;
      int32 end;
    } _recs;
};

class hdfistream_vgroup : public hdfistream_obj {
public:
    hdfistream_vgroup(const string filename="");
    hdfistream_vgroup(const hdfistream_vgroup&) { THROW(hcerr_copystream); }
    virtual ~hdfistream_vgroup(void) { _del(); }
    void operator=(const hdfistream_vgroup&) { THROW(hcerr_copystream); }
    virtual void open(const char *filename); // open stream, seek to BOS
    virtual void open(const string& filename); // open stream, seek to BOS
    virtual void close(void);	       // close stream
    virtual void seek(int index=0);    // seek the index'th Vgroup
    virtual void seek(const char *name); // seek the Vgroup by name
    virtual void seek(const string& name); // seek the Vgroup by name
    virtual void seek_next(void) { _seek_next(); } // seek the next Vgroup in file
    virtual void seek_ref(int ref);      // seek the Vgroup by ref
    virtual void rewind(void) { _rewind(); } // position in front of first Vgroup
    virtual bool bos(void) const    // positioned in front of the first Vgroup?
	{ return (_index <= 0); }
    virtual bool eos(void) const      // positioned past the last Vgroup?
	{ return (_index >= (int)_vgroup_refs.size()); }
    virtual bool eo_attr(void) const;  // positioned past the last attribute?
    void setmeta(bool val) { _meta = val; }  // set metadata loading
    hdfistream_vgroup& operator>>(hdf_vgroup& hs);	       // read a Vgroup
    hdfistream_vgroup& operator>>(vector<hdf_vgroup>& hsv);  // read all Vgroup's
    hdfistream_vgroup& operator>>(hdf_attr& ha);          // read an attribute
    hdfistream_vgroup& operator>>(vector<hdf_attr>& hav); // read all attributes
protected:
    void _init(void);
    void _del(void) { close(); }
    void _get_fileinfo(void);	// get Vgroup info for the current file
    void _seek_next(void); // find the next Vgroup in the stream
    void _seek(const char *name); // find the Vgroup w/ specified name
    void _seek(int32 ref); // find the index'th Vgroup in the stream
    void _rewind(void)  // position to beginning of the stream
	{ _index = _attr_index = 0; if (_vgroup_refs.size() > 0) _seek(_vgroup_refs[0]); }
    int32 _vgroup_id;	     // handle of open object in annotation interface
    int32 _attr_index;       // index of current attribute
    int32 _nattrs;           // number of attributes for this Vgroup
    bool _meta;
    vector<int32> _vgroup_refs;	// list of refs for Vgroup's in the file
    struct {
      bool set;
      int32 begin;
      int32 end;
    } _recs;
};

// Raster input stream class
class hdfistream_gri : public hdfistream_obj {
public:
    hdfistream_gri(const string filename="");
    hdfistream_gri(const hdfistream_gri &) { THROW(hcerr_copystream); }
    virtual ~hdfistream_gri(void) { _del(); }
    void operator=(const hdfistream_gri &) { THROW(hcerr_copystream); }
    virtual void open(const char *filename=0); // open stream
    virtual void close(void);                  // close stream
    virtual void seek(int index=0);            // seek the index'th image
    virtual void seek(const char *name);       // seek image by name
    virtual void seek_next(void) { seek(_index+1); } // seek the next RI
    virtual void seek_ref(int ref);            // seek the RI by ref
    virtual void rewind(void);                 // position in front of first RI
    virtual bool bos(void) const;              // position in front of first RI?
    virtual bool eos(void) const;              // position past last RI?
    virtual bool eo_attr(void) const;          // positioned past last attribute?
    virtual bool eo_pal(void) const;           // positioned past last palette?
    void setmeta(bool val) { _meta = val; }    // set metadata loading
    void setslab(vector<int> start, vector<int> edge, vector<int> stride,
		 bool reduce_rank = false);
    void unsetslab(void) { _slab.set = _slab.reduce_rank = false; }
    void setinterlace(int32 interlace_mode);   // set interlace type for next read
    hdfistream_gri & operator>>(hdf_gri & hr);              // read a single RI
    hdfistream_gri & operator>>(vector<hdf_gri> & hrv);     // read all RI's
    hdfistream_gri & operator>>(hdf_attr & ha);             // read an attribute
    hdfistream_gri & operator>>(vector<hdf_attr> & hav);    // read all attributes
    hdfistream_gri & operator>>(hdf_palette & hp);          // read a palette
    hdfistream_gri & operator>>(vector<hdf_palette> & hpv); // read all palettes
protected:
    void _init(void);
    void _del(void) { close(); }
    void _get_fileinfo(void);   // get image info for the current file
    void _get_iminfo(void);     // get info about the current RI
    void _close_ri(void);       // close the current RI
    void _rewind(void) { _index = -1; _attr_index = _pal_index = 0; }
    int32 _gr_id;               // GR interface id -> can't get rid of this, needed for GRend()
    int32 _ri_id;               // handle for open raster object 
    int32 _attr_index;          // index to current attribute
    int32 _pal_index;           // index to current palette
    int32 _nri;                 // number of rasters in the stream
    int32 _nattrs;              // number of attributes for this RI
    int32 _nfattrs;             // number of file attributes in this RI's file
    int32 _npals;               // number of palettes, set to one for now
    int32 _interlace_mode;      // interlace mode for reading images
    bool _meta;                 // metadata only
    struct {
	bool set;
	bool reduce_rank;
	int32 start[2];
	int32 edge[2];
	int32 stride[2];
    } _slab;
}; /* Note: multiple palettes is not supported in the current HDF 4.0 GR API */

// $Log: hcstream.h,v $
// Revision 1.8  2001/08/27 17:21:34  jimg
// Merged with version 3.2.2
//
// Revision 1.7.4.1  2001/05/15 17:54:47  dan
// Added hdfistream_vdata method isInternalVdata.
//
// Revision 1.7  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.6  1999/05/06 03:23:33  jimg
// Merged changes from no-gnu branch
//
// Revision 1.5  1999/05/05 23:33:43  jimg
// String --> string conversion
//
// Revision 1.4.6.1  1999/05/06 00:35:45  jimg
// Jakes String --> string changes
//
// Revision 1.4  1998/09/10 23:03:46  jehamby
// Add support for Vdata and Vgroup attributes
//
// Revision 1.3  1998/07/13 20:26:35  jimg
// Fixes from the final test of the new build process
//
// Revision 1.2.4.1  1998/05/22 19:50:52  jimg
// Patch from Jake Hamby to support subsetting raster images
//
// Revision 1.2  1998/04/03 18:34:18  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.1  1996/10/31 18:43:01  jimg
// Added.
//
// Revision 1.10  1996/09/20  17:53:42  ike
// Added setinterlace() and _interlace_mode to hdfistream_gri.
//
// Revision 1.9  1996/08/14  22:36:17  ike
// Added hdfistream_vdata:setrecs().
//
// Revision 1.8  1996/08/14  17:56:37  todd
// Added slab setting member function, slab protected data member to hdfistream_sds.
//
// Revision 1.7  1996/07/22  17:13:30  todd
// Const-corrected hdfistream_gri::seek() declaration.
//
// Revision 1.6  1996/06/19  18:28:39  todd
// Fixed a bug in _rewind which caused a core dump if there were no Vdata's in the stream.
//
// Revision 1.5  1996/06/18  21:53:55  todd
// Removed one constructor to be consistent with the interfaces of the other stream
// classes.
//
// Revision 1.4  1996/06/14  23:18:27  ike
// Added hdfistream_gri stream class.
//
// Revision 1.3  1996/06/14  23:07:18  todd
// Added copyright statement.
// Added support for Vdata.
//
// Revision 1.2  1996/04/19  17:40:18  todd
// Added seek(const char *) and _seek_arr(const string&) mfuncs.
// Added a cast in the eos() mfunc to silence g++'s warning.
//
// Revision 1.1  1996/04/19  01:17:02  todd
// Initial revision
//
#endif // ifndef _HCSTREAM_H
