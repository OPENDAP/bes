/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the latitude and longitude fields for some special HDF4 data products.
// The products include TRMML2,TRMML3,CER_AVG,CER_ES4,CER_CDAY,CER_CGEO,CER_SRB,CER_SYN,CER_ZAVG,OBPGL2,OBPGL3
// To know more information about these products,check HDFSP.h.
// Each product stores lat/lon in different way, so we have to retrieve them differently.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFSPArrayGeoField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "hdf.h"
#include "mfhdf.h"
#include "InternalErr.h"
#include <BESDebug.h>
#include "HDFCFUtil.h"
#define SIGNED_BYTE_TO_INT32 1
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

	int *
		offset = new int[rank];
	int *
		count = new int[rank];
	int *
		step = new int[rank];

	int
		nelms;

	try {
		nelms = format_constraint (offset, step, count);
	}
	catch (...) {
		delete[]offset;
		delete[]step;
		delete[]count;
		throw;
	}

	int32 *
		offset32 = new int32[rank];
	int32 *
		count32 = new int32[rank];
	int32 *
		step32 = new int32[rank];

	for (int i = 0; i < rank; i++) {
		offset32[i] = (int32) offset[i];
		count32[i] = (int32) count[i];
		step32[i] = (int32) step[i];
	}

	switch (sptype) {
	case TRMML2:
		{
			try {
				readtrmml2 (offset32, count32, step32, nelms);
			}
			catch (...) {
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				throw;
			}
			break;
		}
	case TRMML3:
		{
			try {
				readtrmml3 (offset32, count32, step32, nelms);
			}
			catch (...) {
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				throw;
			}
			break;
		}
	case CER_AVG:
		{
			try {
				readceravgsyn (offset32, count32, step32, nelms);
			}
			catch (...) {
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				throw;
			}

			break;
		}
	case CER_ES4:
		{
			try {
				readceres4ig (offset32, count32, step32, nelms);
			}
			catch (...) {
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				throw;
			}
			break;
		}
	case CER_CDAY:
		{
			try {
				readcersavgid2 (offset, count, step, nelms);
			}
			catch (...) {
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				throw;
			}

			break;
		}

	case CER_CGEO:
		{
			try {
				readceres4ig (offset32, count32, step32, nelms);
			}
			catch (...) {
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				throw;
			}

			break;
		}
	case CER_SRB:
		{
			if (rank == 1) {
				try {
					readcersavgid1 (offset, count, step, nelms);
				}
				catch (...) {
					HDFCFUtil::ClearMem (offset32, count32, step32, offset,
										 count, step);
					throw;
				}

			}
			else if (rank == 2) {
				try {
					readcersavgid2 (offset, count, step, nelms);
				}
				catch (...) {
					HDFCFUtil::ClearMem (offset32, count32, step32, offset,
										 count, step);
					throw;
				}
			}

			break;
		}
	case CER_SYN:
		{
			try {
				readceravgsyn (offset32, count32, step32, nelms);
			}
			catch (...) {
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				throw;
			}
			break;
		}
	case CER_ZAVG:
		{
			try {
				readcerzavg (offset32, count32, step32, nelms);
			}
			catch (...) {
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				throw;
			}

			break;
		}
	case OBPGL2:
		{
			try {
				readobpgl2 (offset32, count32, step32, nelms);
			}
			catch (...) {
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				throw;
			}

			break;
		}
	case OBPGL3:
		{
			try {
				readobpgl3 (offset, count, step, nelms);
			}
			catch (...) {
				HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
									 step);
				throw;
			}

			break;
		}
	case OTHERHDF:
		{
			HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
								 step);
			throw
			InternalErr (__FILE__, __LINE__, "Unsupported HDF files");

			break;
		}
	default:
		{

			HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
								 step);
			throw
			InternalErr (__FILE__, __LINE__, "Unsupported HDF files");

			break;
		}
	}

	HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);

	return false;

}


// Read TRMM level 2 lat/lon. We need to split geolocation field.
// geolocation[YDIM][XDIM][0] is latitude
// geolocation[YDIM][XDIM][1] is longitude

void
HDFSPArrayGeoField::readtrmml2 (int32 * offset32, int32 * count32,
								int32 * step32, int nelms)
{

	int32 sdid = 0;
        int32 sdsid = 0;
	int32 *geooffset32 = new int32[rank + 1];
	int32 *geocount32 = new int32[rank + 1];
	int32 *geostep32 = new int32[rank + 1];

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

	sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);

	if (sdid < 0) {
		HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
		ostringstream eherr;
		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	int32 sdsindex = SDreftoindex (sdid, (int32) sdsref);

	if (sdsindex == -1) {
		SDend (sdid);
		HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
		ostringstream eherr;
		eherr << "SDS index " << sdsindex << " is not right.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	sdsid = SDselect (sdid, sdsindex);
	if (sdsid < 0) {
		SDend (sdid);
		HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
		ostringstream eherr;
		eherr << "SDselect failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	void *val = NULL;
	int32 r = 0;


	switch (dtype) {

	case DFNT_INT8:
		{
			val = new int8[nelms];
			r = SDreaddata (sdsid, geooffset32, geostep32, geocount32, val);
			if (r != 0) {
				SDend (sdid);
				SDendaccess (sdsid);

				delete[](int8 *) val;
				HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);

				ostringstream eherr;

				eherr << "SDreaddata failed.";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

#ifndef SIGNED_BYTE_TO_INT32
			set_value ((dods_byte *) val, nelms);
			delete[](int8 *) val;
#else
			int32 *newval;
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
		val = new uint8[nelms];
		r = SDreaddata (sdsid, geooffset32, geostep32, geocount32, val);
		if (r != 0) {
			SDend (sdid);
			SDendaccess (sdsid);

			delete[](uint8 *) val;
			HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_byte *) val, nelms);
		delete[](uint8 *) val;
		break;

	case DFNT_INT16:
		val = new int16[nelms];
		r = SDreaddata (sdsid, geooffset32, geostep32, geocount32, val);
		if (r != 0) {

			SDend (sdid);
			SDendaccess (sdsid);

			delete[](int16 *) val;
			HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}


		set_value ((dods_int16 *) val, nelms);
		delete[](int16 *) val;
		break;

	case DFNT_UINT16:
		val = new uint16[nelms];
		r = SDreaddata (sdsid, geooffset32, geostep32, geocount32, val);
		if (r != 0) {
			SDend (sdid);
			SDendaccess (sdsid);

			delete[](uint16 *) val;
			HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_uint16 *) val, nelms);
		delete[](uint16 *) val;
		break;
	case DFNT_INT32:
		val = new int32[nelms];
		r = SDreaddata (sdsid, geooffset32, geostep32, geocount32, val);
		if (r != 0) {
			SDend (sdid);
			SDendaccess (sdsid);

			delete[](int32 *) val;
			HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_int32 *) val, nelms);
		delete[](int32 *) val;
		break;
	case DFNT_UINT32:
		val = new uint32[nelms];
		r = SDreaddata (sdsid, geooffset32, geostep32, geocount32, val);
		if (r != 0) {
			SDend (sdid);
			SDendaccess (sdsid);

			delete[](uint32 *) val;
			HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}
		set_value ((dods_uint32 *) val, nelms);
		delete[](uint32 *) val;
		break;
	case DFNT_FLOAT32:
		val = new float32[nelms];
		r = SDreaddata (sdsid, geooffset32, geostep32, geocount32, val);
		if (r != 0) {

			SDend (sdid);
			SDendaccess (sdsid);

			delete[](float32 *) val;
			HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_float32 *) val, nelms);
		delete[](float32 *) val;
		break;
	case DFNT_FLOAT64:
		val = new float64[nelms];
		r = SDreaddata (sdsid, geooffset32, geostep32, geocount32, val);
		if (r != 0) {

			SDend (sdid);
			SDendaccess (sdsid);

			delete[](float64 *) val;
			HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
			ostringstream eherr;

			eherr << "SDreaddata failed";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_float64 *) val, nelms);
		delete[](float64 *) val;
		break;
	default:
		SDend (sdid);
		SDendaccess (sdsid);
		HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
		InternalErr (__FILE__, __LINE__, "unsupported data type.");
	}

	r = SDendaccess (sdsid);
	if (r != 0) {
		SDend (sdid);
		HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
		ostringstream eherr;

		eherr << "SDendaccess failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	r = SDend (sdid);
	if (r != 0) {
		HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);
		ostringstream eherr;

		eherr << "SDend failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}
	HDFCFUtil::ClearMem2 (geooffset32, geocount32, geostep32);

}

// TRMM level 3 case. Have to follow http://disc.sci.gsfc.nasa.gov/additional/faq/precipitation_faq.shtml#lat_lon
// to calculate lat/lon.
void
HDFSPArrayGeoField::readtrmml3 (int32 * offset32, int32 * count32,
	int32 * step32, int nelms)
{

	const float slat = -49.875; // jhrg 3/16/11; Added const, rm step
	const float slon = -179.875;
	// float step = 0.25;
	float *val = new float[nelms];

	if (fieldtype == 1) {		//latitude 
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
	set_value ((dods_float32 *) val, nelms);
	delete[]val;
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

	int32 sd_id, sdsid;
	intn status;

	sd_id = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);

	if (sd_id < 0) {
		ostringstream eherr;

		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	// Read File attributes to otain the segment

	int32 attr_index, attr_dtype, n_values;
	int32 num_pixel_data;
	int32 num_point_data;
        int32 num_scan_data;

	attr_index = SDfindattr (sd_id, NUM_PIXEL_NAME);
	if (attr_index == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDfindattr failed ");
	}

	char attr_name[H4_MAX_NC_NAME];
	status =
		SDattrinfo (sd_id, attr_index, attr_name, &attr_dtype, &n_values);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDattrinfo failed ");
	}

	if (n_values != 1) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__,
						   "Only one value of number of scan line ");
	}

	status = SDreadattr (sd_id, attr_index, &num_pixel_data);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDreadattr failed ");
	}

	attr_index = SDfindattr (sd_id, NUM_POINTS_LINE_NAME);
	if (attr_index == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDfindattr failed, should find this attribute ");
	}

	status =
		SDattrinfo (sd_id, attr_index, attr_name, &attr_dtype, &n_values);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDattrinfo failed ");
	}

	if (n_values != 1) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__,
						   "Only one value of number of point ");
	}

	status = SDreadattr (sd_id, attr_index, &num_point_data);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDreadattr failed ");
	}

	attr_index = SDfindattr (sd_id, NUM_SCAN_LINE_NAME);
	if (attr_index == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDfindattr failed, should find this attribute ");
	}

	status =
		SDattrinfo (sd_id, attr_index, attr_name, &attr_dtype, &n_values);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDattrinfo failed ");
	}

	if (n_values != 1) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__,
						   "Only one value of number of point ");
	}

	status = SDreadattr (sd_id, attr_index, &num_scan_data);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDreadattr failed ");
	}


        if ( 0 == num_scan_data || 0 == num_point_data || 0 == num_pixel_data) {
                SDend (sd_id);
                throw InternalErr (__FILE__, __LINE__, "num_scan or num_point or num_pixel should not be zero. ");
        }

        if ( 1 == num_point_data && num_pixel_data != 1) {
                SDend (sd_id);
                throw InternalErr (__FILE__, __LINE__, "num_point is 1 and  num_pixel is not 1, interpolation cannot be done ");
        }
	bool compmapflag = false;

	if (num_pixel_data == num_point_data)
		compmapflag = true;

	int32 sdsindex = SDreftoindex (sd_id, (int32) sdsref);

	if (sdsindex == -1) {
		SDend (sd_id);
		ostringstream eherr;

		eherr << "SDS index " << sdsindex << " is not right.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	sdsid = SDselect (sd_id, sdsindex);
	if (sdsid < 0) {
		SDend (sd_id);
		ostringstream eherr;

		eherr << "SDselect failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
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
	case DFNT_FLOAT64:
		SDendaccess (sdsid);
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__,
						   "datatype is not float, unsupported.");
		break;
	case DFNT_FLOAT32:

		{
			float32 *val = new float32[nelms];

			if (compmapflag) {
				r = SDreaddata (sdsid, offset32, step32, count32, val);
				if (r != 0) {
					delete[](float32 *) val;
					SDendaccess (sdsid);
					SDend (sd_id);
					ostringstream eherr;

					eherr << "SDreaddata failed";
					throw InternalErr (__FILE__, __LINE__, eherr.str ());
				}
			}

			else {

                              
                              int total_elm = num_scan_data * num_point_data;
				float32 *orival = new float32[total_elm];

                                int32 all_start[2],all_edges[2];
                                all_start[0] = 0;
                                all_start[1] = 0;
                                all_edges[0] = num_scan_data;
                                all_edges[1] = num_point_data;

				r = SDreaddata (sdsid, all_start, NULL, all_edges,
								orival);
				if (r != 0) {
					delete[] orival;
					SDendaccess (sdsid);
					SDend (sd_id);
					ostringstream eherr;

					eherr << "SDreaddata failed";
					throw InternalErr (__FILE__, __LINE__, eherr.str ());
				}
                                int interpolate_elm = num_scan_data *num_pixel_data;

                                float32 * interp_val = new float[interpolate_elm];

                                // Number of scan line doesn't change, so just interpolate according to the fast changing dimension
                                int tempseg = 0;
                                float tempdiff = 0;

		                if (num_pixel_data % num_point_data == 0)
			            tempseg = num_pixel_data / num_point_data;
		                else
			            tempseg = num_pixel_data / num_point_data + 1;

                                int last_tempseg = (num_pixel_data%num_point_data)?(num_pixel_data-1-(tempseg*(num_point_data-2))):tempseg;

                                if ( 0 == last_tempseg || 0 == tempseg) {
                                    SDendaccess(sdsid);
                                    SDend(sd_id);
                                    throw InternalErr(__FILE__,__LINE__,"Segments cannot be zero");
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
                                    tempdiff = orival[i*num_point_data+num_point_data-1]-orival[i*num_point_data+num_point_data-2];
                                    for (int k = 0; k <last_tempseg; k++) {
                                            interp_val[interp_val_index] = orival[i*num_point_data+num_point_data-2] +
                                                tempdiff/last_tempseg *k;
                                            interp_val_index++;
                                    }

                                    interp_val[interp_val_index] = orival[i*num_point_data+num_point_data-1];
                                    interp_val_index++;

                                }

                                LatLon2DSubset(val,num_scan_data,num_pixel_data,interp_val,offset32,count32,step32);

                                delete[] orival;
                                delete[] interp_val;
                            }
                            // Leave the following comments until the next release
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

			set_value ((dods_float32 *) val, nelms);
			delete[](float32 *) val;
		}
		break;
	default:

		SDendaccess (sdsid);
		SDend (sd_id);
		InternalErr (__FILE__, __LINE__, "unsupported data type.");
	}

	r = SDendaccess (sdsid);
	if (r != 0) {
		SDend (sd_id);
		ostringstream eherr;

		eherr << "SDendaccess failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	r = SDend (sd_id);
	if (r != 0) {
		ostringstream eherr;

		eherr << "SDend failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

}

// Obtain lat/lon for OBPG CZCS, MODISA, MODIST,OCTS and SeaWIFS products
// lat/lon should be calculated based on the file attribute.
// Geographic projection: lat,lon starting point and also lat/lon steps.
void
HDFSPArrayGeoField::readobpgl3 (int *offset, int *count, int *step, int nelms)
{

	int32 sd_id;
	intn status;

	sd_id = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);

	if (sd_id < 0) {
		ostringstream eherr;

		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

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
	attr_index = SDfindattr (sd_id, NUM_LAT_NAME);
	if (attr_index == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDfindattr failed ");
	}

	char attr_name[H4_MAX_NC_NAME];
	status =
		SDattrinfo (sd_id, attr_index, attr_name, &attr_dtype, &n_values);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDattrinfo failed ");
	}

	if (n_values != 1) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "Only should have one value ");
	}

	status = SDreadattr (sd_id, attr_index, &num_lat_data);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDreadattr failed ");
	}

	// Obtain number of longitude
	attr_index = SDfindattr (sd_id, NUM_LON_NAME);
	if (attr_index == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDfindattr failed ");
	}

	status =
		SDattrinfo (sd_id, attr_index, attr_name, &attr_dtype, &n_values);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDattrinfo failed ");
	}

	if (n_values != 1) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "Only should have one value ");
	}

	status = SDreadattr (sd_id, attr_index, &num_lon_data);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDreadattr failed ");
	}

	// obtain latitude step
	attr_index = SDfindattr (sd_id, LAT_STEP_NAME);
	if (attr_index == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDfindattr failed ");
	}

	status =
		SDattrinfo (sd_id, attr_index, attr_name, &attr_dtype, &n_values);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDattrinfo failed ");
	}

	if (n_values != 1) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "Only should have one value ");
	}

	status = SDreadattr (sd_id, attr_index, &lat_step);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDreadattr failed ");
	}

	// Obtain longitude step
	attr_index = SDfindattr (sd_id, LON_STEP_NAME);
	if (attr_index == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDfindattr failed ");
	}

	status =
		SDattrinfo (sd_id, attr_index, attr_name, &attr_dtype, &n_values);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDattrinfo failed ");
	}

	if (n_values != 1) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "Only should have one value ");
	}

	status = SDreadattr (sd_id, attr_index, &lon_step);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDreadattr failed ");
	}

	// obtain south west corner latitude
	attr_index = SDfindattr (sd_id, SWP_LAT_NAME);
	if (attr_index == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDfindattr failed ");
	}

	status =
		SDattrinfo (sd_id, attr_index, attr_name, &attr_dtype, &n_values);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDattrinfo failed ");
	}

	if (n_values != 1) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "Only should have one value ");
	}

	status = SDreadattr (sd_id, attr_index, &swp_lat);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDreadattr failed ");
	}

	// obtain south west corner longitude
	attr_index = SDfindattr (sd_id, SWP_LON_NAME);
	if (attr_index == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDfindattr failed ");
	}

	status =
		SDattrinfo (sd_id, attr_index, attr_name, &attr_dtype, &n_values);
	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDattrinfo failed ");
	}

	if (n_values != 1) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "Only should have one value ");
	}

	status = SDreadattr (sd_id, attr_index, &swp_lon);

	if (status == FAIL) {
		SDend (sd_id);
		throw InternalErr (__FILE__, __LINE__, "SDreadattr failed ");
	}


	if (fieldtype == 1) {

		float32 *allval = new float32[num_lat_data];

		// The first index is from the north, so we need to reverse the calculation of the index

		for (int j = 0; j < num_lat_data; j++)
			allval[j] = (num_lat_data - j - 1) * lat_step + swp_lat;

		float32 *val = new float32[nelms];

		for (int k = 0; k < nelms; k++)
			val[k] = allval[(int) (offset[0] + step[0] * k)];

		set_value ((dods_float32 *) val, nelms);
		delete[](float32 *) val;
		delete[](float32 *) allval;
	}

	if (fieldtype == 2) {

		float32 *allval = new float32[num_lon_data];

		for (int j = 0; j < num_lon_data; j++)
			allval[j] = swp_lon + j * lon_step;

		float32 *val = new float32[nelms];

		for (int k = 0; k < nelms; k++)
			val[k] = allval[(int) (offset[0] + step[0] * k)];

		set_value ((dods_float32 *) val, nelms);
		delete[](float32 *) val;
		delete[](float32 *) allval;

	}

	status = SDend (sd_id);
	if (status != 0) {
		ostringstream eherr;

		eherr << "SDend failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

}


// CERES SAVG and ISSP DAY case(CER_SAVG and CER_ISCCP_.._DAY )
// We need to calculate latitude/longitude based on CERES nested grid formula
// http://eosweb.larc.nasa.gov/PRODOCS/ceres/SRBAVG/Quality_Summaries/srbavg_ed2d/nestedgrid.html
void
HDFSPArrayGeoField::readcersavgid2 (int *offset, int *count, int *step,
									int nelms)
{

	int dimsize0 = 180;
	int dimsize1 = 360;
	float32 val[count[0]][count[1]];
	float32 orival[dimsize0][dimsize1];

	// Following CERES Nested grid
	// URL http://eosweb.larc.nasa.gov/PRODOCS/ceres/SRBAVG/Quality_Summaries/srbavg_ed2d/nestedgrid.html
	if (fieldtype == 1) {		// Calculate the latitude

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

	if (fieldtype == 2) {		// Calculate the longitude

		int i, j, k;
		int lonextent;
		int latindex_south, latindex_north, latrange;

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

		for (int i = 0; i < count[0]; i++) {
			for (int j = 0; j < count[1]; j++) {
				val[i][j] =
					orival[offset[0] + step[0] * i][offset[1] + step[1] * j];
			}
		}
	}
	set_value ((dods_float32 *) (&val[0][0]), nelms);

}

// This function calculate zonal average(longitude is fixed) of CERES SAVG and CERES_ISCCP_DAY_LIKE. 
void
HDFSPArrayGeoField::readcersavgid1 (int *offset, int *count, int *step,
									int nelms)
{


	// Following CERES Nested grid
	// URL http://eosweb.larc.nasa.gov/PRODOCS/ceres/SRBAVG/Quality_Summaries/srbavg_ed2d/nestedgrid.html
	if (fieldtype == 1) {		// Calculate the latitude
		int dimsize0 = 180;
		float32 val[count[0]];
		float32 orival[dimsize0];

		for (int i = 0; i < dimsize0; i++)
			orival[i] = 89.5 - i;

		for (int i = 0; i < count[0]; i++) {
			val[i] = orival[offset[0] + step[0] * i];
		}
		set_value ((dods_float32 *) (&val[0]), nelms);

	}

	if (fieldtype == 2) {		// Calculate the longitude
		// Assume the longitude is 0 in average
		float32 val = 0;

		if (nelms > 1)
			InternalErr (__FILE__, __LINE__,
						 "the number of element must be 1");
		set_value ((dods_float32 *) (&val), nelms);

	}

}

// Calculate CERES AVG and SYN lat and lon.
// We just need to retrieve lat/lon from the field.
void
HDFSPArrayGeoField::readceravgsyn (int32 * offset32, int32 * count32,
								   int32 * step32, int nelms)
{

	// 
	int i;
	int32 sdid, sdsid;

	sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);

	if (sdid < 0) {
		ostringstream eherr;

		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	int32 sdsindex = SDreftoindex (sdid, sdsref);

	if (sdsindex == -1) {
		SDend (sdid);
		ostringstream eherr;

		eherr << "SDS index " << sdsindex << " is not right.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	sdsid = SDselect (sdid, sdsindex);
	if (sdsid < 0) {
		SDend (sdid);
		ostringstream eherr;

		eherr << "SDselect failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
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
		SDend (sdid);
		throw InternalErr (__FILE__, __LINE__,
						   "datatype is not float, unsupported.");
	case DFNT_FLOAT32:
		{
			float32 *val = new float32[nelms];

			r = SDreaddata (sdsid, offset32, step32, count32, val);
			if (r != 0) {
				delete[](float32 *) val;
				SDendaccess (sdsid);
				SDend (sdid);
				ostringstream eherr;

				eherr << "SDreaddata failed";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
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

			set_value ((dods_float32 *) val, nelms);
			delete[](float32 *) val;
			break;
		}
	case DFNT_FLOAT64:
		{
			float64 *val = new float64[nelms];

			r = SDreaddata (sdsid, offset32, step32, count32, val);
			if (r != 0) {
				delete[](float64 *) val;
				SDendaccess (sdsid);
				SDend (sdid);
				ostringstream eherr;

				eherr << "SDreaddata failed";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
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


			set_value ((dods_float64 *) val, nelms);
			delete[](float64 *) val;
			break;
		}
	default:
		SDendaccess (sdsid);
		SDend (sdid);
		InternalErr (__FILE__, __LINE__, "unsupported data type.");
	}

	r = SDendaccess (sdsid);
	if (r != 0) {
		SDend (sdid);
		ostringstream eherr;

		eherr << "SDendaccess failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	r = SDend (sdid);
	if (r != 0) {
		ostringstream eherr;

		eherr << "SDend failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

}

// Calculate CERES ES4 and GEO lat/lon.
// We have to retrieve the original lat/lon and condense it from >1-D to 1-D.
void
HDFSPArrayGeoField::readceres4ig (int32 * offset32, int32 * count32,
								  int32 * step32, int nelms)
{

	int32 sdid = 0;
        int32 sdsid = 0;
	intn status = 0;

	sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
	if (sdid < 0) {
		ostringstream eherr;

		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	int32 sdsindex = SDreftoindex (sdid, (int32) sdsref);
	if (sdsindex == -1) {
		SDend (sdid);
		ostringstream eherr;

		eherr << "SDS index " << sdsindex << " is not right.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	sdsid = SDselect (sdid, sdsindex);
	if (sdsid < 0) {
		SDend (sdid);
		ostringstream eherr;

		eherr << "SDselect failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	int32 sdsrank, sds_dtype, n_attrs;
	char sdsname[H4_MAX_NC_NAME];
	int32 dim_sizes[H4_MAX_VAR_DIMS];

	status =
		SDgetinfo (sdsid, sdsname, &sdsrank, dim_sizes, &sds_dtype, &n_attrs);
	if (status < 0) {
		SDendaccess (sdsid);
		SDend (sdid);
		ostringstream eherr;

		eherr << "SDgetinfo failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	int32 *orioffset32 = new int32[sdsrank];
	int32 *oricount32 = new int32[sdsrank];
	int32 *oristep32 = new int32[sdsrank];

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
		SDend (sdid);
		HDFCFUtil::ClearMem2 (orioffset32, oricount32, oristep32);
		throw InternalErr (__FILE__, __LINE__,
						   "datatype is not float, unsupported.");
	case DFNT_FLOAT32:
		{
			float32 *val = new float32[nelms];

			if (fieldtype == 1) {
				if (sptype == CER_CGEO) {
					if (sdsrank != 3) {
						SDendaccess (sdsid);
						SDend (sdid);
						delete[](float32 *) val;
						HDFCFUtil::ClearMem2 (orioffset32, oricount32,
											  oristep32);
						throw InternalErr (__FILE__, __LINE__,
										   "For CER_ISCCP-D2like-GEO case, lat/lon must be 3-D");
					}
					orioffset32[0] = 0;
					orioffset32[1] = offset32[0];	// The second dimension of the original latitude should be condensed
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
						SDend (sdid);
						delete[](float32 *) val;
						HDFCFUtil::ClearMem2 (orioffset32, oricount32,
											  oristep32);
						throw InternalErr (__FILE__, __LINE__,
										   "For CER_ES4 case, lat/lon must be 2-D");
					}
					orioffset32[0] = offset32[0];	// The first dimension of the original latitude should be condensed
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
						SDend (sdid);
						delete[](float32 *) val;
						HDFCFUtil::ClearMem2 (orioffset32, oricount32,
											  oristep32);
						throw InternalErr (__FILE__, __LINE__,
										   "For CER_ISCCP-D2like-GEO case, lat/lon must be 3-D");
					}
					orioffset32[0] = 0;
					orioffset32[2] = offset32[0];	// The third dimension of the original latitude should be condensed
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
						SDend (sdid);
						delete[](float32 *) val;
						HDFCFUtil::ClearMem2 (orioffset32, oricount32,
											  oristep32);
						throw InternalErr (__FILE__, __LINE__,
										   "For CER_ES4 case, lat/lon must be 2-D");
					}
					orioffset32[1] = offset32[0];	// The second dimension of the original latitude should be condensed
					orioffset32[0] = 0;
					oricount32[1] = count32[0];
					oricount32[0] = 1;
					oristep32[1] = step32[0];
					oristep32[0] = 1;
				}
			}

			r = SDreaddata (sdsid, orioffset32, oristep32, oricount32, val);
			if (r != 0) {
				SDendaccess (sdsid);
				SDend (sdid);
				delete[](float32 *) val;
				HDFCFUtil::ClearMem2 (orioffset32, oricount32, oristep32);

				ostringstream eherr;

				eherr << "SDreaddata failed";
				throw InternalErr (__FILE__, __LINE__, eherr.str ());
			}

			if (fieldtype == 1)
				for (int i = 0; i < nelms; i++)
					val[i] = 90 - val[i];
			if (fieldtype == 2) {
				// Since Panoply cannot handle the case when the longitude is jumped from 180 to -180
				// So turn it off and see if it works with other clients,change my mind, should contact Panoply developer to solve this

				for (int i = 0; i < nelms; i++)
					if (val[i] > 180.0)
						val[i] = val[i] - 360.0;
			}

			set_value ((dods_float32 *) val, nelms);
			delete[](float32 *) val;
			break;
		}
	default:
		SDendaccess (sdsid);
		SDend (sdid);
		HDFCFUtil::ClearMem2 (orioffset32, oricount32, oristep32);
		InternalErr (__FILE__, __LINE__, "unsupported data type.");
	}

	r = SDendaccess (sdsid);
	if (r != 0) {
		SDend (sdid);
		HDFCFUtil::ClearMem2 (orioffset32, oricount32, oristep32);
		InternalErr (__FILE__, __LINE__, "unsupported data type.");
		ostringstream eherr;

		eherr << "SDendaccess failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	r = SDend (sdid);
	if (r != 0) {
		HDFCFUtil::ClearMem2 (orioffset32, oricount32, oristep32);
		ostringstream eherr;

		eherr << "SDend failed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	delete[]orioffset32;
	delete[]oricount32;
	delete[]oristep32;

}

void
HDFSPArrayGeoField::readcerzavg (int32 * offset32, int32 * count32,
								 int32 * step32, int nelms)
{

	if (fieldtype == 1) {
		float32 *val = new float32[nelms];
		float32 latstep = 1.0;

		for (int i = 0; i < nelms; i++)
			val[i] =
				89.5 - ((int) (offset32[0]) +
						((int) (step32[0])) * i) * latstep;
		set_value ((dods_float32 *) val, nelms);
		delete[] val;
	}
	if (fieldtype == 2) {
		if (count32[0] != 1 || nelms != 1)
			throw InternalErr (__FILE__, __LINE__,
							   "Longitude should only have one value for zonal mean");

		float32 val = 0.;		// our convention

		set_value ((dods_float32 *) & val, nelms);
	}
}


// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDFSPArrayGeoField::format_constraint (int *offset, int *step, int *count)
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
		count[id] = ((stop - start) / stride) + 1;	// count of elements
		nels *= count[id];		// total number of values for variable

		 BESDEBUG("h4",
			 "=format_constraint():" << "id=" << id << " offset=" <<
			 offset[id]
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
                                                                                                                  int majordim,
                                                                                                                  int minordim,
                                                                                                                  T * latlon,
                                                                                                                  int32 * offset,
                                                                                                                  int32 * count,
                                                                                                                  int32 * step)
{

//          float64 templatlon[majordim][minordim];
        T (*templatlonptr)[majordim][minordim] = (typeof templatlonptr) latlon;
     int i, j, k;

    // do subsetting
    // Find the correct index
    int dim0count = count[0];
    int dim1count = count[1]; 
    int dim0index[dim0count], dim1index[dim1count];

    for (i = 0; i < count[0]; i++)      // count[0] is the least changing dimension 
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

}

