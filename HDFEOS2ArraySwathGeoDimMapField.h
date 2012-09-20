/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath having dimension maps

//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////
// We will interpolate the latitude and longitude fields based on the dimension map parameters 
// struct dimmap_entry includes all the entries.
// We are using the linear interpolation method to interpolate the latitude and longitude.
// So far we only see float32 and float64 types of latitude and longitude. So we only
// interpolate latitude and longitude of these data types.
#ifdef USE_HDFEOS2_LIB

#ifndef HDFEOS2ARRAYSWATHGEODIMMAPFIELD_H
#define HDFEOS2ARRAYSWATHGEODIMMAPFIELD_H

#include "Array.h"
#include "HDFCFUtil.h"
#include "HdfEosDef.h"

using namespace libdap;

class HDFEOS2ArraySwathGeoDimMapField:public Array
{
  public:
  HDFEOS2ArraySwathGeoDimMapField (int dtype, int rank, const std::string & filename, const std::string & swathname, const std::string & fieldname, const std::vector < struct dimmap_entry >&dimmaps, const string & n = "", BaseType * v = 0):
	Array (n, v),
		dtype (dtype),
		rank (rank),
		filename (filename),
		swathname (swathname), fieldname (fieldname), dimmaps (dimmaps) {
	}
	virtual ~ HDFEOS2ArraySwathGeoDimMapField ()
	{
	}
	int format_constraint (int *cor, int *step, int *edg);


	// Obtain latitude and longitude
	template < class T > int GetLatLon (int32, const std::string &,
										std::vector < struct dimmap_entry >&,
										std::vector < T > &, int32 *,
										int32 *);

	// The internal routine to do the interpolation
	template < class T >
		int _expand_dimmap_field (std::vector < T > *pvals, int32 rank,
								  int32 dimsa[], int dimindex, int32 ddimsize,
								  int32 offset, int32 inc);

	// routine to ensure the subsetted lat/lon to be returned.
	template < class T > bool LatLonSubset (T * outlatlon, int majordim,
											  int minordim, T * latlon,
											  int32 * offset, int32 * count,
											  int32 * step);
	// routine to ensure the subsetted 1D lat/lon to be returned.
	template < class T > bool LatLon1DSubset (T * outlatlon, int majordim,
											  T * latlon,
											  int32 * offset, int32 * count,
											  int32 * step);
	// routine to ensure the subsetted 2D lat/lon to be returned.
	template < class T > bool LatLon2DSubset (T * outlatlon, int majordim,
											  int minordim, T * latlon,
											  int32 * offset, int32 * count,
											  int32 * step);



	BaseType *ptr_duplicate ()
	{
		return new HDFEOS2ArraySwathGeoDimMapField (*this);
	}

	virtual bool read ();

  private:
	int dtype, rank;
	std::string filename, swathname, fieldname;
	std::vector < struct dimmap_entry >dimmaps;
};

#endif
#endif
