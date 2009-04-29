/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// Copyright (c) 2009 The HDF Group
//
/////////////////////////////////////////////////////////////////////////////

using namespace std;

#include "HDFCFSwath.h"


HDFCFSwath::HDFCFSwath()
{
  _swath = false;
  eos_to_cf_map["MissingValue"] = "missing_value";
  eos_to_cf_map["Units"] = "units";
  eos_to_cf_map["nTime"] = "lat"; 
  eos_to_cf_map["nXtrack"] = "lon"; 
  eos_to_cf_map["Longitude"] = "lon"; 
  eos_to_cf_map["Latitude"] = "lat";
 
  eos_to_cf_map["Offset"] = "add_offset";
  eos_to_cf_map["ScaleFactor"] = "scale_factor";
  eos_to_cf_map["ValidRange"] = "valid_range";
  eos_to_cf_map["Title"] = "title";

  cf_to_eos_map["lon"] = "nXtrack";
  cf_to_eos_map["lat"] = "nTime";
}

HDFCFSwath::~HDFCFSwath()
{
  
}

void HDFCFSwath::add_data_path_swath(string full_path)
{

  DBG(cerr << "Full path is:" << full_path << endl);
  _full_data_paths_swath.push_back(full_path);
  
  
}

void HDFCFSwath::add_dimension_map_swath(string dimension_name, int dimension)
{

  bool has_dimension = false;
  DBG(cerr << "add_dimension_map_swath " << dimension_name << " = " << dimension << endl);
  
  int i;
  for (i = 0; i < (int) _dimensions_swath.size(); i++) {
    std::string str = _dimensions_swath.at(i);
    if (str == dimension_name) {
      has_dimension = true;
      if(_dimension_map_swath[dimension_name] != dimension){
	cerr << "Inconsistent dimension size " << dimension << " for dimension " << dimension_name;
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


const char* HDFCFSwath::get_CF_name(char *eos_name)
{
  string str(eos_name);

  DBG(cerr << ">get_CF_name:" << str << endl);
  DBG(cerr << eos_to_cf_map[str] << endl);
  if(is_swath()){
    if(str.find("/Longitude") != string::npos){
      return "lon";
    }
    if(str.find("/Latitude") != string::npos){
      return "lat";
    }    
  }
  if (eos_to_cf_map[str].size() > 0) {
    return eos_to_cf_map[str].c_str();
  } else {
    // <hyokyung 2009.04.23. 15:36:37>    
    // HACK: The following appends weird character under Hyrax.
    // return str.c_str(); 
    return (const char*) eos_name;
  }
}

string HDFCFSwath::get_EOS_name(string str)
{
  DBG(cerr << cf_to_eos_map[str] << endl);
  if (cf_to_eos_map[str].size() > 0) {
    return cf_to_eos_map[str];
  } else {
    return str;
  }
}



bool HDFCFSwath::is_swath()
{
   return _swath;
}

// See is_grid(string varname)
bool HDFCFSwath::is_swath(string varname)
{
  int i;
  for (i = 0; i < (int) _full_data_paths_swath.size(); i++) {
    std::string str = _full_data_paths_swath.at(i);
    if (str == varname) {
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


