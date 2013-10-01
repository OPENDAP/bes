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
//#define DODS_DEBUG 1

#include <vector>

// Include this on linux to suppres an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>

#include <hdfclass.h>
#include <hcstream.h>

#include <escaping.h>
#include <Error.h>
#include <debug.h>
#include <BESDebug.h>

#include "HDFArray.h"
#include "dhdferr.h"

HDFArray::HDFArray(const string &n, const string &d, BaseType * v) :
	Array(n, d, v) {
}

HDFArray::~HDFArray() {
}

BaseType *HDFArray::ptr_duplicate() {
	return new HDFArray(*this);
}
void LoadArrayFromSDS(HDFArray * ar, const hdf_sds & sds);
void LoadArrayFromGR(HDFArray * ar, const hdf_gri & gr);

// Read in an Array from either an SDS or a GR in an HDF file.
bool HDFArray::read() {
	int err = 0;
	int status = read_tagref(-1, -1, err);

	if (err)
		throw Error(unknown_error, "Could not read from dataset.");

	return status;
}

// todo: refactor: get rid of the err value-result parameter; throw from
// within this method.
bool HDFArray::read_tagref(int32 tag, int32 ref, int &err) {
	if (read_p())
		return true;

	// get the HDF dataset name, SDS name
	string hdf_file = dataset();
	string hdf_name = this->name();

	// get slab constraint
	vector<int> start, edge, stride;
	bool isslab = GetSlabConstraint(start, edge, stride);

	bool foundsds = false;
	hdf_sds sds;
	if (tag == -1 || tag == DFTAG_NDG) {
		if (SDSExists(hdf_file.c_str(), hdf_name.c_str())) {
			hdfistream_sds sdsin(hdf_file.c_str());
			if (ref != -1) {
				BESDEBUG("h4", "sds seek with ref = " << ref << endl);
				sdsin.seek_ref(ref);
			} else {
				BESDEBUG("h4", "sds seek with name = '" << hdf_name << "'" << endl);
				sdsin.seek(hdf_name.c_str());
			}
			if (isslab)
				sdsin.setslab(start, edge, stride, false);
			sdsin >> sds;
			sdsin.close();
			foundsds = true;
		}
	}

	bool foundgr = false;
	hdf_gri gr;
	if (!foundsds && (tag == -1 || tag == DFTAG_VG)) {
		if (GRExists(hdf_file.c_str(), hdf_name.c_str())) {
			hdfistream_gri grin(hdf_file.c_str());
			if (ref != -1)
				grin.seek_ref(ref);
			else
				grin.seek(hdf_name.c_str());
			if (isslab)
				grin.setslab(start, edge, stride, false);
			grin >> gr;
			grin.close();
			foundgr = true;
		}
	}

	// Todo: refactor: move this stuff up into the above if stmts.
	if (foundsds)
		LoadArrayFromSDS(this, sds);
	else if (foundgr)
		LoadArrayFromGR(this, gr);

	if (foundgr || foundsds) {
		set_read_p(true); // Moved here; see bug 136
		err = 0; // no error
		return true;
	} else {
		err = 1;
		return false;
	}
}

// Read the slab constraint parameters; the arrays start_array, edge_array,
// stride_array.  Returns true if there is a slab constraint, false otherwise.
bool HDFArray::GetSlabConstraint(vector<int>&start_array,
		vector<int>&edge_array, vector<int>&stride_array) {
	int start = 0, stop = 0, stride = 0;
	int edge = 0;

	start_array = vector<int> (0);
	edge_array = vector<int> (0);
	stride_array = vector<int> (0);

	for (Array::Dim_iter p = dim_begin(); p != dim_end(); ++p) {
		start = dimension_start(p, true);
		stride = dimension_stride(p, true);
		stop = dimension_stop(p, true);
		if (start == 0 && stop == 0 && stride == 0)
			return false; // no slab constraint
		if (start > stop)
			THROW(dhdferr_arrcons);
		edge = (int) ((stop - start) / stride) + 1;
		if (start + edge > dimension_size(p))
			THROW(dhdferr_arrcons);

		start_array.push_back(start);
		edge_array.push_back(edge);
		stride_array.push_back(stride);
	}
	return true;
}

/**
 * Transfer attributes from a separately built DAS to the DDS. This method
 * overrides the implementation found in libdap to accommodate the special
 * characteristics of the HDF4 handler's DAS object. The notworthy feature
 * of this handler's DAS is that it lacks the specified structure that
 * provides an easy way to match DAS and DDS items. Instead the DAS is
 * flat.
 *
 * This version of the method first calls the libdap implementation which,
 * in turn, looks for attribtues that match the name of the variable exactly.
 * Then it looks for 'dimension' attributes that should be bound to this
 * array by searching for attribtue containers whose names fit the pattern
 * <var>_dim_<digit>, where <var> is the name of this variable and <digit> is
 * some interger, usually small.
 *
 * @param at An AttrTable for the entire DAS. Search this for attribtues
 * by name.
 * @see HDFSequence::transfer_attributes
 * @see HDFGrid::transfer_attributes
 * @see HDFStructure::transfer_attributes
 */
void HDFArray::transfer_attributes(AttrTable *at) {
	BESDEBUG("h4","Transferring attributes for " << name() << endl);

	BaseType::transfer_attributes(at);

	BESDEBUG("h4","...Now looking for the " << name() << " _dim_n containers." << endl);

	// Here we should look for the *_dim_n where '*' is name() and n is 0, 1, ...
	string dim_name_base = name() + "_dim_";

	AttrTable::Attr_iter a_p = at->attr_begin();
	while (a_p != at->attr_end()) {
		string::size_type i = at->get_name(a_p).find(dim_name_base);
		// Found a matching container?
		// To avoid matching both Month_dim_0 and  DayOfMonth_dim_0, et c.,
		// check that i == 0 and not just i != string::npos. jhrg 8/17/11
		if (i == 0 && at->get_attr_type(a_p) == Attr_container) {
			AttrTable *dim = at->get_attr_table(a_p);
			try {
				BESDEBUG("h4","Found a dimension container for " << name() << endl);
				transfer_dimension_attribute(dim);
			}
			catch (Error &e) {
				BESDEBUG("h4","Caught an error transferring dimension attribute " << dim->get_name() << " for variable " << name() << endl);
				throw e;
			}
		}

		a_p++;
	}
}

void HDFArray::transfer_dimension_attribute(AttrTable *dim) {
	// Mark the table as not global
	dim->set_is_global_attribute(false);
	// copy the table
	AttrTable *at = new AttrTable(*dim);
	// add it to this variable using just the 'dim_<digit>' part of the name
	string name = at->get_name().substr(at->get_name().find("dim"));
	get_attr_table().append_container(at, name);
}

