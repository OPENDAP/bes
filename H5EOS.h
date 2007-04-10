////////////////////////////////////////////////////////////////////////////////
/// This file is part of h5_dap_handler, A C++ implementation of the DAP handler
/// for HDF5 data.
///
/// This file contains functions that parses NASA EOS StructMetadata and generates
/// the necessary data for OPeNDAP.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2007 HDF Group
///
/// All rights reserved.
//////////////////////////////////////////////////////////////////////////////////

#ifndef _H5EOS_H
#define _H5EOS_H

#include <hdf5.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Array.h"
#include "hdfeos.tab.h"

using namespace std;

class H5EOS {
  
private:

  bool                 valid;

  
  hid_t                hid_hdfeos_information;

  map<string, int>     dimension_map;     
  map<string, string>  full_data_path_to_dimension_list_map;
  
  vector<string>       dimensions;  
  vector<string>       full_data_paths;



  bool has_group(hid_t id, const char* name);
  bool has_dataset(hid_t id, const char* name);
  
public:
  
  H5EOS();
  virtual ~H5EOS();
  
  dods_float32* xdimbuf;
  dods_float32* ydimbuf;
  dods_float32* ncandidatebuf;
  dods_float32** dimension_data;
  
  float point_lower;
  float point_upper;
  float point_left;
  float point_right;

  
  void   add_data_path(const string full_path);
  void   add_dimension_list(const string full_path, const string dimension_list);
  void   add_dimension_map(const string dimension_name, int dimension);  
  bool   check_eos(hid_t id);
  int    get_dimension_data_location(const string name);
  int    get_dimension_size(const string name);
  void   get_dimensions(const string name, vector<string>& tokens);
  string get_grid_name(const string full_path);
  bool   set_dimension_array();
  bool   is_grid(const string name);  
  bool   is_valid();
  void   print(); // For debugging

  
};
#endif
