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
#include "BESH4MCache.h"


class HDFSPArray_RealField:public libdap::Array
{
    public:
        HDFSPArray_RealField (int32 rank, const std::string& filename, const int sdfd, int32 fieldref, int32 dtype, SPType & sptype, const std::string & fieldname, const std::vector<int32> & h4_dimsizes, const std::string & n = "", libdap::BaseType * v = 0):
            Array (n, v),
            rank (rank),
            filename(filename),
            sdfd (sdfd),
            fieldref (fieldref), 
            dtype (dtype), 
            sptype (sptype), 
            fieldname (fieldname),
            dimsizes(h4_dimsizes) {
        }
        virtual ~ HDFSPArray_RealField ()
        {
        }
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate ()
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
        std::string fieldname;
        std::vector<int32>dimsizes;
        //void write_data_to_cache(int32,const std::string&,short);
        void write_data_to_cache(int32,const std::string&,short,const std::vector<char>&, int);
        bool obtain_cached_data(BESH4Cache*,const std::string&, int,std::vector<int>&, std::vector<int>&,size_t,short);
        template<typename T> int subset(const T input[],int,std::vector<int32>&,std::vector<int>&,std::vector<int>&,std::vector<int>&,std::vector<T>*,std::vector<int32>&,int);
#if 0
int subset(
    const T input[],
    int rank,
    vector<int> & dim,
    vector<int> & start,
    vector<int> & stride,
    vector<int> & edge,
    std::vector<T> *poutput,
    vector<int>& pos,
    int index);
#endif
};


#endif
