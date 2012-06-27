/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath with dimension map
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

// Currently the handling of swath data fields with dimension maps is the same as other data fields(HDFEOS2Array_RealField.cc etc)
// The reason to keep it in separate is, in theory, that data fields with dimension map may need special handlings.
// So we will leave it here for this release(2010-8), it may be removed in the future. HDFEOS2Array_RealField.cc may be used. 
#ifndef HDFEOS2ARRAYSWATHDIMMAPFIELD_H
#define HDFEOS2ARRAYSWATHDIMMAPFIELD_H

#include "Array.h"
using namespace libdap;

#include "HDFEOS2Util.h"

#include "hdfdesc.h"

class HDFEOS2ArraySwathDimMapField:public Array
{
  public:
  HDFEOS2ArraySwathDimMapField (int rank, const std::string & filename, const std::string & gridname, const std::string & swathname, const std::string & fieldname, const std::vector < struct dimmap_entry >&dimmaps, const string & n = "", BaseType * v = 0):
	Array (n, v),
		rank (rank),
		filename (filename),
		gridname (gridname),
		swathname (swathname), fieldname (fieldname), dimmaps (dimmaps) {
	}
	virtual ~ HDFEOS2ArraySwathDimMapField ()
	{
	}
	int format_constraint (int *cor, int *step, int *edg);

        // Obtain Field value
        template < class T > int GetFieldValue (int32, const std::string &,
                                                                                std::vector < struct dimmap_entry >&,
                                                                                std::vector < T > &, int32 *,
                                                                                int32 *);

        // The internal routine to do the interpolation
        template < class T >
                int _expand_dimmap_field (std::vector < T > *pvals, int32 rank,
                                                                  int32 dimsa[], int dimindex, int32 ddimsize,
                                                                  int32 offset, int32 inc);

        // 2-D subsetting routine to ensure the subsetted lat/lon to be returned.
        template < class T > bool FieldSubset (T * outlatlon, int majordim,
                                                                                          int minordim, T * latlon,
                                                                                          int32 * offset, int32 * count,
                                                                                          int32 * step);


	BaseType *ptr_duplicate ()
	{
		return new HDFEOS2ArraySwathDimMapField (*this);
	}

	virtual bool read ();

  private:
	std::string filename, gridname, swathname, fieldname;
	std::vector < struct dimmap_entry >dimmaps;
	int rank;
};


#endif
