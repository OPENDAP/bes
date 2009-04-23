/// A class for generating CF compliant output.
///
/// This class contains functions that generate CF compliant output
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2009 The HDF Group
///
/// All rights reserved.

#ifndef _H5CF_H
#define _H5CF_H

#include "config_hdf5.h"

#include "H5ShortName.h"
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <debug.h>

using namespace std;

class H5CF: public H5ShortName {
  
private:
  
  bool _swath;
  map < string, int > _dimension_map_swath;
  vector < string >   _dimensions_swath;
  vector < string >   _full_data_paths_swath;
  
public:
  bool OMI; // for L2 OMI swath  <hyokyung 2009.04.22. 15:21:26>  
  bool shared_dimension;
  map < string, string > eos_to_cf_map;
  map < string, string > cf_to_eos_map;

  
  H5CF();
  virtual ~H5CF();
  /// Remembers the data full path of a Swath variable including the name.
  /// 
  /// It pushes the EOS-metadata-parsed full path variable name
  /// into the vector for future processing.
  /// \param[in] full_path a full path information of a variable in metadata.
  void        add_data_path_swath(string full_path);
  void        add_dimension_map_swath(string dimension_name, int dimension);  
  const char* get_CF_name(char *eos_name);
  string      get_EOS_name(string cf_name);
  bool        is_shared_dimension_set();
  /// Check if the current HDF5 file is a valid NASA EOS SWATH file.
  ///
  /// \return true if it has a set of correct meta data files.
  /// \return false otherwise  
  bool        is_swath();
  /// Check if the argument string is a NASA EOS SWATH dataset.
  ///
  /// \return true if it is a Swath dataset.
  /// \return false otherwise  
  bool        is_swath(string varname);
  void        reset();
  void        set_shared_dimension();

  /// Sets the flag if HDF5 file is a valid NASA EOS SWATH file.
  void        set_swath(bool flag);

  
};

#endif
