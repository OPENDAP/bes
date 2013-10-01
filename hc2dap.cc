///////////////////////////////////////////////////////////////////////////////
/// \file hc2dap.cc
/// \brief Generate DAP Grids from HDF-EOS2 Grid files.
///
/// This file contains the functions that generate DAP Grid out of
/// HDF-EOS2 grid files when --enable-cf configuration option is enabled.
///
// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2008-2012 The HDF Group
// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org>
//
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

/////////////////////////////////////////////////////////////////////////////
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

// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
//////////////////////////////////////////////////////////////////////////////


#include "config_hdf.h"

// STL includes
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <BESDebug.h>
#include <debug.h>


using namespace std;
// HDF and HDFClass includes
// Include this on linux to suppres an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>

// DODS/HDF includes
#include "escaping.h"
#include "HDFInt32.h"
#include "HDFInt16.h"
#include "HDFUInt32.h"
#include "HDFUInt16.h"
#include "HDFFloat64.h"
#include "HDFFloat32.h"
#include "HDFByte.h"
#include "HDFStr.h"
#include "HDFArray.h"
#include "HDFGrid.h"
#include "HDFSequence.h"
#include "HDFStructure.h"
#include "hdfutil.h"
#include "dhdferr.h"
#include "hdf-maps.h"
#include "debug.h"


// Undefine the following to send signed bytes using unsigned bytes. 1/13/98
// jhrg.
#define SIGNED_BYTE_TO_INT32 1

BaseType *NewDAPVar(const string &varname,
		    const string &dataset,
		    int32 hdf_type);
void LoadStructureFromField(HDFStructure * stru, hdf_field & f, int row);

// STL predicate comparing equality of hdf_field objects based on their names
class fieldeq {
public:
    fieldeq(const string & s) {
        _val = s;
    }

    bool operator() (const hdf_field & f) const {
        return (f.name == _val);
    }

private:
    string _val;
};

// Create a DAP HDFSequence from an hdf_vdata.
HDFSequence *NewSequenceFromVdata(const hdf_vdata &vd, const string &dataset)
{
    // check to make sure hdf_vdata object is set up properly
    // Vdata must have a name
    if (!vd || vd.fields.size() == 0 || vd.name.empty())
        return 0;

    // construct HDFSequence
    HDFSequence *seq = new HDFSequence(vd.name, dataset);

    // step through each field and create a variable in the DAP Sequence
    for (int i = 0; i < (int) vd.fields.size(); ++i) {
        if (!vd.fields[i] || vd.fields[i].vals.size() < 1 ||
            vd.fields[i].name.empty()) {
            delete seq;         // problem with the field
            return 0;
        }
        HDFStructure *st = 0;
        try {
            st = new HDFStructure(vd.fields[i].name, dataset);

            // for each subfield add the subfield to st
            if (vd.fields[i].vals[0].number_type() == DFNT_CHAR8
                || vd.fields[i].vals[0].number_type() == DFNT_UCHAR8) {

                // collapse char subfields into one string
                string subname = vd.fields[i].name + "__0";
                BaseType *bt = new HDFStr(subname, dataset);
                st->add_var(bt); // *st now manages *bt
                delete bt;
            }
            else {
                // create a DODS variable for each subfield
                for (int j = 0; j < (int) vd.fields[i].vals.size(); ++j) {
                    ostringstream strm;
                    strm << vd.fields[i].name << "__" << j;
                    BaseType *bt =
                        NewDAPVar(strm.str(), dataset,
                                  vd.fields[i].vals[j].number_type());
                    st->add_var(bt); // *st now manages *bt
                    delete bt;
                }
            }
            seq->add_var(st); // *seq now manages *st
            delete st;
        }
        catch (...) {
            delete seq;
            delete st;
            throw;
        }
    }

    return seq;
}

// Create a DAP HDFStructure from an hdf_vgroup.
HDFStructure *NewStructureFromVgroup(const hdf_vgroup &vg, vg_map &vgmap,
                                     sds_map &sdmap, vd_map &vdmap,
                                     gr_map &grmap, const string &dataset)
{
    // check to make sure hdf_vgroup object is set up properly
    if (vg.name.length() == 0)  // Vgroup must have a name
        return 0;
    if (!vg)                    // Vgroup must have some tagrefs
        return 0;

    // construct HDFStructure
    HDFStructure *str = new HDFStructure(vg.name, dataset);
    bool nonempty = false;
    BaseType *bt = 0;
    try {
        // step through each tagref and copy its contents to DAP
        for (int i = 0; i < (int) vg.tags.size(); ++i) {
            int32 tag = vg.tags[i];
            int32 ref = vg.refs[i];

            switch (tag) {
            case DFTAG_VH:
                bt = NewSequenceFromVdata(vdmap[ref].vdata, dataset);
                break;
            case DFTAG_NDG:
                if (sdmap[ref].sds.has_scale()) {
                    bt = NewGridFromSDS(sdmap[ref].sds, dataset);
                } else {
                    bt = NewArrayFromSDS(sdmap[ref].sds, dataset);
                }
                break;
            case DFTAG_VG:
                // GR's are also stored as Vgroups
                if (grmap.find(ref) != grmap.end()){
                    bt = NewArrayFromGR(grmap[ref].gri, dataset);
                }
                else
                    bt = NewStructureFromVgroup(vgmap[ref].vgroup, vgmap,
                                                sdmap, vdmap, grmap, dataset);
                break;
            default:
                break;
            }
            if (bt) {
                str->add_var(bt);   // *st now manages *bt
                delete bt;
                nonempty = true;
            }
        }
    }
    catch(...) {
    	delete str;
    	delete bt;
    	throw;
    }

    if (nonempty) {
        return str;
    } else {
        delete str;
        return 0;
    }
}

// Create a DAP HDFArray out of the primary array in an hdf_sds
HDFArray *NewArrayFromSDS(const hdf_sds & sds, const string &dataset)
{
    if (sds.name.length() == 0) // SDS must have a name
        return 0;
    if (sds.dims.size() == 0)   // SDS must have rank > 0
        return 0;

    // construct HDFArray, assign data type
    BaseType *bt = NewDAPVar(sds.name, dataset, sds.data.number_type());
    if (bt == 0) {              // something is not right with SDS number type?
        return 0;
    }
    try {
        HDFArray *ar = 0;
        ar = new HDFArray(sds.name,dataset,bt);
        delete bt;

        // add dimension info to HDFArray
        for (int i = 0; i < (int) sds.dims.size(); ++i)
            ar->append_dim(sds.dims[i].count, sds.dims[i].name);

        return ar;
    }
    catch (...) {
        delete bt;
        throw;
    }
}

// Create a DAP HDFArray out of a general raster
HDFArray *NewArrayFromGR(const hdf_gri & gr, const string &dataset)
{
    if (gr.name.length() == 0)  // GR must have a name
        return 0;

    // construct HDFArray, assign data type
    BaseType *bt = NewDAPVar(gr.name, dataset, gr.image.number_type());
    if (bt == 0) {              // something is not right with GR number type?
        return 0;
    }

    try {
        HDFArray *ar = 0;
        ar = new HDFArray(gr.name, dataset, bt);

        // Array duplicates the base type passed, so delete here
        delete bt;

        // add dimension info to HDFArray
        if (gr.num_comp > 1)
            ar->append_dim(gr.num_comp, gr.name + "__comps");
        ar->append_dim(gr.dims[1], gr.name + "__Y");
        ar->append_dim(gr.dims[0], gr.name + "__X");
        return ar;
    }
    catch (...) {
        delete bt;
        throw;
    }
}

// Create a DAP HDFGrid out of the primary array and dim scale in an hdf_sds
HDFGrid *NewGridFromSDS(const hdf_sds & sds, const string &dataset)
{
    BESDEBUG("h4", "NewGridFromSDS" << endl);
    if (!sds.has_scale())       // we need a dim scale to make a Grid
        return 0;

    // Create the HDFGrid and the primary array.  Add the primary array to
    // the HDFGrid.
    HDFArray *ar = NewArrayFromSDS(sds, dataset);
    if (ar == 0)
        return 0;

    HDFGrid *gr = 0;
    HDFArray *dmar = 0;
    BaseType *dsbt = 0;
    try {
        gr = new HDFGrid(sds.name, dataset);
        gr->add_var(ar, array); // note: gr now manages ar
        delete ar;

        // create dimension scale HDFArrays (i.e., maps) and
        // add them to the HDFGrid
        string mapname;
        for (int i = 0; i < (int) sds.dims.size(); ++i) {
            if (sds.dims[i].name.length() == 0) { // the dim must be named
                delete gr;
                return 0;
            }
            mapname = sds.dims[i].name;
            if ((dsbt = NewDAPVar(mapname, dataset,
                                  sds.dims[i].scale.number_type())) == 0) {
                delete gr; // note: ~HDFGrid() cleans up the attached ar
                return 0;
            }
            dmar = new HDFArray(mapname, dataset, dsbt);
            delete dsbt;
            dmar->append_dim(sds.dims[i].count); // set dimension size
            gr->add_var(dmar, maps); // add dimension map to grid;
            delete dmar;
        }
        return gr;
    }
    catch (...) {
        delete dmar;
        delete dsbt;
        delete gr;
        delete ar;
        throw;
    }
}

// Return a ptr to DAP atomic data object corresponding to an HDF Type, or
// return 0 if the HDF Type is invalid or not supported.
BaseType *NewDAPVar(const string &varname,
		    const string &dataset,
		    int32 hdf_type)
{
    switch (hdf_type) {
    case DFNT_FLOAT32:
        return new HDFFloat32(varname, dataset);

    case DFNT_FLOAT64:
        return new HDFFloat64(varname, dataset);

    case DFNT_INT16:
        return new HDFInt16(varname, dataset);

#ifdef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_INT32:
        return new HDFInt32(varname, dataset);

    case DFNT_UINT16:
        return new HDFUInt16(varname, dataset);

    case DFNT_UINT32:
        return new HDFUInt32(varname, dataset);

        // INT8 and UINT8 *should* be grouped under Int32 and UInt32, but
        // that breaks too many programs. jhrg 12/30/97
#ifndef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_UINT8:
    case DFNT_UCHAR8:
    case DFNT_CHAR8:
        return new HDFByte(varname, dataset);

    default:
        return 0;
    }
}

// Return the DAP type name that corresponds to an HDF data type
string DAPTypeName(int32 hdf_type)
{
    switch (hdf_type) {
    case DFNT_FLOAT32:
        return string("Float32");

    case DFNT_FLOAT64:
        return string("Float64");

    case DFNT_INT16:
        return string("Int16");

#ifdef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_INT32:
        return string("Int32");

    case DFNT_UINT16:
        return string("UInt16");

    case DFNT_UINT32:
        return string("UInt32");

        // See the note above about INT8 and UINT8. jhrg 12/30/97.
#ifndef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_UINT8:
        return string("Byte");

    case DFNT_CHAR8:
    case DFNT_UCHAR8:
        // note: DFNT_CHAR8 is Byte in DDS but String in DAS
        return string("String");

    default:
        return string("");
    }
}

// load an HDFArray from an SDS
void LoadArrayFromSDS(HDFArray * ar, const hdf_sds & sds)
{
#ifdef SIGNED_BYTE_TO_INT32
    switch (sds.data.number_type()) {
    case DFNT_INT8:{
        char *data = static_cast < char *>(ExportDataForDODS(sds.data));
        ar->val2buf(data);
        delete[]data;
        break;
    }
    default:
        ar->val2buf(const_cast < char *>(sds.data.data()));
    }
#else
    ar->val2buf(const_cast < char *>(sds.data.data()));
#endif
    return;
}

// load an HDFArray from a GR image
void LoadArrayFromGR(HDFArray * ar, const hdf_gri & gr)
{
#ifdef SIGNED_BYTE_TO_INT32
    switch (gr.image.number_type()) {
    case DFNT_INT8:{
        char *data = static_cast < char *>(ExportDataForDODS(gr.image));
        ar->val2buf(data);
        delete[]data;
        break;
    }

    default:
        ar->val2buf(const_cast < char *>(gr.image.data()));
    }
#else
    ar->val2buf(const_cast < char *>(gr.image.data()));
#endif
    return;
}

// load an HDFGrid from an SDS
// I modified Todd's code so that only the parts of a Grid that are marked as
// to be sent will be read. 1/29/2002 jhrg
void LoadGridFromSDS(HDFGrid * gr, const hdf_sds & sds)
{

    // load data into primary array
    HDFArray & primary_array = static_cast < HDFArray & >(*gr->array_var());
    if (primary_array.send_p()) {
        LoadArrayFromSDS(&primary_array, sds);
        primary_array.set_read_p(true);
    }
    // load data into maps
    if (primary_array.dimensions() != sds.dims.size())
        THROW(dhdferr_consist); // # of dims of SDS and HDFGrid should agree!

    Grid::Map_iter p = gr->map_begin();
    for (unsigned int i = 0;
         i < sds.dims.size() && p != gr->map_end(); ++i, ++p) {
        if ((*p)->send_p()) {
#ifdef SIGNED_BYTE_TO_INT32
            switch (sds.dims[i].scale.number_type()) {
            case DFNT_INT8:{
                char *data = static_cast < char *>(ExportDataForDODS(sds.dims[i].scale));
                (*p)->val2buf(data);
                delete[]data;
                break;
            }
            default:
                (*p)->val2buf(const_cast < char *>
                              (sds.dims[i].scale.data()));
            }
#else
            (*p)->val2buf(const_cast < char *>(sds.dims[i].scale.data()));
#endif
            (*p)->set_read_p(true);
        }
    }
    return;
}

// load an HDFSequence from a row of an hdf_vdata
void LoadSequenceFromVdata(HDFSequence * seq, hdf_vdata & vd, int row)
{
    Constructor::Vars_iter p;
    for (p = seq->var_begin(); p != seq->var_end(); ++p) {
        HDFStructure & stru = static_cast < HDFStructure & >(**p);

        // find corresponding field in vd
        vector < hdf_field >::iterator vf =
            find_if(vd.fields.begin(), vd.fields.end(),
                    fieldeq(stru.name()));
        if (vf == vd.fields.end())
            THROW(dhdferr_consist);

        // for each field component of field, extract the proper data element
        // for the current row being requested and load into the Structure
        // variable
        LoadStructureFromField(&stru, *vf, row);
        stru.set_read_p(true);
    }
}

// Load an HDFStructure with the components of a row of an hdf_field.  If the
// field is made of char8 components, collapse these into one String component
void LoadStructureFromField(HDFStructure * stru, hdf_field & f, int row)
{

    if (row < 0 || f.vals.size() <= 0 || row > (int) f.vals[0].size())
        THROW(dhdferr_conv);

    BaseType *firstp = *stru->var_begin();
    if (firstp->type() == dods_str_c) {
        // If the Structure contains a String, then that is all it will
        // contain.  In that case, concatenate the different char8
        // components of the field and load the DODS String with the value.
        string str = "";
        for (unsigned int i = 0; i < f.vals.size(); ++i) {
            //DBG(cerr << i << ": " << f.vals[i].elt_char8(row) << endl);
            str += f.vals[i].elt_char8(row);
        }

        firstp->val2buf(static_cast < void *>(&str));   // data);
        firstp->set_read_p(true);
    } else {
        // for each component of the field, load the corresponding component
        // of the DODS Structure.
        int i = 0;
        Constructor::Vars_iter q;
        for (q = stru->var_begin(); q != stru->var_end(); ++q, ++i) {
            char *val = static_cast <char *>(ExportDataForDODS(f.vals[i], row));
            (*q)->val2buf(val);
#if 0
            delete[] val;
#endif
            delete val;
            (*q)->set_read_p(true);
        }

    }
    return;
}

// Load an HDFStructure with the contents of a vgroup.
void LoadStructureFromVgroup(HDFStructure * str, const hdf_vgroup & vg,
                             const string & hdf_file)
{
    int i = 0;
    int err = 0;
    Constructor::Vars_iter q;
    for (q = str->var_begin(); err == 0 && q != str->var_end(); ++q, ++i) {
        BaseType *p = *q;
        BESDEBUG("h4", "Reading within LoadStructureFromVgroup: " << p->name()
        	 << ", send_p: " << p->send_p() << ", vg.names[" << i << "]: "
        	 << vg.vnames[i] << endl);
        if (p && p->send_p() && p->name() == vg.vnames[i]) {
            (dynamic_cast < ReadTagRef & >(*p)).read_tagref(vg.tags[i],
                                                            vg.refs[i],
                                                            err);
        }
    }
}
