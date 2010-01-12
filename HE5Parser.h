// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

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

#ifndef _HE5Parser_H
#define _HE5Parser_H

// Some NASA EOS Aura metadata files are split into several files
// when they are really big (> 65536) like StructMetadata.0, StructMetada.1,
// ... StructMetadat.n. The split point can be anywhere like the middle
// of the variable name or object description
// To parse the metadata correctly, it is necessary to merge the split
// files first. Thus, we define a big enough buffer to hold the merged
// metadata --- 10 * (2^16). This number ensures that up to 10 metadata files
// can be merged per metadata type (coremetadata, archivedmetadata, etc.).
#define BUFFER_MAX 655360       

#include "config_hdf5.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>

#include <hdf5.h>
#include <debug.h>
#include <util.h>
#include <string.h>
#include <Array.h>
#include "he5dds.tab.hh"
#include "HE5CF.h"

using namespace std;
using namespace libdap;

/// A class for handling NASA EOS AURA data.
///
/// This class contains functions that parse NASA EOS AURA StructMetadata
/// and prepares the necessary (grid) data for OPeNDAP.
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2007 The HDF Group
///
/// All rights reserved.
class HE5Parser:public HE5CF {

private:


    bool _valid;
    bool has_group(hid_t id, const char *name);
    bool has_dataset(hid_t id, const char *name);
    
    // The below reset() function is necessary for Hyrax.
    // Under Hyrax, unlike CGI-based server, the handler program does not
    // terminate after a serving an HDF-EOS5 data. Thus, you need to manually
    // reset the shared dimensions and other variables whenever a new DAP
    // request comes in for a different HDF-EOS5 file.
    void reset();


public:
    bool MLS;
    bool OMI;
    bool TES;
    bool valid_projection;
    bool grid_structure_found;
    bool swath_structure_found;
    /// a flag to indicate if structMetdata dataset is processed or not.
    bool bmetadata_Struct;

    /// a buffer for the merged structMetadata dataset
    char metadata_Struct[BUFFER_MAX];

    int  parser_state;

#ifdef NASA_EOS_META
    // Flag for merged metadata. Once it's set, don't prcoess for other
    // attribute variables that start with the same name.
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

  
    HE5Parser();
    virtual ~ HE5Parser();


    /// Check if this file is EOS file by examining metadata
    ///
    /// \param id root group id
    /// \return 1, if it is EOS file.
    /// \return 0, if not.
    bool check_eos(hid_t id);


    /// Check if the current HDF-EOS5 file is a TES file. 
    /// 
    /// This function is required since TES dimension values need a special
    /// attention. The computation of Grid geographic map data is slightly
    /// different from OMI data, which is default. Please see the inside of
    /// set_dimension_array() function for details.
    ///
    /// \return true if it is a TES product.
    /// \return false otherwise
    /// \see set_dimension_array()
    bool is_TES();

  
    /// Check if the current HDF5 file is a valid NASA EOS file.
    ///
    /// \return true if it has a set of correct meta data files.
    /// \return false otherwise  
    bool is_valid();

    /// Merges metafiles and Sets metdata buffer
    ///
    /// This function is needed because some metada files are split
    /// into several files like StructMetadata.0, StructMetadata.1, ... and
    /// StructMetadata.n.
    /// Here, we assume that there are less than 10 metadata split files
    /// for each metadata type.
    ///
    /// \param[in] id dataset id
    /// \param[in]  metadata_name pre-defined names like "StructMetadata" or
    /// "CoreMetadata"
    /// \param[out]  metadata_buffer the buffer for merged strings.
    /// \return ture if there exists \a metadata_name string variable exists
    /// \return false otherwise.
    bool set_metadata(hid_t id, char *metadata_name,
                      char *metadata_buffer);

};
#endif
