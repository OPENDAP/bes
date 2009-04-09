#ifndef _H5EOS_H
#define _H5EOS_H
#define BUFFER_MAX 655360       // 10 * (2^16); 10 metadata files can be merged per metadata type.

#include <hdf5.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <Array.h>
#include "hdfeos.tab.hh"
#include "H5CF.h"

using namespace std;
using namespace libdap;

/// A class for handling NASA EOS data.
/// This class contains functions that parse NASA EOS StructMetadata
/// and prepares the necessary (grid) data for OPeNDAP.
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2007 The HDF Group
///
/// All rights reserved.
class H5EOS:public H5CF {

private:

  bool TES;
  bool valid;
  hid_t hid_hdfeos_information;

  map < string, int > dimension_map;
  map < string, string > full_data_path_to_dimension_list_map;

  vector < string > dimensions;
  vector < string > full_data_paths;

  bool has_group(hid_t id, const char *name);
  bool has_dataset(hid_t id, const char *name);
  void reset();

public:

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

  /// Pointers for map data arrays
  dods_float32 **dimension_data;
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
  
  H5EOS();
  virtual ~ H5EOS();

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

  /// Check if this file is EOS file by examining metadata
  ///
  /// \param id root group id
  /// \return 1, if it is EOS file.
  /// \return 0, if not.
  bool check_eos(hid_t id);

  /// Get the vector of all dimensions used in this HDF-EOS5 file.
  ///
  /// \param[in] tokens a vector to be fetched 
  void  get_all_dimensions(vector < string > &tokens);
  
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

  /// Check if the current HDF-EOS5 file is a TES file.
  ///
  /// \return true if it is a TES product.
  /// \return false otherwise  
  bool is_TES();

  
  /// Check if the current HDF5 file is a valid NASA EOS file.
  ///
  /// \return true if it has a set of correct meta data files.
  /// \return false otherwise  
  bool is_valid();

  /// Prints out some information parsed from meta data
  ///
  /// This function is provided for debugging purporse
  /// to ensure that parsing is done properly.
  void print();

  /// Generates artifical dimension array.
  ///
  /// This function generates dimension arrays based on metadata information.
  /// Since there's no map data for a Grid-like array in NASA EOS data,
  /// map data should be generated based on struct metadata.
  ///
  /// \return ture if dimension arrays are generated successfully.
  /// \return false if dimension size is negative due to error in parsing.
  bool set_dimension_array();

  /// Merges metafiles and Sets metdata buffer
  ///
  /// This function is needed because some metada files are split
  /// like StructMetadata.0, StructMetadata.1, ... and StructMetadata.n.
  /// We assume that there are less than 10 metadata splits for each meta data type.
  ///
  /// \param id dataset id
  /// \param metadata_name pre-defined names like "StructMetadata" or "CoreMetadata"
  /// \param metadata_buffer the buffer for merged strings.
  /// \return ture if there exists \a metadata_name string variable exists
  /// \return false otherwise.
  bool set_metadata(hid_t id, char *metadata_name,
		    char *metadata_buffer);

};
#endif
