
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

#include "DmrppArray.h"
#include "DmrppUtil.h"
#include "Odometer.h"

using namespace dmrpp;
using namespace libdap;
using namespace std;

namespace dmrpp {

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
 * @brief Compute the index of the address_in_target for an an array of target_shape.
 * Since we store multidimensional arrays as a single one dimensional array
 * internally we need to be able to locate a particular address in the one dimensional
 * storage utilizing an n-tuple (where n is the dimension of the array). The get_index
 * function does this by computing the location based on the n-tuple address_in_target
 * and the shape of the array, passed in as target_shape.
 */
unsigned long long get_index(vector<unsigned int> address_in_target, const vector<unsigned int> target_shape){
	if(address_in_target.size() != target_shape.size()){
		ostringstream oss;
		oss << "The target_shape  (size: "<< target_shape.size() << ")" <<
				" and the address_in_target (size: " << address_in_target.size() << ")" <<
				" have different dimensionality.";
		throw  BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
	}
	unsigned long long digit_multiplier=1;
	unsigned long long subject_index = 0;
	for(int i=target_shape.size()-1; i>=0 ;i--){
		if(address_in_target[i]>target_shape[i]){
			ostringstream oss;
			oss << "The address_in_target["<< i << "]: " << address_in_target[i] <<
					" is larger than  target_shape[" << i << "]: " << target_shape[i] <<
					" This will make the bad things happen.";
			throw  BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
		}
		subject_index += address_in_target[i]*digit_multiplier;
		digit_multiplier *= target_shape[i];
	}
	return subject_index;
}

/**
 * @brief This recursive private method collects values from the rbuf and copies
 * them into buf. It supports stop, stride, and start and while correct is not
 * efficient.
 */
void
DmrppArray::read_constrained(
		Dim_iter dimIter,
		unsigned long *target_index,
		vector<unsigned int> &subsetAddress,
		const vector<unsigned int> &array_shape)
{
    BESDEBUG("dmrpp", "DmrppArray::read_constrained() - subsetAddress.size(): " << subsetAddress.size() << endl);

    unsigned int bytesPerElt = prototype()->width();
	char *sourceBuf = get_rbuf();
	char *targetBuf = get_buf();

	unsigned int start = this->dimension_start(dimIter);
	unsigned int stop = this->dimension_stop(dimIter,true);
	unsigned int stride = this->dimension_stride(dimIter,true);
    BESDEBUG("dmrpp","DmrppArray::read_constrained() - start: " << start << " stride: " << stride << " stop: " << stop << endl);

    dimIter++;

    // This is the end case for the recursion.
    // TODO stride == 1 belongs inside this or else rewrite this as if else if else
    // see below.
    if (dimIter == dim_end() && stride == 1) {
        BESDEBUG("dmrpp", "DmrppArray::read_constrained() - stride is 1, copying from all values from start to stop." << endl);

    	subsetAddress.push_back(start);
		unsigned int start_index = get_index(subsetAddress,array_shape);
        BESDEBUG("dmrpp", "DmrppArray::read_constrained() - start_index: " << start_index << endl);
    	subsetAddress.pop_back();

    	subsetAddress.push_back(stop);
		unsigned int stop_index = get_index(subsetAddress,array_shape);
        BESDEBUG("dmrpp", "DmrppArray::read_constrained() - stop_index: " << start_index << endl);
    	subsetAddress.pop_back();

    	// Copy data block from start_index to stop_index
    	// FIXME Replace this loop with a call to std::memcpy()
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
    		if (dimIter != dim_end()) {
    			// Nope!
    			// then we recurse to the last dimension to read stuff
    			subsetAddress.push_back(myDimIndex);
    			read_constrained(dimIter,target_index,subsetAddress, array_shape);
    			subsetAddress.pop_back();
    		}
    		else {
				// We are at the last (inner most) dimension.
				// So it's time to copy values.
				subsetAddress.push_back(myDimIndex);
				unsigned int sourceIndex = get_index(subsetAddress,array_shape);
    	        BESDEBUG("dmrpp", "DmrppArray::read_constrained()  - "
    	        		"Copying source value at sourceIndex: " << sourceIndex << endl);
				subsetAddress.pop_back();
				// Copy a single value.
	    		unsigned long target_byte = *target_index * bytesPerElt;
			    unsigned long source_byte = sourceIndex  * bytesPerElt;

			    // FIXME Replace this loop with a call to std::memcpy()
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

    vector<H4ByteStream> chunk_refs = get_immutable_chunks();
    if(chunk_refs.size() == 0){
        ostringstream oss;
        oss << "DmrppArray::read() - Unable to obtain a byteStream object for array " << name()
        		<< " Without a byteStream we cannot read! "<< endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }
    else {
		BESDEBUG("dmrpp", "DmrppArray::read() - Found chunks: " << endl);
    	for(unsigned long i=0; i<chunk_refs.size(); i++){
    		BESDEBUG("dmrpp", "DmrppArray::read() - chunk[" << i << "]: " << chunk_refs[i].to_string() << endl);
    	}
    }

    // For now we only handle the one chunk case.
    H4ByteStream h4bs = chunk_refs[0];

    // First cut at subsetting; read the whole thing and then subset that.
    unsigned long long array_nbytes = h4bs.get_size();	// TODO remove. jhrg

    rbuf_size(array_nbytes);

    BESDEBUG("dmrpp", "DmrppArray::read() - Reading  " << h4bs.get_data_url() << ": " << h4bs.get_curl_range_arg_string() << endl);

    curl_read_bytes(h4bs.get_data_url(), h4bs.get_curl_range_arg_string(), dynamic_cast<DmrppCommon*>(this));

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
        vector<unsigned int> array_shape, subset;
        // number of array elements in the constrained array
        unsigned long long constrained_size = 1;
        for(Dim_iter dim=dim_begin(); dim!=dim_end(); dim++){
        	array_shape.push_back(dimension_size(dim,false));
        	constrained_size *= dimension_size(dim,true);
        }
        BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " - constrained_size:  " << constrained_size << endl);

        reserve_value_capacity(constrained_size);
        unsigned long target_index = 0;
        read_constrained(dim_begin(), &target_index, subset, array_shape); // TODO rename; something other than read. jhrg
        BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " Copied " << target_index << " constrained  values." << endl);


#if 0  // This code, using the Odometer, doesn't work for stride which makes me think that the Odometer could be improved

        Odometer::shape array_shape, subset_shape;
        // number of array elements in the constrained array
        unsigned long long constrained_size = 1;
        for(Dim_iter dim=dim_begin(); dim!=dim_end(); dim++){
        	array_shape.push_back(dimension_size(dim,false));
        	subset_shape.push_back(dimension_size(dim,true));
        	constrained_size *= dimension_size(dim,true);
        }
        BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " - constrained_size:  " << constrained_size << endl);

        Odometer odometer(array_shape);
        reserve_value_capacity(constrained_size);
        unsigned long target_index = 0, offset;

        odometer.indices(subset_shape);
        offset = odometer.next();
        while(target_index<constrained_size && offset!=odometer.end()){
        	get_buf()[target_index] = get_rbuf()[offset];
            offset = odometer.next();
            target_index++;
        }
        BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " Copied " << target_index << " constrained  values." << endl);

#endif

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
#if 0
    strm << DapIndent::LMarg << "offset:   " << get_offset() << endl;
    strm << DapIndent::LMarg << "size:     " << get_size() << endl;
    strm << DapIndent::LMarg << "md5:      " << get_md5() << endl;
    strm << DapIndent::LMarg << "uuid:     " << get_uuid() << endl;
    strm << DapIndent::LMarg << "data_url: " << get_data_url() << endl;
#endif
    vector<H4ByteStream> chunk_refs = get_immutable_chunks();
    strm << DapIndent::LMarg << "H4ByteStreams (aka chunks):"
    		<< (chunk_refs.size()?"":"None Found.") << endl;
    DapIndent::Indent();
    for(unsigned int i=0; i<chunk_refs.size() ;i++){
        strm << DapIndent::LMarg << chunk_refs[i].to_string() << endl;
    }
    DapIndent::UnIndent();
    Array::dump(strm);
    strm << DapIndent::LMarg << "value: " << "----" << /*d_buf <<*/ endl;
    DapIndent::UnIndent();
}

} // namespace dmrpp
