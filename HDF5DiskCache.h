////////////////////////////////////////////////////////////////////////////
//// This file includes header files of the cache handling routines for the HDF4 handler.
//// The skeleton of the code is adapted from BESDapResponseCache.cc under bes/dap 
////  Authors:   MuQun Yang <myang6@hdfgroup.org>  
//// Copyright (c) 2017 The HDF Group
///////////////////////////////////////////////////////////////////////////////
//
#ifndef _h5_disk_cache_h
#define _h5_disk_cache_h

#include <unistd.h>
#include <string>
#include <vector>
#include "BESFileLockingCache.h"

class HDF5DiskCache: public BESFileLockingCache
{
private: 
    static HDF5DiskCache *d_instance;
    static void delete_instance() { delete d_instance; d_instance = 0; }

    HDF5DiskCache(const unsigned long long,const std::string&, const std::string&);

public:
    static const std::string PATH_KEY;
    static const std::string SIZE_KEY;
    static const std::string PREFIX_KEY;
    virtual ~HDF5DiskCache() {}

    static long getCacheSizeFromConfig(const long cache_size);
    static std::string getCachePrefixFromConfig(const std::string&);
    static std::string getCacheDirFromConfig(const std::string&);

    bool is_valid(const std::string & cache_file_name, const int expected_file_size);
    static HDF5DiskCache *get_instance(const long, const std::string&, const std::string&);
    bool get_data_from_cache(const std::string &cache_file_name, const int expected_file_size,int &fd);
    bool write_cached_data(const std::string &cache_file_name,const int expected_file_size,const std::vector<double> &val);
    bool write_cached_data2(const std::string &cache_file_name,const int expected_file_size,const void *buf);
};

#endif

