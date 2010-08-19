/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath without using dimension maps

//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////
// For the swath without using dimension maps, for most cases, the retrieving of latitude and 
// longitude is the same as retrieving other fields. Some MODIS latitude and longitude need
// to be arranged specially.
#ifndef HDFEOS2ARRAY_SWATHGEOFIELD_H
#define HDFEOS2ARRAY_SWATHGEOFIELD_H

#include "Array.h"
using namespace libdap;

class HDFEOS2ArraySwathGeoField:public Array
{
  public:
  HDFEOS2ArraySwathGeoField (int rank, const std::string & filename, const std::string & swathname, const std::string & fieldname, const string & n = "", BaseType * v = 0):
	Array (n, v),
		rank (rank),
		filename (filename), swathname (swathname), fieldname (fieldname) {
	}
	virtual ~ HDFEOS2ArraySwathGeoField ()
	{
	}
	int format_constraint (int *cor, int *step, int *edg);

	BaseType *ptr_duplicate ()
	{
		return new HDFEOS2ArraySwathGeoField (*this);
	}

	virtual bool read ();

  private:
	std::string filename, swathname, fieldname;
	int rank;
};


#endif
