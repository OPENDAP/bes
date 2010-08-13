// This file is part of hdf5_handler - a HDF5 file handler for the OPeNDAP
// data server.

// Authors: 
// Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// Muqun Yang <myang6@hdfgroup.org> 

// Copyright (c) 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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
#ifndef _HE5CFGRID_H
#define _HE5CFGRID_H

#include "config_hdf5.h"
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

#include <math.h>
#include <dods-datatypes.h>
#include <util.h>
#include <debug.h>
#include <InternalErr.h>

using namespace std;
using namespace libdap;
/// A class for holding HDF-EOS5 grid information.
///
/// This class contains functions that generate CF-convention compliant output
/// for HDF-EOS5 Grid.
/// By default, hdf5 handler cannot generate the DAP ouput that OPeNDAP
/// visualization clients can display due to the discrepancy between the 
/// HDF-EOS5 model and the CF-1.x convention model. Most visualization
/// clients require an output that follows CF-convention in order to display
/// data directly on a map. 
/// 
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
class HE5CFGrid {
  
private:
    bool _grid;                // Is it grid with Geographic porjection?
    bool _grid_TES;            // Is it TES grid?
    int _grid_lat;
    int _grid_lon;
    int _grid_lev;
    int _grid_time;
    
    map < string, int >    _grid_dimension_size;
    map < string, string > _grid_variable_dimensions;
    vector < string >      _grid_dimension_list;
    vector < string >      _grid_variable_list;

public:
  
    HE5CFGrid();
    virtual ~HE5CFGrid();

    /// Pointers for Grid map and shared dimension data arrays
    dods_float32 **dimension_data;


	/////////////////////////////////////////////
	// Grid geoloc calculation
	/////////////////////////////////////////////
    /// The bottom coordinate value of a Grid
    float point_lower;
    /// The top coordinate value of a Grid
    float point_upper;
    /// The leftmost coordinate value of a Grid
    float point_left;
    /// The righttmost coordinate value of a Grid
    float point_right;
    /// The resolution of longitude
    float gradient_x;
    /// The resolution of latitude
    float gradient_y;

	// PixelRegistration
	enum {HE5_HDFE_CENTER, HE5_HDFE_CORNER};	
		// These are actually EOS5 consts, but we define these 
		// since we do not depend on the EOS5 lib.
	int pixelregistration;	// either _HE5_HDFE_CENTER or _HE5_HDFE_CORNER

	// GridOrigin
	enum {
		HE5_HDFE_GD_UL, HE5_HDFE_GD_UR,
		HE5_HDFE_GD_LL, HE5_HDFE_GD_LR};
	int gridorigin;	// one of HE5_HDFE_GD_UL, _UR, _LL, _LR

	// These vars show whether the vars above are already read from the StructMetadata or not.
	// These are used to check two (or more) grids have the same such vars or not.
	bool bRead_point_lower,
		 bRead_point_upper,
		 bRead_point_left,
		 bRead_point_right,
		 bRead_pixelregistration,
		 bRead_gridorigin;

	/////////////////////////////////////////////
	/////////////////////////////////////////////
	

    /// Checks if the current HDF5 file is a valid HDF-EOS5 Grid file.
    ///
    /// \return true if it has a set of correct meta data files.
    /// \return false otherwise  
    bool get_grid();


    /// Checks if the current HDF5 file is a HDF-EOS5 TES file.
    ///
    /// \return true if it is NASA AURA TES Grid file.
    /// \return false otherwise  
    bool get_grid_TES();

    /// Checks if the current HDF-EOS5 file has YDim.
    ///
    /// \return dimension size if it has lat dimension.
    /// \return 0 otherwise  
    int get_grid_lat();

    /// Checks if the current HDF-EOS5 file has XDim.
    ///
    /// \return dimensino size if it has lon dimension.
    /// \return 0 otherwise  
    int get_grid_lon();

    /// Checks if the current HDF-EOS5 file has nCandidate.
    ///
    /// \return dimension size if it has lev dimension.
    /// \return 0 otherwise  
    int get_grid_lev();

    /// Checks if the current HDF-EOS5 file has nWavel.
    ///
    /// \return dimension size if it has time dimension.
    /// \return 0 otherwise  
    int get_grid_time();
    
    /// Get the index of dimension data from the dimension map
    ///
    /// \param name like XDim, YDim and ZDim
    /// \return integer index
    /// \see HDF5ArrayEOS HDF5GridEOS
    int get_grid_dimension_data_location(string name);

    /// Get the vector of all dimensions used in this HDF-EOS5 file.
    ///
    /// \param[in] tokens a vector to be fetched 
    void  get_grid_dimension_list(vector < string > &tokens);

    /// Get the size informationfrom the \a name dimension.
    ///
    /// \param name like nTimes
    /// \return the size of dimension in integer
    int  get_grid_dimension_size(string name);

    
    /// Checks if the argument string is a HDF-EOS5 GRID dataset.
    ///
    /// \return true if it is a Grid dataset.
    /// \return false otherwise  
    bool get_grid_variable(string varname);

    /// Get dimension names of a grid variable.
    ///
    /// Checks if this class parsed the \a name as grid.
    /// Retrieve the dimension list from the \a name grid
    /// and tokenize the list into the string vector.
    /// \param[in] name a grid variable name
    /// \param[out] tokens a vector of ',' delimited dimension names
    void get_grid_variable_dimensions(string name, vector < string > &tokens);


    /// Prints out some information parsed from meta data
    ///
    /// This function is provided for debugging purporse
    /// to ensure that parsing is done properly.
    void print();

    /// Clears all internal map variables and flags.
    void set();

    /// Sets the flag if HDF5 file is a valid NASA EOS GRID file.
    void set_grid(bool flag);

    /// Sets the flag if HDF5 is a TES as specified in CoreMetadata.
    void set_grid_TES(bool flag);

    /// Generates artifical dimension array data.
    ///
    /// This function generates dimension array data based on 
    /// the StructMetadata information.
    /// Since there's no map data for a Grid-like array in NASA EOS data,
    /// map data should be generated based on StructMetadata.
    ///
    /// \return ture if dimension array data are generated successfully.
    /// \return false if dimension size is negative due to error in parsing.
    bool set_grid_dimension_data();

    /// Remembers the dimension size of each dimension for a Grid variable.
    /// 
    /// It builds a map vector of dimension name and its size. It also checks
    /// for consistency among dimensions used in grid variables. For example,
    /// if a grid variable s1 has dimensions of  x=100 and y=200 and
    /// another grid variable s2 has dimensions of x=50 and y=100,
    /// this function throws an exception.
    ///    
    /// \param[in] dimension_name a name of dimension in a grid variable.
    /// \param[in] dimension a size of \a dimension_name dimension
    void set_grid_dimension_size(string dimension_name, int dimension);

    /// Get the only grid name part from the full name
    ///
    /// For example, it returns '/HDFEOS/GRIDS/CloudFractionAndPressure/' from
    /// /HDFEOS/GRIDS/CloudFractionAndPressure/Data_Fields/CloudPressurePrecision variable.
    /// \param[in] full_path a full path name of a Grid variable
    /// \see h5dds.cc
    string get_grid_name(string full_path);

    /// Remembers the full path of a Grid dataset variable.
    /// 
    /// It pushes the StructMetadata-parsed full path variable name
    /// into the vector for further processing.
    /// \param[in] full_path a full path information of a variable in metadata.
    void set_grid_variable_list(string full_path);

    /// Remebers the dimension names associated with a variable.
    ///
    /// It pushes the EOS-metadata-parsed dimension names list into
    /// map. The map keeps track of the association between variable
    /// and its dimension names for CF-compliant DDS/DAS generation.
    /// \param[in] full_path a full path information of a variable in metadata.
    /// \param[in] dimension_list a list of dimensions in metadata.
    void set_grid_variable_dimensions(string full_path, string dimension_list);

};

#endif
