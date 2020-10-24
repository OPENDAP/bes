/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitudes and longitudes of an HDF-EOS2 Swath using multiple dimension maps

//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2020 The HDF Group
/////////////////////////////////////////////////////////////////////////////
// For the swath using the multiple dimension maps,  
// Latitude/longitude will be interpolated accordingly.
#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_SWATHGEOMULTIDIMMAPFIELD_H
#define HDFEOS2ARRAY_SWATHGEOMULTIDIMMAPFIELD_H

#include "Array.h"
#include "HDFCFUtil.h"
#include "HdfEosDef.h"


class HDFEOS2ArraySwathGeoMultiDimMapField:public libdap::Array
{
    public:
        HDFEOS2ArraySwathGeoMultiDimMapField (int rank, const std::string & filename, 
                                         const int swathfd, const std::string & swathname, 
                                         const std::string & fieldname, 
                                         const int dim0size,const int dim0offset,const int dim0inc,
                                         const int dim1size,const int dim1offset,const int dim1inc,
                                         const std::string & n = "", 
                                         libdap::BaseType * v = 0):
            libdap::Array (n, v),
            rank (rank),
            filename(filename),
            swathfd (swathfd), 
            swathname (swathname), 
            fieldname (fieldname),
            dim0size(dim0size),
            dim0offset(dim0offset),
            dim0inc(dim0inc),
            dim1size(dim1size),
            dim1offset(dim1offset),
            dim1inc(dim1inc){
        }
        virtual ~ HDFEOS2ArraySwathGeoMultiDimMapField ()
        {
        }

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate ()
        {
            return new HDFEOS2ArraySwathGeoMultiDimMapField (*this);
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

        int dim0size;
        int dim0offset;
        int dim0inc;
        int dim1size;
        int dim1offset;
        int dim1inc;
        // Obtain Field value
        template < class T > int GetFieldValue (int32, const std::string &,const std::vector <int>&,const std::vector <int>&,const std::vector<int>&, std::vector < T > &, std::vector<int32>&); 

        // The internal routine to do the interpolation
        template < class T > int _expand_dimmap_field (std::vector < T > *pvals, int32 rank, int32 dimsa[], int dimindex, int32 ddimsize, int32 offset, int32 inc);

        // subsetting routine to ensure the subsetted 2D field to be returned.
        template < class T > bool Field2DSubset (T * outlatlon, const int majordim, const int minordim, T * latlon, int32 * offset, int32 * count, int32 * step);
 


};


#endif
#endif
