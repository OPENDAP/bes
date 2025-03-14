/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath having dimension maps
//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////
// SOME MODIS products provide the latitude and longitude files for
// swaths that use dimension maps. The files are still HDF-EOS2 files.
// The name of such a file is determined at the hdfdesc.cc.
// Since the latitude and longitude fields are stored as 
// the real data fields in an HDF-EOS2 file, 
// The read function is essentially the same as retrieving the data value of a
// general HDF-EOS2 field. 

#ifdef USE_HDFEOS2_LIB
#include "config.h"
#include "HDFEOS2ArraySwathGeoDimMapExtraField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include "HDFEOS2.h"
#include "HDFCFUtil.h"
#include <libdap/InternalErr.h>
#include "BESDebug.h"

using namespace std;
using namespace libdap;

#define SIGNED_BYTE_TO_INT32 1


bool
HDFEOS2ArraySwathGeoDimMapExtraField::read ()
{

    BESDEBUG("h4","Coming to HDFEOS2ArraySwathGeoDimMapExtraField read "<<endl);

    if (length() == 0)
        return true;

    // Declare offset, count and step
    vector<int>offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = format_constraint(offset.data(),step.data(),count.data());

    // Just declare offset,count and step in the int32 type.
    vector<int32>offset32;
    offset32.resize(rank);
    vector<int32>count32;
    count32.resize(rank);
    vector<int32>step32;
    step32.resize(rank);

    // Just obtain the offset,count and step in the datatype of int32.
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


    // Define function pointers to handle the swath
    openfunc = SWopen;
    closefunc = SWclose;
    attachfunc = SWattach;
    detachfunc = SWdetach;
    fieldinfofunc = SWfieldinfo;
    readfieldfunc = SWreadfield;
    inqfunc = SWinqswath;

    // We may eventually combine the following code with other code, so
    // we don't add many comments from here to the end of the file. 
    // The jira ticket about combining code is HFRHANDLER-166.
    int32 fileid = -1;
    int32 swathid = -1; 

    fileid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
    if (fileid < 0) {
        ostringstream eherr;
        eherr << "File " << filename.c_str () << " cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Check if this file only contains one swath
    int numswath = 0;
    int32 swathnamesize = 0; 
    numswath = inqfunc (const_cast < char *>(filename.c_str ()), NULL,
                        &swathnamesize);

    if (numswath == -1) {
        closefunc (fileid);
        ostringstream eherr;
        eherr << "File " << filename.c_str () << " cannot obtain the swath list.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if (numswath != 1) {
        closefunc (fileid);
        ostringstream eherr;
        eherr << " Currently we only support reading geo-location fields from one swath."
              << " This file has more than one swath. ";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    vector<char> swathname(swathnamesize+1);
    numswath = inqfunc (const_cast < char *>(filename.c_str ()), swathname.data(),
                        &swathnamesize);
    if (numswath == -1) {
        closefunc (fileid);
        ostringstream eherr;
        eherr << "File " << filename.c_str () << " cannot obtain the swath list.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    swathid = attachfunc (fileid, swathname.data());
    if (swathid < 0) {
        closefunc (fileid);
        ostringstream eherr;
        string swname_str(swathname.begin(),swathname.end());
        eherr << "Grid/Swath " << swname_str.c_str() << " cannot be attached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    int32 tmp_rank = 0;
    int32 tmp_dims[rank];
    char tmp_dimlist[1024];
    int32 type = 0;
    intn r = -1;
    r = fieldinfofunc (swathid, const_cast < char *>(fieldname.c_str ()),
                       &tmp_rank, tmp_dims, &type, tmp_dimlist);
    if (r != 0) {
        detachfunc (swathid);
        closefunc (fileid);
        ostringstream eherr;
        eherr << "Field " << fieldname.c_str () << " information cannot be obtained.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    switch (type) {

        case DFNT_INT8:
        {
            vector<int8>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), 
                               offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

#ifndef SIGNED_BYTE_TO_INT32
            set_value ((dods_byte *) val.data(), nelms);
#else

            vector<int32>newval;
            newval.resize(nelms);
            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (val[counter]);

            set_value ((dods_int32 *) newval.data(), nelms);
#endif
        }

            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        {
            vector<uint>val;
            val.resize(nelms);

            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), 
                               offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_byte *) val.data(), nelms);
        }
            break;

        case DFNT_INT16:
        {
            vector<int16>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), 
                               offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_int16 *) val.data(), nelms);
        }
            break;
        case DFNT_UINT16:
        {
            vector<uint16>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), 
                               offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_uint16 *) val.data(), nelms);
        }
            break;
        case DFNT_INT32:
        {
            vector<int32>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), 
                               offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_int32 *) val.data(), nelms);
        }
            break;
        case DFNT_UINT32:
        {
            vector<uint32>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), 
                               offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_uint32 *) val.data(), nelms);
        }
            break;
        case DFNT_FLOAT32:
        {
            vector<float32>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), 
                               offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_float32 *) val.data(), nelms);
        }
            break;
        case DFNT_FLOAT64:
        {
            vector<float64>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), 
                               offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                closefunc (fileid);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            set_value ((dods_float64 *) val.data(), nelms);
        }
            break;
        default: 
        {
            detachfunc(swathid);
            closefunc(fileid);
            throw InternalErr (__FILE__, __LINE__, "unsupported data type.");
        }
    }

    r = detachfunc (swathid);
    if (r != 0) {
        closefunc (fileid);
        throw InternalErr (__FILE__, __LINE__, "The swath cannot be detached.");
    }


    r = closefunc (fileid);
    if (r != 0) {
        ostringstream eherr;

        eherr << "Grid/Swath " << filename.c_str () << " cannot be closed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    return false;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFEOS2ArraySwathGeoDimMapExtraField::format_constraint (int *offset, int *step, int *count)
{
    int nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();
    while (p != dim_end ()) {

        int start = dimension_start (p, true);
        int stride = dimension_stride (p, true);
        int stop = dimension_stop (p, true);

        // Check for illegal  constraint
        if (start > stop) {
            ostringstream oss;
            oss << "Array/Grid hyperslab start point "<< start <<
                   " is greater than stop point " <<  stop <<".";
            throw Error(malformed_expr, oss.str());
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1;      // count of elements
        nels *= count[id];              // total number of values for variable

        BESDEBUG ("h4",
                         "=format_constraint():"
                         << "id=" << id << " offset=" << offset[id]
                         << " step=" << step[id]
                         << " count=" << count[id]
                         << endl);

        id++;
        p++;
    }// while 

    return nels;
}

#endif
