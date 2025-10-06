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

//////////////////////////////////////////////////////////////////////////////
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
//
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: vdata.cc,v $ - classes for HDF VDATA
//
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <mfhdf.h>

#ifdef __POWERPC__
#undef isascii
#endif

#include <string>
#include <vector>
#include <set>
#include <algorithm>

// libdap
#include <libdap/util.h>
#include <libdap/debug.h>

// bes
#include <BESDebug.h>
#include <BESInternalError.h>
#include <hcstream.h>
#include <hdfclass.h>

using std::set;
using std::less;
using namespace libdap;


static void LoadField(int32 vid, int index, int32 begin, int32 end,
		hdf_field & f);
static bool IsInternalVdata(int32 fid, int32 ref);

//
// hdfistream_vdata -- protected member functions
//

// initialize hdfistream_vdata
void hdfistream_vdata::_init(void) {
	_vdata_id = _index = _attr_index = _nattrs = 0;
	_meta = false;
	_vdata_refs.clear();
	_recs.set = false;
	return;
}

void hdfistream_vdata::_get_fileinfo(void) {
	// build list ref numbers of all Vdata's in the file
	int32 ref = -1;
	while ((ref = VSgetid(_file_id, ref)) != -1) {
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
	auto r = find(_vdata_refs.begin(), _vdata_refs.end(),ref);
	if (r == _vdata_refs.end())
		THROW(hcerr_vdatafind);
	_index = r - _vdata_refs.begin();
	if ((_vdata_id = VSattach(_file_id, ref, "r")) < 0) {
		_vdata_id = 0;
		THROW(hcerr_vdataopen);
	}
	_attr_index = 0;
	_nattrs = VSfnattrs(_vdata_id, _HDF_VDATA);
	return;
}

//
// hdfistream_vdata -- public member functions
//

hdfistream_vdata::hdfistream_vdata(const string& filename) :
	hdfistream_obj(filename) {
	_init();
	if (_filename.size() != 0) // if ctor specified a null filename
		open(_filename.c_str());
	return;
}

void hdfistream_vdata::open(const string & filename) {
	open(filename.c_str());
	return;
}

void hdfistream_vdata::open(const char *filename) {
	if (_file_id != 0)
		close();
	if ((_file_id = Hopen(filename, DFACC_RDONLY, 0)) < 0)
		THROW(hcerr_openfile);
	if (Vstart(_file_id) < 0) // Vstart is a macro for Vinitialize
		THROW(hcerr_openfile);

	BESDEBUG("h4", "vdata file opened: id=" << _file_id << endl);

	_filename = filename;
	_get_fileinfo();
	rewind();
	return;
}

void hdfistream_vdata::close(void) {
	BESDEBUG("h4",
			"vdata file closed: id=" << _file_id << ", this: " << this << endl);

	if (_vdata_id != 0)
		VSdetach(_vdata_id);
	if (_file_id != 0) {
		int status = Vend(_file_id); // Vend is a macro for Vfinish
		BESDEBUG("h4",
				"vdata Vend status: " << status << ", this: " << this << endl);

		status = Hclose(_file_id);
		BESDEBUG("h4",
				"vdata HClose status: " << status << ", this: " << this << endl);
	}
	_vdata_id = _file_id = _index = _attr_index = _nattrs = 0;
	_vdata_refs.clear(); // clear refs
	_recs.set = false;
	return;
}

void hdfistream_vdata::seek(int index) {
	if (index < 0 || index >= (int) _vdata_refs.size())
		THROW(hcerr_range);
	_seek(_vdata_refs[index]);
	_index = index;
	return;
}

void hdfistream_vdata::seek_ref(int ref) {
	_seek(ref); // _seek() sets _index
	return;
}

void hdfistream_vdata::seek(const string & name) {
	seek(name.c_str());
}

void hdfistream_vdata::seek(const char *name) {
	_seek(name);
	return;
}

bool hdfistream_vdata::setrecs(int32 begin, int32 end) {
	if (_vdata_id != 0) {
		int32 il;
		VSQueryinterlace(_vdata_id, &il);
		if (il != FULL_INTERLACE)
			return false;
		else {
			int32 cnt;
			VSQuerycount(_vdata_id, &cnt);
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

// check to see if stream is positioned past the last attribute in the
// currently open Vdata
bool hdfistream_vdata::eo_attr(void) const {
	if (_filename.size() == 0) // no file open
		THROW(hcerr_invstream);
	if (eos() && !bos()) // if eos(), then always eo_attr()
		return true;
	else {
		return (_attr_index >= _nattrs); // or positioned after last Vdata attr?
	}
}

// Read all attributes in the stream
hdfistream_vdata & hdfistream_vdata::operator>>(vector<hdf_attr> &hav) {
	//    hav = vector<hdf_attr>0;  // reset vector
	for (hdf_attr att; !eo_attr();) {
		*this >> att;
		hav.push_back(att);
	}
	return *this;
}

// read all Vdata's in the stream
hdfistream_vdata & hdfistream_vdata::operator>>(vector<hdf_vdata> &hvv) {
	for (hdf_vdata hv; !eos();) {
		*this >> hv;
		hvv.push_back(hv);
	}
	return *this;
}

// read an attribute from the stream
hdfistream_vdata & hdfistream_vdata::operator>>(hdf_attr & ha) {
	// delete any previous data in ha
	ha.name = string();
	ha.values = hdf_genvec();

	if (_filename.size() == 0) // no file open
		THROW(hcerr_invstream);
	if (eo_attr()) // if positioned past last attr, do nothing
		return *this;

	char name[hdfclass::MAXSTR];
	int32 number_type;
        int32 count;
        int32 size;

	if (VSattrinfo(_vdata_id, _HDF_VDATA, _attr_index, name, &number_type,
			&count, &size) < 0)
		THROW(hcerr_vdatainfo);

	// allocate a temporary C array to hold data from VSgetattr()
	auto data = new char[count * DFKNTsize(number_type)];
	if (data == nullptr)
		THROW(hcerr_nomemory);

	// read attribute values and store them in an hdf_genvec
	if (VSgetattr(_vdata_id, _HDF_VDATA, _attr_index, data) < 0) {
		delete[] data; // problem: clean up and throw an exception
		THROW(hcerr_vdatainfo);
	}
	if (count > 0) {
            ha.values = hdf_genvec(number_type, data, count);
	}
	delete[] data; // deallocate temporary C array

	// increment attribute index to next attribute
	++_attr_index;
	ha.name = name; // assign attribute name
	return *this;
}

// read a Vdata from the stream
hdfistream_vdata & hdfistream_vdata::operator>>(hdf_vdata & hv) {

	// delete any previous data in hv
	hv.fields.clear();
	hv.vclass = hv.name = string();

	if (_vdata_id == 0)
		THROW(hcerr_invstream); // no vdata open!
	if (eos())
		return *this;

	// assign Vdata ref
	hv.ref = _vdata_refs[_index];
	// retrieve Vdata attributes
	*this >> hv.attrs;
	// retrieve Vdata name, class, number of records
	char name[hdfclass::MAXSTR];
	char vclass[hdfclass::MAXSTR];
	int32 nrecs;
	if (VSinquire(_vdata_id, &nrecs, nullptr, nullptr, nullptr, name)
			< 0)
		THROW(hcerr_vdatainfo);
	hv.name = string(name);
	if (VSgetclass(_vdata_id, vclass) < 0)
		THROW(hcerr_vdatainfo);
	hv.vclass = string(vclass);

	// retrieve number of fields
	int nfields = VFnfields(_vdata_id);
	if (nfields < 0)
		THROW(hcerr_vdatainfo);

	// retrieve field information
	hv.fields = vector<hdf_field> ();
	for (int i = 0; i < nfields; ++i) {
		hv.fields.push_back(hdf_field());
		if (_meta)
			LoadField(_vdata_id, i, 0, 0, hv.fields[i]);
		else if (_recs.set)
			LoadField(_vdata_id, i, _recs.begin, _recs.end, hv.fields[i]);
		else
			LoadField(_vdata_id, i, 0, nrecs - 1, hv.fields[i]);
	}
	_seek_next();
	return *this;
}

// The following code causes memory leaking when called in vgroup.cc. 
// Since it is only used in vgroup.cc, we move the code to vgroup.cc.
// The memory leaking is gone.
#if 0
bool hdfistream_vdata::isInternalVdata(int ref) const {
	set<string, less<string> > reserved_names;
	reserved_names.insert("RIATTR0.0N");

	set<string, less<string> > reserved_classes;
	reserved_classes.insert("Attr0.0");
	reserved_classes.insert("RIATTR0.0C");
	reserved_classes.insert("DimVal0.0");
	reserved_classes.insert("DimVal0.1");
	reserved_classes.insert("_HDF_CHK_TBL_0");

	// get name, class of vdata
	int vid;
	if ((vid = VSattach(_file_id, ref, "r")) < 0) {
		THROW(hcerr_vdataopen);
	}
	char name[hdfclass::MAXSTR];
	char vclass[hdfclass::MAXSTR];
	if (VSgetname(vid, name) < 0) {
		VSdetach(vid);
		THROW(hcerr_vdatainfo);
	}
	if (reserved_names.find(string(name)) != reserved_names.end()) {
		VSdetach(vid);
		return true;
	}

	if (VSgetclass(vid, vclass) < 0) {
		VSdetach(vid);
		THROW(hcerr_vdatainfo);
	}

	VSdetach(vid);

	if (reserved_classes.find(string(vclass)) != reserved_classes.end())
		return true;

	return false;
}
#endif

static void LoadField(int32 vid, int index, int32 begin, int32 end,
		hdf_field & f) {
	DBG(cerr << "LoadField - vid: " << vid << endl);

	// position to first record too read
	if (VSseek(vid, begin) < 0)
		THROW(hcerr_vdataseek);
	int32 nrecs = end - begin + 1;

	// retrieve name of field
	DBG(cerr << "vid: " << vid << ", index: " << index << endl);
	const char *fieldname = VFfieldname(vid, index);
	if (fieldname == nullptr)
		THROW(hcerr_vdatainfo);
	f.name = string(fieldname);

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
	vector<char> data;
	if (nrecs > 0) { // if nrecs > 0 then load data for field
		data.resize(fieldsize * nrecs);
		DBG(cerr << "LoadField: vid=" << vid << ", fieldname=" << fieldname << endl);
		// TODO: Is this correct?
		// This code originally treated a negative return as a failure and
		// threw an exception. I'm still waiting on a verdict, but it seems
		// that the negative return may have been indicating an empty vdata
		// (or SDS dataset?) instead. For now, simply calling return here
		// addresses a problem where some NASA data files cannot otherwise be
		// read. See Trac ticket http://scm.opendap.org/trac/ticket/1793.
		// jhrg 8/17/11
		if (VSsetfields(vid, fieldname) < 0) {
			return;
		}

		if (VSread(vid, (uchar8 *)data.data(), nrecs, FULL_INTERLACE) < 0) {
                        string msg = "VSread error with the field: " + f.name + " (" + long_to_string(vid) + ").";
			throw BESInternalError(msg,__FILE__, __LINE__);
		}
#if 0
		if ((VSsetfields(vid, fieldname) < 0) || (VSread(vid, (uchar8 *) data,
				nrecs, FULL_INTERLACE) < 0)) {
			delete[] data;
			throw hcerr_vdataread(__FILE__, __LINE__);
		}
#endif
	}
	int stride = fieldorder;
	for (int i = 0; i < fieldorder; ++i) {
		if (nrecs == 0)
			gv = hdf_genvec(fieldtype, nullptr, 0, 0, 0);
		else
			gv = hdf_genvec(fieldtype, data.data(), i, (nrecs * fieldorder) - 1, stride);
		f.vals.push_back(gv);
	}
}

//
// hdf_vdata related member functions
//

// verify that the hdf_field class is in an OK state.
bool hdf_field::_ok(void) const {

	// make sure that there is some data stored in the hdf_field
	if (vals.empty() == true)
		return false;

	// if the field has order > 1, check to make sure that the number types of all
	// the columns in the field are the same and are not 0
	if (vals.size() > 1) {
		int32 nt = vals[0].number_type();
		if (nt == 0)
			return false;
		for (int i = 1; i < (int) vals.size(); ++i)
			if (vals[i].number_type() != nt || vals[i].number_type() == 0)
				return false;
	}

	return true; // passed all the tests
}

bool hdf_vdata::_ok(void) const {

	// make sure there are fields stored in this vdata
	if (fields.empty() == true)
		return false;

	// make sure the fields are themselves OK
	for (const auto & field:fields)
		if (!field)
			return false;

	return true; // passed all the tests
}

static bool IsInternalVdata(int32 fid, int32 ref) {
	set<string, less<string> > reserved_names;
	reserved_names.insert("RIATTR0.0N");

	set<string, less<string> > reserved_classes;
	reserved_classes.insert("Attr0.0");
	reserved_classes.insert("RIATTR0.0C");
	reserved_classes.insert("DimVal0.0");
	reserved_classes.insert("DimVal0.1");
	reserved_classes.insert("_HDF_CHK_TBL_0");

	// get name, class of vdata
	int vid;
	if ((vid = VSattach(fid, ref, "r")) < 0) {
		THROW(hcerr_vdataopen);
	}
	char name[hdfclass::MAXSTR];
	char vclass[hdfclass::MAXSTR];
	if (VSgetname(vid, name) < 0) {
		VSdetach(vid);
		THROW(hcerr_vdatainfo);
	}
	if (reserved_names.find(string(name)) != reserved_names.end()) {
		VSdetach(vid);
		return true;
	}

	if (VSgetclass(vid, vclass) < 0) {
		VSdetach(vid);
		THROW(hcerr_vdatainfo);
	}

	VSdetach(vid);

	if (reserved_classes.find(string(vclass)) != reserved_classes.end())
		return true;

	return false;
}
