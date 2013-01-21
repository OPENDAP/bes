/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath having dimension maps
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
// SOME MODIS products provide the latitude and longitude files for
// swaths using dimension map. The files are still HDF-EOS2 files.
// The file name is determined at the hdfdesc.cc.
// Since the latitude and longitude fields are stored as 
// the real data fields in an HDF-EOS2 file, this file is just
// like the HDFEOS_RealField.cc.

#ifdef USE_HDFEOS2_LIB
#include "HDFEOS2ArraySwathGeoDimMapExtraField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "HDFEOS2.h"
#include "HDFCFUtil.h"
#include "InternalErr.h"
#include "BESDebug.h"
#define SIGNED_BYTE_TO_INT32 1


bool
HDFEOS2ArraySwathGeoDimMapExtraField::read ()
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
    int32 (*inqfunc) (char *, char *, int32 *);


    openfunc = SWopen;
    closefunc = SWclose;
    attachfunc = SWattach;
    detachfunc = SWdetach;
    fieldinfofunc = SWfieldinfo;
    readfieldfunc = SWreadfield;
    inqfunc = SWinqswath;

    //Swath 
    int32 fileid, swathid; //[LD Comment 11/13/2012]

    fileid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);

    if (fileid < 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        ostringstream eherr;

        eherr << "File " << filename.c_str () << " cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Check if this file only contains one swath
    int numswath; //LD Comment 11/13/2012]
    int32 swathnamesize; //[LD Comment 11/13/2012]
    numswath = inqfunc (const_cast < char *>(filename.c_str ()), NULL,
                        &swathnamesize);

    if (numswath == -1) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        closefunc (fileid);
        ostringstream eherr;

        eherr << "File " << filename.c_str () << " cannot obtain the swath list.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if (numswath != 1) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        closefunc (fileid);
        ostringstream eherr;

        eherr << " Currently we only support reading geo-location fields from one swath."
              << " This file has more than one swath. ";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    char *swathname = new char[swathnamesize + 1];
    numswath = inqfunc (const_cast < char *>(filename.c_str ()), swathname,
                        &swathnamesize);
    if (numswath == -1) {
        delete[]swathname;
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        closefunc (fileid);
        ostringstream eherr;

        eherr << "File " << filename.c_str () << " cannot obtain the swath list.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    swathid = attachfunc (fileid, swathname);
    if (swathid < 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        closefunc (fileid);
        ostringstream eherr;

        eherr << "Grid/Swath " << swathname << " cannot be attached.";
        delete[]swathname;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    delete[]swathname;

    int32 tmp_rank, tmp_dims[rank]; //[LD Comment 11/13/2012]
    char tmp_dimlist[1024];
    int32 type;
    intn r;
    r = fieldinfofunc (swathid, const_cast < char *>(fieldname.c_str ()),
                       &tmp_rank, tmp_dims, &type, tmp_dimlist);
    if (r != 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        detachfunc (swathid);
        closefunc (fileid);
        ostringstream eherr;

        eherr << "Field " << fieldname.c_str () << " information cannot be obtained.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    void *val; //[LD Comment 11/13/2012]

    switch (type) {

        case DFNT_INT8:
        {
            val = new int8[nelms];
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                delete[](int8 *) val;
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

#ifndef SIGNED_BYTE_TO_INT32
            set_value ((dods_byte *) val, nelms);
            delete[](int8 *) val;
#else
            int32 *newval; //[LD Comment 11/13/2012]
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
        {
            val = new uint8[nelms];
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                delete[](uint8 *) val;
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_byte *) val, nelms);
            delete[](uint8 *) val;
        }
            break;

        case DFNT_INT16:
        {
            val = new int16[nelms];
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32, step32, count32, val);

            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                delete[](int16 *) val;
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_int16 *) val, nelms);
            delete[](int16 *) val;
        }
            break;
        case DFNT_UINT16:
        {
            val = new uint16[nelms];
	    r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                delete[](uint16 *) val;
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_uint16 *) val, nelms);
            delete[](uint16 *) val;
        }
            break;
        case DFNT_INT32:
        {
            val = new int32[nelms];
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                delete[](int32 *) val;
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_int32 *) val, nelms);
            delete[](int32 *) val;
        }
            break;
        case DFNT_UINT32:
        {
            val = new uint32[nelms];
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                delete[](uint32 *) val;
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_uint32 *) val, nelms);
            delete[](uint32 *) val;
        }
            break;
        case DFNT_FLOAT32:
        {
            val = new float32[nelms];
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                delete[](float32 *) val;
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_float32 *) val, nelms);
            delete[](float32 *) val;
        }
            break;
        case DFNT_FLOAT64:
        {
            val = new float64[nelms];
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                delete[](float64 *) val;
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            set_value ((dods_float64 *) val, nelms);
            delete[](float64 *) val;
        }
            break;
        default:
            InternalErr (__FILE__, __LINE__, "unsupported data type.");
    }

    r = detachfunc (swathid);
    if (r != 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        closefunc (fileid);
        throw InternalErr (__FILE__, __LINE__, "The swath cannot be detached.");
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

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDFEOS2ArraySwathGeoDimMapExtraField::format_constraint (int *offset, int *step, int *count)
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
#endif
