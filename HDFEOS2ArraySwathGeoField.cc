/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath without using dimension maps
//
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
// For the swath without using dimension maps, for most cases, the retrieving of latitude and
// longitude is the same as retrieving other fields. Some MODIS latitude and longitude need
// to be arranged specially.

#ifdef USE_HDFEOS2_LIB

#include "HDFEOS2ArraySwathGeoField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "InternalErr.h"
#include "BESDebug.h"

using namespace std;
#define SIGNED_BYTE_TO_INT32 1

bool
HDFEOS2ArraySwathGeoField::read ()
{

    BESDEBUG("h4","Coming to HDFEOS2ArraySwathGeoField read "<<endl);

    string check_pass_fileid_key_str="H4.EnablePassFileID";
    bool check_pass_fileid_key = false;
    check_pass_fileid_key = HDFCFUtil::check_beskeys(check_pass_fileid_key_str);

    // Declare offset, count and step
    vector<int>offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int  nelms = format_constraint (&offset[0], &step[0], &count[0]);

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

    // Define function pointers to handle the swath
    string datasetname;
    openfunc = SWopen;
    closefunc = SWclose;
    attachfunc = SWattach;
    detachfunc = SWdetach;
    fieldinfofunc = SWfieldinfo;
    readfieldfunc = SWreadfield;
    datasetname = swathname;

    // We may eventually combine the following code with other code, so
    // we don't add many comments from here to the end of the file. 
    // The jira ticket about combining code is HFRHANDLER-166.
    int32 sfid = -1, swathid = -1;

    if(false == check_pass_fileid_key) {
        sfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (sfid < 0) {
            ostringstream eherr;
            eherr << "File " << filename.c_str () << " cannot be open.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }
    }
    else 
        sfid = swathfd;

    swathid = attachfunc (sfid, const_cast < char *>(datasetname.c_str ()));

    if (swathid < 0) {
        HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
        ostringstream eherr;
        eherr << "Swath " << datasetname.c_str () << " cannot be attached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    int32 tmp_rank, tmp_dims[rank];
    char tmp_dimlist[1024];
    int32 type;
    intn r;
    r = fieldinfofunc (swathid, const_cast < char *>(fieldname.c_str ()),
        &tmp_rank, tmp_dims, &type, tmp_dimlist);
    if (r != 0) {
        detachfunc (swathid);
        HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
        ostringstream eherr;
        eherr << "Field " << fieldname.c_str () << " information cannot be obtained.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    switch (type) {
        case DFNT_INT8:
        {
            vector<int8>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }


#ifndef SIGNED_BYTE_TO_INT32
            set_value ((dods_byte *) &val[0], nelms);
#else
            vector<int32>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (val[counter]);
            set_value ((dods_int32 *) &newval[0], nelms);
#endif
        }
            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        {
            vector<uint8>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_byte *) &val[0], nelms);
        }
            break;

        case DFNT_INT16:
        {
            vector<int16>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            // We found a special MODIS file that used int16 to store lat/lon. The scale-factor is 0.01.
            // We cannot retrieve attributes from EOS. To make this work, we have to manually check the values of lat/lon;
            // If lat/lon value is in thousands, we will hard code by applying the scale factor to calculate the latitude and longitude.  
            // We will see and may find a good solution in the future. KY 2010-7-12
            bool needadjust = false;

            if ((val[0] < -1000) || (val[0] > 1000))
                needadjust = true;
            if (!needadjust)
                if ((val[nelms / 2] < -1000)
                    || (val[nelms / 2] > 1000))
                    needadjust = true;
            if (!needadjust)
                if ((val[nelms - 1] < -1000)
                    || (val[nelms - 1] > 1000))
                    needadjust = true;
            if (true == needadjust) {
                vector<float32>newval;
                newval.resize(nelms);
                float scale_factor = 0.01;

                for (int i = 0; i < nelms; i++)
                    newval[i] = scale_factor * (float32) val[i];
                set_value ((dods_float32 *) &newval[0], nelms);
            }

            else {
                set_value ((dods_int16 *) &val[0], nelms);
            }
        }
            break;

        case DFNT_UINT16:
        {
            vector<uint16>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_uint16 *) &val[0], nelms);
        }
            break;

        case DFNT_INT32:
        {
            vector<int32>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_int32 *) &val[0], nelms);
        }
            break;

        case DFNT_UINT32:
        {
            vector<uint32>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_uint32 *) &val[0], nelms);
        }
            break;
        case DFNT_FLOAT32:
        {

            vector<float32>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            set_value ((dods_float32 *) &val[0], nelms);
        }
            break;
        case DFNT_FLOAT64:
        {
            vector<float64>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            set_value ((dods_float64 *) &val[0], nelms);
        }
            break;
        default:
            detachfunc (swathid);
            HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
            InternalErr (__FILE__, __LINE__, "unsupported data type.");
    }

    r = detachfunc (swathid);
    if (r != 0) {
        HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
        ostringstream eherr;
        eherr << "Swath " << datasetname.c_str () << " cannot be detached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);

#if 0
    r = closefunc (sfid);
    if (r != 0) {
        ostringstream eherr;
        eherr << "Swath " << filename.c_str () << " cannot be closed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }
#endif

    return false;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFEOS2ArraySwathGeoField::format_constraint (int *offset, int *step, int *count)
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
