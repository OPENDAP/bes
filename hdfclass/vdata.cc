//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1996, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//		       NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: vdata.cc,v $ - classes for HDF VDATA
//
// $Log: vdata.cc,v $
// Revision 1.1  1996/10/31 18:43:07  jimg
// Added.
//
// Revision 1.5  1996/08/22  20:56:03  todd
// Corrected bug in LoadField call.
//
// Revision 1.4  1996/08/14  22:34:43  ike
// Added hdfistream_vdata::setrecs().
//
// Revision 1.3  1996/07/22  17:28:43  todd
// Changed implementation of IsInternalVdata() to use the STL set class.  This allows the
// routine to work in both g++ and SGI C++.
//
// Revision 1.2  1996/07/11  20:36:26  todd
// Added check to see if Vdata's are "internal" ones used by HDF library.  Modified
// stream class to omit such vdatas from the stream.
//
// Revision 1.1  1996/06/17  23:28:11  todd
// Initial revision
//
//////////////////////////////////////////////////////////////////////////////

#include <mfhdf.h>
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif
#include <vector.h>
#include <set.h>
#include <algo.h>
#include <hcstream.h>
#include <hdfclass.h>

static void LoadField(int32 vid, int index, int32 begin, int32 end, hdf_field& f);
static bool IsInternalVdata(int32 fid, int32 ref);

//
// hdfistream_vdata -- protected member functions
//

// initialize hdfistream_vdata
void hdfistream_vdata::_init(void) {
    _vdata_id = _index = 0;
    _meta = false;
    _vdata_refs = vector<int32>();
    _recs.set = false;
    return;
}

void hdfistream_vdata::_get_fileinfo(void) {

    // build list ref numbers of all Vdata's in the file
    int32 ref = -1;
    while ( (ref = VSgetid(_file_id, ref)) != -1) {
	if (!IsInternalVdata(_file_id, ref))
	    _vdata_refs.push_back(ref);
    }

    return;
}

void hdfistream_vdata::_seek_next(void) {
    _index++;
    if (!eos())
	_seek(_vdata_refs[_index]);
    return;
}

void hdfistream_vdata::_seek(const char *name) {
    int32 ref = VSfind(_file_id, name);
    if (ref < 0)
	THROW(hcerr_vdatafind);
    else
	_seek(ref);
	
    return;
}

void hdfistream_vdata::_seek(int32 ref) {
    if (_vdata_id != 0)
      VSdetach(_vdata_id);
    vector<int32>::iterator r = find(_vdata_refs.begin(), _vdata_refs.end(), ref);
    if (r == _vdata_refs.end())
      THROW(hcerr_vdatafind);
    _index = r - _vdata_refs.begin();
    if ( (_vdata_id = VSattach(_file_id, ref, "r")) < 0) {
      _vdata_id = 0;
      THROW(hcerr_vdataopen);
    }
    return;
}


//
// hdfistream_vdata -- public member functions
//

hdfistream_vdata::hdfistream_vdata(const char *filename) : hdfistream_obj(filename) {
    _init();
    if (_filename.length() != 0) // if ctor specified a null filename
#ifdef __GNUG__  // GNU string
	open(_filename.chars());
#else // STL string
	open(_filename.data());
#endif
    return;
}

void hdfistream_vdata::open(const String& filename) {
#ifdef __GNUG__
    open(filename.chars());
#else
    open(filename.data());
#endif
    return;
}

void hdfistream_vdata::open(const char *filename) {
    if (_file_id != 0)
	close();
    if ( (_file_id = Hopen(filename, DFACC_RDONLY, 0)) < 0)
	THROW(hcerr_openfile);
    if (Vstart(_file_id) < 0)
	THROW(hcerr_openfile);
    _filename = filename;
    _get_fileinfo();
    rewind();
    return;
}

void hdfistream_vdata::close(void) {
    if (_vdata_id != 0)
	VSdetach(_vdata_id);
    if (_file_id != 0) {
	Vend(_file_id);
	Hclose(_file_id);
    }
    _vdata_id = _file_id = _index = 0;
    _vdata_refs = vector<int32>(); // clear refs
    _recs.set = false;
    return;
}

void hdfistream_vdata::seek(int index) {
    if (index < 0  ||  index >= (int)_vdata_refs.size())
	THROW(hcerr_range);
    _seek(_vdata_refs[index]);
    _index = index;
    return;
}

void hdfistream_vdata::seek(const String& name) {
#ifdef __GNUG__
    seek(name.chars());
#else
    seek(name.data());
#endif
}

void hdfistream_vdata::seek(const char *name) {
    _seek(name);
    return;
}

bool hdfistream_vdata::setrecs(int32 begin, int32 end) {
  if (_vdata_id != 0) {
    int32 il;
    VSQueryinterlace(_vdata_id,&il);
    if (il != FULL_INTERLACE)
      return false;
    else {
      int32 cnt;
      VSQuerycount(_vdata_id,&cnt);
      if (begin < 0 || end >= cnt)
	return false;
      else {
	_recs.begin = begin;
	_recs.end = end;
	_recs.set = true;
      }
    }
  }
  return true;
}


// read all Vdata's in the stream
hdfistream_vdata& hdfistream_vdata::operator>>(vector<hdf_vdata>& hvv) {
    for (hdf_vdata hv;!eos();) {
	*this>>hv;
	hvv.push_back(hv);
    }
    return *this;
}

// read a Vdata from the stream
hdfistream_vdata& hdfistream_vdata::operator>>(hdf_vdata& hv) {

    // delete any previous data in hv
    hv.fields = vector<hdf_field>();
    hv.vclass = hv.name = String();

    if (_vdata_id == 0)
	THROW(hcerr_invstream);	// no vdata open!
    if (eos())
	return *this;

    // retrieve Vdata name, class, number of records
    char name[hdfclass::MAXSTR];
    char vclass[hdfclass::MAXSTR];
    int32 nrecs;
    if (VSinquire(_vdata_id, &nrecs, (int32 *)0, (char *)0, (int32 *)0, name) 
	< 0)
	THROW(hcerr_vdatainfo);
    hv.name = String(name);
    if (VSgetclass(_vdata_id, vclass) < 0)
	THROW(hcerr_vdatainfo);
    hv.vclass = String(vclass);

    // retrieve number of fields
    int nfields = VFnfields(_vdata_id);
    if (nfields < 0)
        THROW(hcerr_vdatainfo);
    
    // retrieve field information
    hv.fields = vector<hdf_field>();
    for (int i=0; i<nfields; ++i) {
        hv.fields.push_back(hdf_field());
	if (_meta)
	    LoadField(_vdata_id, i, 0, 0, hv.fields[i]);
	else if (_recs.set)
	    LoadField(_vdata_id, i, _recs.begin, _recs.end, hv.fields[i]);
	else
	    LoadField(_vdata_id, i, 0, nrecs-1, hv.fields[i]);
    }
    _seek_next();
    return *this;
}

static void LoadField(int32 vid, int index, int32 begin, int32 end, hdf_field& f) {

    // position to first record too read
    if (VSseek(vid, begin) < 0)
  	THROW(hcerr_vdataseek);
    int32 nrecs = end - begin + 1;

    // retrieve name of field
    char *fieldname = VFfieldname(vid, index);
    if (fieldname == 0)
	THROW(hcerr_vdatainfo);
    f.name = String(fieldname);

    // retrieve order of field
    int32 fieldorder = VFfieldorder(vid, index);
    if (fieldorder < 0)
	THROW(hcerr_vdatainfo);

    // retrieve size of the field in memory
    int32 fieldsize = VFfieldisize(vid, index);
    if (fieldsize < 0)
	THROW(hcerr_vdatainfo);
    
    // retrieve HDF data type of field
    int32 fieldtype = VFfieldtype(vid, index);
    if (fieldtype < 0)
	THROW(hcerr_vdatainfo);

    // for each component, set type and optionally load data
    hdf_genvec gv;
    void *data = 0;
    if (nrecs > 0) {		// if nrecs > 0 then load data for field
	data = (void *)new char[fieldsize*nrecs];
	if (VSsetfields(vid, fieldname) < 0) // set field to read
	    THROW(hcerr_vdataread);
	if (VSread(vid, (uchar8 *)data, nrecs, FULL_INTERLACE) < 0) {
	    delete []data;
	    THROW(hcerr_vdataread);
	}
    }
    int stride = fieldorder;
    for (int i=0; i<fieldorder; ++i) {
	// try {  // try to create an hdf_genvec and add it to the vals vector
	if (nrecs == 0)
	    gv = hdf_genvec(fieldtype, 0, 0, 0, 0);
	else
	    gv = hdf_genvec(fieldtype, data, i, (nrecs*fieldorder)-1, stride);
	f.vals.push_back(gv);
	// }
	// catch(...) {  // problem: clean up and rethrow
	//     delete []data;
	//     throw;
	// }
    }
    delete []data;
}



//
// hdf_vdata related member functions
//

// verify that the hdf_field class is in an OK state.  
bool hdf_field::_ok(void) const {
    
    // make sure that there is some data stored in the hdf_field
    if (vals.size() == 0)
	return false;

    // if the field has order > 1, check to make sure that the number types of all
    // the columns in the field are the same and are not 0
    if (vals.size() > 1) {
	int32 nt = vals[0].number_type();
	if (nt == 0)
	    return false;
	for (int i=1; i<(int)vals.size(); ++i)
	    if (vals[i].number_type() != nt  || vals[i].number_type() == 0)
		return false;
    }

    return true;		// passed all the tests
}

bool hdf_vdata::_ok(void) const {
    
    // make sure there are fields stored in this vdata
    if (fields.size() == 0)
	return false;

    // make sure the fields are themselves OK
    for (int i=0; i<(int)fields.size(); ++i)
	if (!fields[i])
	    return false;

    return true;		// passed all the tests
}

bool IsInternalVdata(int32 fid, int32 ref) { 
    set<String, less<String> > reserved_names;
    reserved_names.insert("RIATTR0.0N");

    set<String, less<String> > reserved_classes;
    reserved_classes.insert("Attr0.0");
    reserved_classes.insert("RIATTR0.0C");
    reserved_classes.insert("DimVal0.0");
    reserved_classes.insert("DimVal0.1");

    // get name, class of vdata
    int vid;
    if ( (vid = VSattach(fid, ref, "r")) < 0) {
	vid = 0;
	THROW(hcerr_vdataopen);
    }
    char name[hdfclass::MAXSTR];
    char vclass[hdfclass::MAXSTR];
    if (VSgetname(vid, name) < 0)
	THROW(hcerr_vdatainfo);
    if (reserved_names.find(String(name)) != reserved_names.end())
	return true;

    if (VSgetclass(vid, vclass) < 0)
	THROW(hcerr_vdatainfo);
    if (reserved_classes.find(String(vclass)) != reserved_classes.end())
	return true;
    return false;
}
 
