/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1996, California Institute of Technology.  
// ALL RIGHTS RESERVED.   U.S. Government Sponsorship acknowledged. 
//
// Please read the full copyright notice in the file COPYRIGH 
// in this directory.
//
// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: hdfutil.cc,v $ - Miscellaneous routines for DODS HDF server
//
// $Log: hdfutil.cc,v $
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
//
/////////////////////////////////////////////////////////////////////////////

#include <String.h>
#include <mfhdf.h>
#include <hdfclass.h>
#include "hdfutil.h"
#include "dhdferr.h"

void *ExportDataForDODS(const hdf_genvec& v) {
    void *rv;			// reminder: rv is an array; must be deleted
				// with delete[] not delete

    switch (v.number_type()) {
    case DFNT_INT8:
    case DFNT_INT16:
    case DFNT_INT32:
	rv = v.export_int32();
	break;
    case DFNT_UINT8:
    case DFNT_UINT16:
    case DFNT_UINT32:
	rv = v.export_uint32();
	break;
    case DFNT_FLOAT32: 
    case DFNT_FLOAT64: 
	rv = v.export_float64();
	break;
    case DFNT_UCHAR8:
	rv = v.export_uchar8();
	break;
    case DFNT_CHAR8:
//	rv = v.export_char8();
	rv = (void *)new String((char *)v.export_char8());
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
    case DFNT_INT8:
    case DFNT_INT16:
    case DFNT_INT32:
	rv = (void *)new int32;
	*((int32 *)rv)= v.elt_int32(i);
	break;
    case DFNT_UINT8:
    case DFNT_UINT16:
    case DFNT_UINT32:
	rv = (void *)new uint32;
	*((uint32 *)rv)= v.elt_uint32(i);
	break;
    case DFNT_FLOAT32: 
    case DFNT_FLOAT64: 
	rv = (void *)new float64;
	*((float64 *)rv)= v.elt_float64(i);
	break;
    case DFNT_UCHAR8:
	rv = (void *)new uchar8;
	*((uchar8 *)rv)= v.elt_uchar8(i);
	break;
    case DFNT_CHAR8:
//	rv = (void *)new char8;
//	*((char8 *)rv)= v.elt_char8(i);
	rv = (void *)new String(v.elt_char8(i));
	break;
    default:
	THROW(dhdferr_datatype);
    }
    return rv;
}

