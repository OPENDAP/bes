/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the latitude and longitude fields for some special HDF4 data products.
// The products include TRMML2_V6,TRMML3B_V6,CER_AVG,CER_ES4,CER_CDAY,CER_CGEO,CER_SRB,CER_SYN,CER_ZAVG,OBPGL2,OBPGL3
// To know more information about these products,check HDFSP.h.
// Each product stores lat/lon in different way, so we have to retrieve them differently.
//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFSPArrayGeoField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include "hdf.h"
#include "mfhdf.h"
#include <BESInternalError.h>
#include <BESDebug.h>
#include "HDFCFUtil.h"
#include "HDF4RequestHandler.h"

using namespace std;
using namespace libdap;
#define SIGNED_BYTE_TO_INT32 1

// The following macros provide names of latitude and longitude for specific HDF4 products.
#define NUM_PIXEL_NAME "Pixels per Scan Line"
#define NUM_POINTS_LINE_NAME "Number of Pixel Control Points"
#define NUM_SCAN_LINE_NAME "Number of Scan Lines"
#define NUM_LAT_NAME "Number of Lines"
#define NUM_LON_NAME "Number of Columns"
#define LAT_STEP_NAME "Latitude Step"
#define LON_STEP_NAME "Longitude Step"
#define SWP_LAT_NAME "SW Point Latitude"
#define SWP_LON_NAME "SW Point Longitude"


bool HDFSPArrayGeoField::read ()
{

    BESDEBUG("h4","Coming to HDFSPArrayGeoField read "<<endl);

    if(length() == 0)
        return true; 

    // Declare offset, count and step
    vector<int> offset;
    offset.resize(rank);
    vector<int> count;
    count.resize(rank);
    vector<int> step;
    step.resize(rank);

    // Obtain offset, step and count from the client expression constraint
    int nelms = -1;
    nelms = format_constraint (offset.data(), step.data(), count.data());

    // Just declare offset,count and step in the int32 type.
    vector<int32>offset32;
    offset32.resize(rank);
    vector<int32>count32;
    count32.resize(rank);
    vector<int32>step32;
    step32.resize(rank);


    // Just obtain the offset,count and step in the int32 type.
    for (int i = 0; i < rank; i++) {
        offset32[i] = (int32) offset[i];
        count32[i] = (int32) count[i];
        step32[i] = (int32) step[i];
    }

    // Loop through the functions to obtain lat/lon for the specific HDF4 products the handler 
    // supports.
    switch (sptype) {
   
        // TRMM swath 
        case TRMML2_V6:
        {
            readtrmml2_v6 (offset32.data(), count32.data(), step32.data(), nelms);
            break;
        }

        // TRMM grid
        case TRMML3A_V6:
        {
            readtrmml3a_v6 (offset32.data(), count32.data(), step32.data(), nelms);
            break;
        }

        case TRMML3B_V6:
        {
            readtrmml3b_v6 (offset32.data(), count32.data(), step32.data(), nelms);
            break;
        }

        case TRMML3C_V6:
        {
            readtrmml3c_v6 (offset32.data(), count32.data(), step32.data(), nelms);
            break;
        }


        // TRMM V7 grid
        case TRMML3S_V7:
        case TRMML3M_V7:
        {
            readtrmml3_v7 (offset32.data(),  step32.data(), nelms);
            break;
        }
        // CERES CER_AVG_Aqua-FM3-MODIS,CER_AVG_Terra-FM1-MODIS products
        case CER_AVG:
        {
            readceravgsyn (offset32.data(), count32.data(), step32.data(), nelms);
            break;
        }

        // CERES CER_ES4_??? products
        case CER_ES4:
        {
            readceres4ig (offset32.data(), count32.data(), step32.data(), nelms);
            break;
        }

        // CERES CER_ISCCP-D2like-Day product
        case CER_CDAY:
        {
            readcersavgid2 (offset.data(), count.data(), step.data(), nelms);
            break;
        }

        // CERES CER_ISCCP-D2like-GEO product
        case CER_CGEO:
        {
            readceres4ig (offset32.data(), count32.data(), step32.data(), nelms);
            break;
        }

        // CERES CER_SRBAVG3_Aqua product
        case CER_SRB:
        {
            // When longitude is fixed
            if (rank == 1) {
                readcersavgid1 (offset.data(), count.data(), step.data(), nelms);
            }
            // When longitude is not fixed
            else if (rank == 2) {
                readcersavgid2 (offset.data(), count.data(), step.data(), nelms);
            }
            break;
        }

        // CERES SYN Aqua products
	case CER_SYN:
        {
            readceravgsyn (offset32.data(), count32.data(), step32.data(), nelms);
            break;
        }

        // CERES Zonal Average products
        case CER_ZAVG:
        {
            readcerzavg (offset32.data(), count32.data(), step32.data(), nelms);
            break;
        }

        // OBPG level 2 products
        case OBPGL2:
        {
            readobpgl2 (offset32.data(), count32.data(), step32.data(), nelms);
            break;
        }

        // OBPG level 3 products
        case OBPGL3:
        {
            readobpgl3 (offset.data(),  step.data(), nelms);
            break;
        }

        // We don't handle any OtherHDF products
        case OTHERHDF:
        {
            throw BESInternalError("Unsupported HDF files.",__FILE__, __LINE__);

        }
        default:
        {

            throw BESInternalError("Unsupported HDF files.",__FILE__, __LINE__);

        }
    }

    return true;

}


// Read TRMM level 2 lat/lon. We need to split geolocation field.
// geolocation[YDIM][XDIM][0] is latitude
// geolocation[YDIM][XDIM][1] is longitude

void
HDFSPArrayGeoField::readtrmml2_v6 (const int32 * offset32, const int32 * count32,
                                const int32 * step32, int nelms)
{

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    int32 sdid = -1;

    if(false == check_pass_fileid_key) {
        sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (sdid < 0) {
            string msg = "File " + filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__, __LINE__);
        }
    }
    else 
        sdid = sdfd;

    int32 sdsid = 0;
    
    vector<int32>geooffset32;
    geooffset32.resize(rank+1);

    vector<int32>geocount32;
    geocount32.resize(rank+1);

    vector<int32>geostep32;
    geostep32.resize(rank+1);  

    for (int i = 0; i < rank; i++) {
        geooffset32[i] = offset32[i];
        geocount32[i] = count32[i];
        geostep32[i] = step32[i];
    }

    if (fieldtype == 1) {
        geooffset32[rank] = 0;
        geocount32[rank] = 1;
        geostep32[rank] = 1;
    }

    if (fieldtype == 2) {
        geooffset32[rank] = 1;
        geocount32[rank] = 1;
        geostep32[rank] = 1;
    }

    int32 sdsindex = SDreftoindex (sdid, (int32) fieldref);

    if (sdsindex == -1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string msg =  "SDS index " + to_string(sdsindex) + " is not right.";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    sdsid = SDselect (sdid, sdsindex);
    if (sdsid < 0) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDselect failed.",__FILE__, __LINE__);
    }

    int32 r = 0;

    switch (dtype) {

        case DFNT_INT8:
        {
            vector <int8> val;
            val.resize(nelms);
            r = SDreaddata (sdsid, geooffset32.data(), geostep32.data(), geocount32.data(), (void*)(val.data()));
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
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
            vector<uint8> val;
            val.resize(nelms);
            r = SDreaddata (sdsid, geooffset32.data(), geostep32.data(), geocount32.data(), (void*)(val.data()));
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
            }

            set_value ((dods_byte *) val.data(), nelms);
        }
            break;

        case DFNT_INT16:
        {
            vector<int16> val;
            val.resize(nelms);
            r = SDreaddata (sdsid, geooffset32.data(), geostep32.data(), geocount32.data(), (void*)(val.data()));
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
            }

            set_value ((dods_int16 *) val.data(), nelms);
        }
            break;

        case DFNT_UINT16:
        {
            vector<uint16>val;
            val.resize(nelms);
            r = SDreaddata (sdsid, geooffset32.data(), geostep32.data(), geocount32.data(), (void*)(val.data()));
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
            }

            set_value ((dods_uint16 *) val.data(), nelms);
        }
	    break;
        case DFNT_INT32:
        {
            vector<int32>val;
            val.resize(nelms);
            r = SDreaddata (sdsid, geooffset32.data(), geostep32.data(), geocount32.data(), (void*)(val.data()));
            if (r != 0) {
	        SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
            }

            set_value ((dods_int32 *) val.data(), nelms);
        }
            break;
	case DFNT_UINT32:
        {
            vector<uint32>val;
            val.resize(nelms);
            r = SDreaddata (sdsid, geooffset32.data(), geostep32.data(), geocount32.data(), (void*)(val.data()));
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
            }
            set_value ((dods_uint32 *) val.data(), nelms);
        }
            break;
        case DFNT_FLOAT32:
        {
            vector<float32>val;
            val.resize(nelms);
            r = SDreaddata (sdsid, geooffset32.data(), geostep32.data(), geocount32.data(), (void*)(val.data()));
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
            }

            set_value ((dods_float32 *) val.data(), nelms);
        }
            break;
        case DFNT_FLOAT64:
        {
            vector<float64>val;
            val.resize(nelms);
            r = SDreaddata (sdsid, geooffset32.data(), geostep32.data(), geocount32.data(), (void*)(val.data()));
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
            }

            set_value ((dods_float64 *) val.data(), nelms);
        }
            break;
        default:
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError("unsupported data type.",__FILE__, __LINE__);
        }

        r = SDendaccess (sdsid);
        if (r != 0) {
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError("SDendaccess failed.",__FILE__, __LINE__);
        }

        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);

}

// Read TRMM level 3a version 6 lat/lon. 
// We follow TRMM document to retrieve the lat and lon.
void
HDFSPArrayGeoField::readtrmml3a_v6 (const int32 * offset32, const int32 * count32,
                                const int32 * step32, int nelms)
{

    const float slat = 89.5; 
    const float slon = 0.5;
    vector<float> val;
    val.resize(nelms);

    if (fieldtype == 1) {//latitude 

        int icount = 0;
        float sval = slat - offset32[0];

        while (icount < (int) (count32[0])) {
            val[icount] = sval -  step32[0] * icount;
            icount++;
        }
    }

    if (fieldtype == 2) {		//longitude
        int icount = 0;
        float sval = slon + offset32[0];

        while (icount < (int) (count32[0])) {
            val[icount] = sval + step32[0] * icount;
            icount++;
        }
    }
    set_value ((dods_float32 *) val.data(), nelms);
}


// TRMM level 3 case. Have to follow http://disc.sci.gsfc.nasa.gov/additional/faq/precipitation_faq.shtml#lat_lon
// to calculate lat/lon.
void
HDFSPArrayGeoField::readtrmml3b_v6 (const int32 * offset32, const int32 * count32,
                                const int32 * step32, int nelms)
{

    const float slat = -49.875; 
    const float slon = -179.875;
    vector<float> val;
    val.resize(nelms);

    if (fieldtype == 1) {//latitude 

        int icount = 0;
        float sval = slat + 0.25 * (int) (offset32[0]);

        while (icount < (int) (count32[0])) {
            val[icount] = sval + 0.25 * (int) (step32[0]) * icount;
            icount++;
        }
    }

    if (fieldtype == 2) {		//longitude
        int icount = 0;
        float sval = slon + 0.25 * (int) (offset32[0]);

        while (icount < (int) (count32[0])) {
            val[icount] = sval + 0.25 * (int) (step32[0]) * icount;
            icount++;
        }
    }
    set_value ((dods_float32 *) val.data(), nelms);
}

// Read TRMM level 3c(CSH) version 6 lat/lon. 
// We follow TRMM document to retrieve the lat and lon.
void
HDFSPArrayGeoField::readtrmml3c_v6 (const int32 * offset32, const int32 * count32,
                                const int32 * step32, int nelms)
{

    const float slat = -36.75; 
    const float slon = -179.75;
    vector<float> val;
    val.resize(nelms);

    if (fieldtype == 1) {//latitude 

        int icount = 0;
        float sval = slat + 0.5 * (int) (offset32[0]);

        while (icount < (int) (count32[0])) {
            val[icount] = sval + 0.5 * (int) (step32[0]) * icount;
            icount++;
        }
    }

    if (fieldtype == 2) {		//longitude
        int icount = 0;
        float sval = slon + 0.5 * (int) (offset32[0]);

        while (icount < (int) (count32[0])) {
            val[icount] = sval + 0.5 * (int) (step32[0]) * icount;
            icount++;
        }
    }
    set_value ((dods_float32 *) val.data(), nelms);
}
// Read latlon of TRMM version 7 products.
// Lat/lon parameters can be retrieved from attribute GridHeader.
void
HDFSPArrayGeoField::readtrmml3_v7 (const int32 * offset32, 
                                const int32 * step32, int nelms)
{

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    int32 sdid = -1;

    if(false == check_pass_fileid_key) {
        sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (sdid < 0) {
            string msg = "File " + filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__, __LINE__);
        }
    }
    else 
        sdid = sdfd;


    string gridinfo_name = "GridHeader";
    intn status = 0;

    if(fieldref != -1) {  

        if (fieldref >9) {
            string msg = "The maximum number of grids to be supported in the current implementation is 9.";
            throw BESInternalError(msg,__FILE__, __LINE__);
        }

        else {
            ostringstream fieldref_str;
            fieldref_str << fieldref;
            gridinfo_name = gridinfo_name + fieldref_str.str();
        }
    }

    int32 attr_index = 0;
    attr_index = SDfindattr (sdid, gridinfo_name.c_str());
    if (attr_index == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string msg = "SDfindattr failed,should find attribute "+gridinfo_name+" .";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    int32 attr_dtype = 0;
    int32 n_values = 0;

    char attr_name[H4_MAX_NC_NAME];
    status =
        SDattrinfo (sdid, attr_index, attr_name, &attr_dtype, &n_values);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDattrinfo failed. ",__FILE__, __LINE__);
    }
    
    vector<char> attr_value; 
    attr_value.resize(n_values * DFKNTsize(attr_dtype));

    status = SDreadattr (sdid, attr_index, attr_value.data());
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDreadattr failed.",__FILE__, __LINE__);
    }

    float lat_start = 0.;
    float lon_start = 0.;
    float lat_res = 0.;
    float lon_res = 0.;

    int   latsize = 0;
    int   lonsize = 0;
    
    HDFCFUtil::parser_trmm_v7_gridheader(attr_value,latsize,lonsize,
                                         lat_start,lon_start,lat_res,lon_res,false);

    if(0 == latsize || 0 == lonsize) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("Either latitude or longitude size is 0. ",__FILE__, __LINE__);
    }


    vector<float>val;
    val.resize(nelms);

    if(fieldtype == 1) {
        for (int i = 0; i < nelms; ++i) 
            val[i] = lat_start+offset32[0]*lat_res+lat_res/2 + i*lat_res*step32[0];
    }
    else if(fieldtype == 2) {
        for (int i = 0; i < nelms; ++i) 
            val[i] = lon_start+offset32[0]*lon_res+lon_res/2 + i*lon_res*step32[0];
    }
    
    set_value ((dods_float32 *) val.data(), nelms);

    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
}

    
 
// OBPG Level 2 lat/lon including CZCS, MODISA, MODIST, OCTS and SewWIFS.
// We need to use information retrieved from the attribute to interpoloate
// the latitude/longitude. This is similar to the Swath dimension map case.
// "Pixels per Scan Line" and "Number of Pixel Control Points"
// should be used to interpolate.
// "Pixels per Scan Line" is the final number of elements for lat/lon along the 2nd dimension
// "Number of Pixel Control Points" includes the original number of elements for lat/lon along 
// the 2nd dimension.
void
HDFSPArrayGeoField::readobpgl2 (int32 * offset32, int32 * count32,
                                int32 * step32, int nelms)
{

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    int32 sdid = -1;

    if(false == check_pass_fileid_key) {
        sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (sdid < 0) {
            string msg = "File "+ filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__, __LINE__);
        }
    }
    else 
        sdid = sdfd;

    int32 sdsid = -1;
    intn  status = 0;

    // Read File attributes to otain the segment
    int32 attr_index = 0;
    int32 attr_dtype = 0;
    int32 n_values = 0;
    int32 num_pixel_data = 0;
    int32 num_point_data = 0;
    int32 num_scan_data = 0;

    attr_index = SDfindattr (sdid, NUM_PIXEL_NAME);
    if (attr_index == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string attr_name(NUM_PIXEL_NAME);
        string msg = "SDfindattr failed,should find attribute "+attr_name+" .";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    char attr_name[H4_MAX_NC_NAME];
    status =
        SDattrinfo (sdid, attr_index, attr_name, &attr_dtype, &n_values);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDattrinfo failed. ",__FILE__, __LINE__);
    }

    if (n_values != 1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("Only one value of number of scan line. ",__FILE__, __LINE__);
    }

    status = SDreadattr (sdid, attr_index, &num_pixel_data);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDreadattr failed. ",__FILE__, __LINE__);
    }

    attr_index = SDfindattr (sdid, NUM_POINTS_LINE_NAME);
    if (attr_index == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string attr_name(NUM_POINTS_LINE_NAME);
        string msg = "SDfindattr failed,should find attribute "+attr_name+" .";
        throw BESInternalError(msg,__FILE__, __LINE__);

    }

    status =
        SDattrinfo (sdid, attr_index, attr_name, &attr_dtype, &n_values);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDattrinfo failed. ",__FILE__, __LINE__);
    }

    if (n_values != 1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("Only one value of number of point ",__FILE__, __LINE__);
    }

    status = SDreadattr (sdid, attr_index, &num_point_data);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDreadattr failed. ",__FILE__, __LINE__);
    }

    attr_index = SDfindattr (sdid, NUM_SCAN_LINE_NAME);
    if (attr_index == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string attr_name(NUM_SCAN_LINE_NAME);
        string msg = "SDfindattr failed,should find attribute "+attr_name+" .";
        throw BESInternalError(msg,__FILE__, __LINE__);

    }

    status =
        SDattrinfo (sdid, attr_index, attr_name, &attr_dtype, &n_values);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDattrinfo failed. ",__FILE__, __LINE__);
    }

    if (n_values != 1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("Only one value of number of point ",__FILE__, __LINE__);
    }

    status = SDreadattr (sdid, attr_index, &num_scan_data);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDreadattr failed. ",__FILE__, __LINE__);
    }


    if ( 0 == num_scan_data || 0 == num_point_data || 0 == num_pixel_data) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("num_scan or num_point or num_pixel should not be zero. ",__FILE__, __LINE__);
    }

    if ( 1 == num_point_data && num_pixel_data != 1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string msg = "num_point is 1 and  num_pixel is not 1, interpolation cannot be done.";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    bool compmapflag = false;
    if (num_pixel_data == num_point_data)
        compmapflag = true;

    int32 sdsindex = SDreftoindex (sdid, (int32) fieldref);
    if (sdsindex == -1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string msg = "SDS index " + to_string(sdsindex) + " is not right.";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    sdsid = SDselect (sdid, sdsindex);
    if (sdsid < 0) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDselect failed.",__FILE__, __LINE__);
    }

    int32 r = 0;

    switch (dtype) {
        case DFNT_INT8:
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        case DFNT_CHAR8:
        case DFNT_INT16:
        case DFNT_UINT16:
        case DFNT_INT32:
        case DFNT_UINT32:
        case DFNT_FLOAT64:
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError("datatype is not float, unsupported.",__FILE__, __LINE__);
        case DFNT_FLOAT32:
        {
            vector<float32> val;
            val.resize(nelms);
            if (compmapflag) {
                r = SDreaddata (sdsid, offset32, step32, count32, val.data());
                if (r != 0) {
                    SDendaccess (sdsid);
                    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                    throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
                }
            }
            else {

                int total_elm = num_scan_data * num_point_data;
                vector<float32>orival;
                orival.resize(total_elm); 
                int32 all_start[2];
                int32 all_edges[2];

                all_start[0] = 0;
                all_start[1] = 0;
                all_edges[0] = num_scan_data;
                all_edges[1] = num_point_data;

                r = SDreaddata (sdsid, all_start, nullptr, all_edges,
                                orival.data());
                if (r != 0) {
                    SDendaccess (sdsid);
                    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                    throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
                }
                int interpolate_elm = num_scan_data *num_pixel_data;

                vector<float32> interp_val;
                interp_val.resize(interpolate_elm);

                // Number of scan line doesn't change, so just interpolate according to the fast changing dimension
                int tempseg = 0;
                float tempdiff = 0;

                if (num_pixel_data % num_point_data == 0)
                    tempseg = num_pixel_data / num_point_data;
                else
                    tempseg = num_pixel_data / num_point_data + 1;

                int last_tempseg = 
                       (num_pixel_data%num_point_data)?(num_pixel_data-1-(tempseg*(num_point_data-2))):tempseg;

                if ( 0 == last_tempseg || 0 == tempseg) {
                    SDendaccess(sdsid);
                    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                    throw BESInternalError("Segments cannot be zero.",__FILE__, __LINE__);
                }

                int interp_val_index = 0;

                for (int i = 0; i <num_scan_data; i++) {

                    // All the segements except the last one
                    for( int j =0; j <num_point_data -2; j ++) {
                        tempdiff = orival[i*num_point_data+j+1] - orival[i*num_point_data+j];
                        for (int k = 0; k <tempseg; k++) {
                            interp_val[interp_val_index] = orival[i*num_point_data+j] +
                                                           tempdiff/tempseg *k;
                            interp_val_index++;
                        }
                    }
                                    
                    // The last segment
                    tempdiff =  orival[i*num_point_data+num_point_data-1]
                               -orival[i*num_point_data+num_point_data-2];
                    for (int k = 0; k <last_tempseg; k++) {
                        interp_val[interp_val_index] = orival[i*num_point_data+num_point_data-2] +
                                                       tempdiff/last_tempseg *k;
                        interp_val_index++;
                    }

                    interp_val[interp_val_index] = orival[i*num_point_data+num_point_data-1];
                    interp_val_index++;

                }

                LatLon2DSubset(val.data(),num_pixel_data,interp_val.data(),
                               offset32,count32,step32);

            }
            // Leave the following comments 
#if 0
	// WE SHOULD INTERPOLATE ACCORDING TO THE FAST CHANGING DIMENSION
                                // THis method will save some memory, but it will cause greater error
                                // if the step is very large. However, we should still take advantage 
                                // of the small memory approach in the future implementation. KY 2012-09-04
				float tempdiff;
				int i, j, k, k2;
				int32 realcount2 = oricount32[1] - 1;

				for (i = 0; i < (int) count32[0]; i++) {
					for (j = 0; j < (int) realcount2 - 1; j++) {
						tempdiff =
							orival[i * (int) oricount32[1] + j + 1] -
							orival[i * (int) oricount32[1] + j];
						for (k = 0; k < tempnewseg; k++) {
							val[i * (int) count32[1] + j * tempnewseg + k] =
								orival[i * (int) oricount32[1] + j] +
								tempdiff / tempnewseg * k;
						}
					}
					tempdiff =
						orival[i * (int) oricount32[1] + j + 1] -
						orival[i * (int) oricount32[1] + j];
					// There are three cases:
					// 1. when the oricount is 1
					//              int lastseg = num_pixel_data - tempnewseg*((int)oricount32[1]-1)-1;
					int lastseg =
						(int) (count32[1] -
							   tempnewseg * (int) (realcount2 - 1));
					for (k2 = 0; k2 <= lastseg; k2++)
						val[i * (int) count32[1] + j * tempnewseg + k2] =
							orival[i * (int) oricount32[1] + j] +
							tempdiff / lastseg * k2;
				}

				delete[](float32 *) orival;
			}
#endif

                set_value ((dods_float32 *) val.data(), nelms);
            }
            break;
        default:
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError("Unsupported data type.",__FILE__, __LINE__);
    }

    r = SDendaccess (sdsid);
    if (r != 0) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDendaccess failed.",__FILE__, __LINE__);
    }

    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
}

// Obtain lat/lon for OBPG CZCS, MODISA, MODIST,OCTS and SeaWIFS products
// lat/lon should be calculated based on the file attribute.
// Geographic projection: lat,lon starting point and also lat/lon steps.
void
HDFSPArrayGeoField::readobpgl3 (const int *offset,  const int *step, const int nelms)
{

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    int32 sdid = -1;

    if(false == check_pass_fileid_key) {
        sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (sdid < 0) {
            string msg = "File " + filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }
    else 
        sdid = sdfd;

    intn status = 0;

    // Read File attributes to otain the segment
    int32 attr_index = 0;
    int32 attr_dtype = 0;
    int32 n_values = 0;
    int32 num_lat_data = 0;
    int32 num_lon_data = 0;
    float32 lat_step = 0.;
    float32 lon_step = 0.;
    float32 swp_lat = 0.;
    float32 swp_lon = 0.;

    // Obtain number of latitude
    attr_index = SDfindattr (sdid, NUM_LAT_NAME);
    if (attr_index == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string attr_name(NUM_LAT_NAME);
        string msg = "SDfindattr failed,should find attribute "+attr_name+" .";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    char attr_name[H4_MAX_NC_NAME];
    status =
        SDattrinfo (sdid, attr_index, attr_name, &attr_dtype, &n_values);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDattrinfo failed. ",__FILE__, __LINE__);
    }

    if (n_values != 1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("Only should have one value. ",__FILE__, __LINE__);
    }

    status = SDreadattr (sdid, attr_index, &num_lat_data);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDreadattr failed. " ,__FILE__, __LINE__);
    }

    // Obtain number of longitude
    attr_index = SDfindattr (sdid, NUM_LON_NAME);
    if (attr_index == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string attr_name2(NUM_LON_NAME);
        string msg = "SDfindattr failed,should find attribute "+attr_name2+" .";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    status =
        SDattrinfo (sdid, attr_index, attr_name, &attr_dtype, &n_values);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDattrinfo failed. ",__FILE__, __LINE__);
    }

    if (n_values != 1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("Only should have one value. ",__FILE__, __LINE__);
    }

    status = SDreadattr (sdid, attr_index, &num_lon_data);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDreadattr failed. ",__FILE__, __LINE__);
    }

    // obtain latitude step
    attr_index = SDfindattr (sdid, LAT_STEP_NAME);
    if (attr_index == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string attr_name2(LAT_STEP_NAME);
        string msg = "SDfindattr failed,should find attribute "+attr_name2+" .";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    status =
        SDattrinfo (sdid, attr_index, attr_name, &attr_dtype, &n_values);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDattrinfo failed. ",__FILE__, __LINE__);
    }

    if (n_values != 1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("Only should have one value. ",__FILE__, __LINE__);
    }

    status = SDreadattr (sdid, attr_index, &lat_step);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDreadattr failed. ",__FILE__, __LINE__);
    }

    // Obtain longitude step
    attr_index = SDfindattr (sdid, LON_STEP_NAME);
    if (attr_index == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string attr_name2(LON_STEP_NAME);
        string msg = "SDfindattr failed,should find attribute "+attr_name2+" .";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    status =
        SDattrinfo (sdid, attr_index, attr_name, &attr_dtype, &n_values);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDattrinfo failed. ",__FILE__, __LINE__);
    }

    if (n_values != 1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("Only should have one value. ",__FILE__, __LINE__);
    }

    status = SDreadattr (sdid, attr_index, &lon_step);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDreadattr failed. ",__FILE__, __LINE__);
    }

    // obtain south west corner latitude
    attr_index = SDfindattr (sdid, SWP_LAT_NAME);
    if (attr_index == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string attr_name2(SWP_LAT_NAME);
        string msg = "SDfindattr failed,should find attribute "+attr_name2+" .";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    status =
        SDattrinfo (sdid, attr_index, attr_name, &attr_dtype, &n_values);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDattrinfo failed. ",__FILE__, __LINE__);
    }

    if (n_values != 1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("Only should have one value. ",__FILE__, __LINE__);
    }

    status = SDreadattr (sdid, attr_index, &swp_lat);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDreadattr failed. ",__FILE__, __LINE__);
    }

    // obtain south west corner longitude
    attr_index = SDfindattr (sdid, SWP_LON_NAME);
    if (attr_index == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string attr_name2(SWP_LON_NAME);
        string msg = "SDfindattr failed,should find attribute "+attr_name2+" .";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    status =
        SDattrinfo (sdid, attr_index, attr_name, &attr_dtype, &n_values);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDattrinfo failed. ",__FILE__, __LINE__);
    }

    if (n_values != 1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("Only should have one value. ",__FILE__, __LINE__);
    }

    status = SDreadattr (sdid, attr_index, &swp_lon);
    if (status == FAIL) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDreadattr failed. ",__FILE__, __LINE__);
    }

    if (fieldtype == 1) {

        vector<float32> allval;
        allval.resize(num_lat_data);

        // The first index is from the north, so we need to reverse the calculation of the index

        for (int j = 0; j < num_lat_data; j++)
            allval[j] = (num_lat_data - j - 1) * lat_step + swp_lat;

        vector<float32>val;
        val.resize(nelms);

        for (int k = 0; k < nelms; k++)
            val[k] = allval[(int) (offset[0] + step[0] * k)];

        set_value ((dods_float32 *) val.data(), nelms);
    }

    if (fieldtype == 2) {

        vector<float32>allval;
        allval.resize(num_lon_data);
        for (int j = 0; j < num_lon_data; j++)
            allval[j] = swp_lon + j * lon_step;

        vector<float32>val;
        val.resize(nelms);
        for (int k = 0; k < nelms; k++)
            val[k] = allval[(int) (offset[0] + step[0] * k)];

        set_value ((dods_float32 *) val.data(), nelms);

    }

    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);

}


// CERES SAVG and ISSP DAY case(CER_SAVG and CER_ISCCP_.._DAY )
// We need to calculate latitude/longitude based on CERES nested grid formula
// http://eosweb.larc.nasa.gov/PRODOCS/ceres/SRBAVG/Quality_Summaries/srbavg_ed2d/nestedgrid.html
// or https://eosweb.larc.nasa.gov/sites/default/files/project/ceres/quality_summaries/srbavg_ed2d/nestedgrid.pdf 
void
HDFSPArrayGeoField::readcersavgid2 (const int *offset, const int *count, const int *step,
                                    int nelms)
{

    const int dimsize0 = 180;
    const int dimsize1 = 360;
    float32 val[count[0]][count[1]];
    float32 orival[dimsize0][dimsize1];

    // Following CERES Nested grid
    // URL http://eosweb.larc.nasa.gov/PRODOCS/ceres/SRBAVG/Quality_Summaries/srbavg_ed2d/nestedgrid.html
    // or https://eosweb.larc.nasa.gov/sites/default/files/project/ceres/quality_summaries/srbavg_ed2d/nestedgrid.pdf 
    if (fieldtype == 1) {// Calculate the latitude
        for (int i = 0; i < dimsize0; i++)
            for (int j = 0; j < dimsize1; j++)
                orival[i][j] = 89.5 - i;

        for (int i = 0; i < count[0]; i++) {
            for (int j = 0; j < count[1]; j++) {
                val[i][j] =
                        orival[offset[0] + step[0] * i][offset[1] + step[1] * j];
            }
        }
    }

    if (fieldtype == 2) {// Calculate the longitude

        int i = 0;
        int j = 0;
        int k = 0;

        // Longitude extent
        int lonextent;

        // Latitude index
        int latindex_south;

        // Latitude index
        int latindex_north;

        // Latitude range
        int latrange;


        //latitude 89-90 (both south and north) 1 value each part
        for (j = 0; j < dimsize1; j++) {
            orival[0][j] = -179.5;
            orival[dimsize0 - 1][j] = -179.5;
        }

        //latitude 80-89 (both south and north) 45 values each part
        // longitude extent is 8
        lonextent = 8;
        // From 89 N to 80 N
        latindex_north = 1;
        latrange = 9;
        for (i = 0; i < latrange; i++)
            for (j = 0; j < (dimsize1 / lonextent); j++)
                for (k = 0; k < lonextent; k++)
                    orival[i + latindex_north][j * lonextent + k] =
                                                        -179.5 + lonextent * j;

        // From 89 S to 80 S
        latindex_south = dimsize0 - 1 - latrange;
        for (i = 0; i < latrange; i++)
            for (j = 0; j < (dimsize1 / lonextent); j++)
                for (k = 0; k < lonextent; k++)
                    orival[i + latindex_south][j * lonextent + k] =
		                                        -179.5 + lonextent * j;

        // From 80 N to 70 N
        latindex_north = latindex_north + latrange;
        latrange = 10;
        lonextent = 4;

        for (i = 0; i < latrange; i++)
            for (j = 0; j < (dimsize1 / lonextent); j++)
                for (k = 0; k < lonextent; k++)
                    orival[i + latindex_north][j * lonextent + k] =
                                                        -179.5 + lonextent * j;

        // From 80 S to 70 S
        latindex_south = latindex_south - latrange;
        for (i = 0; i < latrange; i++)
            for (j = 0; j < (dimsize1 / lonextent); j++)
                for (k = 0; k < lonextent; k++)
                    orival[i + latindex_south][j * lonextent + k] =
                                                        -179.5 + lonextent * j;

        // From 70 N to 45 N
        latindex_north = latindex_north + latrange;
        latrange = 25;
        lonextent = 2;

        for (i = 0; i < latrange; i++)
            for (j = 0; j < (dimsize1 / lonextent); j++)
                for (k = 0; k < lonextent; k++)
                    orival[i + latindex_north][j * lonextent + k] =
                                                        -179.5 + lonextent * j;

        // From 70 S to 45 S
        latindex_south = latindex_south - latrange;

        for (i = 0; i < latrange; i++)
            for (j = 0; j < (dimsize1 / lonextent); j++)
                for (k = 0; k < lonextent; k++)
                    orival[i + latindex_south][j * lonextent + k] =
                                                        -179.5 + lonextent * j;

        // From 45 N to 0 N
        latindex_north = latindex_north + latrange;
        latrange = 45;
        lonextent = 1;

        for (i = 0; i < latrange; i++)
            for (j = 0; j < (dimsize1 / lonextent); j++)
                for (k = 0; k < lonextent; k++)
                    orival[i + latindex_north][j * lonextent + k] =
                                                        -179.5 + lonextent * j;

        // From 45 S to 0 S
        latindex_south = latindex_south - latrange;
        for (i = 0; i < latrange; i++)
            for (j = 0; j < (dimsize1 / lonextent); j++)
                for (k = 0; k < lonextent; k++)
                    orival[i + latindex_south][j * lonextent + k] =
                                                        -179.5 + lonextent * j;

        for (i = 0; i < count[0]; i++) {
            for (j = 0; j < count[1]; j++) {
                val[i][j] =
                    orival[offset[0] + step[0] * i][offset[1] + step[1] * j];
            }
        }
    }
    set_value ((dods_float32 *) (&val[0][0]), nelms);

}

// This function calculate zonal average(longitude is fixed) of CERES SAVG and CERES_ISCCP_DAY_LIKE. 
void
HDFSPArrayGeoField::readcersavgid1 (const int *offset, const int *count, const int *step, int nelms) 
{

    // Following CERES Nested grid
    // URL http://eosweb.larc.nasa.gov/PRODOCS/ceres/SRBAVG/Quality_Summaries/srbavg_ed2d/nestedgrid.html
    // or https://eosweb.larc.nasa.gov/sites/default/files/project/ceres/quality_summaries/srbavg_ed2d/nestedgrid.pdf 
    if (fieldtype == 1) {		// Calculate the latitude

        const int dimsize0 = 180;
        vector<float32> val(count[0]);
        vector<float32> orival(dimsize0);

        for (int i = 0; i < dimsize0; i++)
            orival[i] = 89.5 - i;

        for (int i = 0; i < count[0]; i++) {
            val[i] = orival[offset[0] + step[0] * i];
        }
        set_value ((dods_float32 *)(val.data()), nelms);

    }

    if (fieldtype == 2) {		// Calculate the longitude

        // Assume the longitude is 0 in average
        float32 val = 0;
        if (nelms > 1)
            throw BESInternalError("The number of element must be 1.",__FILE__, __LINE__);
        set_value ((dods_float32 *) (&val), nelms);

    }

}

// Calculate CERES AVG and SYN lat and lon.
// We just need to retrieve lat/lon from the field.
void
HDFSPArrayGeoField::readceravgsyn (int32 * offset32, int32 * count32,
                                   int32 * step32, int nelms)
{

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    int32 sdid = -1;

    if(false == check_pass_fileid_key) {
        sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (sdid < 0) {
            string msg = "File " + filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__, __LINE__);
        }
    }
    else 
        sdid = sdfd;

    int i = 0;
    int32 sdsid = -1;

    int32 sdsindex = SDreftoindex (sdid, fieldref);

    if (sdsindex == -1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string msg = "SDS index " +to_string(sdsindex) + " is not right.";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    sdsid = SDselect (sdid, sdsindex);
    if (sdsid < 0) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDselect failed.",__FILE__, __LINE__);
    }

    int32 r;

    switch (dtype) {
        case DFNT_INT8:
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        case DFNT_CHAR8:
        case DFNT_INT16:
        case DFNT_UINT16:
        case DFNT_INT32:
        case DFNT_UINT32:
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError("Datatype is not float, unsupported.",__FILE__, __LINE__);

        case DFNT_FLOAT32:
        {
            vector<float32>val;
            val.resize(nelms);
            r = SDreaddata (sdsid, offset32, step32, count32, val.data());
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
            }

            if (fieldtype == 1) {
                for (i = 0; i < nelms; i++)
                    val[i] = 90 - val[i];
            }
            if (fieldtype == 2) {
                for (i = 0; i < nelms; i++)
                    if (val[i] > 180.0)
                        val[i] = val[i] - 360.0;
            }

            set_value ((dods_float32 *) val.data(), nelms);
            break;
        }
        case DFNT_FLOAT64:
        {
            vector<float64>val;
            val.resize(nelms);

            r = SDreaddata (sdsid, offset32, step32, count32, val.data());
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
            }

            if (fieldtype == 1) {
                for (i = 0; i < nelms; i++)
                    val[i] = 90 - val[i];
            }
            if (fieldtype == 2) {
                for (i = 0; i < nelms; i++)
                    if (val[i] > 180.0)
                        val[i] = val[i] - 360.0;
            }
            set_value ((dods_float64 *) val.data(), nelms);
            break;
        }
        default:
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError("Unsupported data type.",__FILE__, __LINE__);
    }

    r = SDendaccess (sdsid);
    if (r != 0) {
        throw BESInternalError("SDendaccess failed.",__FILE__, __LINE__);
    }

    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
}

// Calculate CERES ES4 and GEO lat/lon.
// We have to retrieve the original lat/lon and condense it from >1-D to 1-D.
void
HDFSPArrayGeoField::readceres4ig (const int32 * offset32, const int32 * count32,
                                  const int32 * step32, int nelms)
{

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    int32 sdid = -1;

    if(false == check_pass_fileid_key) {
        sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (sdid < 0) {
            string msg = "File " + filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__, __LINE__);
        }
    }
    else 
        sdid = sdfd;


    int32 sdsid = -1;
    intn status = 0;

    int32 sdsindex = SDreftoindex (sdid, (int32) fieldref);
    if (sdsindex == -1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        string msg ="SDS index " +  to_string(sdsindex) + " is not right.";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    sdsid = SDselect (sdid, sdsindex);
    if (sdsid < 0) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDselect failed.",__FILE__, __LINE__);
    }

    int32 sdsrank = 0;
    int32 sds_dtype = 0;
    int32 n_attrs = 0;
    char sdsname[H4_MAX_NC_NAME];
    int32 dim_sizes[H4_MAX_VAR_DIMS];

    status =
        SDgetinfo (sdsid, sdsname, &sdsrank, dim_sizes, &sds_dtype, &n_attrs);
    if (status < 0) {
        SDendaccess (sdsid);
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        throw BESInternalError("SDgetinfo failed.",__FILE__, __LINE__);
    }
    vector<int32> orioffset32;
    vector<int32> oricount32;
    vector<int32> oristep32;
    orioffset32.resize(sdsrank);
    oricount32.resize(sdsrank);
    oristep32.resize(sdsrank);

    int32 r;

    switch (sds_dtype) {
        case DFNT_INT8:
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        case DFNT_CHAR8:
        case DFNT_INT16:
        case DFNT_UINT16:
        case DFNT_INT32:
        case DFNT_UINT32:
        case DFNT_FLOAT64:
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError("Datatype is not float, unsupported.",__FILE__, __LINE__);
        case DFNT_FLOAT32:
        {
            vector<float32> val;
            val.resize(nelms);
            if (fieldtype == 1) {
                if (sptype == CER_CGEO) {
                    if (sdsrank != 3) {
                        SDendaccess (sdsid);
                        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                        string msg = "For CER_ISCCP-D2like-GEO case, lat/lon must be 3-D.";
                        throw BESInternalError(msg,__FILE__, __LINE__);
                    }
                    orioffset32[0] = 0;

                    // The second dimension of the original latitude should be condensed
                    orioffset32[1] = offset32[0];	
                    orioffset32[2] = 0;
                    oricount32[0] = 1;
                    oricount32[1] = count32[0];
                    oricount32[2] = 1;
                    oristep32[0] = 1;
                    oristep32[1] = step32[0];
                    oristep32[2] = 1;
                }
                if (sptype == CER_ES4) {
                    if (sdsrank != 2) {
                        SDendaccess (sdsid);
                        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                        string msg = "For CER_ES4 case, lat/lon must be 2-D.";
                        throw BESInternalError(msg,__FILE__, __LINE__);
                    }
                    // The first dimension of the original latitude should be condensed
                    orioffset32[0] = offset32[0];	
                    orioffset32[1] = 0;
                    oricount32[0] = count32[0];
                    oricount32[1] = 1;
                    oristep32[0] = step32[0];
                    oristep32[1] = 1;
                }
            }

            if (fieldtype == 2) {
                if (sptype == CER_CGEO) {
                    if (sdsrank != 3) {
                        SDendaccess (sdsid);
                        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                        string msg = "For CER_ISCCP-D2like-GEO case, lat/lon must be 3-D.";
                        throw BESInternalError(msg,__FILE__, __LINE__);
                    }
                    orioffset32[0] = 0;
                    orioffset32[2] = offset32[0];// The third dimension of the original latitude should be condensed
                    orioffset32[1] = 0;
                    oricount32[0] = 1;
                    oricount32[2] = count32[0];
                    oricount32[1] = 1;
                    oristep32[0] = 1;
                    oristep32[2] = step32[0];
                    oristep32[1] = 1;
                }
                if (sptype == CER_ES4) {
                    if (sdsrank != 2) {
                        SDendaccess (sdsid);
                        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                        string msg = "For CER_ES4 case, lat/lon must be 2-D.";
                        throw BESInternalError(msg,__FILE__, __LINE__);
                    }
                    orioffset32[1] = offset32[0]; // The second dimension of the original latitude should be condensed
                    orioffset32[0] = 0;
                    oricount32[1] = count32[0];
                    oricount32[0] = 1;
                    oristep32[1] = step32[0];
                    oristep32[0] = 1;
                }
            }

            r = SDreaddata (sdsid, orioffset32.data(), oristep32.data(), oricount32.data(), val.data());
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                throw BESInternalError("SDreaddata failed.",__FILE__, __LINE__);
            }

            if (fieldtype == 1)
                for (int i = 0; i < nelms; i++)
                    val[i] = 90 - val[i];
            if (fieldtype == 2) {
                // Since Panoply cannot handle the case when the longitude is jumped from 180 to -180
                // So turn it off and see if it works with other clients,
                // change my mind, should contact Panoply developer to solve this
                // Just check Panoply(3.2.1) with the latest release(1.9). This is no longer an issue.
                for (int i = 0; i < nelms; i++)
                    if (val[i] > 180.0)
                        val[i] = val[i] - 360.0;
            }

            set_value ((dods_float32 *) val.data(), nelms);
            break;
        }
        default:
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);
    }

    r = SDendaccess (sdsid);
    if (r != 0) {
        throw BESInternalError("SDendaccess failed.",__FILE__,__LINE__);
    }

    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
}

// Read CERES Zonal average latitude field
void
HDFSPArrayGeoField::readcerzavg (const int32 * offset32, const int32 * count32,
                                 const int32 * step32, int nelms)
{
    if (fieldtype == 1) {

        vector<float32>val;
        val.resize(nelms);

        float32 latstep = 1.0;

        for (int i = 0; i < nelms; i++)
            val[i] =
                     89.5 - ((int) (offset32[0]) +
                     ((int) (step32[0])) * i) * latstep;
        set_value ((dods_float32 *) val.data(), nelms);
    }

    if (fieldtype == 2) {
        if (count32[0] != 1 || nelms != 1) {
            string msg = "Longitude should only have one value for zonal mean.";
            throw BESInternalError(msg,__FILE__, __LINE__);
        }

        // We don't need to specify the longitude value.
        float32 val = 0.;// our convention
        set_value ((dods_float32 *) & val, nelms);
    }
}


// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFSPArrayGeoField::format_constraint (int *offset, int *step, int *count)
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
    }

    return nels;
}



// Subset of latitude and longitude to follow the parameters from the DAP expression constraint
template < typename T >
void HDFSPArrayGeoField::LatLon2DSubset (T * outlatlon,
                                         int minordim,
                                         T * latlon,
                                         const int32 * offset,
                                         const int32 * count,
                                         const int32 * step)
{

    int i = 0;
    int j = 0;
    int k = 0;

    // do subsetting
    // Find the correct index
    const int dim0count = count[0];
    const int dim1count = count[1]; 
    vector <int> dim0index(dim0count);
    vector <int> dim1index(dim1count);

    for (i = 0; i < count[0]; i++)      // count[0] is the least changing dimension 
        dim0index[i] = offset[0] + i * step[0];

    for (j = 0; j < count[1]; j++) 
        dim1index[j] = offset[1] + j * step[1];

    // Now assign the subsetting data
    k = 0;

    for (i = 0; i < count[0]; i++) {
        for (j = 0; j < count[1]; j++) {
            outlatlon[k] = *(latlon + (dim0index[i] * minordim) + dim1index[j]); //[dim0index[i]][dim1index[j]];
            k++;
        }
    }
}

