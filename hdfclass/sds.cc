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
// $RCSfile: sds.cc,v $ - input stream class for HDF SDS
//
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <mfhdf.h>

#ifdef __POWERPC__
#undef isascii
#endif

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <cerrno>

#include <hcstream.h>
#include <hdfclass.h>

#include <BESDebug.h>

using std::cerr;
using std::endl;

// minimum function
inline int min(int t1, int t2)
{
    return (t1 < t2 ? t1 : t2);
}

// static initializations
const string hdfistream_sds::long_name = "long_name";
const string hdfistream_sds::units = "units";
const string hdfistream_sds::format = "format";

//
// protected member functions
//

// initialize an hdfistream_sds, opening file if given
void hdfistream_sds::_init(void)
{
    _sds_id = _attr_index = _dim_index = _nsds = _rank = _nattrs =
        _nfattrs = 0;
    _index = -1;                // set BOS
    _meta = _slab.set = false;
    _map_ce_set = false;
    return;
}

// retrieve descriptive information about file containing SDS
void hdfistream_sds::_get_fileinfo(void)
{
    if (SDfileinfo(_file_id, &_nsds, &_nfattrs) < 0)
        THROW(hcerr_sdsinfo);
    return;
}

// retrieve descriptive information about currently open SDS
void hdfistream_sds::_get_sdsinfo(void)
{
    char junk0[hdfclass::MAXSTR];
    int32 junk1[hdfclass::MAXDIMS];
    int32 junk2;

    // all we care about is rank and number of attributes
    if (SDgetinfo(_sds_id, junk0, &_rank, junk1, &junk2, &_nattrs) < 0)
        THROW(hcerr_sdsinfo);

    if (_rank > hdfclass::MAXDIMS)      // too many dimensions
        THROW(hcerr_maxdim);
    return;
}

// end access to currently open SDS
void hdfistream_sds::_close_sds(void)
{
    if (_sds_id != 0) {
        (void) SDendaccess(_sds_id);
        _sds_id = _attr_index = _dim_index = _rank = _nattrs = 0;
        _index = -1;
    }
    return;
}

// find the next SDS array (not necessarily the next SDS) in the file
void hdfistream_sds::_seek_next_arr(void)
{
    if (_sds_id != 0) {
        BESDEBUG("h4", "hdfistream_sds::_seek_next_arr called with an open sds: "
                 << _sds_id << endl);
        SDendaccess(_sds_id);
        _sds_id = 0;
    }

    for (_index++, _dim_index = _attr_index = 0; _index < _nsds; ++_index) {
        if (_sds_id != 0) {
            BESDEBUG("h4", "hdfistream_sds::_seek_next_arr inside for-loop with an open sds: "
                     << _sds_id << endl);
        }
        if ((_sds_id = SDselect(_file_id, _index)) < 0)
            THROW(hcerr_sdsopen);
        if (!SDiscoordvar(_sds_id))
            break;
        SDendaccess(_sds_id);
        _sds_id = 0;
    }
}

// find the arr_index'th SDS array in the file (don't count non-array SDS's)
void hdfistream_sds::_seek_arr(int arr_index)
{
    int arr_count = 0;
    for (_rewind(); _index < _nsds && arr_count <= arr_index;
         _seek_next_arr(), arr_count++);
}

// find the SDS array with specified name
void hdfistream_sds::_seek_arr(const string & name)
{
    if (_sds_id != 0) {
        BESDEBUG("h4", "hdfistream_sds::_seek_arr called with an open sds: "
                 << _sds_id << endl);
        _close_sds();
    }

    int index;
    const char *nm = name.c_str();
    if ((index = SDnametoindex(_file_id, (char *) nm)) < 0)
        THROW(hcerr_sdsfind);
    if ((_sds_id = SDselect(_file_id, index)) < 0)
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
void hdfistream_sds::_seek_arr_ref(int ref)
{
    if (_sds_id != 0) {
        BESDEBUG("h4", "hdfistream_sds::_seek_arr_ref called with an open sds: "
                 << _sds_id << endl);
        _close_sds();
    }

    int index;
    if ((index = SDreftoindex(_file_id, ref)) < 0)
        THROW(hcerr_sdsfind);
    if ((_sds_id = SDselect(_file_id, index)) < 0)
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
hdfistream_sds::hdfistream_sds(const string filename):
hdfistream_obj(filename)
{
    _init();
    if (_filename.length() != 0)        // if ctor specified a file to open
        open(_filename.c_str());
    return;
}

// check to see if stream has been positioned past last SDS in file
bool hdfistream_sds::eos(void) const
{
    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    if (_nsds == 0)             // eos() is always true of there are no SDS's in file
        return true;
    else {
        if (bos())              // eos() is never true if at bos() and there are SDS's
            return false;
        else
            return (_index >= _nsds);   // are we indexed past the last SDS?
    }
}

// check to see if stream is positioned in front of the first SDS in file
bool hdfistream_sds::bos(void) const
{
    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    if (_nsds == 0)
        return true;            // if there are no SDS's we still want to read file attrs so both eos() and bos() are true
    if (_index == -1)
        return true;
    else
        return false;
}

// check to see if stream is positioned past the last attribute in the currently
// open SDS
bool hdfistream_sds::eo_attr(void) const
{
    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    if (eos() && !bos())        // if eos(), then always eo_attr()
        return true;
    else {
        if (bos())              // are we at BOS and are positioned past last file attributes?
            return (_attr_index >= _nfattrs);
        else
            return (_attr_index >= _nattrs);    // or positioned after last SDS attr?
    }
}

// check to see if stream is positioned past the last dimension in the currently
// open SDS
bool hdfistream_sds::eo_dim(void) const
{
    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    if (eos())                  // if eos(), then always eo_dim()
        return true;
    else {
        if (bos())              // if at BOS, then never eo_dim()
            return true;
        else
            return (_dim_index >= _rank);       // are we positioned after last dim?
    }
}

// open a new file
void hdfistream_sds::open(const char *filename)
{
    if (filename == 0)          // no filename given
        THROW(hcerr_openfile);
    BESDEBUG("h4", "sds opening file " << filename << endl);
    if (_file_id != 0)          // close any currently open file
        close();
    if ((_file_id = SDstart((char *) filename, DFACC_READ)) < 0)
    {
        THROW(hcerr_openfile);
    }

    BESDEBUG("h4", "sds file opened: id=" << _file_id << endl);

    _filename = filename;       // assign filename
    _get_fileinfo();            // get file information
    rewind();                   // position at BOS to start
    return;
}

// close currently open file (if any)
void hdfistream_sds::close(void)
{                               // close file
    BESDEBUG("h4", "sds file closed: id=" << _file_id << ", this: " << this<< endl);

    _close_sds();               // close any currently open SDS
    if (_file_id != 0)          // if open file, then close it
        (void) SDend(_file_id);
    _file_id = _nsds = _nfattrs = 0;    // zero file info
    return;
}

// position SDS array index to index'th SDS array (not necessarily index'th SDS)
void hdfistream_sds::seek(int index)
{
    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    _close_sds();               // close any currently open SDS
    _seek_arr(index);           // seek to index'th SDS array
    if (!eos() && !bos())       // if not BOS or EOS, get SDS information
        _get_sdsinfo();
}

// position SDS array index to SDS array with name "name"
void hdfistream_sds::seek(const char *name)
{
    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    _close_sds();               // close any currently open SDS
    _seek_arr(string(name));    // seek to index'th SDS array
    if (!eos() && !bos())       // if not BOS or EOS, get SDS information
        _get_sdsinfo();
}

// position SDS array index in front of first SDS array
void hdfistream_sds::rewind(void)
{
    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    _close_sds();               // close any already open SDS
    _rewind();                  // seek to BOS
}

// position to next SDS array in file
void hdfistream_sds::seek_next(void)
{
    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    _seek_next_arr();           // seek to next SDS array
    if (!eos())                 // if not EOS, get SDS information
        _get_sdsinfo();
}

// position to SDS array by ref
void hdfistream_sds::seek_ref(int ref)
{
    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    _close_sds();               // close any currently open SDS
    _seek_arr_ref(ref);         // seek to SDS array by reference
    if (!eos() && !bos())       // if not BOS or EOS, get SDS information
        _get_sdsinfo();
}

// set slab parameters
void hdfistream_sds::setslab(vector < int >start, vector < int >edge,
                             vector < int >stride, bool reduce_rank)
{
    // check validity of input
    if (start.size() != edge.size() || edge.size() != stride.size()
        || start.size() == 0)
        THROW(hcerr_invslab);

    int i;
    for (i = 0; i < (int) start.size() && i < hdfclass::MAXDIMS; ++i) {
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
// memory exceeded error. 2/25/98 jhrg
// load currently open SDS into an hdf_sds object
hdfistream_sds & hdfistream_sds::operator>>(hdf_sds & hs)
{

    // delete any previous data in hs
    hs.dims = vector < hdf_dim > ();
    hs.attrs = vector < hdf_attr > ();
    hs.data = hdf_genvec();
    hs.name = string();

    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    if (bos())                  // if at BOS, advance to first SDS array
        seek(0);
    if (eos())                  // if at EOS, do nothing
        return *this;

    // get basic info about SDS
    char name[hdfclass::MAXSTR];
    int32 rank;
    int32 dim_sizes[hdfclass::MAXDIMS];
    int32 number_type;
    int32 nattrs;
    if (SDgetinfo(_sds_id, name, &rank, dim_sizes, &number_type, &nattrs) <
        0)
        THROW(hcerr_sdsinfo);

    // assign SDS index
    hs.ref = SDidtoref(_sds_id);
    // load dimensions and attributes into the appropriate objects
    *this >> hs.dims;
    *this >> hs.attrs;
    hs.name = name;             // assign SDS name
    char *data = 0;
    int nelts = 1;
    if (_meta)                  // if _meta is set, just load type information
        hs.data.import(number_type);
    else {
        if (_slab.set) {        // load a slab of SDS array data
            for (int i = 0; i < rank; ++i)
                nelts *= _slab.edge[i];

            // allocate a temporary C array to hold the data from SDreaddata()
            int datasize = nelts * DFKNTsize(number_type);
            data = new char[datasize];
            if (data == 0)
                THROW(hcerr_nomemory);
            BESDEBUG("h4", "SDreaddata() on line 387. _sds_id: " << _sds_id
                << endl);
            if (SDreaddata(_sds_id, _slab.start, _slab.stride, _slab.edge,
                           data) < 0) {
                delete[]data;   // problem: clean up and throw an exception
                THROW(hcerr_sdsread);
            }
        } else {                // load entire SDS array
            // prepare for SDreaddata(): make an array of zeroes and calculate
            // number of elements of array data
            int32 zero[hdfclass::MAXDIMS];
            for (int i = 0; i < rank && i < hdfclass::MAXDIMS; ++i) {
                zero[i] = 0;
                nelts *= dim_sizes[i];
            }

            // allocate a temporary C array to hold the data from SDreaddata()
            int datasize = nelts * DFKNTsize(number_type);
            data = new char[datasize];
            if (data == 0)
                THROW(hcerr_nomemory);

            // read the data and store it in an hdf_genvec
            if (SDreaddata(_sds_id, zero, 0, dim_sizes, data) < 0) {
                delete[]data;   // problem: clean up and throw an exception
                THROW(hcerr_sdsread);
            }
        }

        hs.data.import(number_type, data, nelts);
        delete[]data;           // deallocate temporary C array
    }

    seek_next();                // position to next SDS array
    return *this;
}

// Functor to help look for a particular map's ce in the vector of array_ce
// objects.
class ce_name_match:public std::unary_function < array_ce, bool > {
    string name;
  public:
    ce_name_match(const string & n):name(n) {
    } bool operator() (const array_ce & a_ce) {
        return name == a_ce.name;
    }
};

// load dimension currently positioned at
hdfistream_sds & hdfistream_sds::operator>>(hdf_dim & hd)
{

    // delete any previous data in hd
    hd.name = hd.label = hd.unit = hd.format = string();
    hd.count = 0;
    hd.scale = hdf_genvec();
    hd.attrs = vector < hdf_attr > ();

    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    if (bos())                  // if at BOS, advance to first SDS array
        seek(0);

    // This code looks like an optimization to avoid reading unneeded
    // dimensions. If we're here because we're reading the whole Grid, it
    // needs to be run, If we're here because the client has asked only for
    // the Grid's maps, then it's harmless since the Grid's array's
    // constraint is the default (the whole array) and so there'll be no
    // reduction in rank. 2/5/2002 jhrg
    //
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
    if ((dim_id = SDgetdimid(_sds_id, _dim_index)) < 0)
        THROW(hcerr_sdsinfo);

    // get dimension information

    char name[hdfclass::MAXSTR];
    int32 count, number_type, nattrs;
    if (SDdiminfo(dim_id, name, &count, &number_type, &nattrs) < 0)
        THROW(hcerr_sdsinfo);
    else
        hd.name = name;         // assign dim name

    // Grab the current slab, save its value, and set _slab using map_ce. To
    // choose the correct constraint scan the vector of array_ce objects
    // looking for the one with the same name as this dimension. Doing this
    // assures us that the constraint that's used when data is extracted from
    // the hdf_genvec is done according to the constraint on this map. If we
    // used _slab as it's set on entry to this method we would be using the
    // constraint associated with the Grid's array. If the client asked only
    // for maps, that constraint would default to the whole array and hence
    // we'd be returning the whole map vector and ignoring the actual
    // constraint sent by the client. 2/5/2002 jhrg
    slab s = _slab;
    if (is_map_ce_set()) {      // Only go here if the map_ce_vec has been
        // set. The is_map_ce_set() predicate is
        // false by default.
#if 0
        cerr << "dim name: " << name << endl;
        cerr << "slab set: " << _slab.set << endl;
        cerr << "dim index: " << _dim_index << endl;
        cerr << "slab start: " << _slab.start[_dim_index] << endl;
        cerr << "slab edge: " << _slab.edge[_dim_index] << endl;
#endif

        vector < array_ce > ce = get_map_ce();
        vector < array_ce >::iterator ce_iter =
            find_if(ce.begin(), ce.end(), ce_name_match(string(name)));
#if 0
        cerr << "ce name: " << ce_iter->name << endl;
        cerr << "ce set: " << (ce_iter->start != 0 || ce_iter->edge != 0
                               || ce_iter->stride != 0) << endl;
        cerr << "ce start: " << ce_iter->start << endl;
        cerr << "ce edge: " << ce_iter->edge << endl << endl;
#endif

        _slab.set = ce_iter->start != 0 || ce_iter->edge != 0
            || ce_iter->stride != 0;
        _slab.reduce_rank = false;      // hard to reduce the rank of a vector...
        _slab.start[_dim_index] = ce_iter->start;
        _slab.edge[_dim_index] = ce_iter->edge;
        _slab.stride[_dim_index] = ce_iter->stride;
    }
    // Catch any throws and reset _slab.
    try {
        char label[hdfclass::MAXSTR];
        char unit[hdfclass::MAXSTR];
        char cformat[hdfclass::MAXSTR];
        if (SDgetdimstrs(dim_id, label, unit, cformat, hdfclass::MAXSTR) ==
            0) {
            hd.label = label;   // assign dim label
            hd.unit = unit;     // assign dim unit
            hd.format = cformat;        // assign dim format
        }
        // if we are dealing with a dimension of size unlimited, then call
        // SDgetinfo to get the current size of the dimension
        if (count == 0) {       // unlimited dimension
            if (_dim_index != 0)
                THROW(hcerr_sdsinfo);   // only first dim can be unlimited
            char junk[hdfclass::MAXSTR];
            int32 junk2, junk3, junk4;
            int32 dim_sizes[hdfclass::MAXDIMS];
            if (SDgetinfo(_sds_id, junk, &junk2, dim_sizes, &junk3, &junk4)
                < 0)
                THROW(hcerr_sdsinfo);
            count = dim_sizes[0];
        }
        // load user-defined attributes for the dimension
        // TBD!  Not supported at present

        // load dimension scale if there is one

        if (number_type != 0) { // found a dimension scale

            /*
             *  Currently, this server cannot support dimension scales
             *  that are stored as character arrays. See bugs 748 and 756.
             */
            if (number_type != DFNT_CHAR) {
                // allocate a temporary C array to hold data from
                // SDgetdimscale()
                char *data = new char[count * DFKNTsize(number_type)];

                if (data == 0)
                    THROW(hcerr_nomemory);

                // read the scale data and store it in an hdf_genvec
                if (SDgetdimscale(dim_id, data) < 0) {
                    delete[]data;       // problem: clean up and throw an exception
                    THROW(hcerr_sdsinfo);
                }

                if (_slab.set) {
                    void *datastart = (char *) data +
                        _slab.start[_dim_index] * DFKNTsize(number_type);
                    hd.scale = hdf_genvec(number_type, datastart, 0,
                                          _slab.edge[_dim_index] *
                                          _slab.stride[_dim_index] - 1,
                                          _slab.stride[_dim_index]);
                } else
                    hd.scale = hdf_genvec(number_type, data, count);

                delete[]data;   // deallocate temporary C array
            }
        }
        // assign dim size; if slabbing is set, assigned calculated size,
        // otherwise assign size from SDdiminfo()
        if (_slab.set)
            hd.count = _slab.edge[_dim_index];
        else
            hd.count = count;
        _dim_index++;
    }
    catch(...) {
        _slab = s;
        throw;
    }

    _slab = s;                  // reset _slab

    return *this;
}

// load attribute currently positioned at
hdfistream_sds & hdfistream_sds::operator>>(hdf_attr & ha)
{

    // delete any previous data in ha
    ha.name = string();
    ha.values = hdf_genvec();

    if (_filename.length() == 0)        // no file open
        THROW(hcerr_invstream);
    if (eo_attr())              // if positioned past last attribute, do nothing
        return *this;

    // prepare to read attribute information: set nattrs, id depending on whether
    // reading file attributes or SDS attributes
    int32 id;
    //int nattrs;
    if (bos()) {                // if at BOS, then read file attributes
        //nattrs = _nfattrs;
        id = _file_id;
    } else {                    // else read SDS attributes
        //nattrs = _nattrs;
        id = _sds_id;
    }
    char name[hdfclass::MAXSTR];
    int32 number_type, count;
    if (SDattrinfo(id, _attr_index, name, &number_type, &count) < 0)
        THROW(hcerr_sdsinfo);

    // allowcate a temporary C array to hold data from SDreadattr()
    char *data;
    data = new char[count * DFKNTsize(number_type)];
    if (data == 0)
        THROW(hcerr_nomemory);

    // read attribute values and store them in an hdf_genvec
    if (SDreadattr(id, _attr_index, data) < 0) {
        delete[]data;           // problem: clean up and throw an exception
        THROW(hcerr_sdsinfo);
    }
    // eliminate trailing null characters from the data string;
    // they cause GNU's String class problems
    // NOTE: removed because count=0 if initial char is '\0' and we're not
    //   using GNU String anymore
#if 0
    if (number_type == DFNT_CHAR)
        count = (int32) min((int) count, (int) strlen((char *) data));
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
    delete[]data;               // deallocate temporary C array

    // increment attribute index to next attribute
    ++_attr_index;
    ha.name = name;             // assign attribute name
    return *this;
}

// This function, when compiled with gcc 2.8 and -O2, causes a virtual
// Memory exceeded error. 2/25/98 jhrg
// read in all the SDS arrays in a file
hdfistream_sds & hdfistream_sds::operator>>(vector < hdf_sds > &hsv)
{
    //    hsv = vector<hdf_sds>0;   // reset vector
    for (hdf_sds sds; !eos();) {
        *this >> sds;
        hsv.push_back(sds);
    }
    return *this;
}

// read in all of the SDS attributes for currently open SDS
hdfistream_sds & hdfistream_sds::operator>>(vector < hdf_attr > &hav)
{
    //    hav = vector<hdf_attr>0;  // reset vector
    for (hdf_attr att; !eo_attr();) {
        *this >> att;
        hav.push_back(att);
    }
    return *this;
}

// read in all of the SDS dimensions for currently open SDS
hdfistream_sds & hdfistream_sds::operator>>(vector < hdf_dim > &hdv)
{
    //    hdv = vector<hdf_dim>0;   // reset vector
    for (hdf_dim dim; !eo_dim();) {
        *this >> dim;
        hdv.push_back(dim);
    }
    return *this;
}

// Check to see if this hdf_sds has a dim scale: if all of the dimensions have
// scale sizes = to their size, it does
bool hdf_sds::has_scale(void) const
{
    bool has_scale;
    if (!_ok(&has_scale)) {
        THROW(hcerr_sdsscale);
        return false;
    } else
        return has_scale;
}

// Verify that the hdf_sds is in an OK state (i.e., that it is initialized
// correctly) if user has passed a ptr to a bool then set that to whether
// there is a dimension scale for at least one of the dimensions
//
// Added `if (*has_scale)...' because `has_scale' defaults to null and
// dereferencing it causes segmentation faults, etc.

bool hdf_sds::_ok(bool * has_scale) const
{

    if (has_scale)
        *has_scale = false;

    // Check to see that for each SDS dimension scale, that the length of the
    // scale matches the size of the dimension.
    for (int i = 0; i < (int) dims.size(); ++i)
        if (dims[i].scale.size() != 0) {
            if (has_scale)
                *has_scale = true;
            if (dims[i].scale.size() != dims[i].count)
                return false;
        }

    return true;
}
