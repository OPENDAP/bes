/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Author:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group

#ifndef HDF5_DATA_MEM_CACHE_H_
#define HDF5_DATA_MEM_CACHE_H_


#include <libdap/DapObj.h>
#include <libdap/InternalErr.h>
#include<vector>
#include<string>

class HDF5DataMemCache : public libdap::DapObj {
private:
    //string varname;
    std::vector <char>  databuf;
public:
    HDF5DataMemCache() = default;
#if 0
    //HDF5DataMemCache(const string &name) {varname = name; }
    //HDF5DataMemCache(const HDF5DataMemCache & h5datacache);
    //const string get_varname() {return varname;}
#endif
    const size_t get_var_buf_size() {return databuf.size();}
#if 0
    //void get_var_buf(vector<char>&var_buf) { var_buf = databuf;}
#endif
    void* get_var_buf() { return &databuf[0];}
#if 0
    //void set_varname(const string& name) {varname = name; }
#endif
    void set_databuf(const std::vector<char> &buf){databuf = buf;}
    virtual ~HDF5DataMemCache() = default;
    void dump(std::ostream &strm) const override;
    
};

#endif



