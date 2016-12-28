
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <string>
#include <vector>

#include <BESDebug.h>

#include "DmrppCommon.h"
#include "H4ByteStream.h"

using namespace std;

namespace dmrpp {
/**
 * Interface for the size and offset information of data described by
 * DMR++ files.
 */

    void
	DmrppCommon::ingest_chunk_dimension_sizes(std::string chunk_dim_sizes_string)
    {
    	if(!chunk_dim_sizes_string.length())
    		return;

    	// Clear the thing if it's got stuff in it.
    	if(d_chunk_dimension_sizes.size())
    		d_chunk_dimension_sizes.clear();

    	string space(" ");
    	size_t strPos = 0;
        string strVal;

       // Process comma delimited content
    	while ((strPos = chunk_dim_sizes_string.find(space)) != string::npos) {
    		strVal = chunk_dim_sizes_string.substr(0, strPos);
    		BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " -  Parsing: " << strVal << endl);
    		d_chunk_dimension_sizes.push_back(strtol(strVal.c_str(),NULL,10));
    		chunk_dim_sizes_string.erase(0, strPos + space.length());
    	}
    }

    void
	DmrppCommon::ingest_compression_type(std::string compression_type_string)
    {
    	if(!compression_type_string.length())
    		return;

    	// Clear previous state
    	d_compression_type_deflate = false;
    	d_compression_type_shuffle = false;

    	string deflate("deflate");
    	string shuffle("shuffle");
    	size_t strPos = 0;

       // Process content
    	if ((strPos = compression_type_string.find(deflate)) != string::npos){
    		d_compression_type_deflate = true;
    	}


    	if ((strPos = compression_type_string.find(shuffle)) != string::npos){
    		d_compression_type_shuffle = true;
    	}

		BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " - Processed compressionType string. "
				"d_compression_type_shuffle: " << (d_compression_type_shuffle?"true":"false") <<
				"d_compression_type_deflate: " << (d_compression_type_deflate?"true":"false") << endl);
    }

} // namepsace dmrpp


