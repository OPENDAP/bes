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
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <map>
#include <string>
// Include this on linux to suppres an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>
#include "HDFSequence.h"
#include "HDFStructure.h"
#include "escaping.h"

#include "Error.h"

HDFSequence::HDFSequence(const string &n, const string &d)
    : Sequence(n, d), row(0)
{
}

HDFSequence::~HDFSequence()
{
}

BaseType *HDFSequence::ptr_duplicate()
{
    return new HDFSequence(*this);
}

void LoadSequenceFromVdata(HDFSequence * seq, hdf_vdata & vd, int row);

bool HDFSequence::read()
{
    int err = 0;
    int status = read_tagref(-1, -1, err);
    if (err)
        throw Error(unknown_error, "Could not read from dataset.");
    return status;
}

bool HDFSequence::read_tagref(int32 /*tag*/, int32 ref, int &err)
{
    string hdf_file = dataset();
    string hdf_name = this->name();

    // check to see if vd is empty; if so, read in Vdata
    if (vd.name.length() == 0) {
        hdfistream_vdata vin(hdf_file.c_str());
        if (ref != -1)
            vin.seek_ref(ref);
        else
            vin.seek(hdf_name.c_str());
        vin >> vd;
        vin.close();
        if (!vd) {              // something is wrong
            err = 1;            // indicate error
            return false;
        }
    }
    // Return false when no more data are left to be read. Note that error is
    // also false (i.e., no error occurred). 02/06/98 jhrg
    if (row >= vd.fields[0].vals[0].size()) {
        set_read_p(true);
        err = 0;                // everything is OK
        return true;            // Indicate EOF
    }
    // is this an empty Vdata.
    // I'm not sure that it should be an error to read from an empty vdata.
    // It maybe that valid files have empty vdatas when they are first
    // created. 02/06/98 jhrg
    if (vd.fields.size() <= 0 || vd.fields[0].vals.size() <= 0) {
        err = 1;
        return false;
    }

    LoadSequenceFromVdata(this, vd, row++);

    set_read_p(true);
    err = 0;                    // everything is OK

    return false;
}

void HDFSequence::transfer_attributes(AttrTable *at)
{
    if (at) {
	Vars_iter var = var_begin();
	while (var != var_end()) {
	    (*var)->transfer_attributes(at);
	    var++;
	}

	AttrTable *mine = at->get_attr_table(name());

	if (mine) {
	    mine->set_is_global_attribute(false);
	    AttrTable::Attr_iter at_p = mine->attr_begin();
	    while (at_p != mine->attr_end()) {
		if (mine->get_attr_type(at_p) == Attr_container)
		    get_attr_table().append_container(new AttrTable(
			    *mine->get_attr_table(at_p)), mine->get_name(at_p));
		else
		    get_attr_table().append_attr(mine->get_name(at_p),
			    mine->get_type(at_p), mine->get_attr_vector(at_p));
		at_p++;
	    }
	}
    }
}


