/////////////////////////////////////////////////////////////////////////////
// retrieves the latitude and longitude of  the HDF-EOS2 Grid
//  Authors:   MuQun Yang <myang6@hdfgroup.org> 
// Copyright (c) 2009-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB

#include "HDFEOS2ArrayGridGeoField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "HDFEOS2.h"
#include "InternalErr.h"
#include <BESDebug.h>
#include "HDFCFUtil.h"

#include "misrproj.h"
#include "errormacros.h"
#include <proj.h>

#define SIGNED_BYTE_TO_INT32 1

// These two functions are used to handle MISR products with the SOM projections.
extern "C" {
    int inv_init(int insys, int inzone, double *inparm, int indatum, char *fn27, char *fn83, int *iflg, int (*inv_trans[])(double, double, double*, double*));
    int sominv(double y, double x, double *lon, double *lat);
}

bool
HDFEOS2ArrayGridGeoField::read ()
{
    // Currently The latitude and longitude rank from HDF-EOS2 grid must be either 1-D or 2-D.
    // However, For SOM projection the final rank will become 3. 
    if (rank < 1 || rank > 2) {
        throw InternalErr (__FILE__, __LINE__, "The rank of geo field is greater than 2, currently we don't support 3-D lat/lon cases.");
    }

    // MISR SOM file's final rank is 3. So declare a new variable. 
    int final_rank = -1;

    if (true == condenseddim)
        final_rank = 1;
    else if(4 == specialformat)// For the SOM projection, the final output of latitude/longitude rank should be 3.
        final_rank = 3;
    else 
        final_rank = rank;

    vector<int> offset;
    offset.resize(final_rank);
    vector<int> count;
    count.resize(final_rank);
    vector<int> step;
    step.resize(final_rank);

    int nelms = -1;

    // Obtain the number of the subsetted elements
    nelms = format_constraint (&offset[0], &step[0], &count[0]);

    // Define function pointers to handle both grid and swath Note: in
    // this code, we only handle grid, implementing this way is to
    // keep the same style as the read functions in other files.
    int32 (*openfunc) (char *, intn);
    intn (*closefunc) (int32);
    int32 (*attachfunc) (int32, char *);
    intn (*detachfunc) (int32);
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);
    intn (*readfieldfunc) (int32, char *, int32 *, int32 *, int32 *, void *);

    std::string datasetname;
    openfunc      = GDopen;
    closefunc     = GDclose;
    attachfunc    = GDattach;
    detachfunc    = GDdetach;
    fieldinfofunc = GDfieldinfo;
    readfieldfunc = GDreadfield;
    datasetname   = gridname;

    int32 gfid   = -1;
    int32 gridid = -1;

    // Obtain the grid id
    gfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
    if (gfid < 0) {
        ostringstream eherr;
        eherr << "File " << filename.c_str () << " cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Attach the grid id; make the grid valid.
    gridid = attachfunc (gfid, const_cast < char *>(datasetname.c_str ()));
    if (gridid < 0) {
        // Can use std::string here. jhrg
        closefunc(gfid);
        ostringstream eherr;
        eherr << "Grid " << datasetname.c_str () << " cannot be attached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // SOM projection should be handled differently. 
    if(specialformat == 4) {// SOM projection
        CalculateSOMLatLon(gridid, &offset[0], &count[0], &step[0], nelms);
        return false;
    }

    // We define offset,count and step in int32 datatype.
    vector<int32>offset32;
    offset32.resize(rank);

    vector<int32>count32;
    count32.resize(rank);

    vector<int32>step32;
    step32.resize(rank);


    // Obtain offset32 with the correct rank, the rank of lat/lon of
    // GEO and CEA projections in the file may be 2 instead of 1.
    getCorrectSubset (&offset[0], &count[0], &step[0], &offset32[0], &count32[0], &step32[0], condenseddim, ydimmajor, fieldtype, rank);

    // The following case handles when the lat/lon is not provided.
    if (llflag == false) {		// We have to calculate the lat/lon

        vector<float64>latlon;
        latlon.resize(nelms);

        int32 projcode = -1; 
        int32 zone     = -1;
        int32 sphere   = -1;
        float64 params[16];
        intn r = -1;

        r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
        if (r!=0) {
            detachfunc(gridid);
            closefunc(gfid);
            throw InternalErr (__FILE__, __LINE__, "GDprojinfo failed");
        }

        // Handle LAMAZ projection first.
        if (GCTP_LAMAZ == projcode) { 
            CalculateLAMAZLatLon(gridid, fieldtype, &latlon[0], &offset[0], &count[0], &step[0], nelms);
            set_value ((dods_float64 *) &latlon[0], nelms);
            detachfunc(gridid);
            closefunc(gfid);
            return false;
        }
         
        // Aim to handle large MCD Grid such as 21600*43200 lat,lon
        if (specialformat == 1) {

            CalculateLargeGeoLatLon(gfid,gridid, fieldtype,&latlon[0], &offset[0], &count[0], &step[0], nelms);
            set_value((dods_float64 *)&latlon[0],nelms);
            detachfunc(gridid);
            closefunc(gfid);
            return false;
        }

        // Now handle other cases.
        if (specialformat == 3)	// Have to provide latitude and longitude by ourselves
            CalculateSpeLatLon (gridid, fieldtype, &latlon[0], &offset32[0], &count32[0], &step32[0], nelms);
        else // This is mostly general case, it will calculate lat/lon with GDij2ll.
            CalculateLatLon (gridid, fieldtype, specialformat, &latlon[0],
                             &offset32[0], &count32[0], &step32[0], nelms);

        // Some longitude values need to be corrected.
        if (speciallon && fieldtype == 2) {
            CorSpeLon (&latlon[0], nelms);
        }

        set_value ((dods_float64 *) &latlon[0], nelms);

        return false;
    }


    // Now lat and lon are stored as HDF-EOS2 fields. We need to read the lat and lon values from the fields.
    int32 tmp_rank = -1;
    int32 tmp_dims[rank];
    char tmp_dimlist[1024];
    int32 type     = -1;
    intn r         = -1;

    // Obtain field info.
    r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
                       &tmp_rank, tmp_dims, &type, tmp_dimlist);

    if (r != 0) {
        detachfunc(gridid);
        closefunc(gfid);
        ostringstream eherr;
        eherr << "Field " << fieldname.c_str () << " information cannot be obtained.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Retrieve dimensions and X-Y coordinates of corners
    int32 xdim = 0;
    int32 ydim = 0;

    float64 upleft[2];
    float64 lowright[2];

    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r != 0) {
        detachfunc(gridid);
        closefunc(gfid);
        ostringstream eherr;

        eherr << "Grid " << datasetname.c_str () << " information cannot be obtained.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Retrieve all GCTP projection information
    int32 projcode = -1;
    int32 zone     = -1;
    int32 sphere   = -1;
    float64 params[16];

    r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
    if (r != 0) {
        detachfunc(gridid);
        closefunc(gfid);
        ostringstream eherr;
        eherr << "Grid " << datasetname.c_str () << " projection info. cannot be obtained.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if (projcode != GCTP_GEO) {	// Just retrieve the data like other fields
        // We have to loop through all datatype and read the lat/lon out.
        switch (type) {
        case DFNT_INT8:
            {
                vector<int8> val;
                val.resize(nelms);
                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                if (r != 0) {
                    detachfunc(gridid);
                    closefunc(gfid);
                    ostringstream eherr;

                    eherr << "field " << fieldname.c_str () << "cannot be read.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                // DAP2 requires the map of SIGNED_BYTE to INT32 if
                // SIGNED_BYTE_TO_INT32 is defined.
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
        case DFNT_CHAR8:

            {
                vector<uint8> val;
                val.resize(nelms);
                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                if (r != 0) {
                    detachfunc(gridid);
                    closefunc(gfid);
                    ostringstream eherr;

                    eherr << "field " << fieldname.c_str () << "cannot be read.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                set_value ((dods_byte *) &val[0], nelms);

            }
            break;

        case DFNT_INT16:

            {
                vector<int16> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                if (r != 0) {
                    detachfunc(gridid);
                    closefunc(gfid);

                    ostringstream eherr;

                    eherr << "field " << fieldname.c_str () << "cannot be read.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                set_value ((dods_int16 *) &val[0], nelms);

            }
            break;
        case DFNT_UINT16:

            {
                vector<uint16> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                if (r != 0) {
                    detachfunc(gridid);
                    closefunc(gfid);

                    ostringstream eherr;

                    eherr << "field " << fieldname.c_str () << "cannot be read.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                set_value ((dods_uint16 *) &val[0], nelms);
            }
            break;
        case DFNT_INT32:

            {
                vector<int32> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                if (r != 0) {
                    detachfunc(gridid);
                    closefunc(gfid);

                    ostringstream eherr;

                    eherr << "field " << fieldname.c_str () << "cannot be read.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                set_value ((dods_int32 *) &val[0], nelms);
            }
            break;
        case DFNT_UINT32:

            {
                vector<uint32> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                if (r != 0) {
                    detachfunc(gridid);
                    closefunc(gfid);

                    ostringstream eherr;

                    eherr << "field " << fieldname.c_str () << "cannot be read.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                set_value ((dods_uint32 *) &val[0], nelms);
            }
            break;
        case DFNT_FLOAT32:

            {
                vector<float32> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                if (r != 0) {
                    detachfunc(gridid);
                    closefunc(gfid);

                    ostringstream eherr;

                    eherr << "field " << fieldname.c_str () << "cannot be read.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                set_value ((dods_float32 *) &val[0], nelms);
            }
            break;
        case DFNT_FLOAT64:

            {
                vector<float64> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                if (r != 0) {
                    detachfunc(gridid);
                    closefunc(gfid);

                    ostringstream eherr;

                    eherr << "field " << fieldname.c_str () << "cannot be read.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                set_value ((dods_float64 *) &val[0], nelms);
            }
            break;
        default:
            InternalErr (__FILE__, __LINE__, "unsupported data type.");

        }
    }
    else {// Only handle special cases for the Geographic Projection 
        // We find that lat/lon of the geographic projection in some
        // files include fill values. So we recalculate lat/lon based
        // on starting value,step values and number of steps.
        // GDgetfillvalue will return 0 if having fill values. 
        // The other returned value indicates no fillvalue is found inside the lat or lon.
        switch (type) {
        case DFNT_INT8:
            {
                vector<int8> val;
                val.resize(nelms);


                int8 fillvalue = 0;

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    int ifillvalue = fillvalue;

                    vector <int8> temp_total_val;
                    temp_total_val.resize(xdim*ydim*4);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        NULL, NULL, NULL, (void *)(&temp_total_val[0]));

                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }

                    // Recalculate lat/lon for the geographic projection lat/lon that has fill values
                    HandleFillLatLon(temp_total_val, (int8*)&val[0],ydimmajor,fieldtype,xdim,ydim,&offset32[0],&count32[0],&step32[0],ifillvalue);

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
			&offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }
                }

                if (speciallon && fieldtype == 2)
                    CorSpeLon ((int8 *) &val[0], nelms);


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
      case DFNT_CHAR8:
            {
                vector<uint8> val;
                val.resize(nelms);

                uint8 fillvalue = 0;

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    int ifillvalue =  fillvalue;
                    vector <uint8> temp_total_val;
                    temp_total_val.resize(xdim*ydim*4);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        NULL, NULL, NULL, (void *)(&temp_total_val[0]));

                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }

                    HandleFillLatLon(temp_total_val, (uint8*)&val[0],ydimmajor,fieldtype,xdim,ydim,&offset32[0],&count32[0],&step32[0],ifillvalue);

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }
                }
	    
                if (speciallon && fieldtype == 2)
                    CorSpeLon ((uint8 *) &val[0], nelms);
                set_value ((dods_byte *) &val[0], nelms);

            }
            break;

        case DFNT_INT16:
            {
                vector<int16> val;
                val.resize(nelms);

                int16 fillvalue = 0;

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    int ifillvalue =  fillvalue;
                    vector <int16> temp_total_val;
                    temp_total_val.resize(xdim*ydim*4);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        NULL, NULL, NULL, (void *)(&temp_total_val[0]));

                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }

                    HandleFillLatLon(temp_total_val, (int16*)&val[0],ydimmajor,fieldtype,xdim,ydim,&offset32[0],&count32[0],&step32[0],ifillvalue);

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }
                }

	    
                if (speciallon && fieldtype == 2)
                    CorSpeLon ((int16 *) &val[0], nelms);

                set_value ((dods_int16 *) &val[0], nelms);
            }
            break;
        case DFNT_UINT16:
            {
                uint16  fillvalue = 0;
                vector<uint16> val;
                val.resize(nelms);


                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    int ifillvalue =  fillvalue;

                    vector <uint16> temp_total_val;
                    temp_total_val.resize(xdim*ydim*4);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        NULL, NULL, NULL, (void *)(&temp_total_val[0]));

                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }

                    HandleFillLatLon(temp_total_val, (uint16*)&val[0],ydimmajor,fieldtype,xdim,ydim,&offset32[0],&count32[0],&step32[0],ifillvalue);
                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
			&offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    } 
                }

                if (speciallon && fieldtype == 2)
                    CorSpeLon ((uint16 *) &val[0], nelms);

                set_value ((dods_uint16 *) &val[0], nelms);

            }
            break;

        case DFNT_INT32:
            {
                vector<int32> val;
                val.resize(nelms);

                int32 fillvalue = 0;

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    int ifillvalue =  fillvalue;

                    vector <int32> temp_total_val;
                    temp_total_val.resize(xdim*ydim*4);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        NULL, NULL, NULL, (void *)(&temp_total_val[0]));

                    if (r != 0) {
                        ostringstream eherr;
                        detachfunc(gridid);
                        closefunc(gfid);


                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }

                    HandleFillLatLon(temp_total_val, (int32*)&val[0],ydimmajor,fieldtype,xdim,ydim,&offset32[0],&count32[0],&step32[0],ifillvalue);

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    } 

                }
                if (speciallon && fieldtype == 2)
                    CorSpeLon ((int32 *) &val[0], nelms);

                set_value ((dods_int32 *) &val[0], nelms);

            }
            break;
        case DFNT_UINT32:

            {
                vector<uint32> val;
                val.resize(nelms);

                uint32 fillvalue = 0;

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    // this may cause overflow. Although we don't find the overflow in the NASA HDF products, may still fix it later. KY 2012-8-20
                    int ifillvalue = (int)fillvalue;
                    vector <uint32> temp_total_val;
                    temp_total_val.resize(xdim*ydim*4);
                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        NULL, NULL, NULL, (void *)(&temp_total_val[0]));

                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }

                    HandleFillLatLon(temp_total_val, (uint32*)&val[0],ydimmajor,fieldtype,xdim,ydim,&offset32[0],&count32[0],&step32[0],ifillvalue);

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
			&offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    } 

                }
                if (speciallon && fieldtype == 2)
                    CorSpeLon ((uint32 *) &val[0], nelms);

                set_value ((dods_uint32 *) &val[0], nelms);

            }
            break;
        case DFNT_FLOAT32:

            {
                vector<float32> val;
                val.resize(nelms);

                float32 fillvalue =0;
                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);


                if (r == 0) {
                    // May cause overflow,not find this happen in NASA HDF files, may still need to handle later.
                    // KY 2012-08-20
                    int ifillvalue =(int)fillvalue;
     
                    vector <float32> temp_total_val;
                    temp_total_val.resize(xdim*ydim*4);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        NULL, NULL, NULL, (void *)(&temp_total_val[0]));

                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }

                    HandleFillLatLon(temp_total_val, (float32*)&val[0],ydimmajor,fieldtype,xdim,ydim,&offset32[0],&count32[0],&step32[0],ifillvalue);

                }
                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }

                }
                if (speciallon && fieldtype == 2)
                    CorSpeLon ((float32 *) &val[0], nelms);

                set_value ((dods_float32 *) &val[0], nelms);

            }
            break;
        case DFNT_FLOAT64:

            {
                vector<float64> val;
                val.resize(nelms);

                float64 fillvalue = 0;
                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    // May cause overflow,not find this happen in NASA HDF files, may still need to handle later.
                    // KY 2012-08-20
 
                    int ifillvalue = (int)fillvalue;
                    vector <float64> temp_total_val;
                    temp_total_val.resize(xdim*ydim*4);
                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        NULL, NULL, NULL, (void *)(&temp_total_val[0]));

                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    }

                    HandleFillLatLon(temp_total_val, (float64*)&val[0],ydimmajor,fieldtype,xdim,ydim,&offset32[0],&count32[0],&step32[0],ifillvalue);

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        &offset32[0], &step32[0], &count32[0], (void*)(&val[0]));
                    if (r != 0) {
                        detachfunc(gridid);
                        closefunc(gfid);

                        ostringstream eherr;

                        eherr << "field " << fieldname.c_str () << "cannot be read.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                    } 

                }
                if (speciallon && fieldtype == 2)
                    CorSpeLon ((float64 *) &val[0], nelms);

                set_value ((dods_float64 *) &val[0], nelms);

            }
            break;
        default:
            detachfunc(gridid);
            closefunc(gfid);
            InternalErr (__FILE__, __LINE__, "unsupported data type.");
        }

    }

    r = detachfunc (gridid);
    if (r != 0) {
        closefunc(gfid);

        ostringstream eherr;

        eherr << "Grid " << datasetname.c_str () << " cannot be detached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    r = closefunc (gfid);
    if (r != 0) {
        ostringstream eherr;

        eherr << "Grid " << filename.c_str () << " cannot be closed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    return false;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFEOS2ArrayGridGeoField::format_constraint (int *offset, int *step,
                                             int *count)
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
        count[id] = ((stop - start) / stride) + 1;// count of elements
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

// Calculate lat/lon based on HDF-EOS2 APIs.
void
HDFEOS2ArrayGridGeoField::CalculateLatLon (int32 gridid, int fieldtype,
                                           int specialformat,
                                           float64 * outlatlon,
                                           int32 * offset, int32 * count,
                                           int32 * step, int nelms)
{

    // Retrieve dimensions and X-Y coordinates of corners 
    int32 xdim = 0;
    int32 ydim = 0; 
    int r = -1;
    float64 upleft[2];
    float64 lowright[2];

    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r != 0) {
        ostringstream eherr;

        eherr << "cannot obtain grid information.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // The coordinate values(MCD products) are set to -180.0, -90.0, etc.
    // We have to change them to DDDMMMSSS.SS format, so 
    // we have to multiply them by 1000000.
    if (specialformat == 1) {
        upleft[0] = upleft[0] * 1000000;
        upleft[1] = upleft[1] * 1000000;
        lowright[0] = lowright[0] * 1000000;
        lowright[1] = lowright[1] * 1000000;
    }

    // The coordinate values(CERES TRMM) are set to default,which are zeros.
    // Based on the grid names and size, we find it covers the whole global.
    // So we set the corner coordinates to (-180000000.00,90000000.00) and
    // (180000000.00,-90000000.00).
    if (specialformat == 2) {
        upleft[0] = 0.0;
        upleft[1] = 90000000.0;
        lowright[0] = 360000000.0;
        lowright[1] = -90000000.0;
    }

    // Retrieve all GCTP projection information 
    int32 projcode = 0; 
    int32 zone = 0; 
    int32 sphere = 0;
    float64 params[16];

    r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
    if (r != 0) {
        ostringstream eherr;

        eherr << "cannot obtain grid projection information";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Retrieve pixel registration information 
    int32 pixreg = 0; 

    r = GDpixreginfo (gridid, &pixreg);
    if (r != 0) {
        ostringstream eherr;

        eherr << "cannot obtain grid pixel registration info.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    //Retrieve grid pixel origin 
    int32 origin = 0; 

    r = GDorigininfo (gridid, &origin);
    if (r != 0) {
        ostringstream eherr;

        eherr << "cannot obtain grid origin info.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    vector<int32>rows;
    vector<int32>cols;
    vector<float64>lon;
    vector<float64>lat;
    rows.resize(xdim*ydim);
    cols.resize(xdim*ydim);
    lon.resize(xdim*ydim);
    lat.resize(xdim*ydim);


    int i = 0, j = 0, k = 0; 

    if (ydimmajor) {
        /* Fill two arguments, rows and columns */
        // rows             cols
        //   /- xdim  -/      /- xdim  -/
        //   0 0 0 ... 0      0 1 2 ... x
        //   1 1 1 ... 1      0 1 2 ... x
        //       ...              ...
        //   y y y ... y      0 1 2 ... x

        for (k = j = 0; j < ydim; ++j) {
            for (i = 0; i < xdim; ++i) {
                rows[k] = j;
                cols[k] = i;
                ++k;
            }
        }
    }
    else {
        // rows             cols
        //   /- ydim  -/      /- ydim  -/
        //   0 1 2 ... y      0 0 0 ... y
        //   0 1 2 ... y      1 1 1 ... y
        //       ...              ...
        //   0 1 2 ... y      2 2 2 ... y

        for (k = j = 0; j < xdim; ++j) {
            for (i = 0; i < ydim; ++i) {
                rows[k] = i;
                cols[k] = j;
                ++k;
            }
        }
    }


    r = GDij2ll (projcode, zone, params, sphere, xdim, ydim, upleft, lowright,
                 xdim * ydim, &rows[0], &cols[0], &lon[0], &lat[0], pixreg, origin);

    if (r != 0) {
        ostringstream eherr;
        eherr << "cannot calculate grid latitude and longitude";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // 2-D Lat/Lon, need to decompose the data for subsetting.
    if (nelms == (xdim * ydim)) {	// no subsetting return all, for the performance reason.
        if (fieldtype == 1)
            memcpy (outlatlon, &lat[0], xdim * ydim * sizeof (double));
        else
            memcpy (outlatlon, &lon[0], xdim * ydim * sizeof (double));
    }
    else {						// Messy subsetting case, needs to know the major dimension
        if (ydimmajor) {
            if (fieldtype == 1) // Lat 
                LatLon2DSubset (outlatlon, ydim, xdim, &lat[0], offset, count,
                                step);
            else // Lon
                LatLon2DSubset (outlatlon, ydim, xdim, &lon[0], offset, count,
                                step);
        }
        else {
            if (fieldtype == 1) // Lat
                LatLon2DSubset (outlatlon, xdim, ydim, &lat[0], offset, count,
                                step);
            else // Lon
                LatLon2DSubset (outlatlon, xdim, ydim, &lon[0], offset, count,
                    step);
        }
    }
}


// Map the subset of the lat/lon buffer to the corresponding 2D array.
template<class T> void
HDFEOS2ArrayGridGeoField::LatLon2DSubset (T * outlatlon, int majordim,
                                          int minordim, T * latlon,
                                          int32 * offset, int32 * count,
                                          int32 * step)
{

    // float64 templatlon[majordim][minordim];
    T (*templatlonptr)[majordim][minordim] =
        (typeof templatlonptr) latlon;
    int i, j, k;

    // do subsetting
    // Find the correct index
    int dim0count = count[0];
    int dim1count = count[1];
    int dim0index[dim0count], dim1index[dim1count];

    for (i = 0; i < count[0]; i++)	// count[0] is the least changing dimension 
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

// Some HDF-EOS2 geographic projection lat/lon fields have fill values. 
// This routine is used to replace those fill values by using the formula to calculate
// the lat/lon of the geographic projection.
template < class T > bool HDFEOS2ArrayGridGeoField::CorLatLon (T * latlon,
                                                               int fieldtype,
                                                               int elms,
                                                               int fv)
{

    // Since we only find the contiguous fill value of lat/lon from some position to the end 
    // So to speed up the performance, the following algorithm is limited to that case.

    // The first two values cannot be fill value.
    // We find a special case :the first latitude(index 0) is a special value.
    // So we need to have three non-fill values to calculate the increment.

    if (elms < 3) {
        for (int i = 0; i < elms; i++)
            if ((int) (latlon[i]) == fv)
                return false;
        return true;
    }

    // Number of elements is greater than 3.

    for (int i = 0; i < 3; i++)	// The first three elements should not include fill value.
        if ((int) (latlon[i]) == fv)
            return false;

    if ((int) (latlon[elms - 1]) != fv)
        return true;

    T increment = latlon[2] - latlon[1];

    int index = 0;

    // Find the first fill value
    index = findfirstfv (latlon, 0, elms - 1, fv);
    if (index < 2) {
        ostringstream eherr;
        eherr << "cannot calculate the fill value. ";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    for (int i = index; i < elms; i++) {

        latlon[i] = latlon[i - 1] + increment;

        // The latitude must be within (-90,90)
        if (i != (elms - 1) && (fieldtype == 1) &&
            ((float) (latlon[i]) < -90.0 || (float) (latlon[i]) > 90.0))
            return false;

        // For longitude, since some files use (0,360)
        // some files use (-180,180), for simple check
        // we just choose (-180,360). 
        // I haven't found longitude has missing value.
        if (i != (elms - 1) && (fieldtype == 2) &&
            ((float) (latlon[i]) < -180.0 || (float) (latlon[i]) > 360.0))
            return false;
    }
    if (fieldtype == 1) {
        if ((float) (latlon[elms - 1]) < -90.0)
            latlon[elms - 1] = (T)-90;
        if ((float) (latlon[elms - 1]) > 90.0)
            latlon[elms - 1] = (T)90;
    }

    if (fieldtype == 2) {
        if ((float) (latlon[elms - 1]) < -180.0)
            latlon[elms - 1] = (T)-180.0;
        if ((float) (latlon[elms - 1]) > 360.0)
            latlon[elms - 1] = (T)360.0;
    }
    return true;
}

// Make longitude (0-360) to (-180 - 180)
template < class T > void
HDFEOS2ArrayGridGeoField::CorSpeLon (T * lon, int xdim)
{
    int i;
    float64 accuracy = 1e-3;	// in case there is a lon value = 180.0 in the middle, make the error to be less than 1e-3.
    float64 temp = 0;

    // Check if this lon. field falls to the (0-360) case.
    int speindex = -1;

    for (i = 0; i < xdim; i++) {
        if ((double) lon[i] < 180.0)
            temp = 180.0 - (double) lon[i];
        if ((double) lon[i] > 180.0)
            temp = (double) lon[i] - 180.0;

        if (temp < accuracy) {
            speindex = i;
            break;
        }
        else if ((static_cast < double >(lon[i]) < 180.0)
                 &&(static_cast<double>(lon[i + 1]) > 180.0)) {
            speindex = i;
            break;
        }
        else
            continue;
    }

    if (speindex != -1) {
        for (i = speindex + 1; i < xdim; i++) {
            lon[i] =
                static_cast < T > (static_cast < double >(lon[i]) - 360.0);
        }
    }
    return;
}

// Get correct subsetting indexes. This is especially useful when 2D lat/lon can be condensed to 1D.
void
HDFEOS2ArrayGridGeoField::getCorrectSubset (int *offset, int *count,
                                            int *step, int32 * offset32,
                                            int32 * count32, int32 * step32,
                                            bool condenseddim, bool ydimmajor,
                                            int fieldtype, int rank)
{

    if (rank == 1) {
        offset32[0] = (int32) offset[0];
        count32[0] = (int32) count[0];
        step32[0] = (int32) step[0];
    }
    else if (condenseddim) {

        // Since offset,count and step for some dimensions will always
        // be 1, so first assign offset32,count32,step32 to 1.
        for (int i = 0; i < rank; i++) {
            offset32[i] = 0;
            count32[i] = 1;
            step32[i] = 1;
        }

        if (ydimmajor && fieldtype == 1) {// YDim major, User: Lat[YDim], File: Lat[YDim][XDim]
            offset32[0] = (int32) offset[0];
            count32[0] = (int32) count[0];
            step32[0] = (int32) step[0];
        }
        else if (ydimmajor && fieldtype == 2) {	// YDim major, User: Lon[XDim],File: Lon[YDim][XDim]
            offset32[1] = (int32) offset[0];
            count32[1] = (int32) count[0];
            step32[1] = (int32) step[0];
        }
        else if (!ydimmajor && fieldtype == 1) {// XDim major, User: Lat[YDim], File: Lat[XDim][YDim]
            offset32[1] = (int32) offset[0];
            count32[1] = (int32) count[0];
            step32[1] = (int32) step[0];
        }
        else if (!ydimmajor && fieldtype == 2) {// XDim major, User: Lon[XDim], File: Lon[XDim][YDim]
            offset32[0] = (int32) offset[0];
            count32[0] = (int32) count[0];
            step32[0] = (int32) step[0];
        }

        else {// errors
            InternalErr (__FILE__, __LINE__,
                         "Lat/lon subset is wrong for condensed lat/lon");
        }
    }
    else {
        for (int i = 0; i < rank; i++) {
            offset32[i] = (int32) offset[i];
            count32[i] = (int32) count[i];
            step32[i] = (int32) step[i];
        }
    }
}

// Correct latitude and longitude that have fill values. Although I only found this
// happens for AIRS CO2 grids, I still implemented this as general as I can.

template <class T> void
HDFEOS2ArrayGridGeoField::HandleFillLatLon(vector<T> total_latlon, T* latlon,bool ydimmajor, int fieldtype, int32 xdim , int32 ydim, int32* offset, int32* count, int32* step, int fv)  {

   class vector <T> temp_lat;
   class vector <T> temp_lon;

   if (true == ydimmajor) {

        if (1 == fieldtype) {
            temp_lat.resize(ydim);
            for (int i = 0; i <(int)ydim; i++)
                temp_lat[i] = total_latlon[i*xdim];

            if (false == CorLatLon(&temp_lat[0],fieldtype,ydim,fv))
                InternalErr(__FILE__,__LINE__,"Cannot handle the fill values in lat/lon correctly");
           
            for (int i = 0; i <(int)(count[0]); i++)
                latlon[i] = temp_lat[offset[0] + i* step[0]];
        }
        else {

            temp_lon.resize(xdim);
            for (int i = 0; i <(int)xdim; i++)
                temp_lon[i] = total_latlon[i];


            if (false == CorLatLon(&temp_lon[0],fieldtype,xdim,fv))
                InternalErr(__FILE__,__LINE__,"Cannot handle the fill values in lat/lon correctly");
           
            for (int i = 0; i <(int)(count[1]); i++)
                latlon[i] = temp_lon[offset[1] + i* step[1]];

        }
   }
   else {

        if (1 == fieldtype) {
            temp_lat.resize(xdim);
            for (int i = 0; i <(int)xdim; i++)
                temp_lat[i] = total_latlon[i];

            if (false == CorLatLon(&temp_lat[0],fieldtype,ydim,fv))
                InternalErr(__FILE__,__LINE__,"Cannot handle the fill values in lat/lon correctly");
           
            for (int i = 0; i <(int)(count[1]); i++)
                latlon[i] = temp_lat[offset[1] + i* step[1]];
        }
        else {

            temp_lon.resize(ydim);
            for (int i = 0; i <(int)ydim; i++)
                temp_lon[i] = total_latlon[i*xdim];


            if (false == CorLatLon(&temp_lon[0],fieldtype,xdim,fv))
                InternalErr(__FILE__,__LINE__,"Cannot handle the fill values in lat/lon correctly");
           
            for (int i = 0; i <(int)(count[0]); i++)
                latlon[i] = temp_lon[offset[0] + i* step[0]];
        }

    }
}
            
// A helper recursive function to find the first filled value index.
template < class T > int
HDFEOS2ArrayGridGeoField::findfirstfv (T * array, int start, int end,
                                       int fillvalue)
{

    if (start == end || start == (end - 1)) {
        if (static_cast < int >(array[start]) == fillvalue)
            return start;
        else
            return end;
    }
    else {
        int current = (start + end) / 2;

        if (static_cast < int >(array[current]) == fillvalue)
            return findfirstfv (array, start, current, fillvalue);
        else
            return findfirstfv (array, current, end, fillvalue);
    }
}

// Calculate Special Latitude and Longitude.
//One MOD13C2 file doesn't provide projection code
// The upperleft and lowerright coordinates are all -1
// We have to calculate lat/lon by ourselves.
// Since it doesn't provide the project code, we double check their information
// and find that it covers the whole globe with 0.05 degree resolution.
// Lat. is from 90 to -90 and Lon is from -180 to 180.
void
HDFEOS2ArrayGridGeoField::CalculateSpeLatLon (int32 gridid, int fieldtype,
                                              float64 * outlatlon,
                                              int32 * offset32,
                                              int32 * count32, int32 * step32,
                                              int nelms)
{

    // Retrieve dimensions and X-Y coordinates of corners 
    int32 xdim = 0;
    int32 ydim = 0;
    int r = -1;
    float64 upleft[2];
    float64 lowright[2];

    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r != 0) {
        ostringstream eherr;
        eherr << "cannot obtain grid information.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }
    //Since this is a special calcuation out of using the GDij2ll function,
    // the rank is always assumed to be 2 and we condense to 1. So the 
    // count for longitude should be count[1] instead of count[0]. See function GetCorSubset

    // Since the project parameters in StructMetadata are all set to be default, I will use
    // the default HDF-EOS2 cell center as the origin of the coordinate. See the HDF-EOS2 user's guide
    // for details. KY 2012-09-10

    if(0 == xdim || 0 == ydim) 
        throw InternalErr(__FILE__,__LINE__,"xdim or ydim cannot be zero");

    if (fieldtype == 1) {
        double latstep = 180.0 / ydim;

        for (int i = 0; i < (int) (count32[0]); i++)
            outlatlon[i] = 90.0 -latstep/2 - latstep * (offset32[0] + i * step32[0]);
    }
    else {// Longitude should use count32[1] etc. 
        double lonstep = 360.0 / xdim;

        for (int i = 0; i < (int) (count32[1]); i++)
            outlatlon[i] = -180.0 + lonstep/2 + lonstep * (offset32[1] + i * step32[1]);
    }
}

// Calculate latitude and longitude for the MISR SOM projection HDF-EOS2 product.
// since the latitude and longitude of the SOM projection are 3-D, so we need to handle this projection in a special way. 
// Based on our current understanding, the third dimension size is always 180. 
// This is according to the MISR Lat/lon calculation document 
// at http://eosweb.larc.nasa.gov/PRODOCS/misr/DPS/DPS_v50_RevS.pdf
void
HDFEOS2ArrayGridGeoField::CalculateSOMLatLon(int32 gridid, int *start, int *count, int *step, int nelms)
{
    int32 projcode = -1;
    int32 zone = -1; 
    int32 sphere = -1;
    float64 params[NPROJ];
    intn r = -1;

    r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
    if (r!=0)
        throw InternalErr (__FILE__, __LINE__, "GDprojinfo doesn't return the correct values");

    int MAXNDIM = 10;
    int32 dim[MAXNDIM];
    char dimlist[STRLEN];
    r = GDinqdims(gridid, dimlist, dim);
    // r is the number of dims. or 0.
    // So the valid returned value can be greater than 0. Only throw error when r is less than 0.
    if (r<0)
        throw InternalErr (__FILE__, __LINE__, "GDinqdims doesn't return the correct values");

    bool is_block_180 = false;
    for(int i=0; i<MAXNDIM; i++)
    {
        if(dim[i]==NBLOCK)
        {
            is_block_180 = true;
            break;
        }
    }
    if(false == is_block_180) {
        ostringstream eherr;
        eherr <<"Number of Block is not " << NBLOCK ;
        throw InternalErr(__FILE__,__LINE__,eherr.str());
    }

    int32 xdim = 0; 
    int32 ydim = 0;
    float64 ulc[2];
    float64 lrc[2];

    r = GDgridinfo (gridid, &xdim, &ydim, ulc, lrc);
    if (r!=0) 
        throw InternalErr(__FILE__,__LINE__,"GDgridinfo doesn't return the correct values");


    float32 offset[NOFFSET]; 
    r = GDblkSOMoffset(gridid, offset, NOFFSET, "r");
    if(r!=0) 
        throw InternalErr(__FILE__,__LINE__,"GDblkSOMoffset doesn't return the correct values");

    int status = misr_init(NBLOCK, xdim, ydim, offset, ulc, lrc);
    if(status!=0) 
        throw InternalErr(__FILE__,__LINE__,"misr_init doesn't return the correct values");

    long iflg = 0;
    int (*inv_trans[MAXPROJ+1])(double, double, double*, double*);
    inv_init((long)projcode, (long)zone, (double*)params, (long)sphere, NULL, NULL, (int*)&iflg, inv_trans);
    if(iflg) 
        throw InternalErr(__FILE__,__LINE__,"inv_init doesn't return correct values");

    // Change to vector in the future. KY 2012-09-20
    double *latlon = NULL;
    double somx = 0.;
    double somy = 0.;
    double lat_r = 0.;
    double lon_r = 0.;
    int i = 0;
    int j = 0;
    int k = 0;
    int b = 0;
    int npts=0; 
    float l = 0;
    float s = 0;

    // Seems setting blockdim = 0 always, need to understand this more. KY 2012-09-20
    int blockdim=0; //20; //84.2115,84.2018, 84.192, ... //0 for all
    if(blockdim==0) //66.2263, 66.224, ....
    {
        latlon = new double[nelms]; //180*xdim*ydim]; //new double[180*xdim*ydim];
        int s1=start[0]+1, e1=s1+count[0]*step[0];
        int s2=start[1],   e2=s2+count[1]*step[1];
        int s3=start[2],   e3=s3+count[2]*step[2];
        for(i=s1; i<e1; i+=step[0]) //i = 1; i<180+1; i++)
            for(j=s2; j<e2; j+=step[1])//j=0; j<xdim; j++)
                for(k=s3; k<e3; k+=step[2])//k=0; k<ydim; k++)
                {
                    b = i;
                    l = j;
                    s = k;
                    misrinv(b, l, s, &somx, &somy); /* (b,l.l,s.s) -> (X,Y) */
                    sominv(somx, somy, &lon_r, &lat_r); /* (X,Y) -> (lat,lon) */
                    if(fieldtype==1)
                        latlon[npts] = lat_r*R2D;
                    else
                        latlon[npts] = lon_r*R2D;
                    npts++;
                }
                set_value ((dods_float64 *) latlon, nelms); //(180*xdim*ydim)); //nelms);
    } 
    if (latlon != NULL)
        delete [] latlon;
}

// The following code aims to handle large MCD Grid(GCTP_GEO projection) such as 21600*43200 lat and lon.
// These MODIS MCD files don't follow standard format for lat/lon (DDDMMMSSS);
// they simply represent lat/lon as -180.0000000 or -90.000000.
// HDF-EOS2 library won't give the correct value based on these value.
// We need to calculate the latitude and longitude values.
void
HDFEOS2ArrayGridGeoField::CalculateLargeGeoLatLon(int32 gfid, int32 gridid,  int fieldtype, float64* latlon, int *start, int *count, int *step, int nelms)
{

    int32 xdim = 0;
    int32 ydim = 0;
    float64 upleft[2];
    float64 lowright[2];
    int r = 0;
    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r!=0) {
        GDdetach(gridid);
        GDclose(gfid);
        throw InternalErr(__FILE__,__LINE__, "GDgridinfo failed");
    }

    if (0 == xdim || 0 == ydim) {
        GDdetach(gridid);
        GDclose(gfid);
        throw InternalErr(__FILE__,__LINE__, "xdim or ydim should not be zero. ");
    }

    if (upleft[0]>180.0 || upleft[0] <-180.0 ||
        upleft[1]>90.0 || upleft[1] <-90.0 ||
        lowright[0] >180.0 || lowright[0] <-180.0 ||
        lowright[1] >90.0 || lowright[1] <-90.0) {

        GDdetach(gridid);
        GDclose(gfid);
        throw InternalErr(__FILE__,__LINE__, "lat/lon corner points are out of range. ");
    }

    if (count[0] != nelms) {
        GDdetach(gridid);
        GDclose(gfid);
        throw InternalErr(__FILE__,__LINE__, "rank is not 1 ");
    }
    float lat_step = (lowright[1] - upleft[1])/ydim;
    float lon_step = (lowright[0] - upleft[0])/xdim;

    // Treat the origin of the coordinate as the center of the cell.
    // This has been the setting of MCD43 data.  KY 2012-09-10
    if (1 == fieldtype) { //Latitude
        float start_lat = upleft[1] + start[0] *lat_step + lat_step/2;
        float step_lat  = lat_step *step[0];
        for (int i = 0; i < count[0]; i++) 
            latlon[i] = start_lat +i *step_lat;
    }
    else { // Longitude
        float start_lon = upleft[0] + start[0] *lon_step + lon_step/2;
        float step_lon  = lon_step *step[0];
        for (int i = 0; i < count[0]; i++) 
            latlon[i] = start_lon +i *step_lon;
    }

}


// Calculate latitude and longitude for LAMAZ projection lat/lon products.
// GDij2ll returns infinite numbers over the north pole or the south pole.
void
HDFEOS2ArrayGridGeoField::CalculateLAMAZLatLon(int32 gridid, int fieldtype, float64* latlon, int *start, int *count, int *step, int nelms)
{
    int32 xdim = 0;
    int32 ydim = 0;
    intn r = 0;
    float64 upleft[2];
    float64 lowright[2];

    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r != 0)
        throw InternalErr(__FILE__,__LINE__,"GDgridinfo failed");

    vector<float64> tmp1;
    tmp1.resize(xdim*ydim);
    int32 tmp2[] = {0, 0};
    int32 tmp3[] = {xdim, ydim};
    int32 tmp4[] = {1, 1};
	
    CalculateLatLon (gridid, fieldtype, specialformat, &tmp1[0], tmp2, tmp3, tmp4, xdim*ydim);

    vector<float64> tmp5;
    tmp5.resize(xdim*ydim);

    for(int w=0; w < xdim*ydim; w++)
        tmp5[w] = tmp1[w];

    // If we find infinite number among lat or lon values, we use the nearest neighbor method to calculate lat or lon.
    if(ydimmajor) {
        if(fieldtype==1) {// Lat.
            for(int i=0; i<ydim; i++)
                for(int j=0; j<xdim; j++)
                    if(isundef_lat(tmp1[i*xdim+j]))
                        tmp1[i*xdim+j]=nearestNeighborLatVal(&tmp5[0], i, j, ydim, xdim);
        } else if(fieldtype==2){ // Lon.
            for(int i=0; i<ydim; i++)
                for(int j=0; j<xdim; j++)
                    if(isundef_lon(tmp1[i*xdim+j]))
                        tmp1[i*xdim+j]=nearestNeighborLonVal(&tmp5[0], i, j, ydim, xdim);
        }
    } else { // end if(ydimmajor)
        if(fieldtype==1) {
            for(int i=0; i<xdim; i++)
                for(int j=0; j<ydim; j++)
                    if(isundef_lat(tmp1[i*xdim+j]))
                        tmp1[i*xdim+j]=nearestNeighborLatVal(&tmp5[0], i, j, ydim, xdim);
            } else if(fieldtype==2) {
                for(int i=0; i<xdim; i++)
                    for(int j=0; j<ydim; j++)
                        if(isundef_lon(tmp1[i*xdim+j]))
                            tmp1[i*xdim+j]=nearestNeighborLonVal(&tmp5[0], i, j, ydim, xdim);
            }
    }

    for(int i=start[0], k=0; i<start[0]+count[0]*step[0]; i+=step[0])
        for(int j=start[1]; j<start[1]+count[1]*step[1]; j+= step[1])
            latlon[k++] = tmp1[i*ydim+j];

}
#endif

