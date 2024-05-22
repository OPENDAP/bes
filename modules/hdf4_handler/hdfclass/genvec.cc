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

//////////////////////////////////////////////////////////////////////////////
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

// U.S. Government Sponsorship under NASA Contract
// NAS7-1260 is acknowledged.
//
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: genvec.cc,v $ - implementation of HDF generic vector class
//
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <mfhdf.h>

#ifdef __POWERPC__
#undef isascii
#endif
#include <sstream>
#include <string>
#include <cstring>
#include <vector>

#include <libdap/InternalErr.h>

#include <hcerr.h>
#include <hdfclass.h>

using namespace std;
using namespace libdap;

// Convert an array of U with length nelts into an array of T by casting each
// element of array to a T.
template < class T, class U >
    void ConvertArrayByCast(U * array, int nelts, T ** carray)
{
    if (nelts == 0) {
        *carray = nullptr;
        return;
    }
    *carray = new T[nelts];
    if (*carray == nullptr)           // Harmless but should never be used.
        THROW(hcerr_nomemory);
    for (int i = 0; i < nelts; ++i) {
        *(*carray + i) = static_cast < T > (*(array + i));
    }
}

//
// protected member functions
//

// Initialize an hdf_genvec from an input C array. If data, begin, end,
// stride are zero, a zero-length hdf_genvec of the specified type will be
// initialized.
void hdf_genvec::_init(int32 nt, void *data, int begin, int end,
                       int stride)
{

    // input checking - nt must be a valid HDF number type,
    // data, nelts can optionally both together be 0.
    int32 eltsize;              // number of bytes per element
    if ((eltsize = DFKNTsize(nt)) <= 0)
        THROW(hcerr_dftype);    // invalid number type
    bool zerovec = (data == nullptr && begin == 0 && end == 0 && stride == 0);
    if (zerovec) {              // if this is a zero-length vector
        _nelts = 0;
        _data = nullptr;
    } else {
        if (begin < 0 || end < 0 || stride <= 0 || end < begin)
            THROW(hcerr_range); // invalid range given for subset of data
        if (data == nullptr)
            THROW(hcerr_invarr);        // if specify a range, need a data array!

        // allocate memory for _data and assign _nt, _nelts
        auto nelts = (end - begin)/stride + 1;
        _data = new char[nelts * eltsize];      // allocate memory
        if (_data == nullptr)
            THROW(hcerr_nomemory);
        if (stride == 1)        // copy data directly
            (void) memcpy(_data, (void *) ((char *) data + begin),
                          eltsize * nelts);
        else {
            for (int i = 0, j = begin; i < nelts; ++i, j += stride)     // subsample data
                memcpy((void *) (_data + i * eltsize),
                       (void *) ((char *) data + j * eltsize), eltsize);
        }
        _nelts = nelts;         // assign number of elements
    }
    _nt = nt;                   // assign HDF number type
    return;
}

// initialize an empty hdf_genvec
void hdf_genvec::_init(void)
{
    _data = nullptr;
    _nelts = _nt = 0;
    return;
}

// initialize hdf_genvec from another hdf_genvec
void hdf_genvec::_init(const hdf_genvec & gv)
{
    if (gv._nt == 0 && gv._nelts == 0 && gv._data == nullptr)
        _init();
    else if (gv._nelts == 0)
        _init(gv._nt, nullptr, 0, 0, 0);
    else
        _init(gv._nt, gv._data, 0, gv._nelts - 1, 1);
    return;
}

// free up memory of hdf_genvec
void hdf_genvec::_del(void)
{
    delete[]_data;
    _nelts = _nt = 0;
    _data = nullptr;
    return;
}



//
// public member functions
//

hdf_genvec::hdf_genvec(void)
{
    _init();
    return;
}

hdf_genvec::hdf_genvec(int32 nt, void *data, int begin, int end,
                       int stride)
{
    _init(nt, data, begin, end, stride);
    return;
}

hdf_genvec::hdf_genvec(int32 nt, void *data, int nelts)
{
    _init(nt, data, 0, nelts - 1, 1);
    return;
}

hdf_genvec::hdf_genvec(const hdf_genvec & gv)
{
    _init(gv);
    return;
}

hdf_genvec::~hdf_genvec(void)
{
    _del();
    return;
}

hdf_genvec & hdf_genvec::operator=(const hdf_genvec & gv)
{
    if (this == &gv)
        return *this;
    _del();
    _init(gv);
    return *this;
}

// An append method...
void hdf_genvec::append(int32 nt, const char *new_data, int32 nelts)
{
    // input checking: nt must be a valid HDF number type,
    // data, nelts can optionally both together be 0.
    int32 eltsize;              // number of bytes per element
    if ((eltsize = DFKNTsize(nt)) <= 0)
        THROW(hcerr_dftype);    // invalid number type

    if (new_data == nullptr && nelts == 0) {  // if this is a zero-length vector
        _nelts = 0;
        _data = nullptr;
    } else {
        if (nelts == 0)
            THROW(hcerr_range); // invalid range given for subset of data
        if (new_data == nullptr)
            THROW(hcerr_invarr);        // if specify a range, need a data array!

        // allocate memory for _data and assign _nt, _nelts
        auto d = new char[(_nelts + nelts) * eltsize]; // allocate memory
        memcpy(d, _data, _nelts);
        memcpy(d + _nelts, new_data, nelts);

        delete[]_data;

        _data = d;
        _nelts += nelts;        // assign number of elements
    }

    _nt = nt;                   // assign HDF number type
    return;
}

// import new data into hdf_genvec (old data is deleted)
void hdf_genvec::import_vec(int32 nt, void *data, int begin, int end,
                        int stride)
{
    _del();
    if (nt == 0)
        _init();
    else
        _init(nt, data, begin, end, stride);
    return;
}

// import new data into hdf_genvec from a vector of strings
void hdf_genvec::import_vec(int32 nt, const vector < string > &sv)
{
    static char strbuf[hdfclass::MAXSTR];

    int eltsize = DFKNTsize(nt);
    if (eltsize == 0)
        THROW(hcerr_invnt);
    if (sv.empty() == true) {
        this->import_vec(nt);
        return;
    }

    auto obuf = new char[DFKNTsize(nt) * sv.size()];
    switch (nt) {
    case DFNT_FLOAT32:{
            float32 val;
            for (int i = 0; i < (int) sv.size(); ++i) {
                strncpy(strbuf, sv[i].c_str(), hdfclass::MAXSTR - 1);
                istringstream(strbuf) >> val;
                *((float32 *) obuf + i) = val;
            }
            break;
        }
    case DFNT_FLOAT64:{
            float64 val;
            for (int i = 0; i < (int) sv.size(); ++i) {
                strncpy(strbuf, sv[i].c_str(), hdfclass::MAXSTR - 1);
                istringstream(strbuf) >> val;
                *((float64 *) obuf + i) = val;
            }
            break;
        }
    case DFNT_INT8:{
            int8 val;
            for (int i = 0; i < (int) sv.size(); ++i) {
                strncpy(strbuf, sv[i].c_str(), hdfclass::MAXSTR - 1);
                istringstream iss(strbuf);
                iss >> val;
                *((int8 *) obuf + i) = val;
            }
            break;
        }
    case DFNT_INT16:{
            int16 val;
            for (int i = 0; i < (int) sv.size(); ++i) {
                istringstream(strbuf) >> val;
                *((int16 *) obuf + i) = val;
            }
            break;
        }
    case DFNT_INT32:{
            int32 val;
            for (int i = 0; i < (int) sv.size(); ++i) {
                strncpy(strbuf, sv[i].c_str(), hdfclass::MAXSTR - 1);
                istringstream(strbuf) >> val;
                *((int32 *) obuf + i) = val;
            }
            break;
        }
    case DFNT_UINT8:{
            uint8 val;
            for (int i = 0; i < (int) sv.size(); ++i) {
                strncpy(strbuf, sv[i].c_str(), hdfclass::MAXSTR - 1);
                istringstream iss(strbuf);
                iss >> val;
                *((uint8 *) obuf + i) = val;
            }
            break;
        }
    case DFNT_UINT16:{
            uint16 val;
            for (int i = 0; i < (int) sv.size(); ++i) {
                strncpy(strbuf, sv[i].c_str(), hdfclass::MAXSTR - 1);
                istringstream(strbuf) >> val;
                *((uint16 *) obuf + i) = val;
            }
            break;
        }
    case DFNT_UINT32:{
            uint32 val;
            for (int i = 0; i < (int) sv.size(); ++i) {
                strncpy(strbuf, sv[i].c_str(), hdfclass::MAXSTR - 1);
                istringstream(strbuf) >> val;
                *((uint32 *) obuf + i) = val;
            }
            break;
        }
    case DFNT_UCHAR8:{
            uchar8 val;
            for (int i = 0; i < (int) sv.size(); ++i) {
                strncpy(strbuf, sv[i].c_str(), hdfclass::MAXSTR - 1);
                istringstream iss(strbuf);
                iss >> val;
                *((uchar8 *) obuf + i) = val;
            }
            break;
        }
    case DFNT_CHAR8:{
            char8 val;
            for (int i = 0; i < (int) sv.size(); ++i) {
                strncpy(strbuf, sv[i].c_str(), hdfclass::MAXSTR - 1);
                istringstream iss(strbuf);
                iss >> val;
                *((char8 *) obuf + i) = val;
            }
            break;
        }
    default:
    	delete[] obuf;
        THROW(hcerr_invnt);
    }

    this->import_vec(nt, obuf, (int) sv.size());
    delete[] obuf;
    return;
}

// export an hdf_genvec holding uint8 or uchar8 data to a uchar8 array
// Added export of int8 to uchar8. A bad idea, but needed to fix some
// clients. The same `fix' has been applied to some other mfuncs that follow.
// 1/13/98 jhrg.
//
// It looks like this code treats all 8-bit datatypes as the same the user
// has to know if they are signed or not. 4/8/2002 jhrg
uchar8 *hdf_genvec::export_uchar8(void) const
{
    uchar8 *rv = 0;
    if (_nt == DFNT_UINT8)
        ConvertArrayByCast((uint8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_UCHAR8)
        ConvertArrayByCast((uchar8 *) _data, _nelts, &rv);
    // Added the following case. jhrg 1/13/98.
#if 0
    else if (_nt == DFNT_INT8)
        ConvertArrayByCast((int8 *) _data, _nelts, &rv);
#endif
    else
        THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a uchar8
uchar8 hdf_genvec::elt_uchar8(int i) const
{
    uchar8 rv;
    if (i < 0 || i > _nelts)
        THROW(hcerr_range);
    if (_nt == DFNT_UINT8)
        rv = (uchar8) * ((uint8 *) _data + i);
    else if (_nt == DFNT_UCHAR8)
        rv = *((uchar8 *) _data + i);
    // Added the following case. 1/13/98 jhrg.
#if 0
    else if (_nt == DFNT_INT8)
        rv = *((int8 *) _data + i);
#endif
    else
        THROW(hcerr_dataexport);
    return rv;
}

// export an hdf_genvec holding uint8 or uchar8 data to a uchar8 vector
vector < uchar8 > hdf_genvec::exportv_uchar8(void) const
{
    auto rv = vector < uchar8 > (0);
    uchar8 *dtmp = nullptr;
    if (_nt == DFNT_UINT8)      // cast to uchar8 array and export
        ConvertArrayByCast((uint8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_UCHAR8)
        dtmp = (uchar8 *) _data;
    // Added the following case. 1/13/98 jhrg.
#if 0
    else if (_nt == DFNT_INT8)
        ConvertArrayByCast((int8 *) _data, _nelts, &dtmp);
#endif
    else
        THROW(hcerr_dataexport);
    rv = vector < uchar8 > (dtmp, dtmp + _nelts);
    if (dtmp != (uchar8 *) _data)
        delete[]dtmp;
    return rv;
}

// export an hdf_genvec holding int8 or char8 data to a char8 array
char8 *hdf_genvec::export_char8(void) const
{
    char8 *rv = nullptr;
    if (_nt == DFNT_INT8)
        ConvertArrayByCast((int8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_CHAR8)
        ConvertArrayByCast((char8 *) _data, _nelts, &rv);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a char8
char8 hdf_genvec::elt_char8(int i) const
{
    char8 rv;
    if (i < 0 || i > _nelts)
        THROW(hcerr_range);
    if (_nt == DFNT_INT8)
        rv = (char8) * ((int8 *) _data + i);
    else if (_nt == DFNT_CHAR8 || _nt == DFNT_UCHAR8)
        rv = *((char8 *) _data + i);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// export an hdf_genvec holding int8 or char8 data to a char8 vector
vector < char8 > hdf_genvec::exportv_char8(void) const
{
    auto rv = vector < char8 > (0);
    char8 *dtmp = nullptr;
    if (_nt == DFNT_INT8)       // cast to char8 array and export
        ConvertArrayByCast((int8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_CHAR8)
        ConvertArrayByCast((char8 *) _data, _nelts, &dtmp);
    else
        THROW(hcerr_dataexport);
    if (!dtmp)
    	throw InternalErr(__FILE__, __LINE__, "No data returned for the character array.");
    rv = vector < char8 > (dtmp, dtmp + _nelts);
    if (dtmp != (char8 *) _data)
        delete[]dtmp;
    return rv;
}

// export an hdf_genvec holding uchar8 or uint8 data to a uint8 array
uint8 *hdf_genvec::export_uint8(void) const
{
    uint8 *rv = nullptr;
    if (_nt == DFNT_UCHAR8 || _nt == DFNT_CHAR8)
        ConvertArrayByCast((uchar8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_UINT8)
        ConvertArrayByCast((uint8 *) _data, _nelts, &rv);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a uint8
uint8 hdf_genvec::elt_uint8(int i) const
{
    uint8 rv;
    if (i < 0 || i > _nelts)
        THROW(hcerr_range);
    if (_nt == DFNT_UCHAR8 || _nt == DFNT_CHAR8)
        rv = (uint8) * ((uchar8 *) _data + i);
    else if (_nt == DFNT_UINT8)
        rv = *((uint8 *) _data + i);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// export an hdf_genvec holding uchar8 or uint8 data to a uint8 vector
vector < uint8 > hdf_genvec::exportv_uint8(void) const
{
    auto rv = vector < uint8 > (0);
    uint8 *dtmp = nullptr;
    if (_nt == DFNT_UCHAR8 || _nt == DFNT_CHAR8)        // cast to uint8 array and export
        ConvertArrayByCast((uchar8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT8)
        dtmp = (uint8 *) _data;
    else
        THROW(hcerr_dataexport);

    rv = vector < uint8 > (dtmp, dtmp + _nelts);
    if (dtmp != (uint8 *) _data)
        delete[]dtmp;
    return rv;
}

// export an hdf_genvec holding char8 or int8 data to a int8 array
int8 *hdf_genvec::export_int8(void) const
{
    int8 *rv = nullptr;
    if (_nt == DFNT_CHAR8)
        ConvertArrayByCast((char8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_INT8)
        ConvertArrayByCast((int8 *) _data, _nelts, &rv);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a int8
int8 hdf_genvec::elt_int8(int i) const
{
    int8 rv;
    if (i < 0 || i > _nelts)
        THROW(hcerr_range);
    if (_nt == DFNT_CHAR8)
        rv = (int8) * ((char8 *) _data + i);
    else if (_nt == DFNT_INT8)
        rv = *((int8 *) _data + i);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// export an hdf_genvec holding int8 data to a int8 vector
vector < int8 > hdf_genvec::exportv_int8(void) const
{
    auto rv = vector < int8 > (0);
    int8 *dtmp = nullptr;
    if (_nt == DFNT_CHAR8)      // cast to int8 array and export
        ConvertArrayByCast((char8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_INT8)
        dtmp = (int8 *) _data;
    else
        THROW(hcerr_dataexport);
    rv = vector < int8 > (dtmp, dtmp + _nelts);
    if (dtmp != (int8 *) _data)
        delete[]dtmp;
    return rv;
}

// export an hdf_genvec holding uchar8, uint8 or uint16 data to a uint16 array
uint16 *hdf_genvec::export_uint16(void) const
{
    uint16 *rv = nullptr;
    if (_nt == DFNT_UCHAR8)     // cast to uint16 array and export
        ConvertArrayByCast((uchar8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_UINT8) // cast to uint16 array and export
        ConvertArrayByCast((uint8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_UINT16)
        ConvertArrayByCast((uint16 *) _data, _nelts, &rv);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a uint16
uint16 hdf_genvec::elt_uint16(int i) const
{
    uint16 ret_value = 0;
    if (i < 0 || i > _nelts)
        THROW(hcerr_range);
    if (_nt == DFNT_UCHAR8)
        ret_value = (uint16) * ((uchar8 *) _data + i);
    else if (_nt == DFNT_UINT8)
        ret_value = (uint16) * ((uint8 *) _data + i);
    else if (_nt == DFNT_UINT16)
        ret_value = *((uint16 *) _data + i);
    else
        THROW(hcerr_dataexport);
    return ret_value;
}

// export an hdf_genvec holding uchar8, uint8 or uint16 data to a uint16 vector
vector < uint16 > hdf_genvec::exportv_uint16(void) const
{
    auto rv = vector < uint16 > (0);
    uint16 *dtmp = nullptr;
    if (_nt == DFNT_UCHAR8)     // cast to uint16 array and export
        ConvertArrayByCast((uchar8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT8) // cast to uint16 array and export
        ConvertArrayByCast((uint8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT16)
        dtmp = (uint16 *) _data;
    else
        THROW(hcerr_dataexport);
    rv = vector < uint16 > (dtmp, dtmp + _nelts);
    if (dtmp != (uint16 *) _data)
        delete[]dtmp;
    return rv;
}

// export an hdf_genvec holding uchar8, char8, uint8, int8 or int16 data to
// an int16 array
int16 *hdf_genvec::export_int16(void) const
{
    int16 *rv = nullptr;
    if (_nt == DFNT_UCHAR8)     // cast to int16 array and export
        ConvertArrayByCast((uchar8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_CHAR8) // cast to int16 array and export
        ConvertArrayByCast((char8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_UINT8) // cast to int16 array and export
        ConvertArrayByCast((uint8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_INT8)  // cast to int16 array and export
        ConvertArrayByCast((int8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_INT16)
        ConvertArrayByCast((int16 *) _data, _nelts, &rv);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a int16
int16 hdf_genvec::elt_int16(int i) const
{
    int16 ret_value = 0;
    if (i < 0 || i > _nelts)
        THROW(hcerr_range);
    if (_nt == DFNT_UCHAR8)
        ret_value = (int16) (*((uchar8 *) _data + i));
    else if (_nt == DFNT_CHAR8)
        ret_value = (int16) (*((char8 *) _data + i));
    else if (_nt == DFNT_UINT8)
        ret_value = (int16) (*((uint8 *) _data + i));
    else if (_nt == DFNT_INT8)
        ret_value =  (int16) (*((int8 *) _data + i));
    else if (_nt == DFNT_INT16)
        ret_value =  *((int16 *) _data + i);
    else
        THROW(hcerr_dataexport);
    return ret_value;
}

// export an hdf_genvec holding int8 or int16 data to an int16 vector
vector < int16 > hdf_genvec::exportv_int16(void) const
{
    auto rv = vector < int16 > (0);
    int16 *dtmp = nullptr;
    if (_nt == DFNT_UCHAR8)     // cast to int16 array and export
        ConvertArrayByCast((uchar8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_CHAR8) // cast to int16 array and export
        ConvertArrayByCast((char8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT8) // cast to int16 array and export
        ConvertArrayByCast((uint8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_INT8)  // cast to int16 array and export
        ConvertArrayByCast((int8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_INT16)
        dtmp = (int16 *) _data;
    else
        THROW(hcerr_dataexport);
    rv = vector < int16 > (dtmp, dtmp + _nelts);
    if (dtmp != (int16 *) _data)
        delete[]dtmp;
    return rv;
}

// export an hdf_genvec holding uchar8, uint8, uint16 or uint32 data to a
// uint32 array
uint32 *hdf_genvec::export_uint32(void) const
{
    uint32 *rv = nullptr;
    if (_nt == DFNT_UCHAR8)     // cast to uint32 array and export
        ConvertArrayByCast((uchar8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_UINT8) // cast to uint32 array and export
        ConvertArrayByCast((uint8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_UINT16)        // cast to uint32 array and export
        ConvertArrayByCast((uint16 *) _data, _nelts, &rv);
    else if (_nt == DFNT_UINT32)
        ConvertArrayByCast((uint32 *) _data, _nelts, &rv);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a uint32
uint32 hdf_genvec::elt_uint32(int i) const
{
    uint32 ret_value = 0;
    if (i < 0 || i > _nelts)
        THROW(hcerr_range);
    if (_nt == DFNT_UCHAR8)
        ret_value = (uint32) (*((uchar8 *) _data + i));
    else if (_nt == DFNT_UINT8)
        ret_value =  (uint32) (*((uint8 *) _data + i));
    else if (_nt == DFNT_UINT16)
        ret_value = (uint32) (*((uint16 *) _data + i));
    else if (_nt == DFNT_UINT32)
        ret_value = *((uint32 *) _data + i);
    else
        THROW(hcerr_dataexport);
    return ret_value;
}

// export an hdf_genvec holding uchar8, uint8, uint16 or uint32 data to a
// uint32 vector
vector < uint32 > hdf_genvec::exportv_uint32(void) const
{
    auto rv = vector < uint32 > (0);
    uint32 *dtmp = nullptr;
    if (_nt == DFNT_UCHAR8)     // cast to uint32 array and export
        ConvertArrayByCast((uchar8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT8) // cast to uint32 array and export
        ConvertArrayByCast((uint8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT16)        // cast to uint32 array and export
        ConvertArrayByCast((uint16 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT32)
        dtmp = (uint32 *) _data;
    else
        THROW(hcerr_dataexport);
    rv = vector < uint32 > (dtmp, dtmp + _nelts);
    if (dtmp != (uint32 *) _data)
        delete[]dtmp;
    return rv;
}

// export an hdf_genvec holding uchar8, char8, uint8, int8, uint16, int16 or
// int32 data to a int32 array
int32 *hdf_genvec::export_int32(void) const
{
    int32 *rv = nullptr;
    if (_nt == DFNT_UCHAR8)     // cast to int32 array and export
        ConvertArrayByCast((uchar8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_CHAR8) // cast to int32 array and export
        ConvertArrayByCast((char8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_UINT8) // cast to int32 array and export
        ConvertArrayByCast((uint8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_INT8)  // cast to int32 array and export
        ConvertArrayByCast((int8 *) _data, _nelts, &rv);
    else if (_nt == DFNT_UINT16)
        ConvertArrayByCast((uint16 *) _data, _nelts, &rv);
    else if (_nt == DFNT_INT16)
        ConvertArrayByCast((int16 *) _data, _nelts, &rv);
    else if (_nt == DFNT_INT32)
        ConvertArrayByCast((int32 *) _data, _nelts, &rv);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a int32
int32 hdf_genvec::elt_int32(int i) const
{
    int32 ret_value = 0;
    if (i < 0 || i > _nelts)
        THROW(hcerr_range);
    if (_nt == DFNT_UCHAR8)
        ret_value = (int32) (*((uchar8 *) _data + i));
    else if (_nt == DFNT_CHAR8)
        ret_value =  (int32) (*((char8 *) _data + i));
    else if (_nt == DFNT_UINT8)
        ret_value =  (int32) (*((uint8 *) _data + i));
    else if (_nt == DFNT_INT8)
        ret_value =  (int32) (*((int8 *) _data + i));
    else if (_nt == DFNT_UINT16)
        ret_value =  (int32) (*((uint16 *) _data + i));
    else if (_nt == DFNT_INT16)
        ret_value =  (int32) (*((int16 *) _data + i));
    else if (_nt == DFNT_INT32)
        ret_value = *((int32 *) _data + i);
    else
        THROW(hcerr_dataexport);
    return ret_value;
}

// export an hdf_genvec holding uchar8, char8, uint8, int8, uint16, int16 or
// int32 data to a int32 vector
vector < int32 > hdf_genvec::exportv_int32(void) const
{
    auto rv = vector < int32 > (0);
    int32 *dtmp = nullptr;
    if (_nt == DFNT_UCHAR8)     // cast to int32 array and export
        ConvertArrayByCast((uchar8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_CHAR8) // cast to int32 array and export
        ConvertArrayByCast((char8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT8) // cast to int32 array and export
        ConvertArrayByCast((uint8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_INT8)  // cast to int32 array and export
        ConvertArrayByCast((int8 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT16)        // cast to int32 array and export
        ConvertArrayByCast((uint16 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_INT16) // cast to int32 array and export
        ConvertArrayByCast((int16 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_INT32)
        dtmp = (int32 *) _data;
    else
        THROW(hcerr_dataexport);
    rv = vector < int32 > (dtmp, dtmp + _nelts);
    if (dtmp != (int32 *) _data)
        delete[]dtmp;
    return rv;
}

// export an hdf_genvec holding float32 data to a float32 array
float32 *hdf_genvec::export_float32(void) const
{
    float32 *rv = nullptr;
    if (_nt != DFNT_FLOAT32)
        THROW(hcerr_dataexport);
    else
        ConvertArrayByCast((float32 *) _data, _nelts, &rv);
    return rv;
}

// return the i'th element of the vector as a float32
float32 hdf_genvec::elt_float32(int i) const
{
    if (i < 0 || i > _nelts)
        THROW(hcerr_range);
    if (_nt != DFNT_FLOAT32)
        THROW(hcerr_dataexport);
    return *((float32 *) _data + i);
}

// export an hdf_genvec holding float32 data to a float32 vector
vector < float32 > hdf_genvec::exportv_float32(void) const
{
    if (_nt != DFNT_FLOAT32) {
        THROW(hcerr_dataexport);
    } else
        return vector < float32 > ((float32 *) _data,
                                   (float32 *) _data + _nelts);
}

// export an hdf_genvec holding float32 or float64 data to a float64 array
float64 *hdf_genvec::export_float64(void) const
{
    float64 *rv = nullptr;
    if (_nt == DFNT_FLOAT64)
        ConvertArrayByCast((float64 *) _data, _nelts, &rv);
    else if (_nt == DFNT_FLOAT32)       // cast to float64 array and export
        ConvertArrayByCast((float32 *) _data, _nelts, &rv);
    else
        THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a float64
float64 hdf_genvec::elt_float64(int i) const
{
    if (i < 0 || i > _nelts)
        THROW(hcerr_range);
    if (_nt == DFNT_FLOAT64)
        return *((float64 *) _data + i);
    else if (_nt == DFNT_FLOAT32)
        return (float64) (*((float32 *) _data + i));
    else
        THROW(hcerr_dataexport);
}

// export an hdf_genvec holding float32 or float64 data to a float64 vector
vector < float64 > hdf_genvec::exportv_float64(void) const
{
    auto rv = vector < float64 > (0);
    float64 *dtmp = nullptr;
    if (_nt == DFNT_FLOAT32)    // cast to float64 array and export
        ConvertArrayByCast((float32 *) _data, _nelts, &dtmp);
    else if (_nt == DFNT_FLOAT64)
        dtmp = (float64 *) _data;
    else
        THROW(hcerr_dataexport);
    rv = vector < float64 > (dtmp, dtmp + _nelts);
    if (dtmp != (float64 *) _data)
        delete[]dtmp;
    return rv;
}

// export an hdf_genvec holding char data to a string
string hdf_genvec::export_string(void) const
{
    if (_nt != DFNT_CHAR8 && _nt != DFNT_UCHAR8) {
        THROW(hcerr_dataexport);
    }
    else {
        if (_data == nullptr)
            return string();
        else
            return string((char *) _data, _nelts);
    }
}

// print all of the elements of hdf_genvec to a vector of string
void hdf_genvec::print(vector < string > &sv) const
{
    if (_nelts > 0)
        print(sv, 0, _nelts - 1, 1);
    return;
}

// print the elements of hdf_genvec to a vector of string; start with initial
// element "begin", end with "end" and increment by "stride" elements.
void hdf_genvec::print(vector < string > &sv, int begin, int end,
                       int stride) const
{
    if (begin < 0 || begin > _nelts || stride < 1 || end < 0 || end < begin
        || stride <= 0 || end > _nelts - 1)
        THROW(hcerr_range);
    if (_nt == DFNT_CHAR8 || _nt == DFNT_UCHAR8) {
        string sub;
        sub = string((char *) _data + begin, (end - begin + 1));
        if (stride > 1) {
            string x;
            for (int i = 0; i < (end - begin + 1); i += stride)
                x += sub[i];
            sub = x;
        }
        sv.push_back(sub);
    } else {
#if 0
        char buf[hdfclass::MAXSTR];
#endif
        int i;
        switch (_nt) {
#if 0
        case DFNT_UCHAR8:
            for (i = begin; i <= end; i += stride) {
                ostrstream(buf, hdfclass::MAXSTR) <<
                    (int) *((uchar8 *) _data + i) << ends;
                sv.push_back(string(buf));
            }
            break;
#endif
        case DFNT_UINT8:
            for (i = begin; i <= end; i += stride) {
                ostringstream buf;
                buf << (unsigned int) *((uint8 *) _data + i);
                sv.push_back(buf.str());
            }
            break;
        case DFNT_INT8:
            for (i = begin; i <= end; i += stride) {
                ostringstream buf;
                buf << (int) *((int8 *) _data + i);
                sv.push_back(buf.str());
            }
            break;
        case DFNT_UINT16:
            for (i = begin; i <= end; i += stride) {
                ostringstream buf;
                buf << *((uint16 *) _data + i);
                sv.push_back(buf.str());
            }
            break;
        case DFNT_INT16:
            for (i = begin; i <= end; i += stride) {
                ostringstream buf;
                buf << *((int16 *) _data + i);
                sv.push_back(buf.str());
            }
            break;
        case DFNT_UINT32:
            for (i = begin; i <= end; i += stride) {
                ostringstream buf;
                buf << *((uint32 *) _data + i);
                sv.push_back(buf.str());
            }
            break;
        case DFNT_INT32:
            for (i = begin; i <= end; i += stride) {
                ostringstream buf;
                buf << *((int32 *) _data + i);
                sv.push_back(buf.str());
            }
            break;
        case DFNT_FLOAT32:
            for (i = begin; i <= end; i += stride) {
                ostringstream buf;
                buf << *((float32 *) _data + i);
                sv.push_back(buf.str());
            }
            break;
        case DFNT_FLOAT64:
            for (i = begin; i <= end; i += stride) {
                ostringstream buf;
                buf << *((float64 *) _data + i);
                sv.push_back(buf.str());
            }
            break;
        default:
            THROW(hcerr_dftype);
        }
    }
    return;
}

// $Log: genvec.cc,v $
// Revision 1.7.4.1.2.1  2004/02/23 02:08:03  rmorris
// There is some incompatibility between the use of isascii() in the hdf library
// and its use on OS X.  Here we force in the #undef of isascii in the osx case.
//
// Revision 1.7.4.1  2003/05/21 16:26:58  edavis
// Updated/corrected copyright statements.
//
// Revision 1.7  2003/01/31 02:08:37  jimg
// Merged with release-3-2-7.
//
// Revision 1.6.4.3  2002/12/18 23:32:50  pwest
// gcc3.2 compile corrections, mainly regarding the using statement. Also,
// missing semicolon in .y file
//
// Revision 1.6.4.2  2002/04/11 03:15:43  jimg
// Minor change to the ConvertArrayByCast template function. Still, avoid using
// this if possible.
//
// Revision 1.6.4.1  2001/10/30 06:36:35  jimg
// Added genvec::append(...) method.
// Fixed up some comments in genvec.
// Changed genvec's data member from void * to char * to quell warnings
// about void * being passed to delete.
//
// Revision 1.6  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.5  1999/05/06 03:23:33  jimg
// Merged changes from no-gnu branch
//
// Revision 1.4  1999/05/05 23:33:43  jimg
// String --> string conversion
//
// Revision 1.3.6.1  1999/05/06 00:35:45  jimg
// Jakes String --> string changes
//
// Revision 1.3  1998/09/10 21:33:24  jehamby
// Map DFNT_CHAR8 and DFNT_UCHAR8 to Byte instead of string in SDS.
//
// Revision 1.2  1998/02/05 20:14:29  jimg
// DODS now compiles with gcc 2.8.x
//
// Revision 1.1  1996/10/31 18:42:56  jimg
// Added.
//
// Revision 1.12  1996/10/14 23:07:53  todd
// Added a new import function to hdf_genvec to allow import from a vector
// of string.
//
// Revision 1.11  1996/08/22 20:54:14  todd
// Rewrite of export function suite to correctly support casts.  Now all casts towards
// integers of greater size are supported.  (E.g., char8 -> int8 ->int16 -> int32, but
// disallow int16 -> uint32 or uint32 -> int32).
//
// Revision 1.10  1996/08/21  23:18:59  todd
// Added mfuncs to return the i'th element of a genvec.
//
// Revision 1.9  1996/07/22  17:12:20  todd
// Changed export_*() mfuncs so they return a C++ array.  Added exportv_*() mfuncs
// to return STL vectors.
//
// Revision 1.8  1996/06/18  21:49:21  todd
// Fixed pointer expressions so they would be in conformance with proper usage of void *
//
// Revision 1.7  1996/06/18  18:37:42  todd
// Added support for initialization from a subsampled array to the _init() and
// constructor mfuncs; also added support for print() to output a subsample of
// the hdf_genvec.
// Added copyright notice.
//
// Revision 1.6  1996/05/02  18:10:51  todd
// Fixed a bug in print() and in export_string().
//
// Revision 1.5  1996/04/23  21:11:50  todd
// Fixed declaration of print mfunc so it was const correct.
//
// Revision 1.4  1996/04/18  19:06:37  todd
// Added print() mfunc
//
// Revision 1.3  1996/04/04  01:11:30  todd
// Added support for empty vectors that have a number type.
// Added import_vec() public member function.
//
// Revision 1.2  1996/04/03  00:18:18  todd
// Fixed a bug in _init(int32, void *, int)
//
// Revision 1.1  1996/04/02  20:36:43  todd
// Initial revision
//
