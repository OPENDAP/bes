
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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

#include "config.h"

#include <sstream>
#include <cstdlib>
#include <BESDebug.h>

#include "H4ByteStream.h"

using namespace std;

namespace dmrpp {

void
H4ByteStream::ingest_position_in_array(string pia){
	if(!pia.length())
		return;
	// Clear the thing if it's got stuff in it.
	if(d_chunk_position_in_array.size())
		d_chunk_position_in_array.clear();

	string space = " ";
	size_t strPos = 0;
	string strVal;

	while ((strPos = pia.find(space)) != string::npos) {
		strVal = pia.substr(0, strPos);
		BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " -  Parsing: " << strVal << endl);
		d_chunk_position_in_array.push_back(strtol(strVal.c_str(),NULL,10));
		pia.erase(0, strPos + space.length());
	}
}

/**
 * @brief Returns a curl range argument.
 * The libcurl requires a string argument for range-ge activitys, this method
 * constructs one in the required syntax from the offset and size information
 * for this byteStream.
 *
 */
std::string
H4ByteStream::get_curl_range_arg_string(){
    ostringstream range;   // range-get needs a string arg for the range
    range << d_offset << "-" << d_offset + d_size - 1;
    return range.str();
}

/**
 *
 *  unsigned long long d_size;
 *  unsigned long long d_offset;
 *  std::string d_md5;
 *  std::string d_uuid;
 *  std::string d_data_url;
 *  std::vector<unsigned int> d_chunk_position_in_array;
 *
 */
std::string
H4ByteStream::to_string(){
    ostringstream oss;
    oss << "H4ByteStream";
    oss << "[data_url='" <<  d_data_url  << "']";
    oss << "[offset=" <<  d_offset  << "]";
    oss << "[size=" <<  d_size  << "]";
    oss << "[md5=" <<  d_md5  << "]";
    oss << "[uuid=" <<  d_uuid  << "]";
    oss << "[chunk_position_in_array=(";
	for(unsigned long i=0; i<d_chunk_position_in_array.size();i++){
		if(i) oss << ",";
		oss << d_chunk_position_in_array[i];
	}
	oss << ")]";
	return oss.str();
}

} // namespace dmrpp


