/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the real field values.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  Eunsoo Seo
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_REALFIELD_H
#define HDFEOS2ARRAY_REALFIELD_H

#include "Array.h"

#include "HDFCFUtil.h"
#include "HdfEosDef.h"


#include "HDFEOS2EnumType.h"

using namespace libdap;

class HDFEOS2Array_RealField:public Array
{
    public:
    HDFEOS2Array_RealField (int rank, const std::string & filename, const std::string & gridname, const std::string & swathname, const std::string & fieldname, SOType sotype, const string & n = "", BaseType * v = 0):
        Array (n, v),
        rank (rank),
        filename (filename),
        gridname (gridname), 
        swathname (swathname), 
        fieldname (fieldname),
        sotype(sotype) {
        }
        virtual ~ HDFEOS2Array_RealField ()
        {
        }

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int format_constraint (int *cor, int *step, int *edg);

        BaseType *ptr_duplicate ()
        {
            return new HDFEOS2Array_RealField (*this);
        }

        // Read the data.
        virtual bool read ();

    private:

        // Field array rank
        int rank;

        // HDF-EOS2 file name
        std::string filename;

        // HDF-EOS2 grid name
        std::string gridname;

        // HDF-EOS2 swath name
        std::string swathname; 

        // HDF-EOS2 field name
        std::string fieldname;

        // MODIS scale and offset type
        // Some MODIS files don't use the CF linear equation y = scale * x + offset,
        // the scaletype distinguishs products following different scale and offset rules. 
        SOType sotype;

        int write_dap_data_scale_comp(int32 gfid, int32 gridid, int nelms, vector<int32> &offset32,vector<int32> &count32,vector<int32> &step32);
        int write_dap_data_disable_scale_comp(int32 gfid, int32 gridid, int nelms, int32 *offset32,int32*count32,int32*step32);
};


#endif
#endif
