/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//  Authors:   MuQun Yang <myang6@hdfgroup.org> 
// Copyright (c) 2009 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDF5CFGEO_CF1D_H
#define HDF5CFGEO_CF1D_H

#include "HDF5BaseArray.h"
using namespace libdap;

class HDF5CFGeoCF1D:public HDF5BaseArray
{
    public:
        HDF5CFGeoCF1D (EOS5GridPCType eos5_proj_code, double eos2_svalue, double eos2_evalue, int eos2_dim_size, const std::string & n = "", BaseType * v = 0):
            HDF5BaseArray (n, v), proj_code(eos5_proj_code), svalue (eos2_svalue),evalue(eos2_evalue),tnumelm(eos2_dim_size) {
            }
        virtual ~ HDF5CFGeoCF1D ()
        {
        }

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        //int format_constraint (int *cor, int *step, int *edg);
        virtual void read_data_NOT_from_mem_cache(bool add_cache,void*buf);

        BaseType *ptr_duplicate ()
        {
            return new HDF5CFGeoCF1D (*this);
        }

        virtual bool read ();

    private:

        // Projection code
        EOS5GridPCType proj_code;
 
        // Start value
        double svalue;

        // End value
        double evalue;

        // Total number of elements
        int tnumelm;
};


#endif
