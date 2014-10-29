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
// Copyright (c) 2009-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_GRIDGEOFIELD_H
#define HDFEOS2ARRAY_GRIDGEOFIELD_H

#include <Array.h>
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"

using namespace libdap;
class HDFEOS2ArrayGridGeoField:public Array
{
    public:
        HDFEOS2ArrayGridGeoField (int rank, int fieldtype, bool llflag, bool ydimmajor, bool condenseddim, bool speciallon, int specialformat, /*short field_cache,*/const std::string &filename, const int gridfd,  const std::string & gridname, const std::string & fieldname,const string & n = "", BaseType * v = 0):
            Array (n, v),
            rank (rank),
            fieldtype (fieldtype),
            llflag (llflag),
            ydimmajor (ydimmajor),
            condenseddim (condenseddim),
            speciallon (speciallon),
            specialformat (specialformat),
            /*field_cache(field_cache),*/
            filename(filename),
            gridfd(gridfd),
            gridname (gridname), 
            fieldname (fieldname)
        {
        }
        virtual ~ HDFEOS2ArrayGridGeoField ()
        {
        }
        int format_constraint (int *cor, int *step, int *edg);

        BaseType *ptr_duplicate ()
        {
            return new HDFEOS2ArrayGridGeoField (*this);
        }

        virtual bool read ();

    private:

        // Field array rank
        int  rank;
       
        // Distinguish coordinate variables from general variables.
        // For fieldtype values:
        // 0 the field is a general field
        // 1 the field is latitude.
        // 2 the field is longtitude.    
        // 3 the field is a coordinate variable defined as level.
        // 4 the field is an inserted natural number.
        // 5 the field is time.
        int  fieldtype;
       
        // The flag to indicate if lat/lon is an existing field in the file or needs to be calculated.
        bool llflag;

        // Flag to check if this lat/lon field is YDim major(YDim,XDim). This is necessary to use GDij2ll
        bool ydimmajor;

        // Flag to check if this 2-D lat/lon can be condensed to 1-D lat/lon
        bool condenseddim;

        // Flag to check if this file's longitude needs to be handled specially.
        // Note: longitude values range from 0 to 360 for some fields. We need to map the values to -180 to 180.
        bool speciallon;

        // Latitude and longitude values of some HDF-EOS2 grids need to be handled in special ways.
        // There are four cases that we need to calculate lat/lon differently.
        // This number is used to distinguish them.      
        // 1) specialformat = 1 
        // Projection: Geographic
        // upleft and lowright coordinates don't follow EOS's DDDMMMSSS conventions.
        // Instead, they simply represent lat/lon values as -180.0 or -90.0.
        // Products: mostly MODIS MCD Grid

        // 2) specialformat = 2
        // Projection: Geographic
        // upleft and lowright coordinates don't follow EOS's DDDMMMSSS conventions.
        // Instead, their upleft or lowright are simply represented as default(-1).
        // Products: mostly TRMM CERES Grid

        // 3) specialformat = 3
        // One MOD13C2 doesn't provide project code 
        // The upperleft and lowerright coordinates are all -1
        // We have to calculate lat/lon by ourselves.
        // Since it doesn't provide the project code, we double check their information
        // and find that it covers the whole globe with 0.05 degree resolution.
        // Lat. is from 90 to -90 and Lon is from -180 to 180.

        // 4) specialformat = 4
        // Projection: Space Oblique Mercator(SOM) 
        // The lat/lon needs to be handled differently for the SOM projection
        // Products: MISR
        int  specialformat;

        //short field_cache;

        // Temp here: HDF-EOS2 file name
        std::string filename;
        
        int gridfd;

        // HDF-EOS2 grid name
        std::string gridname; 

        // HDF-EOS2 field name
        std::string fieldname;
        // Calculate Lat and Lon based on HDF-EOS2 library.
        void CalculateLatLon (int32 gridid, int fieldtype, int specialformat, float64 * outlatlon, float64* latlon_all, int32 * offset, int32 * count, int32 * step, int nelms,bool write_latlon_cache);

        // Calculate Special Latitude and Longitude.
        //One MOD13C2 file doesn't provide projection code
        // The upperleft and lowerright coordinates are all -1
        // We have to calculate lat/lon by ourselves.
        // Since it doesn't provide the project code, we double check their information
        // and find that it covers the whole globe with 0.05 degree resolution.
        // Lat. is from 90 to -90 and Lon is from -180 to 180.
        void CalculateSpeLatLon (int32 gridid, int fieldtype, float64 * outlatlon, int32 * offset, int32 * count, int32 * step, int nelms);

        // Calculate Latitude and Longtiude for the Geo-projection for very large number of elements per dimension.
        void CalculateLargeGeoLatLon(int32 gridid,  int fieldtype, float64* latlon, float64* latlon_all, int *start, int *count, int *step, int nelms,bool write_latlon_cache);
        // test for undefined values returned by longitude-latitude calculation
        bool isundef_lat(double value)
        {
            if(isinf(value)) return(true);
            if(isnan(value)) return(true);
            // GCTP_LAMAZ returns "1e+51" for values at the opposite poles
            if(value < -90.0 || value > 90.0) return(true);
                return(false);
        } // end bool isundef_lat(double value)

        bool isundef_lon(double value)
        {
            if(isinf(value)) return(true);
            if(isnan(value)) return(true);
            // GCTP_LAMAZ returns "1e+51" for values at the opposite poles
            if(value < -180.0 || value > 180.0) return(true);
                return(false);
        } // end bool isundef_lat(double value)

        // Given rol, col address in double array of dimension YDim x XDim
        // return value of nearest neighbor to (row,col) which is not undefined
        double nearestNeighborLatVal(double *array, int row, int col, int YDim, int XDim)
        {
            // test valid row, col address range
            if(row < 0 || row > YDim || col < 0 || col > XDim)
            {
                cerr << "nearestNeighborLatVal("<<row<<", "<<col<<", "<<YDim<<", "<<XDim;
                cerr <<"): index out of range"<<endl;
                return(0.0);
            }
            // address (0,0)
            if(row < YDim/2 && col < XDim/2)
            { /* search by incrementing both row and col */
                if(!isundef_lat(array[(row+1)*XDim+col])) return(array[(row+1)*XDim+col]);
                if(!isundef_lat(array[row*XDim+col+1])) return(array[row*XDim+col+1]);
                if(!isundef_lat(array[(row+1)*XDim+col+1])) return(array[(row+1)*XDim+col+1]);
                /* recurse on the diagonal */
                return(nearestNeighborLatVal(array, row+1, col+1, YDim, XDim));
            }
            if(row < YDim/2 && col > XDim/2)
            { /* search by incrementing row and decrementing col */
                if(!isundef_lat(array[(row+1)*XDim+col])) return(array[(row+1)*XDim+col]);
                if(!isundef_lat(array[row*XDim+col-1])) return(array[row*XDim+col-1]);
                if(!isundef_lat(array[(row+1)*XDim+col-1])) return(array[(row+1)*XDim+col-1]);
                /* recurse on the diagonal */
                return(nearestNeighborLatVal(array, row+1, col-1, YDim, XDim));
            }
            if(row > YDim/2 && col < XDim/2)
            { /* search by incrementing col and decrementing row */
                if(!isundef_lat(array[(row-1)*XDim+col])) return(array[(row-1)*XDim+col]);
                if(!isundef_lat(array[row*XDim+col+1])) return(array[row*XDim+col+1]);
                if(!isundef_lat(array[(row-1)*XDim+col+1])) return(array[(row-1)*XDim+col+1]);
                /* recurse on the diagonal */
                return(nearestNeighborLatVal(array, row-1, col+1, YDim, XDim));
            }
            if(row > YDim/2 && col > XDim/2)
            { /* search by decrementing both row and col */
                if(!isundef_lat(array[(row-1)*XDim+col])) return(array[(row-1)*XDim+col]);
                if(!isundef_lat(array[row*XDim+col-1])) return(array[row*XDim+col-1]);
                if(!isundef_lat(array[(row-1)*XDim+col-1])) return(array[(row-1)*XDim+col-1]);
                /* recurse on the diagonal */
                return(nearestNeighborLatVal(array, row-1, col-1, YDim, XDim));
            }
            // dummy return, turn off the compiling warning
            return 0.0;
        } // end

        double nearestNeighborLonVal(double *array, int row, int col, int YDim, int XDim)
        {
            // test valid row, col address range
            if(row < 0 || row > YDim || col < 0 || col > XDim)
            {
                cerr << "nearestNeighborLonVal("<<row<<", "<<col<<", "<<YDim<<", "<<XDim;
                cerr <<"): index out of range"<<endl;
                return(0.0);
            }
            // address (0,0)
            if(row < YDim/2 && col < XDim/2)
            { /* search by incrementing both row and col */
                if(!isundef_lon(array[(row+1)*XDim+col])) return(array[(row+1)*XDim+col]);
                if(!isundef_lon(array[row*XDim+col+1])) return(array[row*XDim+col+1]);
                if(!isundef_lon(array[(row+1)*XDim+col+1])) return(array[(row+1)*XDim+col+1]);
                /* recurse on the diagonal */
                return(nearestNeighborLonVal(array, row+1, col+1, YDim, XDim));
            }
            if(row < YDim/2 && col > XDim/2)
            { /* search by incrementing row and decrementing col */
                if(!isundef_lon(array[(row+1)*XDim+col])) return(array[(row+1)*XDim+col]);
                if(!isundef_lon(array[row*XDim+col-1])) return(array[row*XDim+col-1]);
                if(!isundef_lon(array[(row+1)*XDim+col-1])) return(array[(row+1)*XDim+col-1]);
                /* recurse on the diagonal */
                return(nearestNeighborLonVal(array, row+1, col-1, YDim, XDim));
            }
            if(row > YDim/2 && col < XDim/2)
            { /* search by incrementing col and decrementing row */
                if(!isundef_lon(array[(row-1)*XDim+col])) return(array[(row-1)*XDim+col]);
                if(!isundef_lon(array[row*XDim+col+1])) return(array[row*XDim+col+1]);
                if(!isundef_lon(array[(row-1)*XDim+col+1])) return(array[(row-1)*XDim+col+1]);
                /* recurse on the diagonal */
                return(nearestNeighborLonVal(array, row-1, col+1, YDim, XDim));
            }
            if(row > YDim/2 && col > XDim/2)
            { /* search by decrementing both row and col */
                if(!isundef_lon(array[(row-1)*XDim+col])) return(array[(row-1)*XDim+col]);
                if(!isundef_lon(array[row*XDim+col-1])) return(array[row*XDim+col-1]);
                if(!isundef_lon(array[(row-1)*XDim+col-1])) return(array[(row-1)*XDim+col-1]);
                /* recurse on the diagonal */
                return(nearestNeighborLonVal(array, row-1, col-1, YDim, XDim));
            }

            // dummy return, turn off the compiling warning
            return 0.0;
        } // end

        // Calculate Latitude and Longitude for SOM Projection.
        // since the latitude and longitude of the SOM projection are 3-D, so we need to handle this projection in a special way. 
        // Based on our current understanding, the third dimension size is always 180. 
        // This is according to the MISR Lat/lon calculation document 
        // at http://eosweb.larc.nasa.gov/PRODOCS/misr/DPS/DPS_v50_RevS.pdf
        void CalculateSOMLatLon(int32, int*, int*, int*, int,const string &, bool);

        // Calculate Latitude and Longitude for LAMAZ Projection.
        void CalculateLAMAZLatLon(int32, int, float64*, float64*,int*, int*, int*, int,bool);

        // Subsetting the latitude and longitude.
        template <class T> void LatLon2DSubset (T* outlatlon, int ydim, int xdim, T* latlon, int32 * offset, int32 * count, int32 * step);

        // Handle latitude and longitude when having fill value for geographic projection 
        //template <class T> void HandleFillLatLon(T* total_latlon, T* latlon,bool ydimmajor,
        template <class T> void HandleFillLatLon(vector<T> total_latlon, T* latlon,bool ydimmajor, int fieldtype, int32 xdim , int32 ydim, int32* offset, int32* count, int32* step, int fv);

        // Corrected Latitude and longitude when the lat/lon has fill value case.
        template < class T > bool CorLatLon (T * latlon, int fieldtype, int elms, int fv);

        // Converted longitude from 0-360 to -180-180.
        template < class T > void CorSpeLon (T * lon, int xdim);

        // Lat and Lon for GEO and CEA projections need to be condensed from 2-D to 1-D.
        // This function does this.
        void getCorrectSubset (int *offset, int *count, int *step, int32 * offset32, int32 * count32, int32 * step32, bool condenseddim, bool ydimmajor, int fieldtype, int rank);

        // Helper function to handle the case that lat. and lon. contain fill value.
        template < class T > int findfirstfv (T * array, int start, int end, int fillvalue);

};
#endif
#endif
