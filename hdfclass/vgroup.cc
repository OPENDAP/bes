//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1998, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//		       NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//         Jake.Hamby@jpl.nasa.gov
//
// $RCSfile: vgroup.cc,v $ - classes for HDF VGROUP
//
// $Log: vgroup.cc,v $
// Revision 1.1  1998/04/06 16:40:36  jimg
// Added by Jake Hamby (via patch)
//
// Revision 1.1  1998/03/07 01:49:17  jehamby
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

static bool IsInternalVgroup(int32 fid, int32 ref);

//
// hdfistream_vgroup -- protected member functions
//

// initialize hdfistream_vgroup
void hdfistream_vgroup::_init(void) {
    _vgroup_id = _index = 0;
    _meta = false;
    _vgroup_refs.clear();
    _recs.set = false;
    return;
}

void hdfistream_vgroup::_get_fileinfo(void) {

    // build list ref numbers of all Vgroup's in the file
    int32 ref = -1;
    while ( (ref = Vgetid(_file_id, ref)) != -1) {
	if (!IsInternalVgroup(_file_id, ref))
	    _vgroup_refs.push_back(ref);
    }

    return;
}

void hdfistream_vgroup::_seek_next(void) {
    _index++;
    if (!eos())
	_seek(_vgroup_refs[_index]);
    return;
}

void hdfistream_vgroup::_seek(const char *name) {
    int32 ref = Vfind(_file_id, name);
    if (ref < 0)
	THROW(hcerr_vgroupfind);
    else
	_seek(ref);
	
    return;
}

void hdfistream_vgroup::_seek(int32 ref) {
    if (_vgroup_id != 0)
      Vdetach(_vgroup_id);
    vector<int32>::iterator r = find(_vgroup_refs.begin(), _vgroup_refs.end(), ref);
    if (r == _vgroup_refs.end())
      THROW(hcerr_vgroupfind);
    _index = r - _vgroup_refs.begin();
    if ( (_vgroup_id = Vattach(_file_id, ref, "r")) < 0) {
      _vgroup_id = 0;
      THROW(hcerr_vgroupopen);
    }
    return;
}


//
// hdfistream_vgroup -- public member functions
//

hdfistream_vgroup::hdfistream_vgroup(const char *filename) : hdfistream_obj(filename) {
    _init();
    if (_filename.length() != 0) // if ctor specified a null filename
#ifdef __GNUG__  // GNU string
	open(_filename.chars());
#else // STL string
	open(_filename.data());
#endif
    return;
}

void hdfistream_vgroup::open(const String& filename) {
#ifdef __GNUG__
    open(filename.chars());
#else
    open(filename.data());
#endif
    return;
}

void hdfistream_vgroup::open(const char *filename) {
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

void hdfistream_vgroup::close(void) {
    if (_vgroup_id != 0)
	Vdetach(_vgroup_id);
    if (_file_id != 0) {
	Vend(_file_id);
	Hclose(_file_id);
    }
    _vgroup_id = _file_id = _index = 0;
    _vgroup_refs = vector<int32>(); // clear refs
    _recs.set = false;
    return;
}

void hdfistream_vgroup::seek(int index) {
    if (index < 0  ||  index >= (int)_vgroup_refs.size())
	THROW(hcerr_range);
    _seek(_vgroup_refs[index]);
    _index = index;
    return;
}

void hdfistream_vgroup::seek_ref(int ref) {
    _seek(ref);  // _seek() sets _index
    return;
}

void hdfistream_vgroup::seek(const String& name) {
#ifdef __GNUG__
    seek(name.chars());
#else
    seek(name.data());
#endif
}

void hdfistream_vgroup::seek(const char *name) {
    _seek(name);
    return;
}


// read all Vgroup's in the stream
hdfistream_vgroup& hdfistream_vgroup::operator>>(vector<hdf_vgroup>& hvv) {
    for (hdf_vgroup hv;!eos();) {
	*this>>hv;
	hvv.push_back(hv);
    }
    return *this;
}

// read a Vgroup from the stream
hdfistream_vgroup& hdfistream_vgroup::operator>>(hdf_vgroup& hv) {

    // delete any previous data in hv
    hv.tags.clear();
    hv.refs.clear();
    hv.vclass = hv.name = String();

    if (_vgroup_id == 0)
	THROW(hcerr_invstream);	// no vgroup open!
    if (eos())
	return *this;

    // assign Vgroup ref
    hv.ref = _vgroup_refs[_index];
    // retrieve Vgroup name, class, number of entries
    char name[hdfclass::MAXSTR];
    char vclass[hdfclass::MAXSTR];
    int32 nentries;
    if (Vinquire(_vgroup_id, &nentries, name) 
	< 0)
	THROW(hcerr_vgroupinfo);
    hv.name = String(name);
    if (Vgetclass(_vgroup_id, vclass) < 0)
	THROW(hcerr_vgroupinfo);
    hv.vclass = String(vclass);

    // retrieve entry tags and refs
    int32 npairs = Vntagrefs(_vgroup_id);
    for (int i=0; i<npairs; ++i) {
      int32 tag, ref;
      if (Vgettagref(_vgroup_id, i, &tag, &ref) < 0)
	THROW(hcerr_vgroupread);
      hv.tags.push_back(tag);
      hv.refs.push_back(ref);
    }
    _seek_next();
    return *this;
}

//
// hdf_vgroup related member functions
//

bool hdf_vgroup::_ok(void) const {
    
    // make sure there are tags stored in this vgroup
    if (tags.size() == 0)
	return false;

    // make sure there are refs stored in this vgroup
    if (refs.size() == 0)
        return false;

    return true;		// passed all the tests
}

bool IsInternalVgroup(int32 fid, int32 ref) { 
    // block vgroups used internally
    set<String, less<String> > reserved_names;
    reserved_names.insert("RIATTR0.0N");
    reserved_names.insert("RIG0.0");

    set<String, less<String> > reserved_classes;
    reserved_classes.insert("Attr0.0");
    reserved_classes.insert("RIATTR0.0C");
    reserved_classes.insert("DimVal0.0");
    reserved_classes.insert("DimVal0.1");
    reserved_classes.insert("CDF0.0");
    reserved_classes.insert("Var0.0");
    reserved_classes.insert("Dim0.0");
    reserved_classes.insert("UDim0.0");
    reserved_classes.insert("Data0.0");
    reserved_classes.insert("RI0.0");

    // get name, class of vdata
    int vid;
    if ( (vid = Vattach(fid, ref, "r")) < 0) {
        vid = 0;
        THROW(hcerr_vdataopen);
    }
    char name[hdfclass::MAXSTR];
    char vclass[hdfclass::MAXSTR];
    if (Vgetname(vid, name) < 0)
        THROW(hcerr_vdatainfo);
    if (reserved_names.find(String(name)) != reserved_names.end())
        return true;

    if (Vgetclass(vid, vclass) < 0)
        THROW(hcerr_vdatainfo);
    if (reserved_classes.find(String(vclass)) != reserved_classes.end())
        return true;
    return false;

}
 
