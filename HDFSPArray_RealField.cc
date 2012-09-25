/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the real SDS field values for special HDF4 data products.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  Eunsoo Seo
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#include "HDFSPArray_RealField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "hdf.h"
#include "mfhdf.h"
#include "InternalErr.h"
#include <BESDebug.h>
#include "BESInternalError.h"
#include "HDFCFUtil.h"
#define SIGNED_BYTE_TO_INT32 1


bool
HDFSPArray_RealField::read ()
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

	//Obtain SDS IDs. 
	int32 sdid = 0;
        int32 sdsid = 0;

	sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);

	if (sdid < 0) {
		HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
		ostringstream eherr;
		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	int32 sdsindex = SDreftoindex (sdid, (int32) sdsref);

	if (sdsindex == -1) {
		HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
		SDend (sdid);
		ostringstream eherr;

		eherr << "SDS index " << sdsindex << " is not right.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	sdsid = SDselect (sdid, sdsindex);
	if (sdsid < 0) {
		SDend (sdid);
		HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
		ostringstream eherr;

		eherr << "SDselect failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	void *val = NULL;
	int32 r = 0;

	switch (dtype) {

	case DFNT_INT8:
		{
			val = new int8[nelms];
			r = SDreaddata (sdsid, offset32, step32, count32, val);
			if (r != 0) {
				SDendaccess (sdsid);
				SDend (sdid);
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				delete[](int8 *) val;
				ostringstream eherr;

				eherr << "SDreaddata failed.";
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
		r = SDreaddata (sdsid, offset32, step32, count32, val);
		if (r != 0) {
			SDendaccess (sdsid);
			SDend (sdid);
			HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
								 step);
			delete[](uint8 *) val;
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_byte *) val, nelms);
		delete[](uint8 *) val;
		break;

	case DFNT_INT16:
		val = new int16[nelms];
		r = SDreaddata (sdsid, offset32, step32, count32, val);
		if (r != 0) {
			SDendaccess (sdsid);
			SDend (sdid);
			HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
								 step);
			delete[](int16 *) val;
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		// WILL HANDLE this later if necessary. KY 2010-8-17
		//if(sptype == TRMML2) 
		// Find scale_factor, add_offset attributes
		// create a new val to float32, remember to change int16 at HDFSP.cc to float32
		// if(sptype == OBPGL3) 16-bit unsigned integer 
		// Find slope and intercept, 
		// CZCS: also find base. data = base**((slope*l3m_data)+intercept)
		// Others : slope*data+intercept
		// OBPGL2: 16-bit signed integer, (8-bit signed integer or 32-bit signed integer)
		// If slope = 1 and intercept = 0 no need to do conversion

		set_value ((dods_int16 *) val, nelms);
		delete[](int16 *) val;
		break;

	case DFNT_UINT16:
		val = new uint16[nelms];
		r = SDreaddata (sdsid, offset32, step32, count32, val);
		if (r != 0) {
			SDendaccess (sdsid);
			SDend (sdid);
			HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
								 step);
			delete[](uint16 *) val;
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_uint16 *) val, nelms);
		delete[](uint16 *) val;
		break;
	case DFNT_INT32:
		val = new int32[nelms];
		r = SDreaddata (sdsid, offset32, step32, count32, val);
		if (r != 0) {
			SDendaccess (sdsid);
			SDend (sdid);
			HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
								 step);
			delete[](int32 *) val;
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_int32 *) val, nelms);
		delete[](int32 *) val;
		break;
	case DFNT_UINT32:
		val = new uint32[nelms];
		r = SDreaddata (sdsid, offset32, step32, count32, val);
		if (r != 0) {
			SDendaccess (sdsid);
			SDend (sdid);
			HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
								 step);
			delete[](uint32 *) val;
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}
		set_value ((dods_uint32 *) val, nelms);
		delete[](uint32 *) val;
		break;
	case DFNT_FLOAT32:
		val = new float32[nelms];
		r = SDreaddata (sdsid, offset32, step32, count32, val);
		if (r != 0) {
			SDendaccess (sdsid);
			SDend (sdid);
			HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
								 step);
			delete[](float32 *) val;
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_float32 *) val, nelms);
		delete[](float32 *) val;
		break;
	case DFNT_FLOAT64:
		val = new float64[nelms];
		r = SDreaddata (sdsid, offset32, step32, count32, val);
		if (r != 0) {
			SDendaccess (sdsid);
			SDend (sdid);
			HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
								 step);
			delete[](float64 *) val;
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_float64 *) val, nelms);
		delete[](float64 *) val;
		break;
	default:
		SDendaccess (sdsid);
		SDend (sdid);
		HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
		InternalErr (__FILE__, __LINE__, "unsupported data type.");
	}

	r = SDendaccess (sdsid);
	if (r != 0) {
		SDend (sdid);
		HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
		ostringstream eherr;

		eherr << "SDendaccess failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	r = SDend (sdid);
	if (r != 0) {

		HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
		ostringstream eherr;

		eherr << "SDend failed.";
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
HDFSPArray_RealField::format_constraint (int *offset, int *step, int *count)
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
