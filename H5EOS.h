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

/// A class for handling NASA EOS data.
/// This class contains functions that parse NASA EOS StructMetadata
/// and prepares the necessary (grid) data for OPeNDAP.
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2007 HDF Group
///
/// All rights reserved.
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
#ifdef SHORT_PATH
  string get_short_name(string a_name);
#endif
  
public:
  
  H5EOS();
  virtual ~H5EOS();
  
  dods_float32** dimension_data;
  
  float point_lower;
  float point_upper;
  float point_left;
  float point_right;

  
  void   add_data_path(string full_path);
  void   add_dimension_list(string full_path, string dimension_list);
  void   add_dimension_map(string dimension_name, int dimension);  
  bool   check_eos(hid_t id);
  int    get_dimension_data_location(string name);
  int    get_dimension_size(string name);
  void   get_dimensions(string name, vector<string>& tokens);
  string get_grid_name(string full_path);
  bool   is_grid(string name);  
  bool   is_valid();
  void   print(); // For debugging
  bool   set_dimension_array();
  
};
#endif
