//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1996, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//		       NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: sds.cc,v $ - input stream class for HDF SDS
// 
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <mfhdf.h>

#include <string>
#include <vector>

#include <hcstream.h>
#include <hdfclass.h>

// minimum function
inline int min(int t1, int t2) { return ( t1 < t2? t1 : t2 ); }

// static initializations
const string hdfistream_sds::long_name = "long_name";
const string hdfistream_sds::units = "units";
const string hdfistream_sds::format = "format";

//
// protected member functions
//

// initialize an hdfistream_sds, opening file if given
void hdfistream_sds::_init(void) {
    _sds_id = _attr_index = _dim_index = _nsds = _rank = _nattrs = _nfattrs = 0;
    _index = -1;		// set BOS
    _meta = _slab.set = false;
    return;
}

// retrieve descriptive information about file containing SDS
void hdfistream_sds::_get_fileinfo(void) {
    if (SDfileinfo(_file_id, &_nsds, &_nfattrs) < 0)
	THROW(hcerr_sdsinfo);
    return;
}
    
// retrieve descriptive information about currently open SDS
void hdfistream_sds::_get_sdsinfo(void) {
    char junk0[hdfclass::MAXSTR];
    int32 junk1[hdfclass::MAXDIMS];
    int32 junk2;

    // all we care about is rank and number of attributes
    if (SDgetinfo(_sds_id, junk0, &_rank, junk1, &junk2, &_nattrs) < 0)
	THROW(hcerr_sdsinfo);

    if (_rank > hdfclass::MAXDIMS) // too many dimensions
	THROW(hcerr_maxdim);
    return;
}

// end access to currently open SDS
void hdfistream_sds::_close_sds(void) {
    if (_sds_id != 0) {
	(void)SDendaccess(_sds_id);
	_sds_id = _attr_index = _dim_index = _rank = _nattrs = 0;
	_index = -1;
    }
    return;
}

// find the next SDS array (not necessarily the next SDS) in the file
void hdfistream_sds::_seek_next_arr(void) {
    for (_index++,_dim_index=_attr_index=0; _index < _nsds; ++_index) {
	if ( (_sds_id = SDselect(_file_id, _index)) < 0)
	    THROW(hcerr_sdsopen);
	if (!SDiscoordvar(_sds_id))
	    break;
	SDendaccess(_sds_id);
	_sds_id = 0;
    }
}

// find the arr_index'th SDS array in the file (don't count non-array SDS's)
void hdfistream_sds::_seek_arr(int arr_index) {
    int arr_count = 0;
    for (_rewind();
	 _index < _nsds  &&  arr_count <= arr_index; 
	 _seek_next_arr(), arr_count++
	 );
}

// find the SDS array with specified name
void hdfistream_sds::_seek_arr(const string& name) {
    int index;
    const char *nm = name.c_str();

    if ( (index = SDnametoindex(_file_id, (char *)nm)) < 0)
	THROW(hcerr_sdsfind);
    if ( (_sds_id = SDselect(_file_id, index)) < 0)
	THROW(hcerr_sdsopen);
    bool iscoord = SDiscoordvar(_sds_id);
    if (iscoord) {
	SDendaccess(_sds_id);
	_sds_id = 0;
	THROW(hcerr_sdsfind);
    }
    _index = index;
    return;
}

// find the SDS array with specified ref
void hdfistream_sds::_seek_arr_ref(int ref) {
    int index;

    if ( (index = SDreftoindex(_file_id, ref)) < 0)
	THROW(hcerr_sdsfind);
    if ( (_sds_id = SDselect(_file_id, index)) < 0)
	THROW(hcerr_sdsopen);
    bool iscoord = SDiscoordvar(_sds_id);
    if (iscoord) {
	SDendaccess(_sds_id);
	_sds_id = 0;
	THROW(hcerr_sdsfind);
    }
    _index = index;
    return;
}
    
//
// public member functions
//


// constructor
hdfistream_sds::hdfistream_sds(const string filename) : hdfistream_obj(filename) {
    _init();
    if (_filename.length() != 0) // if ctor specified a file to open
	open(_filename.c_str());
    return;
}

// check to see if stream has been positioned past last SDS in file
bool hdfistream_sds::eos(void) const {
    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    if (_nsds == 0)	     // eos() is always true of there are no SDS's in file
	return true;
    else {
	if (bos())  // eos() is never true if at bos() and there are SDS's
	    return false;
	else
	    return (_index >= _nsds); // are we indexed past the last SDS?
    }
}

// check to see if stream is positioned in front of the first SDS in file
bool hdfistream_sds::bos(void) const {
    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    if (_nsds == 0)
        return true;     // if there are no SDS's we still want to read file attrs so both eos() and bos() are true
    if (_index == -1)
	return true;
    else
	return false;
}

// check to see if stream is positioned past the last attribute in the currently
// open SDS
bool hdfistream_sds::eo_attr(void) const {
    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    if (eos() && !bos())	// if eos(), then always eo_attr()
	return true;
    else {
	if (bos())  // are we at BOS and are positioned past last file attributes?
	    return (_attr_index >= _nfattrs);
	else
	    return (_attr_index >= _nattrs); // or positioned after last SDS attr?
    }
}


// check to see if stream is positioned past the last dimension in the currently
// open SDS
bool hdfistream_sds::eo_dim(void) const {
    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    if (eos())			// if eos(), then always eo_dim()
	return true;
    else {
	if (bos())  // if at BOS, then never eo_dim()
	    return true;
	else
	    return (_dim_index >= _rank); // are we positioned after last dim?
    }
}

// open a new file 
void hdfistream_sds::open(const char *filename) {
    if (filename == 0)		// no filename given
	THROW(hcerr_openfile);
    if (_file_id != 0)		// close any currently open file
	close();
    if ( (_file_id = SDstart((char *)filename, DFACC_READ)) < 0)
	THROW(hcerr_openfile);
    _filename = filename;	// assign filename
    _get_fileinfo();		// get file information
    rewind();			// position at BOS to start
    return;
}

// close currently open file (if any)
void hdfistream_sds::close(void) { // close file
    _close_sds();		   // close any currently open SDS
    if (_file_id != 0)		   // if open file, then close it
	(void)SDend(_file_id);
    _file_id = _nsds = _nfattrs = 0; // zero file info
    return;
}


// position SDS array index to index'th SDS array (not necessarily index'th SDS)
void hdfistream_sds::seek(int index) {
    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    _close_sds();		// close any currently open SDS
    _seek_arr(index);		// seek to index'th SDS array
    if (!eos() && !bos())	// if not BOS or EOS, get SDS information
	_get_sdsinfo();
}

// position SDS array index to SDS array with name "name"
void hdfistream_sds::seek(const char *name) {
    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    _close_sds();		// close any currently open SDS
    _seek_arr(string(name));		// seek to index'th SDS array
    if (!eos() && !bos())	// if not BOS or EOS, get SDS information
	_get_sdsinfo();
}

// position SDS array index in front of first SDS array
void hdfistream_sds::rewind(void) { 
    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    _close_sds();		// close any already open SDS
    _rewind();		// seek to BOS
}

// position to next SDS array in file
void hdfistream_sds::seek_next(void) { 
    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    _seek_next_arr();		// seek to next SDS array
    if (!eos())			// if not EOS, get SDS information
	_get_sdsinfo();
}

// position to SDS array by ref
void hdfistream_sds::seek_ref(int ref) {
    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    _close_sds();		// close any currently open SDS
    _seek_arr_ref(ref);		// seek to SDS array by reference
    if (!eos() && !bos())	// if not BOS or EOS, get SDS information
	_get_sdsinfo();
}

// set slab parameters
void hdfistream_sds::setslab(vector<int> start, vector<int> edge, 
			     vector<int> stride, bool reduce_rank) {
    // check validity of input
    if (start.size() != edge.size()  ||  edge.size() != stride.size()  ||
	start.size() == 0)
	THROW(hcerr_invslab);

    int i;
    for (i=0; i<(int)start.size() && i<hdfclass::MAXDIMS; ++i) {
	if (start[i] < 0)
	    THROW(hcerr_invslab);
	if (edge[i] <= 0)
	    THROW(hcerr_invslab);
	if (stride[i] <= 0)
	    THROW(hcerr_invslab);
	_slab.start[i] = start[i];
	_slab.edge[i] = edge[i];
	_slab.stride[i] = stride[i];
    }
    _slab.set = true;
    _slab.reduce_rank = reduce_rank;
}

// This function, when compiled with gcc 2.8 and -O2, causes a virtual
// memeory exceeded error. 2/25/98 jhrg
// load currently open SDS into an hdf_sds object	
hdfistream_sds& hdfistream_sds::operator>>(hdf_sds &hs) {

    // delete any prevous data in hs
    hs.dims = vector<hdf_dim>();
    hs.attrs = vector<hdf_attr>();
    hs.data = hdf_genvec();
    hs.name = string();

    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    if (bos())		// if at BOS, advance to first SDS array
	seek(0);
    if (eos())			// if at EOS, do nothing
	return *this;

    // get basic info about SDS
    char name[hdfclass::MAXSTR];
    int32 rank;
    int32 dim_sizes[hdfclass::MAXDIMS];
    int32 number_type;
    int32 nattrs;
    if (SDgetinfo(_sds_id, name, &rank, dim_sizes, &number_type, &nattrs) < 0)
	THROW(hcerr_sdsinfo);
    
    // assign SDS index
    hs.ref = SDidtoref(_sds_id);
    // load dimensions and attributes into the appropriate objects
    *this >> hs.dims;	
    *this >> hs.attrs;	
    hs.name = name;		// assign SDS name

    void *data = 0;
    int nelts = 1;
    if (_meta) 		// if _meta is set, just load type information
	hs.data.import(number_type);
    else {
	if (_slab.set) {	// load a slab of SDS array data
	    for (int i=0; i<rank; ++i)
 		nelts *= _slab.edge[i];
#if 0
		nelts *= (int)( (_slab.edge[i] - 1) / _slab.stride[i] + 1);
#endif
	    
	    // allocate a temporray C array to hold the data from SDreaddata()
	    int datasize = nelts * DFKNTsize(number_type);
	    data = (void *)new char[datasize];
	    if (data == 0) 
		THROW(hcerr_nomemory);
	    if (SDreaddata(_sds_id, _slab.start, _slab.stride, _slab.edge, 
			   data) < 0) {
		delete []data;	// problem: clean up and throw an exception
		THROW(hcerr_sdsread);
	    }
	}
	else {			// load entire SDS array
	    // prepare for SDreaddata(): make an array of zeroes and calculate
	    // number of elements of array data
	    int32 zero[hdfclass::MAXDIMS];
	    for (int i=0; i<rank; ++i) {
		zero[i] = 0;
		nelts *= dim_sizes[i];
	    }
	    
	    // allocate a temporray C array to hold the data from SDreaddata()
	    int datasize = nelts * DFKNTsize(number_type);
	    data = (void *)new char[datasize];
	    if (data == 0) 
		THROW(hcerr_nomemory);
	    
	    // read the data and store it in an hdf_genvec
	    if (SDreaddata(_sds_id, zero, 0, dim_sizes, data) < 0) {
		delete []data;	// problem: clean up and throw an exception
		THROW(hcerr_sdsread);
	    }
	}    
	// try {  // try to import into an hdf_genvec
	hs.data.import(number_type, data, nelts);
	// }
	// catch(...)  // problem: clean up and rethrow
	//     delete []data;
	//     throw;
	// }
	delete []data; // deallocate temporary C array
    }

    seek_next();		// position to next SDS array
    return *this;
}

// load dimension currently positioned at 
hdfistream_sds& hdfistream_sds::operator>>(hdf_dim &hd) {

    // delete any previous data in hd
    hd.name = hd.label = hd.unit = hd.format = string();
    hd.count = 0;
    hd.scale = hdf_genvec();
    hd.attrs = vector<hdf_attr>();

    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    if (bos())		// if at BOS, advance to first SDS array
	seek(0);

    // if reduce_rank is true, hyperslab dimensions of length 1 will be 
    // eliminated from the dimension object, thus reducing the rank of the 
    // hyperslab
    while (_slab.set && _slab.reduce_rank && !eo_dim() &&
	   _slab.edge[_dim_index] == 1) 
	++_dim_index;

    // if positioned past last dimension, do nothing
    if (eo_dim())
	return *this;

    // open dimension currently positioned at and increment dim index
    int32 dim_id;
    if ( (dim_id = SDgetdimid(_sds_id, _dim_index)) < 0)
	THROW(hcerr_sdsinfo);

    // get dimension information
	
    char name[hdfclass::MAXSTR];
    int32 count, number_type, nattrs;
    if (SDdiminfo(dim_id, name, &count, &number_type, &nattrs) < 0)
	THROW(hcerr_sdsinfo);
    else
	hd.name = name;		// assign dim name
    char label[hdfclass::MAXSTR];
    char unit[hdfclass::MAXSTR];
    char cformat[hdfclass::MAXSTR];
    if (SDgetdimstrs(dim_id, label, unit, cformat, hdfclass::MAXSTR) == 0) {
	hd.label = label;	// assign dim label
	hd.unit = unit;		// assign dim unit
	hd.format = cformat;	// assign dim format
    }

    // if we are dealing with a dimension of size unlimited, then call
    // SDgetinfo to get the current size of the dimension
    if (count == 0) {		// unlimited dimension
	if (_dim_index != 0)
	    THROW(hcerr_sdsinfo); // only first dim can be unlimited
	char junk[hdfclass::MAXSTR];
	int32 junk2, junk3, junk4;
	int32 dim_sizes[hdfclass::MAXDIMS];
	if (SDgetinfo(_sds_id, junk, &junk2, dim_sizes, &junk3, &junk4) < 0)
	    THROW(hcerr_sdsinfo);
	count = dim_sizes[0];
    }

    // load user-defined attributes for the dimension
    // TBD!  Not supported at present
    
    // load dimension scale if there is one
    if (number_type != 0) { // found a dimension scale

	// allocate a temporary C array to hold data from SDgetdimscale()
	void *data = (void *) new char[count*DFKNTsize(number_type)];
	if (data == 0)
	    THROW(hcerr_nomemory);

	// read the scale data and store it in an hdf_genvec
	if (SDgetdimscale(dim_id, data) < 0) { 
	    delete []data; // problem: clean up and throw an exception
	    THROW(hcerr_sdsinfo);
	}
	// try { // try to allocate an hdf_genvec
	if (_slab.set) {
	    void *datastart = (char *)data + 
		_slab.start[_dim_index] * DFKNTsize(number_type);
	    hd.scale = hdf_genvec(number_type, datastart, 0, 
				  _slab.edge[_dim_index]*_slab.stride[_dim_index]-1,
#if 0
				  _slab.edge[_dim_index]-1, 
#endif
				  _slab.stride[_dim_index]);
	}
	else
	    hd.scale = hdf_genvec(number_type, data, count);
	// }
	// catch (...) { // problem allocating hdf_genvec: clean up and rethrow
	//    delete []data;
	//    throw;
	// }
	delete []data; // deallocate temporary C array
    }

    // assign dim size; if slabbing is set, assigned calculated size, otherwise
    // assign size from SDdiminfo()
    if (_slab.set)
 	hd.count = _slab.edge[_dim_index];
#if 0
	hd.count = ( _slab.edge[_dim_index] - 1) /_slab.stride[_dim_index] + 1;
#endif
    else
	hd.count = count;
    _dim_index++;

    return *this;
}
    

// load attribute currently positioned at 
hdfistream_sds& hdfistream_sds::operator>>(hdf_attr& ha) {

    // delete any previous data in ha
    ha.name = string();
    ha.values = hdf_genvec();

    if (_filename.length() == 0) // no file open
	THROW(hcerr_invstream);
    if (eo_attr())		// if positioned past last attribute, do nothing
	return *this;

    // prepare to read attribute information: set nattrs, id depending on whether
    // reading file attributes or SDS attributes
    int32 id;
    int nattrs;
    if (bos()) {		// if at BOS, then read file attributes
	nattrs = _nfattrs;
	id = _file_id;
    }
    else {			// else read SDS attributes
	nattrs = _nattrs;
	id = _sds_id;
    }
    char name[hdfclass::MAXSTR];
    int32 number_type, count;
    if (SDattrinfo(id, _attr_index, name, &number_type, &count) < 0)
	THROW(hcerr_sdsinfo);
    
    // allowcate a temporary C array to hold data from SDreadattr()
    void *data;
    data = (void *)new char[count*DFKNTsize(number_type)];
    if (data == 0)
	THROW(hcerr_nomemory);

    // read attribute values and store them in an hdf_genvec
    if (SDreadattr(id, _attr_index, data) < 0) {
	delete []data; // problem: clean up and throw an exception
	THROW(hcerr_sdsinfo);
    }

    // eliminate trailing null characters from the data string; 
    // they cause GNU's String class problems
    // NOTE: removed because count=0 if initial char is '\0' and we're not
    //   using GNU String anymore
#if 0
    if (number_type == DFNT_CHAR)
	count = (int32)min((int)count,(int)strlen((char *)data));
#endif

    // try { // try to allocate an hdf_genvec
    if (count > 0) {
	ha.values = hdf_genvec(number_type, data, count);
	// }
	// catch(...) { // problem allocating hdf_genvec: clean up and rethrow
	//    delete []data;
	//    throw;
	// }
    }
    delete []data; // deallocate temporary C array

    // increment attribute index to next attribute
    ++_attr_index;
    ha.name = name;		// assign attribute name
    return *this;
}

// This function, when compiled with gcc 2.8 and -O2, causes a virtual
// memeory exceeded error. 2/25/98 jhrg
// read in all the SDS arrays in a file
hdfistream_sds& hdfistream_sds::operator>>(vector<hdf_sds>& hsv) {
//    hsv = vector<hdf_sds>0;	// reset vector
    for (hdf_sds sds;!eos();) {
	*this>>sds;
	hsv.push_back(sds);
    }
    return *this;
}

// read in all of the SDS attributes for currently open SDS
hdfistream_sds& hdfistream_sds::operator>>(vector<hdf_attr>& hav) {
//    hav = vector<hdf_attr>0;	// reset vector
    for (hdf_attr att;!eo_attr();) {
	*this>>att;
	hav.push_back(att);
    }
    return *this;
}

// read in all of the SDS dimensions for currently open SDS
hdfistream_sds& hdfistream_sds::operator>>(vector<hdf_dim>& hdv) {
//    hdv = vector<hdf_dim>0;	// reset vector
    for (hdf_dim dim;!eo_dim();) {
	*this>>dim;
	hdv.push_back(dim);
    }
    return *this;
}

// Check to see if this hdf_sds has a dim scale: if all of the dimensions have
// scale sizes = to their size, it does
bool hdf_sds::has_scale(void) const {
    bool has_scale;
    if (!_ok(&has_scale)) {
	THROW(hcerr_sdsscale);
	return false;
    }
    else
	return has_scale;
}

// Verify that the hdf_sds is in an OK state (i.e., that it is initialized
// correctly) if user has passed a ptr to a bool then set that to whether
// there is a dimension scale for at least one of the dimensions
//
// Added `if (*has_scale)...' because `has_scale' defaults to null and
// dereferencing it causes segmentation faults, etc.

bool hdf_sds::_ok(bool *has_scale) const {

    if (has_scale)
	*has_scale = false;

    // Check to see that for each SDS dimension scale, that the length of the
    // scale matches the size of the dimension.
    for (int i=0; i<(int)dims.size(); ++i)
	if (dims[i].scale.size() != 0) {
	    if (has_scale)
		*has_scale = true;
	    if (dims[i].scale.size() != dims[i].count)
		return false;
	}

    return true;
}

// $Log: sds.cc,v $
// Revision 1.12  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.11  1999/05/06 03:23:33  jimg
// Merged changes from no-gnu branch
//
// Revision 1.10  1999/05/05 23:33:43  jimg
// String --> string conversion
//
// Revision 1.9.6.1  1999/05/06 00:35:45  jimg
// Jakes String --> string changes
//
// Revision 1.9  1998/09/10 23:11:25  jehamby
// Fix for SDS not outputting global attributes if no SDS's are in the dataset
//
// Revision 1.8  1998/09/10 21:33:24  jehamby
// Map DFNT_CHAR8 and DFNT_UCHAR8 to Byte instead of string in SDS.
//
// Revision 1.7  1998/04/03 18:34:19  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.6  1998/03/26 00:29:34  jimg
// Fixed bug in _ok() where has_scale was incorrectly dereferenced. The pointer
// was supposed to be tested and then dereferenced, but instead was dereferenced
// first. Pointed out by Jake Hamby.
//
// Revision 1.5  1998/03/17 18:01:02  jimg
// Added comments about virtual memory error when compiling
//
// Revision 1.4  1997/12/16 01:46:46  jimg
// Merged release 2.14d changes
//
// Revision 1.3  1997/10/04 00:33:10  jimg
// Release 2.14c fixes
//
// Revision 1.2.6.1  1997/09/05 21:28:05  jimg
// Fixed _ok() so that if `has_scale' is null (the default) it is not
// dereferenced.
//
// Applied Todd's patch for the `stride bug' (See the TODO list).
//
// Revision 1.1  1996/10/31 18:43:06  jimg
// Added.
//
// Revision 1.15  1996/10/14 19:06:14  todd
// Minor fix to support compilation on Sun platform.
//
// Revision 1.14  1996/09/23 23:46:18  todd
// Fixed hdfistream_sds::operator>>(hdf_dim&) to correctly read size of
// unlimited dimensions.
//
// Revision 1.13  1996/09/23  22:50:51  todd
// Fixed bug in checking of dimension scales.
//
// Revision 1.12  1996/09/17  23:21:56  todd
// Added support for reading subsets of SDS's to hdfistream_sds.
//
// Revision 1.11  1996/07/19  23:21:59  todd
// Fixed bug in dimension and attribute indexing.
//
// Revision 1.10  1996/06/14  23:12:02  todd
// Fixed minor bug in operator>>().
//
// Revision 1.9  1996/05/23  18:17:55  todd
// Added copyright statement.
//
// Revision 1.8  1996/05/08  15:48:38  todd
// Moved hdf_sds::has_scale() into this module and modified it to use the new _ok mfunc.
// Added new _ok protected mfunc which verifies that an hdf_sds is in an OK state.
//
// Revision 1.7  1996/05/06  17:32:34  todd
// Fixed a bug in the patch applied to operator>>(hdf_attr&).
//
// Revision 1.6  1996/05/03  22:50:03  todd
// Added a patch to the operator>>(hdf_attr&) mfunc to strip away any null characters
// that are padded onto the end of an attribute.  For some strange reason,
// AVHRR Pathfinder SST files seem to have a null character padded at the end of the
// values of text attributes.  This caused problems when the text was loaded into
// libg++ strings.
//
// Revision 1.5  1996/04/19  17:36:19  todd
// Added public mfunc seek(const char *) and protected mfunc _seek_arr(const string&)
// which is used in implementing t.
//
// Revision 1.4  1996/04/19  01:14:28  todd
// Changed eof, bof to eos, bos (end of stream).  That is better terminology, since
// a stream of SDS's does not correspond to the entire HDF file.
//
// Revision 1.3  1996/04/09  23:28:44  todd
// Modified so open() is called by constructor and not by _init().  This makes the
// semantics of _init() more logical and consistent with hdfistream_annot.
//
// Revision 1.2  1996/04/04  01:41:30  todd
// Fixed operator>>(hdf_dim&) so it would tolerate a failure of SDgetdimstrs().
// Fixed operator>>(hdf_sds&) to allow storage of type information in zero-length
// hdf_genvec if _meta is set.
//
// Revision 1.1  1996/04/02  21:47:22  todd
// Initial revision
//
