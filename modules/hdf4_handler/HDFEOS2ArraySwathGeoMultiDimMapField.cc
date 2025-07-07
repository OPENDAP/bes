/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath without using dimension maps
//
//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////
//For the swath using the multiple dimension maps,
// Latitude/longitude will be interpolated accordingly.

#ifdef USE_HDFEOS2_LIB

#include "HDFEOS2ArraySwathGeoMultiDimMapField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include <libdap/InternalErr.h>
#include <BESInternalError.h>
#include "BESDebug.h"
#include "HDF4RequestHandler.h"

using namespace std;
using namespace libdap;
#define SIGNED_BYTE_TO_INT32 1

bool
HDFEOS2ArraySwathGeoMultiDimMapField::read ()
{

    BESDEBUG("h4","Coming to HDFEOS2ArraySwathGeoMultiDimMapField read "<<endl);

    if (length() == 0)
        return true; 

    if (rank !=2) 
        throw BESInternalError("The field rank must be 2 for swath multi-dimension map reading.",__FILE__, __LINE__);

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

    // Declare dimension map  size, offset and inc
    vector<int>dm_dimsizes;
    dm_dimsizes.resize(rank);
    vector<int>dm_offsets;
    dm_offsets.resize(rank);
    vector<int>dm_incs;
    dm_incs.resize(rank);
    bool no_interpolation = true;

    if (dim0inc != 0 || dim1inc !=0 || dim0offset != 0 || dim1offset != 0) {
        dm_dimsizes[0] = dim0size;
        dm_dimsizes[1] = dim1size;
        dm_offsets[0] = dim0offset;
        dm_offsets[1] = dim1offset;
        dm_incs[0] = dim0inc;
        dm_incs[1] = dim1inc;
        no_interpolation = false;
    }

    // We may eventually combine the following code with other code, so
    // we don't add many comments from here to the end of the file. 
    // The jira ticket about combining code is HFRHANDLER-166.
    int32 sfid = -1;
    int32 swathid = -1;

    if (false == check_pass_fileid_key) {
        sfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (sfid < 0) {
            string msg = "File " + filename + " cannot be open.";
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

    vector<int32> newdims;
    newdims.resize(tmp_rank);


    switch (type) {
        case DFNT_INT8:
        {
            if (true == no_interpolation) {
                vector<int8>val;
                val.resize(nelms);
                r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
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
            else {
                // Obtaining the total value and interpolating the data 
                // according to dimension map
                vector <int8> total_val8;
                r = GetFieldValue (swathid, fieldname, dm_dimsizes,dm_offsets,dm_incs, total_val8, newdims);

                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);

                }
                vector<int8>val8;
                val8.resize(nelms);
                Field2DSubset (val8.data(), newdims[0], newdims[1],total_val8.data(),
                                 offset32.data(), count32.data(), step32.data());

#ifndef SIGNED_BYTE_TO_INT32
                set_value ((dods_byte *) val8.data(), nelms);
#else
                vector<int32>newval;
                newval.resize(nelms);

                for (int counter = 0; counter < nelms; counter++)
                    newval[counter] = (int32) (val8[counter]);
                set_value ((dods_int32 *) newval.data(), nelms);
#endif
            }
        }
            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        {
            if (no_interpolation == false) {
                vector <uint8> total_val_uint8;
                r = GetFieldValue (swathid, fieldname, dm_dimsizes,dm_offsets,dm_incs, total_val_uint8, newdims);

                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);

                }
                vector<uint8>val_uint8;
                val_uint8.resize(nelms);
                Field2DSubset (val_uint8.data(), newdims[0], newdims[1],total_val_uint8.data(),
                             offset32.data(), count32.data(), step32.data());

                set_value ((dods_byte *) val_uint8.data(), nelms);
            }
            else {
 
                vector<uint8>val;
                val.resize(nelms);
                r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                set_value ((dods_byte *) val.data(), nelms);
            }
        }
            break;

        case DFNT_INT16:
        {
            if (no_interpolation == false) {
                vector <int16> total_val_int16;
                r = GetFieldValue (swathid, fieldname, dm_dimsizes,dm_offsets,dm_incs, total_val_int16, newdims);

                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);

                }
                vector<int16>val_int16;
                val_int16.resize(nelms);
                Field2DSubset (val_int16.data(), newdims[0], newdims[1],total_val_int16.data(),
                             offset32.data(), count32.data(), step32.data());

                set_value ((dods_int16 *) val_int16.data(), nelms);
            }

            else {
                vector<int16>val;
                val.resize(nelms);
                r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                set_value ((dods_int16 *) val.data(), nelms);
            }
        }
            break;

        case DFNT_UINT16:
        {
            if (no_interpolation == false) {
                vector <uint16> total_val_uint16;
                r = GetFieldValue (swathid, fieldname, dm_dimsizes,dm_offsets,dm_incs, total_val_uint16, newdims);

                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);

                }
                vector<uint16>val_uint16;
                val_uint16.resize(nelms);
                Field2DSubset (val_uint16.data(), newdims[0], newdims[1],total_val_uint16.data(),
                             offset32.data(), count32.data(), step32.data());

                set_value ((dods_uint16 *) val_uint16.data(), nelms);
            }

            else {
                vector<uint16>val;
                val.resize(nelms);
                r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                set_value ((dods_uint16 *) val.data(), nelms);
            }
        }
            break;

        case DFNT_INT32:
        {
            if (no_interpolation == false) {
                vector <int32> total_val_int32;
                r = GetFieldValue (swathid, fieldname, dm_dimsizes,dm_offsets,dm_incs, total_val_int32, newdims);

                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);

                }
                vector<int32>val_int32;
                val_int32.resize(nelms);
                Field2DSubset (val_int32.data(), newdims[0], newdims[1],total_val_int32.data(),
                             offset32.data(), count32.data(), step32.data());

                set_value ((dods_int32 *) val_int32.data(), nelms);
            }
            else {
                vector<int32>val;
                val.resize(nelms);
                r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                set_value ((dods_int32 *) val.data(), nelms);
            }
        }
            break;

        case DFNT_UINT32:
        {
            if (no_interpolation == false) {
                vector <uint32> total_val_uint32;
                r = GetFieldValue (swathid, fieldname, dm_dimsizes,dm_offsets,dm_incs, total_val_uint32, newdims);

                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);

                }
                vector<uint32>val_uint32;
                val_uint32.resize(nelms);
                Field2DSubset (val_uint32.data(), newdims[0], newdims[1],total_val_uint32.data(),
                             offset32.data(), count32.data(), step32.data());

                set_value ((dods_uint32 *) val_uint32.data(), nelms);
            }
            else {
                vector<uint32>val;
                val.resize(nelms);
                r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                set_value ((dods_uint32 *) val.data(), nelms);
            }
        }
            break;
        case DFNT_FLOAT32:
        {
            if (no_interpolation == false) {
                vector <float32> total_val_f32;
                r = GetFieldValue (swathid, fieldname, dm_dimsizes,dm_offsets,dm_incs, total_val_f32, newdims);

                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);

                }
                vector<float32>val_f32;
                val_f32.resize(nelms);
                Field2DSubset (val_f32.data(), newdims[0], newdims[1],total_val_f32.data(),
                             offset32.data(), count32.data(), step32.data());

                set_value ((dods_float32 *) val_f32.data(), nelms);
            }
            else {
                vector<float32>val;
                val.resize(nelms);
                r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                set_value ((dods_float32 *) val.data(), nelms);
            }
        }
            break;
        case DFNT_FLOAT64:
        {
            if (no_interpolation == false) {
                vector <float64> total_val_f64;
                r = GetFieldValue (swathid, fieldname, dm_dimsizes,dm_offsets,dm_incs, total_val_f64, newdims);

                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);

                }
                vector<float64>val_f64;
                val_f64.resize(nelms);
                Field2DSubset (val_f64.data(), newdims[0], newdims[1],total_val_f64.data(),
                             offset32.data(), count32.data(), step32.data());

                set_value ((dods_float64 *) val_f64.data(), nelms);
            }
            else {
                vector<float64>val;
                val.resize(nelms);
                r = readfieldfunc (swathid, const_cast < char *>(fieldname.c_str ()), offset32.data(), step32.data(), count32.data(), val.data());
                if (r != 0) {
                    detachfunc (swathid);
                    HDFCFUtil::close_fileid(-1,-1,-1,sfid,check_pass_fileid_key);
                    string msg = "Field " + fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                set_value ((dods_float64 *) val.data(), nelms);
            }
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
HDFEOS2ArraySwathGeoMultiDimMapField::format_constraint (int *offset, int *step, int *count)
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

// Subset of latitude and longitude to follow the parameters 
// from the DAP expression constraint
template < class T >
bool HDFEOS2ArraySwathGeoMultiDimMapField::Field2DSubset (T * outlatlon,
                                                  const int ,
                                                  const int minordim,
                                                  T * latlon,
                                                  const int32 * offset,
                                                  const int32 * count,
                                                  const int32 * step) const
{

    int	i = 0;
    int j = 0; 

    // do subsetting
    // Find the correct index
    int	dim0count = count[0];
    int dim1count = count[1];

    int	dim0index[dim0count];
    int dim1index[dim1count];

    for (i = 0; i < count[0]; i++) // count[0] is the least changing dimension
        dim0index[i] = offset[0] + i * step[0];


    for (j = 0; j < count[1]; j++)
        dim1index[j] = offset[1] + j * step[1];

    // Now assign the subsetting data
    int k = 0;

    for (i = 0; i < count[0]; i++) {
        for (j = 0; j < count[1]; j++) {
            outlatlon[k] = *(latlon + (dim0index[i] * minordim) + dim1index[j]);
            k++;
        }
    }
    return true;
}

// Get latitude and longitude fields. 
// It will call expand_dimmap_field to interpolate latitude and longitude.
template < class T > int
HDFEOS2ArraySwathGeoMultiDimMapField::
GetFieldValue (int32 swathid, const string & geofieldname,
    const vector <int>&dimsizes,const vector<int>&offset,const vector<int>&inc,
    vector < T > &vals, vector<int32>&newdims) 
{

    int32 ret = -1; 
    int32 size = -1;
    int32 sw_rank = -1;
    int32 dims[130];
    int32  type = -1;

    // Two dimensions for lat/lon; each dimension name is < 64 characters,
    // The dimension names are separated by a comma.
    char dimlist[130];
    ret = SWfieldinfo (swathid, const_cast < char *>(geofieldname.c_str ()),
        &sw_rank, dims, &type, dimlist);
    if (ret != 0)
        return -1;

    if (sw_rank !=2)
        return -1;

    size = 1;
    for (int i = 0; i <sw_rank; i++)
        size *= dims[i];

    vals.resize (size);

    ret = SWreadfield (swathid, const_cast < char *>(geofieldname.c_str ()),
                       NULL, NULL, NULL, (void *) vals.data());
    if (ret != 0)
        return -1;

    vector < string > dimname;
    HDFCFUtil::Split (dimlist, ',', dimname);

    for (int i = 0; i < sw_rank; i++) {

        int r;
        r = _expand_dimmap_field (&vals, sw_rank, dims, i, dimsizes[i], offset[i], inc[i]);
        if (r != 0)
            return -1;
    }

    // dims[] are expanded already.
    for (int i = 0; i < sw_rank; i++) { 
        //cerr<<"i "<< i << " "<< dims[i] <<endl;
        if (dims[i] < 0)
            return -1;
        newdims[i] = dims[i];
    }

    return 0;
}

// expand the dimension map field.
template < class T > int
HDFEOS2ArraySwathGeoMultiDimMapField::_expand_dimmap_field (vector < T >
                                                    *pvals, int32 sw_rank,
                                                    int32 dimsa[],
                                                    int dimindex,
                                                    int32 ddimsize,
                                                    int32 offset,
                                                    int32 inc) const
{
    vector < T > orig = *pvals;
    vector < int32 > pos;
    vector < int32 > dims;
    vector < int32 > newdims;
    pos.resize (sw_rank);
    dims.resize (sw_rank);

    for (int i = 0; i < sw_rank; i++) {
        pos[i] = 0;
        dims[i] = dimsa[i];
    }
    newdims = dims;
    newdims[dimindex] = ddimsize;
    dimsa[dimindex] = ddimsize;

    int newsize = 1;

    for (int i = 0; i < sw_rank; i++) {
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
            vector < T > v;
            for (int i = 0; i < dims[dimindex]; i++) {
                pos[dimindex] = i;
                v.push_back (orig[INDEX_nD_TO_1D (dims, pos)]);
            }
            // expand them

            vector < T > w;
            for (int32 j = 0; j < ddimsize; j++) {
                int32 i = (j - offset) / inc;
                T f;

                if (i * inc + offset == j) // perfect match
                {
                    f = (v[i]);
                }
                else {
                    int32 i1 = 0;
                    int32 i2 = (i<=0)?1:0;
                    int32 j1 = 0;
                    int32 j2 = 0; 

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
        pos[sw_rank - 1]++;
        for (int i = sw_rank - 1; i > 0; i--) {
            if (pos[i] == dims[i]) {
                pos[i] = 0;
                pos[i - 1]++;
            }
        }
    }

    return 0;
}


#endif
