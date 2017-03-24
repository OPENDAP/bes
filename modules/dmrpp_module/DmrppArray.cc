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
#include <iomanip>
#include <set>
#include <stack>

#include <cstring>
#include <cassert>

#include <unistd.h>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppArray.h"
#include "DmrppUtil.h"
#include "Odometer.h"


#if 0
using namespace dmrpp;
#endif
using namespace libdap;
using namespace std;

namespace dmrpp {

/**
 * @brief Write an int vector to a string.
 * @note Only used by BESDEBUG calls
 * @param v
 * @return The string
 */
static string vec2str(vector<unsigned int> v)
{
    ostringstream oss;
    oss << "(";
    for (unsigned long long i = 0; i < v.size(); i++) {
        oss << (i ? "," : "") << v[i];
    }
    oss << ")";
    return oss.str();
}

void DmrppArray::_duplicate(const DmrppArray &)
{
}

DmrppArray::DmrppArray(const string &n, BaseType *v) :
                Array(n, v, true /*is dap4*/), DmrppCommon()
{
}

DmrppArray::DmrppArray(const string &n, const string &d, BaseType *v) :
                Array(n, d, v, true), DmrppCommon()
{
}

BaseType *
DmrppArray::ptr_duplicate()
{
    return new DmrppArray(*this);
}

DmrppArray::DmrppArray(const DmrppArray &rhs) :
                Array(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppArray &
DmrppArray::operator=(const DmrppArray &rhs)
{
    if (this == &rhs) return *this;

    dynamic_cast<Array &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

/**
 * @brief Is this Array subset?
 * @return True if the array has a projection expression, false otherwise
 */
bool DmrppArray::is_projected()
{
    for (Dim_iter p = dim_begin(), e = dim_end(); p != e; ++p)
        if (dimension_size(p, true) != dimension_size(p, false)) return true;

    return false;
}

/**
 * @brief Compute the index of the address_in_target for an an array of target_shape.
 *
 * Internally we store N-dimensional arrays using a one dimensional array (i.e., a
 * vector). Compute the offset in that vector that corresponds to a location in
 * the N-dimensional array. For example, for three dimensional array of size
 * 2 x 3 x 4, the data values are stored in a 24 element vector and the item at
 * location 1,1,1 (zero-based indexing) would be at offset 1*1 + 1*4 + 1 * 4*3 == 15.
 *
 * @param address_in_target N-tuple zero-based index of an element in N-space
 * @param target_shape N-tuple of the array's dimension sizes.
 * @return The offset into the vector used to store the values.
 */
unsigned long long get_index(const vector<unsigned int> &address_in_target, const vector<unsigned int> &target_shape)
{
#if 0
    if (address_in_target.size() != target_shape.size()) {
        ostringstream oss;
        oss << "The target_shape  (size: " << target_shape.size() << ")" << " and the address_in_target (size: "
            << address_in_target.size() << ")" << " have different dimensionality.";
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    unsigned long long digit_multiplier = 1;
    unsigned long long subject_index = 0;
    for (int i = target_shape.size() - 1; i >= 0; i--) {
        if (address_in_target[i] >= target_shape[i]) {      // Changes > to >= size we use zero-based indexing
            ostringstream oss;
            oss << "The address_in_target[" << i << "]: " << address_in_target[i] << " is larger than target_shape["
                << i << "]: " << target_shape[i] << " This will make the bad things happen.";
            throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
        }
        subject_index += address_in_target[i] * digit_multiplier;
        digit_multiplier *= target_shape[i];
    }
#endif

    assert(address_in_target.size() == target_shape.size());    // ranks must be equal

    vector<unsigned int>::const_reverse_iterator shape_index = target_shape.rbegin();
    vector<unsigned int>::const_reverse_iterator index = address_in_target.rbegin(), index_end = address_in_target.rend();

    unsigned long long multiplier = *shape_index++;
    unsigned long long offset = *index++;

    while (index != index_end) {
        assert(*index < *shape_index); // index < shape for each dim

        offset += multiplier * *index++;
        multiplier *= *shape_index++;   // TODO Remove the unneeded multiply. jhrg 3/24/17
    }

    return offset;
}

vector<unsigned int> DmrppArray::get_shape(bool constrained)
{
    vector<unsigned int> array_shape;
    for (Dim_iter dim = dim_begin(); dim != dim_end(); dim++) {
        array_shape.push_back(dimension_size(dim, constrained));
    }

    return array_shape;
}

/**
 * @brief Return the ith dimension object for this Array
 * @param i The index of the dimension object to return
 * @return The dimension object
 */
DmrppArray::dimension DmrppArray::get_dimension(unsigned int i)
{
    assert(i <= (dim_end() - dim_begin()));
    return *(dim_begin() + i);

#if 0
    Dim_iter dimIter = dim_begin();
    unsigned int dim_index = 0;

    while (dimIter != dim_end()) {
        if (dim_num == dim_index) return *dimIter;
        dimIter++;
        dim_index++;
    }
    ostringstream oss;
    oss << "DmrppArray::get_dimension() -" << " The array " << name() << " does not have " << dim_num << " dimensions!";
    throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
#endif
}

/**
 * @brief This recursive private method collects values from the rbuf and copies
 * them into buf. It supports stop, stride, and start and while correct is not
 * efficient.
 */
void DmrppArray::insert_constrained_no_chunk(Dim_iter dimIter, unsigned long *target_index,
    vector<unsigned int> &subsetAddress, const vector<unsigned int> &array_shape, H4ByteStream *h4bytestream)
{
    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - subsetAddress.size(): " << subsetAddress.size() << endl);

    unsigned int bytesPerElt = prototype()->width();
    char *sourceBuf = h4bytestream->get_rbuf();
    char *targetBuf = get_buf();

    unsigned int start = this->dimension_start(dimIter);
    unsigned int stop = this->dimension_stop(dimIter, true);
    unsigned int stride = this->dimension_stride(dimIter, true);
    BESDEBUG("dmrpp",
        "DmrppArray::"<< __func__ << "() - start: " << start << " stride: " << stride << " stop: " << stop << endl);

    dimIter++;

    // This is the end case for the recursion.
    // TODO stride == 1 belongs inside this or else rewrite this as if else if else
    // see below.
    if (dimIter == dim_end() && stride == 1) {
        BESDEBUG("dmrpp",
            "DmrppArray::"<< __func__ << "() - stride is 1, copying from all values from start to stop." << endl);

        subsetAddress.push_back(start);
        unsigned int start_index = get_index(subsetAddress, array_shape);
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - start_index: " << start_index << endl);
        subsetAddress.pop_back();

        subsetAddress.push_back(stop);
        unsigned int stop_index = get_index(subsetAddress, array_shape);
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - stop_index: " << start_index << endl);
        subsetAddress.pop_back();

        // Copy data block from start_index to stop_index
        // FIXME Replace this loop with a call to std::memcpy()
        for (unsigned int sourceIndex = start_index; sourceIndex <= stop_index; sourceIndex++, target_index++) {
            unsigned long target_byte = *target_index * bytesPerElt;
            unsigned long source_byte = sourceIndex * bytesPerElt;
            // Copy a single value.
            for (unsigned int i = 0; i < bytesPerElt; i++) {
                targetBuf[target_byte++] = sourceBuf[source_byte++];
            }
            (*target_index)++;
        }
    }
    else {
        for (unsigned int myDimIndex = start; myDimIndex <= stop; myDimIndex += stride) {
            // Is it the last dimension?
            if (dimIter != dim_end()) {
                // Nope!
                // then we recurse to the last dimension to read stuff
                subsetAddress.push_back(myDimIndex);
                insert_constrained_no_chunk(dimIter, target_index, subsetAddress, array_shape, h4bytestream);
                subsetAddress.pop_back();
            }
            else {
                // We are at the last (inner most) dimension.
                // So it's time to copy values.
                subsetAddress.push_back(myDimIndex);
                unsigned int sourceIndex = get_index(subsetAddress, array_shape);
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ << "() - " "Copying source value at sourceIndex: " << sourceIndex << endl);
                subsetAddress.pop_back();
                // Copy a single value.
                unsigned long target_byte = *target_index * bytesPerElt;
                unsigned long source_byte = sourceIndex * bytesPerElt;

                // FIXME Replace this loop with a call to std::memcpy()
                for (unsigned int i = 0; i < bytesPerElt; i++) {
                    targetBuf[target_byte++] = sourceBuf[source_byte++];
                }
                (*target_index)++;
            }
        }
    }
}

/**
 * @brief Return the total number of elements in this Array
 * @param constrained If true, use the constrained size of the array,
 * otherwise use the full size.
 * @return The number of elements in this Array
 */
unsigned long long DmrppArray::get_size(bool constrained)
{
    // number of array elements in the constrained array
    unsigned long long constrained_size = 1;
    for (Dim_iter dim = dim_begin(), end = dim_end(); dim != end; dim++) {
        constrained_size *= dimension_size(dim, constrained);
    }
    return constrained_size;
}

/**
 * @brief Read an array that is stored with using two or more 'chunks.'
 * @return Always returns true, matching the libdap::Array::read() behavior.
 */
bool DmrppArray::read_no_chunks()
{
    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for " << name() << " BEGIN" << endl);

    vector<H4ByteStream> *chunk_refs = get_chunk_vec();
    if (chunk_refs->size() == 0) {
        ostringstream oss;
        oss << "DmrppArray::" << __func__ << "() - Unable to obtain a ByteStream object for array " << name()
                        << " Without a ByteStream we cannot read!";
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    // For now we only handle the one chunk case.
    H4ByteStream h4_byte_stream = (*chunk_refs)[0];
    h4_byte_stream.read(); // Use the default values for deflate (false) and chunk size (0)

    if (!is_projected()) {      // if there is no projection constraint
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - No projection, copying all values into array. " << endl);
        val2buf(h4_byte_stream.get_rbuf());    // yes, it's not type-safe
    }
    else {
        vector<unsigned int> array_shape = get_shape(false);
        unsigned long long constrained_size = get_size(true);

        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - constrained_size:  " << constrained_size << endl);

        reserve_value_capacity(constrained_size);
        unsigned long target_index = 0;
        vector<unsigned int> subset;
        insert_constrained_no_chunk(dim_begin(), &target_index, subset, array_shape, &h4_byte_stream);
    }

    set_read_p(true);

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for " << name() << " END"<< endl);

    return true;
}

/**
 * Reads a the chunks that make up this array's content and copies just the
 * relevant values into the array's memory buffer.
 *
 * Currently, this will collect a curl_easy handle for each chunk required
 * by the current constraint (might be all of them). The handles are placed in
 * a curl_multi handle. Once collected the curl_multi handle is "run"
 * until everything has been completely retrieved or has erred. With the chunks
 * read and in memory the code then initiates a copy of the results into the
 * array variable's internal buffer.
 */
bool DmrppArray::read_chunks()
{
    BESDEBUG("dmrpp", __FUNCTION__ << " for variable '" << name() << "' - BEGIN" << endl);

    vector<H4ByteStream> *chunk_refs = get_chunk_vec();
    if (chunk_refs->size() == 0) {
        ostringstream oss;
        oss << "DmrppArray::" << __func__ << "() - Unable to obtain a byteStream object for array " << name()
                        << " Without a byteStream we cannot read! " << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }
    // Allocate target memory.
    // FIXME - I think this needs to be the constrained size!
    reserve_value_capacity(length());
    vector<unsigned int> array_shape = get_shape(false);
    BESDEBUG("dmrpp",
        "DmrppArray::"<< __func__ <<"() - dimensions(): " << dimensions(false) << " array_shape.size(): " << array_shape.size() << endl);

    if (this->dimensions(false) != array_shape.size()) {
        ostringstream oss;
        oss << "DmrppArray::" << __func__ << "() - array_shape does not match the number of array dimensions! " << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    BESDEBUG("dmrpp",
        "DmrppArray::"<< __func__ << "() - "<< dimensions() << "D Array. Processing " << chunk_refs->size() << " chunks" << endl);

    /* get a curl_multi handle */
    CURLM *curl_multi_handle = curl_multi_init();

    /*
     * Find the chunks to be read, make curl_easy handles for them, and
     * stuff them into our curl_multi handle. This is a recursive activity
     * which utilizes the same code that copies the data from the chunk to
     * the variables.
     */
    for (unsigned long i = 0; i < chunk_refs->size(); i++) {
        H4ByteStream *h4bs = &(*chunk_refs)[i];
        BESDEBUG("dmrpp",
            "DmrppArray::" << __func__ <<"(): BEGIN Processing chunk[" << i << "]: " << h4bs->to_string() << endl);
        vector<unsigned int> target_element_address = h4bs->get_position_in_array();
        vector<unsigned int> chunk_source_address(dimensions(), 0);
        // Recursive insertion operation.
        bool flag = insert_constrained_chunk(0, &target_element_address, &chunk_source_address, h4bs, curl_multi_handle);
        BESDEBUG("dmrpp",
            "DmrppArray::" << __func__ <<"(): END Processing chunk[" << i << "]  "
                "(chunk was " << (h4bs->is_started()?"QUEUED":"NOT_QUEUED") <<
                " and " << (h4bs->is_read()?"READ":"NOT_READ") << ") flag: "<< flag << endl);
    }

    /*
     * Now that we have all of the curl_easy handles for all the chunks of this array
     * that we need to read in our curl_multi handle
     * we dive into multi_finish() to get all of the chunks read.
     */
    multi_finish(curl_multi_handle, chunk_refs);

    /*
     * The chunks are all read, so we jump back into the recursive code to copy the
     * correct values out of each chunk and into the array memory.
     */
    for (unsigned long i = 0; i < chunk_refs->size(); i++) {
        H4ByteStream *h4bs = &(*chunk_refs)[i];
        BESDEBUG("dmrpp",
            "DmrppArray::" << __func__ <<"(): BEGIN Processing chunk[" << i << "]: " << h4bs->to_string() << endl);
        vector<unsigned int> target_element_address = h4bs->get_position_in_array();
        vector<unsigned int> chunk_source_address(dimensions(), 0);
        // Recursive insertion operation.
        bool flag = insert_constrained_chunk(0, &target_element_address, &chunk_source_address, h4bs, 0);
        BESDEBUG("dmrpp",
            "DmrppArray::" << __func__ <<"(): END Processing chunk[" << i << "]  (chunk was " << (h4bs->is_read()?"READ":"SKIPPED") << ") flag: "<< flag << endl);
    }
    //##############################################################################

    return true;
}

/**
 * This helper method reads completely all of the curl_easy handles in the multi_handle.
 *
 * This means that we are reading some or all of the chunks and the chunk vector is
 * passed in so that the curl_easy handle held in each H4ByteStream that was read can be
 * cleaned up once the request has been completed.
 *
 * Once this method is completed we will be ready to copy all of the data from the
 * chunks to the array memory
 */
void DmrppArray::multi_finish(CURLM *multi_handle, vector<H4ByteStream> *chunk_refs)
{
    BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() BEGIN" << endl);

    int still_running;
    int repeats;
    long long lap_counter = 0;  // TODO Remove or ... see below
    CURLMcode mcode;

    do {
        int numfds;

        lap_counter++;        // TODO make this depend on BESDEBG if we really need it
        BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() Calling curl_multi_perform()" << endl);
        // Read from one or more handles and get the number 'still running'.
        // This returns when there's currently no more to read
        mcode = curl_multi_perform(multi_handle, &still_running);
        BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() Completed curl_multi_perform() mcode: " << mcode << endl);

        if (mcode == CURLM_OK) {
            /* wait for activity, timeout or "nothing" */
            BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() Calling curl_multi_wait()" << endl);
            // Block until one or more handles have new data to be read or until a timer expires.
            // The timer is set to 1000 milliseconds. Return the numer of handles ready for reading.
            mcode = curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
            BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() Completed curl_multi_wait() mcode: " << mcode << endl);
        }

        // TODO Can cmode be anything other than CURLM_OK?
        // TODO Move this to an else clause and maybe add a note that the error is handled below
        // TODO Actually, it would be clearer to throw here...
        if (mcode != CURLM_OK) {
            break;
        }

        // TODO I don't get the point of this loop... I see it in the docs, but I don't see why it's needed

        /* 'numfds' being zero means either a timeout or no file descriptors to
         wait for. Try timeout on first occurrence, then assume no file
         descriptors and no file descriptors to wait for means wait for 100
         milliseconds. */

        if (!numfds) {
            repeats++; /* count number of repeated zero numfds */
            if (repeats > 1) {
                /* sleep 100 milliseconds */
                usleep(100 * 1000);   // usleep takes sleep time in us (1 millionth of a second)
            }
        }
        else
            repeats = 0;

    } while (still_running);

    BESDEBUG("dmrpp",
        "DmrppArray::" << __func__ <<"() CURL-MULTI has finished! laps: " << lap_counter << "  still_running: "<< still_running << endl);

    if (mcode == CURLM_OK) {
        CURLMsg *msg; /* for picking up messages with the transfer status */
        int msgs_left; /* how many messages are left */

        /* See how the transfers went */
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            int found = 0;
            string h4bs_str = "No Chunk Found For Handle!";
            /* Find out which handle this message is about */
            for (unsigned int idx = 0; idx < chunk_refs->size(); idx++) {
                H4ByteStream *this_h4bs = &(*chunk_refs)[idx];

                CURL *curl_handle = this_h4bs->get_curl_handle();
                found = (msg->easy_handle == curl_handle);
                if (found) {
                    //this_h4bs->set_is_read(true);
                    h4bs_str = this_h4bs->to_string();
                    break;
                }
            }

            if (msg->msg == CURLMSG_DONE) {
                BESDEBUG("dmrpp",
                    "DmrppArray::" << __func__ <<"() Chunk Read Completed For Chunk: " << h4bs_str << endl);
            }
            else {
                ostringstream oss;
                oss << "DmrppArray::" << __func__ << "() Chunk Read Did Not Complete. CURLMsg.msg: " << msg->msg
                    << " Chunk: " << h4bs_str;
                BESDEBUG("dmrpp", oss.str() << endl);
                throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
            }
        }

    }

    /* Free the CURL handles */
    for (unsigned int idx = 0; idx < chunk_refs->size(); idx++) {
        CURL *easy_handle = (*chunk_refs)[idx].get_curl_handle();
        curl_multi_remove_handle(multi_handle, easy_handle);
        (*chunk_refs)[idx].cleanup_curl_handle();
    }

    curl_multi_cleanup(multi_handle);

    if (mcode != CURLM_OK) {
        ostringstream oss;
        oss << "DmrppArray: CURL operation Failed!. multi_code: " << mcode << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() END" << endl);
}


/**
 * @brief This recursive call inserts a (previously read) chunk's data into the
 * appropriate parts of the Array object's internal memory.
 *
 * Successive calls climb into the array to the insertion point for the current
 * chunk's innermost row. Once located, this row is copied into the array at the
 * insertion point. The next row for insertion is located by returning from the
 * insertion call to the next dimension iteration in the call recursive call
 * stack.
 *
 * This starts with dimension 0 and the chunk_row_insertion_point_address set
 * to the chunks origin point
 *
 * @param dim is the dimension on which we are working. We recurse from
 * dimension 0 to the last dimension
 * @param target_element_address - This vector is used to hold the element
 * address in the result array to where this chunk's data will be written.
 * @param chunk_source_address - This vector is used to hold the chunk
 * element address from where data will be read. The values of this are relative to
 * the chunk's origin (position in array).
 * @param chunk The H4ByteStream containing the read data values to insert.
 * @return
 */
bool DmrppArray::insert_constrained_chunk(unsigned int dim, vector<unsigned int> *target_element_address,
    vector<unsigned int> *chunk_source_address, H4ByteStream *chunk, CURLM *multi_handle)
{

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - dim: "<< dim << " BEGIN "<< endl);

    // The size, in elements, of each of the chunk's dimensions.
    // TODO We assume all chunks have the same size for any given array.
    vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();

    // The array index of the last dimension
    unsigned int last_dim = chunk_shape.size() - 1;

    // The chunk's origin point a.k.a. its "position in array".
    vector<unsigned int> chunk_origin = chunk->get_position_in_array();

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Retrieving dimension "<< dim << endl);

    dimension thisDim = this->get_dimension(dim);

    BESDEBUG("dmrpp",
        "DmrppArray::"<< __func__ <<"() - thisDim: "<< thisDim.name << " start " << thisDim.start << " stride " << thisDim.stride << " stop " << thisDim.stop << endl);

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned int first_element_offset = 0; // start with 0
    if ((unsigned) thisDim.start < chunk_origin[dim]) {
        // If the start is behind this chunk, then it's special.
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - dim: "<<dim << " thisDim.start: " << thisDim.start << endl);
        if (thisDim.stride != 1) { // And if the stride isn't 1,
            // we have to figure our where to begin in this chunk.
            first_element_offset = (chunk_origin[dim] - thisDim.start) % thisDim.stride;
            // If it's zero great!
            if (first_element_offset != 0) {
                // otherwise we adjustment to get correct first element.
                first_element_offset = thisDim.stride - first_element_offset;
            }
        }
        BESDEBUG("dmrpp",
            "DmrppArray::"<< __func__ <<"() - dim: "<< dim << " first_element_offset: " << first_element_offset << endl);
    }
    else {
        first_element_offset = thisDim.start - chunk_origin[dim];
        BESDEBUG("dmrpp",
            "DmrppArray::"<< __func__ <<"() - dim: "<< dim << " thisDim.start is beyond the chunk origin at this dim. first_element_offset: " << first_element_offset << endl);
    }

    // Is the next point to be sent in this chunk at all?
    if (first_element_offset > chunk_shape[dim]) {
        // Nope! Time to bail
        return false;
    }

    unsigned long long start_element = chunk_origin[dim] + first_element_offset;
    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - dim: "<< dim << " start_element: " << start_element << endl);

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - dim: "<< dim << " thisDim.stop is in this chunk. " << endl);
    }
    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - dim: "<< dim << " end_element: " << end_element << endl);

    // Do we even want this chunk?
    if ((unsigned) thisDim.start > (chunk_origin[dim] + chunk_shape[dim])
        || (unsigned) thisDim.stop < chunk_origin[dim]) {
        // No. No, we do not. Skip this.
        BESDEBUG("dmrpp",
            "DmrppArray::"<< __func__ <<"() - dim: " << dim << " Chunk not accessed by CE. SKIPPING." << endl);
        return false ;
    }

    unsigned long long chunk_start = start_element - chunk_origin[dim];
    unsigned long long chunk_end = end_element - chunk_origin[dim];

    if (dim == last_dim) {
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - dim: "<< dim << " THIS IS THE INNER-MOST DIM. "<< endl);
        if(multi_handle){
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Queuing chunk for retrieval: " << chunk->to_string() << endl);
             chunk->add_to_multi_read_queue(multi_handle);
             return true;

        }
        else {
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Reading " << chunk->to_string() << endl);

            // Read and Process chunk
            chunk->read(is_deflate_compression(), get_chunk_size_in_elements() * var()->width(),
                is_shuffle_compression(), var()->width());
            char * source_buffer = chunk->get_rbuf();

            if (thisDim.stride == 1) {
                //#############################################################################
                // ND - inner_stride == 1

                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - dim: " << dim << " The stride is 1." << endl);

                // Compute how much we are going to copy
                unsigned long long chunk_constrained_inner_dim_elements = end_element - start_element + 1;
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ <<"() - dim: " << dim << " chunk_constrained_inner_dim_elements: " << chunk_constrained_inner_dim_elements << endl);

                unsigned long long chunk_constrained_inner_dim_bytes = chunk_constrained_inner_dim_elements
                    * prototype()->width();
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ <<"() - dim: " << dim << " chunk_constrained_inner_dim_bytes: " << chunk_constrained_inner_dim_bytes << endl);

                // Compute where we need to put it.
                (*target_element_address)[dim] = (start_element - thisDim.start) / thisDim.stride;
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ <<"() - dim: " << dim << " target_element_address: " << vec2str(*target_element_address) << endl);

                unsigned int target_start_element_index = get_index(*target_element_address, get_shape(true));
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ <<"() - dim: " << dim << " target_start_element_index: " << target_start_element_index << endl);

                unsigned int target_char_start_index = target_start_element_index * prototype()->width();
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ <<"() - dim: " << dim << " target_char_start_index: " << target_char_start_index << endl);

                // Compute where we are going to read it from
                (*chunk_source_address)[dim] = first_element_offset;
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ <<"() - dim: " << dim << " chunk_source_address: " << vec2str(*chunk_source_address) << endl);

                unsigned int chunk_start_element_index = get_index(*chunk_source_address, chunk_shape);
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ <<"() - dim: " << dim << " chunk_start_element_index: " << chunk_start_element_index << endl);

                unsigned int chunk_char_start_index = chunk_start_element_index * prototype()->width();
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ <<"() - dim: " << dim << " chunk_char_start_index: " << chunk_char_start_index << endl);

                char *target_buffer = get_buf();

                // Copy the bytes
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ <<"() - dim: " << dim << " Using memcpy to transfer " << chunk_constrained_inner_dim_bytes << " bytes." << endl);
                memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index,
                    chunk_constrained_inner_dim_bytes);
            }
            else {
                //#############################################################################
                // inner_stride != 1
                unsigned long long vals_in_chunk = 1 + (end_element - start_element) / thisDim.stride;
                BESDEBUG("dmrpp",
                    "DmrppArray::"<< __func__ <<"() - dim: "<<dim<<" InnerMostStride is equal to " << thisDim.stride << ". Copying " << vals_in_chunk << " individual values." << endl);

                unsigned long long chunk_start = start_element - chunk_origin[dim];
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - ichunk_start: " << chunk_start << endl);

                unsigned long long chunk_end = end_element - chunk_origin[dim];
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_end: " << chunk_end << endl);

                for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() --------- idim_index: " << chunk_index << endl);

                    // Compute where we need to put it.
                    (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;
                    BESDEBUG("dmrpp",
                        "DmrppArray::"<< __func__ <<"() - target_element_address: " << vec2str(*target_element_address) << endl);

                    unsigned int target_start_element_index = get_index(*target_element_address, get_shape(true));
                    BESDEBUG("dmrpp",
                        "DmrppArray::"<< __func__ <<"() - target_start_element_index: " << target_start_element_index << endl);

                    unsigned int target_char_start_index = target_start_element_index * prototype()->width();
                    BESDEBUG("dmrpp",
                        "DmrppArray::"<< __func__ <<"() - target_char_start_index: " << target_char_start_index << endl);

                    // Compute where we are going to read it from
                    (*chunk_source_address)[dim] = chunk_index;
                    BESDEBUG("dmrpp",
                        "DmrppArray::"<< __func__ <<"() - chunk_source_address: " << vec2str(*chunk_source_address) << endl);

                    unsigned int chunk_start_element_index = get_index(*chunk_source_address, chunk_shape);
                    BESDEBUG("dmrpp",
                        "DmrppArray::"<< __func__ <<"() - chunk_start_element_index: " << chunk_start_element_index << endl);

                    unsigned int chunk_char_start_index = chunk_start_element_index * prototype()->width();
                    BESDEBUG("dmrpp",
                        "DmrppArray::"<< __func__ <<"() - chunk_char_start_index: " << chunk_char_start_index << endl);

                    char *target_buffer = get_buf();

                    // Copy the bytes
                    BESDEBUG("dmrpp",
                        "DmrppArray::"<< __func__ <<"() - Using memcpy to transfer " << prototype()->width() << " bytes." << endl);
                    memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index,
                        prototype()->width());
                }
            }
        }
    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int dim_index = chunk_start; dim_index <= chunk_end; dim_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_origin[dim] + dim_index - thisDim.start) / thisDim.stride;
            (*chunk_source_address)[dim] = dim_index;

            BESDEBUG("dmrpp",
                "DmrppArray::" << __func__ << "() - RECURSION STEP - " << "Departing dim: " << dim << " dim_index: " << dim_index << " target_element_address: " << vec2str((*target_element_address)) << " chunk_source_address: " << vec2str((*chunk_source_address)) << endl);

            // Re-entry here:
            bool flag = insert_constrained_chunk(dim + 1, target_element_address, chunk_source_address, chunk, multi_handle);
            if(flag)
                return true;
        }
    }
    return false;
}

/**
 * Reads chunked array data from the relevant sources (as indicated by each
 * H4ByteStream object) for this array.
 */
bool DmrppArray::read()
{
    if (read_p()) return true;

    // IF the variable is not chunked then go read it.
    if (get_chunk_dimension_sizes().empty()) {
        if (get_immutable_chunks().size() == 1) {
            // This handles the case for arrays that have exactly one h4:byteStream
            return read_no_chunks();
        }
        else {
            ostringstream oss;
            oss << "DmrppArray: Unchunked arrays must have exactly one H4ByteStream object. "
                "This one has " << get_immutable_chunks().size() << endl;
            throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
        }
    }
    else {
        // Handle the more complex case where the data is chunked.
        return read_chunks();
    }
}

void DmrppArray::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppArray::" << __func__ << "(" << (void *) this << ")" << endl;
    DapIndent::Indent();
    DmrppCommon::dump(strm);
    Array::dump(strm);
    strm << DapIndent::LMarg << "value: " << "----" << /*d_buf <<*/endl;
    DapIndent::UnIndent();
}

} // namespace dmrpp
