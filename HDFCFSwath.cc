/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// Copyright (c) 2009 The HDF Group
//
/////////////////////////////////////////////////////////////////////////////

#include <cstdlib>

using namespace std;
// #define DODS_DEBUG
#include "HDFCFSwath.h"


HDFCFSwath::HDFCFSwath()
{
    _swath = false;
    eos_to_cf_map_swath["MissingValue"] = "missing_value";
    eos_to_cf_map_swath["Units"] = "units";
    eos_to_cf_map_swath["nTime"] = "lat";
    eos_to_cf_map_swath["nXtrack"] = "lon";
    eos_to_cf_map_swath["Longitude"] = "lon";
    eos_to_cf_map_swath["Latitude"] = "lat";

    eos_to_cf_map_swath["Offset"] = "add_offset";
    eos_to_cf_map_swath["ScaleFactor"] = "scale_factor";
    eos_to_cf_map_swath["ValidRange"] = "valid_range";
    eos_to_cf_map_swath["Title"] = "title";

    cf_to_eos_map_swath["lon"] = "nXtrack";
    cf_to_eos_map_swath["lat"] = "nTime";
}

HDFCFSwath::~HDFCFSwath()
{

}

void HDFCFSwath::add_data_path_swath(string full_path)
{

    full_path = get_short_name(full_path);
    DBG(cerr << "Full path is:" << full_path << endl);
    _full_data_paths_swath.push_back(full_path);


}

void HDFCFSwath::add_dimension_map_swath(string dimension_name, int dimension)
{

    bool has_dimension = false;
    DBG(cerr
        << "add_dimension_map_swath " << dimension_name
        << " = " << dimension
        << endl);

    int i;
    for (i = 0; i < (int) _dimensions_swath.size(); i++) {
        std::string str = _dimensions_swath.at(i);
        if (str == dimension_name) {
            has_dimension = true;
            if(_dimension_map_swath[dimension_name] != dimension){
                cerr
                    << "Inconsistent dimension size " << dimension
                    << " for dimension " << dimension_name
                    << endl;
                exit(-1);
            }
            break;
        }
    }

    if (!has_dimension) {
        _dimensions_swath.push_back(dimension_name);
        _dimension_map_swath[dimension_name] = dimension;

    }
}


string  HDFCFSwath::get_CF_name_swath(const string &str)
{
#if 0
    DBG(cerr << eos_to_cf_map_swath[str] << endl);
    if (eos_to_cf_map_swath[str].size() > 0) {
        return eos_to_cf_map_swath[str];
    } else {
        return str;
    }
#endif
    map<string,string>::iterator pos = eos_to_cf_map_swath.find(str);
    if (pos != eos_to_cf_map_swath.end())
        return pos->second.c_str();
    else
        return str.c_str();
}

string HDFCFSwath::get_EOS_name_swath(const string &str)
{
#if 0
    DBG(cerr << cf_to_eos_map_swath[str] << endl);
    if (cf_to_eos_map_swath[str].size() > 0) {
        return cf_to_eos_map_swath[str];
    } else {
        return str;
    }
#endif
    map<string,string>::iterator pos = cf_to_eos_map_swath.find(str);
    if (pos != cf_to_eos_map_swath.end())
        return pos->second.c_str();
    else
        return str.c_str();
}


string HDFCFSwath::get_short_name(string varname)
{
    int pos = varname.find_last_of('/', varname.length() - 1);
    return varname.substr(pos + 1);
}

bool HDFCFSwath::is_swath()
{
    return _swath;
}

bool HDFCFSwath::is_swath(string varname)
{
    int i;

    DBG(cerr << ">is_swath() " << varname << endl);
    for (i = 0; i < (int) _full_data_paths_swath.size(); i++) {
        std::string str = _full_data_paths_swath.at(i);
        if (str == varname) {
            DBG(cerr << "=is_swath() " << str << endl);
            return true;
        }
    }
    return false;

}

void HDFCFSwath::reset()
{
    _swath = false;

    if(!_dimension_map_swath.empty()){
        _dimension_map_swath.clear();
    }

    if(!_dimensions_swath.empty()){
        _dimensions_swath.clear();
    }
    if(!_full_data_paths_swath.empty()){
        _full_data_paths_swath.clear();
    }



}

void HDFCFSwath::set_swath(bool flag)
{
    _swath = flag;
}


