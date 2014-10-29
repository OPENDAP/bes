/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath without using dimension maps

//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
// For the swath without using dimension maps, for most cases, the retrieving of latitude and 
// longitude is the same as retrieving other fields. Some MODIS latitude and longitude need
// to be arranged specially.
#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_SWATHGEOFIELD_H
#define HDFEOS2ARRAY_SWATHGEOFIELD_H

#include "Array.h"
#include "HDFCFUtil.h"
#include "HdfEosDef.h"

using namespace libdap;

class HDFEOS2ArraySwathGeoField:public Array
{
    public:
        HDFEOS2ArraySwathGeoField (int rank, const std::string & filename, const int swathfd, const std::string & swathname, const std::string & fieldname, const string & n = "", BaseType * v = 0):
            Array (n, v),
            rank (rank),
            filename(filename),
            swathfd (swathfd), 
            swathname (swathname), 
            fieldname (fieldname) {
        }
        virtual ~ HDFEOS2ArraySwathGeoField ()
        {
        }

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int format_constraint (int *cor, int *step, int *edg);

        BaseType *ptr_duplicate ()
        {
            return new HDFEOS2ArraySwathGeoField (*this);
        }

        // Read the data 
        virtual bool read ();

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
