/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the Vdata fields from NASA HDF4 data products.
// Each Vdata will be decomposed into individual Vdata fields.
// Each field will be mapped to A DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDFSPARRAY_VDFIELD_H
#define HDFSPARRAY_VDFIELD_H

#include "hdf.h"
#include "mfhdf.h"

#include <libdap/Array.h>

class HDFSPArray_VDField:public libdap::Array
{
    public:
        HDFSPArray_VDField (int vdrank, const std::string& filename, const int fileid, int32 objref, int32 dtype, int32 fieldorder, const std::string & fieldname, const std::string & n = "", libdap::BaseType * v = nullptr):
            Array (n, v),
            rank (vdrank),
            filename(filename),
            fileid (fileid),
            vdref (objref),
            dtype (dtype), 
            fdorder (fieldorder), 
            fdname (fieldname) {
        }
        ~ HDFSPArray_VDField () override = default;
       
        // Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
        // Return the number of elements to read.  
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDFSPArray_VDField (*this);
        }

        bool read () override;

    private:

        // Field array rank
        int rank;

        std::string filename;

        // file id
        int32 fileid;

        // Vdata reference number
        int32 vdref;

        // data type
        int32 dtype;

        // field order
        int32 fdorder;

        // field name
        std::string fdname;
};


#endif
