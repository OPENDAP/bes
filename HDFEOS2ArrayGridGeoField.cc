/////////////////////////////////////////////////////////////////////////////
// retrieves the latitude and longitude of  the HDF-EOS2 Grid
//  Authors:   MuQun Yang <myang6@hdfgroup.org> 
// Copyright (c) 2009 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFEOS2ArrayGridGeoField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "HDFEOS2.h"
#include "InternalErr.h"
#include <BESDebug.h>
#include "HDFEOS2Util.h"
#define SIGNED_BYTE_TO_INT32 1

bool
HDFEOS2ArrayGridGeoField::read ()
{
    // Currently we don't support 3-D lat/lon case.
    if (rank < 1 || rank > 2) {
	throw InternalErr (__FILE__, __LINE__,
			   "The rank of geo field is greater than 2, currently we don't support 3-D lat/lon cases.");
    }

    int newrank;

    if (condenseddim)
	newrank = 1;
    else
	newrank = rank;

    // Replace these with vector<int>. jhrg 
    int *offset = new int[newrank];
    int *count = new int[newrank];
    int *step = new int[newrank];

    int nelms;

    // Obtain the number of the subsetted elements
    try {
	nelms = format_constraint (offset, step, count);
    }
    catch (...) {
	delete[]offset;
	delete[]step;
	delete[]count;
	throw;
    }

    // vector<int32>
    int32 *offset32 = new int32[rank];
    int32 *count32 = new int32[rank];
    int32 *step32 = new int32[rank];

    // Obtain offset32 with the correct rank, the rank of lat/lon of
    // GEO and CEA projections in the file may be 2 instead of 1.
    getCorrectSubset (offset, count, step, offset32, count32, step32,
		      condenseddim, ydimmajor, fieldtype, rank);

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
    openfunc = GDopen;
    closefunc = GDclose;
    attachfunc = GDattach;
    detachfunc = GDdetach;
    fieldinfofunc = GDfieldinfo;
    readfieldfunc = GDreadfield;
    datasetname = gridname;

    int32 gfid, gridid;

    // Obtain the grid id
    gfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);

    if (gfid < 0) {
	HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
			       step);
	ostringstream eherr;

	eherr << "File " << filename.c_str () << " cannot be open.";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Attach the grid id; make the grid valid.
    gridid = attachfunc (gfid, const_cast < char *>(datasetname.c_str ()));

    if (gridid < 0) {
	HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
			       step);
	// Can use std::string here. jhrg
	ostringstream eherr;

	eherr << "Grid " << datasetname.c_str () << " cannot be attached.";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // The following case handles when the lat/lon is not provided.
    if (llflag == false) {		// We have to calculate the lat/lon
	float64 *latlon = new float64[nelms];

	// This fuction will calculate the lat/lon,subset and reduce
	// the dimension properly.
	if (specialformat == 3)	// Have to provide latitude and longitude by ourselves
	    CalculateSpeLatLon (gridid, fieldtype, latlon, offset32, count32,
				step32, nelms);
	else
	    CalculateLatLon (gridid, fieldtype, specialformat, latlon,
			     offset32, count32, step32, nelms);
	if (speciallon && fieldtype == 2) {
	    CorSpeLon (latlon, nelms);
	}
	set_value ((dods_float64 *) latlon, nelms);
	delete[]latlon;
	return false;
    }

    int32 tmp_rank, tmp_dims[rank];
    char tmp_dimlist[1024];
    int32 type;
    intn r;

    // Obtain field info.
    r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
		       &tmp_rank, tmp_dims, &type, tmp_dimlist);

    if (r != 0) {
	HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
			       step);
	ostringstream eherr;

	eherr << "Field " << fieldname.
	    c_str () << " information cannot be obtained.";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    void *val;
    void *fillvalue;

    // Retrieve dimensions and X-Y coordinates of corners
    int32 xdim;
    int32 ydim;
    float64 upleft[2];
    float64 lowright[2];

    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r != 0) {
	HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
			       step);
	ostringstream eherr;

	eherr << "Grid " << datasetname.
	    c_str () << " information cannot be obtained.";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    // Retrieve all GCTP projection information
    int32 projcode;
    int32 zone;
    int32 sphere;
    float64 params[16];

    r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
    //         assert(r == 0);
    if (r != 0) {
	HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
			       step);
	ostringstream eherr;

	eherr << "Grid " << datasetname.
	    c_str () << " projection info. cannot be obtained.";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    if (projcode != GCTP_GEO) {	// Just retrieve the data like other fields
	// We have to loop through all datatype and read the lat/lon out.
	switch (type) {
	  case DFNT_INT8:
	      {
		  val = new int8[nelms];
		  r = readfieldfunc (gridid,
				     const_cast < char *>(fieldname.c_str ()),
				     offset32, step32, count32, val);
		  if (r != 0) {
		      HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
					     count, step);
		      ostringstream eherr;

		      eherr << "field " << fieldname.
			  c_str () << "cannot be read.";
		      throw InternalErr (__FILE__, __LINE__, eherr.str ());
		  }

		  // DAP2 requires the map of SIGNED_BYTE to INT32 if
		  // SIGNED_BYTE_TO_INT32 is defined.
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
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }
	    set_value ((dods_byte *) val, nelms);
	    break;

	  case DFNT_INT16:
	    val = new int16[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }

	    set_value ((dods_int16 *) val, nelms);
	    break;
	  case DFNT_UINT16:
	    val = new uint16[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }

	    set_value ((dods_uint16 *) val, nelms);
	    break;
	  case DFNT_INT32:
	    val = new int32[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }

	    set_value ((dods_int32 *) val, nelms);
	    break;
	  case DFNT_UINT32:
	    val = new uint32[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }
	    set_value ((dods_uint32 *) val, nelms);
	    break;
	  case DFNT_FLOAT32:
	    val = new float32[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }

	    set_value ((dods_float32 *) val, nelms);
	    break;
	  case DFNT_FLOAT64:
	    val = new float64[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }

	    set_value ((dods_float64 *) val, nelms);
	    break;
	  default:
	    HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
				   step);
	    InternalErr (__FILE__, __LINE__, "unsupported data type.");

	}
    }
    else {	// Only handle special cases for the Geographic Projection 
	// We find that lat/lon of the geographic projection in some
	// files include fill values. So we recalculate lat/lon based
	// on starting value,step values and number of steps.
	switch (type) {
	  case DFNT_INT8:
	      {
		  val = new int8[nelms];
		  r = readfieldfunc (gridid,
				     const_cast < char *>(fieldname.c_str ()),
				     offset32, step32, count32, val);
		  if (r != 0) {
		      HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
					     count, step);
		      ostringstream eherr;

		      eherr << "field " << fieldname.
			  c_str () << "cannot be read.";
		      throw InternalErr (__FILE__, __LINE__, eherr.str ());
		  }

		  fillvalue = new int8;

		  r = GDgetfillvalue (gridid,
				      const_cast < char *>(fieldname.c_str ()),
				      fillvalue);
		  if (r == 0) {
		      int ifillvalue =
			  static_cast < int >(*((int8 *) fillvalue));
		      bool correctedfill =
			  CorLatLon ((int8 *) val, fieldtype, nelms,
				     ifillvalue);
		      if (!correctedfill) {
			  HDFEOS2Util::ClearMem (offset32, count32, step32,
						 offset, count, step);
			  ostringstream eherr;

			  eherr << "geo-field " << fieldname.
			      c_str () << " fill values cannot be replaced";
			  throw InternalErr (__FILE__, __LINE__, eherr.str ());
		      }
		  }
		  if (speciallon && fieldtype == 2)
		      CorSpeLon ((int8 *) val, nelms);

		  delete (int8 *) fillvalue;

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
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }

	    fillvalue = new uint8;

	    r = GDgetfillvalue (gridid,
				const_cast < char *>(fieldname.c_str ()),
				fillvalue);
	    if (r == 0) {
		int ifillvalue = static_cast < int >(*((uint8 *) fillvalue));
		bool correctedfill =
		    CorLatLon ((uint8 *) val, fieldtype, nelms, ifillvalue);
		if (!correctedfill) {
		    HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
					   count, step);
		    ostringstream eherr;

		    eherr << "geo-field " << fieldname.
			c_str () << " fill values cannot be replaced";
		    throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}
	    }
	    if (speciallon && fieldtype == 2)
		CorSpeLon ((uint8 *) val, nelms);
	    set_value ((dods_byte *) val, nelms);
	    delete[](uint8 *) val;
	    delete (uint8 *) fillvalue;
	    break;

	  case DFNT_INT16:
	    val = new int16[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }


	    fillvalue = new int16;

	    r = GDgetfillvalue (gridid,
				const_cast < char *>(fieldname.c_str ()),
				fillvalue);
	    if (r == 0) {
		int ifillvalue = static_cast < int >(*((int16 *) fillvalue));
		bool correctedfill =
		    CorLatLon ((int16 *) val, fieldtype, nelms, ifillvalue);
		if (!correctedfill) {
		    HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
					   count, step);
		    ostringstream eherr;

		    eherr << "geo-field " << fieldname.
			c_str () << " fill values cannot be replaced";
		    throw InternalErr (__FILE__, __LINE__, eherr.str ());

		}
	    }
	    if (speciallon && fieldtype == 2)
		CorSpeLon ((int16 *) val, nelms);

	    set_value ((dods_int16 *) val, nelms);
	    delete[](int16 *) val;
	    delete (int16 *) fillvalue;
	    break;
	  case DFNT_UINT16:
	    val = new uint16[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }


	    fillvalue = new uint16;

	    r = GDgetfillvalue (gridid,
				const_cast < char *>(fieldname.c_str ()),
				fillvalue);
	    if (r == 0) {
		int ifillvalue = static_cast < int >(*((uint16 *) fillvalue));
		bool correctedfill =
		    CorLatLon ((uint16 *) val, fieldtype, nelms, ifillvalue);
		if (!correctedfill) {
		    HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
					   count, step);
		    ostringstream eherr;

		    eherr << "geo-field " << fieldname.
			c_str () << " fill values cannot be replaced";
		    throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}
	    }

	    if (speciallon && fieldtype == 2)
		CorSpeLon ((uint16 *) val, nelms);

	    set_value ((dods_uint16 *) val, nelms);
	    delete[](uint16 *) val;
	    delete (uint16 *) fillvalue;

	    break;
	  case DFNT_INT32:
	    val = new int32[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }

	    fillvalue = new int32;

	    r = GDgetfillvalue (gridid,
				const_cast < char *>(fieldname.c_str ()),
				fillvalue);
	    if (r == 0) {
		int ifillvalue = static_cast < int >(*((int32 *) fillvalue));
		bool correctedfill =
		    CorLatLon ((int32 *) val, fieldtype, nelms, ifillvalue);
		if (!correctedfill) {
		    HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
					   count, step);
		    ostringstream eherr;

		    eherr << "geo-field " << fieldname.
			c_str () << " fill values cannot be replaced";
		    throw InternalErr (__FILE__, __LINE__, eherr.str ());

		}
	    }
	    if (speciallon && fieldtype == 2)
		CorSpeLon ((int32 *) val, nelms);

	    set_value ((dods_int32 *) val, nelms);
	    delete[](int32 *) val;
	    delete (int32 *) fillvalue;

	    break;
	  case DFNT_UINT32:
	    val = new uint32[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }

	    fillvalue = new uint32;

	    r = GDgetfillvalue (gridid,
				const_cast < char *>(fieldname.c_str ()),
				fillvalue);
	    if (r == 0) {
		int ifillvalue = static_cast < int >(*((uint32 *) fillvalue));
		bool correctedfill =
		    CorLatLon ((uint32 *) val, fieldtype, nelms, ifillvalue);
		if (!correctedfill) {
		    HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
					   count, step);
		    ostringstream eherr;

		    eherr << "geo-field " << fieldname.
			c_str () << " fill values cannot be replaced";
		    throw InternalErr (__FILE__, __LINE__, eherr.str ());

		}
	    }
	    if (speciallon && fieldtype == 2)
		CorSpeLon ((uint32 *) val, nelms);

	    set_value ((dods_uint32 *) val, nelms);
	    delete[](uint32 *) val;
	    delete (uint32 *) fillvalue;

	    break;
	  case DFNT_FLOAT32:
	    val = new float32[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }

	    fillvalue = new float32;

	    r = GDgetfillvalue (gridid,
				const_cast < char *>(fieldname.c_str ()),
				fillvalue);
	    if (r == 0) {
		int ifillvalue =
		    static_cast < int >(*((float32 *) fillvalue));
		bool correctedfill =
		    CorLatLon ((float32 *) val, fieldtype, nelms, ifillvalue);
		if (!correctedfill) {
		    HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
					   count, step);
		    ostringstream eherr;

		    eherr << "geo-field " << fieldname.
			c_str () << " fill values cannot be replaced";
		    throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

	    }
	    if (speciallon && fieldtype == 2)
		CorSpeLon ((float32 *) val, nelms);

	    set_value ((dods_float32 *) val, nelms);
	    delete[](float32 *) val;
	    delete (float32 *) fillvalue;

	    break;
	  case DFNT_FLOAT64:
	    val = new float64[nelms];
	    r = readfieldfunc (gridid,
			       const_cast < char *>(fieldname.c_str ()),
			       offset32, step32, count32, val);
	    if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
				       count, step);
		ostringstream eherr;

		eherr << "field " << fieldname.c_str () << "cannot be read.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	    }


	    fillvalue = new float64;
	    r = GDgetfillvalue (gridid,
				const_cast < char *>(fieldname.c_str ()),
				fillvalue);
	    if (r == 0) {
		int ifillvalue =
		    static_cast < int >(*((float64 *) fillvalue));
		bool correctedfill =
		    CorLatLon ((float64 *) val, fieldtype, nelms, ifillvalue);
		if (!correctedfill) {
		    HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
					   count, step);
		    ostringstream eherr;

		    eherr << "geo-field " << fieldname.
			c_str () << " fill values cannot be replaced";
		    throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}
	    }
	    if (speciallon && fieldtype == 2)
		CorSpeLon ((float64 *) val, nelms);

	    set_value ((dods_float64 *) val, nelms);
	    delete[](float64 *) val;
	    delete (float64 *) fillvalue;

	    break;
	  default:
	    HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
				   step);
	    InternalErr (__FILE__, __LINE__, "unsupported data type.");
	}

    }

    r = detachfunc (gridid);
    if (r != 0) {
	HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
			       step);
	ostringstream eherr;

	eherr << "Grid " << datasetname.c_str () << " cannot be detached.";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    r = closefunc (gfid);
    if (r != 0) {
	HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
			       step);
	ostringstream eherr;

	eherr << "Grid " << filename.c_str () << " cannot be closed.";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    delete[]offset;
    delete[]count;
    delete[]step;

    delete[]offset32;
    delete[]count32;
    delete[]step32;

    return false;
}

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
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
	count[id] = ((stop - start) / stride) + 1;	// count of elements
	nels *= count[id];		// total number of values for variable

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

void
HDFEOS2ArrayGridGeoField::CalculateLatLon (int32 gridid, int fieldtype,
					   int specialformat,
					   float64 * outlatlon,
					   int32 * offset, int32 * count,
					   int32 * step, int nelms)
{

    // Retrieve dimensions and X-Y coordinates of corners 
    int32 xdim;
    int32 ydim;
    int r;
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
	upleft[0] = -180000000.0;
	upleft[1] = 90000000.0;
	lowright[0] = 180000000.0;
	lowright[1] = -90000000.0;
    }

    // Retrieve all GCTP projection information 
    int32 projcode;
    int32 zone;
    int32 sphere;
    float64 params[16];

    r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
    if (r != 0) {
	ostringstream eherr;

	eherr << "cannot obtain grid projection information";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Retrieve pixel registration information 
    int32 pixreg;

    r = GDpixreginfo (gridid, &pixreg);
    if (r != 0) {
	ostringstream eherr;

	eherr << "cannot obtain grid pixel registration info.";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    //Retrieve grid pixel origin 
    int32 origin;

    r = GDorigininfo (gridid, &origin);
    if (r != 0) {
	ostringstream eherr;

	eherr << "cannot obtain grid origin info.";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    int32 *rows = new int32[xdim * ydim];
    int32 *cols = new int32[xdim * ydim];
    float64 *lon = new float64[xdim * ydim];
    float64 *lat = new float64[xdim * ydim];

    int i, j, k;

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
		 xdim * ydim, rows, cols, lon, lat, pixreg, origin);
    if (r != 0) {
	ostringstream eherr;

	eherr << "cannot calculate grid latitude and longitude";
	throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

#if 0
    for (int iii = 0; iii < xdim; iii++)
	for (int jjj = 0; jjj < ydim; jjj++)
	    cerr << "lat " << iii << " " << jjj << " " << lat[iii * ydim +
							      jjj] << endl;
#endif

    // 2-D Lat/Lon, need to decompose the data for subsetting.
    if (nelms == (xdim * ydim)) {	// no subsetting return all, for the performance reason.
	if (fieldtype == 1)
	    memcpy (outlatlon, lat, xdim * ydim * sizeof (double));
	else
	    memcpy (outlatlon, lon, xdim * ydim * sizeof (double));
    }
    else {						// Messy subsetting case, needs to know the major dimension
	if (ydimmajor) {
	    if (fieldtype == 1)	// Lat 
		LatLon2DSubset (outlatlon, ydim, xdim, lat, offset, count,
				step);
	    else				// Lon
		LatLon2DSubset (outlatlon, ydim, xdim, lon, offset, count,
				step);
	}
	else {
	    if (fieldtype == 1)	// Lat
		LatLon2DSubset (outlatlon, xdim, ydim, lat, offset, count,
				step);
	    else				// Lon
		LatLon2DSubset (outlatlon, xdim, ydim, lon, offset, count,
				step);
	}
    }
    delete[]rows;
    delete[]cols;
    delete[]lon;
    delete[]lat;
}


void
HDFEOS2ArrayGridGeoField::LatLon2DSubset (float64 * outlatlon, int majordim,
					  int minordim, float64 * latlon,
					  int32 * offset, int32 * count,
					  int32 * step)
{


    //          float64 templatlon[majordim][minordim];
    float64 (*templatlonptr)[majordim][minordim] =
	(typeof templatlonptr) latlon;
    int i, j, k;

#if 0
    for (int jjj = 0; jjj < majordim; jjj++)
	for (int kkk = 0; kkk < minordim; kkk++)
	    cerr << "templatlon " << jjj << " " << kkk << " " <<
		templatlon[i][j] << endl;
#endif
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
	// Added casts to (T) for the assignments (compiler warnings).
	// jhrg 3/25/11
	if ((float) (latlon[elms - 1]) < -180.0)
	    latlon[elms - 1] = (T)-180.0;
	if ((float) (latlon[elms - 1]) > 360.0)
	    latlon[elms - 1] = (T)360.0;
    }
    return true;
}

template < class T > void
HDFEOS2ArrayGridGeoField::CorSpeLon (T * lon, int xdim)
{
    int i;
    float64 accuracy = 1e-3;	// in case there is a lon value = 180.0 in the middle, make the error to be less than 1e-3.
    float64 temp = 0;
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

	if (ydimmajor && fieldtype == 1) {	// YDim major, User: Lat[YDim], File: Lat[YDim][XDim]
	    offset32[0] = (int32) offset[0];
	    count32[0] = (int32) count[0];
	    step32[0] = (int32) step[0];
	}
	else if (ydimmajor && fieldtype == 2) {	// YDim major, User: Lon[XDim],File: Lon[YDim][XDim]
	    offset32[1] = (int32) offset[0];
	    count32[1] = (int32) count[0];
	    step32[1] = (int32) step[0];
	}
	else if (!ydimmajor && fieldtype == 1) {	// XDim major, User: Lat[YDim], File: Lat[XDim][YDim]
	    offset32[1] = (int32) offset[0];
	    count32[1] = (int32) count[0];
	    step32[1] = (int32) step[0];
	}
	else if (!ydimmajor && fieldtype == 2) {	// XDim major, User: Lon[XDim], File: Lon[XDim][YDim]
	    offset32[0] = (int32) offset[0];
	    count32[0] = (int32) count[0];
	    step32[0] = (int32) step[0];
	}

	else {					// errors
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

void
HDFEOS2ArrayGridGeoField::CalculateSpeLatLon (int32 gridid, int fieldtype,
					      float64 * outlatlon,
					      int32 * offset32,
					      int32 * count32, int32 * step32,
					      int nelms)
{

    // Retrieve dimensions and X-Y coordinates of corners 
    int32 xdim;
    int32 ydim;
    int r;
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

    if (fieldtype == 1) {
	double latstep = 180.0 / ydim;

	for (int i = 0; i < (int) (count32[0]); i++)
	    outlatlon[i] = 90.0 - latstep * (offset32[0] + i * step32[0]);
    }
    else {						// Longitude should use count32[1] etc. 
	double lonstep = 360.0 / xdim;

	for (int i = 0; i < (int) (count32[1]); i++)
	    outlatlon[i] = -180.0 + lonstep * (offset32[1] + i * step32[1]);
    }
}
