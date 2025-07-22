/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath without using dimension maps
//
//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////
// For the swath without using dimension maps, for most cases, the retrieving of latitude and
// longitude is the same as retrieving other fields. Some MODIS latitude and longitude need
// to be arranged specially.

#ifdef USE_HDFEOS2_LIB

#include "config.h"

#include "HDFEOS2ArraySwathGeoField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include <BESInternalError.h>
#include "BESDebug.h"
#include "HDF4RequestHandler.h"

using namespace std;
using namespace libdap;
#define SIGNED_BYTE_TO_INT32 1

bool
HDFEOS2ArraySwathGeoField::read ()
{

    BESDEBUG("h4","Coming to HDFEOS2ArraySwathGeoField read "<<endl);

    if (length() == 0)
        return true; 

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    // Declare offset, count and step
    vector<int>offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int  nelms = format_constraint (offset.data(), step.data(), count.data());

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
    int32 (*attachfunc) (int32, char *);
    intn (*detachfunc) (int32);
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);
    intn (*readfieldfunc) (int32, char *, int32 *, int32 *, int32 *, void *);

    // Define function pointers to handle the swath
    string datasetname;
    openfunc = SWopen;
    attachfunc = SWattach;
    detachfunc = SWdetach;
    fieldinfofunc = SWfieldinfo;
    readfieldfunc = SWreadfield;
    datasetname = swathname;

    // We may eventually combine the following code with other code, so
    // we don't add many comments from here to the end of the file. 
    // The jira ticket about combining code is HFRHANDLER-166.
    int32 sfid = -1;
    int32 swathid = -1;

    if (false == check_pass_fileid_key) {
        sfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (sfid < 0) {
            string msg = "File "  + filename + "cannot be open.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }
    else 
        sfid = swathfd;

    swathid = attachfunc (sfid, const_cast < char *>(datasetname.c_str ()));

    if (swathid < 0) {
        HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
        string msg = "Swath " + datasetname + " cannot be attached.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    int32 tmp_rank = -1;
    vector<int32>tmp_dims;
    tmp_dims.resize(rank);
    char tmp_dimlist[1024];
    int32 type =-1;
    intn r = -1;
    r = fieldinfofunc (swathid, const_cast < char *>(fieldname.c_str ()),
        &tmp_rank, tmp_dims.data(), &type, tmp_dimlist);
    if (r != 0) {
        detachfunc (swathid);
        HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
        string msg = "Field " + fieldname + " information cannot be obtained.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    switch (type) {
        case DFNT_INT8:
        {
            vector<int8>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                string msg = "Field " + fieldname +  "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
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
            vector<uint8>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                string msg = "Field " + fieldname +  "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            set_value ((dods_byte *) val.data(), nelms);
        }
            break;

        case DFNT_INT16:
        {
            vector<int16>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                string msg = "Field " + fieldname +  "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
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
                float scale_factor = 0.01f;

                for (int i = 0; i < nelms; i++)
                    newval[i] = scale_factor * (float32) val[i];
                set_value ((dods_float32 *) newval.data(), nelms);
            }

            else {
                set_value ((dods_int16 *) val.data(), nelms);
            }
        }
            break;

        case DFNT_UINT16:
        {
            vector<uint16>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                string msg = "Field " + fieldname +  "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            set_value ((dods_uint16 *) val.data(), nelms);
        }
            break;

        case DFNT_INT32:
        {
            vector<int32>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                string msg = "Field " + fieldname +  "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            set_value ((dods_int32 *) val.data(), nelms);
        }
            break;

        case DFNT_UINT32:
        {
            vector<uint32>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                string msg = "Field " + fieldname +  "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            set_value ((dods_uint32 *) val.data(), nelms);
        }
            break;
        case DFNT_FLOAT32:
        {

            vector<float32>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                string msg = "Field " + fieldname +  "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value ((dods_float32 *) val.data(), nelms);
        }
            break;
        case DFNT_FLOAT64:
        {
            vector<float64>val;
            val.resize(nelms);
            r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                detachfunc (swathid);
                HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                string msg = "Field " + fieldname +  "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            set_value ((dods_float64 *) val.data(), nelms);
        }
            break;
        default:
            detachfunc (swathid);
            HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
            throw BESInternalError("Unsupported data type.",__FILE__, __LINE__);
    }

    r = detachfunc (swathid);
    if (r != 0) {
        HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
        string msg = "Swath " + datasetname + " cannot be detached.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);

    return false;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFEOS2ArraySwathGeoField::format_constraint (int *offset, int *step, int *count)
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
    }// end of while 

    return nels;
}
#endif
