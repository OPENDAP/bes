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

// TODO Fix this comment. It seems like this scans the list of futures for a ready future and
//  returns true when it find one and if they all time out, it returns false. But that's not what it does.
//  It scans the list of futures until one is found that is ready and returns true when that is found.
//  But if one is found that is not valid, it returns false for that.
//  Simplify this. jhrg 2/7/24
/**
 * @brief Uses future::wait_for() to scan the futures for a ready future, returning true when once get() has been called.
 *
 * When a valid, ready future is found future::get() is called and the thead_counter is decremented.
 * Returns true when it successfully "get"s a future (joins a thread), or if a future turns up as not valid, then
 * it is discarded and the thread is ":finished" and true is returned.
 *
 * @param futures The list of futures to scan
 * @param thread_counter This counter will be decremented when a future is "finished".
 * @param timeout The number of milliseconds to wait for each future to complete.
 * @param debug_prefix This is a string tag used in debugging to associate the particular debug call with the calling
 * method.
 * @return Returns true if future::get() was called on a ready future, false otherwise.
 */
bool get_next_future(list<std::future<bool>> &futures, atomic_uint &thread_counter, unsigned long timeout,
                     const string &debug_prefix) {
    bool future_finished = false;
    bool done = false;
    std::chrono::milliseconds timeout_ms(timeout);

    while (!done) {
        auto futr = futures.begin();
        auto fend = futures.end();
        // TODO If this loop finds a future that is not valid, that indicates an error. However,
        //  as the loop is written, if the first future examined is not valid, the function will
        //  return true (because when future_is_valid is false, future_finished is set to true,
        //  the loop exits and then function returns the value of future_finished). jhrg 1/31/24
        bool future_is_valid = true;
        // TODO This could be a range-based for that uses a break to exit when a complete future is found.
        while (!future_finished && future_is_valid && futr != fend) {
            future_is_valid = (*futr).valid();
            if (future_is_valid) {
                // What happens if wait_for() always returns future_status::timeout for a stuck thread?
                // If that were to happen, the loop would run forever. However, we assume that these
                // threads are never 'stuck.' We assume that their computations always complete, either
                // with success or failure. For the transfer threads, timeouts will stop them if nothing
                // else does and for the decompression threads, the worst case is a segmentation fault.
                // jhrg 2/5/21
                if ((*futr).wait_for(timeout_ms) != std::future_status::timeout) {
                    try {
                        bool success = (*futr).get();
                        future_finished = true;
                        BESDEBUG(dmrpp_3, debug_prefix << prolog << "Called future::get() on a ready future."
                                                       << " success: " << (success ? "true" : "false") << endl);
                        if (!success) {
                            stringstream msg;
                            msg << debug_prefix << prolog << "The std::future has failed!";
                            msg << " thread_counter: " << thread_counter;
                            throw BESInternalError(msg.str(), __FILE__, __LINE__);
                        }
                    }
                    catch (...) {
                        // TODO I had to add this to make the thread counting work when there's errors
                        //  But I think it's primitive because it trashes everything - there's
                        //  surely a way to handle the situation on a per thread basis and maybe even
                        //  retry?
                        // TODO This (futures.clear()) does not stop running tasks in the futures. The code can wait
                        //  for each future and call get() or use a shared atomic as a flag, check the
                        //  flag in the task in the future and exit if a condition is met. jhrg 1/31/24
                        //  See read_super_chunks_concurrent() for am example of how to clean up the list of futures.
                        //  jhrg 1/31/24
                        futures.clear();
                        thread_counter = 0;
                        throw;
                    }
                }
                else {
                    futr++;
                    BESDEBUG(dmrpp_3, debug_prefix << prolog << "future::wait_for() timed out. (timeout: " <<
                                                   timeout << " ms) There are currently " << futures.size()
                                                   << " futures in process."
                                                   << " thread_counter: " << thread_counter << endl);
                }
            }
            else {
                BESDEBUG(dmrpp_3, debug_prefix << prolog << "The future was not valid. Dumping... " << endl);
                future_finished = true;
            }
        }

        // TODO There is no way for futr == fend and futrue_finished == true. Since the function
        //  only waits for one future, this could be moved up into the above else clause and the
        //  bool future_finished could be removed. jhrg 1/31/24
        if (futr != fend && future_finished) {
            futures.erase(futr);
            thread_counter--;
            BESDEBUG(dmrpp_3, debug_prefix << prolog << "Erased future from futures list. (Erased future was "
                                           << (future_is_valid ? "" : "not ") << "valid at start.) There are currently "
                                           <<
                                           futures.size() << " futures in process. thread_counter: " << thread_counter
                                           << endl);
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
 * @param arg_list A pointer to a one_child_chunk_args
 */
// TODO Since this code is for contiguous data, the offset for 'the_one_chunk'
//  (maybe better named 'contiguous_block'?) is always zero. jhrg 1/31/24
bool one_child_chunk_thread_new(const unique_ptr<one_child_chunk_args_new> &args) {
    args->child_chunk->read_chunk();

    one_child_chunk_thread_new_sanity_check(args.get());

    // the_one_chunk offset \/
    // the_one_chunk:  mmmmmmmmmmmmmmmm
    // child chunks:   1111222233334444 (there are four child chunks)
    // child offsets:  ^   ^   ^   ^
    // For this example, child_1_offset - the_one_chunk_offset == 0 (that's always true)
    // child_2_offset - the_one_chunk_offset == 4; child_3_offset - the_one_chunk_offset == 8
    // and child_4_offset - the_one_chunk_offset == 12.
    // Those are the starting locations with in the data buffer of the the_one_chunk
    // where that child chunk should be written.
    // Note: all the offset values start at the beginning of the file.

    unsigned long long offset_within_the_one_chunk =
            args->child_chunk->get_offset() - args->the_one_chunk->get_offset();

    memcpy(args->the_one_chunk->get_rbuf() + offset_within_the_one_chunk, args->child_chunk->get_rbuf(),
           args->child_chunk->get_bytes_read());

    return true;
}

// TODO It owuld be easier to follow this if the function used 'contiguous'
//  instead of 'one_child_chunk' in the name.
//  Also, since the code uses futures and not pthreads, the structures are
//  not needed since async can take more than one argument for the task. jhrg 1/31/24
//  And, this is used once (in void DmrppArray::read_contiguous()) so if we modify it,
//  only that code need to be changed.
//  It can be made static. jhrg 1/31/24
bool start_one_child_chunk_thread(list<std::future<bool>> &futures, unique_ptr<one_child_chunk_args_new> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck (transfer_thread_pool_mtx);
    if (transfer_thread_counter < DmrppRequestHandler::d_max_transfer_threads) {
        transfer_thread_counter++;
        // TODO The move here is not needed. jhrg 1/31/24
        futures.push_back(std::async(std::launch::async, one_child_chunk_thread_new, std::move(args)));
        retval = true;

        // The args may be null after move(args) is called and causes the segmentation fault in the following BESDEBUG.
        // So remove that part but leave the futures.size() for bookkeeping.
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() <<endl);
    }
    return retval;
}
// TODO The pattern here where one_super_chunk_transfer_thread() is used by
//  start_super_chunk_transfer_thread() whiich is used by read_super_chunks_concurrent()
//  which is used by DmrppArray::read_chunks() which is used by DmrppArray:read()
//  is _likely_ repeated closely by read_super_chunks_unconstrained_concurrent() and
//  read_super_chunks_unconstrained_concurrent_dio() that follow. I have looked at the
//  code and that seems to be the case.
//
// TODO If we continue to stop using parallel transfers, all of this software can be
//  eliminated. Suppose we remove it to a new file and drop the parallel transfers?
//  Given the logic in read(), read_chunks(), ..., it will be easy enough to add the
//  feature back in later.
//  Roughly the first 550 LOC are these functions! jhrg 1/31/24
/**
 * @brief A single argument wrapper for process_super_chunk() for use with std::async().
 * @param args A unique_ptr to an instance of one_super_chunk_args.
 * @return True unless an exception is throw in which case neither true or false apply.
 */
bool one_super_chunk_transfer_thread(const unique_ptr<one_super_chunk_args> &args) {

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

// TODO This is used only in read_super_chunks_concurrent() below. jhrg 1/31/24
/**
 * @brief Starts the super_chunk_thread function using std::async() and places the returned future in the queue futures.
 * @param futures The queue into which to place the std::future returned by std::async().
 * @param args The arguments for the super_chunk_thread function
 * @return Returns true if the std::async() call was made and a future was returned, false if the
 * transfer_thread_counter has reached the maximum allowable size.
 */
bool start_super_chunk_transfer_thread(list<std::future<bool>> &futures, unique_ptr<one_super_chunk_args> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck(transfer_thread_pool_mtx);
    // TODO This logic supports calling start_super_chunk_transfer_thread() from multiple locations,
    //  but that is not possible - read_super_chunks_concurrent() which is the only caller of this
    //  is only called by DmrppArray::read_chunks() in one place and read_chunks is only called in
    //  one place by DmrppArray:read(). Thus, we can move the thread counter into this function as a
    //  local variable. jhrg 1/31/24
    //  Idea: why not use the number of elements in the list of futures to manage the number of threads?
    //  jhrg 1/31/24
    if (transfer_thread_counter < DmrppRequestHandler::d_max_transfer_threads) {
        transfer_thread_counter++;
        futures.push_back(std::async(std::launch::async, one_super_chunk_transfer_thread, std::move(args)));
        retval = true;

        // The args may be null after move(args) is called and causes the segmentation fault in the following BESDEBUG.
        // So remove that part but leave the futures.size() for bookkeeping.
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() << endl);
    }
    return retval;
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
void read_super_chunks_concurrent(queue<shared_ptr<SuperChunk> > &super_chunks, DmrppArray *array) {
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + " name: " + array->name(), "");

    // Parallel version based on read_chunks_unconstrained(). There is
    // substantial duplication of the code in read_chunks_unconstrained(), but
    // wait to remove that when we move to C++11 which has threads integrated.

    // We maintain a list  of futures to track our parallel activities.
    // TODO Rewrite this nested loop so that the exit criteria is 'if both the queue of SuperChunks and
    //  the list of futures are empty, the work is complete.
    list<future<bool>> futures;
    // TODO Suppose the loop was written: For each SuperChunk in the queue, if there is space
    //  in the list of futures, add a future to get the next SC in teh queue. If there is not
    //  space for a SC, wait for a future to complete. Once there are no more SCs in the queue,
    //  wait for all the futures to complete. jhrg 1/31/24
    try {
        bool done = false;
        bool future_finished = true;
        while (!done) {

            if (!futures.empty())
                future_finished = get_next_future(futures, transfer_thread_counter, DMRPP_WAIT_FOR_FUTURE_MS, prolog);

            // If future_finished is true this means that the chunk_processing_thread_counter has been decremented,
            // because future::get() was called or a call to future::valid() returned false.
            BESDEBUG(dmrpp_3, prolog << "future_finished: " << (future_finished ? "true" : "false") << endl);

            if (!super_chunks.empty()) {
                // Next we try to add a new Chunk compute thread if we can - there might be room.
                bool thread_started = true;
                while (thread_started && !super_chunks.empty()) {
                    auto super_chunk = super_chunks.front();
                    BESDEBUG(dmrpp_3, prolog << "Starting thread for " << super_chunk->to_string(false) << endl);

                    // TODO 'array' is not used. one_super_chunk_args is defined in DmrppArray.h - could be def'd
                    //  in this file jhrg 1/31/24
                    auto args = unique_ptr<one_super_chunk_args>(new one_super_chunk_args(super_chunk, array));
                    thread_started = start_super_chunk_transfer_thread(futures, std::move(args));

                    if (thread_started) {
                        super_chunks.pop();
                        BESDEBUG(dmrpp_3, prolog << "STARTED thread for " << super_chunk->to_string(false) << endl);
                    }
                    else {
                        // Thread did not start, ownership of the arguments was not passed to the thread.
                        BESDEBUG(dmrpp_3, prolog << "Thread not started. args deleted, Chunk remains in queue.)" <<
                                                 " transfer_thread_counter: " << transfer_thread_counter <<
                                                 " futures.size(): " << futures.size() << endl);
                    }
                }
            }
            else {
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

/**
 * @brief A single argument wrapper for process_super_chunk_unconstrained() for use with std::async().
 * @param args A unique_ptr to an instance of one_super_chunk_args.
 * @return True unless an exception is throw in which case neither true or false apply.
 */
bool one_super_chunk_unconstrained_transfer_thread(const unique_ptr<one_super_chunk_args> &args) {

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

/**
 * @brief Starts the one_super_chunk_unconstrained_transfer_thread function using std::async() and places the returned future in the queue futures.
 * @param futures The queue into which to place the future returned by std::async().
 * @param args The arguments for the super_chunk_thread function
 * @return Returns true if the async call was made and a future was returned, false if the transfer_thread_counter has
 * reached the maximum allowable size.
 */
bool start_super_chunk_unconstrained_transfer_thread(list<std::future<bool>> &futures,
                                                     unique_ptr<one_super_chunk_args> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck(transfer_thread_pool_mtx);
    if (transfer_thread_counter < DmrppRequestHandler::d_max_transfer_threads) {
        transfer_thread_counter++;
        futures.push_back(
                std::async(std::launch::async, one_super_chunk_unconstrained_transfer_thread, std::move(args)));
        retval = true;

        // The args may be null after move(args) is called and causes the segmentation fault in the following BESDEBUG.
        // So remove that part but leave the futures.size() for bookkeeping.
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() << endl);

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
void read_super_chunks_unconstrained_concurrent(queue<shared_ptr<SuperChunk>> &super_chunks, DmrppArray *array) {
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + " name: " + array->name(), "");

    // Parallel version based on read_chunks_unconstrained(). There is
    // substantial duplication of the code in read_chunks_unconstrained(), but
    // wait to remove that when we move to C++11 which has threads integrated.

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

            if (!super_chunks.empty()) {
                // Next we try to add a new Chunk compute thread if we can - there might be room.
                bool thread_started = true;
                while (thread_started && !super_chunks.empty()) {
                    auto super_chunk = super_chunks.front();
                    BESDEBUG(dmrpp_3, prolog << "Starting thread for " << super_chunk->to_string(false) << endl);

                    auto args = unique_ptr<one_super_chunk_args>(new one_super_chunk_args(super_chunk, array));
                    thread_started = start_super_chunk_unconstrained_transfer_thread(futures, std::move(args));

                    if (thread_started) {
                        super_chunks.pop();
                        BESDEBUG(dmrpp_3, prolog << "STARTED thread for " << super_chunk->to_string(false) << endl);
                    }
                    else {
                        // Thread did not start, ownership of the arguments was not passed to the thread.
                        BESDEBUG(dmrpp_3, prolog << "Thread not started. args deleted, Chunk remains in queue.)" <<
                                                 " transfer_thread_counter: " << transfer_thread_counter <<
                                                 " futures.size(): " << futures.size() << endl);
                    }
                }
            }
            else {
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

bool one_super_chunk_unconstrained_transfer_thread_dio(const unique_ptr<one_super_chunk_args> &args) {

#if DMRPP_ENABLE_THREAD_TIMERS
    stringstream timer_tag;
    timer_tag << prolog << "tid: 0x" << std::hex << std::this_thread::get_id() <<
    " parent_tid: 0x" << std::hex << args->parent_thread_id  << " sc_id: " << args->super_chunk->id();
    BESStopWatch sw(TRANSFER_THREADS);
    sw.start(timer_tag.str());
#endif

    args->super_chunk->read_unconstrained_dio();
    return true;
}

bool start_super_chunk_unconstrained_transfer_thread_dio(list<std::future<bool>> &futures,
                                                         unique_ptr<one_super_chunk_args> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck(transfer_thread_pool_mtx);
    if (transfer_thread_counter < DmrppRequestHandler::d_max_transfer_threads) {
        transfer_thread_counter++;
        futures.push_back(
                std::async(std::launch::async, one_super_chunk_unconstrained_transfer_thread_dio, std::move(args)));
        retval = true;
        BESDEBUG(dmrpp_3, prolog << "Got std::future '" << futures.size() <<
                                 "' from std::async, transfer_thread_counter: " << transfer_thread_counter << endl);
    }
    return retval;
}

// Clone of read_super_chunks_unconstrained_concurrent for direct IO. 
// Doing this to ensure direct IO won't affect the regular operations.
void read_super_chunks_unconstrained_concurrent_dio(queue<shared_ptr<SuperChunk>> &super_chunks, DmrppArray *array) {
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + " name: " + array->name(), "");

    // Parallel version based on read_chunks_unconstrained(). There is
    // substantial duplication of the code in read_chunks_unconstrained(), but
    // wait to remove that when we move to C++11 which has threads integrated.

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

            if (!super_chunks.empty()) {
                // Next we try to add a new Chunk compute thread if we can - there might be room.
                bool thread_started = true;
                while (thread_started && !super_chunks.empty()) {
                    auto super_chunk = super_chunks.front();
                    BESDEBUG(dmrpp_3, prolog << "Starting thread for " << super_chunk->to_string(false) << endl);

                    auto args = unique_ptr<one_super_chunk_args>(new one_super_chunk_args(super_chunk, array));

                    // direct IO calling
                    thread_started = start_super_chunk_unconstrained_transfer_thread_dio(futures, std::move(args));

                    if (thread_started) {
                        super_chunks.pop();
                        BESDEBUG(dmrpp_3, prolog << "STARTED thread for " << super_chunk->to_string(false) << endl);
                    }
                    else {
                        // Thread did not start, ownership of the arguments was not passed to the thread.
                        BESDEBUG(dmrpp_3, prolog << "Thread not started. args deleted, Chunk remains in queue.)" <<
                                                 " transfer_thread_counter: " << transfer_thread_counter <<
                                                 " futures.size(): " << futures.size() << endl);
                    }
                }
            }
            else {
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

// TODO This is where the parallel transfer _functions_ end. There is one used of
//  get_next_future() beyond this code in DmrppArray::read_contiguous() and that
//  cann also be removed (see the comment there) jhrg 1/31/24

/**
 * @brief Compute the index of the address for an an array of array_shape.
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
 * @param address N-tuple zero-based index of an element in N-space
 * @param array_shape N-tuple of the array's dimension sizes.
 * @return The offset into the vector used to store the values.
 */
unsigned long long
get_index(const vector<unsigned long long> &address, const vector<unsigned long long> &array_shape)
{
    if (address.size() != array_shape.size()) {  // ranks must be equal
        throw BESInternalError("get_index: address != array_shape", __FILE__, __LINE__);
    }

    if (address.empty()) {    // both address and array_shape are empty
        return 0ULL;
    }

    auto shape_index = array_shape.rbegin();
    auto index = address.rbegin(), index_end = address.rend();

    if (*index >= *shape_index) {
        throw BESInternalError("get_index: index >= shape_index", __FILE__, __LINE__);
    }

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

// TODO Add en explanation for this code - it is used in insert_chunk_unconstrained()
//  as an important optimization. If we can apply this to the get_index code (or the code
//  that calls it) we can reduce runtime of the data insert operations jhrg 2/2/24
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
                                               const vector<unsigned long long> &array_shape, char /*Chunk*/*src_buf)
{
    BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - subsetAddress.size(): " << subset_addr.size() << endl);

    unsigned int bytes_per_elem = prototype()->width();

    char *dest_buf = get_buf();

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
            uint64_t target_byte = *target_index * bytes_per_elem;
            uint64_t source_byte = source_index * bytes_per_elem;
            // Copy a single value.
            for (unsigned long i = 0; i < bytes_per_elem; i++) {
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
                insert_constrained_contiguous(dim_iter, target_index, subset_addr, array_shape, src_buf);
                subset_addr.pop_back();
            }
            else {
                // We are at the last (innermost) dimension, so it's time to copy values.
                subset_addr.push_back(myDimIndex);
                unsigned int sourceIndex = get_index(subset_addr, array_shape);
                subset_addr.pop_back();

                // Copy a single value.
                uint64_t target_byte = *target_index * bytes_per_elem;
                uint64_t source_byte = sourceIndex * bytes_per_elem;

                for (unsigned int i = 0; i < bytes_per_elem; i++) {
                    dest_buf[target_byte++] = src_buf[source_byte++];
                }
                (*target_index)++;
            }
        }
    }
}

/**
 * @brief Insert data into a variable for structure.  
 *
 * This method is a clone of the method insert_constrained_contiguous with the minimal addition to handle the structure.
 *
 * This method is used only for contiguous data. It is called only by itself
 * and read_contiguous().
 */
void DmrppArray::insert_constrained_contiguous_structure(Dim_iter dim_iter, unsigned long *target_index,
                                               vector<unsigned long long> &subset_addr,
                                               const vector<unsigned long long> &array_shape, char /*Chunk*/*src_buf, vector<char> &dest_buf)
{
    BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - subsetAddress.size(): " << subset_addr.size() << endl);

    unsigned int bytes_per_elem = prototype()->width();

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
            uint64_t target_byte = *target_index * bytes_per_elem;
            uint64_t source_byte = source_index * bytes_per_elem;
            // Copy a single value.
            for (unsigned long i = 0; i < bytes_per_elem; i++) {
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
                insert_constrained_contiguous(dim_iter, target_index, subset_addr, array_shape, src_buf);
                subset_addr.pop_back();
            }
            else {
                // We are at the last (innermost) dimension, so it's time to copy values.
                subset_addr.push_back(myDimIndex);
                unsigned int sourceIndex = get_index(subset_addr, array_shape);
                subset_addr.pop_back();

                // Copy a single value.
                uint64_t target_byte = *target_index * bytes_per_elem;
                uint64_t source_byte = sourceIndex * bytes_per_elem;

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
 * @note Currently String Arrays are handled using a different method. Do not
 * call this with String data. jhrg 3/3/22
 *
 * @return Always returns true, matching the libdap::Array::read() behavior.
 */
void DmrppArray::read_contiguous()
{

    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + " name: "+name(), "");

    // Get the single chunk that makes up this CONTIGUOUS variable.
    if (get_chunks_size() != 1)
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
    // TODO If we follow the idea to remove the parallel transfer code, this else clause would be
    //  removed. jhrg 1/31/24
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
                        // FYI This is the sole use of start_one_child_chunk_thread(). jhrg 1/31/24
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

    // Now that the_one_chunk has been read, we do what is necessary...
    if (!is_filters_empty() && !get_one_chunk_fill_value()) {
        the_one_chunk->filter_chunk(get_filters(), get_chunk_size_in_elements(), var()->width());
    }

    // TODO Once the dat from the chunk is transferred into the Array, is the memory for the chunk
    //  released? jhrg 2/2/24
    // The 'the_one_chunk' now holds the data values. Transfer it to the Array.
    if (!is_projected()) {  // if there is no projection constraint
        reserve_value_capacity_ll(get_size(false));

        // We need to handle the structure data differently.
        if (this->var()->type() != dods_structure_c)
           val2buf(the_one_chunk->get_rbuf());      // yes, it's not type-safe
        else { // Structure 
            // Check if we can handle this case. 
            // Currently we only handle one-layer simple int/float types, and the data is not compressed. 
            bool can_handle_struct = check_struct_handling();
            if (can_handle_struct) {
                char *buf_value = the_one_chunk->get_rbuf();
                unsigned long long value_size = the_one_chunk->get_size();
                vector<char> values(buf_value,buf_value+value_size);
                read_array_of_structure(values);
            }
            else
                // TODO Edit the message and use a BESError. Maybe a SyntaxUserError?
                //  Maybe we need an error for something the BES cannot do yet, but could
                //  do in the future? jhrg 2/2/24
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
            insert_constrained_contiguous(dim_begin(), &target_index, subset, array_shape, the_one_chunk->get_rbuf());
        }
        else {
            // Currently we only handle one-layer simple int/float types, and the data is not compressed. 
            bool can_handle_struct = check_struct_handling();
            if (can_handle_struct) {
                unsigned long long value_size = get_size(true)*width_ll();
                vector<char> values;
                values.resize(value_size);
                vector<unsigned long long> array_shape = get_shape(false);
                unsigned long target_index = 0;
                vector<unsigned long long> subset;
                insert_constrained_contiguous_structure(dim_begin(), &target_index, subset, array_shape, the_one_chunk->get_rbuf(),values);
                read_array_of_structure(values);
            }
            else
                // TODO BES Error type. jhrg 2/2/14
                throw InternalErr(__FILE__, __LINE__, "Only handle integer and float base types. Cannot handle the array of complex structure yet."); 
        }
    }

    set_read_p(true);
}

// TODO Comment this. jhrg 2/2/24
void DmrppArray::read_one_chunk_dio() {

    // Get the single chunk that makes up this one-chunk compressed variable.
    if (get_chunks_size() != 1)
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
        unsigned int elem_width = prototype()->width();

        array_offset += chunk_origin[dim];

        // Compute how much we are going to copy
        unsigned long long chunk_bytes = (end_element - chunk_origin[dim] + 1) * elem_width;
        char *source_buffer = chunk->get_rbuf();
        char *target_buffer = get_buf();
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
    memcpy(target_buffer + chunk->get_direct_io_offset(), source_buffer, chunk->get_size());
 
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
    if (get_chunks_size() < 2)
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

    reserve_value_capacity_ll(get_size());
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
            super_chunk->read_unconstrained();
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

//The direct chunk IO routine of read chunks., mostly copy from the general IO handling routines.
void DmrppArray::read_chunks_dio_unconstrained()
{

    if (get_chunks_size() < 2)
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
        BESStopWatch sw(dmrpp_3);
        sw.start(prolog + "Serial SuperChunk Processing.");
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
        stringstream timer_name;
        timer_name << prolog << "Concurrent SuperChunk Processing. d_max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads;
        BESStopWatch sw(dmrpp_3);
        sw.start(timer_name.str());
#endif
        // Call direct IO routine for parallel transfers
        read_super_chunks_unconstrained_concurrent_dio(super_chunks, this);
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
        BESDEBUG(dmrpp_3, prolog << " END, This is the last_dim. chunk: " << chunk->to_string() << endl);
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
    if ((unsigned long long) thisDim.stop < end_element) {
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
    if (get_chunks_size() < 2)
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

// TODO Remove this - it's wrong (it works only for a limited case and we have more
//  general code here. jhrg 2/2/24
/**
 * @brief Process String Array so long as it has only one element
 *
 * This method is pretty limited, but this is a common case and the DMR++
 * will need more information for the Dmrpp handler to support the general
 * case of an N-dimensional array.
 */
void DmrppArray::read_contiguous_string()
{
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + " name: "+name(), "");

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

string DmrppArray::ingest_fixed_length_string(char *buf, unsigned long long fixed_str_len, string_pad_type pad_type) {
    switch (pad_type) {
        case null_pad:
        case null_term: {
            // reordered the tests to fix an off-by-one. jhrg 2/7/24
            unsigned long long str_len = 0;
            while (str_len < fixed_str_len && buf[str_len] != 0) {
                str_len++;
            }
            BESDEBUG(MODULE, prolog << DmrppArray::pad_type_to_str(pad_type) << " scheme. str_len: " << str_len << endl);
            return string(buf, str_len);
        }

        case space_pad: {
            unsigned long long str_len = fixed_str_len;
            while (str_len > 0 && (buf[str_len - 1] == ' ' || buf[str_len - 1] == 0)) {
                str_len--;
            }
            BESDEBUG(MODULE, prolog << DmrppArray::pad_type_to_str(pad_type) << " scheme. str_len: " << str_len << endl);
            return string(buf, str_len);
        }

        case not_set:
        default:
            // Do nothing.
            BESDEBUG(MODULE, prolog << "pad_type: NOT_SET" << endl);
            break;
    }

    return "";
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

// TODO This is torturred code. One goal of the code review is to simplify this.
//  Not sure if that is possble, though. jhrg 2/2/24
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
    if (read_p()) return true;

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
        if (array_to_read->get_chunks_size() == 1) {
            BESDEBUG(MODULE, prolog << "Reading data from a single contiguous chunk." << endl);
            // KENT: here we need to add the handling of direct chunk IO for one chunk. 
            if (this->get_dio_flag())
                array_to_read->read_one_chunk_dio();
            else 
                array_to_read->read_contiguous();    // Throws on various errors
        }
        else {  // Handle the more complex case where the data is chunked.
            if (!array_to_read->is_projected()) {
                BESDEBUG(MODULE, prolog << "Reading data from chunks, unconstrained." << endl);
                 // KENT: Only here we need to consider the direct buffer IO.
                // The best way is to hold another function but with direct buffer
                if (this->get_dio_flag())
                    array_to_read->read_chunks_dio_unconstrained();
                else 
                    array_to_read->read_chunks_unconstrained();
            }
            else {
                BESDEBUG(MODULE, prolog << "Reading data from chunks." << endl);
                array_to_read->read_chunks();
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
            default: break; // Do nothing for all other types.
        }
    }

    return true;
}

// TODO Move this into the class. jhrg 2/1/24
unsigned long long DmrppArray::set_fixed_string_length(const string &length_str) {
    try {
        d_fixed_str_length = stoull(length_str);
    }
    // TODO I don't see how this can be thrown - the method will only be used with a std::string.
    //  jhrg 2/1/24
    catch (std::invalid_argument e) {
        stringstream err_msg;
        err_msg << "The value of the length string could not be parsed. Message: " << e.what();
        throw BESInternalError(err_msg.str(), __FILE__, __LINE__);
    }
    return d_fixed_str_length;
}

// TODO Move this into the method below since that is the only place it is used. jhrg 2/1/24
//  The reason I'm suggesting that is having this as both a function and a method implies that
//  it is used both in code that is part of the object and not.
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

// TODO Same thing here - only used in the following method. jhrg 2/1/24
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

// TODO Since we have std:string in the header, this method code be moved there and then
//  ons would be defeined in just one place. jhrg 2/1/24
ons::ons(const std::string &ons_pair_str) {
    const string colon(":");
    size_t colon_pos = ons_pair_str.find(colon);

    string offset_str = ons_pair_str.substr(0, colon_pos);
    offset = stoull(offset_str);

    string size_str = ons_pair_str.substr(colon_pos + 1);
    size = stoull(size_str);
}

void DmrppArray::set_ons_string(const std::string &ons_str) {
    d_vlen_ons_str = ons_str;
}

void DmrppArray::set_ons_string(const vector<ons> &ons_pairs) {
    // TODO Use string. jhrg 2/1/24
    stringstream ons_ss;
    bool first = true;
    for (auto &ons_pair: ons_pairs) {
        if (!first) {
            ons_ss << ",";
        }
        ons_ss << ons_pair.offset << ":" << ons_pair.size;
    }
    d_vlen_ons_str = ons_ss.str();
}

// TODO This is never used. jhrg 2/1/24
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
void flsa_xml_element(XMLWriter &xml, DmrppArray &a) {

    string element_name("dmrpp:FixedLengthStringArray");
    string str_len_attr_name("string_length");
    string pad_attr_name("pad");

    if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar *) element_name.c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write " + element_name + " element");

    // TODO This use of stringstream can be repplaced with to_string(a.get_fixed_string_length()).c_str()
    //  stringstream is slow. jhrg 2/1/24
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
                // TODO The code can avoid copying the values by accessing d_buf and pssing that to Base64::encode()
                //  also means the try-catch and delete[] can be skipped. jhrg 2/1/24
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
                    // TODO same as above. jhrg 2/1/24
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
        string size = constrained ? to_string(d.c_size) : to_string(d.size);
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "size",
                                        (const xmlChar *) size.c_str()) < 0)
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
    if (DmrppCommon::d_print_chunks && (get_chunks_size() > 0 || get_uses_fill_value()))
        print_chunks_element(xml, DmrppCommon::d_ns_prefix);

    // If this variable uses the COMPACT layout, encode the values for
    // the array using base64. Note that strings are a special case; each
    // element of the array is a string and is encoded in its own base64
    // xml element. So, while an array of 10 int32 will be encoded in a
    // single base64 element, an array of 10 strings will use 10 base64
    // elements. This is because the size of each string's value is different.
    // Not so for an int32.
    // TODO Is it an error to be here and have both d_print_chunks and is_compact_layout
    //  true and have read_p false? jhrg 2/1/24
    if (DmrppCommon::d_print_chunks && is_compact_layout() && read_p()) {
        compact_data_xml_element(xml, *this);
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
    }
    return Vector::buf2val(val);
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

    for (int64_t element = 0; element < nelms; ++element) {

        auto dmrpp_s = dynamic_cast<DmrppStructure*>(var()->ptr_duplicate());
        try {
            dmrpp_s->structure_read(values,values_offset);
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
    // Currently doesn't support compressed array of structure.
    if (this->get_filters().empty()) {

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
    }
    else
        ret_value = false;

    return ret_value;
}


} // namespace dmrpp
