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
#include <vector>
#include <memory>
#include <queue>
#include <iterator>
#include <thread>
#include <future>         // std::async, std::future
#include <chrono>         // std::chrono::milliseconds

#include <cstring>
#include <cassert>
#include <cerrno>

#include <pthread.h>
#include <cmath>

#include <unistd.h>

#include <D4Enum.h>
#include <D4Attributes.h>
#include <D4Maps.h>
#include <D4Group.h>

#include "BESInternalError.h"
#include "BESDebug.h"
#include "BESLog.h"
#include "BESStopWatch.h"

#include "byteswap_compat.h"
#include "CurlHandlePool.h"
#include "Chunk.h"
#include "DmrppArray.h"
#include "DmrppRequestHandler.h"
#include "DmrppNames.h"
#include "Base64.h"

// Used with BESDEBUG
#define dmrpp_3 "dmrpp:3"
#define dmrpp_4 "dmrpp:4"

using namespace libdap;
using namespace std;

#define MB (1024*1024)
#define prolog std::string("DmrppArray::").append(__func__).append("() - ")

namespace dmrpp {


// Transfer Thread Pool state variables.
std::mutex transfer_thread_pool_mtx;     // mutex for critical section
atomic_uint transfer_thread_counter(0);



/**
 * @brief Uses future::wait_for() to scan the futures for a ready future, returning true when once get() has been called.
 *
 * When a valid, ready future is found future::get() is called and the thead_counter is decremented.
 * Returns true when it successfully "get"s a future (joins a thread), or if a future turns up as not valid, then
 * it is discarded and the thread is ":finsihed" and true is returned.
 *
 * @param futures The list of futures to scan
 * @param thread_counter This counter will be decremented when a future is "finished".
 * @param timeout The number of milliseconds to wait for each future to complete.
 * @param debug_prefix This is a string tag used in debugging to associate the particular debug call with the calling
 * method.
 * @return Returns true if future::get() was called on a ready future, false otherwise.
 */
bool get_next_future(list<std::future<bool>> &futures, atomic_uint &thread_counter, unsigned long timeout, string debug_prefix) {
    bool future_finished = false;
    bool done = false;
    std::chrono::milliseconds timeout_ms (timeout);

    while(!done){
        auto futr = futures.begin();
        auto fend = futures.end();
        bool future_is_valid = true;
        while(!future_finished && future_is_valid && futr != fend){
            future_is_valid = (*futr).valid();
            if(future_is_valid){
                // What happens if wait_for() always returns future_status::timeout for a stuck thread?
                // If that were to happen, the loop would run forever. However, we assume that these
                // threads are never 'stuck.' We assume that their computations always complete, either
                // with success or failure. For the transfer threads, timeouts will stop them if nothing
                // else does and for the decompression threads, the worst case is a segmentation fault.
                // jhrg 2/5/21
                if((*futr).wait_for(timeout_ms) != std::future_status::timeout){
                    try {
                        bool success = (*futr).get();
                        future_finished = true;
                        BESDEBUG(dmrpp_3, debug_prefix << prolog << "Called future::get() on a ready future."
                            << " success: " << (success?"true":"false") << endl);
                        if(!success){
                            stringstream msg;
                            msg << debug_prefix << prolog << "The std::future has failed!";
                            msg << " thread_counter: " << thread_counter;
                            throw BESInternalError(msg.str(), __FILE__, __LINE__);
                        }
                    }
                    catch(...){
                        // TODO I had to add this to make the thread counting work when there's errors
                        //  But I think it's primitive because it trashes everything - there's
                        //  surely a way to handle the situation on a per thread basis and maybe even
                        //  retry?
                        futures.clear();
                        thread_counter=0;
                        throw;
                    }
                }
                else {
                    futr++;
                    BESDEBUG(dmrpp_3, debug_prefix << prolog << "future::wait_for() timed out. (timeout: " <<
                        timeout << " ms) There are currently " << futures.size() << " futures in process."
                        << " thread_counter: " << thread_counter << endl);
                }
            }
            else {
                BESDEBUG(dmrpp_3, debug_prefix << prolog << "The future was not valid. Dumping... " << endl);
                future_finished = true;
            }
        }
        if (futr!=fend && future_finished) {
            futures.erase(futr);
            thread_counter--;
            BESDEBUG(dmrpp_3, debug_prefix << prolog << "Erased future from futures list. (Erased future was "
                                  << (future_is_valid?"":"not ") << "valid at start.) There are currently " <<
                                  futures.size() << " futures in process. thread_counter: " << thread_counter << endl);
        }
        done = future_finished || futures.empty();
    }
    return future_finished;
}



/**
 * @brief Manage parallel transfer for contiguous data
 *
 * Read data for one of the 'child chunks' made to read data for a variable
 * with contiguous storage in parallel.
 *
 * This is only used for threads started by read_contiguous().
 *
 * @param arg_list A pointer to a one_child_chunk_args
 */
bool one_child_chunk_thread_new(unique_ptr<one_child_chunk_args_new> args)
{

    args->child_chunk->read_chunk();

    assert(args->the_one_chunk->get_rbuf());
    assert(args->child_chunk->get_rbuf());
    assert(args->child_chunk->get_bytes_read() == args->child_chunk->get_size());

    // the_one_chunk offset \/
    // the_one_chunk:  mmmmmmmmmmmmmmmm
    // child chunks:   1111222233334444 (there are four child chunks)
    // child offsets:  ^   ^   ^   ^
    // For this example, child_1_offset - the_one_chunk_offset == 0 (that's always true)
    // child_2_offset - the_one_chunk_offset == 4; child_2_offset - the_one_chunk_offset == 8
    // and child_3_offset - the_one_chunk_offset == 12.
    // Those are the starting locations with in the data buffer of the the_one_chunk
    // where that child chunk should be written.
    // Note: all of the offset values start at the beginning of the file.

    unsigned long long  offset_within_the_one_chunk = args->child_chunk->get_offset() - args->the_one_chunk->get_offset();

    memcpy(args->the_one_chunk->get_rbuf() + offset_within_the_one_chunk, args->child_chunk->get_rbuf(),
           args->child_chunk->get_bytes_read());

    return true;
}



/**
 * @brief A single argument wrapper for process_super_chunk() for use with std::async().
 * @param args A unique_ptr to an instance of one_super_chunk_args.
 * @return True unless an exception is throw in which case neither true or false apply.
 */
bool one_super_chunk_transfer_thread(unique_ptr<one_super_chunk_args> args)
{

#if DMRPP_ENABLE_THREAD_TIMERS
    stringstream timer_tag;
    timer_tag << prolog << "tid: 0x" << std::hex << std::this_thread::get_id() <<
    " parent_tid: 0x" << std::hex << args->parent_thread_id << " sc_id: " << args->super_chunk->id();
    BESStopWatch sw(TRANSFER_THREADS);
    sw.start(timer_tag.str());
#endif

    args->super_chunk->read();
    return true;
}

/**
 * @brief A single argument wrapper for process_super_chunk_unconstrained() for use with std::async().
 * @param args A unique_ptr to an instance of one_super_chunk_args.
 * @return True unless an exception is throw in which case neither true or false apply.
 */
bool one_super_chunk_unconstrained_transfer_thread(unique_ptr<one_super_chunk_args> args)
{

#if DMRPP_ENABLE_THREAD_TIMERS
    stringstream timer_tag;
    timer_tag << prolog << "tid: 0x" << std::hex << std::this_thread::get_id() <<
    " parent_tid: 0x" << std::hex << args->parent_thread_id  << " sc_id: " << args->super_chunk->id();
    BESStopWatch sw(TRANSFER_THREADS);
    sw.start(timer_tag.str());
#endif

    args->super_chunk->read_unconstrained();
    return true;
}


bool start_one_child_chunk_thread(list<std::future<bool>> &futures, unique_ptr<one_child_chunk_args_new> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck (transfer_thread_pool_mtx);
    if (transfer_thread_counter < DmrppRequestHandler::d_max_transfer_threads) {
        transfer_thread_counter++;
        futures.push_back( std::async(std::launch::async, one_child_chunk_thread_new, std::move(args)));
        retval = true;
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() <<
                                 "' from std::async for " << args->child_chunk->to_string() << endl);
    }
    return retval;
}


/**
 * @brief Starts the super_chunk_thread function using std::async() and places the returned future in the queue futures.
 * @param futures The queue into which to place the std::future returned by std::async().
 * @param args The arguments for the super_chunk_thread function
 * @return Returns true if the std::async() call was made and a future was returned, false if the
 * transfer_thread_counter has reached the maximum allowable size.
 */
bool start_super_chunk_transfer_thread(list<std::future<bool>> &futures, unique_ptr<one_super_chunk_args> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck (transfer_thread_pool_mtx);
    if (transfer_thread_counter < DmrppRequestHandler::d_max_transfer_threads) {
        transfer_thread_counter++;
        futures.push_back(std::async(std::launch::async, one_super_chunk_transfer_thread, std::move(args)));
        retval = true;
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() <<
                                            "' from std::async for " << args->super_chunk->to_string(false) << endl);
    }
    return retval;
}

/**
 * @brief Starts the one_super_chunk_unconstrained_transfer_thread function using std::async() and places the returned future in the queue futures.
 * @param futures The queue into which to place the future returned by std::async().
 * @param args The arguments for the super_chunk_thread function
 * @return Returns true if the async call was made and a future was returned, false if the transfer_thread_counter has
 * reached the maximum allowable size.
 */
bool start_super_chunk_unconstrained_transfer_thread(list<std::future<bool>> &futures, unique_ptr<one_super_chunk_args> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck (transfer_thread_pool_mtx);
    if(transfer_thread_counter < DmrppRequestHandler::d_max_transfer_threads) {
        transfer_thread_counter++;
        futures.push_back(std::async(std::launch::async, one_super_chunk_unconstrained_transfer_thread, std::move(args)));
        retval = true;
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() <<
                                            "' from std::async, transfer_thread_counter: " << transfer_thread_counter << endl);
    }
    return retval;
}


/**
 * @brief Uses std::async() and std::future to process the SuperChunks in super_chunks into the DmrppArray array.
 *
 * For each SuperChunk in the queue, retrive the chunked data by using std::async() to generate a std::future which will
 * perform the data retrieval and subsequent computational steps(inflate/shuffle/etc) and finally insertion into the
 * DmrppArray's internal data buffer.
 *
 * NOTE: There are 4 variants of this function:
 *
 *  - process_chunks_concurrent()
 *  - process_chunks_unconstrained_concurrent()
 *  - read_super_chunks_concurrent()
 *  - read_super_chunks_unconstrained_concurrent
 *
 *  If structural/algorithmic changes need to be made to this function it is almost certain that similar changes will
 *  be required in the other 3 functions.
 *
 * @param super_chunks The queue of SuperChunk objects to process.
 * @param array The DmrppArray into which the chunk data will be placed.
 */
void read_super_chunks_unconstrained_concurrent(queue<shared_ptr<SuperChunk>> &super_chunks, DmrppArray *array)
{
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + " name: "+array->name(), "");

    // Parallel version based on read_chunks_unconstrained(). There is
    // substantial duplication of the code in read_chunks_unconstrained(), but
    // wait to remove that when we move to C++11 which has threads integrated.

    // We maintain a list  of futures to track our parallel activities.
    list<future<bool>> futures;
    try {
        bool done = false;
        bool future_finished = true;
        while (!done) {

            if(!futures.empty())
                future_finished = get_next_future(futures, transfer_thread_counter, DMRPP_WAIT_FOR_FUTURE_MS, prolog);

            // If future_finished is true this means that the chunk_processing_thread_counter has been decremented,
            // because future::get() was called or a call to future::valid() returned false.
            BESDEBUG(dmrpp_3, prolog << "future_finished: " << (future_finished ? "true" : "false") << endl);

            if (!super_chunks.empty()){
                // Next we try to add a new Chunk compute thread if we can - there might be room.
                bool thread_started = true;
                while(thread_started && !super_chunks.empty()) {
                    auto super_chunk = super_chunks.front();
                    BESDEBUG(dmrpp_3, prolog << "Starting thread for " << super_chunk->to_string(false) << endl);

                    auto args = unique_ptr<one_super_chunk_args>(new one_super_chunk_args(super_chunk, array));
                    thread_started = start_super_chunk_unconstrained_transfer_thread(futures, std::move(args));

                    if (thread_started) {
                        super_chunks.pop();
                        BESDEBUG(dmrpp_3, prolog << "STARTED thread for " << super_chunk->to_string(false) << endl);
                    } else {
                        // Thread did not start, ownership of the arguments was not passed to the thread.
                        BESDEBUG(dmrpp_3, prolog << "Thread not started. args deleted, Chunk remains in queue.)" <<
                                                            " transfer_thread_counter: " << transfer_thread_counter <<
                                                            " futures.size(): " << futures.size() << endl);
                    }
                }
            }
            else {
                // No more Chunks and no futures means we're done here.
                if(futures.empty())
                    done = true;
            }
            future_finished = false;
        }
    }
    catch (...) {
        // Complete all of the futures, otherwise we'll have threads out there using up resources
        while(!futures.empty()){
            if(futures.back().valid())
                futures.back().get();
            futures.pop_back();
        }
        // re-throw the exception
        throw;
    }
}




/**
 * @brief Uses std::async and std::future to process the SuperChunks in super_chunks into the DmrppArray array.
 *
 * For each SuperChunk in the queue, retrive the chunked data by using std::async() to generate a std::future which will
 * perform the data retrieval and subsequent computational steps(inflate/shuffle/etc) and finally insertion into the
 * DmrppArray's internal data buffer.
 *
 * NOTE: There are 4 variants of this function:
 *
 *  - process_chunks_concurrent()
 *  - process_chunks_unconstrained_concurrent()
 *  - read_super_chunks_concurrent()
 *  - read_super_chunks_unconstrained_concurrent
 *
 *  If structural/algorithmic changes need to be made to this function it is almost certain that similar changes will
 *  be required in the other 3 functions.
 *
 * @param super_chunks The queue of SuperChunk objects to process.
 * @param array The DmrppArray into which the chunk data will be placed.
 */
void read_super_chunks_concurrent(queue<shared_ptr<SuperChunk>> &super_chunks, DmrppArray *array)
{
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + " name: "+array->name(), "");

    // Parallel version based on read_chunks_unconstrained(). There is
    // substantial duplication of the code in read_chunks_unconstrained(), but
    // wait to remove that when we move to C++11 which has threads integrated.

    // We maintain a list  of futures to track our parallel activities.
    list<future<bool>> futures;
    try {
        bool done = false;
        bool future_finished = true;
        while (!done) {

            if(!futures.empty())
                future_finished = get_next_future(futures, transfer_thread_counter, DMRPP_WAIT_FOR_FUTURE_MS, prolog);

            // If future_finished is true this means that the chunk_processing_thread_counter has been decremented,
            // because future::get() was called or a call to future::valid() returned false.
            BESDEBUG(dmrpp_3, prolog << "future_finished: " << (future_finished ? "true" : "false") << endl);

            if (!super_chunks.empty()){
                // Next we try to add a new Chunk compute thread if we can - there might be room.
                bool thread_started = true;
                while(thread_started && !super_chunks.empty()) {
                    auto super_chunk = super_chunks.front();
                    BESDEBUG(dmrpp_3, prolog << "Starting thread for " << super_chunk->to_string(false) << endl);

                    auto args = unique_ptr<one_super_chunk_args>(new one_super_chunk_args(super_chunk, array));
                    thread_started = start_super_chunk_transfer_thread(futures, std::move(args));

                    if (thread_started) {
                        super_chunks.pop();
                        BESDEBUG(dmrpp_3, prolog << "STARTED thread for " << super_chunk->to_string(false) << endl);
                    } else {
                        // Thread did not start, ownership of the arguments was not passed to the thread.
                        BESDEBUG(dmrpp_3, prolog << "Thread not started. args deleted, Chunk remains in queue.)" <<
                                                            " transfer_thread_counter: " << transfer_thread_counter <<
                                                            " futures.size(): " << futures.size() << endl);
                    }
                }
            }
            else {
                // No more Chunks and no futures means we're done here.
                if(futures.empty())
                    done = true;
            }
            future_finished = false;
        }
    }
    catch (...) {
        // Complete all of the futures, otherwise we'll have threads out there using up resources
        while(!futures.empty()){
            if(futures.back().valid())
                futures.back().get();
            futures.pop_back();
        }
        // re-throw the exception
        throw;
    }
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
 * @note When getting the whole AIRS file, the profiler shows that the code spends
 * about 1s here. There is a better performing replacement for this function. See
 * multiplier(const vector<unsigned int> &shape, unsigned int k) below in
 * read_chunk_unconstrained() and elsewhere in this file.
 *
 * @param address_in_target N-tuple zero-based index of an element in N-space
 * @param target_shape N-tuple of the array's dimension sizes.
 * @return The offset into the vector used to store the values.
 */
static unsigned long long
get_index(const vector<unsigned long long> &address_in_target, const vector<unsigned long long> &target_shape)
{
    assert(address_in_target.size() == target_shape.size());    // ranks must be equal

    auto shape_index = target_shape.rbegin();
    auto index = address_in_target.rbegin(), index_end = address_in_target.rend();

    unsigned long long multiplier_var = *shape_index++;
    unsigned long long offset = *index++;

    while (index != index_end) {
        assert(*index < *shape_index); // index < shape for each dim

        offset += multiplier_var * *index++;
        multiplier_var *= *shape_index++;
    }

    return offset;
}

/// Read data for a chunked array when the whole array is to be returned.
/// See below for the most general case - when chunked data are constrained.

/**
 * For dimension \arg k, compute the multiplier needed for the row-major array
 * offset formula. The formula is:
 *
 * Given an Array A has dimension sizes N0, N1, N2, ..., Nd-1
 *
 * for k = 0 to d-1 sum ( for l = k+1 to d-1 product ( Nl ) nk )
 *                        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *                                    multiplier
 *
 * @param shape The sizes of the dimensions of the array
 * @param k The dimension in question
 */
static unsigned long multiplier(const vector<unsigned long long> &shape, unsigned int k)
{
    assert(shape.size() > 1);
    assert(shape.size() > k + 1);

    vector<unsigned long long>::const_iterator i = shape.begin(), e = shape.end();
    advance(i, k + 1);
    unsigned long multiplier = *i++;
    while (i != e) {
        multiplier *= *i++;
    }

    return multiplier;
}




//#####################################################################################################################
//#####################################################################################################################
//#####################################################################################################################
//
// DmrppArray code begins here.
//
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


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
    DmrppCommon::m_duplicate_common(rhs);

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
 * @brief Return the total number of elements in this Array
 * @param constrained If true, use the constrained size of the array,
 * otherwise use the full size.
 * @return The number of elements in this Array
 */
unsigned long long DmrppArray::get_size(bool constrained)
{
    // number of array elements in the constrained array
    unsigned long long size = 1;
    for (Dim_iter dim = dim_begin(), end = dim_end(); dim != end; dim++) {
        size *= dimension_size(dim, constrained);
    }
    return size;
}

/**
 * @brief Get the array shape
 *
 * @param constrained If true, return the shape of the constrained array.
 * @return A vector<int> that describes the shape of the array.
 */
vector<unsigned long long> DmrppArray::get_shape(bool constrained)
{
    Dim_iter dim = dim_begin(), edim = dim_end();
    vector<unsigned long long> shape;

    // For a 3d array, this method took 14ms without reserve(), 5ms with
    // (when called many times).
    shape.reserve(edim - dim);

    for (; dim != edim; dim++) {
        shape.push_back(dimension_size(dim, constrained));
    }

    return shape;
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
}

/// The first of three read() implementations, for the case where the data
/// are not chunked (which this module treats as using a single chunk).

/**
 * @brief Insert data into a variable. A helper method.
 *
 * This recursive private method collects values from the rbuf and copies
 * them into buf. It supports stop, stride, and start and while correct is not
 * efficient.
 *
 * This method is used only for contiguous data. It is called only by itself
 * and read_contiguous().
 */
void DmrppArray::insert_constrained_contiguous(Dim_iter dim_iter, unsigned long *target_index,
                                               vector<unsigned long long> &subset_addr,
                                               const vector<unsigned long long> &array_shape, char /*Chunk*/*src_buf)
{
    BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - subsetAddress.size(): " << subset_addr.size() << endl);

    unsigned int bytes_per_elem = prototype()->width();

    char *dest_buf = get_buf();

    unsigned int start = this->dimension_start(dim_iter, true);
    unsigned int stop = this->dimension_stop(dim_iter, true);
    unsigned int stride = this->dimension_stride(dim_iter, true);

    dim_iter++;

    // The end case for the recursion is dimIter == dim_end(); stride == 1 is an optimization
    // See the else clause for the general case.
    if (dim_iter == dim_end() && stride == 1) {
        // For the start and stop indexes of the subset, get the matching indexes in the whole array.
        subset_addr.push_back(start);
        unsigned long long start_index = get_index(subset_addr, array_shape);
        subset_addr.pop_back();

        subset_addr.push_back(stop);
        unsigned long long stop_index = get_index(subset_addr, array_shape);
        subset_addr.pop_back();

        // Copy data block from start_index to stop_index
        // TODO Replace this loop with a call to std::memcpy()
        for (unsigned long source_index = start_index; source_index <= stop_index; source_index++) {
            unsigned long target_byte = *target_index * bytes_per_elem;
            unsigned long source_byte = source_index * bytes_per_elem;
            // Copy a single value.
            for (unsigned long i = 0; i < bytes_per_elem; i++) {
                dest_buf[target_byte++] = src_buf[source_byte++];
            }
            (*target_index)++;
        }

    }
    else {
        for (unsigned int myDimIndex = start; myDimIndex <= stop; myDimIndex += stride) {

            // Is it the last dimension?
            if (dim_iter != dim_end()) {
                // Nope! Then we recurse to the last dimension to read stuff
                subset_addr.push_back(myDimIndex);
                insert_constrained_contiguous(dim_iter, target_index, subset_addr, array_shape, src_buf);
                subset_addr.pop_back();
            }
            else {
                // We are at the last (inner most) dimension, so it's time to copy values.
                subset_addr.push_back(myDimIndex);
                unsigned int sourceIndex = get_index(subset_addr, array_shape);
                subset_addr.pop_back();

                // Copy a single value.
                unsigned long target_byte = *target_index * bytes_per_elem;
                unsigned long source_byte = sourceIndex * bytes_per_elem;

                for (unsigned int i = 0; i < bytes_per_elem; i++) {
                    dest_buf[target_byte++] = src_buf[source_byte++];
                }
                (*target_index)++;
            }
        }
    }
}

/**
 * @brief Read an array that is stored using one 'chunk.'
 *
 * If parallel transfers are enabled in the BES configuration files, this
 * method will take the one chunk that holds the variables data into a
 * number of 'child' chunks. It will transfer those child chunks in
 * parallel. Once all of the child chunks have been received, they are
 * assembled, into the on chunk and result is decompressed and then inserted
 * into the variable's memory.
 *
 * If the size of the contiguous variable is < the value of
 * DmrppRequestHandler::d_contiguous_concurrent_threshold, or if parallel
 * transfers are not enabled, the chunk is transferred in one I/O operation.
 *
 * @return Always returns true, matching the libdap::Array::read() behavior.
 */
void DmrppArray::read_contiguous()
{
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + " name: "+name(), "");

    // Get the single chunk that makes up this CONTIGUOUS variable.
    auto chunk_refs = get_chunks();
    if (chunk_refs.size() != 1)
        throw BESInternalError(string("Expected only a single chunk for variable ") + name(), __FILE__, __LINE__);

    // This is the original chunk for this 'contiguous' variable.
    auto the_one_chunk = chunk_refs[0];

    unsigned long long the_one_chunk_offset = the_one_chunk->get_offset();
    unsigned long long the_one_chunk_size = the_one_chunk->get_size();

    // We only want to read in the Chunk concurrently if:
    // - Concurrent transfers are enabled (DmrppRequestHandler::d_use_transfer_threads)
    // - The variables size is above the threshold value held in DmrppRequestHandler::d_contiguous_concurrent_threshold
    if (!DmrppRequestHandler::d_use_transfer_threads || the_one_chunk_size <= DmrppRequestHandler::d_contiguous_concurrent_threshold) {
        // Read the the_one_chunk as is. This is the non-parallel I/O case
        the_one_chunk->read_chunk();
    }
    else {

        // Allocate memory for the 'the_one_chunk' so the transfer threads can transfer data
        // from the child chunks to it.
        the_one_chunk->set_rbuf_to_size();

        // The number of child chunks are determined based on the size of the data.
        // If the size of the the_one_chunk is 3MB then 3 chunks will be made. We will round down
        // when necessary and handle the remainder later on (3.3MB = 3 chunks, 4.2MB = 4 chunks, etc.)
        unsigned long long num_chunks = floor(the_one_chunk_size / MB);
        if (num_chunks >= DmrppRequestHandler::d_max_transfer_threads)
            num_chunks = DmrppRequestHandler::d_max_transfer_threads;

        // Use the original chunk's size and offset to evenly split it into smaller chunks
        unsigned long long chunk_size = the_one_chunk_size / num_chunks;
        std::string chunk_byteorder = the_one_chunk->get_byte_order();

        // If the size of the the_one_chunk is not evenly divisible by num_chunks, capture
        // the remainder here and increase the size of the last chunk by this number of bytes.
        unsigned long long chunk_remainder = the_one_chunk_size % num_chunks;

        auto chunk_url = the_one_chunk->get_data_url();

        // Setup a queue to break up the original the_one_chunk and keep track of the pieces
        queue<shared_ptr<Chunk>> chunks_to_read;

        // Make the Chunk objects
        unsigned long long chunk_offset = the_one_chunk_offset;
        for (unsigned int i = 0; i < num_chunks - 1; i++) {
            chunks_to_read.push(shared_ptr<Chunk>(new Chunk(chunk_url, chunk_byteorder, chunk_size, chunk_offset)));
            chunk_offset += chunk_size;
        }
        // Make the the remainder Chunk, see above for details.
        chunks_to_read.push(shared_ptr<Chunk>(new Chunk(chunk_url, chunk_byteorder, chunk_size + chunk_remainder, chunk_offset)));

        // We maintain a list  of futures to track our parallel activities.
        list<future<bool>> futures;
        try {
            bool done = false;
            bool future_finished = true;
            while (!done) {

                if (!futures.empty())
                    future_finished = get_next_future(futures, transfer_thread_counter, DMRPP_WAIT_FOR_FUTURE_MS, prolog);

                // If future_finished is true this means that the chunk_processing_thread_counter has been decremented,
                // because future::get() was called or a call to future::valid() returned false.
                BESDEBUG(dmrpp_3, prolog << "future_finished: " << (future_finished ? "true" : "false") << endl);

                if (!chunks_to_read.empty()) {
                    // Next we try to add a new Chunk compute thread if we can - there might be room.
                    bool thread_started = true;
                    while (thread_started && !chunks_to_read.empty()) {
                        auto current_chunk = chunks_to_read.front();
                        BESDEBUG(dmrpp_3, prolog << "Starting thread for " << current_chunk->to_string() << endl);

                        auto args = unique_ptr<one_child_chunk_args_new>(new one_child_chunk_args_new(current_chunk, the_one_chunk));
                        thread_started = start_one_child_chunk_thread(futures, std::move(args));

                        if (thread_started) {
                            chunks_to_read.pop();
                            BESDEBUG(dmrpp_3, prolog << "STARTED thread for " << current_chunk->to_string() << endl);
                        } else {
                            // Thread did not start, ownership of the arguments was not passed to the thread.
                            BESDEBUG(dmrpp_3, prolog << "Thread not started. args deleted, Chunk remains in queue.)" <<
                                                     " transfer_thread_counter: " << transfer_thread_counter <<
                                                     " futures.size(): " << futures.size() << endl);
                        }
                    }
                } else {
                    // No more Chunks and no futures means we're done here.
                    if (futures.empty())
                        done = true;
                }
                future_finished = false;
            }
        }
        catch (...) {
            // Complete all of the futures, otherwise we'll have threads out there using up resources
            while (!futures.empty()) {
                if (futures.back().valid())
                    futures.back().get();
                futures.pop_back();
            }
            // re-throw the exception
            throw;
        }
    }

    // Now that the_one_chunk has been read, we do the needful...
    if (is_deflate_compression() || is_shuffle_compression())
       the_one_chunk->inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(),var()->width());

    // The 'the_one_chunk' now holds the data values. Transfer it to the Array.
    if (!is_projected()) {  // if there is no projection constraint
        reserve_value_capacity(get_size(false));
        val2buf(the_one_chunk->get_rbuf());      // yes, it's not type-safe
    }
    else {                  // apply the constraint
        vector<unsigned long long> array_shape = get_shape(false);

        // Reserve space in this array for the constrained size of the data request
        reserve_value_capacity(get_size(true));
        unsigned long target_index = 0;
        vector<unsigned long long> subset;

        insert_constrained_contiguous(dim_begin(), &target_index, subset, array_shape, the_one_chunk->get_rbuf());
    }

    set_read_p(true);
}

/**
 * @brief Insert a chunk into an unconstrained Array
 *
 * This code is called recursively, until the \arg dim is rank-1 for this
 * Array.
 *
 * @note dimensions 0..k are d0, d1, d2, ..., dk, and dk, the rightmost
 * dimension, varies the fastest (row-major order). To compute the offset
 * for coordinate c0, c1, c2, ..., ck. e.g., c0(d1 * d2) + c1(d2) + c2 when
 * k == 2
 *
 * @param chunk The chunk that holds data to insert
 * @param dim The current dimension
 * @param array_offset Insert values at this point in the Array
 * @param array_shape The size of the Array's dimensions
 * @param chunk_offset Insert data from this point in the chunk
 * @param chunk_shape The size of the chunk's dimensions
 * @param chunk_origin Where this chunk fits into the Array
 */
void DmrppArray::insert_chunk_unconstrained(shared_ptr<Chunk> chunk, unsigned int dim, unsigned long long array_offset,
                                            const vector<unsigned long long> &array_shape,
                                            unsigned long long chunk_offset, const vector<unsigned long long> &chunk_shape,
                                            const vector<unsigned long long> &chunk_origin)
{
    // Now we figure out the correct last element. It's possible that a
    // chunk 'extends beyond' the Array bounds. Here 'end_element' is the
    // last element of the destination array
    dimension thisDim = this->get_dimension(dim);
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_end = end_element - chunk_origin[dim];

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
        unsigned int elem_width = prototype()->width();

        array_offset += chunk_origin[dim];

        // Compute how much we are going to copy
        unsigned long long chunk_bytes = (end_element - chunk_origin[dim] + 1) * elem_width;
        char *source_buffer = chunk->get_rbuf();
        char *target_buffer = get_buf();
        memcpy(target_buffer + (array_offset * elem_width), source_buffer + (chunk_offset * elem_width), chunk_bytes);
    }
    else {
        unsigned long mc = multiplier(chunk_shape, dim);
        unsigned long ma = multiplier(array_shape, dim);

        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = 0 /*chunk_start*/; chunk_index <= chunk_end; ++chunk_index) {
            unsigned long long next_chunk_offset = chunk_offset + (mc * chunk_index);
            unsigned long long next_array_offset = array_offset + (ma * (chunk_index + chunk_origin[dim]));

            // Re-entry here:
            insert_chunk_unconstrained(chunk, dim + 1, next_array_offset, array_shape, next_chunk_offset, chunk_shape,
                                       chunk_origin);
        }
    }
}

/**
 * @brief Read data for an unconstrained chunked array
 *
 * Read data for an array when those data are split across multiple
 * chunks. This is virtually always HDF5 data, but it could be any
 * format. This method contains optimizations for the case when the
 * entire array will be read. It's faster than the code that can
 * process a constraint because that code includes a step where the
 * chunks needed are computed and then only those chunks are read.
 * This code always reads all the chunks.
 */
void DmrppArray::read_chunks_unconstrained()
{

    auto chunk_refs = get_chunks();
    if (chunk_refs.size() < 2)
        throw BESInternalError(string("Expected chunks for variable ") + name(), __FILE__, __LINE__);

    // Find all the required chunks to read. I used a queue to preserve the chunk order, which
    // made using a debugger easier. However, order does not matter, AFAIK.

    unsigned long long sc_count=0;
    stringstream sc_id;
    sc_id << name() << "-" << sc_count++;
    queue<shared_ptr<SuperChunk>> super_chunks;
    auto current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(),this)) ;
    super_chunks.push(current_super_chunk);

    // Make the SuperChunks using all the chunks.
    for(const auto& chunk: get_chunks()){
        bool added = current_super_chunk->add_chunk(chunk);
        if(!added){
            sc_id.str(std::string());
            sc_id << name() << "-" << sc_count++;
            current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(),this));
            super_chunks.push(current_super_chunk);
            if(!current_super_chunk->add_chunk(chunk)){
                stringstream msg ;
                msg << prolog << "Failed to add Chunk to new SuperChunk. chunk: " << chunk->to_string();
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }
        }
    }
    reserve_value_capacity(get_size());
    // The size in element of each of the array's dimensions
    const vector<unsigned long long> array_shape = get_shape(true);
    // The size, in elements, of each of the chunk's dimensions
    const vector<unsigned long long> chunk_shape = get_chunk_dimension_sizes();


    BESDEBUG(dmrpp_3, prolog << "d_use_transfer_threads: " << (DmrppRequestHandler::d_use_transfer_threads ? "true" : "false") << endl);
    BESDEBUG(dmrpp_3, prolog << "d_max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads << endl);

    if (!DmrppRequestHandler::d_use_transfer_threads) {  // Serial transfers
#if DMRPP_ENABLE_THREAD_TIMERS
        BESStopWatch sw(dmrpp_3);
        sw.start(prolog + "Serial SuperChunk Processing.");
#endif
        while(!super_chunks.empty()) {
            auto super_chunk = super_chunks.front();
            super_chunks.pop();
            BESDEBUG(dmrpp_3, prolog << super_chunk->to_string(true) << endl );
            super_chunk->read();
        }
    }
    else {      // Parallel transfers
#if DMRPP_ENABLE_THREAD_TIMERS
        stringstream timer_name;
        timer_name << prolog << "Concurrent SuperChunk Processing. d_max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads;
        BESStopWatch sw(dmrpp_3);
        sw.start(timer_name.str());
#endif
        read_super_chunks_unconstrained_concurrent(super_chunks, this);
    }
    set_read_p(true);
}


/// This is the most general version of the read() code. It reads chunked
/// data that are constrained to be less than the array's whole size.

/**
 * @brief What is the first element to use from a chunk
 *
 * For a chunk that fits in the array at \arg chunk_origin, what is the first element
 * of that chunk that will be transferred to the array? It may be that the first element
 * is actually not part of the chunk (given the array, its constraint, and the
 * \arg chunk_origin), and that indicates this chunk will not be used at all.
 *
 * @param thisDim Look at this dimension of the chunk and array
 * @param chunk_origin The chunk's position in the array for this given dimension
 * @return The first _element_ of the chunk to transfer.
 */
unsigned long long DmrppArray::get_chunk_start(const dimension &thisDim, unsigned int chunk_origin)
{
    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long first_element_offset = 0; // start with 0
    if ((unsigned) (thisDim.start) < chunk_origin) {
        // If the start is behind this chunk, then it's special.
        if (thisDim.stride != 1) {
            // And if the stride isn't 1, we have to figure our where to begin in this chunk.
            first_element_offset = (chunk_origin - thisDim.start) % thisDim.stride;
            // If it's zero great!
            if (first_element_offset != 0) {
                // otherwise we adjustment to get correct first element.
                first_element_offset = thisDim.stride - first_element_offset;
            }
        }
    }
    else {
        first_element_offset = thisDim.start - chunk_origin;
    }

    return first_element_offset;
}

/**
 * @brief Look at all the chunks and mark those that should be read.
 *
 * This method is used by read_chunks_parallel() to determine which
 * of the chunks that make up this array should be read, decompressed
 * and inserted into the array. The assumption is that the array is
 * subset in some way, so not all of the chunks need to be read.
 *
 * This method works in elements, not bytes.
 *
 * This method calls itself, completing to computation when \arg dim
 * has gone from 0 to the rank (rank-1, actually) of the array. As it does this, the
 * vector `target_element_address` is built up for the given chunk.
 * When \arg dim is the array's rank, `target_element_address` will
 * have a value for all but the rightmost dimension.
 *
 * @param dim Starting with 0, compute values for this dimension of the array
 * @param target_element_address Initially empty, this becomes the location
 * in the array where data should be written.
 * @param chunk This is the chunk.
 */
shared_ptr<Chunk>
DmrppArray::find_needed_chunks(unsigned int dim, vector<unsigned long long> *target_element_address, shared_ptr<Chunk> chunk)
{
    BESDEBUG(dmrpp_3, prolog << " BEGIN, dim: " << dim << endl);

    // The size, in elements, of each of the chunk's dimensions.
    const vector<unsigned long long> &chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    const vector<unsigned long long> &chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // Do we even want this chunk?
    if ((unsigned) thisDim.start > (chunk_origin[dim] + chunk_shape[dim]) ||
        (unsigned) thisDim.stop < chunk_origin[dim]) {
        return nullptr; // No. No, we do not. Skip this chunk.
    }

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long chunk_start = get_chunk_start(thisDim, chunk_origin[dim]);

    // Is the next point to be sent in this chunk at all? If no, return.
    if (chunk_start > chunk_shape[dim]) {
        return nullptr;
    }

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_end = end_element - chunk_origin[dim];

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
        BESDEBUG(dmrpp_3, prolog << " END, This is the last_dim. chunk: " << chunk->to_string() << endl);
        return chunk;
    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;

            // Re-entry here:
            auto needed = find_needed_chunks(dim + 1, target_element_address, chunk);
            if (needed){
                BESDEBUG(dmrpp_3, prolog << " END, Found chunk: " << needed->to_string() << endl);
                return needed;
            }

        }
    }
    BESDEBUG(dmrpp_3, prolog << " END, dim: " << dim << endl);

    return nullptr;
}

/**
 * @brief Insert a chunk into this array
 *
 * This method inserts the given chunk into the array. Unlike other versions of this
 * method, it _does not_ first check to see if the chunk should be inserted.
 *
 * This method is called recursively, with successive values of \arg dim, until
 * dim is equal to the rank of the array (act. rank - 1). The \arg target_element_address
 * and \arg chunk_element_address are the addresses, in 'element space' of the
 * location in this array where
 *
 * @note Only call this method when it is known that \arg chunk should be inserted
 * into the array. The chunk must be both read and decompressed.
 *
 * @param dim
 * @param target_element_address
 * @param chunk_element_address
 * @param chunk
 */
void DmrppArray::insert_chunk(
        unsigned int dim,
        vector<unsigned long long> *target_element_address,
        vector<unsigned long long> *chunk_element_address,
        shared_ptr<Chunk> chunk,
        const vector<unsigned long long> &constrained_array_shape){

    // The size, in elements, of each of the chunk's dimensions.
    const vector<unsigned long long> &chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    const vector<unsigned long long> &chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long chunk_start = get_chunk_start(thisDim, chunk_origin[dim]);

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_end = end_element - chunk_origin[dim];

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
        char *source_buffer = chunk->get_rbuf();
        char *target_buffer = get_buf();
        unsigned int elem_width = prototype()->width();

        if (thisDim.stride == 1) {
            // The start element in this array
            unsigned long long start_element = chunk_origin[dim] + chunk_start;
            // Compute how much we are going to copy
            unsigned long long chunk_constrained_inner_dim_bytes = (end_element - start_element + 1) * elem_width;

            // Compute where we need to put it.
            (*target_element_address)[dim] = (start_element - thisDim.start); // / thisDim.stride;
            // Compute where we are going to read it from
            (*chunk_element_address)[dim] = chunk_start;

            // See below re get_index()
            unsigned long long target_char_start_index =
                    get_index(*target_element_address, constrained_array_shape) * elem_width;
            unsigned long long chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;

            memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index,
                   chunk_constrained_inner_dim_bytes);
        }
        else {
            // Stride != 1
            for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
                // Compute where we need to put it.
                (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;

                // Compute where we are going to read it from
                (*chunk_element_address)[dim] = chunk_index;

                // These calls to get_index() can be removed as with the insert...unconstrained() code.
               unsigned int target_char_start_index =
                        get_index(*target_element_address, constrained_array_shape) * elem_width;
                unsigned int chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;

                memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index, elem_width);
            }
        }
    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;
            (*chunk_element_address)[dim] = chunk_index;

            // Re-entry here:
            insert_chunk(dim + 1, target_element_address, chunk_element_address, chunk, constrained_array_shape);
        }
    }
}




/**
 * @brief Read chunked data by building SuperChunks from the required chunks and reading the SuperChunks
 *
 * Read chunked data, using either parallel or serial data transfers, depending on
 * the DMR++ handler configuration parameters.
 */
void DmrppArray::read_chunks()
{
    auto chunk_refs = get_chunks();
    if (chunk_refs.size() < 2)
        throw BESInternalError(string("Expected chunks for variable ") + name(), __FILE__, __LINE__);

    // Find all the required chunks to read. I used a queue to preserve the chunk order, which
    // made using a debugger easier. However, order does not matter, AFAIK.
    unsigned long long sc_count=0;
    stringstream sc_id;
    sc_id << name() << "-" << sc_count++;
    queue<shared_ptr<SuperChunk>> super_chunks;
    auto current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(), this)) ;
    super_chunks.push(current_super_chunk);

    // TODO We know that non-contiguous chunks may be forward or backward in the file from
    //  the current offset. When an add_chunk() call fails, prior to making a new SuperChunk
    //  we might want want try adding the rejected Chunk to the other existing SuperChunks to see
    //  if it's contiguous there.
    // Find the required Chunks and put them into SuperChunks.
    for(const auto& chunk: get_chunks()){
        vector<unsigned long long> target_element_address = chunk->get_position_in_array();
        auto needed = find_needed_chunks(0 /* dimension */, &target_element_address, chunk);
        if (needed){
            bool added = current_super_chunk->add_chunk(chunk);
            if(!added){
                sc_id.str(std::string()); // Clears stringstream.
                sc_id << name() << "-" << sc_count++;
                current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(),this));
                super_chunks.push(current_super_chunk);
                if(!current_super_chunk->add_chunk(chunk)){
                    stringstream msg ;
                    msg << prolog << "Failed to add Chunk to new SuperChunk. chunk: " << chunk->to_string();
                    throw BESInternalError(msg.str(), __FILE__, __LINE__);
                }
            }
        }
    }

    reserve_value_capacity(get_size(true));

    BESDEBUG(dmrpp_3, prolog << "d_use_transfer_threads: " << (DmrppRequestHandler::d_use_transfer_threads ? "true" : "false") << endl);
    BESDEBUG(dmrpp_3, prolog << "d_max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads << endl);
    BESDEBUG(dmrpp_3, prolog << "d_use_compute_threads: " << (DmrppRequestHandler::d_use_compute_threads ? "true" : "false") << endl);
    BESDEBUG(dmrpp_3, prolog << "d_max_compute_threads: " << DmrppRequestHandler::d_max_compute_threads << endl);
    BESDEBUG(dmrpp_3, prolog << "SuperChunks.size(): " << super_chunks.size() << endl);

    if (!DmrppRequestHandler::d_use_transfer_threads) {
        // This version is the 'serial' version of the code. It reads a chunk, inserts it,
        // reads the next one, and so on.
#if DMRPP_ENABLE_THREAD_TIMERS
        BESStopWatch sw(dmrpp_3);
        sw.start(prolog + "Serial SuperChunk Processing.");
#endif
        while (!super_chunks.empty()) {
            auto super_chunk = super_chunks.front();
            super_chunks.pop();
            BESDEBUG(dmrpp_3, prolog << super_chunk->to_string(true) << endl );
            super_chunk->read();
        }
    }
    else {
#if DMRPP_ENABLE_THREAD_TIMERS
        stringstream timer_name;
        timer_name << prolog << "Concurrent SuperChunk Processing. d_max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads;
        BESStopWatch sw(dmrpp_3);
        sw.start(timer_name.str());
#endif
        read_super_chunks_concurrent(super_chunks, this);
    }
    set_read_p(true);
}


#ifdef USE_READ_SERIAL
/**
 * Insert data from \arg chunk into the array given the current constraint
 *
 * Recursive calls build up the two vectors \arg target_element_address and
 * \arg chunk_element_address. These vectors start out with \arg dim elements,
 * the \arg chunk_element_address holds 0, 0, ..., 0 and the \arg target_element_address
 * holds the index of the first value of this chunk in the target array
 *
 * @note This method will be called several time for any given chunk, so the
 * chunk_read() and chunk_inflate() methods 'protect' the chunk against being read
 * or decompressed more than once. For reading this is not a fatal error (but a waste
 * of time), but it is a fatal error decompression. The code in read_chunk_parallel()
 * does not have this problem (but it uses the same read and inflate code and thus
 * I've left in the tracking booleans.
 *
 * @param dim
 * @param target_element_address
 * @param chunk_element_address
 * @param chunk
 * @return
 */
void DmrppArray::insert_chunk_serial(unsigned int dim, vector<unsigned int> *target_element_address, vector<unsigned int> *chunk_element_address,
    Chunk *chunk)
{
    BESDEBUG("dmrpp", __func__ << " dim: "<< dim << " BEGIN "<< endl);

    // The size, in elements, of each of the chunk's dimensions.
    const vector<unsigned int> &chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    const vector<unsigned int> &chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // Do we even want this chunk?
    if ((unsigned) thisDim.start > (chunk_origin[dim] + chunk_shape[dim]) || (unsigned) thisDim.stop < chunk_origin[dim]) {
        return; // No. No, we do not. Skip this.
    }

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned int first_element_offset = get_chunk_start(dim, chunk_origin);

    // Is the next point to be sent in this chunk at all? If no, return.
    if (first_element_offset > chunk_shape[dim]) {
        return;
    }

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_start = first_element_offset; //start_element - chunk_origin[dim];
    unsigned long long chunk_end = end_element - chunk_origin[dim];
    vector<unsigned int> constrained_array_shape = get_shape(true);

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
        // Read and Process chunk
        chunk->read_chunk();

        chunk->inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), var()->width());

        char *source_buffer = chunk->get_rbuf();
        char *target_buffer = get_buf();
        unsigned int elem_width = prototype()->width();

        if (thisDim.stride == 1) {
            // The start element in this array
            unsigned long long start_element = chunk_origin[dim] + first_element_offset;
            // Compute how much we are going to copy
            unsigned long long chunk_constrained_inner_dim_bytes = (end_element - start_element + 1) * elem_width;

            // Compute where we need to put it.
            (*target_element_address)[dim] = (start_element - thisDim.start) / thisDim.stride;
            // Compute where we are going to read it from
            (*chunk_element_address)[dim] = first_element_offset;

            unsigned int target_char_start_index = get_index(*target_element_address, constrained_array_shape) * elem_width;
            unsigned int chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;

            memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index, chunk_constrained_inner_dim_bytes);
        }
        else {
            // Stride != 1
            for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
                // Compute where we need to put it.
                (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;

                // Compute where we are going to read it from
                (*chunk_element_address)[dim] = chunk_index;

                unsigned int target_char_start_index = get_index(*target_element_address, constrained_array_shape) * elem_width;
                unsigned int chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;

                memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index, elem_width);
            }
        }
    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;
            (*chunk_element_address)[dim] = chunk_index;

            // Re-entry here:
            insert_chunk_serial(dim + 1, target_element_address, chunk_element_address, chunk);
        }
    }
}

void DmrppArray::read_chunks_serial()
{
    BESDEBUG("dmrpp", __func__ << " for variable '" << name() << "' - BEGIN" << endl);

    vector<Chunk> &chunk_refs = get_chunk_vec();
    if (chunk_refs.size() == 0) throw BESInternalError(string("Expected one or more chunks for variable ") + name(), __FILE__, __LINE__);

    // Allocate target memory.
    reserve_value_capacity(get_size(true));

    /*
     * Find the chunks to be read, make curl_easy handles for them, and
     * stuff them into our curl_multi handle. This is a recursive activity
     * which utilizes the same code that copies the data from the chunk to
     * the variables.
     */
    for (unsigned long i = 0; i < chunk_refs.size(); i++) {
        Chunk &chunk = chunk_refs[i];

        vector<unsigned int> chunk_source_address(dimensions(), 0);
        vector<unsigned int> target_element_address = chunk.get_position_in_array();

        // Recursive insertion operation.
        insert_chunk_serial(0, &target_element_address, &chunk_source_address, &chunk);
    }

    set_read_p(true);

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for " << name() << " END"<< endl);
}
#endif

/**
 * @brief Read data for the array
 *
 * This reads data for a variable and loads it into memory. The software is
 * specialized for reading data using HTTP for either arrays stored in one
 * contiguous piece of memory or in a series of chunks.
 *
 * @return Always returns true
 * @exception BESError Thrown when the data cannot be read, for a number of
 * reasons, including various network I/O issues.
 */
bool DmrppArray::read()
{
    if (read_p()) return true;

    // Single chunk and 'contiguous' are the same for this code.

    if (get_immutable_chunks().size() == 1) { // Removed: || get_chunk_dimension_sizes().empty()) {
        BESDEBUG(dmrpp_4, "Calling read_contiguous() for " << name() << endl);
        read_contiguous();    // Throws on various errors
    }
    else {  // Handle the more complex case where the data is chunked.
        if (!is_projected()) {
            BESDEBUG(dmrpp_4, "Calling read_chunks_unconstrained() for " << name() << endl);
            read_chunks_unconstrained();
        }
        else {
            BESDEBUG(dmrpp_4, "Calling read_chunks() for " << name() << endl);
            read_chunks();
        }
    }

    if (this->twiddle_bytes()) {
        int num = this->length();
        Type var_type = this->var()->type();

        switch (var_type) {
            case dods_int16_c:
            case dods_uint16_c: {
                dods_uint16 *local = reinterpret_cast<dods_uint16*>(this->get_buf());
                while (num--) {
                    *local = bswap_16(*local);
                    local++;
                }
                break;
            }
            case dods_int32_c:
            case dods_uint32_c: {
                dods_uint32 *local = reinterpret_cast<dods_uint32*>(this->get_buf());;
                while (num--) {
                    *local = bswap_32(*local);
                    local++;
                }
                break;
            }
            case dods_int64_c:
            case dods_uint64_c: {
                dods_uint64 *local = reinterpret_cast<dods_uint64*>(this->get_buf());;
                while (num--) {
                    *local = bswap_64(*local);
                    local++;
                }
                break;
            }
            default: break; // Do nothing for all other types..
        }
    }

    return true;
}

/**
 * Classes used with the STL for_each() algorithm; stolen from libdap::Array.
 */
///@{
class PrintD4ArrayDimXMLWriter : public unary_function<Array::dimension &, void> {
    XMLWriter &xml;
    // Was this variable constrained using local/direct slicing? i.e., is d_local_constraint set?
    // If so, don't use shared dimensions; instead emit Dim elements that are anonymous.
    bool d_constrained;
public:

    PrintD4ArrayDimXMLWriter(XMLWriter &xml, bool c) :
            xml(xml), d_constrained(c)
    {
    }

    void operator()(Array::dimension &d)
    {
        // This duplicates code in D4Dimensions (where D4Dimension::print_dap4() is defined
        // because of the need to print the constrained size of a dimension. I think that
        // the constraint information has to be kept here and not in the dimension (since they
        // are shared dims). Could hack print_dap4() to take the constrained size, however.
        if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar *) "Dim") < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write Dim element");

        string name = (d.dim) ? d.dim->fully_qualified_name() : d.name;
        // If there is a name, there must be a Dimension (named dimension) in scope
        // so write its name but not its size.
        if (!d_constrained && !name.empty()) {
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "name",
                                            (const xmlChar *) name.c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
        }
        else if (d.use_sdim_for_slice) {
            assert(!name.empty());
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "name",
                                            (const xmlChar *) name.c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
        }
        else {
            ostringstream size;
            size << (d_constrained ? d.c_size : d.size);
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "size",
                                            (const xmlChar *) size.str().c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
        }

        if (xmlTextWriterEndElement(xml.get_writer()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not end Dim element");
    }
};

class PrintD4ConstructorVarXMLWriter : public unary_function<BaseType *, void> {
    XMLWriter &xml;
    bool d_constrained;
public:
    PrintD4ConstructorVarXMLWriter(XMLWriter &xml, bool c) :
            xml(xml), d_constrained(c)
    {
    }

    void operator()(BaseType *btp)
    {
        btp->print_dap4(xml, d_constrained);
    }
};

class PrintD4MapXMLWriter : public unary_function<D4Map *, void> {
    XMLWriter &xml;

public:
    PrintD4MapXMLWriter(XMLWriter &xml) :
            xml(xml)
    {
    }

    void operator()(D4Map *m)
    {
        m->print_dap4(xml);
    }
};
///@}

/**
 * @brief Shadow libdap::Array::print_dap4() - optionally prints DMR++ chunk information
 *
 * This version of libdap::BaseType::print_dap4() will print information about
 * HDF5 chunks when the value of the static class filed dmrpp::DmrppCommon::d_print_chunks
 * is true. The method DMRpp::print_dmrpp() will set the _d_pprint_chunks_ field to
 * true causing this method to include the _chunks_ elements in its output. When
 * the field's value is false, this method prints the same output as libdap::Array.
 *
 * @note There are, no doubt, better ways to do this than using what is essentially a
 * global flag; one way is to  synchronize access to a DMR C++ object and a DOM
 * tree for the same DMR document. The chunk information can be read from the DMR and
 * inserted into the DOM tree, which then printed. If the
 * approach I took here becomes an issue (i.e., if we have to fix problems in libdap and
 * here because of code duplication), we should probably recode this and the related
 * methods to use the 'DOM tree approach.'
 *
 * @param xml Write the XML to this instance of XMLWriter
 * @param constrained True if the response should be constrained. False by default
 *
 * @see DmrppCommon::print_dmrpp()
 * @see DMRpp::print_dmrpp()
 */
void DmrppArray::print_dap4(XMLWriter &xml, bool constrained /*false*/)
{
    if (constrained && !send_p()) return;

    if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar *) var()->type_name().c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write " + type_name() + " element");

    if (!name().empty())
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "name", (const xmlChar *) name().c_str()) <
            0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");

    // Hack job... Copied from D4Enum::print_xml_writer. jhrg 11/12/13
    if (var()->type() == dods_enum_c) {
        D4Enum *e = static_cast<D4Enum *>(var());
        string path = e->enumeration()->name();
        if (e->enumeration()->parent()) {
            // print the FQN for the enum def; D4Group::FQN() includes the trailing '/'
            path = static_cast<D4Group *>(e->enumeration()->parent()->parent())->FQN() + path;
        }
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "enum", (const xmlChar *) path.c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for enum");
    }

    if (prototype()->is_constructor_type()) {
        Constructor &c = static_cast<Constructor &>(*prototype());
        for_each(c.var_begin(), c.var_end(), PrintD4ConstructorVarXMLWriter(xml, constrained));
        // bind2nd(mem_fun_ref(&BaseType::print_dap4), xml));
    }

    // Drop the local_constraint which is per-array and use a per-dimension on instead
    for_each(dim_begin(), dim_end(), PrintD4ArrayDimXMLWriter(xml, constrained));

    attributes()->print_dap4(xml);

    for_each(maps()->map_begin(), maps()->map_end(), PrintD4MapXMLWriter(xml));

    // Only print the chunks info if there. This is the code added to libdap::Array::print_dap4().
    // jhrg 5/10/18
    if (DmrppCommon::d_print_chunks && get_immutable_chunks().size() > 0)
        print_chunks_element(xml, DmrppCommon::d_ns_prefix);

    // If this variable uses the COMPACT layout, encode the values for
    // the array using base64. Note that strings are a special case; each
    // element of the array is a string and is encoded in its own base64
    // xml element. So, wghile an array of 10 int32 will be encoded in a
    // single base64 element, an array of 10 strings will use 10 base64
    // elements. This is because the size of each string's value is different.
    // Not so for an int32.
    if (DmrppCommon::d_print_chunks && is_compact_layout() && read_p()) {
        switch (var()->type()) {
            case dods_byte_c:
            case dods_char_c:
            case dods_int8_c:
            case dods_uint8_c:
            case dods_int16_c:
            case dods_uint16_c:
            case dods_int32_c:
            case dods_uint32_c:
            case dods_int64_c:
            case dods_uint64_c:

            case dods_enum_c:

            case dods_float32_c:
            case dods_float64_c: {
                u_int8_t *values = 0;
                try {
                    size_t size = buf2val(reinterpret_cast<void **>(&values));
                    string encoded = base64::Base64::encode(values, size);
                    delete[] values;
                    print_compact_element(xml, DmrppCommon::d_ns_prefix, encoded);
                }
                catch (...) {
                    delete[] values;
                    throw;
                }
                break;
            }

            case dods_str_c:
            case dods_url_c: {
                string *values = 0;
                try {
                    // discard the return value of buf2val()
                    buf2val(reinterpret_cast<void **>(&values));
                    string str;
                    for (int i = 0; i < length(); ++i) {
                        str = (*(static_cast<string *> (values) + i));
                        string encoded = base64::Base64::encode(reinterpret_cast<const u_int8_t *>(str.c_str()), str.size());
                        print_compact_element(xml, DmrppCommon::d_ns_prefix, encoded);
                    }
                    delete[] values;
                }
                catch (...) {
                    delete[] values;
                    throw;
                }
                break;
            }

            default:
                throw InternalErr(__FILE__, __LINE__, "Vector::val2buf: bad type");
        }
    }
    if (xmlTextWriterEndElement(xml.get_writer()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not end " + type_name() + " element");
}

void DmrppArray::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "DmrppArray::" << __func__ << "(" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Array::dump(strm);
    strm << BESIndent::LMarg << "value: " << "----" << /*d_buf <<*/endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp
