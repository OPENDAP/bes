/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the missing fields for some special NASA HDF4 data products.
// The products include TRMML2,TRMML3,CER_AVG,CER_ES4,CER_CDAY,CER_CGEO,CER_SRB,CER_SYN,CER_ZAVG,OBPGL2,OBPGL3
// To know more information about these products,check HDFSP.h.
// Some third-dimension coordinate variable values are not provided.
// What we do here is to provide natural number series(1,2,3, ...) for
// these missing values. It doesn't make sense to visualize or analyze
// with vertical cross-section. One can check the data level by level.

//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFSPArrayMissField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "mfhdf.h"
#include "hdf.h"
#include "InternalErr.h"


bool
HDFSPArrayMissGeoField::read ()
{


	int *offset = new int[rank];
	int *count = new int[rank];
	int *step = new int[rank];

	int nelms;

	try {
		nelms = format_constraint (offset, step, count);
	}
	catch (...) {
		delete[]offset;
		delete[]step;
		delete[]count;
		throw;
	}

	// Since we always assign the the missing Z dimension as 32-bit integer, so need
	// to check the type.
	int *val = new int[nelms];

	if (nelms == tnumelm) {
		for (int i = 0; i < nelms; i++)
			val[i] = i;
		set_value ((dods_int32 *) val, nelms);
	}
	else {
		if (rank != 1) {
			delete[]val;
			delete[]offset;
			delete[]step;
			delete[]count;
			InternalErr (__FILE__, __LINE__,
						 "Currently the rank of the missing field should be 1");
		}
		for (int i = 0; i < count[0]; i++)
			val[i] = offset[0] + step[0] * i;
		set_value ((dods_int32 *) val, nelms);
	}

	delete[]val;
	delete[]offset;
	delete[]count;
	delete[]step;

	return false;
}

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDFSPArrayMissGeoField::format_constraint (int *offset, int *step, int *count)
{
	long nels = 1;
	int id = 0;

	Dim_iter p = dim_begin ();

	while (p != dim_end ()) {

		int start = dimension_start (p, true);
		int stride = dimension_stride (p, true);
		int stop = dimension_stop (p, true);


		// Check for illegical  constraint
		if (stride < 0 || start < 0 || stop < 0 || start > stop) {
			ostringstream oss;

			oss << "Array/Grid hyperslab indices are bad: [" << start <<
				":" << stride << ":" << stop << "]";
			throw Error (malformed_expr, oss.str ());
		}

		// Check for an empty constraint and use the whole dimension if so.
		if (start == 0 && stop == 0 && stride == 0) {
			start = dimension_start (p, false);
			stride = dimension_stride (p, false);
			stop = dimension_stop (p, false);
		}

		offset[id] = start;
		step[id] = stride;
		count[id] = ((stop - start) / stride) + 1;	// count of elements
		nels *= count[id];		// total number of values for variable

		DBG (cerr
			 << "=format_constraint():"
			 << "id=" << id << " offset=" << offset[id]
			 << " step=" << step[id]
			 << " count=" << count[id]
			 << endl);

		id++;
		p++;
	}

	return nels;
}
