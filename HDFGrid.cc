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
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
// $RCSfile: HDFGrid.cc,v $ - HDFGrid class implementation
//
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <vector>
// Include this on linux to suppres an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>

#include "escaping.h"
#include "HDFGrid.h"
#include "HDFArray.h"
#include "hdfutil.h"
#include "dhdferr.h"

#include "Error.h"

HDFGrid::HDFGrid(const string & n):Grid(n)
{
}

HDFGrid::~HDFGrid()
{
}
BaseType *HDFGrid::ptr_duplicate()
{
    return new HDFGrid(*this);
}

void LoadGridFromSDS(HDFGrid * gr, const hdf_sds & sds);

// Build a vector of array_ce structs. This holds the constraint
// information *for each map* of the Grid.
vector < array_ce > HDFGrid::get_map_constraints()
{
    vector < array_ce > a_ce_vec;

    // Load the array_ce vector with info about each map vector.
    for (Grid::Map_iter p = map_begin(); p != map_end(); ++p) {
        Array & a = dynamic_cast < Array & >(**p);
        Array::Dim_iter q = a.dim_begin();      // maps have only one dimension.
        int start = a.dimension_start(q, true);
        int stop = a.dimension_stop(q, true);
        int stride = a.dimension_stride(q, true);
        int edge = (int) ((stop - start) / stride) + 1;
        array_ce a_ce(a.name(), start, edge, stride);
        a_ce_vec.push_back(a_ce);
    }

    return a_ce_vec;
}

// Read in a Grid from an SDS in an HDF file.
bool HDFGrid::read(const string & dataset)
{
    int err = 0;
    int status = read_tagref(dataset, -1, -1, err);
    if (err)
        throw Error(unknown_error, "Could not read from dataset.");
    return status;
}

bool HDFGrid::read_tagref(const string & dataset, int32 tag, int32 ref,
                          int &err)
{
    if (read_p())
        return true;

    err = 0;                    // OK initially

    string hdf_file = dataset;
    string hdf_name = this->name();

    hdf_sds sds;

    // read in SDS
    hdfistream_sds sdsin(hdf_file.c_str());
    try {
        vector < int >start, edge, stride;
        HDFArray *primary_array = dynamic_cast < HDFArray * >(array_var());
        bool isslab =
            primary_array->GetSlabConstraint(start, edge, stride);

        // get slab constraint from primary array
        if (isslab)
            sdsin.setslab(start, edge, stride, false);

        // get the constraints on each map
        sdsin.set_map_ce(get_map_constraints());

        if (ref != -1)
            sdsin.seek_ref(ref);
        else
            sdsin.seek(hdf_name.c_str());

        // If we read the array, we also read the maps. 2/3/2002 jhrg
        if (array_var()->send_p() || array_var()->is_in_selection()) {
            sdsin >> sds;
            if (!sds) {
                throw Error(string("Could not read ") + array_var()->name()
                            + string(" from dataset ") + dataset
                            + string("."));
            }

            LoadGridFromSDS(this, sds); // load data into primary array
        }
        // load map data. There's little point in checking if the maps really
        // need to be read. If the array was read, chances are good and the
        // map vectors are much smaller. If the array was not read, then some
        // map must be marked to be sent or we wouldn't be here. So just load
        // the maps...

        // Read only if not above. sdsin >> hdf_sds also reads the maps so we
        // should read here only if we didn't read above.
        if (!(array_var()->send_p() || array_var()->is_in_selection())) {
            // This initialization is done by hdfistream_sds op>>(hdf_sds&)
            // but not hdfistream_sds op>>(hdf_dim&).
            sds.dims = vector < hdf_dim > ();
            sds.data = hdf_genvec();    // needed?
            //      sds.ref = SDidtoref(_sds_id);
            sdsin >> sds.dims;
        }

        for (Grid::Map_iter p = map_begin(); p != map_end(); ++p) {
            if ((*p)->send_p() || (*p)->is_in_selection()) {
                for (unsigned int i = 0; i < sds.dims.size(); i++) {
                    if ((*p)->name() == sds.dims[i].name) {
                        // Read the data from the sds dimension.
                        char *data =
                            static_cast <
                            char *>(ExportDataForDODS(sds.dims[i].scale));
                        (*p)->val2buf(data);
                        delete[]data;
                        (*p)->set_read_p(true);
                    }
                }
            }
        }

        sdsin.close();
    }
    catch(...) {
        sdsin.close();
        err = 1;
        return false;
    }

    return true;
}

#if 0
Grid *NewGrid(const string & n)
{
    return new HDFGrid(n);
}
#endif

// $Log: HDFGrid.cc,v $
// Revision 1.12.4.2  2003/09/06 23:33:14  jimg
// I modified the read() method implementations so that they test the new
// in_selection property. If it is true, the methods will read values
// even if the send_p property is not true. This is so that variables used
// in the selection part of the CE, or as function arguments, will be read.
// See bug 657.
//
// Revision 1.12.4.1  2003/05/21 16:26:51  edavis
// Updated/corrected copyright statements.
//
// Revision 1.12  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.10.4.8  2002/04/12 00:07:04  jimg
// I removed old code that was wrapped in #if 0 ... #endif guards.
//
// Revision 1.10.4.7  2002/04/12 00:03:14  jimg
// Fixed casts that appear throughout the code. I changed most/all of the
// casts to the new-style syntax. I also removed casts that we're not needed.
//
// Revision 1.10.4.6  2002/04/10 18:38:10  jimg
// I modified the server so that it knows about, and uses, all the DODS
// numeric datatypes. Previously the server cast 32 bit floats to 64 bits and
// cast most integer data to 32 bits. Now if an HDF file contains these
// datatypes (32 bit floats, 16 bit ints, et c.) the server returns data
// using those types (which DODS has supported for a while...).
//
// Revision 1.10.4.5  2002/03/14 19:15:07  jimg
// Fixed use of int err in read() so that it's always initialized to zero.
// This is a fix for bug 135.
//
// Revision 1.10.4.4  2002/02/05 17:29:32  jimg
// Added a new method (get_map_constraints()) that extracts the constraints
// placed on map vectors and loads them into a vector<array_ce> object which can
// then be assigned to a hdfistream_sds object.
// I change hdfistream_sds so that it can hold a vector<array_ce> object. The
// operator>>(hdf_dim&) method now uses this new object to correctly set the
// hdfistream_sds::_slab member when maps are requested but the array is not.
// Currently 17 tests (run make check after installing the server and the test
// datasets) fail.
//
// Revision 1.10.4.3  2002/02/02 00:11:37  dan
// Updated read_p flag for map vectors.
//
// Revision 1.10.4.2  2002/02/01 23:53:17  dan
// test code in read_tagref
//
// Revision 1.11  2001/08/27 17:21:34  jimg
// Merged with version 3.2.2
//
// Revision 1.10.4.1  2001/07/28 00:25:15  jimg
// I removed the code which escapes names. This function is now handled
// for all the servers by the dap.
//
// Revision 1.10  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.9  2000/03/31 16:56:06  jimg
// Merged with release 3.1.4
//
// Revision 1.8.8.1  2000/03/20 22:26:52  jimg
// Switched to the id2dods, etc. escaping function in the dap.
//
// Revision 1.8  1999/05/06 03:23:34  jimg
// Merged changes from no-gnu branch
//
// Revision 1.7.6.1  1999/05/06 00:27:21  jimg
// Jakes String --> string changes
//
// Revision 1.5  1998/04/06 16:08:18  jimg
// Patch from Jake Hamby; change from switch to Mixin class for read_ref()
//
// Revision 1.4  1998/04/03 18:34:22  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.3  1997/03/10 22:45:25  jimg
// Update for 2.12
//
// Revision 1.4  1996/09/24 20:53:26  todd
// Added copyright and header.
