/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath without using dimension maps

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////
// For the swath without using dimension maps, for most cases, the retrieving of latitude and 
// longitude is the same as retrieving other fields. Some MODIS latitude and longitude need
// to be arranged specially.
#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_SWATHGEOFIELD_H
#define HDFEOS2ARRAY_SWATHGEOFIELD_H

#include <libdap/Array.h>
#include "HDFCFUtil.h"
#include "HdfEosDef.h"


class HDFEOS2ArraySwathGeoField:public libdap::Array
{
    public:
        HDFEOS2ArraySwathGeoField (int rank, const std::string & filename, const int swathfd, const std::string & swathname, const std::string & fieldname, const string & n = "", libdap::BaseType * v = nullptr):
            libdap::Array (n, v),
            rank (rank),
            filename(filename),
            swathfd (swathfd), 
            swathname (swathname), 
            fieldname (fieldname) {
        }
        ~ HDFEOS2ArraySwathGeoField () override = default;

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDFEOS2ArraySwathGeoField (*this);
        }

        // Read the data 
        bool read () override;

    private:

        // Field array rank
        int rank;

        // HDF-EOS2 file name 
        std::string filename;

        int swathfd;

        // HDF-EOS2 swath name
        std::string swathname;

        // HDF-EOS2 field name
        std::string fieldname;
};


#endif
#endif
