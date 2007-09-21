#ifndef _H5EOS_H
#define _H5EOS_H

#include <hdf5.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <Array.h>
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
#define BUFFER_MAX 655360 // 10 * (2^16); 10 metadata files can be merged per metadata type.

class H5EOS {
  
private:

  bool                 valid;
#ifdef CF  
  bool                 shared_dimension;
#endif
  
  hid_t                hid_hdfeos_information;

  map<string, int>     dimension_map;     
  map<string, string>  full_data_path_to_dimension_list_map;
#ifdef CF
  map<string, string>  eos_to_grads_map;
#endif
  
  vector<string>       dimensions;  
  vector<string>       full_data_paths;



  bool has_group(hid_t id, const char* name);
  bool has_dataset(hid_t id, const char* name);
#ifdef SHORT_PATH
  string get_short_name(string a_name);
#endif
  
public:
  
  bool bmetadata_Struct;  
  char metadata_Struct[BUFFER_MAX];
  
#ifdef NASA_EOS_META
  // Flag for merged metadata. Once it's set, don't prcoess for other attribute variables that start with the same name.
  bool bmetadata_Archived;
  bool bmetadata_Core;
  bool bmetadata_core;
  bool bmetadata_product;
  bool bmetadata_subset;
  
  // Holder for the merged metadata information.
  char metadata_Archived[BUFFER_MAX];
  char metadata_Core[BUFFER_MAX];
  char metadata_core[BUFFER_MAX];
  char metadata_product[BUFFER_MAX];
  char metadata_subset[BUFFER_MAX];
#endif
  
  dods_float32** dimension_data;
  
  float point_lower;
  float point_upper;
  float point_left;
  float point_right;

  H5EOS();
  virtual ~H5EOS();
  
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
  bool   set_metadata(hid_t id, char* metadata_name, char* metadata_buffer);
  
#ifdef CF
  bool  is_shared_dimension_set();
  void  set_shared_dimension();
  const char*  set_grads_attribute(char* attr_name);
#endif  
  
};
#endif
