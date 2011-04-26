/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath with dimension map
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

// Currently the handling of swath data fields with dimension maps is the same as other data fields(HDFEOS2Array_RealField.cc etc)
// The reason to keep it in separate is, in theory, that data fields with dimension map may need special handlings.
// So we will leave it here for this release(2010-8), it may be removed in the future. HDFEOS2Array_RealField.cc may be used.

#include "HDFEOS2ArraySwathDimMapField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "InternalErr.h"
#include "BESDebug.h"
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"
#include "HDFEOS2Util.h"
#define SIGNED_BYTE_TO_INT32 1


bool
HDFEOS2ArraySwathDimMapField::read ()
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

	int32 *offset32 = new int32[rank];
	int32 *count32 = new int32[rank];
	int32 *step32 = new int32[rank];

	for (int i = 0; i < rank; i++) {
		offset32[i] = (int32) offset[i];
		count32[i] = (int32) count[i];
		step32[i] = (int32) step[i];
	}

	int32 (*openfunc) (char *, intn);
	intn (*closefunc) (int32);
	int32 (*attachfunc) (int32, char *);
	intn (*detachfunc) (int32);
	intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);
	intn (*readfieldfunc) (int32, char *, int32 *, int32 *, int32 *, void *);

	std::string datasetname;
	if (swathname == "") {
		openfunc = GDopen;
		closefunc = GDclose;
		attachfunc = GDattach;
		detachfunc = GDdetach;
		fieldinfofunc = GDfieldinfo;
		readfieldfunc = GDreadfield;
		datasetname = gridname;
	}
	else if (gridname == "") {
		openfunc = SWopen;
		closefunc = SWclose;
		attachfunc = SWattach;
		detachfunc = SWdetach;
		fieldinfofunc = SWfieldinfo;
		readfieldfunc = SWreadfield;
		datasetname = swathname;
	}
	else {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		throw InternalErr (__FILE__, __LINE__,
						   "It should be either grid or swath.");
	}

	// Swath ID, swathid is actually in this case only the id of latitude and longitude.
	int32 sfid, swathid;

	// Open, attach and obtain datatype information based on HDF-EOS2 APIs.
	sfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);

	if (sfid < 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	swathid = attachfunc (sfid, const_cast < char *>(datasetname.c_str ()));

	if (swathid < 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "Grid/Swath " << datasetname.
			c_str () << " cannot be attached.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	int32 tmp_rank, tmp_dims[rank];
	char tmp_dimlist[1024];
	int32 type;
	intn r;
	r = fieldinfofunc (swathid, const_cast < char *>(fieldname.c_str ()),
					   &tmp_rank, tmp_dims, &type, tmp_dimlist);
	if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "Field " << fieldname.
			c_str () << " information cannot be obtained.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	void *val;

	// Loop through the data type. 
	switch (type) {

	case DFNT_INT8:
		{
			val = new int8[nelms];
			r = readfieldfunc (swathid,
							   const_cast < char *>(fieldname.c_str ()),
							   offset32, step32, count32, val);
			if (r != 0) {
				HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
									   count, step);
				ostringstream eherr;

				eherr << "field " << fieldname.c_str () << "cannot be read.";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

#ifndef SIGNED_BYTE_TO_INT32
			set_value ((dods_byte *) val, nelms);
			delete[](int8 *) val;
#else
			int32 *newval;
			int8 *newval8;

			newval = new int32[nelms];
			newval8 = (int8 *) val;
			for (int counter = 0; counter < nelms; counter++)
				newval[counter] = (int32) (newval8[counter]);

			set_value ((dods_int32 *) newval, nelms);
			delete[](int8 *) val;
			delete[]newval;
#endif
		}

		break;
	case DFNT_UINT8:
	case DFNT_UCHAR8:
	case DFNT_CHAR8:
		val = new uint8[nelms];
		r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()),
						   offset32, step32, count32, val);
		if (r != 0) {
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
			ostringstream eherr;

			eherr << "field " << fieldname.c_str () << "cannot be read.";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_byte *) val, nelms);
		delete[](uint8 *) val;
		break;

	case DFNT_INT16:
		val = new int16[nelms];
		r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()),
						   offset32, step32, count32, val);

		if (r != 0) {
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
			ostringstream eherr;

			eherr << "field " << fieldname.c_str () << "cannot be read.";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_int16 *) val, nelms);
		delete[](int16 *) val;
		break;
	case DFNT_UINT16:
		val = new uint16[nelms];
		r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()),
						   offset32, step32, count32, val);
		if (r != 0) {
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
			ostringstream eherr;

			eherr << "field " << fieldname.c_str () << "cannot be read.";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_uint16 *) val, nelms);
		delete[](uint16 *) val;
		break;
	case DFNT_INT32:
		val = new int32[nelms];
		r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()),
						   offset32, step32, count32, val);
		if (r != 0) {
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
			ostringstream eherr;

			eherr << "field " << fieldname.c_str () << "cannot be read.";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_int32 *) val, nelms);
		delete[](int32 *) val;
		break;
	case DFNT_UINT32:
		val = new uint32[nelms];
		r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()),
						   offset32, step32, count32, val);
		if (r != 0) {
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
			ostringstream eherr;

			eherr << "field " << fieldname.c_str () << "cannot be read.";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_uint32 *) val, nelms);
		delete[](uint32 *) val;
		break;
	case DFNT_FLOAT32:
		val = new float32[nelms];
		r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()),
						   offset32, step32, count32, val);
		if (r != 0) {
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
			ostringstream eherr;

			eherr << "field " << fieldname.c_str () << "cannot be read.";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_float32 *) val, nelms);
		delete[](float32 *) val;
		break;
	case DFNT_FLOAT64:
		val = new float64[nelms];
		r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()),
						   offset32, step32, count32, val);
		if (r != 0) {
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
			ostringstream eherr;

			eherr << "field " << fieldname.c_str () << "cannot be read.";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}
		set_value ((dods_float64 *) val, nelms);
		delete[](float64 *) val;
		break;
	default:
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		InternalErr (__FILE__, __LINE__, "unsupported data type.");
	}

	r = detachfunc (swathid);
	if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "Grid/Swath " << datasetname.
			c_str () << " cannot be detached.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	r = closefunc (sfid);
	if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "Grid/Swath " << filename.c_str () << " cannot be closed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	delete[]offset;
	delete[]count;
	delete[]step;

	delete[]offset32;
	delete[]count32;
	delete[]step32;


	return false;
}

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDFEOS2ArraySwathDimMapField::format_constraint (int *offset, int *step,
												 int *count)
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

                 BESDEBUG ("h4",
                         "=format_constraint():"
                         << "id=" << id << " offset=" << offset[id]
                         << " step=" << step[id]
                         << " count=" << count[id]
                         << endl);

		id++;
		p++;
	}

	return nels;
}
