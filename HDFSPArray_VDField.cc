/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the Vdata fields from NASA HDF4 data products.
// Each Vdata will be decomposed into individual Vdata fields.
// Each field will be mapped to A DAP variable.

//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFSPArray_VDField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "hdf.h"
#include "mfhdf.h"
#include "InternalErr.h"
#include "HDFSPUtil.h"
#define SIGNED_BYTE_TO_INT32 1


bool
HDFSPArray_VDField::read ()
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


	int32 file_id, vdata_id;

	// Open the file
	file_id = Hopen (filename.c_str (), DFACC_READ, 0);
	if (file_id < 0) {
		HDFSPUtil::ClearMem3 (offset, count, step);
		ostringstream eherr;

		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	// Start the Vdata interface
	if (Vstart (file_id) < 0) {
		Hclose (file_id);
		HDFSPUtil::ClearMem3 (offset, count, step);
		ostringstream eherr;

		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	// Attach the vdata
	vdata_id = VSattach (file_id, vdref, "r");
	if (vdata_id == -1) {
		Vend (file_id);
		Hclose (file_id);
		HDFSPUtil::ClearMem3 (offset, count, step);
		ostringstream eherr;

		eherr << "Vdata cannot be attached.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	int32 r;

	// Seek the position of the starting point
	if (VSseek (vdata_id, (int32) offset[0]) == -1) {
		VSdetach (vdata_id);
		Vend (file_id);
		Hclose (file_id);
		HDFSPUtil::ClearMem3 (offset, count, step);
		ostringstream eherr;

		eherr << "VSseek failed at " << offset[0];
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	// Prepare the vdata field
	if (VSsetfields (vdata_id, fdname.c_str ()) == -1) {
		VSdetach (vdata_id);
		Vend (file_id);
		Hclose (file_id);
		HDFSPUtil::ClearMem3 (offset, count, step);
		ostringstream eherr;

		eherr << "VSsetfields failed with the name " << fdname;
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	int32 vdfelms = fdorder * count[0] * step[0];

	// Loop through each data type
	switch (dtype) {
	case DFNT_INT8:
		{
			int8 *val = new int8[nelms];
			int8 *orival = new int8[vdfelms];

			// Read the data
			r = VSread (vdata_id, (uint8 *) orival, count[0] * step[0],
						FULL_INTERLACE);

			if (r == -1) {
				VSdetach (vdata_id);
				Vend (file_id);
				Hclose (file_id);
				HDFSPUtil::ClearMem3 (offset, count, step);
				delete[]val;
				delete[]orival;
				ostringstream eherr;

				eherr << "VSread failed.";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

			// Obtain the subset portion of the data
			if (fdorder > 1) {
				for (int i = 0; i < count[0]; i++)
					for (int j = 0; j < count[1]; j++)
						val[i * count[1] + j] =
							orival[i * fdorder * step[0] + offset[1] +
								   j * step[1]];
			}
			else {
				for (int i = 0; i < count[0]; i++)
					val[i] = orival[i * step[0]];
			}


#ifndef SIGNED_BYTE_TO_INT32
			set_value ((dods_byte *) val, nelms);
			delete[]val;
			delete[]orival;
#else
			int32 *newval;
			int8 *newval8;

			newval = new int32[nelms];
			newval8 = (int8 *) val;
			for (int counter = 0; counter < nelms; counter++)
				newval[counter] = (int32) (newval8[counter]);

			set_value ((dods_int32 *) newval, nelms);

			delete[]val;
			delete[]newval;
			delete[]orival;
#endif
		}

		break;
	case DFNT_UINT8:
	case DFNT_UCHAR8:
	case DFNT_CHAR8:
		{
			uint8 *val = new uint8[nelms];
			uint8 *orival = new uint8[vdfelms];

			r = VSread (vdata_id, orival, count[0] * step[0], FULL_INTERLACE);
			if (r == -1) {
				VSdetach (vdata_id);
				Vend (file_id);
				Hclose (file_id);
				HDFSPUtil::ClearMem3 (offset, count, step);
				delete[]val;
				delete[]orival;
				ostringstream eherr;

				eherr << "VSread failed.";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

			if (fdorder > 1) {
				for (int i = 0; i < count[0]; i++)
					for (int j = 0; j < count[1]; j++)
						val[i * count[1] + j] =
							orival[i * fdorder * step[0] + offset[1] +
								   j * step[1]];
			}
			else {
				for (int i = 0; i < count[0]; i++)
					val[i] = orival[i * step[0]];
			}

			set_value ((dods_byte *) val, nelms);
			delete[]val;
			delete[]orival;
		}

		break;

	case DFNT_INT16:
		{
			int16 *val = new int16[nelms];
			int16 *orival = new int16[vdfelms];

			r = VSread (vdata_id, (uint8 *) orival, count[0] * step[0],
						FULL_INTERLACE);
			if (r == -1) {
				VSdetach (vdata_id);

				Vend (file_id);
				Hclose (file_id);
				HDFSPUtil::ClearMem3 (offset, count, step);
				delete[]val;
				delete[]orival;

				ostringstream eherr;

				eherr << "VSread failed.";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

			if (fdorder > 1) {
				for (int i = 0; i < count[0]; i++)
					for (int j = 0; j < count[1]; j++)
						val[i * count[1] + j] =
							orival[i * fdorder * step[0] + offset[1] +
								   j * step[1]];
			}
			else {
				for (int i = 0; i < count[0]; i++)
					val[i] = orival[i * step[0]];
			}

			set_value ((dods_int16 *) val, nelms);
			delete[]val;
			delete[]orival;
		}
		break;

	case DFNT_UINT16:

		{
			uint16 *val = new uint16[nelms];
			uint16 *orival = new uint16[vdfelms];

			r = VSread (vdata_id, (uint8 *) orival, count[0] * step[0],
						FULL_INTERLACE);
			if (r == -1) {

				VSdetach (vdata_id);
				Vend (file_id);
				Hclose (file_id);
				HDFSPUtil::ClearMem3 (offset, count, step);
				delete[]val;
				delete[]orival;

				ostringstream eherr;

				eherr << "VSread failed.";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

			if (fdorder > 1) {
				for (int i = 0; i < count[0]; i++)
					for (int j = 0; j < count[1]; j++)
						val[i * count[1] + j] =
							orival[i * fdorder * step[0] + offset[1] +
								   j * step[1]];
			}
			else {
				for (int i = 0; i < count[0]; i++)
					val[i] = orival[i * step[0]];
			}

			set_value ((dods_uint16 *) val, nelms);
			delete[]val;
			delete[]orival;
		}

		break;
	case DFNT_INT32:
		{
			int32 *val = new int32[nelms];
			int32 *orival = new int32[vdfelms];

			r = VSread (vdata_id, (uint8 *) orival, count[0] * step[0],
						FULL_INTERLACE);
			if (r == -1) {

				VSdetach (vdata_id);
				Vend (file_id);
				Hclose (file_id);
				HDFSPUtil::ClearMem3 (offset, count, step);
				delete[]val;
				delete[]orival;

				ostringstream eherr;

				eherr << "VSread failed.";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

			if (fdorder > 1) {
				for (int i = 0; i < count[0]; i++)
					for (int j = 0; j < count[1]; j++)
						val[i * count[1] + j] =
							orival[i * fdorder * step[0] + offset[1] +
								   j * step[1]];
			}
			else {
				for (int i = 0; i < count[0]; i++)
					val[i] = orival[i * step[0]];
			}

			set_value ((dods_int32 *) val, nelms);
			delete[]val;
			delete[]orival;
		}
		break;

	case DFNT_UINT32:

		{
			uint32 *val = new uint32[nelms];
			uint32 *orival = new uint32[vdfelms];

			r = VSread (vdata_id, (uint8 *) orival, count[0] * step[0],
						FULL_INTERLACE);
			if (r == -1) {

				VSdetach (vdata_id);
				Vend (file_id);
				Hclose (file_id);
				HDFSPUtil::ClearMem3 (offset, count, step);
				delete[]val;
				delete[]orival;

				ostringstream eherr;

				eherr << "VSread failed.";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

			if (fdorder > 1) {
				for (int i = 0; i < count[0]; i++)
					for (int j = 0; j < count[1]; j++)
						val[i * count[1] + j] =
							orival[i * fdorder * step[0] + offset[1] +
								   j * step[1]];
			}
			else {
				for (int i = 0; i < count[0]; i++)
					val[i] = orival[i * step[0]];
			}

			set_value ((dods_uint32 *) val, nelms);
			delete[]val;
			delete[]orival;
		}
		break;
	case DFNT_FLOAT32:
		{
			float32 *val = new float32[nelms];
			float32 *orival = new float32[vdfelms];

			r = VSread (vdata_id, (uint8 *) orival, count[0] * step[0],
						FULL_INTERLACE);
			if (r == -1) {

				VSdetach (vdata_id);
				Vend (file_id);
				Hclose (file_id);
				HDFSPUtil::ClearMem3 (offset, count, step);
				delete[]val;
				delete[]orival;

				ostringstream eherr;

				eherr << "VSread failed.";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

			if (fdorder > 1) {
				for (int i = 0; i < count[0]; i++)
					for (int j = 0; j < count[1]; j++)
						val[i * count[1] + j] =
							orival[i * fdorder * step[0] + offset[1] +
								   j * step[1]];
			}
			else {
				for (int i = 0; i < count[0]; i++)
					val[i] = orival[i * step[0]];
			}

			set_value ((dods_float32 *) val, nelms);
			delete[]val;
			delete[]orival;
		}
		break;
	case DFNT_FLOAT64:
		{
			float64 *val = new float64[nelms];
			float64 *orival = new float64[vdfelms];

			r = VSread (vdata_id, (uint8 *) orival, count[0] * step[0],
						FULL_INTERLACE);
			if (r == -1) {

				VSdetach (vdata_id);
				Vend (file_id);
				Hclose (file_id);
				HDFSPUtil::ClearMem3 (offset, count, step);
				delete[]val;
				delete[]orival;

				ostringstream eherr;

				eherr << "VSread failed.";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

			if (fdorder > 1) {
				for (int i = 0; i < count[0]; i++)
					for (int j = 0; j < count[1]; j++)
						val[i * count[1] + j] =
							orival[i * fdorder * step[0] + offset[1] +
								   j * step[1]];
			}
			else {
				for (int i = 0; i < count[0]; i++)
					val[i] = orival[i * step[0]];
			}

			set_value ((dods_float64 *) val, nelms);
			delete[]val;
			delete[]orival;
		}
		break;
	default:
		VSdetach (vdata_id);
		Vend (file_id);
		Hclose (file_id);
		HDFSPUtil::ClearMem3 (offset, count, step);

		InternalErr (__FILE__, __LINE__, "unsupported data type.");
	}

	if (VSdetach (vdata_id) == -1) {
		Vend (file_id);
		Hclose (file_id);
		HDFSPUtil::ClearMem3 (offset, count, step);

		ostringstream eherr;

		eherr << "VSdetach failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	if (Vend (file_id) == -1) {
		Hclose (file_id);
		HDFSPUtil::ClearMem3 (offset, count, step);
		ostringstream eherr;

		eherr << "VSdetach failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	if (Hclose (file_id) == -1) {

		HDFSPUtil::ClearMem3 (offset, count, step);
		ostringstream eherr;

		eherr << "VSdetach failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	delete[]offset;
	delete[]count;
	delete[]step;

	return false;
}

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDFSPArray_VDField::format_constraint (int *offset, int *step, int *count)
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
