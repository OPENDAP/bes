/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Author:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group

#ifndef HDF5_DATA_MEM_CACHE_H_
#define HDF5_DATA_MEM_CACHE_H_


#include <DapObj.h>
#include <InternalErr.h>
#include<vector>
#include<string>

//using namespace libdap;
//using namespace std;

/**
 * bla,bla
 *
 * @todo bla,bla
 */
class HDF5DataMemCache : public libdap::DapObj {
private:
    //string varname;
    std::vector <char>  databuf;
public:
    HDF5DataMemCache() { }
#if 0
    //HDF5DataMemCache(const string &name) {varname = name; }
    //HDF5DataMemCache(const HDF5DataMemCache & h5datacache);
    //const string get_varname() {return varname;}
#endif
    size_t get_var_buf_size() {return databuf.size();}
#if 0
    //void get_var_buf(vector<char>&var_buf) { var_buf = databuf;}
#endif
    void* get_var_buf() { return &databuf[0];}
#if 0
    //void set_varname(const string& name) {varname = name; }
#endif
    void set_databuf(std::vector<char> &buf){databuf = buf;}
    virtual ~HDF5DataMemCache() { };
    virtual void dump(std::ostream &strm) const;
    
};

#endif



