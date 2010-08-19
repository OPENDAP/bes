/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the latitude and longitude of  the HDF-EOS2 Grid 
// There are two typical cases: 
// read the lat/lon from the file and calculate lat/lon using the EOS2 library.
// Several variations are also handled:
// 1. For geographic and Cylinderic Equal Area projections, condense 2-D to 1-D.
// 2. For some files, the longitude is within 0-360 range instead of -180 - 180 range.
// We need to convert 0-360 to -180-180.
// 3. Some files have fillvalues in the lat. and lon. for the geographic projection.
// 4. Several MODIS files don't have the correct parameters inside StructMetadata.
// We can obtain the starting point, the step and replace the fill value.
//  Authors:   MuQun Yang <myang6@hdfgroup.org> Choonghwan Lee
// Copyright (c) 2009 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifndef HDFEOS2ARRAY_GRIDGEOFIELD_H
#define HDFEOS2ARRAY_GRIDGEOFIELD_H

#include "Array.h"
#include "HDFEOS2.h"
using namespace libdap;

class HDFEOS2ArrayGridGeoField:public Array
{
  public:
  HDFEOS2ArrayGridGeoField (int rank, int fieldtype, bool llflag, bool ydimmajor, bool condenseddim, bool speciallon, int specialformat, const std::string & filename, const std::string & gridname, const std::string & fieldname, const string & n = "", BaseType * v = 0):
	Array (n, v),
		rank (rank),
		fieldtype (fieldtype),
		llflag (llflag),
		ydimmajor (ydimmajor),
		condenseddim (condenseddim),
		speciallon (speciallon),
		specialformat (specialformat),
		filename (filename), gridname (gridname), fieldname (fieldname) {
	}
	virtual ~ HDFEOS2ArrayGridGeoField ()
	{
	}
	int format_constraint (int *cor, int *step, int *edg);

	BaseType *ptr_duplicate ()
	{
		return new HDFEOS2ArrayGridGeoField (*this);
	}

	// Calculate Lat and Lon based on HDF-EOS2 library.
	void CalculateLatLon (int32 gfid, int fieldtype, int specialformat,
						  float64 * outlatlon, int32 * offset, int32 * count,
						  int32 * step, int nelms);

	// Calculate Special Latitude and Longitude
	void CalculateSpeLatLon (int32 gfid, int fieldtype, float64 * outlatlon,
							 int32 * offset, int32 * count, int32 * step,
							 int nelms);

	// Subsetting the latitude and longitude.
	void LatLon2DSubset (float64 * outlatlon, int ydim, int xdim,
						 float64 * latlon, int32 * offset, int32 * count,
						 int32 * step);

	// Corrected Latitude and longitude when the lat/lon has fill value case.
	template < class T > bool CorLatLon (T * latlon, int fieldtype, int elms,
										 int fv);

	// Converted longitude from 0-360 to -180-180.
	template < class T > void CorSpeLon (T * lon, int xdim);

	// Lat and Lon for GEO and CEA projections need to be condensed from 2-D to 1-D.
	// This function does this.
	void getCorrectSubset (int *offset, int *count, int *step,
						   int32 * offset32, int32 * count32, int32 * step32,
						   bool condenseddim, bool ydimmajor, int fieldtype,
						   int rank);

	// Helper function to handle the case that lat. and lon. contain fill value.
	template < class T > int findfirstfv (T * array, int start, int end,
										  int fillvalue);

	virtual bool read ();

  private:
	std::string filename, gridname, fieldname;
	int rank, fieldtype, specialformat;
	bool llflag, ydimmajor, condenseddim, speciallon;
};
#endif
