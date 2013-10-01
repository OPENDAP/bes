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

#include <vector>
// Include this on linux to suppres an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>

#include <Error.h>
#include <InternalErr.h>
#include <debug.h>
#include <escaping.h>

#include <BESDebug.h>

#include "HDFGrid.h"
#include "HDFArray.h"
#include "hdfutil.h"

HDFGrid::HDFGrid(const string &n, const string &d) :
	Grid(n, d) {
}

HDFGrid::~HDFGrid() {
}
BaseType *HDFGrid::ptr_duplicate() {
	return new HDFGrid(*this);
}

void LoadGridFromSDS(HDFGrid * gr, const hdf_sds & sds);

// Build a vector of array_ce structs. This holds the constraint
// information *for each map* of the Grid.
vector<array_ce> HDFGrid::get_map_constraints() {
	vector<array_ce> a_ce_vec;

	// Load the array_ce vector with info about each map vector.
	for (Grid::Map_iter p = map_begin(); p != map_end(); ++p) {
		Array & a = static_cast<Array &> (**p);
		Array::Dim_iter q = a.dim_begin(); // maps have only one dimension.
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
bool HDFGrid::read() {
	int err = 0;
	int status = read_tagref(-1, -1, err);
	if (err)
		throw Error(unknown_error, "Could not read from dataset.");
	return status;
}

bool HDFGrid::read_tagref(int32, int32 ref, int &err) {
	if (read_p())
		return true;

	err = 0; // OK initially

	string hdf_file = dataset();
	string hdf_name = this->name();

	hdf_sds sds;

	// read in SDS
	hdfistream_sds sdsin(hdf_file.c_str());
	try {
		vector<int> start, edge, stride;
		HDFArray *primary_array = static_cast<HDFArray *> (array_var());
		if (!primary_array)
			throw InternalErr(__FILE__, __LINE__, "Expected an HDFArray.");

		bool isslab = primary_array->GetSlabConstraint(start, edge, stride);

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
				throw Error(
						string("Could not read ") + array_var()->name()
								+ string(" from dataset ") + dataset()
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
			sds.dims = vector<hdf_dim> ();
			sds.data = hdf_genvec(); // needed?
			//      sds.ref = SDidtoref(_sds_id);
			sdsin >> sds.dims;
		}

		for (Grid::Map_iter p = map_begin(); p != map_end(); ++p) {
			if ((*p)->send_p() || (*p)->is_in_selection()) {
				for (unsigned int i = 0; i < sds.dims.size(); i++) {
					if ((*p)->name() == sds.dims[i].name) {
						// Read the data from the sds dimension.
						char *data = static_cast<char *> (ExportDataForDODS(
								sds.dims[i].scale));
						(*p)->val2buf(data);
						delete[] data;
						(*p)->set_read_p(true);
					}
				}
			}
		}

		sdsin.close();
	} catch (...) {
		sdsin.close();
		err = 1;
		return false;
	}

	return true;
}

void HDFGrid::transfer_attributes(AttrTable *at) {
	if (at) {
		array_var()->transfer_attributes(at);

		Map_iter map = map_begin();
		while (map != map_end()) {
			(*map)->transfer_attributes(at);
			map++;
		}

		AttrTable *mine = at->get_attr_table(name());

		if (mine) {
			mine->set_is_global_attribute(false);
			AttrTable::Attr_iter at_p = mine->attr_begin();
			while (at_p != mine->attr_end()) {
				if (mine->get_attr_type(at_p) == Attr_container)
					get_attr_table().append_container(
							new AttrTable(*mine->get_attr_table(at_p)),
							mine->get_name(at_p));
				else
					get_attr_table().append_attr(mine->get_name(at_p),
							mine->get_type(at_p), mine->get_attr_vector(at_p));
				at_p++;
			}
		}

		// Now look for those pesky <var>_dim_<digit> attributes

		string dim_name_base = name() + "_dim_";

		AttrTable::Attr_iter a_p = at->attr_begin();
		while (a_p != at->attr_end()) {
			string::size_type i = at->get_name(a_p).find(dim_name_base);
			// Found a matching container?
			// See comment in HDFArray::transfer_attributes regarding 'i == 0'
			// jhrg 8/17/11
			if (i == 0 && at->get_attr_type(a_p) == Attr_container) {
				AttrTable *dim = at->get_attr_table(a_p);
				// Get the integer from the end of the name and use that as the
				// index to find the matching Map variable.
				BESDEBUG("h4", "dim->name(): " << dim->get_name() << endl);
				BESDEBUG("h4",  "dim->get_name().substr(i + dim_name_base.length()): "
						<< dim->get_name().substr(i + dim_name_base.length()) << endl);
				int n = atoi(dim->get_name().substr(i + dim_name_base.length()).c_str());
				// Note that the maps are HDFArray instances, so we use that
				// for the actual copy operation.
				BESDEBUG("h4",  "Inside HDFGrid::transfer_attreibutes: n = " << n << endl);
				static_cast<HDFArray&> (*(*(map_begin() + n))).transfer_dimension_attribute(dim);
			}

			a_p++;
		}
	}
}

