/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath with dimension map
//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////
// SOME MODIS products provide the latitude and longitude files for
// swaths using dimension map. The files are still HDF-EOS2 files.
// The file name is determined at hdfdesc.cc.
#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_SWATHGEODIMMAPEXTRAFIELD_H
#define HDFEOS2ARRAY_SWATHGEODIMMAPEXTRAFIELD_H

#include <libdap/Array.h>
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"


// swathname is not provided because the additional geo-location file uses 
// a different swath name.
class HDFEOS2ArraySwathGeoDimMapExtraField:public libdap::Array
{
    public:
    HDFEOS2ArraySwathGeoDimMapExtraField (int rank, const std::string & filename, const std::string & fieldname, const string & n = "", libdap::BaseType * v = nullptr):
        libdap::Array (n, v), rank (rank), filename (filename), fieldname (fieldname) {
        }
        ~ HDFEOS2ArraySwathGeoDimMapExtraField () override = default;

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDFEOS2ArraySwathGeoDimMapExtraField (*this);
        }

        // Read the data
        bool read () override;

    private:

        // Field array rank
        int rank;

        // HDF-EOS2 file name
        std::string filename;

        // HDF-EOS2 field name
        std::string fieldname;
};


#endif
#endif
