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
#include <iomanip>
#include <cmath>
#include <zlib.h>

#include <libdap/D4Enum.h>
#include <libdap/D4Attributes.h>
#include <libdap/D4Maps.h>
#include <libdap/D4Group.h>
#include <libdap/Byte.h>
#include <libdap/util.h>

#include "BESInternalError.h"
#include "BESInternalFatalError.h"
#include "BESDebug.h"
#include "BESLog.h"
#include "BESStopWatch.h"

#include "byteswap_compat.h"
#include "float_byteswap.h"
#include "CurlHandlePool.h"
#include "Chunk.h"
#include "DmrppArray.h"
#include "DmrppStructure.h"
#include "DmrppRequestHandler.h"
#include "DmrppNames.h"
#include "Base64.h"
#include "vlsa_util.h"

// Used with BESDEBUG
#define dmrpp_3 "dmrpp:3"

using namespace libdap;
using namespace std;

#define MB (1024*1024)
#define prolog std::string("DmrppArray::").append(__func__).append("() - ")

namespace dmrpp {


// Transfer Thread Pool state variables.
std::mutex transfer_thread_pool_mtx;     // mutex for critical section
//atomic_ullong transfer_thread_counter(0);
atomic_uint transfer_thread_counter(0);



/**
 * @brief Uses future::wait_for() to scan the futures for a ready future, returning true when once get() has been called.
 *
 * When a valid, ready future is found future::get() is called and the thead_counter is decremented.
 * Returns true when it successfully "get"s a future (joins a thread), or if a future turns up as not valid, then
 * it is discarded, and the thread is ":finished" and true is returned.
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

static void one_child_chunk_thread_new_sanity_check(const one_child_chunk_args_new *args) {
    if (!args->the_one_chunk->get_rbuf()) {
        throw BESInternalError("one_child_chunk_thread_new_sanity_check() - the_one_chunk->get_rbuf() is NULL!", __FILE__, __LINE__);
    }
    if (!args->child_chunk->get_rbuf()) {
        throw BESInternalError("one_child_chunk_thread_new_sanity_check() - child_chunk->get_rbuf() is NULL!", __FILE__, __LINE__);
    }
    if (args->child_chunk->get_bytes_read() != args->child_chunk->get_size()) {
        throw BESInternalError("one_child_chunk_thread_new_sanity_check() - child_chunk->get_bytes_read() != child_chunk->get_size()!", __FILE__, __LINE__);
    }
}

/**
 * @brief Manage parallel transfer for contiguous data
 *
 * Read data for one of the 'child chunks' made to read data for a variable
 * with contiguous storage in parallel.
 *
 * This is only used for threads started by read_contiguous().
 *
 * @param args A pointer to a one_child_chunk_args
 */
bool one_child_chunk_thread_new(const unique_ptr<one_child_chunk_args_new> &args)
{
    args->child_chunk->read_chunk();

    one_child_chunk_thread_new_sanity_check(args.get());

    // the_one_chunk offset \/
    // the_one_chunk:  mmmmmmmmmmmmmmmm
    // child chunks:   1111222233334444 (there are four child chunks)
    // child offsets:  ^   ^   ^   ^
    // For this example, child_1_offset - the_one_chunk_offset == 0 (that's always true)
    // child_2_offset - the_one_chunk_offset == 4; child_2_offset - the_one_chunk_offset == 8
    // and child_3_offset - the_one_chunk_offset == 12.
    // Those are the starting locations with in the data buffer of the the_one_chunk
    // where that child chunk should be written.
    // Note: all the offset values start at the beginning of the file.

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
bool one_super_chunk_transfer_thread(const unique_ptr<one_super_chunk_args> &args)
{

#if DMRPP_ENABLE_THREAD_TIMERS
    stringstream timer_tag;
    timer_tag << prolog << "tid: 0x" << std::hex << std::this_thread::get_id() <<
    " parent_tid: 0x" << std::hex << args->parent_thread_id << " sc_id: " << args->super_chunk->id();
    BES_STOPWATCH_START(TRANSFER_THREADS, prolog + timer_tag.str());
#endif

    args->super_chunk->read();
    return true;
}

/**
 * @brief A single argument wrapper for process_super_chunk_unconstrained() for use with std::async().
 * @param args A unique_ptr to an instance of one_super_chunk_args.
 * @return True unless an exception is throw in which case neither true or false apply.
 */
bool one_super_chunk_unconstrained_transfer_thread(const unique_ptr<one_super_chunk_args> &args)
{

#if DMRPP_ENABLE_THREAD_TIMERS
    stringstream timer_tag;
    timer_tag << prolog << "tid: 0x" << std::hex << std::this_thread::get_id() <<
    " parent_tid: 0x" << std::hex << args->parent_thread_id  << " sc_id: " << args->super_chunk->id();
    BES_STOPWATCH_START(TRANSFER_THREADS, prolog + timer_tag.str());
#endif

    args->super_chunk->read_unconstrained();
    return true;
}

bool one_super_chunk_unconstrained_transfer_thread_dio(const unique_ptr<one_super_chunk_args> &args)
{

#if DMRPP_ENABLE_THREAD_TIMERS
    stringstream timer_tag;
    timer_tag << prolog << "tid: 0x" << std::hex << std::this_thread::get_id() <<
    " parent_tid: 0x" << std::hex << args->parent_thread_id  << " sc_id: " << args->super_chunk->id();
    BES_STOPWATCH_START(TRANSFER_THREADS, prolog + timer_tag.str());
#endif

    args->super_chunk->read_unconstrained_dio();
    return true;
}


bool start_one_child_chunk_thread(list<std::future<bool>> &futures, unique_ptr<one_child_chunk_args_new> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck (transfer_thread_pool_mtx);
    if (transfer_thread_counter < DmrppRequestHandler::d_max_transfer_threads) {
        transfer_thread_counter++;
        futures.push_back(std::async(std::launch::async, one_child_chunk_thread_new, std::move(args)));
        retval = true;

        // The args may be null after move(args) is called and causes the segmentation fault in the following BESDEBUG.
        // So remove that part but leave the futures.size() for bookkeeping.
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() <<endl);
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
       
        // The args may be null after move(args) is called and causes the segmentation fault in the following BESDEBUG.
        // So remove that part but leave the futures.size() for bookkeeping.
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() <<endl);
 
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

        // The args may be null after move(args) is called and causes the segmentation fault in the following BESDEBUG.
        // So remove that part but leave the futures.size() for bookkeeping.
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() <<endl);
 
    }
    return retval;
}

bool start_super_chunk_unconstrained_transfer_thread_dio(list<std::future<bool>> &futures, unique_ptr<one_super_chunk_args> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck (transfer_thread_pool_mtx);
    if(transfer_thread_counter < DmrppRequestHandler::d_max_transfer_threads) {
        transfer_thread_counter++;
        futures.push_back(std::async(std::launch::async, one_super_chunk_unconstrained_transfer_thread_dio, std::move(args)));
        retval = true;
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() <<
                                            "' from std::async, transfer_thread_counter: " << transfer_thread_counter << endl);
    }
    return retval;
}


/**
 * @brief Uses std::async() and std::future to process the SuperChunks in super_chunks into the DmrppArray array.
 *
 * For each SuperChunk in the queue, retrieve the chunked data by using std::async() to generate a std::future which will
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
    BES_STOPWATCH_START(MODULE, prolog + "Timing array name: "+array->name());

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
        // Complete all the futures, otherwise we'll have threads out there using up resources
        while(!futures.empty()){
            if(futures.back().valid())
                futures.back().get();
            futures.pop_back();
        }
        // re-throw the exception
        throw;
    }
}

// Clone of read_super_chunks_unconstrained_concurrent for direct IO. 
// Doing this to ensure direct IO won't affect the regular operations.
void read_super_chunks_unconstrained_concurrent_dio(queue<shared_ptr<SuperChunk>> &super_chunks, DmrppArray *array)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing array name: "+array->name());

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

                    // direct IO calling
                    thread_started = start_super_chunk_unconstrained_transfer_thread_dio(futures, std::move(args));

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
        // Complete all the futures, otherwise we'll have threads out there using up resources
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
 * For each SuperChunk in the queue, retrieve the chunked data by using std::async() to generate a std::future which will
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
void read_super_chunks_concurrent(queue< shared_ptr<SuperChunk> > &super_chunks, DmrppArray *array)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing array name: "+array->name());

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
        // Complete all the futures, otherwise we'll have threads out there using up resources
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
    if (address_in_target.size() != target_shape.size()) {  // ranks must be equal
        throw BESInternalError("get_index: address_in_target != target_shape", __FILE__, __LINE__);
    }

    auto shape_index = target_shape.rbegin();
    auto index = address_in_target.rbegin(), index_end = address_in_target.rend();

    unsigned long long multiplier_var = *shape_index++;
    unsigned long long offset = *index++;

    while (index != index_end) {
        if (*index >= *shape_index) {
            throw BESInternalError("get_index: index >= shape_index", __FILE__, __LINE__);
        }

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
static unsigned long long multiplier(const vector<unsigned long long> &shape, unsigned int k)
{
    if (!(shape.size() > k + 1)) {
        throw BESInternalError("multiplier: !(shape.size() > k + 1)", __FILE__, __LINE__);
    }

    vector<unsigned long long>::const_iterator i = shape.begin(), e = shape.end();
    advance(i, k + 1);
    unsigned long long multiplier = *i++;
    while (i != e) {
        multiplier *= *i++;
    }

    return multiplier;
}

//#####################################################################################################################
//
// DmrppArray code begins here.
//
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

DmrppArray &
DmrppArray::operator=(const DmrppArray &rhs)
{
    if (this == &rhs) return *this;

    dynamic_cast<Array &>(*this) = rhs; // run Constructor=

    dynamic_cast<DmrppCommon &>(*this) = rhs;
    // Removed DmrppCommon::m_duplicate_common(rhs); jhrg 11/12/21

    return *this;
}

/**
 * @brief Is this Array subset?
 * @return True if the array has a projection expression, false otherwise
 */
bool DmrppArray::is_projected() 
{
    for (Dim_iter p = dim_begin(), e = dim_end(); p != e; ++p)
        if (dimension_size_ll(p, true) != dimension_size_ll(p, false)) return true;

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
    unsigned long long asize = 1;
    for (Dim_iter dim = dim_begin(), end = dim_end(); dim != end; dim++) {
        auto dim_size =  dimension_size_ll(dim, constrained);
        asize *= dim_size;
    }
    return asize;
}

unsigned long long DmrppArray::get_maximum_constrained_buffer_nelmts()
{
    // The stride here doesn't matter since we need to obtain the maximum contrained buffer. KY 2025-12-08
    unsigned long long asize = 1;
    for (Dim_iter dim = dim_begin(), end = dim_end(); dim != end; dim++) {
        int64_t start = dimension_start_ll(dim,true);
        int64_t stop = dimension_stop_ll(dim,true);
        asize *= stop-start+1;
    }
    return asize;

}

/**
 * @brief Get the array shape
 *
 * @param constrained If true, return the shape of the constrained array.
 * @return A vector<int> that describes the shape of the array.
 */
vector<unsigned long long> DmrppArray::get_shape(bool constrained)
{
    auto dim = dim_begin(), edim = dim_end();
    vector<unsigned long long> shape;

    // For a 3d array, this method took 14ms without reserve(), 5ms with
    // (when called many times).
    shape.reserve(edim - dim);

    for (; dim != edim; dim++) {
        shape.push_back(dimension_size_ll(dim, constrained));
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
    if (i > (dim_end() - dim_begin())) {
        throw BESInternalError("get_dimension: i > (dim_end() - dim_begin())", __FILE__, __LINE__);
    }
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
                                               const vector<unsigned long long> &array_shape, char /*Chunk*/*src_buf, char *dest_buf)
{
    BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - subsetAddress.size(): " << subset_addr.size() << endl);

    uint64_t start = this->dimension_start_ll(dim_iter, true);
    uint64_t stop = this->dimension_stop_ll(dim_iter, true);
    uint64_t stride = this->dimension_stride_ll(dim_iter, true);

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
        for (uint64_t source_index = start_index; source_index <= stop_index; source_index++) {
            uint64_t target_byte = *target_index * bytes_per_element;
            uint64_t source_byte = source_index * bytes_per_element;
            // Copy a single value.
            for (unsigned long i = 0; i < bytes_per_element; i++) {
                dest_buf[target_byte++] = src_buf[source_byte++];
            }
            (*target_index)++;
        }

    }
    else {
        for (uint64_t myDimIndex = start; myDimIndex <= stop; myDimIndex += stride) {

            // Is it the last dimension?
            if (dim_iter != dim_end()) {
                // Nope! Then we recurse to the last dimension to read stuff
                subset_addr.push_back(myDimIndex);
                insert_constrained_contiguous(dim_iter, target_index, subset_addr, array_shape, src_buf, dest_buf);
                subset_addr.pop_back();
            }
            else {
                // We are at the last (innermost) dimension, so it's time to copy values.
                subset_addr.push_back(myDimIndex);
                unsigned int sourceIndex = get_index(subset_addr, array_shape);
                subset_addr.pop_back();

                // Copy a single value.
                uint64_t target_byte = *target_index * bytes_per_element;
                uint64_t source_byte = sourceIndex * bytes_per_element;

                for (unsigned int i = 0; i < bytes_per_element; i++) {
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
 * @note Currently String Arrays are handled using a different method. Do not
 * call this with String data. jhrg 3/3/22
 *
 * @return Always returns true, matching the libdap::Array::read() behavior.
 */
void DmrppArray::read_contiguous()
{

    BESDEBUG(dmrpp_3, prolog << "NOT using direct IO " << endl);
    BES_STOPWATCH_START(MODULE, prolog + "Timing array name: "+name());

    // Get the single chunk that makes up this CONTIGUOUS variable.
    if (get_chunk_count() != 1)
        throw BESInternalError(string("Expected only a single chunk for variable ") + name(), __FILE__, __LINE__);

    // This is the original chunk for this 'contiguous' variable.
    auto the_one_chunk = get_immutable_chunks()[0];

    unsigned long long the_one_chunk_offset = the_one_chunk->get_offset();
    unsigned long long the_one_chunk_size = the_one_chunk->get_size();

    // We only want to read in the Chunk concurrently if:
    // - Concurrent transfers are enabled (DmrppRequestHandler::d_use_transfer_threads)
    // - The variable's size is above the threshold value held in DmrppRequestHandler::d_contiguous_concurrent_threshold
    if (!DmrppRequestHandler::d_use_transfer_threads || the_one_chunk_size <= DmrppRequestHandler::d_contiguous_concurrent_threshold) {
        // Read the the_one_chunk as is. This is the non-parallel I/O case
        the_one_chunk->read_chunk();

    }
    else {
        // Allocate memory for the 'the_one_chunk' so the transfer threads can transfer data
        // from the child chunks to it.
        the_one_chunk->set_rbuf_to_size();

        // We need to load fill values if using fill values. KY 2023-02-17
        if (the_one_chunk->get_uses_fill_value()) {
            the_one_chunk->load_fill_values();
        }
        // The number of child chunks are determined based on the size of the data.
        // If the size of the the_one_chunk is 3 MB then 3 chunks will be made. We will round down
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

        // Set up a queue to break up the original the_one_chunk and keep track of the pieces
        queue<shared_ptr<Chunk>> chunks_to_read;

        // Make the Chunk objects
        unsigned long long chunk_offset = the_one_chunk_offset;
        for (unsigned int i = 0; i < num_chunks - 1; i++) {
            if (chunk_url == nullptr) {
                BESDEBUG(dmrpp_3, "chunk_url is null, this may be a variable that covers the fill values." <<endl);
                chunks_to_read.push(shared_ptr<Chunk>(new Chunk(chunk_byteorder,the_one_chunk->get_fill_value(),the_one_chunk->get_fill_value_type(), chunk_size, chunk_offset)));
            }
            else
                chunks_to_read.push(shared_ptr<Chunk>(new Chunk(chunk_url, chunk_byteorder, chunk_size, chunk_offset)));
            chunk_offset += chunk_size;
        }
        // Make the remainder Chunk, see above for details.
        if (chunk_url != nullptr) 
            chunks_to_read.push(shared_ptr<Chunk>(new Chunk(chunk_url, chunk_byteorder, chunk_size + chunk_remainder, chunk_offset)));
        else 
            chunks_to_read.push(shared_ptr<Chunk>(new Chunk(chunk_byteorder,the_one_chunk->get_fill_value(),the_one_chunk->get_fill_value_type(), chunk_size, chunk_offset)));

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
            // Complete all the futures, otherwise we'll have threads out there using up resources
            while (!futures.empty()) {
                if (futures.back().valid())
                    futures.back().get();
                futures.pop_back();
            }
            // re-throw the exception
            throw;
        }
    }
    BESDEBUG(dmrpp_3, prolog << "Before is_filter " << endl);

    // Check if this is a DAP structure we can handle
    // Now that the_one_chunk has been read, we do what is necessary...
    if (!is_filters_empty() && !get_one_chunk_fill_value()) {
        the_one_chunk->filter_chunk(get_filters(), get_chunk_size_in_elements(), bytes_per_element);
    }
    // The 'the_one_chunk' now holds the data values. Transfer it to the Array.
    if (!is_projected()) {  // if there is no projection constraint
        reserve_value_capacity_ll(get_size(false));

        // We need to handle the structure data differently.
        if (this->var()->type() != dods_structure_c)
           val2buf(the_one_chunk->get_rbuf());      // yes, it's not type-safe
        else { // Structure 
            // Check if we can handle this case. 
            // Currently we only handle one-layer simple int/float types. 
            if (is_readable_struct) {
                // Only "one chunk", we can simply obtain the buf_value.
                char *buf_value = the_one_chunk->get_rbuf();
                
                unsigned long long value_size = this->length_ll() * bytes_per_element;  
                vector<char> values(buf_value,buf_value+value_size);
                read_array_of_structure(values);
            }
            else 
                throw InternalErr(__FILE__, __LINE__, "Only handle integer and float base types. Cannot handle the array of complex structure yet."); 
        }
    }
    else {                  // apply the constraint

        if (this->var()->type() != dods_structure_c) { 

            vector<unsigned long long> array_shape = get_shape(false);
            unsigned long target_index = 0;
            vector<unsigned long long> subset;

            // Reserve space in this array for the constrained size of the data request
            reserve_value_capacity_ll(get_size(true));
            char *dest_buf = get_buf();
            insert_constrained_contiguous(dim_begin(), &target_index, subset, array_shape, the_one_chunk->get_rbuf(),dest_buf);
        }
        else {
            // Currently we only handle one-layer simple int/float types. 
            if (is_readable_struct) {
                unsigned long long value_size = get_size(true)*bytes_per_element;
                vector<char> values;
                values.resize(value_size);
                vector<unsigned long long> array_shape = get_shape(false);
                unsigned long target_index = 0;
                vector<unsigned long long> subset;
                insert_constrained_contiguous(dim_begin(), &target_index, subset, array_shape, the_one_chunk->get_rbuf(),values.data());
                read_array_of_structure(values);
            }
            else 
                throw InternalErr(__FILE__, __LINE__, "Only handle integer and float base types. Cannot handle the array of complex structure yet."); 
        }
    }

    set_read_p(true);

    BESDEBUG(dmrpp_3, prolog << " NOT using direct IO : end of this method." << endl);
}

void DmrppArray::read_one_chunk_dio() {

    BESDEBUG(dmrpp_3, prolog << "Using direct IO " << endl);
    // Get the single chunk that makes up this one-chunk compressed variable.
    if (get_chunk_count() != 1)
        throw BESInternalError(string("Expected only a single chunk for variable ") + name(), __FILE__, __LINE__);

    // This is the chunk for this variable.
    auto the_one_chunk = get_immutable_chunks()[0];

    // For this version, we just read the whole chunk all at once.
    the_one_chunk->read_chunk_dio();
    reserve_value_capacity_ll_byte(get_var_chunks_storage_size());
    const char *source_buffer = the_one_chunk->get_rbuf();
    char *target_buffer = get_buf();
    memcpy(target_buffer, source_buffer , the_one_chunk->get_size());

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
    if ((unsigned long long ) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_end = end_element - chunk_origin[dim];

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {

        unsigned int elem_width = bytes_per_element;
        array_offset += chunk_origin[dim];

        // Compute how much we are going to copy
        unsigned long long chunk_bytes = (end_element - chunk_origin[dim] + 1) * bytes_per_element;
        char *source_buffer = chunk->get_rbuf();
        char *target_buffer = nullptr;
        if (is_readable_struct) 
            target_buffer = d_structure_array_buf.data();
        else 
            target_buffer = get_buf();
        memcpy(target_buffer + (array_offset * elem_width), source_buffer + (chunk_offset * elem_width), chunk_bytes);

    }
    else {
        unsigned long long mc = multiplier(chunk_shape, dim);
        unsigned long long ma = multiplier(array_shape, dim);

        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned long long  chunk_index = 0 /*chunk_start*/; chunk_index <= chunk_end; ++chunk_index) {
            unsigned long long next_chunk_offset = chunk_offset + (mc * chunk_index);
            unsigned long long next_array_offset = array_offset + (ma * (chunk_index + chunk_origin[dim]));

            // Re-entry here:
            insert_chunk_unconstrained(chunk, dim + 1, next_array_offset, array_shape, next_chunk_offset, chunk_shape,
                                       chunk_origin);
        }
    }
}

// The direct IO routine to insert the unconstrained chunks.
void DmrppArray::insert_chunk_unconstrained_dio(shared_ptr<Chunk> chunk) {

    const char *source_buffer = chunk->get_rbuf();
    char *target_buffer = get_buf();

    // copy the chunk buffer to the variable buffer at the right location.
    memcpy(target_buffer + chunk->get_direct_io_offset(), source_buffer,chunk->get_size());
 
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
    if (get_chunk_count() < 2)
        throw BESInternalError(string("Expected chunks for variable ") + name(), __FILE__, __LINE__);

    // Find all the required chunks to read. I used a queue to preserve the chunk order, which
    // made using a debugger easier. However, order does not matter, AFAIK.

    unsigned long long sc_count=0;
    stringstream sc_id;
    sc_id << name() << "-" << sc_count++;
    queue<shared_ptr<SuperChunk>> super_chunks;
    auto current_super_chunk = std::make_shared<SuperChunk>(sc_id.str(), this) ;
    super_chunks.push(current_super_chunk);

    // Make the SuperChunks using all the chunks.
    for(const auto& chunk: get_immutable_chunks()) {
        bool added = current_super_chunk->add_chunk(chunk);
        if (!added) {
            sc_id.str(std::string());
            sc_id << name() << "-" << sc_count++;
            current_super_chunk = std::make_shared<SuperChunk>(sc_id.str(), this);
            super_chunks.push(current_super_chunk);
            if (!current_super_chunk->add_chunk(chunk)) {
                stringstream msg ;
                msg << prolog << "Failed to add Chunk to new SuperChunk. chunk: " << chunk->to_string();
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }
        }
    }

    reserve_value_capacity_ll(get_size());
    if (is_readable_struct) {
        d_structure_array_buf.resize(this->length_ll()*bytes_per_element);
    }


    // The size in element of each of the array's dimensions
    const vector<unsigned long long> array_shape = get_shape(true);
    // The size, in elements, of each of the chunk's dimensions
    const vector<unsigned long long> chunk_shape = get_chunk_dimension_sizes();

    
    BESDEBUG(dmrpp_3, prolog << "d_use_transfer_threads: " << (DmrppRequestHandler::d_use_transfer_threads ? "true" : "false") << endl);
    BESDEBUG(dmrpp_3, prolog << "d_max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads << endl);

    if (!DmrppRequestHandler::d_use_transfer_threads) {  // Serial transfers
#if DMRPP_ENABLE_THREAD_TIMERS
        BES_STOPWATCH_START(dmrpp_3, prolog + "Serial SuperChunk Processing.");
#endif
        while(!super_chunks.empty()) {
            auto super_chunk = super_chunks.front();
            super_chunks.pop();
            BESDEBUG(dmrpp_3, prolog << super_chunk->to_string(true) << endl );
            super_chunk->read_unconstrained();
        }
    }
    else {      // Parallel transfers
#if DMRPP_ENABLE_THREAD_TIMERS
        string timer_name = prolog + "Concurrent SuperChunk Processing. d_max_transfer_threads: "  + to_string(DmrppRequestHandler::d_max_transfer_threads);
        BES_STOPWATCH_START(dmrpp_3, timer_name);
#endif
        read_super_chunks_unconstrained_concurrent(super_chunks, this);
    }

    if (is_readable_struct) 
        read_array_of_structure(d_structure_array_buf);
    set_read_p(true);

}

//The direct chunk IO routine of read chunks., mostly copy from the general IO handling routines.
void DmrppArray::read_chunks_dio_unconstrained()
{

    if (get_chunk_count() < 2)
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
    for(const auto& chunk: get_immutable_chunks()) {
        bool added = current_super_chunk->add_chunk(chunk);
        if (!added) {
            sc_id.str(std::string());
            sc_id << name() << "-" << sc_count++;
            current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(),this));
            super_chunks.push(current_super_chunk);
            if (!current_super_chunk->add_chunk(chunk)) {
                stringstream msg ;
                msg << prolog << "Failed to add Chunk to new SuperChunk. chunk: " << chunk->to_string();
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }
        }
    }

    //Change to the total storage buffer size to just the compressed buffer size. 
    reserve_value_capacity_ll_byte(get_var_chunks_storage_size());

    // The size in element of each of the array's dimensions
    const vector<unsigned long long> array_shape = get_shape(true);
    // The size, in elements, of each of the chunk's dimensions
    const vector<unsigned long long> chunk_shape = get_chunk_dimension_sizes();

    BESDEBUG(dmrpp_3, prolog << "d_use_transfer_threads: " << (DmrppRequestHandler::d_use_transfer_threads ? "true" : "false") << endl);
    BESDEBUG(dmrpp_3, prolog << "d_max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads << endl);

    if (!DmrppRequestHandler::d_use_transfer_threads) {  // Serial transfers
#if DMRPP_ENABLE_THREAD_TIMERS
        BES_STOPWATCH_START(dmrpp_3, prolog + "Serial SuperChunk Processing.");
#endif
        while(!super_chunks.empty()) {
            auto super_chunk = super_chunks.front();
            super_chunks.pop();
            BESDEBUG(dmrpp_3, prolog << super_chunk->to_string(true) << endl );

            // Call direct IO routine 
            super_chunk->read_unconstrained_dio();
        }
    }
    else {      // Parallel transfers
#if DMRPP_ENABLE_THREAD_TIMERS
        string timer_name = prolog + "Concurrent SuperChunk Processing. d_max_transfer_threads: " + to_string( DmrppRequestHandler::d_max_transfer_threads);
        BES_STOPWATCH_START(dmrpp_3, timer_name);
#endif
        // Call direct IO routine for parallel transfers
        read_super_chunks_unconstrained_concurrent_dio(super_chunks, this);
    }
    set_read_p(true);
}

//The direct chunk IO routine of read chunks with the buffer chunk, mostly copy from the general IO handling routines.
void DmrppArray::read_buffer_chunks_dio_unconstrained()
{

    if (get_chunk_count() < 2)
        throw BESInternalError(string("Expected chunks for variable ") + name(), __FILE__, __LINE__);

    // We need to pre-calculate the buffer_end_position for each buffer chunk to find the optimial buffer size.
    unsigned long long buffer_offset = 0;

    // The maximum buffer size is set to the current variable size. 
    // This seems an issue for a highly compressed variable. However, it is not since we are
    // going to calculate the optimal buffer size. It will be confined within the file size.
    unsigned long long max_buffer_size = bytes_per_element * this->get_size(false);

    vector<unsigned long long> buf_end_pos_vec;

    // 1. Since the chunks may be filled, we need to find the first non-filled chunk and make the chunk offset
    // as the first buffer offset. The current implementation seems to indicate that the first chunk is always a 
    // chunk that stores the real data. 
    for (const auto &chunk: get_immutable_chunks()) {
        if (chunk->get_offset() != 0) {
            buffer_offset =  chunk->get_offset();
            break;
        }
    }

    auto chunks = this->get_chunks();
    // 2. We need to know the chunk index of the last non-filled chunk to fill in the last real data buffer chunk position.
    unsigned long long last_unfilled_chunk_index = 0;
    for (unsigned long long i = (chunks.size()-1);i>0;i--) {
        if (chunks[i]->get_offset()!=0) {
            last_unfilled_chunk_index = i;
            break;
        }
    }

    BESDEBUG(MODULE, prolog <<" NEW BUFFER maximum buffer size: "<<max_buffer_size<<endl);
    BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_offset: "<<buffer_offset<<endl);

    vector<bool> subset_chunks_needed;

    obtain_buffer_end_pos_vec(subset_chunks_needed,max_buffer_size, buffer_offset, last_unfilled_chunk_index, buf_end_pos_vec);

    // Kent: Old code
    // Follow the general superchunk way.
    unsigned long long sc_count=0;
    stringstream sc_id;
    sc_count++;
    sc_id << name() << "-" << sc_count;
    queue<shared_ptr<SuperChunk>> super_chunks;
    auto current_super_chunk = std::make_shared<SuperChunk>(sc_id.str(), this) ;

    // Set the non-contiguous chunk flag
    current_super_chunk->set_non_contiguous_chunk_flag(true);
    super_chunks.push(current_super_chunk);

    unsigned long long buf_end_pos_counter = 0;
    for (const auto & chunk:chunks) {
    // for (unsigned long long i = 0; i < chunks.size(); i++) {
        //if (chunks_needed[i]){
            //bool added = current_super_chunk->add_chunk_non_contiguous(chunks[i],buf_end_pos_vec[buf_end_pos_counter]);
            bool added = current_super_chunk->add_chunk_non_contiguous(chunk,buf_end_pos_vec[buf_end_pos_counter]);
            if(!added){
                sc_id.str(std::string()); // clears stringstream.
                sc_count++;
                sc_id << name() << "-" << sc_count;
                current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(),this));

                // We need to mark that this superchunk includes non-contiguous chunks.
                current_super_chunk->set_non_contiguous_chunk_flag(true);
                super_chunks.push(current_super_chunk);

                buf_end_pos_counter++;
                //if(!current_super_chunk->add_chunk_non_contiguous(chunks[i],buf_end_pos_vec[buf_end_pos_counter])){
                if(!current_super_chunk->add_chunk_non_contiguous(chunk, buf_end_pos_vec[buf_end_pos_counter])){
                    stringstream msg ;
                    //msg << prolog << "Failed to add chunk to new superchunk. chunk: " << (chunks[i])->to_string();
                    msg << prolog << "Failed to add chunk to new superchunk. chunk: " << chunk->to_string();
                    throw BESInternalError(msg.str(), __FILE__, __LINE__);

                }
            }
        //}
    }


#if 0
    // Find all the required chunks to read. I used a queue to preserve the chunk order, which
    // made using a debugger easier. However, order does not matter, AFAIK.

    unsigned long long sc_count=0;
    stringstream sc_id;
    sc_id << name() << "-" << sc_count++;
    queue<shared_ptr<SuperChunk>> super_chunks;
    auto current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(),this)) ;
    super_chunks.push(current_super_chunk);

    // Make the SuperChunks using all the chunks.
    for(const auto& chunk: get_immutable_chunks()) {
        bool added = current_super_chunk->add_chunk(chunk);
        if (!added) {
            sc_id.str(std::string());
            sc_id << name() << "-" << sc_count++;
            current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(),this));
            super_chunks.push(current_super_chunk);
            if (!current_super_chunk->add_chunk(chunk)) {
                stringstream msg ;
                msg << prolog << "Failed to add Chunk to new SuperChunk. chunk: " << chunk->to_string();
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }
        }
    }
#endif

    //Change to the total storage buffer size to just the compressed buffer size. 
    reserve_value_capacity_ll_byte(get_var_chunks_storage_size());

    while(!super_chunks.empty()) {
        auto super_chunk = super_chunks.front();
        super_chunks.pop();
        super_chunk->read_unconstrained_dio();
    }


    set_read_p(true);
}



// Retrieve data from the linked blocks.  We don't need to use the super chunk technique
// since the adjacent blocks are already combined. We just need to read the data
// from each chunk, combine them and decompress the buffer if necessary.
// Note: HDF4 vdata doesn't have the alignment issue.
void DmrppArray::read_linked_blocks(){

    unsigned int num_linked_blocks = this->get_total_linked_blocks();
    if (num_linked_blocks <2)
        throw BESInternalError("The number of linked blocks must be >1 to read the data.", __FILE__, __LINE__);

    vector<unsigned long long> accumulated_lengths;
    accumulated_lengths.resize(num_linked_blocks);
    vector<unsigned long long> individual_lengths;
    individual_lengths.resize(num_linked_blocks);

    // Here we cannot assume that the index of the linked block always increases
    // in the loop of chunks so we use the linked block index.
    // For the HDF4 case, we observe the index of the linked block is always consistent with the
    //  chunk it loops, though.
    for(const auto& chunk: get_immutable_chunks()) {
        individual_lengths[chunk->get_linked_block_index()] = chunk->get_size();
    }
    accumulated_lengths[0] = 0;
    for (unsigned int i = 1; i < num_linked_blocks; i++)
        accumulated_lengths[i] = individual_lengths[i-1] + accumulated_lengths[i-1];

    if (this->var()->type() == dods_structure_c) {

        // Check if we can handle this case. 
        // Currently we only handle one-layer simple int/float types, and the data is not compressed. 
        if (!is_readable_struct) 
            throw InternalErr(__FILE__, __LINE__, "Only handle integer and float base types. Cannot handle the array of complex structure yet."); 
 
        string filters_str = this->get_filters();
        if (filters_str.find("deflate")!=string::npos) 
            throw InternalErr(__FILE__, __LINE__, "We don't handle compressed array of structure now.");

        vector<char> values;
        values.resize(get_var_chunks_storage_size());
        char *target_buffer = values.data();
    
        for(const auto& chunk: get_immutable_chunks()) {
            chunk->read_chunk();
            BESDEBUG(dmrpp_3, prolog << "linked_block_index: " << chunk->get_linked_block_index() << endl);
            BESDEBUG(dmrpp_3, prolog << "accumlated_length: " << accumulated_lengths[chunk->get_linked_block_index()]  << endl);
            const char *source_buffer = chunk->get_rbuf();
            memcpy(target_buffer + accumulated_lengths[chunk->get_linked_block_index()], source_buffer,chunk->get_size());
        }

        read_array_of_structure(values);
    }
    else {

        //Change to the total storage buffer size to just the compressed buffer size.
        reserve_value_capacity_ll_byte(get_var_chunks_storage_size());

        char *target_buffer = get_buf();
    
        for(const auto& chunk: get_immutable_chunks()) {
            chunk->read_chunk();
            BESDEBUG(dmrpp_3, prolog << "linked_block_index: " << chunk->get_linked_block_index() << endl);
            BESDEBUG(dmrpp_3, prolog << "accumlated_length: " << accumulated_lengths[chunk->get_linked_block_index()]  << endl);
            const char *source_buffer = chunk->get_rbuf();
            memcpy(target_buffer + accumulated_lengths[chunk->get_linked_block_index()], source_buffer,chunk->get_size());
        }
        string filters_string = this->get_filters();
        if (filters_string.find("deflate")!=string::npos) {
    
            char *in_buf = get_buf();
    
            char **destp = nullptr;
            char *dest_deflate = nullptr;
            unsigned long long dest_len = get_var_chunks_storage_size();
            unsigned long long src_len = get_var_chunks_storage_size();
            dest_deflate = new char[dest_len];
            destp = &dest_deflate;
            inflate_simple(destp, dest_len, in_buf, src_len);
            
            this->clear_local_data();
            reserve_value_capacity_ll_byte(this->width_ll());
            char *out_buf = get_buf();
            memcpy(out_buf,dest_deflate,this->width_ll());
            delete []dest_deflate; 
        }
    }


    set_read_p(true);

    // Leave the following commented code for the time being since we may add this feature in the future. KY 2024-03-20
#if 0
    // The size in element of each of the array's dimensions
    const vector<unsigned long long> array_shape = get_shape(true);

    // The size, in elements, of each of the chunk's dimensions
    const vector<unsigned long long> chunk_shape = get_chunk_dimension_sizes();

    BESDEBUG(dmrpp_3, prolog << "d_use_transfer_threads: " << (DmrppRequestHandler::d_use_transfer_threads ? "true" : "false") << endl);
    BESDEBUG(dmrpp_3, prolog << "d_max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads << endl);

    if (!DmrppRequestHandler::d_use_transfer_threads) {  // Serial transfers
#if DMRPP_ENABLE_THREAD_TIMERS
        BES_STOPWATCH_START(dmrpp_3, prolog + "Serial SuperChunk Processing.");
#endif
        while(!super_chunks.empty()) {
            auto super_chunk = super_chunks.front();
            super_chunks.pop();
            BESDEBUG(dmrpp_3, prolog << super_chunk->to_string(true) << endl );

            // Call direct IO routine
            super_chunk->read_unconstrained_dio();
        }
    }
    else {      // Parallel transfers
#if DMRPP_ENABLE_THREAD_TIMERS
        string timer_name = prolog + "Concurrent SuperChunk Processing. d_max_transfer_threads: " + to_string(DmrppRequestHandler::d_max_transfer_threads);
        BES_STOPWATCH_START(dmrpp_3, timer_name);
#endif
        // Call direct IO routine for parallel transfers
        read_super_chunks_unconstrained_concurrent_dio(super_chunks, this);
    }
    set_read_p(true);
#endif

}

void DmrppArray::read_linked_blocks_constrained(){

    unsigned int num_linked_blocks = this->get_total_linked_blocks();
    if (num_linked_blocks <2)
        throw BESInternalError("The number of linked blocks must be >1 to read the data.", __FILE__, __LINE__);

    if (this->var()->type() == dods_structure_c) 
        throw InternalErr(__FILE__, __LINE__, "We don't handle constrained array of structure now.");

    // Gather information of linked blocks 
    vector<unsigned long long> accumulated_lengths;
    accumulated_lengths.resize(num_linked_blocks);
    vector<unsigned long long> individual_lengths;
    individual_lengths.resize(num_linked_blocks);

    // Here we cannot assume that the index of the linked block always increases
    // in the loop of chunks so we use the linked block index.
    // For the HDF4 case, we observe the index of the linked block is always consistent with the
    //  chunk it loops, though.
    for(const auto& chunk: get_immutable_chunks()) {
        individual_lengths[chunk->get_linked_block_index()] = chunk->get_size();
    }
    accumulated_lengths[0] = 0;
    for (unsigned int i = 1; i < num_linked_blocks; i++)
        accumulated_lengths[i] = individual_lengths[i-1] + accumulated_lengths[i-1];

    // Allocate the final constrained buffer
    //size_t array_var_type_size = prototype()->width();
    size_t array_var_type_size = bytes_per_element;
    reserve_value_capacity_ll_byte(array_var_type_size*get_size(true));

    // For the linked block compressed case, we need to obtain the whole buffer to do subsetting. 
    // Since this is the major use case, we will not do any optimiziaton for the uncompressed case.

    vector<char> values;
    values.resize(get_var_chunks_storage_size());
    char *target_buffer = values.data();

    for(const auto& chunk: get_immutable_chunks()) {
        chunk->read_chunk();
        BESDEBUG(dmrpp_3, prolog << "linked_block_index: " << chunk->get_linked_block_index() << endl);
        BESDEBUG(dmrpp_3, prolog << "accumlated_length: " << accumulated_lengths[chunk->get_linked_block_index()]  << endl);
        const char *source_buffer = chunk->get_rbuf();
        memcpy(target_buffer + accumulated_lengths[chunk->get_linked_block_index()], source_buffer,chunk->get_size());
    }

    vector<char>uncompressed_values;
    bool is_compressed = false;
    string filters_string = this->get_filters();

    // The linked blocks are compressed.
    if (filters_string.find("deflate")!=string::npos) {

        char **destp = nullptr;
        char *dest_deflate = nullptr;
        unsigned long long dest_len = get_var_chunks_storage_size();
        unsigned long long src_len = get_var_chunks_storage_size();
        dest_deflate = new char[dest_len];
        destp = &dest_deflate;
        unsigned long long deflated_length = inflate_simple(destp, dest_len, target_buffer, src_len);
        BESDEBUG(dmrpp_3, prolog << "deflated length: " <<  deflated_length << endl);
        BESDEBUG(dmrpp_3, prolog << "array size: " <<  deflated_length << endl);
        
        uncompressed_values.resize(deflated_length);
        memcpy(uncompressed_values.data(),dest_deflate,deflated_length);
#if 0
        uncompressed_values.resize(this->width_ll());
        memcpy(uncompressed_values.data(),dest_deflate,this->width_ll());
#endif
        delete []dest_deflate; 

        is_compressed = true;
    }

    // Now this falls to the contiguous constrained case. We just call the existing method.
    vector<unsigned long long> array_shape = get_shape(false);
    unsigned long target_index = 0;
    vector<unsigned long long> subset;
    char *dest_buf = get_buf();
    if (is_compressed) 
        insert_constrained_contiguous(dim_begin(), &target_index, subset, array_shape, uncompressed_values.data(),dest_buf);
    else 
        insert_constrained_contiguous(dim_begin(), &target_index, subset, array_shape, values.data(), dest_buf);


    set_read_p(true);
}

void DmrppArray::read_chunks_with_linked_blocks() {

    reserve_value_capacity_ll(get_size(false));
    for(const auto& chunk: get_immutable_chunks()) {
        if (chunk->get_multi_linked_blocks()) {
            vector<std::pair<unsigned long long, unsigned long long>>  cur_chunk_lb_offset_lengths;
            chunk->obtain_multi_linked_offset_length(cur_chunk_lb_offset_lengths);
            unsigned long long cb_buffer_size =0;
            for (const auto &tp:cur_chunk_lb_offset_lengths) 
                cb_buffer_size +=tp.second;

            chunk->set_size(cb_buffer_size);
            // Now we get the chunk buffer size, set it.
            if (chunk->get_read_buffer_is_mine()) 
                chunk->set_rbuf_to_size();
            else 
                throw BESInternalError("For multi-linked blocks, the chunk buffer ownship must be true", __FILE__, __LINE__);

            char *temp_cb_buffer = chunk->get_rbuf();

            for (const auto &tp:cur_chunk_lb_offset_lengths) {

                // Obtain this chunk block's offset and length.
                auto cb_offset = tp.first;
                auto cb_length = tp.second;
    
                // Obtain this chunk's other information:byteOrder,URL,chunk position.
                // Create a block chunk for each block in order to obtain the data.
                
                auto cb_data_url = chunk->get_data_url();
                auto cb_position_in_array = chunk->get_position_in_array();
                auto cb_byte_order = chunk->get_byte_order();
                
                // Cannot use the shared pointer here somehow.
                Chunk* block_chunk = nullptr;
                if (cb_data_url == nullptr) 
                    block_chunk = new Chunk(cb_byte_order,cb_length,cb_offset,cb_position_in_array);
                else 
                    block_chunk = new Chunk(cb_data_url,cb_byte_order,cb_length,cb_offset,cb_position_in_array);
                
                block_chunk->read_chunk();
                const char *block_chunk_buffer = block_chunk->get_rbuf();
                if (block_chunk->get_bytes_read() != cb_length) {
                    ostringstream oss;
                    oss << "Wrong number of bytes read for chunk; read: " << block_chunk->get_bytes_read() << ", expected: " << cb_length;
                    throw BESInternalError(oss.str(), __FILE__, __LINE__);
                }
                memcpy(temp_cb_buffer,block_chunk_buffer,cb_length);
                temp_cb_buffer +=cb_length;
                delete block_chunk;
                
            }
            chunk->set_is_read(true);

        }
        else { // General Chunk
            chunk->read_chunk();
        }
        // Now we need to handle the filters.
        if (chunk->get_uses_fill_value()) {
            //No, we won't handle the filled chunks case since HDF4 doesn't have this.
            throw BESInternalError(string("Encounters filled linked-block chunks for variable ") + name(), __FILE__, __LINE__);
        }
        else if (!is_filters_empty())
            chunk->filter_chunk(get_filters(), get_chunk_size_in_elements(), get_bytes_per_element());

        // No, HDF4 doesn't have linked-block chunk structure AFAIK
        if (var()->type() == libdap::dods_structure_c)
            throw BESInternalError(string("Encounters linked-block chunk structures  for variable ") + name(), __FILE__, __LINE__);

        // Now we go to the insert_chunk routine

        vector<unsigned long long> array_shape = get_shape(false);
        vector<unsigned long long> chunk_shape = get_chunk_dimension_sizes();
        vector<unsigned long long> chunk_origin = chunk->get_position_in_array();

        this->insert_chunk_unconstrained(chunk, 0, 0, array_shape, 0,chunk_shape,chunk_origin);

    }
    set_read_p(true);

}

void DmrppArray::read_chunks_with_linked_blocks_constrained() {

    reserve_value_capacity_ll(get_size(true));
    char *dest_buf = this->get_buf();
    vector<unsigned long long> constrained_array_shape = this->get_shape(true);
    for(const auto& chunk: get_immutable_chunks()){
        vector<unsigned long long> chunk_element_address = chunk->get_position_in_array();
        auto needed = find_needed_chunks(0 /* dimension */, &chunk_element_address, chunk);
        if (needed){
            if (chunk->get_multi_linked_blocks()) {
                vector<std::pair<unsigned long long, unsigned long long>>  cur_chunk_lb_offset_lengths;
                chunk->obtain_multi_linked_offset_length(cur_chunk_lb_offset_lengths);
                unsigned long long cb_buffer_size =0;
                for (const auto &tp:cur_chunk_lb_offset_lengths) 
                    cb_buffer_size +=tp.second;
    
                chunk->set_size(cb_buffer_size);
                // Now we get the chunk buffer size, set it.
                if (chunk->get_read_buffer_is_mine()) 
                    chunk->set_rbuf_to_size();
                else 
                    throw BESInternalError("For multi-linked blocks, the chunk buffer ownship must be true", __FILE__, __LINE__);
    
                char *temp_cb_buffer = chunk->get_rbuf();
    
                for (const auto &tp:cur_chunk_lb_offset_lengths) {
    
                    // Obtain this chunk block's offset and length.
                    auto cb_offset = tp.first;
                    auto cb_length = tp.second;
        
                    // Obtain this chunk's other information:byteOrder,URL,chunk position.
                    // Create a block chunk for each block in order to obtain the data.
                    
                    auto cb_data_url = chunk->get_data_url();
                    auto cb_position_in_array = chunk->get_position_in_array();
                    auto cb_byte_order = chunk->get_byte_order();
                    
                    // Cannot use the shared pointer here somehow.
                    Chunk* block_chunk = nullptr;
                    if (cb_data_url == nullptr) 
                        block_chunk = new Chunk(cb_byte_order,cb_length,cb_offset,cb_position_in_array);
                    else 
                        block_chunk = new Chunk(cb_data_url,cb_byte_order,cb_length,cb_offset,cb_position_in_array);
                    
                    block_chunk->read_chunk();
                    const char *block_chunk_buffer = block_chunk->get_rbuf();
                    if (block_chunk->get_bytes_read() != cb_length) {
                        ostringstream oss;
                        oss << "Wrong number of bytes read for chunk; read: " << block_chunk->get_bytes_read() << ", expected: " << cb_length;
                        throw BESInternalError(oss.str(), __FILE__, __LINE__);
                    }
                    memcpy(temp_cb_buffer,block_chunk_buffer,cb_length);
                    temp_cb_buffer +=cb_length;
                    delete block_chunk;
                    
                }
                chunk->set_is_read(true);
    
            }
            else { // General Chunk
                chunk->read_chunk();
            }
            // Now we need to handle the filters.
            if (chunk->get_uses_fill_value()) {
                //No, we won't handle the filled chunks case since HDF4 doesn't have this.
                throw BESInternalError(string("Encounters filled linked-block chunks for variable ") + name(), __FILE__, __LINE__);
            }
            else if (!is_filters_empty())
                chunk->filter_chunk(get_filters(), get_chunk_size_in_elements(), get_bytes_per_element());
    
            // No, HDF4 doesn't have linked-block chunk structure AFAIK
            if (var()->type() == libdap::dods_structure_c)
                throw BESInternalError(string("Encounters linked-block chunk structures  for variable ") + name(), __FILE__, __LINE__);
    
            // Now we go to the insert_chunk routine
            vector<unsigned long long> target_element_address = chunk->get_position_in_array();
            vector<unsigned long long> chunk_source_address(this->dimensions(), 0);
            insert_chunk(0, &target_element_address, &chunk_source_address,chunk, constrained_array_shape, dest_buf);

        }
    }

    set_read_p(true);
 
}

unsigned long long DmrppArray::inflate_simple(char **destp, unsigned long long dest_len, char *src, unsigned long long src_len) {


    /* Sanity check */

    if (src_len == 0) {
        string msg = prolog + "ERROR! The number of bytes to inflate is zero.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    if (dest_len == 0) {
        string msg = prolog + "ERROR! The number of bytes to inflate into is zero.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    if (!destp || !*destp) {
        string msg = prolog + "ERROR! The destination buffer is NULL.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    if (!src) {
        string msg = prolog + "ERROR! The source buffer is NULL.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    /* Input; uncompress */
    z_stream z_strm; /* zlib parameters */

    /* Set the decompression parameters */
    memset(&z_strm, 0, sizeof(z_strm));
    z_strm.next_in = (Bytef *) src;
    z_strm.avail_in = src_len;
    z_strm.next_out = (Bytef *) (*destp);
    z_strm.avail_out = dest_len;

    size_t nalloc = dest_len;

    char *outbuf = *destp;

    /* Initialize the decompression routines */
    if (Z_OK != inflateInit(&z_strm))
        throw BESError("Failed to initialize inflate software.", BES_INTERNAL_ERROR, __FILE__, __LINE__);


    /* Loop to uncompress the buffer */
    int status = Z_OK;
    do {
        /* Uncompress some data */
        status = inflate(&z_strm, Z_SYNC_FLUSH);

        /* Check if we are done decompressing data */
        if (Z_STREAM_END == status) break; /*done*/

        /* Check for error */
        if (Z_OK != status) {
            stringstream err_msg;
            err_msg << "Failed to inflate data chunk.";
            char const *err_msg_cstr = z_strm.msg;
            if(err_msg_cstr)
                err_msg << " zlib message: " << err_msg_cstr;
            (void) inflateEnd(&z_strm);
            throw BESError(err_msg.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
        }
        else {
            // If we're not done and just ran out of buffer space, we need to extend the buffer.
            // We may encounter this case when the deflate filter is used twice. KY 2022-08-03
            if (0 == z_strm.avail_out) {

                /* Allocate a buffer twice as big */
                size_t outbuf_size = nalloc;
                nalloc *= 2;
                char* new_outbuf = new char[nalloc];
                memcpy((void*)new_outbuf,(void*)outbuf,outbuf_size);
                delete[] outbuf;
                outbuf = new_outbuf;

                /* Update pointers to buffer for next set of uncompressed data */
                z_strm.next_out = (unsigned char*) outbuf + z_strm.total_out;
                z_strm.avail_out = (uInt) (nalloc - z_strm.total_out);

            } /* end if */
        } /* end else */
    } while (true /* status == Z_OK */);    // Exit via the break statement after the call to inflate(). jhrg 11/8/21

    *destp = outbuf;
    outbuf = nullptr;
    /* Finish decompressing the stream */
    (void) inflateEnd(&z_strm);

    return z_strm.total_out;
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
unsigned long long DmrppArray::get_chunk_start(const dimension &thisDim, unsigned long long  chunk_origin)
{
    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long first_element_offset = 0; // start with 0
    if ((unsigned long long) (thisDim.start) < chunk_origin) {
        // If the start is behind this chunk, then it's special.
        if (thisDim.stride != 1) {
            // And if the stride isn't 1, we have to figure our where to begin in this chunk.
            first_element_offset = (chunk_origin - thisDim.start) % thisDim.stride;
            // If it's zero great!
            if (first_element_offset != 0) {
                // otherwise, adjust to get correct first element.
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
    if ((unsigned long long) thisDim.start > (chunk_origin[dim] + chunk_shape[dim]) ||
        (unsigned long long) thisDim.stop < chunk_origin[dim]) {
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
    if ((unsigned long long) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_end = end_element - chunk_origin[dim];

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
        // The last chunk may be a filled chunk, chunk->to_string() has an issue to  print the chunk info. So comment it out.
        //BESDEBUG(dmrpp_3, prolog << " END, This is the last_dim. chunk: " << chunk->to_string() << endl);
        return chunk;
    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned long long chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
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
        const vector<unsigned long long> &constrained_array_shape,char *target_buffer){

    // The size, in elements, of each of the chunk's dimensions.
    const vector<unsigned long long> &chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    const vector<unsigned long long> &chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long chunk_start = get_chunk_start(thisDim, chunk_origin[dim]);

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned long long) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_end = end_element - chunk_origin[dim];

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
        char *source_buffer = chunk->get_rbuf();
        unsigned int elem_width = bytes_per_element;

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
               unsigned long long  target_char_start_index =
                        get_index(*target_element_address, constrained_array_shape) * elem_width;
                unsigned long long  chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;

                memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index, elem_width);
            }
        }
    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned long long chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;
            (*chunk_element_address)[dim] = chunk_index;

            // Re-entry here:
            insert_chunk(dim + 1, target_element_address, chunk_element_address, chunk, constrained_array_shape, target_buffer);
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
    if (get_chunk_count() < 2)
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
    //  we might want try adding the rejected Chunk to the other existing SuperChunks to see
    //  if it's contiguous there.
    // Find the required Chunks and put them into SuperChunks.
    bool found_needed_chunks = false;
    for(const auto& chunk: get_immutable_chunks()){
        vector<unsigned long long> target_element_address = chunk->get_position_in_array();
        auto needed = find_needed_chunks(0 /* dimension */, &target_element_address, chunk);
        if (needed){
            found_needed_chunks = true;
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
    BESDEBUG(dmrpp_3, prolog << "found_needed_chunks: " << (found_needed_chunks?"true":"false") << endl);
    if(!found_needed_chunks){  // Ouch! Something went horribly wrong...
        throw BESInternalError("ERROR - Failed to locate any chunks that correspond to the requested data.", __FILE__, __LINE__);
    }

    reserve_value_capacity_ll(get_size(true));
    if (is_readable_struct) 
        d_structure_array_buf.resize(get_size(true)*bytes_per_element);

    BESDEBUG(dmrpp_3, prolog << "d_use_transfer_threads: " << (DmrppRequestHandler::d_use_transfer_threads ? "true" : "false") << endl);
    BESDEBUG(dmrpp_3, prolog << "d_max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads << endl);
    BESDEBUG(dmrpp_3, prolog << "d_use_compute_threads: " << (DmrppRequestHandler::d_use_compute_threads ? "true" : "false") << endl);
    BESDEBUG(dmrpp_3, prolog << "d_max_compute_threads: " << DmrppRequestHandler::d_max_compute_threads << endl);
    BESDEBUG(dmrpp_3, prolog << "SuperChunks.size(): " << super_chunks.size() << endl);

    if (!DmrppRequestHandler::d_use_transfer_threads) {
        // This version is the 'serial' version of the code. It reads a chunk, inserts it,
        // reads the next one, and so on.
#if DMRPP_ENABLE_THREAD_TIMERS
        BES_STOPWATCH_START(dmrpp_3, prolog + "Serial SuperChunk Processing.");
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
        string timer_name = prolog + "Concurrent SuperChunk Processing. d_max_transfer_threads: " + to_string(DmrppRequestHandler::d_max_transfer_threads);
        BES_STOPWATCH_START(dmrpp_3, timer_name);
#endif
        read_super_chunks_concurrent(super_chunks, this);
    }
    if (is_readable_struct)
        read_array_of_structure(d_structure_array_buf);
    set_read_p(true);
}

void DmrppArray::read_buffer_chunks()
{

    BESDEBUG(dmrpp_3, prolog << "coming to read_buffer_chunks()  "  << endl);
    if (get_chunk_count() < 2)
        throw BESInternalError(string("Expected chunks for variable ") + name(), __FILE__, __LINE__);

    // We need to pre-calculate the buffer_end_position for each buffer chunk to find the optimial buffer size.
    unsigned long long buffer_offset = 0;

    // The maximum buffer size is limited to the current constrained domain.
    unsigned long long max_buffer_size = bytes_per_element * this->get_maximum_constrained_buffer_nelmts();

    // TODO: Sometimes the constraint size is too small and the constraint is across quite a few chunks. We may need
    // to enlarge the max_buffer_size to 4K or even 1M. 

    vector<unsigned long long> buf_end_pos_vec;
    bool find_first_non_filled_chunk = true;
    vector<bool> chunks_needed;

    // 1. We have to start somewhere, so we will search the first non_filled buffer offset.
    //    We also obtain the needed chunks info. 
    for (const auto &chunk: get_immutable_chunks()) {
        vector<unsigned long long> target_element_address = chunk->get_position_in_array();
        auto needed = find_needed_chunks(0 /* dimension */, &target_element_address, chunk);
        if (needed) {
            chunks_needed.push_back(true);
        }
        else
            chunks_needed.push_back(false);
        if (needed && find_first_non_filled_chunk){
            if (chunk->get_offset()!=0) {
                buffer_offset =  chunk->get_offset();
                find_first_non_filled_chunk = false;
            }
        }
    }
 
    auto chunks = this->get_chunks();

    // 2. We need to know the chunk index of the last non-filled chunk to fill in the last buffer chunk position.
    unsigned long long last_unfilled_chunk_index = 0;
    for (unsigned long long i = (chunks.size()-1);i>0;i--) {
        if (chunks_needed[i] && chunks[i]->get_offset()!=0) {
            last_unfilled_chunk_index = i;
            break;
        }
    }
    BESDEBUG(MODULE, prolog <<" NEW BUFFER maximum buffer size: "<<max_buffer_size<<endl);
    BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_offset: "<<buffer_offset<<endl);

    obtain_buffer_end_pos_vec(chunks_needed,max_buffer_size, buffer_offset, last_unfilled_chunk_index, buf_end_pos_vec);
// KENT: BUFFER POSITION 
#if 0
    // Loop through the needed chunks to figure out the end position of a buffer. 
    vector <unsigned long long> temp_buffer_pos_vec;
    for (unsigned long long i = 0; i < chunks.size(); i++) {

        if (chunks_needed[i]){

            // We may encounter the filled chunks. those chunks will be handled separately.
            // The offset of a filled chunk is 0.
            // So the buffer end position doesn't matter. We currently also set the buffer end postion of a filled chunk to 0.

            unsigned long long chunk_offset = chunks[i]->get_offset();

            if (chunk_offset != 0) {

                unsigned long long chunk_size = chunks[i]->get_size();

                if (i == last_unfilled_chunk_index) {

                    // We encounter the last non-filled chunk, so this is the last buffer. We mark the end buffer position.
                    unsigned long long buffer_end_pos =  obtain_buffer_end_pos(temp_buffer_pos_vec,chunk_offset+chunk_size);
                    buf_end_pos_vec.push_back(buffer_end_pos);
                    BESDEBUG(MODULE, prolog <<" last_unfilled_chunk index: "<<i<<endl);
                    BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_end_pos: "<<buffer_end_pos<<endl);
                }
                else {

                    // Note: the next chunk may not be the adjacent chunk. It should be the next needed non-filled chunks.
                    //       So we need to calculate. 
                    //       Although we need to have a nested for-loop, for most cases the next one is the adjacent one. So we are OK.
                    unsigned long long next_chunk_offset = (chunks[i+1])->get_offset();
                    if (!chunks_needed[i+1] || chunks[i+1]->get_offset()==0)  {
                        for (unsigned j = i+2; j<chunks.size();j++) {
                            if(chunks_needed[j] && chunks[j]->get_offset()!=0) {
                                next_chunk_offset = (chunks[j])->get_offset();
                                break;
                            }
                        }
                    }

                    long long chunk_gap = next_chunk_offset -(chunk_offset + chunk_size);

                    BESDEBUG(MODULE, prolog <<" NEW BUFFER next_chunk_offset: "<<next_chunk_offset<<endl);
                    BESDEBUG(MODULE, prolog <<" NEW BUFFER chunk_gap: "<<chunk_gap<<endl);

                    //This is not a contiguous super chunk any more.
                    if (chunk_gap != 0) {

                        // Whenever we have a gap, we need to recalculate the buffer size.
                        if (chunk_gap > 0) {

                            // If the non-contiguous chunk is going forward; check if it exceeds the maximum buffer boundary.
                            if (next_chunk_offset >(max_buffer_size + buffer_offset)) {
                                unsigned long long buffer_end_pos = obtain_buffer_end_pos(temp_buffer_pos_vec,chunk_offset+chunk_size);
                                temp_buffer_pos_vec.clear();
    
                                // The current buffer end position
                                buf_end_pos_vec.push_back(buffer_end_pos);

                                BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_end_pos: "<<buffer_end_pos<<endl);
                                // Set the new buffer offset
                                buffer_offset = next_chunk_offset;
                                BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_offset: "<<buffer_offset<<endl);
                            }
                        }
                        else {

                            // If the non-contiguous chunk is going backward, check if it is beyond the first chunk of this buffer.
                            if (next_chunk_offset < buffer_offset) {
                                unsigned long long buffer_end_pos = obtain_buffer_end_pos(temp_buffer_pos_vec,chunk_offset+chunk_size);
                                temp_buffer_pos_vec.clear();
    
                                // The current buffer end position
                                buf_end_pos_vec.push_back(buffer_end_pos);

                                BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_end_pos: "<<buffer_end_pos<<endl);
                                // Set the new buffer offset
                                buffer_offset = next_chunk_offset;
                                BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_offset: "<<buffer_offset<<endl);
                            }
                            else 
                                // When going backward, we need to store the possible buffer end pos.
                                temp_buffer_pos_vec.push_back(chunk_offset+chunk_size);
    
                        }
                    }
                }
            }
            else 
                buf_end_pos_vec.push_back(0);
        }
    }
// END OF BUFFER POSITION
#endif

// KENT: Start debugging
#if 0
   // Prepare buffer size.
    unsigned long long max_buffer_end_position = 0;

    // For highly compressed chunks, we need to make sure the buffer_size is not too big because it may exceed the file size.
    // For this variable we also need to find the maximum value of the end position of all the chunks.
    // Here we try to loop through all the needed chunks for the constraint case.
    bool first_needed_chunk = true;
    unsigned long long first_needed_chunk_offset = 0;
    unsigned long long first_needed_chunk_size = 0;
    for (const auto &chunk: get_immutable_chunks()) {
        vector<unsigned long long> target_element_address = chunk->get_position_in_array();
        auto needed = find_needed_chunks(0 /* dimension */, &target_element_address, chunk);
        if (needed){
            if (first_needed_chunk == true) {
                first_needed_chunk_offset = chunk->get_offset();
                first_needed_chunk_size = chunk->get_size();
                first_needed_chunk = false;
            }
            // We may encounter the filled chunks. Since those chunks will be handled separately.
            // when considering max_buffer_end_position, we should not consider them since
            // the chunk size may be so big that it may  make the buffer exceed the file size.
            // The offset of filled chunk is 0.
            if (chunk->get_offset()!=0) {
                unsigned long long temp_max_buffer_end_position= chunk->get_size() + chunk->get_offset();
                if(max_buffer_end_position < temp_max_buffer_end_position)
                    max_buffer_end_position = temp_max_buffer_end_position;
            }
        }
    }
    if (max_buffer_end_position == 0) 
        throw BESInternalError("ERROR - Failed to locate any chunks that correspond to the requested data.", __FILE__, __LINE__);

    // Here we can adjust the buffer size as needed, for now we just use the whole array size as the starting point.
    // Note: we can further optimize the buffer_size for the constraint case as needed.
    // However, since the buffer_size will be bounded by the offset and length of chunks, it may not be an issue.
    // So just choose the the whole array size first.
    unsigned long long buffer_size = bytes_per_element * this->get_size(false);

    //unsigned long long max_buffer_size = bytes_per_element * this->get_maximum_constrained_buffer_nelmts();
    //BESDEBUG(MODULE, prolog <<  "max_buffer_size:  " << max_buffer_size << endl);
    

     // Make sure buffer_size at least can hold one chunk.
    if (buffer_size < first_needed_chunk_size)
        buffer_size = first_needed_chunk_size;


    // The end position of the buffer should not exceed the max_buffer_end_position.
    unsigned long long buffer_end_position = min((buffer_size + first_needed_chunk_offset),max_buffer_end_position);
#endif 
//Kent end debugging

    unsigned long long sc_count=0;
    stringstream sc_id;
    sc_count++;
    sc_id << name() << "-" << sc_count;
    queue<shared_ptr<SuperChunk>> super_chunks;
    auto current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(), this)) ;

    // Set the non-contiguous chunk flag
    current_super_chunk->set_non_contiguous_chunk_flag(true);
    super_chunks.push(current_super_chunk);

    unsigned long long buf_end_pos_counter = 0;
    for (unsigned long long i = 0; i < chunks.size(); i++) {
        if (chunks_needed[i]){
            bool added = current_super_chunk->add_chunk_non_contiguous(chunks[i],buf_end_pos_vec[buf_end_pos_counter]);
            if(!added){
                sc_id.str(std::string()); // clears stringstream.
                sc_count++;
                sc_id << name() << "-" << sc_count;
                current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(),this));

                // We need to mark that this superchunk includes non-contiguous chunks.
                current_super_chunk->set_non_contiguous_chunk_flag(true);
                super_chunks.push(current_super_chunk);

                buf_end_pos_counter++;
                if(!current_super_chunk->add_chunk_non_contiguous(chunks[i],buf_end_pos_vec[buf_end_pos_counter])){
                    stringstream msg ;
                    msg << prolog << "Failed to add chunk to new superchunk. chunk: " << (chunks[i])->to_string();
                    throw BESInternalError(msg.str(), __FILE__, __LINE__);

                }
            }
        }
    }

    reserve_value_capacity_ll(get_size(true));

    while(!super_chunks.empty()) {
        auto super_chunk = super_chunks.front();
        super_chunks.pop();
        super_chunk->read();
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
void DmrppArray::insert_chunk_serial(unsigned int dim, vector<unsigned long long> *target_element_address, vector<unsigned long long> *chunk_element_address,
    Chunk *chunk)
{
    BESDEBUG("dmrpp", __func__ << " dim: "<< dim << " BEGIN "<< endl);

    // The size, in elements, of each of the chunk's dimensions.
    const vector<unsigned long long> &chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    const vector<unsigned long long> &chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // Do we even want this chunk?
    if ((unsigned long long) thisDim.start > (chunk_origin[dim] + chunk_shape[dim]) || (unsigned long long) thisDim.stop < chunk_origin[dim]) {
        return; // No. No, we do not. Skip this.
    }

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long first_element_offset = get_chunk_start(dim, chunk_origin);

    // Is the next point to be sent in this chunk at all? If no, return.
    if (first_element_offset > chunk_shape[dim]) {
        return;
    }

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned long long) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_start = first_element_offset; //start_element - chunk_origin[dim];
    unsigned long long chunk_end = end_element - chunk_origin[dim];
    vector<unsigned long long> constrained_array_shape = get_shape(true);

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

            unsigned long long target_char_start_index = get_index(*target_element_address, constrained_array_shape) * elem_width;
            unsigned long long chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;

            memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index, chunk_constrained_inner_dim_bytes);
        }
        else {
            // Stride != 1
            for (unsigned long long chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
                // Compute where we need to put it.
                (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;

                // Compute where we are going to read it from
                (*chunk_element_address)[dim] = chunk_index;

                unsigned long long target_char_start_index = get_index(*target_element_address, constrained_array_shape) * elem_width;
                unsigned long long  chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;

                memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index, elem_width);
            }
        }
    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned long long chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
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
    reserve_value_capacity_ll(get_size(true));

    /*
     * Find the chunks to be read, make curl_easy handles for them, and
     * stuff them into our curl_multi handle. This is a recursive activity
     * which utilizes the same code that copies the data from the chunk to
     * the variables.
     */
    for (unsigned long long i = 0; i < chunk_refs.size(); i++) {
        Chunk &chunk = chunk_refs[i];

        vector<unsigned long long> chunk_source_address(dimensions(), 0);
        vector<unsigned long long> target_element_address = chunk.get_position_in_array();

        // Recursive insertion operation.
        insert_chunk_serial(0, &target_element_address, &chunk_source_address, &chunk);
    }

    set_read_p(true);

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for " << name() << " END"<< endl);
}
#endif

void
DmrppArray::set_send_p(bool state)
{
    if (!get_attributes_loaded())
        load_attributes(this);

    Array::set_send_p(state);
}

/**
 * @brief Process String Array so long as it has only one element
 *
 * This method is pretty limited, but this is a common case and the DMR++
 * will need more information for the Dmrpp handler to support the general
 * case of an N-dimensional array.
 */
void DmrppArray::read_contiguous_string()
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing array name: "+name());

    // This is the original chunk for this 'contiguous' variable.
    auto the_one_chunk = get_immutable_chunks()[0];

    // Read the the_one_chunk as is. This is the non-parallel I/O case
    the_one_chunk->read_chunk();

    // Now that the_one_chunk has been read, we do what is necessary...
    if (!is_filters_empty() && !get_one_chunk_fill_value()){
        the_one_chunk->filter_chunk(get_filters(), get_chunk_size_in_elements(), var()->width_ll());
    }

    // FIXME This part will only work if the array contains a single element. See below.
    //  jhrg 3/3/22
    vector < string > ss;      // Prepare for the general case
    string s(reinterpret_cast<char *>(the_one_chunk->get_rbuf()));
    ss.push_back(s);
    set_value(ss, ss.size());

    set_read_p(true);
}

string DmrppArray::ingest_fixed_length_string(const char *buf, const unsigned long long fixed_str_len, string_pad_type pad_type)
{
    string value;
    unsigned long long str_len = 0;
    switch(pad_type){
        case null_pad:
        case null_term:
        {
            while( str_len < fixed_str_len && buf[str_len]!=0 ){
                str_len++;
            }
            BESDEBUG(MODULE, prolog << DmrppArray::pad_type_to_str(pad_type) << " scheme. str_len: " << str_len << endl);
            value = string(buf,str_len);
            break;
        }
        case space_pad:
        {
            str_len = fixed_str_len;
            while(  str_len>0 && (buf[str_len-1]==' ' || buf[str_len-1]==0)){
                str_len--;
            }
            BESDEBUG(MODULE, prolog << DmrppArray::pad_type_to_str(pad_type) << " scheme. str_len: " << str_len << endl);
            value = string(buf,str_len);
            break;
        }
        case not_set:
        default:
            // Do nothing.
            BESDEBUG(MODULE, prolog << "pad_type: NOT_SET" << endl);
            break;
    }
    BESDEBUG(MODULE, prolog << "value: '" << value << "'" << endl);
    return value;
}

string dims_to_string(const vector<unsigned long long> dims){
    stringstream ss;
    for(auto dim: dims){
        ss << "[" << dim << "]";
    }
    return ss.str();
}

std::string array_to_str(DmrppArray a, const string &banner)  {
    stringstream msg;
    msg << endl << "#  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -" << endl;
    msg << "# " << banner << endl;
    msg << "# " << a.prototype()->type_name() << " " << a.name();

    for(auto dim=a.dim_begin(); dim < a.dim_end(); dim++){
        msg << "[";
        if(!dim->name.empty()){
            msg << dim->name << "=";
        }
        msg << dim->size << "]";
    }
    msg << endl;
    msg << "# " << endl;
    msg << "#           a->get_size(true): " << a.get_size(true) << " (The total number of elements in the array instance)" << endl;
    msg << "#              a->width(true): " << a.width(true) << " (The number of bytes needed to hold the entire array - prot.width * num_elements)" << endl;
    msg << "#             a->length(true): " << a.length() << " (The number of elements in the vector)" << endl;
    msg << "# a->prototype()->width(true): " << a.prototype()->width() << " (Width of the template variable)" << endl;
    msg << "#         a->dimensions(true): " << a.dimensions(true) << endl;
    msg << "# a->chunk_dimension_sizes" << dims_to_string(a.get_chunk_dimension_sizes()) << endl;
    msg << "#                 a->length(): " << a.length() << endl;
    return msg.str();
}

#define HEX( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x )

std::string show_string_buff(char *buff, unsigned long long num_bytes, unsigned long long fixed_string_len) {
    stringstream ss;
    for (unsigned long long i = 0; i < num_bytes; i += fixed_string_len) {
        char *str_ptr = buff + i;
        if (i) { ss << ", "; }
        ss << "{";
        for (unsigned long long j = 0; j < fixed_string_len; j++) {
            char this_char = *(str_ptr + j);
            if (j) { ss << ", "; }
            if (this_char > 32 && this_char < 126) {
                ss << this_char;
            } else {
                ss << "0x" << std::hex << HEX(this_char) << std::dec;
            }
        }
        ss << "}";
    }
    return ss.str();
}

/**
 * Takes the passed array and construsts a DmrppArray of bytes
 * the should be able to read all of the data for the array into the
 * memory biffer correctly. This kind of "recast" is of little use
 * for nominal atomic types, but is very useful for things like
 * arrays of fixed length strings.
 * @param array
 * @return A DmrppArray of Byte that can be used to read the data
 * represented by the passed array.
 */
DmrppArray *get_as_byte_array(DmrppArray &array){

    Type var_type;
    var_type = array.prototype()->type();

    auto *byte_array_proxy = dynamic_cast<DmrppArray *>(array.ptr_duplicate());
    if(!byte_array_proxy){
        throw BESInternalFatalError(prolog + "Server encountered internal state ambiguity. "
                                             "Expected valid DmrppArray pointer. Exiting.",
                                    __FILE__, __LINE__);
    }

    unsigned long long item_size=0;
    if ((var_type == dods_str_c || var_type == dods_url_c)) {
        if (array.is_flsa()) {
            BESDEBUG(MODULE, prolog << "Processing Fixed Length String Array data." << endl);
            item_size = byte_array_proxy->get_fixed_string_length();
            BESDEBUG(MODULE, prolog << "get_fixed_string_length(): " << item_size << endl);
        }
        else {
            // VLSA would have a size of one byte??
            item_size = 1;
        }
    }
    else {
        item_size = byte_array_proxy->prototype()->width();
    }

    unsigned long long total_bytes = byte_array_proxy->length() * item_size;
    BESDEBUG(MODULE, prolog << "total_bytes: " << total_bytes << endl);

    BESDEBUG(MODULE, prolog << array_to_str(*byte_array_proxy,"Source DmrppArray") );

    // Replace prototype
    auto *tmp_proto  = new libdap::Byte(byte_array_proxy->prototype()->name());
    byte_array_proxy->set_prototype(tmp_proto);

    // bytes_per_element should be updated since the proto is updated. 
    byte_array_proxy->set_bytes_per_element(byte_array_proxy->prototype()->width());
    tmp_proto->set_parent(byte_array_proxy);

    // Fiddle Chunk dimension sizes
    auto cdim_sizes = byte_array_proxy->get_chunk_dimension_sizes();
    if(!cdim_sizes.empty()) {
        BESDEBUG(MODULE, prolog << "original chunk_dimension_sizes.back(): " << dims_to_string(cdim_sizes) << endl);

        auto new_last_cdim_size = cdim_sizes.back() * item_size;
        cdim_sizes.pop_back();
        cdim_sizes.emplace_back(new_last_cdim_size);
        BESDEBUG(MODULE, prolog << "New chunk_dimension_sizes" << dims_to_string(cdim_sizes) << endl);

        byte_array_proxy->set_chunk_dimension_sizes(cdim_sizes);
        BESDEBUG(MODULE, prolog << "Updated chunk_dimension_sizes"
                                << dims_to_string(byte_array_proxy->get_chunk_dimension_sizes()) << endl);
    }

    // Fiddle Each chunk's chunk_position_in_array to reflect the change in array element count
    unsigned long long chunk_index = 0;
    for(const auto &chunk: byte_array_proxy->get_immutable_chunks()){
        auto cpia = chunk->get_position_in_array();
        if (!cpia.empty()) {
            auto new_position = cpia.back() * item_size;
            cpia.pop_back();
            cpia.emplace_back(new_position);
            BESDEBUG(MODULE,
                     prolog << "Chunk[" << chunk_index << "] new chunk_position_in_array" << dims_to_string(cpia)
                            << endl);
            chunk->set_position_in_array(cpia);
            BESDEBUG(MODULE, prolog << "Chunk[" << chunk_index << "] UPDATED chunk_position_in_array"
                                    << dims_to_string(chunk->get_position_in_array()) << endl);
        }
        chunk_index++;
    }

    auto t_last_dim = byte_array_proxy->dim_end() - 1;

    BESDEBUG(MODULE, prolog << "Orig last_dim->size: " << t_last_dim->size << endl);

    t_last_dim->size = t_last_dim->size * item_size;
    BESDEBUG(MODULE, prolog << "New last_dim->size: " << t_last_dim->size << endl);

    t_last_dim->c_size = t_last_dim->size;
    BESDEBUG(MODULE, prolog << "New last_dim->c_size: " << t_last_dim->c_size << endl);

    t_last_dim->start = 0;
    BESDEBUG(MODULE, prolog << "New last_dim->start: " << t_last_dim->start << endl);

    t_last_dim->stop = t_last_dim->size - 1;
    BESDEBUG(MODULE, prolog << "New last_dim->stop: " << t_last_dim->stop << endl);

    t_last_dim->stride = 1;
    BESDEBUG(MODULE, prolog << "New last_dim->stride: " << t_last_dim->stride << endl);

    byte_array_proxy->set_length(total_bytes);
    t_last_dim = byte_array_proxy->dim_end() - 1;
    BESDEBUG(MODULE, prolog << "Updated last_dim->size: " << t_last_dim->size << endl);

    BESDEBUG(MODULE, prolog << array_to_str(*byte_array_proxy,"New DmrppArray of Byte") );

    return byte_array_proxy;

}

/**
 * Reads the string data for the fixed length string array flsa from the
 * data buffer of the data array into which it was read.
 * @param flsa
 * @param data
 */
void ingest_flsa_data(DmrppArray &flsa, DmrppArray &data)
{
    if (flsa.is_flsa()) {
        BESDEBUG(MODULE, prolog << "Ingesting Fixed Length String Array Data." << endl);
        auto fstr_len = flsa.get_fixed_string_length();
        BESDEBUG(MODULE, prolog << "flsa.get_fixed_string_length(): " << fstr_len << endl);

        auto pad_type = flsa.get_fixed_length_string_pad();
        BESDEBUG(MODULE, prolog << "flsa.get_fixed_length_string_pad_str(): " << flsa.get_fixed_length_string_pad_str() << endl);

        auto buff = data.get_buf();
        BESDEBUG(MODULE, prolog << "data.get_buf(): " << (void *) buff << endl);
        if(buff == nullptr){
            throw BESInternalError("Failed to acquire byte buffer from which to read string array data.",__FILE__,__LINE__);
        }
        unsigned long long num_bytes = data.length();
        BESDEBUG(MODULE, prolog << "Buffer contains: " << show_string_buff(buff, num_bytes, fstr_len) << endl);

        auto begin = buff;
        char *end = buff + num_bytes;
        while (begin < end) {
            string value = DmrppArray::ingest_fixed_length_string(begin, fstr_len, pad_type);
            flsa.get_str().push_back(value);
            BESDEBUG(MODULE, prolog << "Added String: '" << value << "'" << endl);
            begin += fstr_len;
        }
    }

}


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
    Type var_type = this->var()->type();
    // If the chunks are not loaded, load them now. NB: load_chunks()
    // reads data for HDF5 COMPACT storage, so read_p() will be true
    // (but it does not read any other data). Thus, call load_chunks()
    // before testing read_p() to cover that case. jhrg 11/15/21
    // String Arrays that use COMPACT storage appear to work. jhrg 3/3/22
    if (!get_chunks_loaded())
        load_chunks(this);

    // It's important to note that w.r.t. the compact data layout the DMZ parser reads the values into the
    // DmrppArray at the time it is parsed and the read flag is then set. Thus, the compact layout solution
    // does not explicitly appear in this method as it is handled by the parser.
    if (read_p()) 
        return true;

    // If it is zero size array, we just return empty data.
    if (length_ll() == 0) 
        return true;
#if 0
    // Here we need to reset the dio_flag to false for the time being before calling the method use_direct_io_opt()
    // since the dio_flag may be set to true for reducing the memory usage with a temporary solution. 
    // TODO: we need to reset the direct io flag to false and change back in the future. KY 2023-11-29
    this->set_dio_flag(false);

    // Add direct_io offset for each chunk. This will be used to retrieve individal buffer at fileout netCDF.
    // Direct io offset is only necessary when the direct IO operation is possible.
    if (this->use_direct_io_opt()) { 

        this->set_dio_flag();
        auto chunks = this->get_chunks();

        // Need to provide the offset of a chunk in the final data buffer.
        for (unsigned int i = 0; i<chunks.size();i++) {
            if (i > 0) 
               chunks[i]->set_direct_io_offset(chunks[i-1]->get_direct_io_offset()+chunks[i-1]->get_size());
            BESDEBUG(MODULE, prolog << "direct_io_offset is: " << chunks[i]->get_direct_io_offset() << endl);
        }

        // Fill in the chunk information so that the fileout netcdf can retrieve.
        Array::var_storage_info dmrpp_vs_info;
        dmrpp_vs_info.filter = this->get_filters();
    
        // Provide the deflate compression levels.
        for (const auto &def_lev:this->get_deflate_levels())
            dmrpp_vs_info.deflate_levels.push_back(def_lev);
        
        // Chunk dimension sizes.
        for (const auto &chunk_dim:this->get_chunk_dimension_sizes())
            dmrpp_vs_info.chunk_dims.push_back(chunk_dim);
        
        // Provide chunk offset/length etc. 
        auto im_chunks = this->get_immutable_chunks();
        for (const auto &chunk:im_chunks) {
            Array::var_chunk_info_t vci_t;
            vci_t.filter_mask = chunk->get_filter_mask();
            vci_t.chunk_direct_io_offset = chunk->get_direct_io_offset();
            vci_t.chunk_buffer_size = chunk->get_size();
    
            for (const auto &chunk_coord:chunk->get_position_in_array())
                vci_t.chunk_coords.push_back(chunk_coord);           
            dmrpp_vs_info.var_chunk_info.push_back(vci_t);
        }
        this->set_var_storage_info(dmrpp_vs_info);
    }
#endif

    if (this->get_dio_flag()) {
        BESDEBUG(MODULE, prolog << "dio is turned  on" << endl);

        Array::var_storage_info dmrpp_vs_info = this->get_var_storage_info();

        auto chunks = this->get_chunks();

        // Need to provide the offset of a chunk in the final data buffer.
        for (unsigned int i = 0; i<chunks.size();i++) {
            if (i > 0) 
               chunks[i]->set_direct_io_offset(chunks[i-1]->get_direct_io_offset()+chunks[i-1]->get_size());
            BESDEBUG(MODULE, prolog << "direct_io_offset is: " << chunks[i]->get_direct_io_offset() << endl);
        }

        // Fill in the chunk information so that the fileout netcdf can retrieve.
        // Provide chunk offset/length etc. 
        auto im_chunks = this->get_immutable_chunks();
        for (const auto &chunk:im_chunks) {
            Array::var_chunk_info_t vci_t;
            vci_t.filter_mask = chunk->get_filter_mask();
            vci_t.chunk_direct_io_offset = chunk->get_direct_io_offset();
            vci_t.chunk_buffer_size = chunk->get_size();
    
            for (const auto &chunk_coord:chunk->get_position_in_array())
                vci_t.chunk_coords.push_back(chunk_coord);           
            dmrpp_vs_info.var_chunk_info.push_back(vci_t);
        }
        this->set_var_storage_info(dmrpp_vs_info);
        bytes_per_element = this->var()->width_ll();
    }
    else {
        is_readable_struct = check_struct_handling();       
        if (is_readable_struct) {
            vector<unsigned int> s_off = this->get_struct_offsets();
            if (s_off.empty())
                bytes_per_element = this->var()->width_ll(); 
            else 
                bytes_per_element = s_off.back();
        }
        else 
            bytes_per_element = this->var()->width_ll();
    }
    

    DmrppArray *array_to_read = this;
    if ((var_type == dods_str_c || var_type == dods_url_c)) {
        if (is_flsa()) {
            // For fixed length string we use a proxy array of Byte to retrieve the data.
            array_to_read = get_as_byte_array(*this);
        }
    }
    try {
        if(BESDebug::IsSet(MODULE)) {
            string msg = array_to_str(*array_to_read, "Reading Data From DmrppArray");
            BESDEBUG(MODULE, prolog << msg << endl);
        }
        // Single chunk and 'contiguous' are the same for this code.
        if (array_to_read->get_chunk_count() == 1) {
            BESDEBUG(MODULE, prolog << "Reading data from a single contiguous chunk." << endl);
            // KENT: here we need to add the handling of direct chunk IO for one chunk. 
            if (this->get_dio_flag())
                array_to_read->read_one_chunk_dio();
            else 
                array_to_read->read_contiguous();    // Throws on various errors
        }
        else {  // Handle the more complex case where the data is chunked.
            if (get_using_linked_block()) {
                BESDEBUG(MODULE, prolog << "Reading data linked blocks" << endl);
                if (!array_to_read->is_projected()) {
                    array_to_read->read_linked_blocks();
                }
                else {
                    array_to_read->read_linked_blocks_constrained();
#if 0
                    throw BESInternalFatalError(prolog + "Not support data subset when linked blocks are used. ",
                                           __FILE__, __LINE__);
#endif

                }
            }
            else if (is_multi_linked_blocks_chunk()) {
                if (!array_to_read->is_projected()) {
                    array_to_read->read_chunks_with_linked_blocks();
                }
                else {
                    array_to_read->read_chunks_with_linked_blocks_constrained();
#if 0
                    throw BESInternalFatalError(prolog + "Not support data subset when linked blocks are used. ",
                                           __FILE__, __LINE__);
#endif
                }
            }
            else {
                bool buffer_chunk_case = array_to_read->use_buffer_chunk();
            
                if (!array_to_read->is_projected()) {
                    BESDEBUG(MODULE, prolog << "Reading data from chunks, unconstrained." << endl);
                    // KENT: Only here we need to consider the direct buffer IO.
                    if (this->get_dio_flag()) {
                        BESDEBUG(MODULE, prolog << "Using direct IO" << endl);
                        if (buffer_chunk_case && DmrppRequestHandler::use_buffer_chunk) 
                            array_to_read->read_buffer_chunks_dio_unconstrained();
                        else 
                            array_to_read->read_chunks_dio_unconstrained();
                    }
                    // Also buffer chunks for the non-contiguous chunk case.
                    else if(buffer_chunk_case && DmrppRequestHandler::use_buffer_chunk) { 
                        BESDEBUG(MODULE, prolog << "Using buffer chunk" << endl);
                        array_to_read->read_buffer_chunks_unconstrained();
                    }
                    else {
                        BESDEBUG(MODULE, prolog << "Using general approach" << endl);
                        array_to_read->read_chunks_unconstrained();
                    }
                } else {
                    BESDEBUG(MODULE, prolog << "Reading data from chunks, constrained." << endl);

                    // Also buffer chunks for the non-contiguous chunk case.
                    if (buffer_chunk_case && DmrppRequestHandler::use_buffer_chunk)  {
                        BESDEBUG(MODULE, prolog << "Using buffer chunk" << endl);
                        array_to_read->read_buffer_chunks();
                    }
                    else { 
                        BESDEBUG(MODULE, prolog << "Using general approach" << endl);
                        array_to_read->read_chunks();
                    }
                }
            }
        }

        if ((var_type == dods_str_c || var_type == dods_url_c)) {
            BESDEBUG(MODULE, prolog << "Processing Array of Strings." << endl);
            if(array_to_read == this){
                throw BESInternalFatalError(prolog + "Server encountered internal state conflict. "
                                                     "Expected byte transport array. Exiting.",
                                            __FILE__, __LINE__);
            }

            if (is_flsa()) {
                ingest_flsa_data(*this, *array_to_read);
            }
            else {
                BESDEBUG(MODULE, prolog << "Processing Variable Length String Array data. SKIPPING..." << endl);
#if 0 // @TODO Turn this on...
                ingest_vlsa_data(*this, *array_to_read);
#else
                throw BESInternalError("Arrays of variable length strings are not yet supported.",__FILE__,__LINE__);
#endif
            }
        }
        if(array_to_read && array_to_read != this) {
            delete array_to_read;
            array_to_read = nullptr;
        }

    }
    catch(...){
        if(array_to_read && array_to_read != this) {
            delete array_to_read;
            array_to_read = nullptr;
        }
        throw;
    }

    if (this->twiddle_bytes()) {

        int64_t num = this->length_ll();

        switch (var_type) {
            case dods_int16_c:
            case dods_uint16_c: {
                auto *local = reinterpret_cast<dods_uint16*>(this->get_buf());
                while (num--) {
                    *local = bswap_16(*local);
                    local++;
                }
                break;
            }
            case dods_int32_c:
            case dods_uint32_c: {
                auto *local = reinterpret_cast<dods_uint32*>(this->get_buf());;
                while (num--) {
                    *local = bswap_32(*local);
                    local++;
                }
                break;
            }
            case dods_int64_c:
            case dods_uint64_c: {
                auto *local = reinterpret_cast<dods_uint64*>(this->get_buf());;
                while (num--) {
                    *local = bswap_64(*local);
                    local++;
                }
                break;
            }
            case dods_float32_c: {
                swap_float32(this->get_buf(), num);
                break;
            }
            case dods_float64_c: {
                swap_float64(this->get_buf(), num);
                break;
            }
            default: break; // Do nothing for all other types.
        }
    }

    return true;
}

unsigned long long DmrppArray::set_fixed_string_length(const string &length_str)
{
    try {
        d_fixed_str_length = stoull(length_str);
    }
    catch(std::invalid_argument e){
        stringstream err_msg;
        err_msg << "The value of the length string could not be parsed. Message: " << e.what();
        throw BESInternalError(err_msg.str(),__FILE__,__LINE__);
    }
    return d_fixed_str_length;
}


std::string pad_to_str(string_pad_type pad)
{
    string pad_str;
    switch(pad){
        case null_term:
            pad_str = "null_term";
            break;
        case null_pad:
            pad_str = "null_pad";
            break;
        case space_pad:
            pad_str = "space_pad";
            break;
        case not_set:
            pad_str = "not_set";
            break;
        default:
            throw BESInternalError("ERROR: Unrecognized HDF5 String Padding Scheme!",__FILE__,__LINE__);
            break;
    }
    return pad_str;
}


std::string DmrppArray::pad_type_to_str(string_pad_type pad)
{
    return pad_to_str(pad);
}

string_pad_type str_to_pad_type(const string &pad_str){
    string_pad_type pad_type(not_set);
    if(pad_str=="null_pad"){
        pad_type = null_pad;
    }
    else if(pad_str=="null_term") {
        pad_type = null_term;
    }
    else if (pad_str == "space_pad"){
        pad_type = space_pad;
    }
    else if (pad_str == "not_set"){
        pad_type = not_set;
    }
    else {
        stringstream err_msg;
        err_msg << "The value of the pad string was not recognized. pad_str: " << pad_str;
        throw BESInternalError(err_msg.str(),__FILE__,__LINE__);
    }
    return pad_type;
}


string_pad_type DmrppArray::set_fixed_length_string_pad_type(const string &pad_str)
{
    d_fixed_length_string_pad_type = str_to_pad_type(pad_str);
    return d_fixed_length_string_pad_type;
}


ons::ons(const std::string &ons_pair_str) {
    const string colon(":");
    size_t colon_pos = ons_pair_str.find(colon);

    string offset_str = ons_pair_str.substr(0, colon_pos);
    offset = stoull(offset_str);

    string size_str = ons_pair_str.substr(colon_pos + 1);
    size = stoull(size_str);
}


void DmrppArray::set_ons_string(const std::string &ons_str)
{
    d_vlen_ons_str = ons_str;
}

void DmrppArray::set_ons_string(const vector<ons> &ons_pairs)
{
    stringstream ons_ss;
    bool first = true;
    for(auto &ons_pair: ons_pairs){
        if(!first){
            ons_ss << ",";
        }
        ons_ss << ons_pair.offset << ":" << ons_pair.size;
    }
    d_vlen_ons_str = ons_ss.str();
}


/**
 * Ingests the (possibly long) ons (offset and size) string that itemizes every offset
 * and size for the members of variable length string array and creates from the string
 * offset:size pairs
 * @param ons_str
 * @param vlen_str_addrs
 */
void DmrppArray::get_ons_objs(vector<ons> &ons_pairs)
{
    const string comma(",");
    size_t last = 0;
    size_t next = 0;

    while ((next = d_vlen_ons_str.find(comma, last)) != string::npos) {
        string ona_pair_str = d_vlen_ons_str.substr(last, next-last);
        ons ons_pair(ona_pair_str);
        ons_pairs.push_back(ons_pair);
        last = next + 1;
    }
    // @TODO - Inspect this once we are doing the real implementation
    //   and make sure the "tail" is handled correctly.
    cout << d_vlen_ons_str.substr(last) << endl;
}

/**
 * @brief Write a Fixed Length String Array into the dmr++ document as an XML element with values.
 * <dmrpp:FixedLengthStringArray string_length="##" pad="null_pad | null_term | space_pad" />
 * @param xml
 * @param a
 */
void flsa_xml_element(XMLWriter &xml, DmrppArray &a){

    string element_name("dmrpp:FixedLengthStringArray");
    string str_len_attr_name("string_length");
    string pad_attr_name("pad");

    if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar *) element_name.c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write " + element_name + " element");

    stringstream strlen_str;
    strlen_str << a.get_fixed_string_length();
    if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) str_len_attr_name.c_str(),
                                    (const xmlChar *) strlen_str.str().c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write attribute for 'string_length'");

    if (a.get_fixed_length_string_pad() == not_set) {
        throw BESInternalError("ERROR: Padding Scheme Has Not Been Set!", __FILE__, __LINE__);
    }
    string pad_str = a.pad_type_to_str(a.get_fixed_length_string_pad());
    if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "pad",
                                    (const xmlChar *) pad_str.c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write attribute for 'pad'");

    if (xmlTextWriterEndElement(xml.get_writer()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not end " + a.type_name() + " element");
}


/**
 * Write hdf5 compact data types into the dmr++ document.
 * @param xml
 * @param a
 */
void compact_data_xml_element(XMLWriter &xml, DmrppArray &a) {
    switch (a.var()->type()) {
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
            uint8_t *values = nullptr;
            try {
                auto size = a.buf2val(reinterpret_cast<void **>(&values));
                string encoded = base64::Base64::encode(values, size);
                a.print_compact_element(xml, DmrppCommon::d_ns_prefix, encoded);
                delete[] values;
            }
            catch (...) {
                delete[] values;
                throw;
            }
            break;
        }

        case dods_str_c:
        case dods_url_c: {
            auto sb = a.compact_str_buffer();
            if(!sb.empty()) {
                uint8_t *values = nullptr;
                try {
                    auto size = a.buf2val(reinterpret_cast<void **>(&values));
                    string encoded = base64::Base64::encode(values, size);
                    a.print_compact_element(xml, DmrppCommon::d_ns_prefix, encoded);
                    delete[] values;
                }
                catch (...) {
                    delete[] values;
                    throw;
                }
            }
            break;
        }

        default:
            throw InternalErr(__FILE__, __LINE__, "Vector::val2buf: bad type");
    }
}

bool obtain_compress_encode_data(size_t num_elms, string &encoded_str, const Bytef*source_data,size_t source_data_size, string &err_msg) {

    if (num_elms  == 1) {
        encoded_str = base64::Base64::encode(source_data,(int)source_data_size);
    }
    else {
        auto ssize = (uLong)source_data_size;
        auto csize = (uLongf)ssize*2;
        vector<Bytef> compressed_src;
        compressed_src.resize(source_data_size*2);
    
        int retval = compress(compressed_src.data(), &csize, source_data, ssize);
        if (retval != 0) {
            err_msg = "Fail to compress the data";
            return false;
        }
        encoded_str = base64::Base64::encode(compressed_src.data(),(int)csize);
    }

    return true;

}

void missing_data_xml_element(const XMLWriter &xml, DmrppArray *da) {
    switch (da->var()->type()) {
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
        case dods_float32_c:
        case dods_float64_c: {
            auto source_data_src = (const Bytef *) (da->get_buf());
            
            size_t source_data_size = da->width_ll();
            string encoded_str;
            string err_msg;
            if (false == obtain_compress_encode_data(da->get_size(false),encoded_str,source_data_src,source_data_size,err_msg)) {
                err_msg = "variable name: " + da->name() + " "+err_msg;  
                throw InternalErr(__FILE__, __LINE__, err_msg);
            }

            da->print_missing_data_element(xml, DmrppCommon::d_ns_prefix, encoded_str);
            break;
        }

        default:
            throw InternalErr(__FILE__, __LINE__, "Vector::val2buf: bad type");
    }
}

void special_structure_array_data_xml_element(const XMLWriter &xml, DmrppArray *da) {

    if (da->var()->type() == dods_structure_c) {
        vector<char> struct_array_str_buf = da->get_structure_array_str_buffer();
        string final_encoded_str = base64::Base64::encode((uint8_t*)(struct_array_str_buf.data()),struct_array_str_buf.size());
        da->print_special_structure_element(xml, DmrppCommon::d_ns_prefix, final_encoded_str);
    }

}

/**
 * @brief Print information about one dimension of the array.
 * @param xml
 * @param constrained
 * @param d
 */
static void print_dap4_dimension_helper(const XMLWriter &xml, bool constrained, const Array::dimension &d) {
    // This duplicates code in D4Dimensions (where D4Dimension::print_dap4() is defined
    // because of the need to print the constrained size of a dimension). I think that
    // the constraint information has to be kept here and not in the dimension (since they
    // are shared dims). Could hack print_dap4() to take the constrained size, however.
    if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar *) "Dim") < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write Dim element");

    string name = (d.dim) ? d.dim->fully_qualified_name() : d.name;
    // If there is a name, there must be a Dimension (named dimension) in scope
    // so write its name but not its size.
    if (!constrained && !name.empty()) {
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "name",
                                        (const xmlChar *) name.c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
    }
    else if (d.use_sdim_for_slice) {
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "name",
                                        (const xmlChar *) name.c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
    }
    else {
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "size",
                                        (const xmlChar *) (constrained ? to_string(d.c_size) : to_string(
                                                d.size)).c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
    }

    if (xmlTextWriterEndElement(xml.get_writer()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not end Dim element");
}

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
void DmrppArray::print_dap4(XMLWriter &xml, bool constrained /*false*/) {
    if (constrained && !send_p()) return;

    if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar *) var()->type_name().c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write " + type_name() + " element");

    if (!name().empty()) {
        BESDEBUG(MODULE, prolog << "variable full path: " << FQN() << endl);
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "name", (const xmlChar *) name().c_str()) <
            0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
    }

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
        for_each(c.var_begin(), c.var_end(), [&xml, constrained](BaseType *btp) { btp->print_dap4(xml, constrained); });
    }

    // Drop the local_constraint which is per-array and use a per-dimension on instead
    for_each(dim_begin(), dim_end(), [&xml, constrained](const Array::dimension &d) {
        print_dap4_dimension_helper(xml, constrained, d);
    });

    attributes()->print_dap4(xml);

    for_each(maps()->map_begin(), maps()->map_end(), [&xml](D4Map *m) { m->print_dap4(xml); });

    // Only print the chunks' info if there. This is the code added to libdap::Array::print_dap4().
    // jhrg 5/10/18
    // Update: print the <chunks> element even if the chinks_size value is zero since this
    // might be a variable with all fill values. jhrg 4/24/22
    if (DmrppCommon::d_print_chunks && (get_chunk_count() > 0 || get_uses_fill_value()))
        print_chunks_element(xml, DmrppCommon::d_ns_prefix);

    // If this variable uses the COMPACT layout, encode the values for
    // the array using base64. Note that strings are a special case; each
    // element of the array is a string and is encoded in its own base64
    // xml element. So, while an array of 10 int32 will be encoded in a
    // single base64 element, an array of 10 strings will use 10 base64
    // elements. This is because the size of each string's value is different.
    // Not so for an int32.
    if (DmrppCommon::d_print_chunks && is_compact_layout() && read_p()) {
        compact_data_xml_element(xml, *this);
    }

    if (DmrppCommon::d_print_chunks && is_missing_data() && read_p()) {
        missing_data_xml_element(xml, this);
    }

    // Special structure string array.
    if (DmrppCommon::d_print_chunks && get_special_structure_flag() && read_p()) {
        special_structure_array_data_xml_element(xml, this);
    }



    // Is it an array of strings? Those have issues so we treat them special.
    if (var()->type() == dods_str_c) {
        if (is_flsa() && DmrppCommon::d_print_chunks) {

            // Write the dmr++ for Fix Length String Array
            flsa_xml_element(xml, *this);
        }
        else if (is_vlsa() && DmrppCommon::d_print_chunks) {
            // Write the dmr++ for Variable Length String Array
            vlsa::write(xml, *this);
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

unsigned int DmrppArray::buf2val(void **val){

    if (!val) {
        throw BESInternalError("NULL pointer encountered.", __FILE__, __LINE__);
    }
    if ( var()->type()==dods_str_c  ||  var()->type()==dods_url_c ) {

        auto str_buf = compact_str_buffer();
        auto buf_size = str_buf.size();
        if (str_buf.empty()) {
            stringstream msg;
            msg << prolog << "Logic error: called when cardinal type data buffer was empty!";
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }
        if (!*val) {
            *val = new char[buf_size];
        }
        memcpy(*val, str_buf.data(), buf_size);
        return buf_size;
    } else {
        return (unsigned int)Vector::buf2val_ll(val);
    }
}

// Check if direct chunk IO can be used. 
bool DmrppArray::use_direct_io_opt() {

    bool ret_value = false;
    bool is_integer_le_float = false;

    if (DmrppRequestHandler::is_netcdf4_enhanced_response && this->is_filters_empty() == false) {
        Type t = this->var()->type();
        if (libdap::is_simple_type(t) && t != dods_str_c && t != dods_url_c && t!= dods_enum_c && t!=dods_opaque_c) {
            is_integer_le_float = true;
            if(is_integer_type(t) && this->get_byte_order() =="BE")
                is_integer_le_float = false;
        }
    }

    bool no_constraint = false;

    // Check if it requires a subset of this variable.
    if (is_integer_le_float) {
        no_constraint = true;
        if (this->is_projected())
            no_constraint = false;
    }

    bool has_deflate_filter = false;

    // Check if having the deflate filters.
    if (no_constraint) {
        string filters_string = this->get_filters();
        if (filters_string.find("deflate")!=string::npos)
            has_deflate_filter = true;
    }

    bool is_data_all_fvalues = false;
    // This is the check for a rare case: the variable data just contains the filled values.
    // If this var's storage size is 0. Then it should be filled with the filled values.
    if (has_deflate_filter && this->get_uses_fill_value() && this->get_var_chunks_storage_size() == 0) 
            is_data_all_fvalues = true;

    bool has_dio_filters = false;

    // If the deflate level is not provided, we cannot do the direct IO.
    if (has_deflate_filter && !is_data_all_fvalues) {
        if (this->get_deflate_levels().empty() == false)
            has_dio_filters = true; 
    }

    // Check if the chunk size is greater than the dimension size for any dimension.
    // If this is the case, we will not use the direct chunk IO since netCDF-4 doesn't allow this.
    // TODO later, if the dimension is unlimited, this restriction can be lifted. Current dmrpp doesn't store the
    // unlimited dimension information.

    if (has_dio_filters && this->get_processing_fv_chunks() == false) {

        vector <unsigned long long>chunk_dim_sizes = this->get_chunk_dimension_sizes();
        vector <unsigned long long>dim_sizes;
        Dim_iter p = dim_begin();
        while (p != dim_end()) {
            dim_sizes.push_back((unsigned long long)dimension_size_ll(p));
            p++;
        }

        bool chunk_less_dim = true;
        if (chunk_dim_sizes.size() == dim_sizes.size()) {
            for (unsigned int i = 0; i<dim_sizes.size(); i++) {
                if (chunk_dim_sizes[i] > dim_sizes[i]) {
                     chunk_less_dim = false;
                     break;
                }
            }
        }
        else
            chunk_less_dim = false;

        ret_value = chunk_less_dim;
    }
         
    return ret_value;

} 

// Read the data from the supported array of structure
void DmrppArray::read_array_of_structure(vector<char> &values) {

    size_t values_offset = 0;
    int64_t nelms = this->length_ll();
    if (this->twiddle_bytes()) 
        BESDEBUG(dmrpp_3, prolog << "swap bytes " << endl);

    vector<unsigned int> s_offs = this->get_struct_offsets();

    for (int64_t element = 0; element < nelms; ++element) {

        auto dmrpp_s = dynamic_cast<DmrppStructure*>(var()->ptr_duplicate());
        if(!dmrpp_s)
            throw InternalErr(__FILE__, __LINE__, "Cannot obtain the structure pointer."); 
        try {
            dmrpp_s->set_struct_offsets(s_offs);
            dmrpp_s->structure_read(values,values_offset, this->twiddle_bytes());
        }
        catch(...) {
            delete dmrpp_s;
            string err_msg = "Cannot read the data of a dmrpp structure variable " + var()->name();
            throw InternalErr(__FILE__, __LINE__, err_msg); 
        }
        dmrpp_s->set_read_p(true);
        set_vec_ll((uint64_t)element,dmrpp_s);
        delete dmrpp_s;
    }

    set_read_p(true);

}

// Check if this DAP4 structure is what we can support.
bool DmrppArray::check_struct_handling() {

    bool ret_value = true;

    if (this->var()->type() == dods_structure_c) {

        auto array_base = dynamic_cast<DmrppStructure*>(this->var());
        Constructor::Vars_iter vi = array_base->var_begin();
        Constructor::Vars_iter ve = array_base->var_end();
        for (; vi != ve; vi++) { 

            BaseType *bt = *vi;
            Type t_bt = bt->type();

            // Only support array or scalar of float/int.
            if (libdap::is_simple_type(t_bt) == false) {

                if (t_bt == dods_array_c) {

                    auto t_a = dynamic_cast<Array *>(bt);
                    Type t_array_var = t_a->var()->type();
                    if (!libdap::is_simple_type(t_array_var) || t_array_var == dods_str_c || t_array_var == dods_url_c || t_array_var == dods_enum_c || t_array_var==dods_opaque_c) {
                        ret_value = false;
                        break;
                    }
                }
            }
            else if (t_bt == dods_str_c || t_bt == dods_url_c || t_bt == dods_enum_c || t_bt == dods_opaque_c) {
                ret_value = false;
                break;
            }
        }
    }
    else
        ret_value = false;

    return ret_value;
}

void DmrppArray::read_buffer_chunks_unconstrained() {

    BESDEBUG(dmrpp_3, prolog << "coming to read_buffer_chunks_unconstrained()  "  << endl);

    
    if (get_chunk_count() < 2)
        throw BESInternalError(string("Expected chunks for variable ") + name(), __FILE__, __LINE__);

    // We need to pre-calculate the buffer_end_position for each buffer chunk to find the optimial buffer size.
    unsigned long long buffer_offset = 0;

    // The maximum buffer size is set to the current variable size. 
    // This seems an issue for a highly compressed variable. However, it is not since we are
    // going to calculate the optimal buffer size. It will be confined within the file size.
    unsigned long long max_buffer_size = bytes_per_element * this->get_size(false);

    vector<unsigned long long> buf_end_pos_vec;

    // 1. Since the chunks may be filled, we need to find the first non-filled chunk and make the chunk offset
    // as the first buffer offset. The current implementation seems to indicate that the first chunk is always a 
    // chunk that stores the real data. 
    for (const auto &chunk: get_immutable_chunks()) {
        if (chunk->get_offset() != 0) {
            buffer_offset =  chunk->get_offset();
            break;
        }
    }

    auto chunks = this->get_chunks();
    // 2. We need to know the chunk index of the last non-filled chunk to fill in the last real data buffer chunk position.
    unsigned long long last_unfilled_chunk_index = 0;
    for (unsigned long long i = (chunks.size()-1);i>0;i--) {
        if (chunks[i]->get_offset()!=0) {
            last_unfilled_chunk_index = i;
            break;
        }
    }

    BESDEBUG(MODULE, prolog <<" NEW BUFFER maximum buffer size: "<<max_buffer_size<<endl);
    BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_offset: "<<buffer_offset<<endl);

    vector<bool> subset_chunks_needed;

    obtain_buffer_end_pos_vec(subset_chunks_needed,max_buffer_size, buffer_offset, last_unfilled_chunk_index, buf_end_pos_vec);

    // Kent: Old code
    // Follow the general superchunk way.
    unsigned long long sc_count=0;
    stringstream sc_id;
    sc_count++;
    sc_id << name() << "-" << sc_count;
    queue<shared_ptr<SuperChunk>> super_chunks;
    auto current_super_chunk = std::make_shared<SuperChunk>(sc_id.str(), this) ;

    // Set the non-contiguous chunk flag
    current_super_chunk->set_non_contiguous_chunk_flag(true);
    super_chunks.push(current_super_chunk);

#if 0
    auto array_chunks = get_immutable_chunks();

    // Make sure buffer_size at least can hold one chunk.
    if (buffer_size < (array_chunks[0])->get_size())
        buffer_size = (array_chunks[0])->get_size();

    unsigned long long max_buffer_end_position = 0;

    // For highly compressed chunks, we need to make sure the buffer_size is not too big because it may exceed the file size.
    // For this variable we also need to find the maximum value of the end position of all the chunks.
    for (const auto &chunk: array_chunks) {

        // We may encounter the filled chunks. Since those chunks will be handled separately.
        // when considering max_buffer_end_position, we should not consider them since
        // the chunk size may be so big that it may make the buffer exceed the file size.
        // The offset of filled chunk is 0.
        if (chunk->get_offset()!=0) {
            unsigned long long temp_max_buffer_end_position= chunk->get_size() + chunk->get_offset();
            if(max_buffer_end_position < temp_max_buffer_end_position)
                max_buffer_end_position = temp_max_buffer_end_position;
        }
    }
    
    // The end position of the buffer should not exceed the max_buffer_end_position.
    unsigned long long buffer_end_position = min((buffer_size + (array_chunks[0])->get_offset()),max_buffer_end_position);

    BESDEBUG(dmrpp_3, prolog << "variable name:  "  << this->name() <<endl);
    BESDEBUG(dmrpp_3, prolog << "maximum buffer_end_position:  "  << max_buffer_end_position <<endl);

    // Make the SuperChunks using all the chunks.
    for(const auto& chunk: get_immutable_chunks()) {
        bool added = current_super_chunk->add_chunk_non_contiguous(chunk,buffer_end_position);
        if (!added) {
            sc_id.str(std::string());
            sc_count++;
            sc_id << name() << "-" << sc_count;
            current_super_chunk = std::make_shared<SuperChunk>(sc_id.str(), this);
            // We need to mark this superchunk includes non-contiguous chunks.
            current_super_chunk->set_non_contiguous_chunk_flag(true);
            super_chunks.push(current_super_chunk);

            // Here we need to make sure buffer_size is not too small although this rarely happens.
            if (buffer_size < chunk->get_size())
                buffer_size = chunk->get_size();
            buffer_end_position = min((buffer_size + chunk->get_offset()),max_buffer_end_position);
            if (!current_super_chunk->add_chunk_non_contiguous(chunk,buffer_end_position)) {
                stringstream msg ;
                msg << prolog << "Failed to add Chunk to new SuperChunk for non-contiguous chunks. chunk: " << chunk->to_string();
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }
        }
    }

#endif 

    unsigned long long buf_end_pos_counter = 0;
    for (const auto & chunk:chunks) {
    // for (unsigned long long i = 0; i < chunks.size(); i++) {
        //if (chunks_needed[i]){
            //bool added = current_super_chunk->add_chunk_non_contiguous(chunks[i],buf_end_pos_vec[buf_end_pos_counter]);
            bool added = current_super_chunk->add_chunk_non_contiguous(chunk,buf_end_pos_vec[buf_end_pos_counter]);
            if(!added){
                sc_id.str(std::string()); // clears stringstream.
                sc_count++;
                sc_id << name() << "-" << sc_count;
                current_super_chunk = shared_ptr<SuperChunk>(new SuperChunk(sc_id.str(),this));

                // We need to mark that this superchunk includes non-contiguous chunks.
                current_super_chunk->set_non_contiguous_chunk_flag(true);
                super_chunks.push(current_super_chunk);

                buf_end_pos_counter++;
                //if(!current_super_chunk->add_chunk_non_contiguous(chunks[i],buf_end_pos_vec[buf_end_pos_counter])){
                if(!current_super_chunk->add_chunk_non_contiguous(chunk, buf_end_pos_vec[buf_end_pos_counter])){
                    stringstream msg ;
                    //msg << prolog << "Failed to add chunk to new superchunk. chunk: " << (chunks[i])->to_string();
                    msg << prolog << "Failed to add chunk to new superchunk. chunk: " << chunk->to_string();
                    throw BESInternalError(msg.str(), __FILE__, __LINE__);

                }
            }
        //}
    }


    reserve_value_capacity_ll(get_size());

    while(!super_chunks.empty()) {
        auto super_chunk = super_chunks.front();
        super_chunks.pop();
        super_chunk->read_unconstrained();
    }

    set_read_p(true);
}

bool DmrppArray::use_buffer_chunk() {

    bool ret_value = false;
    auto chunks = this->get_chunks();

    // For our use case, we only need to check if the first chunk and the second chunk are adjacent.
    // Since we find quite a few cases that the chunks are not adjacent in the middle, this causes the expensive 
    // cloud access several times even with super chunks. So we will try to use the buffer chunk for those cases too. KY 2025-11-20
    // To make the process clear and simple, we don't handle structure data.
    // Also when all the chunks are filled with the fill values, we should not use the buffer chunk.
    if (chunks.size() >1 && this->var()->type() !=dods_structure_c && this->get_var_chunks_storage_size()!=0){
        for (unsigned i = 0; i < (chunks.size()-1); i++) {
            unsigned long long chunk_offset = chunks[i]->get_offset();
            unsigned long long chunk_size = chunks[i]->get_size();
            unsigned long long next_chunk_offset = (chunks[i+1])->get_offset();
            if ((chunk_offset + chunk_size) != next_chunk_offset) {
                ret_value = true;
                break;
            }
        }
    }

    return ret_value;
}

void DmrppArray::obtain_buffer_end_pos_vec(const vector<bool>& subset_chunks_needed, unsigned long long max_buffer_size, unsigned long long buffer_offset,
                                           unsigned long long last_unfilled_chunk_index, vector<unsigned long long> & buf_end_pos_vec) const {

    bool select_whole_array = false;
    if (subset_chunks_needed.empty())
        select_whole_array = true;

    auto chunks = this->get_chunks();

    // Loop through the needed chunks to figure out the end position of a buffer. 
    vector <unsigned long long> temp_buffer_pos_vec;
    for (unsigned long long i = 0; i < chunks.size(); i++) {

        bool chunks_needed = select_whole_array?true:subset_chunks_needed[i];
        if (chunks_needed){

            // We may encounter the filled chunks. those chunks will be handled separately.
            // The offset of a filled chunk is 0.
            // So the buffer end position doesn't matter. We currently also set the buffer end postion of a filled chunk to 0.

            unsigned long long chunk_offset = chunks[i]->get_offset();

            if (chunk_offset != 0) {

                unsigned long long chunk_size = chunks[i]->get_size();

                if (i == last_unfilled_chunk_index) {

                    // We encounter the last non-filled chunk, so this is the last buffer. We mark the end buffer position.
                    unsigned long long buffer_end_pos =  obtain_buffer_end_pos(temp_buffer_pos_vec,chunk_offset+chunk_size);
                    buf_end_pos_vec.push_back(buffer_end_pos);
                    BESDEBUG(MODULE, prolog <<" last_unfilled_chunk index: "<<i<<endl);
                    BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_end_pos: "<<buffer_end_pos<<endl);
                }
                else {

                    // Note: the next chunk may not be the adjacent chunk. It should be the next needed non-filled chunks.
                    //       So we need to calculate. 
                    //       Although we need to have a nested for-loop, for most cases the next one is the adjacent one. So we are OK.
                    unsigned long long next_chunk_offset = (chunks[i+1])->get_offset();

                    bool next_chunk_needed = select_whole_array?true:subset_chunks_needed[i+1];
                    if (!next_chunk_needed || chunks[i+1]->get_offset()==0)  {
                    //if (!chunks_needed[i+1] || chunks[i+1]->get_offset()==0)  {
                        for (unsigned long long j = i+2; j<chunks.size();j++) {
                            bool temp_chunk_needed = select_whole_array?true:subset_chunks_needed[j];
                            //if(chunks_needed[j] && chunks[j]->get_offset()!=0) {
                            if(temp_chunk_needed  && chunks[j]->get_offset()!=0) {
                                next_chunk_offset = (chunks[j])->get_offset();
                                break;
                            }
                        }
                    }

                    long long chunk_gap = next_chunk_offset -(chunk_offset + chunk_size);

                    BESDEBUG(MODULE, prolog <<" NEW BUFFER next_chunk_offset: "<<next_chunk_offset<<endl);
                    BESDEBUG(MODULE, prolog <<" NEW BUFFER chunk_gap: "<<chunk_gap<<endl);

                    //This is not a contiguous super chunk any more.
                    if (chunk_gap != 0) {

                        // Whenever we have a gap, we need to recalculate the buffer size.
                        if (chunk_gap > 0) {

                            // If the non-contiguous chunk is going forward; check if it exceeds the maximum buffer boundary.
                            if (next_chunk_offset >(max_buffer_size + buffer_offset)) {
                                unsigned long long buffer_end_pos = obtain_buffer_end_pos(temp_buffer_pos_vec,chunk_offset+chunk_size);
                                temp_buffer_pos_vec.clear();
    
                                // The current buffer end position
                                buf_end_pos_vec.push_back(buffer_end_pos);

                                BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_end_pos: "<<buffer_end_pos<<endl);
                                // Set the new buffer offset
                                buffer_offset = next_chunk_offset;
                                BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_offset: "<<buffer_offset<<endl);
                            }
                        }
                        else {

                            // If the non-contiguous chunk is going backward, check if it is beyond the first chunk of this buffer.
                            if (next_chunk_offset < buffer_offset) {
                                unsigned long long buffer_end_pos = obtain_buffer_end_pos(temp_buffer_pos_vec,chunk_offset+chunk_size);
                                temp_buffer_pos_vec.clear();
    
                                // The current buffer end position
                                buf_end_pos_vec.push_back(buffer_end_pos);

                                BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_end_pos: "<<buffer_end_pos<<endl);
                                // Set the new buffer offset
                                buffer_offset = next_chunk_offset;
                                BESDEBUG(MODULE, prolog <<" NEW BUFFER buffer_offset: "<<buffer_offset<<endl);
                            }
                            else 
                                // When going backward, we need to store the possible buffer end pos.
                                temp_buffer_pos_vec.push_back(chunk_offset+chunk_size);
    
                        }
                    }
                }
            }
            else 
                buf_end_pos_vec.push_back(0);
        }
    }
}

unsigned long long DmrppArray::obtain_buffer_end_pos(const vector<unsigned long long>& t_buf_end_pos_vec, unsigned long long cur_buf_end_pos) const {

    for (const auto &t_buf_end_pos:t_buf_end_pos_vec) {
        if (cur_buf_end_pos <t_buf_end_pos)
            cur_buf_end_pos = t_buf_end_pos;
    }
    return cur_buf_end_pos;

}

} // namespace dmrpp
