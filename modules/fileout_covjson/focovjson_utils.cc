// -*- mode: c++; c-basic-offset:4 -*-
//
// focovjson_utils.cc
//
// This file is part of BES CovJSON File Out Module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
// Author: Corey Hemphill <hemphilc@oregonstate.edu>
// Author: River Hendriksen <hendriri@oregonstate.edu>
// Author: Riley Rimer <rrimer@oregonstate.edu>
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
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "focovjson_utils.h"


#include <BESDebug.h>

#include <sstream>
#include <iomanip>

#define utils_debug_key "focovjson" // Dont know what this does -Riley

namespace focovjson {

std::string escape_for_covjson(const std::string &input) {
    std::stringstream ss;
    for (size_t i = 0; i < input.length(); ++i) {
        if (unsigned(input[i]) < '\x20' || input[i] == '\\' || input[i] == '"') {
            ss << "\\u" << std::setfill('0') << std::setw(4) << std::hex << unsigned(input[i]);
        } else {
            ss << input[i];
        }
    }
    return ss.str();
}

/**
 * Compute the constrained shape of the Array and return it in a vector.
 * Also return the total number of elements in the constrained array.
 *
 * @param a The Array to examine
 * @param shape The shape of the Array, taking into account the constraint
 * @return The total number of elements in the constrained Array.
 */
long computeConstrainedShape(libdap::Array *a, std::vector<unsigned int> *shape ) {
    BESDEBUG(utils_debug_key, "focovjson::computeConstrainedShape() - BEGIN. Array name: "<< a->name() << endl);

    libdap::Array::Dim_iter dIt;
    unsigned int start;
    unsigned int stride;
    unsigned int stop;

    unsigned int dimSize = 1;
    int dimNum = 0;
    long totalSize = 1;

    BESDEBUG(utils_debug_key, "focovjson::computeConstrainedShape() - Array has " << a->dimensions(true) << " dimensions."<< endl);

    for(dIt = a->dim_begin() ; dIt!=a->dim_end() ;dIt++){
        BESDEBUG(utils_debug_key, "focovjson::computeConstrainedShape() - Processing dimension '" << a->dimension_name(dIt)<< "'. (dim# "<< dimNum << ")"<< endl);
        start  = a->dimension_start(dIt, true);
        stride = a->dimension_stride(dIt, true);
        stop   = a->dimension_stop(dIt, true);
        BESDEBUG(utils_debug_key, "focovjson::computeConstrainedShape() - start: " << start << "  stride: " << stride << "  stop: "<<stop<< endl);

        dimSize = 1 + ( (stop - start) / stride);
        BESDEBUG(utils_debug_key, "focovjson::computeConstrainedShape() - dimSize: " << dimSize << endl);

        (*shape)[dimNum++] = dimSize;
        totalSize *= dimSize;
    }
    BESDEBUG(utils_debug_key, "focovjson::computeConstrainedShape() - totalSize: " << totalSize << endl);
    BESDEBUG(utils_debug_key, "focovjson::computeConstrainedShape() - END." << endl);

    return totalSize;
}

#if 0
/**
 * Replace every occurrence of 'char_to_escape' with the same preceded
 * by the backslash '\' character.
 */
std::string backslash_escape(std::string source, char char_to_escape) {
	std::string escaped_result = source;
	if(source.find(char_to_escape) != string::npos ){
		size_t found = 0;
		for(size_t i=0; i< source.length() ; i++){
			if(source[i] == char_to_escape){
				escaped_result.insert( i + found++, "\\");
			}
		}
	}
	return escaped_result;
}
#endif

} /* namespace focovjson */
