/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath having dimension maps

//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
// We will interpolate the latitude and longitude fields based on the dimension map parameters
// struct dimmap_entry includes all the entries.
// We are using the linear interpolation method to interpolate the latitude and longitude.
// So far we only see float32 and float64 types of latitude and longitude. So we only
// interpolate latitude and longitude of these data types.

#ifdef USE_HDFEOS2_LIB

#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include <BESDebug.h>
#include "InternalErr.h"
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"
#include "HDFEOS2.h"
#include "HDFCFUtil.h"
#include "HDFEOS2ArraySwathGeoDimMapField.h"
#define SIGNED_BYTE_TO_INT32 1


bool
HDFEOS2ArraySwathGeoDimMapField::read ()
{

    if (rank > 2) 
        throw InternalErr (__FILE__, __LINE__, "Currently doesn't support rank >2 with the dimension map");

    int *offset = new int[rank];
    int *count = new int[rank];
    int *step = new int[rank];


    int nelms; //[LD Comment 11/13/2012]

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

    openfunc = SWopen;
    closefunc = SWclose;
    attachfunc = SWattach;
    detachfunc = SWdetach;
    fieldinfofunc = SWfieldinfo;
    readfieldfunc = SWreadfield;
    datasetname = swathname;

    // Declare file ID and swath ID
    int32 fileid, swathid; //[LD Comment 11/13/2012]

    fileid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);

    if (fileid < 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        ostringstream eherr;

        eherr << "File " << filename.c_str () << " cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    swathid = attachfunc (fileid, const_cast < char *>(datasetname.c_str ()));

    if (swathid < 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        closefunc (fileid);
        ostringstream eherr;

        eherr << "Grid/Swath " << datasetname.c_str () << " cannot be attached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    intn r;
    int32 majordimsize, minordimsize; //[LD Comment 11/13/2012]


    switch (dtype) {
        case DFNT_INT8:
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        case DFNT_CHAR8:
        case DFNT_INT16:
        case DFNT_UINT16:
        case DFNT_INT32:
        case DFNT_UINT32:

        {
            HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
            detachfunc (swathid);
            closefunc (fileid);
            ostringstream eherr;

            eherr << fieldname.c_str () << " datatype is not float, unsupported.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());

        }
            break;
        case DFNT_FLOAT32:
        {
            std::vector < float32 > latlon32;
            r = GetLatLon (swathid, fieldname, dimmaps, latlon32,
                           &majordimsize, &minordimsize);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
               detachfunc (swathid);
               closefunc (fileid);
               ostringstream eherr;

               eherr << "field " << fieldname.c_str () << "cannot be read.";
               throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            float32 *outlatlon32 = new float32[nelms];

            LatLonSubset (outlatlon32, majordimsize, minordimsize,
                &latlon32[0], offset32, count32, step32);
            set_value ((dods_float32 *) outlatlon32, nelms);
            delete[]outlatlon32;
        }
            break;
	case DFNT_FLOAT64:

        {
            std::vector < float64 > latlon64;
            r = GetLatLon (swathid, fieldname, dimmaps, latlon64,
                &majordimsize, &minordimsize);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            float64 *outlatlon64 = new float64[nelms];

            LatLonSubset (outlatlon64, majordimsize, minordimsize,
                &latlon64[0], offset32, count32, step32);
            set_value ((dods_float64 *) outlatlon64, nelms);
            delete[]outlatlon64;
        }
            break;
        default:
        {
            HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
            detachfunc (swathid);
            closefunc (fileid);
            InternalErr (__FILE__, __LINE__, "unsupported data type.");
        }
    }

    r = detachfunc (swathid);
    if (r != 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        closefunc (fileid);
        ostringstream eherr;

        eherr << "Grid/Swath " << datasetname.c_str () << " cannot be detached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    r = closefunc (fileid);
    if (r != 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        ostringstream eherr;

        eherr << "Grid/Swath " << filename.c_str () << " cannot be closed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);

    return false;
}

// parse constraint expr.
// return number of elements to read. 
int
HDFEOS2ArraySwathGeoDimMapField::format_constraint (int *offset, int *step, int *count)
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
        count[id] = ((stop - start) / stride) + 1; // count of elements
        nels *= count[id]; // total number of values for variable

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

// Get latitude and longitude fields. It will call expand_dimmap_field to interpolate latitude and longitude.
template < class T > int
HDFEOS2ArraySwathGeoDimMapField::HDFEOS2ArraySwathGeoDimMapField::
GetLatLon (int32 swathid, const std::string & geofieldname,
    std::vector < struct dimmap_entry >&dimmaps,
    std::vector < T > &vals, int32 * ydim, int32 * xdim)
{

    int32 ret = 0;
    int32 size = 0;
    int32 rank = 0;
    int32 dims[130];
    int32 type = 0;

    // Two dimensions for lat/lon; each dimension name is < 64 characters,
    // The dimension names are separated by a comma.
    char dimlist[130];
    ret = SWfieldinfo (swathid, const_cast < char *>(geofieldname.c_str ()),
        &rank, dims, &type, dimlist);
    if (ret != 0)
        return -1;

    size = 1;
    for (int i = 0; i < rank; i++)
        size *= dims[i];

    vals.resize (size);

    ret = SWreadfield (swathid, const_cast < char *>(geofieldname.c_str ()),
        NULL, NULL, NULL, (void *) &vals[0]);
    if (ret != 0)
        return -1;

    std::vector < std::string > dimname;
    HDFCFUtil::Split (dimlist, ',', dimname);

    for (int i = 0; i < rank; i++) {
        std::vector < struct dimmap_entry >::iterator it;

        for (it = dimmaps.begin (); it != dimmaps.end (); it++) {
            if (it->geodim == dimname[i]) {
                int32 ddimsize = SWdiminfo (swathid, (char *) it->datadim.c_str ());
                if (ddimsize == -1)
                    return -1;
                int r; //[LD Comment 11/14/2012]

                r = _expand_dimmap_field (&vals, rank, dims, i, ddimsize, it->offset, it->inc);
                if (r != 0)
                    return -1;
            }
        }
    }

    if (rank == 2) {
            
        *ydim = dims[0];
        *xdim = dims[1];
        if (dims[0] <0 || dims[1] <0)
            return -1;
    }

    if (rank == 1){
        *ydim = dims[0];
        *xdim = -1;
        if (dims[0] <0)
            return -1;
    }

    return 0;
}

// expand the dimension map field.
template < class T > int
HDFEOS2ArraySwathGeoDimMapField::_expand_dimmap_field (std::vector < T >  *pvals, int32 rank, int32 dimsa[], int dimindex, int32 ddimsize, int32 offset, int32 inc)
{
    std::vector < T > orig = *pvals;
    std::vector < int32 > pos;
    std::vector < int32 > dims;
    std::vector < int32 > newdims;
    pos.resize (rank);
    dims.resize (rank);

    for (int i = 0; i < rank; i++) {
        pos[i] = 0;
        dims[i] = dimsa[i];
    }
    newdims = dims;
    newdims[dimindex] = ddimsize;
    dimsa[dimindex] = ddimsize;

    int newsize = 1;

    for (int i = 0; i < rank; i++) {
        newsize *= newdims[i];
    }
    pvals->clear ();
    pvals->resize (newsize);

    for (;;) {
        // if end
        if (pos[0] == dims[0]) {
            // we past then end
            break;
        }
        else if (pos[dimindex] == 0) {
            // extract 1D values
            std::vector < T > v;
            for (int i = 0; i < dims[dimindex]; i++) {
                pos[dimindex] = i;
                v.push_back (orig[INDEX_nD_TO_1D (dims, pos)]);
            }
            // expand them

            std::vector < T > w;
            for (int32 j = 0; j < ddimsize; j++) {
                int32 i = (j - offset) / inc;
                T f;

                if (i * inc + offset == j)	// perfect match
                {
                    f = (v[i]);
                }
                else {
                    int32 i1, i2, j1, j2;

                    if (i <= 0) {
                        i1 = 0;
                        i2 = 1;
                    }
                    if ((unsigned int) i + 1 >= v.size ()) {
                        i1 = v.size () - 2;
                        i2 = v.size () - 1;
                    }
                    else {
                        i1 = i;
                        i2 = i + 1;
                    }
                    j1 = i1 * inc + offset;
                    j2 = i2 * inc + offset;
                    f = (((j - j1) * v[i2] + (j2 - j) * v[i1]) / (j2 - j1));
                }
                w.push_back (f);
                pos[dimindex] = j;
                (*pvals)[INDEX_nD_TO_1D (newdims, pos)] = f;
            }
            pos[dimindex] = 0;
        }
        // next pos
        pos[rank - 1]++;
        for (int i = rank - 1; i > 0; i--) {
            if (pos[i] == dims[i]) {
                pos[i] = 0;
                pos[i - 1]++;
            }
        }
    }

    return 0;
}

template < class T >
bool HDFEOS2ArraySwathGeoDimMapField::LatLonSubset (T * outlatlon, int majordim, int minordim, T * latlon,  int32 * offset, int32 * count, int32 * step)
{
    if (minordim < 0)
        return LatLon1DSubset(outlatlon,majordim,latlon,offset,count,step);
    else
        return LatLon2DSubset(outlatlon,majordim,minordim,latlon,offset,count,step);

}

// Subset of 1-D field to follow the parameters from the DAP expression constraint
template < class T >
bool HDFEOS2ArraySwathGeoDimMapField::LatLon1DSubset (T * outlatlon, int majordim, T * latlon, int32 * offset, int32 * count, int32 * step)
{
    if (majordim < count[0]) 
        throw InternalErr(__FILE__, __LINE__,
                          "The number of elements is greater than the total dimensional size");

    for (int i = 0; i < count[0]; i++) 
        outlatlon[i] = latlon[offset[0]+i*step[0]];
    return true;

}

// Subset of latitude and longitude to follow the parameters from the DAP expression constraint
template < class T >
bool HDFEOS2ArraySwathGeoDimMapField::LatLon2DSubset (T * outlatlon, int majordim, int minordim, T * latlon, int32 * offset, int32 * count, int32 * step)
{

    // float64 templatlon[majordim][minordim];
    T (*templatlonptr)[majordim][minordim] = (typeof templatlonptr) latlon;
    int i, j, k;

    // do subsetting
    // Find the correct index
    int	dim0count = count[0];
    int dim1count = count[1];

    int dim0index[dim0count], dim1index[dim1count];

    for (i = 0; i < count[0]; i++) // count[0] is the least changing dimension
        dim0index[i] = offset[0] + i * step[0];


    for (j = 0; j < count[1]; j++)
        dim1index[j] = offset[1] + j * step[1];

    // Now assign the subsetting data
    k = 0;

    for (i = 0; i < count[0]; i++) {
        for (j = 0; j < count[1]; j++) {
            outlatlon[k] = (*templatlonptr)[dim0index[i]][dim1index[j]];
            k++;
        }
    }
    return true;
}
#endif
