////////////////////////////////////////////////////////////////////////////
//// This file includes header files of the cache handling routines for the HDF4 handler.
//// The skeleton of the code is adapted from BESDapResponseCache.cc under bes/dap 
////  Authors:   MuQun Yang <myang6@hdfgroup.org>  
//// Copyright (c) 2014 The HDF Group
///////////////////////////////////////////////////////////////////////////////
//
#ifndef _bes_h4m_cache_h
#define _bes_h4m_cache_h

#include <unistd.h>
#include <string>
#include <vector>
#include "BESFileLockingCache.h"

class BESH4Cache: public BESFileLockingCache
{
private: 
    static bool d_enabled;
    static BESH4Cache *d_instance;
    static void delete_instance() { delete d_instance; d_instance = 0; }

    BESH4Cache();

public:
    static const std::string PATH_KEY;
    static const std::string PREFIX_KEY;
    static const std::string SIZE_KEY;
    virtual ~BESH4Cache() {}

    static long getCacheSizeFromConfig();
    static std::string getCachePrefixFromConfig();
    static std::string getCacheDirFromConfig();

    bool is_valid(const std::string & cache_file_name, const int expected_file_size);
    static BESH4Cache *get_instance();
    bool get_data_from_cache(const std::string &cache_file_name, const int expected_file_size,int &fd);
    bool write_cached_data(const std::string &cache_file_name,const int expected_file_size,const std::vector<double> &val);
    bool write_cached_data2(const std::string &cache_file_name,const int expected_file_size,const void *buf);
};

#endif

