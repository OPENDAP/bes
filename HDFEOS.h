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

// In HDF-EOS2 files, each [Struct|Core]Metadata.x can be up to 65536
// characters long and the splitted meta files need to be merged to make
// the Metadata parser work.
// 655360 = 10 * (2^16); 10 Metadata files can be merged per Metadata type.
#define BUFFER_MAX 655360

#define SHORT_PATH
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <typeinfo>

#include <Array.h>
#include "hdfeos2.tab.hh"

// This macro is used to generate a CF-1.x convention compliant DAP output
// that OPeNDAP visualization clients can display Grids and Swath.
//
// If USE_HDFEOS2_LIB macro is not defined, the CF macro will parse the
// structMetadata in HDF-EOS2 file to generate CF-1.x compliant DAP output.
#include "HDFCFSwath.h"

// If this macro is enabled, parsing structMetadata will be suppressed
// and the HDF-EOS2 library will be used to generate CF-1.x compliant
// DAP output.
#ifdef USE_HDFEOS2_LIB
#include "HDFEOS2.h"
#endif

using namespace std;
using namespace libdap;

/// A class for handling NASA HDF-EOS2 data.
/// This class contains functions that parse NASA EOS StructMetadata
/// and prepares the necessary (grid) data for OPeNDAP.
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
class HDFEOS:public HDFCFSwath {

private:
    // Private member variables
    bool _parsed;
    bool _valid;
    bool _is_swath;
    bool _is_grid;
    bool _is_orthogonal;
    bool _is_ydimmajor;

    int xdimsize;
    int ydimsize;

    string path_name;

    map < string, int > dimension_map;
    map < string, string > full_data_path_to_dimension_list_map;

    vector < string > dimensions;

    bool _shared_dimension;
    map < string, string > eos_to_cf_map;
    map < string, string > cf_to_eos_map;
    // These are 'special' maps to deal with hdfeos swaths; only really needed
    // when building with the hdfeos2 capability, but it's easier to not make
    // their inclusion conditional based on a compile-time switch. jhrg 
    map < string, string > eos_to_cf_map_is_swath;
    map < string, string > cf_to_eos_map_is_swath;

#ifdef USE_HDFEOS2_LIB
    vector < int > types;

   // Need to add comments. This is a key function to retrieve
   // HDF-EOS2 information. ky 2009/11/19
    bool set_dimension_array_hdfeos2();
#endif

    ///   If you look at the hdfclass/sds.cc file, the current hdf4 handler
    /// retrieves dataset based on variable name. Thus, suppressing path
    /// information from structMetadata is required, not an option.
    ///
    ///   Although this is required, we still keep this macro for future
    /// HDF4 handler enhancement work, which will allow us to turn on and off
    /// this feature easily.
    /// information .
    //
    ///   Please do not confuse this Marco with the SHORT_NAME macro that is
    /// defined in a special build of HDF4 handler. SHORT_NAME macro is used
    /// to disambiguate variable name for CERES data.
    ///
    ///   This behavior is opposite from hdf5 handler. The hdf5 handler takes
    ///the group path information seriously.
#ifdef SHORT_PATH
    string get_short_name(string a_name);
#endif


public:
    /// a flag to indicate that origin information in structMetadata
    /// specifies that a map is upside down.
    bool borigin_upper;
    /// a flag to indicate if structMetdata dataset is processed or not.
    bool bmetadata_Struct;
    /// a buffer for the merged structMetadata dataset
    char metadata_Struct[BUFFER_MAX];

#ifdef NASA_EOS_META
    // Flag for merged metadata. Once it's set, don't prcoess for other
    /// attribute variables that start with the same name.
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
public:
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

    /// A vector that keeps track of dataset names in an HDF-EOS file
    ///
    /// The dataset names are collected as a result of the structMetadata file
    /// parsing. The names also include group path information stored in the
    /// structMetadata file.
    vector < string > full_data_paths;

    /// Pointers for map data arrays
    dods_float32 **dimension_data;

#ifdef USE_HDFEOS2_LIB
    /// Pointer to HDFEOS2 class that reads Grid and Swath information using
    /// hdfeos2 library. This pointer will be active if --with-hdfeos2
    /// configuration option is active.
    auto_ptr<HDFEOS2::File> eos2;
#endif

    HDFEOS();
    virtual ~ HDFEOS();

    /// Remember the data full path of a variable including the name.
    ///
    /// It pushes the EOS-metadata-parsed full path variable name
    /// into the vector for future processing.
    /// \param[in] full_path a full path information of a variable in
    /// metadata.
    void add_data_path(string full_path);

    /// Remeber the dimension list associated with a variable.
    ///
    /// It pushes the EOS-metadata-parsed dimension names list into
    /// map. The map keeps track of the association between variable
    /// and its dimension names for later Grid generation.
    /// \param[in] full_path a full path information of a variable in
    /// metadata.
    /// \param[in]  dimension_list a list of dimensions in metadata.
    void add_dimension_list(string full_path, string dimension_list);

    /// Remember the dimension size associated with the dimension name
    ///
    /// It pushes the EOS-metadata-parsed dimension size into
    /// a map. The map keeps track of the association between variable
    /// and its dimension size for later Grid generation.
    /// \param[in] dimension_name a name of dimension in metadata
    /// \param[in] dimension a dimension size specified in metadata
    void add_dimension_map(string dimension_name, int dimension);

    /// Get the CF standard attribute and dimension names.
    ///
    /// HDF-EOS files used a slightly different dimension and attribute names
    /// from the CF standard. This function corrects such deviation for
    /// a given \a eos_name. Although this function works for some HDF-EOS
    /// files, it is desired to have a separate XML configuration file to
    /// cover all possibilities in HDF-EOS files.
    ///
    /// \return a pointer to a CF-compliant character string
    /// \see get_EOS_name()
    string get_CF_name(const string &str);

    /// Get the EOS attribute and dimension names.
    ///
    /// This function is a reverse mapping of get_CF_name() function.
    /// Give \a cf_name, it translates back to the name used in HDF-EOS file.
    ///
    /// \remark This should use an external XML definition file in the future
    //  for easy handler customization.
    /// \return a string used in HDF-EOS convention
    /// \see get_CF_name()
    string get_EOS_name(const string &cf_name);

    /// Retrieve all possible dimensions used in an HDF-EOS file.
    ///
    /// The CF-convention requires that all shared dimension variables are
    /// visible and their values are accessible. However, some HDF-EOS Grid
    /// do not have them. This function returns what kinds of dimension
    /// names are used in HDF-EOS dataset variables and it'll be later used.
    /// to generate the shared dimension variables.
    ///
    /// \param[out] tokens a vector that will hold all dimension names
    /// \see set_dimension_array();
    /// \see is_shared_dimension_set()
    void get_all_dimensions(vector < string > &tokens);

    /// Get the index of dimension data from the dimension map
    ///
    /// \param[in] name like XDim, YDim and ZDim
    /// \return integer index
    /// \see HDF5ArrayEOS HDF5GridEOS
    int get_dimension_data_location(string name);

    /// Get the size informationfrom the \a name dimension.
    ///
    /// \param[in]  name like XDim, YDim and ZDim
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
    /// For example, it returns '/HDFEOS/GRIDS/CloudFractionAndPressure/'
    /// from '/HDFEOS/GRIDS/CloudFractionAndPressure/Data_Fields/CloudPressure
    /// Precision'.
    /// \param[in] full_path a full path name of a Grid variable
    /// \see h5dds.cc
    string get_grid_name(string full_path);

    /// Check if the HDF-EOS2 file has a GCTP_GEO projection via parser or
    /// via HDF-EOS2 library.
    ///
    /// \return true if the structMetadata has a GCTP_GEO projection.
    /// \return false otherwise
    /// \see set_grid()
    bool is_grid();

    /// Check if this class has already parsed the \a name as grid.
    ///
    /// \param[in] name a name of variable
    /// \return true if it is parsed as Grid.
    /// \return false otherwise
    /// \see h5dds.cc
    bool is_grid(string name);

    /// Check if shared dimension is set.
    ///
    /// CF convention requires that the Grid/Swath map data arrays are visible
    /// outside the DAP Grid and they should share the same dimension size
    /// among Grid/Swath variables. This function checks if the shared
    /// dimension variables are already generated in the DAP output.
    ///
    /// \return true if the shared dimension information is already made.
    /// \return false otherwise.
    /// \see set_shared_dimension()
    bool is_shared_dimension_set();

    /// Check if the current HDF5 file is a valid NASA EOS file.
    ///
    ///
    /// \return true if it has a set of correct meta data files.
    /// \return false otherwise
    bool is_valid();

    /// Parse the structMetadata in an HDF-EOS2 file.
    ///
    /// This function parses the \a str_metadata string which concatenated
    /// structMetadata files.
    ///
    /// \return true if metadata is parsed.
    /// \return false if it is not parsed yet.
    bool parse_struct_metadata(const char* str_metadata);

    /// Print out some key information parsed from metadata
    ///
    /// This function is provided for debugging purporse
    /// to ensure that parsing is done properly.
    void print();

    /// Reset all member variables and frees memory.
    ///
    /// This function is critical when the handler is running under BES.
    /// Since BES is a daemon that never terminates, it is essential to
    /// reset this class whenever there's a new HDF4 file is requested.
    void reset();

    /// Generate artifical dimension array.
    ///
    /// This function generates dimension arrays based on metadata
    /// information.
    /// Since there's no map data for a Grid-like array in NASA EOS data,
    /// map data should be generated based on struct metadata.
    ///
    /// \return ture  if dimension arrays are generated successfully.
    /// \return false if dimension size is negative due to error in
    ///               parsing.
    bool set_dimension_array();

    /// Sets the grid flag true.
    ///
    /// Some HDF-EOS2 grid files have non orthographic projection like
    /// polar or sinusoidal. To distinguish them from ortho projection,
    /// the parser will set the flag true if ortho porjection is detected
    /// and thus normal Grids can be generated.
    ///
    /// If you want to support non-ortho projection files, use HDF-EOS2
    /// library which can perform map interpolation. The parser-based handler
    /// cannot support them.
    ///
    /// \see is_grid()
    void set_grid(bool flag);


    /// Sets the shared dimension flag true.
    /// \see is_shared_dimension_set()
    void set_shared_dimension();


#ifdef USE_HDFEOS2_LIB
    /// Remember the type of HDF-EOS dataset variable via HDF-EOS2 library.
    ///
    /// This function keeps track of the data types of Grid/Swath variables
    /// by storing \a type  into a vector sequentially.
    ///
    /// \see add_variable()
    void add_type(int type);

    /// Remember the name of HDF-EOS dataset variable via HDF-EOS2 library.
    ///
    /// This function keeps track of the Grid/Swath variable names by
    /// storing \a var_name into a vector sequentially.
    ///
    /// \see add_type()
    void add_variable(string var_name);

    /// Check if the current HDF-EOS file is a valid NASA EOS Swath file
    /// via HDF-EOS2 library.
    /// \return true if it is a valid EOS Swath file
    /// \return false otherwise
    bool is_swath();

    /// Check if the current HDF-EOS file uses a 1-D projection
    /// via HDF-EOS2 library.
    /// \return true if it uses a 1-D projection
    /// \return false otherwise
    bool is_orthogonal();

    /// Check if the current HDF-EOS file has 2-D lat/lon with a YDim major
    /// via HDF-EOS2 library.
    /// \return true if it has 2-D lat/lon with a YDim major.
    /// \return false otherwise
    bool is_ydimmajor();

    /// Get the pointer to the array of grid_name via HDF-EOS2 library.
    ///
    /// This function reads the actual content of \a grid_name Grid dataset.
    /// \todo allow subsetting
    int get_data_grid(string grid_name, int* offset,int*step,int*count,int nelms,char** val);

    /// Get the pointer to the array of swath_name via HDF-EOS2 library.
    ///
    /// This function reads the actual content of \a swath_name Swath dataset.
    int get_data_swath(string swath_name, int*offset, int*step,int*count,int nelms,char** val);

    /// Get the data type of variable at location i via HDF-EOS2 library.
    int get_data_type(int i);

    /// Get the XDim (lon) size of HDF-EOS Grid via HDF-EOS2 library.
    int get_xdim_size();

    /// Get the YDim (lat) size of HDF-EOS Grid via HDF-EOS2 library.
    int get_ydim_size();

    /// Handle attributes via HDF-EOS2 library
    void handle_attributes(const HDFEOS2::Attribute *attribute);

    /// Handle data fields via HDF-EOS2 library
    void handle_datafield(const HDFEOS2::Field *field);

    /// Handle grids via HDF-EOS2 library
    void handle_grid(const HDFEOS2::GridDataset *grid);

    /// Handle swath via HDF-EOS2 library
    void handle_swath(const HDFEOS2::SwathDataset *swath);


    /// Open HDF-EOS file via HDF-EOS2 library
    int open(char* filename);

#endif  // USE_HDFEOS2_LIB
};
#endif // #ifndef _HDFEOS_H
