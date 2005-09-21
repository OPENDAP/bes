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
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
// $RCSfile: hdfutil.cc,v $ - Miscellaneous routines for DODS HDF server
//
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <string>
#include <mfhdf.h>
#include <hdfclass.h>
#include "hdfutil.h"
#include "dhdferr.h"

#define SIGNED_BYTE_TO_INT32 1

void *ExportDataForDODS(const hdf_genvec& v) {
    void *rv;			// reminder: rv is an array; must be deleted
				// with delete[] not delete

    switch (v.number_type()) {
    case DFNT_INT16:
	rv = v.export_int16();
	break;

#ifdef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_INT32:
	rv = v.export_int32();
	break;

    case DFNT_UINT16:
	rv = v.export_uint16();
	break;

    case DFNT_UINT32:
	rv = v.export_uint32();
	break;

    case DFNT_FLOAT32: 
	rv = v.export_float32();
	break;

    case DFNT_FLOAT64: 
	rv = v.export_float64();
	break;

#ifndef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_UINT8:
    case DFNT_UCHAR8: 
    case DFNT_CHAR8:
	rv = v.export_uint8();
	break;

    default:
	THROW(dhdferr_datatype);
    }

    return rv;
}

void *ExportDataForDODS(const hdf_genvec& v, int i) {
    void *rv;			// reminder: rv is single value, must be
				// deleted with delete, not delete[]
    switch (v.number_type()) {
    case DFNT_INT16:
	rv = new int16;
	*(static_cast<int16 *>(rv))= v.elt_int16(i);
	break;

#ifdef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_INT32:
	rv = new int32;
	*(static_cast<int32 *>(rv))= v.elt_int32(i);
	break;

    case DFNT_UINT16:
	rv = new uint16;
	*(static_cast<uint16 *>(rv))= v.elt_uint16(i);
	break;

    case DFNT_UINT32:
	rv = new uint32;
	*(static_cast<uint32 *>(rv))= v.elt_uint32(i);
	break;

    case DFNT_FLOAT32: 
	rv = new float32;
	*(static_cast<float32 *>(rv))= v.elt_float32(i);
	break;

    case DFNT_FLOAT64: 
	rv = new float64;
	*(static_cast<float64 *>(rv))= v.elt_float64(i);
	break;

#ifndef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_UINT8:
    case DFNT_UCHAR8:
    case DFNT_CHAR8:
	rv = new uint8;
	*(static_cast<uchar8 *>(rv))= v.elt_uint8(i);
	break; 

    default:
	THROW(dhdferr_datatype);
    }

    return rv;
}

// Just like ExportDataFor DODS *except* that this function does not allocate
// memory which must then be delteted. Why write this function? If the
// hdf_genvec::data() method is used, then the client of hdf_genvec must cast
// the returned pointer to one of the numeric datatypes before adding #i# to
// access a given element. There's no need to write an Access function when
// we're not supplying an index because the entire array returned by the
// data() method can be used (there's no need to cast the returned pointer
// because it's typically just passed to BaseType::val2buf() which copies the
// data and into its own internal storage. 4/10/2002 jhrg
void *AccessDataForDODS(const hdf_genvec& v, int i) {

    void *rv = 0;
    switch (v.number_type()) {
    case DFNT_INT16:
	*(static_cast<int16 *>(rv))= v.elt_int16(i);
	break;

#ifdef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_INT32:
	*(static_cast<int32 *>(rv))= v.elt_int32(i);
	break;

    case DFNT_UINT16:
	*(static_cast<uint16 *>(rv))= v.elt_uint16(i);
	break;

    case DFNT_UINT32:
	*(static_cast<uint32 *>(rv))= v.elt_uint32(i);
	break;

    case DFNT_FLOAT32: 
	*(static_cast<float32 *>(rv))= v.elt_float32(i);
	break;

    case DFNT_FLOAT64: 
	*(static_cast<float64 *>(rv))= v.elt_float64(i);
	break;

#ifndef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_UINT8:
    case DFNT_UCHAR8:
    case DFNT_CHAR8:
	*(static_cast<uchar8 *>(rv))= v.elt_uint8(i);
	break; 

    default:
	THROW(dhdferr_datatype);
    }

    return rv;
}

// $Log: hdfutil.cc,v $
// Revision 1.8.4.1  2003/05/21 16:26:55  edavis
// Updated/corrected copyright statements.
//
// Revision 1.8  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.7.4.3  2002/04/11 23:57:12  jimg
// Fixed up the casts. Now the code uses the new style casts and eliminates
// unnecessary casts.
//
// Revision 1.7.4.2  2002/04/11 03:06:54  jimg
// Added AccessDataForDODS. This function provides a similar type of access as
// the ExportDataForDODS except that the Access function does not copy the data
// into new memory.
//
// Revision 1.7.4.1  2002/04/10 18:38:10  jimg
// I modified the server so that it knows about, and uses, all the DODS
// numeric datatypes. Previously the server cast 32 bit floats to 64 bits and
// cast most integer data to 32 bits. Now if an HDF file contains these
// datatypes (32 bit floats, 16 bit ints, et c.) the server returns data
// using those types (which DODS has supported for a while...).
//
// Revision 1.7  2000/10/09 19:46:20  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.6  1999/05/06 03:23:36  jimg
// Merged changes from no-gnu branch
//
// Revision 1.5.6.1  1999/05/06 00:27:25  jimg
// Jakes String --> string changes
//
// Revision 1.4  1998/02/05 20:14:32  jimg
// DODS now compiles with gcc 2.8.x
//
// Revision 1.3  1997/03/10 22:45:56  jimg
// Update for 2.12
//
// Revision 1.4  1997/02/25 02:03:19  todd
// Added misc comments.
//
// Revision 1.3  1996/11/20  22:28:43  todd
// Modified to support UInt32 type.
//
// Revision 1.2  1996/10/07 21:15:17  todd
// Changes escape character to % from _.
//
// Revision 1.1  1996/09/24 22:38:16  todd
// Initial revision
//
