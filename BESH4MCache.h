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
    static BESH4Cache *d_instance;
    BESH4Cache();

public:

    static const string PATH_KEY;
    static const string PREFIX_KEY;
    static const string SIZE_KEY;
    virtual ~BESH4Cache() {}


    static unsigned long getCacheSizeFromConfig();
    static string getCachePrefixFromConfig();
    static string getCacheDirFromConfig();


    bool is_valid(const string & cache_file_name, const int expected_file_size);
    static BESH4Cache *get_instance();
    bool  get_data_from_cache(const string &cache_file_name, const int expected_file_size,int &fd);
    bool write_cached_data(const string &cache_file_name,const int expected_file_size,const std::vector<double> &val);
    static void delete_instance();
    //void dummy_test_func();
    //string get_cache_file_name_h4(const string &src, bool mangle = false);


};

#endif

