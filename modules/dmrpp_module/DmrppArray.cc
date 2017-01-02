
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

#include <cstring>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppArray.h"
#include "DmrppUtil.h"
#include "Odometer.h"

using namespace dmrpp;
using namespace libdap;
using namespace std;

namespace dmrpp {

string
vec2str(vector<unsigned int> v){
	ostringstream oss;
	oss << "(";
	for(unsigned long long i=0; i<v.size() ;i++){
		oss << (i?",":"") << v[i];
	}
	oss << ")";
	return oss.str();
}

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
DmrppArray::read_constrained_no_chunks(
		Dim_iter dimIter,
		unsigned long *target_index,
		vector<unsigned int> &subsetAddress,
		const vector<unsigned int> &array_shape,
		H4ByteStream *h4bytestream)
{
    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - subsetAddress.size(): " << subsetAddress.size() << endl);

    unsigned int bytesPerElt = prototype()->width();
	char *sourceBuf = h4bytestream->get_rbuf();
	char *targetBuf = get_buf();

	unsigned int start = this->dimension_start(dimIter);
	unsigned int stop = this->dimension_stop(dimIter,true);
	unsigned int stride = this->dimension_stride(dimIter,true);
    BESDEBUG("dmrpp","DmrppArray::"<< __func__ << "() - start: " << start << " stride: " << stride << " stop: " << stop << endl);

    dimIter++;

    // This is the end case for the recursion.
    // TODO stride == 1 belongs inside this or else rewrite this as if else if else
    // see below.
    if (dimIter == dim_end() && stride == 1) {
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - stride is 1, copying from all values from start to stop." << endl);

    	subsetAddress.push_back(start);
		unsigned int start_index = get_index(subsetAddress,array_shape);
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - start_index: " << start_index << endl);
    	subsetAddress.pop_back();

    	subsetAddress.push_back(stop);
		unsigned int stop_index = get_index(subsetAddress,array_shape);
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - stop_index: " << start_index << endl);
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
    			read_constrained_no_chunks(dimIter,target_index,subsetAddress, array_shape,h4bytestream);
    			subsetAddress.pop_back();
    		}
    		else {
				// We are at the last (inner most) dimension.
				// So it's time to copy values.
				subsetAddress.push_back(myDimIndex);
				unsigned int sourceIndex = get_index(subsetAddress,array_shape);
    	        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - "
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

vector<unsigned int>
DmrppArray::get_shape(bool constrained){
    vector<unsigned int> array_shape;
    for(Dim_iter dim=dim_begin(); dim!=dim_end(); dim++){
    	array_shape.push_back(dimension_size(dim,constrained));
    }
    return array_shape;
}

unsigned long long
DmrppArray::get_size(bool constrained){
    // number of array elements in the constrained array
    unsigned long long constrained_size = 1;
    for(Dim_iter dim=dim_begin(); dim!=dim_end(); dim++){
    	constrained_size *= dimension_size(dim,constrained);
    }
    return constrained_size;
}

// FIXME This version of read() should work for unconstrained accesses where
// we don't have to think about chunking. jhrg 11/23/16
bool
DmrppArray::read_no_chunks()
{
    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for " << name()  << " BEGIN" << endl);

    if (read_p())
        return true;

    vector<H4ByteStream> *chunk_refs = get_chunk_vec();
    if(chunk_refs->size() == 0){
        ostringstream oss;
        oss << "DmrppArray::"<< __func__ << "() - Unable to obtain a byteStream object for array " << name()
        		<< " Without a byteStream we cannot read! "<< endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }
    else {
		BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - Found chunks: " << endl);
    	for(unsigned long i=0; i<chunk_refs->size(); i++){
    		BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
    	}
    }

    // For now we only handle the one chunk case.
    H4ByteStream h4_byte_stream = (*chunk_refs)[0];
    h4_byte_stream.read();

    if (!is_projected()) {      // if there is no projection constraint
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - No projection, copying all values into array. " << endl);
        val2buf(h4_byte_stream.get_rbuf());    // yes, it's not type-safe
    }
    // else
    // Get the start, stop values for each dimension
    // Determine the size of the destination buffer (should match length() / width())
    // Allocate the dest buffer in the array
    // Use odometer code to copy data out of the rbuf and into the dest buffer of the array
    else {
        vector<unsigned int> array_shape = get_shape(false);
        // number of array elements in the constrained array
        unsigned long long constrained_size = get_size(true);

        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - constrained_size:  " << constrained_size << endl);

        reserve_value_capacity(constrained_size);
        unsigned long target_index = 0;
        vector<unsigned int> subset;
        read_constrained_no_chunks(dim_begin(), &target_index, subset, array_shape, &h4_byte_stream); // TODO rename; something other than read. jhrg
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - Copied " << target_index << " constrained  values." << endl);


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

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for " << name()  << " END"<< endl);

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

bool
DmrppArray::read(){
	if(get_chunk_dimension_sizes().empty()){
		if(get_immutable_chunks().size()==1){
			// This handles the case for arrays that have exactly one h4:byteStream
			// and no chunking setup.
			return read_no_chunks();
		}
		else {
		    ostringstream oss;
		    oss << "DmrppArray: Unchunked arrays must have exactly one H4ByteStream object. "
		    		"This one has " << get_immutable_chunks().size() << endl;
		    throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
		}
	}
    // so now we know we are handling the chunks
	return read_chunked();
}


bool
DmrppArray::read_chunked(){

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for variable '" << name()  << "' - BEGIN" << endl);

    if (read_p())
        return true;

    vector<H4ByteStream> *chunk_refs = get_chunk_vec();
    if(chunk_refs->size() == 0){
        ostringstream oss;
        oss << "DmrppArray::"<< __func__ << "() - Unable to obtain a byteStream object for array " << name()
        		<< " Without a byteStream we cannot read! "<< endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }
    // Allocate target memory.
	reserve_value_capacity(length());
	vector<unsigned int> array_shape = get_shape(false);

	BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - dimensions(): " << dimensions(false)  <<
			" array_shape.size(): " << array_shape.size() << endl);

	if(this->dimensions(false) != array_shape.size()){
        ostringstream oss;
        oss << "DmrppArray::"<< __func__ << "() - array_shape does not match the number of array dimensions! "<< endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
	}
	switch (dimensions()) {
	//########################### OneD Arrays ###############################
	case 1: {
		BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - 1D Array. Reading " << chunk_refs->size() << " chunks" << endl);
		for(unsigned long i=0; i<chunk_refs->size(); i++){
			BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - READING chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
			H4ByteStream h4bs = (*chunk_refs)[i];
			h4bs.read();
			char * source_buffer = h4bs.get_rbuf();
			char * target_buffer = get_buf();
			vector<unsigned int> start_position = h4bs.get_position_in_array();
			// oneD case for now.
			unsigned long long start_char_index = start_position[0] * prototype()->width();
			BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - start_char_index: " << start_char_index << " start_position: " << start_position[0] << endl);
			memcpy(target_buffer+start_char_index, source_buffer, h4bs.get_rbuf_size());
		}
	} break;
	//########################### TwoD Arrays ###############################
	case 2: {
		BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - 2D Array. Reading " << chunk_refs->size() << " chunks" << endl);
		char * target_buffer = get_buf();
		vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();
		for(unsigned long i=0; i<chunk_refs->size(); i++){
			BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - READING chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
			H4ByteStream h4bs = (*chunk_refs)[i];
			h4bs.read();
			char * source_buffer = h4bs.get_rbuf();
			vector<unsigned int> chunk_origin = h4bs.get_position_in_array();
			vector<unsigned int> chunk_row_address = chunk_origin;
			unsigned long long target_element_index = get_index(chunk_origin,array_shape);
			unsigned long long target_char_index = target_element_index * prototype()->width();
			unsigned long long source_element_index = 0;
			unsigned long long source_char_index = source_element_index * prototype()->width();
			unsigned long long chunk_inner_dim_bytes = chunk_shape[1] * prototype()->width();

			BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Packing Array From Chunks: "
					 << " chunk_inner_dim_bytes: " << chunk_inner_dim_bytes << endl);

			for(unsigned int i=0; i<chunk_shape[0] ;i++){
				BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
						"target_char_index: " << target_char_index <<
						" source_char_index: " << source_char_index << endl);
				memcpy(target_buffer+target_char_index, source_buffer+source_char_index, chunk_inner_dim_bytes);
				chunk_row_address[0] += 1;
				target_element_index = get_index(chunk_row_address,array_shape);
				target_char_index = target_element_index * prototype()->width();
				source_element_index += chunk_shape[1];
				source_char_index = source_element_index * prototype()->width();
			}
		}

	} break;
	//########################### ThreeD Arrays ###############################
	case 3: {
		BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - 3D Array. Reading " << chunk_refs->size() << " chunks" << endl);

		char * target_buffer = get_buf();
		vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();
		unsigned long long chunk_inner_dim_bytes = chunk_shape[2] * prototype()->width();

		for(unsigned long i=0; i<chunk_refs->size(); i++){
			BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - READING chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
			H4ByteStream h4bs = (*chunk_refs)[i];
			h4bs.read();

			vector<unsigned int> chunk_origin = h4bs.get_position_in_array();

			char * source_buffer = h4bs.get_rbuf();
			unsigned long long source_element_index = 0;
			unsigned long long source_char_index = 0;

			unsigned long long target_element_index = get_index(chunk_origin,array_shape);
			unsigned long long target_char_index = target_element_index * prototype()->width();


			BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Packing Array From Chunks: "
					 << " chunk_inner_dim_bytes: " << chunk_inner_dim_bytes << endl);

			unsigned int K_DIMENSION = 0; // Outermost dim
			unsigned int J_DIMENSION = 1;
			unsigned int I_DIMENSION = 2; // inner most dim (fastest varying)

			vector<unsigned int> chunk_row_insertion_point_address = chunk_origin;
			for(unsigned int k=0; k<chunk_shape[K_DIMENSION]; k++){
				chunk_row_insertion_point_address[K_DIMENSION] = chunk_origin[K_DIMENSION] + k;
				BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
						<< "k: " << k << "  chunk_row_insertion_point_address: "
						<< vec2str(chunk_row_insertion_point_address) << endl);
				for(unsigned int j=0; j<chunk_shape[J_DIMENSION]; j++){
					chunk_row_insertion_point_address[J_DIMENSION] = chunk_origin[J_DIMENSION] + j;
					target_element_index = get_index(chunk_row_insertion_point_address,array_shape);
					target_char_index = target_element_index * prototype()->width();

					BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
							"k: " << k << " j: " << j <<
							" target_char_index: " << target_char_index <<
							" source_char_index: " << source_char_index <<
							" chunk_row_insertion_point_address: " << vec2str(chunk_row_insertion_point_address) << endl);

					memcpy(target_buffer+target_char_index, source_buffer+source_char_index, chunk_inner_dim_bytes);
					source_element_index += chunk_shape[I_DIMENSION];
					source_char_index = source_element_index * prototype()->width();
				}
			}
		}

	} break;
	//########################### FourD Arrays ###############################
	case 4: {
		BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - 4D Array. Reading " << chunk_refs->size() << " chunks" << endl);

		char * target_buffer = get_buf();
		vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();
		unsigned long long chunk_inner_dim_bytes = chunk_shape[2] * prototype()->width();

		for(unsigned long i=0; i<chunk_refs->size(); i++){
			BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - READING chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
			H4ByteStream h4bs = (*chunk_refs)[i];
			h4bs.read();

			vector<unsigned int> chunk_origin = h4bs.get_position_in_array();

			char * source_buffer = h4bs.get_rbuf();
			unsigned long long source_element_index = 0;
			unsigned long long source_char_index = 0;

			unsigned long long target_element_index = get_index(chunk_origin,array_shape);
			unsigned long long target_char_index = target_element_index * prototype()->width();


			BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Packing Array From Chunk[" << i << "]"
					 << " chunk_origin: " << vec2str(chunk_origin) << endl);

			unsigned int L_DIMENSION = 0; // Outermost dim
			unsigned int K_DIMENSION = 1;
			unsigned int J_DIMENSION = 2;
			unsigned int I_DIMENSION = 3; // inner most dim (fastest varying)

			vector<unsigned int> chunk_row_insertion_point_address = chunk_origin;
			for(unsigned int l=0; l<chunk_shape[L_DIMENSION]; l++){
				chunk_row_insertion_point_address[L_DIMENSION] = chunk_origin[L_DIMENSION] + l;
				BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
						<< "l: " << l << "  chunk_row_insertion_point_address: "
						<< vec2str(chunk_row_insertion_point_address) << endl);
				for(unsigned int k=0; k<chunk_shape[K_DIMENSION]; k++){
					chunk_row_insertion_point_address[K_DIMENSION] = chunk_origin[K_DIMENSION] + k;
					BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
							<< "l: " << l
							<< " k: " << k
							<< " chunk_row_insertion_point_address: "
							<< vec2str(chunk_row_insertion_point_address) << endl);
					for(unsigned int j=0; j<chunk_shape[J_DIMENSION]; j++){
						chunk_row_insertion_point_address[J_DIMENSION] = chunk_origin[J_DIMENSION] + j;
						target_element_index = get_index(chunk_row_insertion_point_address,array_shape);
						target_char_index = target_element_index * prototype()->width();

						BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
								<< "l: " << l  << " k: " << k << " j: " << j <<
								" target_char_index: " << target_char_index <<
								" source_char_index: " << source_char_index <<
								" chunk_row_insertion_point_address: " << vec2str(chunk_row_insertion_point_address) << endl);

						memcpy(target_buffer+target_char_index, source_buffer+source_char_index, chunk_inner_dim_bytes);
						source_element_index += chunk_shape[I_DIMENSION];
						source_char_index = source_element_index * prototype()->width();
					}
				}
			}
		}
	} break;
	//########################### N-D Arrays ###############################
	default: {
	    ostringstream oss;
	    oss << "DmrppArray: Reading chunked arrays of dimension " << dimensions() <<
	    		" is not yet supported/implemented '" << endl;
	    throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);

	} break;

	}



#if 0
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

    #endif

    return true;
}


void DmrppArray::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppArray::"<< __func__ << "(" << (void *) this << ")" << endl;
    DapIndent::Indent();
    DmrppCommon::dump(strm);
    Array::dump(strm);
    strm << DapIndent::LMarg << "value: " << "----" << /*d_buf <<*/ endl;
    DapIndent::UnIndent();
}

} // namespace dmrpp
