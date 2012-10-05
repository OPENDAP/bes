/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath with dimension map
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
// SOME MODIS products provide the latitude and longitude files for
// swaths using dimension map. The files are still HDF-EOS2 files.
// The file name is determined at hdfdesc.cc.
#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_SWATHGEODIMMAPEXTRAFIELD_H
#define HDFEOS2ARRAY_SWATHGEODIMMAPEXTRAFIELD_H

#include "Array.h"

#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"


using namespace libdap;

// swathname is not provided because the additional geo-location file uses 
// a different swath name.
class HDFEOS2ArraySwathGeoDimMapExtraField:public Array
{
  public:
  HDFEOS2ArraySwathGeoDimMapExtraField (int rank, const std::string & filename, const std::string & fieldname, const string & n = "", BaseType * v = 0):
	Array (n, v), rank (rank), filename (filename), fieldname (fieldname) {
	}
	virtual ~ HDFEOS2ArraySwathGeoDimMapExtraField ()
	{
	}
	int format_constraint (int *cor, int *step, int *edg);

	BaseType *ptr_duplicate ()
	{
		return new HDFEOS2ArraySwathGeoDimMapExtraField (*this);
	}

	virtual bool read ();

  private:
	int rank;
	std::string filename, fieldname;
};


#endif
#endif
