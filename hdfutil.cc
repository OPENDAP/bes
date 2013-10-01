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

#include <string>
#include <mfhdf.h>
#include <hdfclass.h>
#include "hdfutil.h"
#include "dhdferr.h"

#define SIGNED_BYTE_TO_INT32 1

void *ExportDataForDODS(const hdf_genvec & v)
{
    void *rv;                   // reminder: rv is an array; must be deleted
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

// Reminder: Use delete and not delete[] to free the storage returned by this
// function. 
// 
// Declaring the pointers to be of specific types addresses an off-by-one
// error when assigning values from the unsigned types.
//
// jhrg 3/2008.
void *ExportDataForDODS(const hdf_genvec & v, int i)
{
    switch (v.number_type()) {
    case DFNT_INT16:{
            int16 *temp = new int16;
            *temp = v.elt_int16(i);
            return (void *) temp;
        }

#ifdef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_INT32:{
            int32 *temp = new int32;
            *temp = v.elt_int32(i);
            return (void *) temp;
        }

    case DFNT_UINT16:{
            uint16 *temp = new uint16;
            *temp = v.elt_uint16(i);
            return (void *) temp;
        }

    case DFNT_UINT32:{
            uint32 *temp = new uint32;
            *temp = v.elt_uint32(i);
            return (void *) temp;
        }

    case DFNT_FLOAT32:{
            float32 *temp = new float32;
            *temp = v.elt_float32(i);
            return (void *) temp;
        }

    case DFNT_FLOAT64:{
            float64 *temp = new float64;
            *temp = v.elt_float64(i);
            return (void *) temp;
        }

#ifndef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_UINT8:
    case DFNT_UCHAR8:
    case DFNT_CHAR8:{
            uint8 *temp = new uint8;
            *temp = v.elt_uint8(i);
            return (void *) temp;
        }

    default:
        THROW(dhdferr_datatype);
    }
}

#if 0
// Just like ExportDataFor DODS *except* that this function does not allocate
// memory which must then be deleted. Why write this function? If the
// hdf_genvec::data() method is used, then the client of hdf_genvec must cast
// the returned pointer to one of the numeric datatypes before adding #i# to
// access a given element. There's no need to write an Access function when
// we're not supplying an index because the entire array returned by the
// data() method can be used (there's no need to cast the returned pointer
// because it's typically just passed to BaseType::val2buf() which copies the
// data and into its own internal storage.
//
// This is a great idea but the problem is that hdf_genvec::elt_int16(), ...
// all allocate and return copies of the value; the change to return the
// address needs to be made in hdf_genvec by adding new accessor methods.
void *AccessDataForDODS(const hdf_genvec & v, int i)
{

    void *rv = 0; // BROKEN HERE
    switch (v.number_type()) {
    case DFNT_INT16:
        *(static_cast < int16 * >(rv)) = v.elt_int16(i);
        break;

#ifdef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_INT32:
        *(static_cast < int32 * >(rv)) = v.elt_int32(i);
        break;

    case DFNT_UINT16:
        *(static_cast < uint16 * >(rv)) = v.elt_uint16(i);
        break;

    case DFNT_UINT32:
        *(static_cast < uint32 * >(rv)) = v.elt_uint32(i);
        break;

    case DFNT_FLOAT32:
        *(static_cast < float32 * >(rv)) = v.elt_float32(i);
        break;

    case DFNT_FLOAT64:
        *(static_cast < float64 * >(rv)) = v.elt_float64(i);
        break;

#ifndef SIGNED_BYTE_TO_INT32
    case DFNT_INT8:
#endif
    case DFNT_UINT8:
    case DFNT_UCHAR8:
    case DFNT_CHAR8:
        *(static_cast < uchar8 * >(rv)) = v.elt_uint8(i);
        break;

    default:
        THROW(dhdferr_datatype);
    }

    return rv;
}
#endif
