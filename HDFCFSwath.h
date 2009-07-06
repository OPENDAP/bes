/// A class for generating CF compliant output.
///
/// This class contains functions that generate CF-1.x compliant Swath output.
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2009 The HDF Group
///
/// All rights reserved.

#ifndef _HDFCFSwath_H
#define _HDFCFSwath_H

#include "config_hdf.h"

#include <debug.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>


using namespace std;

class HDFCFSwath {
  
private:
  
    bool _swath;
    
    map < string, int > _dimension_map_swath;    
    map < string, string > eos_to_cf_map_swath;
    map < string, string > cf_to_eos_map_swath;

    string get_short_name(string varname);
    
    vector < string >   _dimensions_swath;
    vector < string >   _full_data_paths_swath;
  
public:
  
    HDFCFSwath();
    virtual ~HDFCFSwath();
    /// Remembers the data full path of a Swath variable including the name.
    /// 
    /// It pushes the EOS-metadata-parsed full path variable name
    /// into the vector for future processing.
    /// \param[in] full_path a full path information of a variable in metadata.
    void        add_data_path_swath(string full_path);

    /// Remembers
    ///
    /// 
    /// \param[in] dimension_name a 
    /// metadata.
    /// \param[in] dimension a 
    void        add_dimension_map_swath(string dimension_name, int dimension);  
    string      get_CF_name_swath(string eos_name);
    string      get_EOS_name_swath(string cf_name);
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
  
    /// Resets all variables and frees memory.
    void        reset();

    /// Sets the flag if HDF5 file is a valid NASA EOS SWATH file.
    void        set_swath(bool flag);

  
};

#endif
