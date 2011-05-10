// This file is part of hdf5_handler - a HDF5 file handler for the OPeNDAP
// data server.

// Authors: 
// Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// Muqun Yang <myang6@hdfgroup.org> 

// Copyright (c) 2011 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  
#ifndef _HE5CFZA_H
#define _HE5CFZA_H

#include "config_hdf5.h"
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <InternalErr.h>
#include <debug.h>

using namespace std;
using namespace libdap;
/// A class for holding HDF-EOS5 zonal average information.
/// This is really a temporary solution. The handler needs to be reorganized to
/// have a clear design and robust support for handling HDF-EOS5 data. 
/// The current code is mainly the clone of the "HDF-EOS5 za" since zonal average 
/// is very similar to za. KY-2011-5-3 
///
/// This class contains functions that generate CF-convention compliant output
/// for HDF-EOS5 Zonal Average.
/// By default, hdf5 handler cannot generate the DAP ouput that OPeNDAP
/// visualization clients can display due to the discrepancy between the 
/// HDF-EOS5 model and the CF-1.x convention model. Most visualization
/// clients require an output that follows CF-convention in order to display
/// data directly on a map. 
/// 
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
class HE5CFZa {
  
private:
    bool _za;                // Is it za?
    string   _za_time_dimensions;   // The lon dimension names
    string   _za_lat_dimensions;   // The lat dimension names
    string   _za_lev_dimensions;   // The lev dimension names
    string   _za_time_variable;     // The name of lon variable
    string   _za_lat_variable;     // The name of lat variable
    string   _za_lev_variable;     // The name of lev variable

    map < string, int > _za_dimension_size;
    map < string, string > _za_variable_dimensions;
    vector < string >   _za_dimension_list;
    vector < string >   _za_variable_list;

  
public:

    int za_lat;
    int za_lon;
    int za_time;
    int za_lev;

    HE5CFZa();
    virtual ~HE5CFZa();

    /// Checks if the current HDF5 file is a valid HDF-EOS5 Swath file.
    ///
    /// \return true if it has a set of correct meta data files.
    /// \return false otherwise  
    bool get_za() const;


    /// Returns 1 if \a varname matches the dimension names that 
    /// correspond to lat/lev/time, Return 2 if \a variable matches
    /// the dimension names that correspond to lat/lev. For other cases,
    /// return 0. We don't support now. KY-2011-5-3
    int get_za_coordinate_dimension_match(string varname);

    /// Get the vector of all dimensions used in this HDF-EOS5 file.
    ///
    /// \param[in] tokens a vector to be fetched 
    void  get_za_dimension_list(vector < string > &tokens);

    /// Get the size information from the \a name dimension.
    ///
    /// \param name like nTimes
    /// \return the size of dimension in integer
    int  get_za_dimension_size(string name);

    
    /// Checks if the argument string is a HDF-EOS5 ZA dataset.
    ///
    /// \return true if it is a Swath dataset.
    /// \return false otherwise  
    bool get_za_variable(string varname);

    /// Get dimension names of a za variable.
    ///
    /// Checks if this class parsed the \a name as za.
    /// Retrieve the dimension list from the \a name za
    /// and tokenize the list into the string vector.
    /// \param[in] name a grid variable name
    /// \param[out] tokens a vector of ',' delimited dimension names
    void get_za_variable_dimensions(string name, vector < string > &tokens);


    /// Clears all internal map variables and flags.
    void set();

    /// Sets the flag if HDF5 file is a valid NASA EOS ZA file.
    void set_za(bool flag);

    /// Remembers the dimension size of each dimension for a Swath variable.
    /// 
    /// It builds a map vector of dimension name and its size. It also checks
    /// for consistency among dimensions used in za variables. For example,
    /// if a za variable s1 has dimensions of  x=100 and y=200 and
    /// another za variable s2 has dimensions of x=50 and y=100,
    /// this function throws an exception.
    ///    
    /// \param[in] dimension_name a name of dimension in a za variable.
    /// \param[in] dimension a size of \a dimension_name dimension
    void set_za_dimension_size(string dimension_name, int dimension);

    /// Remembers the full path of a Swath dataset variable.
    /// 
    /// It pushes the StructMetadata-parsed full path variable name
    /// into the vector for further processing.
    /// \param[in] full_path a full path information of a variable in metadata.
    void set_za_variable_list(string full_path);

    /// Remebers the dimension names associated with a variable.
    ///
    /// It pushes the EOS-metadata-parsed dimension names list into
    /// map. The map keeps track of the association between variable
    /// and its dimension names for CF-compliant DDS/DAS generation.
    /// \param[in] full_path a full path information of a variable in metadata.
    /// \param[in] dimension_list a list of dimensions in metadata.
    void set_za_variable_dimensions(string full_path,
                                       string dimension_list);
    
};

#endif
