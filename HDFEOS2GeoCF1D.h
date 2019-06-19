/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//  Authors:   MuQun Yang <myang6@hdfgroup.org> 
// Copyright (c) 2009 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2GEO_CF1D_H
#define HDFEOS2GEO_CF1D_H

#include "Array.h"

class HDFEOS2GeoCF1D:public libdap::Array
{
    public:
        HDFEOS2GeoCF1D (int eos2_proj_code, double eos2_svalue, double eos2_evalue, int eos2_dim_size, const std::string & n = "", libdap::BaseType * v = 0):
            libdap::Array (n, v), proj_code(eos2_proj_code), svalue (eos2_svalue),evalue(eos2_evalue),tnumelm(eos2_dim_size) {
            }
        virtual ~ HDFEOS2GeoCF1D ()
        {
        }

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate ()
        {
            return new HDFEOS2GeoCF1D (*this);
        }

        virtual bool read ();

    private:

        // Projection code
        int proj_code;
 
        // Start value
        double svalue;

        // End value
        double evalue;

        // Total number of elements
        int tnumelm;
};


#endif
#endif
