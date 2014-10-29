/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the latitude and longitude fields for some special HDF4 data products. 
// The products include TRMML2_V6,TRMML3B_V6,CER_AVG,CER_ES4,CER_CDAY,CER_CGEO,CER_SRB,CER_SYN,CER_ZAVG,OBPGL2,OBPGL3
// To know more information about these products,check HDFSP.h.
// Each product stores lat/lon in different way, so we have to retrieve them differently. 
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifndef HDFSPARRAYGeoField_H
#define HDFSPARRAYGeoField_H

#include "Array.h"
#include "hdf.h"
#include "mfhdf.h"
#include "HDFSPEnumType.h"
using namespace libdap;

class HDFSPArrayGeoField:public Array
{
    public:
        HDFSPArrayGeoField (int32 rank, const std::string& filename, const int sdfd, int32 fieldref, int32 dtype, SPType sptype, int fieldtype, const std::string & fieldname, const std::string & n = "", BaseType * v = 0):
            Array (n, v),
            rank (rank),
            filename(filename),
            sdfd(sdfd),
            fieldref (fieldref),
            dtype (dtype),
            sptype (sptype), 
            fieldtype (fieldtype), 
            name (fieldname) {
        }
        virtual ~ HDFSPArrayGeoField ()
        {
        }

        // Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
        // Return the number of elements to read. 
        int format_constraint (int *cor, int *step, int *edg);

        BaseType *ptr_duplicate ()
        {
            return new HDFSPArrayGeoField (*this);
        }

        virtual bool read ();


    private:

        /// Field array rank
        int32 rank;

        /// File name
        std::string filename;
        int sdfd;

        /// SDS reference number
        int32 fieldref;

        /// Data type
        int32 dtype;

        /// Special HDF4 products we support(TRMML2_V6,TRMML3B_V6,OBPG etc.)
        SPType sptype;

        /// This flag will specify the fieldtype.
        /// 0 means this field is general field.
        /// 1 means this field is lat.
        /// 2 means this field is lon.
        /// 3 means this field is other dimension coordinate variable.
        /// 4 means this field is added other dimension coordinate variable with nature number.
        /// In this file, field type is either 1 or 2(lat or lon).
        int fieldtype;

        /// Field name(latitude or longitude)
        std::string name;

        // Read TRMM level 2 version 6 lat/lon
        void readtrmml3a_v6 (int32 *, int32 *, int32 *, int);

        // Read TRMM level 2 version 6 lat/lon
        void readtrmml3c_v6 (int32 *, int32 *, int32 *, int);
        // Read TRMM level 2 version 6 lat/lon
        void readtrmml2_v6 (int32 *, int32 *, int32 *, int);


        // Read OBPG level 2 lat/lon
        void readobpgl2 (int32 *, int32 *, int32 *, int);

        // Read OBPG level 3 lat/lon
        void readobpgl3 (int *, int *, int *, int);

        // Read TRMM level 3 version 6 lat/lon
        void readtrmml3b_v6 (int32 *, int32 *, int32 *, int);

        // Read TRMM level 3 version 7 lat/lon
        void readtrmml3_v7 (int32 *, int32 *, int32 *, int);


        // Read CERES SAVG and CERES ICCP_DAYLIKE lat/lon
        void readcersavgid1 (int *, int *, int *, int);

        // Read CERES SAVG and ICCP_DAYLIKE lat/lon
        void readcersavgid2 (int *, int *, int *, int);

        // Read CERES ZAVG lat/lon
        void readcerzavg (int32 *, int32 *, int32 *, int);

        // Read CERES AVG and SYN lat/lon
        void readceravgsyn (int32 *, int32 *, int32 *, int);

        // Read CERES ES4 and ICCP_GEO lat/lon
        void readceres4ig (int32 *, int32 *, int32 *, int);

        template <typename T>  void LatLon2DSubset (T* outlatlon, int ydim, int xdim, T* latlon, int32 * offset, int32 * count, int32 * step);



};


#endif
