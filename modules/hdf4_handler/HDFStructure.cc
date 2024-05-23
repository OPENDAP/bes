/////////////////////////////////////////////////////////////////////////////
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
//#define DODS_DEBUG

#include <vector>

// Include this on linux to suppress an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>

#include <libdap/Error.h>
#include <libdap/escaping.h>
#include <libdap/Sequence.h>
#include <libdap/Array.h>
#include <libdap/util.h>
#include <libdap/debug.h>
#include <BESDebug.h>

#include "HDFStructure.h"

using namespace libdap;
using namespace std;

HDFStructure::HDFStructure(const string &n, const string &d) :
	Structure(n, d) {
}

HDFStructure::~HDFStructure() = default;

BaseType *HDFStructure::ptr_duplicate() {
	return new HDFStructure(*this);
}
void LoadStructureFromVgroup(HDFStructure * str, const hdf_vgroup & vgroup,
		const string & hdf_file);

void HDFStructure::set_read_p(bool state) {
	// override Structure::set_read_p() to not set children as read yet
	BaseType::set_read_p(state);
}

bool HDFStructure::read_from_value(vector<uint8_t> &values, size_t &values_offset) {

#if 0
    Constructor::Vars_iter vi = this->var_begin();
    Constructor::Vars_iter ve = this->var_end();

    for (; vi != ve; vi++) {
#endif

    for (auto &bt:this->variables()) {	     
        Type t_bt = bt->type();
        if (libdap::is_simple_type(t_bt) && t_bt != dods_str_c && t_bt != dods_url_c && t_bt!= dods_enum_c && t_bt!=dods_opaque_c) {

#if 0
            if(t_bt == dods_int32_c) {
                //Int32 *val_int = static_cast<Int32 *>(bt);
                //val_int->set_value(*((dods_int32*)(values.data()+values_offset)));
                BESDEBUG("dmrpp", "int value is: " << *((dods_int32*)(values.data()+values_offset)) << "'" << endl);
            }
            else if (t_bt == dods_float32_c) {
                //Float32 *val_float = static_cast<Float32 *>(bt);
                //val_float->set_value(*((dods_float32*)(values.data()+values_offset)));
                BESDEBUG("dmrpp", "float value is: " << *((dods_float32*)(values.data()+values_offset)) << "'" << endl);
            }
            else 
                bt->val2buf(values.data() + values_offset);
#endif

            bt->val2buf(values.data() + values_offset);
            bt->set_read_p(true);
            values_offset += bt->width_ll();

        }
        else if (t_bt == dods_array_c) {

            auto t_a = dynamic_cast<Array *>(bt);
            Type t_array_var = t_a->var()->type();
            if (libdap::is_simple_type(t_array_var) && t_array_var != dods_str_c && t_array_var != dods_url_c && t_array_var!= dods_enum_c && t_array_var!=dods_opaque_c) {
                t_a->val2buf(values.data()+values_offset);
                t_a->set_read_p(true);
                // update values_offset.
                values_offset +=t_a->width_ll();
            }
            else 
                throw InternalErr(__FILE__, __LINE__, "The base type of this structure is not integer or float.  Currently it is not supported.");
        } 
        else 
            throw InternalErr(__FILE__, __LINE__, "The base type of this structure is not integer or float.  Currently it is not supported.");
    }

    return true;
}
bool HDFStructure::read() {
	int err = 0;
	int status = read_tagref(-1, -1, err);
	if (err)
		throw Error(unknown_error, "Could not read from dataset.");
	return status;
}

// TODO: Combine the try/catch block and the following if/then/else and
// eliminate the boolean 'foundvgroup' Consider moving the
// LoadStructureFromVgroup() from hc2dap.cc here since this is the only
// place it's used.
bool HDFStructure::read_tagref(int32 /*tag //unused SBL 2/7/20*/, int32 ref, int &err) {
	if (read_p())
		return true;

	// get the HDF dataset name, Vgroup name
	string hdf_file = dataset();
	string hdf_name = this->name();

    BESDEBUG("h4", " hdf_name = " << hdf_name << endl);

	hdf_vgroup vgroup;

	// Wrap this with a try/catch block but don't do anything with it. The
	// error condition is checked later in this function. pcw 02/19/2008
	try {
		hdfistream_vgroup vgin(hdf_file.c_str());
		if (ref != -1)
			vgin.seek_ref(ref);
		else
			vgin.seek(hdf_name.c_str());
		vgin >> vgroup;
		vgin.close();

		set_read_p(true);

		LoadStructureFromVgroup(this, vgroup, hdf_file);
		return true;
	}
	catch (...) {
		set_read_p(false);
		err = 1;
		return false;
	}
}

/**
 * Transfer attributes from a separately built DAS to the DDS. This method
 * overrides the implementation found in libdap to accommodate the special
 * characteristics of the HDF4 handler's DAS object. The noteworthy feature
 * of this handler's DAS is that it lacks the specified structure that
 * provides an easy way to match DAS and DDS items. Instead this DAS is
 * flat.
 *
 * Because this handler builds a flat attribute object, each variable has to
 * look at the entire top level set of attribute containers to find its own
 * attribute container. If the DAS were built correctly, then this method would
 * find the container for this Structure and pass only that to the child
 * variables for them to search. See the default method in libdap.
 *
 * @param at An AttrTable for the entire DAS. Search this for attributes
 * by name.
 * @see HDFSequence::transfer_attributes
 * @see HDFGrid::transfer_attributes
 * @see HDFArray::transfer_attributes
 */
void HDFStructure::transfer_attributes(AttrTable *at) {

	BESDEBUG("h4",  "Entering HDFStructure::transfer_attributes for variable " << name() << endl);

	if (at) {
		Vars_iter var = var_begin();
		while (var != var_end()) {
			try {
				BESDEBUG("h4", "Processing the attributes for: " << (*var)->name() << " a " << (*var)->type_name() << endl);
				(*var)->transfer_attributes(at);
				var++;
			} catch (Error &e) {
				 BESDEBUG("h4",  "Got this exception: " << e.get_error_message() << endl);
				var++;
				throw e;
			}
		}

		AttrTable *mine = at->get_attr_table(name());

		if (mine) {
			mine->set_is_global_attribute(false);
			AttrTable::Attr_iter at_p = mine->attr_begin();
			while (at_p != mine->attr_end()) {
				if (mine->get_attr_type(at_p) == Attr_container)
					get_attr_table().append_container(new AttrTable(*mine->get_attr_table(at_p)), mine->get_name(at_p));
				else
					get_attr_table().append_attr(mine->get_name(at_p), mine->get_type(at_p), mine->get_attr_vector(at_p));
				at_p++;
			}
		}
	}
}
