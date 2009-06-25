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
#ifndef _HDFEOS_H
#define _HDFEOS_H

// 655360 = 10 * (2^16); 10 metadata files can be merged per metadata type.
// In HDF-EOS2 files, each [Struct|Core]Metadata.x can be up to 65536
// characters long.
#define BUFFER_MAX 655360

#define SHORT_PATH
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <Array.h>
#include "hdfeos2.tab.hh"

// This macro is used to generate a CF-1.x convention compliant DAP output
// that OPeNDAP visualization clients can display Grids and Swath.
//
// If USE_HDFEOS2 macro is not defined, the CF macro will parse the
// structMetadata in HDF-EOS2 file to generate CF-1.x compliant DAP output.
#ifdef CF  
#include "HDFCFSwath.h"
#endif

// If this macro is enabled, parsing structMetadata will be suppressed
// and the HDF-EOS2 library will be used to generate CF-1.x compliant
// DAP output.
#ifdef USE_HDFEOS2
#include "HDFEOS2.h"
#endif

using namespace std;
using namespace libdap;

/// A class for handling NASA HDF-EOS2 data.
/// This class contains functions that parse NASA EOS StructMetadata
/// and prepares the necessary (grid) data for OPeNDAP.
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2008-2009 The HDF Group
///
/// All rights reserved.
#ifdef CF
class HDFEOS:public HDFCFSwath {
#else
class HDFEOS {
#endif
  
private:
  bool _parsed;
  bool _valid;
#ifdef CF
  bool _shared_dimension;
#endif
  bool _is_swath;
  bool _is_grid;
  bool _is_orthogonal;
  bool _is_ydimmajor;

  int xdimsize;
  int ydimsize;

  string path_name;
  
  map < string, int > dimension_map;
  map < string, string > full_data_path_to_dimension_list_map;
#ifdef CF
  map < string, string > eos_to_cf_map;
  map < string, string > cf_to_eos_map;
#endif

  vector < string > dimensions;
#ifdef USE_HDFEOS2
  vector < int > types;   // <hyokyung 2008.12.17. 13:15:18>
#endif

#ifdef SHORT_PATH
  string get_short_name(string a_name);
#endif
  
#ifdef USE_HDFEOS2
  bool set_dimension_array_hdfeos2();
#endif

public:
  /// a flag to indicate that origin information in structMetadata specifies
  /// that a map is upside down.
  bool borigin_upper; 
  /// a flag to indicate if structMetdata dataset is processed or not.
  bool bmetadata_Struct;
  /// a buffer for the merged structMetadata dataset
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
#ifdef USE_HDFEOS2
  void handle_grid(const HDFEOS2::GridDataset *grid);
  void handle_swath(const HDFEOS2::SwathDataset *swath);  
  void handle_datafield(const HDFEOS2::Field *field);
  void handle_attributes(const HDFEOS2::Attribute *attribute);
#endif  
public:
#ifdef USE_HDFEOS2
  auto_ptr<HDFEOS2::File> eos2;
#endif
  void reset();
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
  
  
  vector < string > full_data_paths;  
  /// Pointers for map data arrays
  dods_float32 **dimension_data;
  
  HDFEOS();
  virtual ~ HDFEOS();
#ifdef USE_HDFEOS2
  /// Remembers a dataset variable.
  void add_variable(string var_name);
  
  /// Remembers the type <hyokyung 2008.12.17. 13:14:26>
  void add_type(int type);
#endif
  /// Remembers the data full path of a variable including the name.
  /// 
  /// It pushes the EOS-metadata-parsed full path variable name
  /// into the vector for future processing.
  /// \param[in] full_path a full path information of a variable in metadata.
  void add_data_path(string full_path);

  /// Remebers the dimension list associated with a variable.
  ///
  /// It pushes the EOS-metadata-parsed dimension names list into
  /// map. The map keeps track of the association between variable
  /// and its dimension names for later Grid generation.
  /// \param[in] full_path a full path information of a variable in metadata.
  /// \param[in]  dimension_list a list of dimensions in metadata.
  void add_dimension_list(string full_path, string dimension_list);

  /// Remembers the dimension size associated with the dimension name
  ///
  /// It pushes the EOS-metadata-parsed dimension size into
  /// a map. The map keeps track of the association between variable
  /// and its dimension size for later Grid generation.
  /// \param[in] dimension_name a name of dimension in metadata
  /// \param[in] dimension a dimension size specified in metadata
  void add_dimension_map(string dimension_name, int dimension);


  /// Get the index of dimension data from the dimension map
  ///
  /// \param name like XDim, YDim and ZDim
  /// \return integer index
  /// \see HDF5ArrayEOS HDF5GridEOS
  int get_dimension_data_location(string name);

  /// Get the size informationfrom the \a name dimension.
  ///
  /// \param name like XDim, YDim and ZDim
  /// \return the size of dimension in integer
  int get_dimension_size(string name);

  /// Get dimension names of a grid variable.
  ///
  /// Checks if this class parsed the \a name as grid.
  /// Retrieve the dimension list from the \a name grid
  /// and tokenize the list into the string vector.
  /// \param[in] name a grid variable name
  /// \param[out] tokens a vector of ',' delimited dimension names
  void get_dimensions(string name, vector < string > &tokens);

  /// Get the only grid name part from the full name
  ///
  /// For example, it returns '/HDFEOS/GRIDS/CloudFractionAndPressure/' from
  /// /HDFEOS/GRIDS/CloudFractionAndPressure/Data_Fields/CloudPressurePrecision.
  /// \param[in] full_path a full path name of a Grid variable
  /// \see h5dds.cc
  string get_grid_name(string full_path);

  /// Check if this class has already parsed the \a name as grid.
  /// 
  /// \param name a name of variable
  /// \return true if it is parsed as Grid.
  /// \return false otherwise
  /// \see h5dds.cc
  bool is_grid(string name);

#ifdef USE_HDFEOS2
  /// Check if the current HDF4 file is a valid NASA EOS Grid file.
  /// 
  /// \param name a name of variable
  /// \return true if it is grid variable.
  /// \return false otherwise
  bool is_grid();
  
  /// Check if the current HDF4 file is a valid NASA EOS Swath file.
  ///
  /// \return true if it is a valid EOS Swath file
  /// \return false otherwise
  bool is_swath();
#endif  
  /// Check if the current HDF5 file is a valid NASA EOS file.
  ///
  /// \return true if it has a set of correct meta data files.
  /// \return false otherwise  
  bool is_valid();
#ifdef USE_HDFEOS2
  /// Check if the current HDF4 file uses a geographic projection.
  ///
  /// \return true if it uses a geographic projection
  /// \return false otherwise  
  bool is_orthogonal();
  
  /// Check if the current HDF4 file has 2-D lat/lon with a YDim major.
  ///
  /// \return true if it has 2-D lat/lon with a YDim major.
  /// \return false otherwise  
  bool is_ydimmajor();
#endif

  /// Prints out some information parsed from meta data
  ///
  /// This function is provided for debugging purporse
  /// to ensure that parsing is done properly.
  void print();
#ifdef USE_HDFEOS2
  /// Opens HDF file
  int open(char* filename);
#endif  
  /// Generates artifical dimension array.
  ///
  /// This function generates dimension arrays based on metadata information.
  /// Since there's no map data for a Grid-like array in NASA EOS data,
  /// map data should be generated based on struct metadata.
  ///
  /// \return ture if dimension arrays are generated successfully.
  /// \return false if dimension size is negative due to error in parsing.
  bool set_dimension_array();
  
  bool parse_struct_metadata(const char* str_metadata);
  
#ifdef CF
  bool is_shared_dimension_set();
  void set_shared_dimension();
  const char *get_CF_name(char *eos_name);
  string get_EOS_name(string cf_name);
  void get_all_dimensions(vector < string > &tokens);  
#endif
#ifdef USE_HDFEOS2
  /// Gets the poitner to the array of grid_name.
  int get_data_grid(string grid_name, char** val);

  /// Gets the poitner to the array of swath_name.
  int get_data_swath(string swath_name, char** val);
  
  /// Gets the data type of variable at location i.
  int get_data_type(int i);

  /// Get the XDim size of HDF-EOS grid.
  int get_xdim_size();
  
  /// Get the XDim size of HDF-EOS grid.
  int get_ydim_size();
#endif
};
#endif
