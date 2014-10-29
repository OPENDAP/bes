/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the real field values for some special NASA HDF data.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDFSPARRAY_REALFIELD_H
#define HDFSPARRAY_REALFIELD_H

#include "mfhdf.h"
#include "hdf.h"

#include "Array.h"

#include "HDFSPEnumType.h"

using namespace libdap;

class HDFSPArray_RealField:public Array
{
    public:
        HDFSPArray_RealField (int32 rank, const std::string& filename, const int sdfd, int32 fieldref, int32 dtype, SPType & sptype, const std::string & fieldname, const std::string & n = "", BaseType * v = 0):
            Array (n, v),
            rank (rank),
            filename(filename),
            sdfd (sdfd),
            fieldref (fieldref), 
            dtype (dtype), 
            sptype (sptype), 
            name (fieldname) {
        }
        virtual ~ HDFSPArray_RealField ()
        {
        }
        int format_constraint (int *cor, int *step, int *edg);

        BaseType *ptr_duplicate ()
        {
            return new HDFSPArray_RealField (*this);
        }

        virtual bool read ();

    private:
        int32 rank;
        string filename;
        int32 sdfd;
        int32 fieldref;
        int32 dtype;
        SPType sptype;
        std::string name;
};


#endif
