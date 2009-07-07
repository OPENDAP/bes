//////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2009 The HDF Group
// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
//////////////////////////////////////////////////////////////////////////////
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

/// A class for generating CF compliant Swath output.
///
/// This class contains functions that generate CF-1.x compliant Swath output 
/// from HDF-EOS2 files. The main role is to remember what variables are
/// swath variables and to generate CF-1.x compliant attributes. Unlike
/// HDF-EOS2 Grid, Swath mapping is simple and straightforward. A Swath 
/// variable does not require a special 1-D map data array
/// generation and they can be mapped to DAP array directly as long as
/// 2-D lat / lon dimension names are used and 2-D lat / lon map variables 
/// exist in the HDF-EOS2 Swath file.
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
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
    /// Remember the data full path of a Swath variable including the name.
    /// 
    /// It pushes the EOS-metadata-parsed full path variable name
    /// into the vector for future processing.
    /// \param[in] full_path a full path information of a variable in metadata.
    void        add_data_path_swath(string full_path);

    /// Remember the dimension size associated with the dimension name
    ///
    /// It pushes the EOS-metadata-parsed dimension size into
    /// a map. The map keeps track of the association between variable
    /// and its dimension size for later Swath generation.
    /// \param[in] dimension_name a name of dimension in metadata
    /// \param[in] dimension a dimension size specified in metadata
    void        add_dimension_map_swath(string dimension_name, int dimension);

    /// Get the CF standard attribute and dimension names.
    ///
    /// HDF-EOS2 Swath files use a wide variety of dimension and attribute
    /// names that are significantly different from the CF standard.
    /// This function corrects such deviation for
    /// a given \a eos_name. Although this function works for some HDF-EOS2 
    /// Swath files, it is desired to have a separate XML configuration file to
    /// cover all possibilities in HDF-EOS2 Swath files.
    ///
    /// \return a pointer to a CF-compliant character string
    /// \see HDFEOS::get_CF_name()    
    /// \see get_EOS_name_swath()
    string      get_CF_name_swath(string eos_name);

    /// Get the EOS attribute and dimension names.
    ///
    /// This function is a reverse mapping of get_CF_name_swath() function.
    /// Give \a cf_name, it translates back to the name used in an HDF-EOS2
    /// Swath file.
    ///
    /// \return a string used in HDF-EOS2 Swath file
    /// \see HDFEOS::get_CF_name()    
    /// \see get_CF_name_swath()
    string      get_EOS_name_swath(string cf_name);
    
    /// Check if the current HDF5 file is a valid NASA HDF-EOS2 Swath file.
    ///
    /// \return true if it has a set of correct meta data files.
    /// \return false otherwise
    //  \see HDFEOS::is_grid()
    bool        is_swath();
    /// Check if the argument string is a NASA HDF-EOS2 Swath dataset.
    ///
    /// \return true if it is a Swath dataset.
    /// \return false otherwise  
    bool        is_swath(string varname);
  
    /// Resets all variables and frees memory.
    void        reset();

    /// Sets the flag if HDF5 file is a valid NASA HDF-EOS2 Swath file.
    void        set_swath(bool flag);

  
};

#endif // #ifndef _HDFCFSwath_H
