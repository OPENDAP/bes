
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

#include "config.h"

#include <string>
#include <sstream>

#include <BESError.h>
#include <BESDebug.h>

#include "Odometer.h"
#include "DmrppArray.h"
#include "DmrppUtil.h"

using namespace dmrpp;
using namespace libdap;
using namespace std;

void
DmrppArray::_duplicate(const DmrppArray &)
{
}

DmrppArray::DmrppArray(const string &n, BaseType *v) : Array(n, v, true /*is dap4*/), DmrppCommon()
{
}

DmrppArray::DmrppArray(const string &n, const string &d, BaseType *v) : Array(n, d, v, true), DmrppCommon()
{
}

BaseType *
DmrppArray::ptr_duplicate()
{
    return new DmrppArray(*this);
}

DmrppArray::DmrppArray(const DmrppArray &rhs) : Array(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppArray &
DmrppArray::operator=(const DmrppArray &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Array &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

/**
 * @brief Is this Array subset?
 * @return True if the array has a projection expression, false otherwise
 */
bool
DmrppArray::is_projected()
{
    for (Dim_iter p = dim_begin(), e = dim_end(); p != e; ++p)
        if (dimension_size(p, true) != dimension_size(p, false))
            return true;

    return false;
}



/**
 * Read data - there must be a better description
 *
 * @param odometer
 * @param dim
 * @param target_index
 * @param subsetAddress
 */
void
DmrppArray::read_constrained(Odometer &odometer, Dim_iter dim, unsigned long *target_index, Odometer::shape &subsetAddress)
{
    BESDEBUG("dmrpp", "DmrppArray::read_constrained() - subsetAddress.size(): " << subsetAddress.size() << endl);

    unsigned int bytesPerElt = prototype()->width();
	char *sourceBuf = get_rbuf();
	char *targetBuf = get_buf();

	// Dim_iter myDim = dim; // TODO Remove. jhrg
	unsigned int start = this->dimension_start(dim,true);
	unsigned int stop = this->dimension_stop(dim,true);
	unsigned int stride = this->dimension_stride(dim,true);
    BESDEBUG("dmrpp","DmrppArray::read_constrained() - start: " << start << " stride: " << stride << " stop: " << stop << endl);

    dim++;

    // This is the end case for the recursion.
    // TODO stride == 1 belongs inside this or else rewrite this as if else if else
    // see below.
    if (dim == dim_end() && stride == 1) {
        BESDEBUG("dmrpp", "DmrppArray::read_constrained() - stride is 1, copying from all values from start to stop." << endl);

    	subsetAddress.push_back(start);
		unsigned int start_index = odometer.set_indices(subsetAddress);
    	subsetAddress.pop_back();

    	subsetAddress.push_back(stop);
		unsigned int stop_index = odometer.set_indices(subsetAddress);
    	subsetAddress.pop_back();

        BESDEBUG("dmrpp", "DmrppArray::read_constrained() - start_index: " << start_index << endl);
        BESDEBUG("dmrpp", "DmrppArray::read_constrained() - stop_index:  " << stop_index << endl);

    	// Copy data block from start_index to stop_index
        // @FIXME I had to make the test <= instead of < and I think that's not right, but it works. HELP?
    	for(unsigned int sourceIndex=start_index; sourceIndex<=stop_index ;sourceIndex++,target_index++){
    		unsigned long target_byte = *target_index * bytesPerElt;
		    unsigned long source_byte = sourceIndex  * bytesPerElt;
			// Copy a single value.
		    for(unsigned int i=0; i< bytesPerElt ; i++){
		    	targetBuf[target_byte++] = sourceBuf[source_byte++];
		    }
		    (*target_index)++;
    	}
    }
    else {

    	for(unsigned int myDimIndex=start; myDimIndex<=stop ;myDimIndex+=stride){
    		// Is it the last dimension?
    		if(dim != dim_end()){
    			// Nope!
    			// then we recurse to the last dimension to read stuff
    			subsetAddress.push_back(myDimIndex);
    			read_constrained(odometer,dim,target_index,subsetAddress);
    			subsetAddress.pop_back();
    		}
    		else {

				// We are at the last (inner most) dimension.
				// So it's time to copy values.
				subsetAddress.push_back(myDimIndex);
				unsigned int sourceIndex = odometer.set_indices(subsetAddress);
				subsetAddress.pop_back();
    	        BESDEBUG("dmrpp", "DmrppArray::read_constrained()  - Copying source value at sourceIndex: " << sourceIndex << endl);
				// Copy a single value.
	    		unsigned long target_byte = *target_index * bytesPerElt;
			    unsigned long source_byte = sourceIndex  * bytesPerElt;
			    for(unsigned int i=0; i< bytesPerElt ; i++){
			    	targetBuf[target_byte++] = sourceBuf[source_byte++];
			    }
			    (*target_index)++;

    		}
    	}
    }
}

// FIXME This version of read() should work for unconstrained accesses where
// we don't have to think about chunking. jhrg 11/23/16
bool
DmrppArray::read()
{
    BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " for " << name()  << " BEGIN" << endl);

    if (read_p())
        return true;

    // First cut at subsetting; read the whole thing and then subset that.
    unsigned long long array_nbytes = get_size();	// TODO remove. jhrg

    rbuf_size(array_nbytes);

    ostringstream range;   // range-get needs a string arg for the range
    range << get_offset() << "-" << get_offset() + get_size() - 1;

    BESDEBUG("dmrpp", "DmrppArray::read() - Reading  " << get_data_url() << ": " << range.str() << endl);

    curl_read_bytes(get_data_url(), range.str(), dynamic_cast<DmrppCommon*>(this));

    // If the expected byte count was not read, it's an error.
    if (array_nbytes != get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppArray: Wrong number of bytes read for '" << name() << "'; expected " << array_nbytes
            << " but found " << get_bytes_read() << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    if (!is_projected()) {      // if there is no projection constraint
        BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " - No projection, copying all values into array. " << endl);
        val2buf(get_rbuf());    // yes, it's not type-safe
    }
    // else
    // Get the start, stop values for each dimension
    // Determine the size of the destination buffer (should match length() / width())
    // Allocate the dest buffer in the array
    // Use odometer code to copy data out of the rbuf and into the dest buffer of the array
    else {
        Odometer::shape full_shape, subset;
        // number of array elements in the constrained array
        unsigned long long constrained_size = 1;
        for(Dim_iter dim=dim_begin(); dim!=dim_end(); dim++){
        	full_shape.push_back(dimension_size(dim,false));
        	constrained_size *= dimension_size(dim,true);
        }
        BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " - constrained_size:  " << constrained_size << endl);

        Odometer odometer(full_shape);
        Dim_iter dimension = dim_begin();	// TODO Remove this. jhrg
        unsigned long target_index = 0;
        reserve_value_capacity(constrained_size);
        read_constrained(odometer, dimension, &target_index, subset); // TODO rename; something other than read. jhrg
        BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " Copied " << target_index << " constrained  values." << endl);
    }

    set_read_p(true);

    BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " for " << name()  << " END"<< endl);

    return true;
}

#if 0
/**
 * This is a simplistic version of read() that looks at the one and two-dimensional
 * cases and solves them with minimal fuss. It might not support stride, we'll see.
 * It certainly won't build, but it can be made 'buildable' with just a bit of work.
 * jhrg
 */
static bool
faux_read()
{
    BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " for " << name()  << " BEGIN" << endl);

    if (read_p())
        return true;

    // allocate the DMR++ Common read buffer
    rbuf_size(get_size(););

    ostringstream range;   // range-get needs a string arg for the range
    range << get_offset() << "-" << get_offset() + get_size() - 1;

    BESDEBUG("dmrpp", "DmrppArray::read() - Reading  " << get_data_url() << ": " << range.str() << endl);

    // Read the entire array.
    curl_read_bytes(get_data_url(), range.str(), dynamic_cast<DmrppCommon*>(this));

    // If the expected byte count was not read, it's an error.
    if (get_size(); != get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppArray: Wrong number of bytes read for '" << name() << "'; expected " << get_size();
            << " but found " << get_bytes_read() << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    if (!is_projected()) {      // if there is no projection constraint
        BESDEBUG("dmrpp", "No projection, copying all values into array." << endl);
        val2buf(get_rbuf());    // yes, it's not type-safe
    }
    // else
    // Get the start, stop values for each dimension
    // Determine the size of the destination buffer (should match length() / width())
    // Allocate the dest buffer in the array
    // Use odometer code to copy data out of the rbuf and into the dest buffer of the array
    else {
    	switch (dimensions()) {
    	case 1:
    		// Access dimension start and stop and use memcpy
    		unsigned long start = dimension_start(dim_begin(), true);

    		start *= sizeof();

    		reserve_value_capacity(length());

    		memcpy(get_buf(), get_rbuf() + start, width());


    		break;
    	case 2:
    		// Access outer dim start and stop and use for loop
    		// Access inner dim start and stop and use memcpy.
    		break;
    	default:
    		// Add general purpose version here
			throw BESError("The DMR++ hander only supports constraints on one and two-dimensional arrays.",
					BES_INTERNAL_ERROR, __FILE__, __LINE__);
    	}
    }

    set_read_p(true);

    BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " for " << name()  << " END"<< endl);

    return true;
}
#endif

void DmrppArray::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppArray::dump - (" << (void *) this << ")" << endl;
    DapIndent::Indent();
    strm << DapIndent::LMarg << "offset: " << get_offset() << endl;
    strm << DapIndent::LMarg << "size:   " << get_size() << endl;
    strm << DapIndent::LMarg << "md5:    " << get_md5() << endl;
    strm << DapIndent::LMarg << "uuid:   " << get_uuid() << endl;
    Array::dump(strm);
    strm << DapIndent::LMarg << "value: " << "----" << /*d_buf <<*/ endl;
    DapIndent::UnIndent();
}
