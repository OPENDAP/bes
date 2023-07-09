/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf5 data handler for the OPeNDAP data server.
// Currently it provides the missing latitude,longitude fields for the HDF-EOS5 files for the default option.
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDF5MISSLLARRAY_H
#define _HDF5MISSLLARRAY_H

// STL includes
#include <string>
#include <vector>

#include <libdap/Array.h>
#include "h5dmr.h"


class HDF5MissLLArray:public libdap::Array {
    public:
        HDF5MissLLArray(bool var_is_lat,int h5_rank,const eos5_grid_info_t & eg_info, const std::string & n="",  libdap::BaseType * v = nullptr):
        libdap::Array(n,v),
        is_lat(var_is_lat),
        rank(h5_rank),
        g_info(eg_info)
        {
        }
        
        ~ HDF5MissLLArray() override = default;
        libdap::BaseType *ptr_duplicate() override;
        bool read() override;

    private:
        bool is_lat;
        int rank;
        eos5_grid_info_t g_info;

        bool read_data_geo();
        void read_data_geo_lat(int64_t nelms, const std::vector<int64_t> &offset, const std::vector<int64_t> &count,
                                          const std::vector<int64_t> &step, std::vector<float> &val) ;
        void read_data_geo_lon(int64_t nelms, const std::vector<int64_t> &offset, const std::vector<int64_t> &count,
                                          const std::vector<int64_t> &step, std::vector<float> &val) ;
        bool read_data_non_geo();
        int64_t format_constraint (int64_t *offset, int64_t *step, int64_t *count);
        size_t INDEX_nD_TO_1D(const std::vector<size_t> &dims, const std::vector<size_t> &pos) const;
        template<typename T> int subset(void* input,
                                        int rank,
                                        const std::vector<size_t> & dim,
                                        int64_t start[],
                                        int64_t stride[],
                                        int64_t edge[],
                                        std::vector<T> *poutput,
                                        std::vector<size_t>& pos,
                                        int index);

};

#endif                          // _HDF5MISSLLARRAY_H

