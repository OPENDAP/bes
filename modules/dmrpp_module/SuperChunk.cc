// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter<ndp@opendap.org>
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
#include <vector>
#include <string>

#include "BESInternalError.h"
#include "BESDebug.h"

#include "DmrppRequestHandler.h"
#include "CurlHandlePool.h"
#include "DmrppArray.h"
#include "DmrppNames.h"
#include "Chunk.h"
#include "SuperChunk.h"

#define prolog std::string("SuperChunk::").append(__func__).append("() - ")

#define SUPER_CHUNK_MODULE "dmrpp:3"

using std::stringstream;
using std::string;
using std::vector;

namespace dmrpp {

// ThreadPool state variables.
std::mutex chunk_processing_thread_pool_mtx;     // mutex for critical section
atomic_uint chunk_processing_thread_counter(0);
#define COMPUTE_THREADS "compute_threads"

#define DMRPP_ENABLE_THREAD_TIMERS 0

/**
 * @brief Reads the Chunk (as needed) and performs the inflate/shuffle/etc. processing after which the values are inserted into the array.
 *
 * This function may be called by a thread in a multi-threaded access scenario
 * or by a DmrppArray method in the serial access case. The Chunk::read_chunk()
 * method may throw an exception. In the multi-threaded case, that exception
 * will only be part of the thread's execution context, not "main()'s" context.
 * The code in the thread task one_chunk_thread above will catch that exception
 * and return an error code using pthread_exit(). That, in turn, will be read
 * by the main thread and turned into an exception that propagates to the top
 * of the BES call stack.
 *
 * @param chunk The chunk to process
 * @param array The DmrppArray instance that called this function
 * @param constrained_array_shape How the DAP Array this chunk is part of was
 * constrained - used to determine where/how to add the chunk's data to the
 * whole array.
 */
void process_one_chunk(shared_ptr<Chunk> chunk, DmrppArray *array, const vector<unsigned long long> &constrained_array_shape)
{
    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "BEGIN" << endl );

    // TODO If this is part of a SuperChunk, hasn't the data been read by SuperChunk::Retrieve_data()?
    //  If so, calling read() here is not needed. Same question below. jhg 5/7/22
    chunk->read_chunk();

    if(array) {
        // If this chunk used/uses hdf5 fill values, do not attempt to deflate, etc., its
        // values since the fill value code makes the chunks 'fully formed.'' jhrg 5/16/22
        if (!chunk->get_uses_fill_value() && !array->is_filters_empty())
            chunk->filter_chunk(array->get_filters(), array->get_chunk_size_in_elements(), array->get_bytes_per_element());

        vector<unsigned long long> target_element_address = chunk->get_position_in_array();
        vector<unsigned long long> chunk_source_address(array->dimensions(), 0);

        char *dest_buf = array->get_buf();
        if (array->var()->type() == libdap::dods_structure_c) {
            dest_buf = array->get_structure_array_buf_ptr();

        }
        array->insert_chunk(0, &target_element_address, &chunk_source_address,
                            chunk, constrained_array_shape, dest_buf);
    }

    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "END" << endl );
}

/**
 * @brief Reads the Chunk (as needed) and performs the inflate/shuffle/etc. processing after which the values are inserted into the array.
 *
 * This function may be called by a thread in a multi-threaded access scenario
 * or by a DmrppArray method in the serial access case. The Chunk::read_chunk()
 * method may throw an exception. In the multi-threaded case, that exception
 * will only be part of the thread's execution context, not "main()'s" context.
 * The code in the thread task one_chuck_thread above will catch that exception
 * and return an error code using pthread_exit(). That, in turn, will be read
 * by the main thread and turned into an exception that propagates to the top
 * of the BES call stack.
 *
 * @param chunk The chunk to process
 * @param chunk_shape The chunk shape
 * @param array The DmrppArray instance that called this function
 * @param array_shape How the DAP Array this chunk is part of was
 * constrained - used to determine where/how to add the chunk's data to the
 * whole array.
 */
void process_one_chunk_unconstrained(shared_ptr<Chunk> chunk, const vector<unsigned long long> &chunk_shape,
                                     DmrppArray *array, const vector<unsigned long long> &array_shape)
{
    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "BEGIN" << endl );

    chunk->read_chunk();

    if(array){
        if (!chunk->get_uses_fill_value() && !array->is_filters_empty())
            chunk->filter_chunk(array->get_filters(), array->get_chunk_size_in_elements(), array->get_bytes_per_element());

        array->insert_chunk_unconstrained(chunk, 0, 0, array_shape, 0, chunk_shape, chunk->get_position_in_array());
    }

    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "END" << endl );
}

void process_one_chunk_unconstrained_dio(shared_ptr<Chunk> chunk, const vector<unsigned long long> &chunk_shape,
                                     DmrppArray *array, const vector<unsigned long long> &array_shape)
{
    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "BEGIN" << endl );

    chunk->read_chunk();

    if(array){
        array->insert_chunk_unconstrained_dio(chunk);
    }

    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "END" << endl );
}

/**
 * @brief A single argument wrapper for process_one_chunk() for use with std::async().
 * @param args A unique_ptr to an instance of one_chunk_args.
 * @return True unless an exception is throw in which case neither true or false apply.
 */
bool one_chunk_compute_thread(unique_ptr<one_chunk_args> args)
{
#if DMRPP_ENABLE_THREAD_TIMERS
    stringstream timer_tag;
    timer_tag << prolog << "tid: 0x" << std::hex << std::this_thread::get_id() <<
        " parent_tid: 0x" << std::hex << args->parent_thread_id << " parent_sc: " << args->parent_super_chunk_id;
    BES_STOPWATCH_START(COMPUTE_THREADS, timer_tag.str());
#endif

    process_one_chunk(args->chunk, args->array, args->array_shape);
    return true;
}

/**
 * @brief A single argument wrapper for process_one_chunk_unconstrained() for use with std::async().
 * @param args A unique_ptr to an instance of one_chunk_args.
 * @return True unless an exception is throw in which case neither true or false apply.
 */
bool one_chunk_unconstrained_compute_thread(unique_ptr<one_chunk_unconstrained_args> args)
{
#if DMRPP_ENABLE_THREAD_TIMERS
    stringstream timer_tag;
    timer_tag << prolog << "tid: 0x" << std::hex << std::this_thread::get_id() <<
          " parent_tid: 0x" << std::hex << args->parent_thread_id << " parent_sc: " << args->parent_super_chunk_id ;
    BES_STOPWATCH_START(COMPUTE_THREADS, timer_tag.str());
#endif

    process_one_chunk_unconstrained(args->chunk, args->chunk_shape, args->array, args->array_shape);
    return true;
}

bool one_chunk_unconstrained_compute_thread_dio(unique_ptr<one_chunk_unconstrained_args> args)
{
#if DMRPP_ENABLE_THREAD_TIMERS
    stringstream timer_tag;
    timer_tag << prolog << "tid: 0x" << std::hex << std::this_thread::get_id() <<
          " parent_tid: 0x" << std::hex << args->parent_thread_id << " parent_sc: " << args->parent_super_chunk_id ;
    BES_STOPWATCH_START(COMPUTE_THREADS, timer_tag.str());
#endif

    process_one_chunk_unconstrained_dio(args->chunk, args->chunk_shape, args->array, args->array_shape);
    return true;
}
/**
 * @brief Asynchronously starts the one_chunk_compute_thread using std::async() and places the returned std::future in the queue futures.
 *
 * NOTE: one_chunk_compute_thread is a wrapper() for process_one_chunk()
 *
 * @param futures The queue into which to place the future returned by async.
 * @param args The arguments for the one_chunk_compute_thread function
 * @return Returns true if the std::async() call was made and a future was returned, false if the
 * chunk_processing_thread_counter has reached the maximum allowable size.
 */
bool start_one_chunk_compute_thread(list<std::future<bool>> &futures, unique_ptr<one_chunk_args> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck (chunk_processing_thread_pool_mtx);
    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "d_max_compute_threads: " << DmrppRequestHandler::d_max_compute_threads << " chunk_processing_thread_counter: " << chunk_processing_thread_counter << endl);
    if (chunk_processing_thread_counter < DmrppRequestHandler::d_max_compute_threads) {
        chunk_processing_thread_counter++;
        futures.push_back(std::async(std::launch::async, one_chunk_compute_thread, std::move(args)));
        retval = true;
        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "Got std::future '" << futures.size() <<
                                            "' from std::async, chunk_processing_thread_counter: " << chunk_processing_thread_counter << endl);
    }
    return retval;
}

/**
 * @brief Asynchronously starts the one_chunk_unconstrained_compute_thread using std::async() and places the returned std::future in the queue futures.
 *
 * NOTE: one_chunk_unconstrained_compute_thread() is a wrapper for process_one_chunk_unconstrained()
 *
 * @param futures The queue into which to place the future returned by async.
 * @param args The arguments for the one_chunk_compute_thread function
 * @return Returns true if the std::async() call was made and a future was returned, false if the
 * chunk_processing_thread_counter has reached the maximum allowable size.
 */
bool start_one_chunk_unconstrained_compute_thread(list<std::future<bool>> &futures, unique_ptr<one_chunk_unconstrained_args> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck (chunk_processing_thread_pool_mtx);
    if (chunk_processing_thread_counter < DmrppRequestHandler::d_max_compute_threads) {
        futures.push_back(std::async(std::launch::async, one_chunk_unconstrained_compute_thread, std::move(args)));
        chunk_processing_thread_counter++;
        retval = true;
        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "Got std::future '" << futures.size() <<
                                            "' from std::async, chunk_processing_thread_counter: " << chunk_processing_thread_counter << endl);
    }
    return retval;
}

bool start_one_chunk_unconstrained_compute_thread_dio(list<std::future<bool>> &futures, unique_ptr<one_chunk_unconstrained_args> args) {
    bool retval = false;
    std::unique_lock<std::mutex> lck (chunk_processing_thread_pool_mtx);
    if (chunk_processing_thread_counter < DmrppRequestHandler::d_max_compute_threads) {
        futures.push_back(std::async(std::launch::async, one_chunk_unconstrained_compute_thread_dio, std::move(args)));
        chunk_processing_thread_counter++;
        retval = true;
        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "Got std::future '" << futures.size() <<
                                            "' from std::async, chunk_processing_thread_counter: " << chunk_processing_thread_counter << endl);
    }
    return retval;
}


/**
 * @brief Uses std::async and std::future to concurrently retrieve/inflate/shuffle/insert/etc the Chunks in the queue "chunks".
 *
 * For each Chunk in the queue, process the chunked data by using std::async() to generate a std::future which will
 * perform the data retrieval (if the Chunk has not been read previously) and subsequent computational steps
 * (inflate/shuffle/etc) and finally insertion into the DmrppArray's internal data buffer.
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
 * @param chunks The queue of Chunk objects to process.
 * @param array The DmrppArray into which the chunk data will be placed.
 * @param array_shape The shape of the DmrppArray (passing is faster than recomputing this value)
 */
void process_chunks_concurrent(
        const string &super_chunk_id,
        queue<shared_ptr<Chunk>> &chunks,
        DmrppArray *array,
        const vector<unsigned long long> &constrained_array_shape ){

    // We maintain a list  of futures to track our parallel activities.
    list<future<bool>> futures;
    try {
        bool done = false;
        bool future_finished = true;
        while (!done) {

            if(!futures.empty())
                future_finished = get_next_future(futures, chunk_processing_thread_counter, DMRPP_WAIT_FOR_FUTURE_MS, prolog);

            // If future_finished is true this means that the chunk_processing_thread_counter has been decremented,
            // because future::get() was called or a call to future::valid() returned false.
            BESDEBUG(SUPER_CHUNK_MODULE, prolog << "future_finished: " << (future_finished ? "true" : "false") << endl);

            if (!chunks.empty()){
                // Next we try to add a new Chunk compute thread if we can - there might be room.
                bool thread_started = true;
                while(thread_started && !chunks.empty()) {
                    auto chunk = chunks.front();
                    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "Starting thread for " << chunk->to_string() << endl);

                    auto args = unique_ptr<one_chunk_args>(new one_chunk_args(super_chunk_id, chunk, array, constrained_array_shape));
                    thread_started = start_one_chunk_compute_thread(futures, std::move(args));

                    if (thread_started) {
                        chunks.pop();
                        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "STARTED thread for " << chunk->to_string() << endl);
                    } else {
                        // Thread did not start, ownership of the arguments was not passed to the thread.
                        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "Thread not started. args deleted, Chunk remains in queue.) " <<
                                                            "chunk_processing_thread_counter: " << chunk_processing_thread_counter << " futures.size(): " << futures.size() << endl);
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
 * @brief Uses std::async and std::future to concurrently retrieve/inflate/shuffle/insert/etc the Chunks in the queue "chunks".
 *
 * For each Chunk in the queue, process the chunked data by using std::async() to generate a std::future which will
 * perform the data retrieval (if the Chunk has not been read previously) and subsequent computational steps
 * (inflate/shuffle/etc) and finally insertion into the DmrppArray's internal data buffer.
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
 * @param chunks The queue of Chunk objects to process.
 * @param chunk_shape The shape of the chunk (passing is faster than recomputing this value)
 * @param array The DmrppArray into which the chunk data will be placed.
 * @param array_shape The shape of the DmrppArray (passing is faster than recomputing this value)
 */
void process_chunks_unconstrained_concurrent(
        const string &super_chunk_id,
        queue<shared_ptr<Chunk>> &chunks,
        const vector<unsigned long long> &chunk_shape,
        DmrppArray *array,
        const vector<unsigned long long> &array_shape){

    // We maintain a list  of futures to track our parallel activities.
    list<future<bool>> futures;
    try {
        bool done = false;
        while (!done) {

            if(!futures.empty())
                get_next_future(futures, chunk_processing_thread_counter, DMRPP_WAIT_FOR_FUTURE_MS, prolog);

            // If future_finished is true this means that the chunk_processing_thread_counter has been decremented,
            // because future::get() was called or a call to future::valid() returned false.

            if (!chunks.empty()){
                // Next we try to add a new Chunk compute thread if we can - there might be room.
                bool thread_started = true;
                while(thread_started && !chunks.empty()) {
                    auto chunk = chunks.front();

                    auto args = unique_ptr<one_chunk_unconstrained_args>(
                            new one_chunk_unconstrained_args(super_chunk_id, chunk, array, array_shape, chunk_shape) );
                    thread_started = start_one_chunk_unconstrained_compute_thread(futures, std::move(args));

                    if (thread_started) {
                        chunks.pop();
                    } else {
                        // Thread did not start, ownership of the arguments was not passed to the thread.
                        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "Thread not started. args deleted, Chunk remains in queue.)" <<
                                                            " chunk_processing_thread_counter: " << chunk_processing_thread_counter <<
                                                            " futures.size(): " << futures.size() << endl);
                    }
                }
            }
            else {
                // No more Chunks and no futures means we're done here.
                if(futures.empty())
                    done = true;
            }
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

//Direct IO routine for processing chunks when the variable is not constrained. 
void process_chunks_unconstrained_concurrent_dio(
        const string &super_chunk_id,
        queue<shared_ptr<Chunk>> &chunks,
        const vector<unsigned long long> &chunk_shape,
        DmrppArray *array,
        const vector<unsigned long long> &array_shape){

    // We maintain a list  of futures to track our parallel activities.
    list<future<bool>> futures;
    try {
        bool done = false;
        while (!done) {

            if(!futures.empty())
                get_next_future(futures, chunk_processing_thread_counter, DMRPP_WAIT_FOR_FUTURE_MS, prolog);

            // If future_finished is true this means that the chunk_processing_thread_counter has been decremented,
            // because future::get() was called or a call to future::valid() returned false.

            if (!chunks.empty()){
                // Next we try to add a new Chunk compute thread if we can - there might be room.
                bool thread_started = true;
                while(thread_started && !chunks.empty()) {
                    auto chunk = chunks.front();

                    auto args = unique_ptr<one_chunk_unconstrained_args>(
                            new one_chunk_unconstrained_args(super_chunk_id, chunk, array, array_shape, chunk_shape) );

                    // Call direct IO routine
                    thread_started = start_one_chunk_unconstrained_compute_thread_dio(futures, std::move(args));

                    if (thread_started) {
                        chunks.pop();
                    } else {
                        // Thread did not start, ownership of the arguments was not passed to the thread.
                        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "Thread not started. args deleted, Chunk remains in queue.)" <<
                                                            " chunk_processing_thread_counter: " << chunk_processing_thread_counter <<
                                                            " futures.size(): " << futures.size() << endl);
                    }
                }
            }
            else {
                // No more Chunks and no futures means we're done here.
                if(futures.empty())
                    done = true;
            }
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


//#####################################################################################################################
//#####################################################################################################################
//#####################################################################################################################
//
// SuperChunk Code Begins Here
//
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

// TODO There are (at least) two ways to handle 'fill value chunks.' The code can group
//  them all together as one big SuperChunk or store each FV chunk in its own SuperChunk.
//  (Of course, there are alternatives...) Using one SuperChunk is probably faster but
//  will require more work on the SuperChunk code. I think we should postpone that for now
//  to focus on getting the values correct (because that problem has yet to be solved).
//  I will add a ticket to return to this code and make that modification. jhrg 5/7/22
//
/**
 * @brief Attempts to add a new Chunk to this SuperChunk.
 *
 * The candidate_chunk is added to this SuperChunk if: it is contiguous with
 * the end of the this SuperChunk and has the same data_url. Note that if the
 * SuperChunk is empty, candidate_chunk meets those criteria by default.
 *
 * @note This method was modified to support fill value chunks as part of the
 * work on HYRAX-635. As a stop-gap implementation, each fill value chunk will
 * get its own SuperChunk, even though that is not the most efficient way forward.
 * See HYRAX-713 for a bit more on this. To add FV chunk support, FV chunks can
 * ony be added when d_chunks is empty. Thus, the is_contiguous(...) test must
 * fail if the chunk uses fill values. To make this hack easier to unwind later
 * on, I'm going to make that test right here - so it's obvious. jhrg 5/7/22
 *
 * @todo Should candidate_chunk be passed by reference? Maybe calls to this
 *   method should use move()? jhrg 5/7/22
 *
 * @todo Should this _only_ be called when d_chunks is not empty? jhrg 5/7/22
 *
 * @param candidate_chunk The Chunk to add.
 * @return True when the chunk is added, false otherwise.
 */
bool SuperChunk::add_chunk(const std::shared_ptr<Chunk> candidate_chunk) {
    bool chunk_was_added = false;
    if(d_chunks.empty()){
        d_chunks.push_back(candidate_chunk);
        d_offset = candidate_chunk->get_offset();
        d_size = candidate_chunk->get_size();
        // When get_uses_fill_value() is true, returns a shared_ptr<Chunk> initialized to nullptr. jhrg 5/7/22
        d_uses_fill_value = candidate_chunk->get_uses_fill_value();
        if (!d_uses_fill_value)
            d_data_url = candidate_chunk->get_data_url();
        else
            d_data_url = nullptr;
        chunk_was_added =  true;
    }
    // For now, if a chunk uses fill values, it gets its own SuperChunk. jhrg 5/7/22
    else if(!candidate_chunk->get_uses_fill_value() && is_contiguous(candidate_chunk)){
        this->d_chunks.push_back(candidate_chunk);
        d_size += candidate_chunk->get_size();
        chunk_was_added =  true;
    }
    return chunk_was_added;
}

// This method is for handling non-contiguous chunks. It follows the logic of add_chunk() but handles the non-contigous chunk case.
bool SuperChunk::add_chunk_non_contiguous(const std::shared_ptr<Chunk> candidate_chunk, unsigned long long &buffer_end_position) {

    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "Coming to add_chunk_non_contiguous" << endl);
    bool chunk_was_added = false;
    if(d_chunks.empty()){
        d_chunks.push_back(candidate_chunk);
        d_offset = candidate_chunk->get_offset();
        if(candidate_chunk->get_uses_fill_value())
            d_size = candidate_chunk->get_size();
        else 
            d_size = buffer_end_position-d_offset;

        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "add_chunk_non d_size: "<<d_size<< endl);
        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "add_chunk_non d_offset: "<<d_offset<< endl);

        // When get_uses_fill_value() is true, returns a shared_ptr<Chunk> initialized to nullptr.
        d_uses_fill_value = candidate_chunk->get_uses_fill_value();
        if (!d_uses_fill_value)
            d_data_url = candidate_chunk->get_data_url();
        else
            d_data_url = nullptr;
        chunk_was_added =  true;
    }
    // For now, if a chunk uses fill values, it gets its own SuperChunk. 
    else if(!candidate_chunk->get_uses_fill_value()){
        if (candidate_chunk->get_offset() >=d_offset) {
            // The chunk exceeds the boundary of this buffer. Create a new SuperChunk.
            if ((candidate_chunk->get_offset() + candidate_chunk->get_size())<=buffer_end_position) { 
                this->d_chunks.push_back(candidate_chunk);
                chunk_was_added =  true;
            }
        }
    }
    
    return chunk_was_added;
}

/**
 * @brief Returns true if candidate_chunk is "contiguous" with the end of the SuperChunk instance.
 *
 * Returns true if the implemented rule for contiguousity determines that the candidate_chunk is
 * contiguous with this SuperChunk and false otherwise.
 *
 * Currently the rule is that the offset of the candidate_chunk must be the same as the current
 * offset + size of the SuperChunk, and the data_url is the same as the one in the SuperChunk.
 *
 * @todo Declare candidate_chunk as a reference to avoid needless copy.
 *
 * @param candidate_chunk The Chunk to evaluate for contiguousness with this SuperChunk.
 * @return True if chunk isdeemed contiguous, false otherwise.
 */
bool SuperChunk::is_contiguous(const std::shared_ptr<Chunk> candidate_chunk) {
    // Are the URLs the same?
    bool contiguous  = candidate_chunk->get_data_url()->str() == d_data_url->str();
    if(contiguous){
        // If the URLs match then see if the locations are matching
        contiguous = (d_offset + d_size) == candidate_chunk->get_offset();
    }
    return contiguous;
}

/**
 * @brief  Assigns each Chunk held by the SuperChunk a read buffer.
 *
 * Each Chunk's read buffer is mapped to the corresponding section of the SuperChunk's
 * enclosing read buffer.
 *
 * This is a convenience/helper function for SuperChunk::read()
 */
void SuperChunk::map_chunks_to_buffer()
{
    unsigned long long bindex = 0;
    for(const auto &chunk : d_chunks){
        chunk->set_read_buffer(d_read_buffer + bindex, chunk->get_size(),0, false);
        bindex += chunk->get_size();
        if (bindex > d_size) {
            stringstream msg;
            msg << "ERROR The computed buffer index, " << bindex << " is larger than expected size of the SuperChunk. ";
            msg << "d_size: " << d_size;
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }
    }
}

void SuperChunk::map_non_contiguous_chunks_to_buffer()
{
    unsigned long long bindex = 0;

    for (unsigned i = 0; i <d_chunks.size(); i++){
        
        (d_chunks[i])->set_read_buffer(d_read_buffer + bindex, (d_chunks[i])->get_size(),0, false);

        // Need to use the next offset to figure out the starting position for this chunk buffer
        if (i <(d_chunks.size()-1)) {
            long long temp_bindex = (d_chunks[i+1])->get_offset()-d_offset;
            if (temp_bindex <0) 
                throw BESInternalError("The non-contiguous chunk offset cannot be smaller than the SuperChunk offset. ", __FILE__, __LINE__);
            bindex = (unsigned long long)temp_bindex;
            if (bindex > d_size) {
                stringstream msg;
                msg << "ERROR The computed buffer index, " << bindex << " is larger than expected size of the SuperChunk. ";
                msg << "d_size: " << d_size;
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }
        }
    }
}
/**
 * @brief Reads the contiguous range of bytes associated with the SuperChunk from the data URL.
 * This is a convenience/helper function for SuperChunk::retrieve_data()
 */
void SuperChunk::read_aggregate_bytes()
{
    // Since we already have a good infrastructure for reading Chunks, we just make a big-ol-Chunk to
    // use for grabbing bytes. Then, once read, we'll use the child Chunks to do the dirty work of inflating
    // and moving the results into the DmrppCommon object.
    Chunk chunk(d_data_url, "NOT_USED", d_size, d_offset);

    chunk.set_read_buffer(d_read_buffer, d_size,0,false);

    dmrpp_easy_handle *handle = DmrppRequestHandler::curl_handle_pool->get_easy_handle(&chunk);
    if (!handle)
        throw BESInternalError(prolog + "No more libcurl handles.", __FILE__, __LINE__);

    try {
        handle->read_data();  // throws if error
        dmrpp::CurlHandlePool::release_handle(handle);
    }
    catch(...) {
        dmrpp::CurlHandlePool::release_handle(handle);
        throw;
    }

    // If the expected byte count was not read, it's an error.
    if (d_size != chunk.get_bytes_read()) {
        ostringstream oss;
        oss << "Wrong number of bytes read for chunk; read: " << chunk.get_bytes_read() << ", expected: " << d_size;
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    d_is_read = true;
}

/**
 * @brief Load a SuperChunk with fill values
 *
 * @note The initial version of this method works with SuperChunks that can
 * only hold a single fill value chunk. This will hopefully change as the code
 * for FV chunks is optimized. jhrg 5/7/22
 */
void SuperChunk::read_fill_value_chunk()
{
    if (d_chunks.size() != 1)
        throw BESInternalError("Found a SuperChunk with uses_fill_value true but more than one child chunk.", __FILE__, __LINE__);

    d_chunks.front()->read_chunk();

    d_is_read=true;
}

/**
 * @brief Cause the SuperChunk and all of it's subordinate Chunks to be read.
 */
void SuperChunk::retrieve_data() {
    // TODO I think this code should set d_is_read. It sets it for the Chunk, which may be redundant). jhrg 5/9/22
    if (d_is_read) {
        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "SuperChunk (" << (void **) this << ") has already been read! Returning." << endl);
        return;
    }
    // TODO Move this into read_aggregate_bytes(), move map_chunks_to_buffer()
    //  after read_aggregate_bytes() and modify map_chunks_to_buffer() to set
    //  the chunk size and read state so the last for loop can be removed.
    //  jhrg 5/6/22
    if (!d_read_buffer) {
        // Allocate memory for SuperChunk receive buffer.
        // release memory in destructor.
        d_read_buffer = new char[d_size];
    }

    // Massage the chunks so that their read/receive/intern data buffer
    // points to the correct section of the d_read_buffer memory.
    // "Slice it up!"
    // We need to map non-contigous chunks differently.
    if (non_contiguous_chunk) 
        map_non_contiguous_chunks_to_buffer();
    else 
        map_chunks_to_buffer();

    // Read the bytes from the target URL. (pthreads, maybe depends on size...)
    // Use one (or possibly more) thread(s) depending on d_size
    // and utilize our friend cURL to stuff the bytes into d_read_buffer
    //
    // TODO Replace or improve this way of handling fill value chunks. jhrg 5/7/22
    if (d_uses_fill_value)
        read_fill_value_chunk();
    else
        read_aggregate_bytes();

    // TODO Check if Chunk::read() sets these. jhrg 5/9/22
    // Set each Chunk's read state to true.
    // Set each chunks byte count to the expected
    // size for the chunk - because upstream events
    // have assured this to be true.
    for(const auto& chunk : d_chunks){
        chunk->set_is_read(true);
        chunk->set_bytes_read(chunk->get_size());
    }
}

// Direct chunk IO routine for retrieve_data, it clones from retrieve_data(). To ensure
// the regular operations. Still use a separate method.
void SuperChunk::retrieve_data_dio() {

    // Leave this comment copied from retrieve_data_dio().
    // TODO I think this code should set d_is_read. It sets it for the Chunk, which may be redundant). jhrg 5/9/22
    if (d_is_read) {
        BESDEBUG(SUPER_CHUNK_MODULE, prolog << "SuperChunk (" << (void **) this << ") has already been read! Returning." << endl);
        return;
    }

    if (!d_read_buffer) {
        // Allocate memory for SuperChunk receive buffer.
        // release memory in destructor.
        d_read_buffer = new char[d_size];
    }

    // Massage the chunks so that their read/receive/intern data buffer
    // points to the correct section of the d_read_buffer memory.
    // "Slice it up!"
    map_chunks_to_buffer();

    // Read the bytes from the target URL. (pthreads, maybe depends on size...)
    // Use one (or possibly more) thread(s) depending on d_size
    // and utilize our friend cURL to stuff the bytes into d_read_buffer
    //
    // TODO Replace or improve this way of handling fill value chunks. jhrg 5/7/22
    read_aggregate_bytes();

    // TODO Check if Chunk::read() sets these. jhrg 5/9/22
    // Set each Chunk's read state to true.
    // Set each chunks byte count to the expected
    // size for the chunk - because upstream events
    // have assured this to be true.
    for(const auto& chunk : d_chunks){
        chunk->set_is_read(true);
        chunk->set_bytes_read(chunk->get_size());
    }
}


/**
 * @brief Reads the SuperChunk, inflates/de-shuffles the subordinate chunks as required and copies the values into array
 * @param target_array The array into which to write the data.
 */
void SuperChunk::process_child_chunks() {
    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "BEGIN" << endl);
    retrieve_data();

    vector<unsigned long long> constrained_array_shape = d_parent_array->get_shape(true);
    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "d_use_compute_threads: " << (DmrppRequestHandler::d_use_compute_threads ? "true" : "false") << endl);
    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "d_max_compute_threads: " << DmrppRequestHandler::d_max_compute_threads << endl);

    if (!DmrppRequestHandler::d_use_compute_threads) {
#if DMRPP_ENABLE_THREAD_TIMERS
        BES_STOPWATCH_START(SUPER_CHUNK_MODULE, prolog+"Serial Chunk Processing. id: " + d_id);
#endif
        for(const auto &chunk: d_chunks){
            process_one_chunk(chunk,d_parent_array,constrained_array_shape);
        }
    }
    else {
#if DMRPP_ENABLE_THREAD_TIMERS
        stringstream timer_tag;
        timer_tag << prolog << "Concurrent Chunk Processing. id: " << d_id;
        BES_STOPWATCH_START(SUPER_CHUNK_MODULE, timer_tag.str());
#endif
        queue< shared_ptr<Chunk> > chunks_to_process;
        for(const auto &chunk: d_chunks)
            chunks_to_process.push(chunk);

        process_chunks_concurrent(d_id, chunks_to_process, d_parent_array, constrained_array_shape);
    }
    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "END" << endl );
}


/**
 * @brief Reads the SuperChunk, inflates/deshuffles the subordinate chunks as required and copies the values into array
 * @param target_array The array into which to write the data.
 */
void SuperChunk::process_child_chunks_unconstrained() {

    BESDEBUG(SUPER_CHUNK_MODULE, prolog << "BEGIN" << endl);
    retrieve_data();

    // The size in element of each of the array's dimensions
    const vector<unsigned long long> array_shape = d_parent_array->get_shape(true);
    // The size, in elements, of each of the chunk's dimensions
    const vector<unsigned long long> chunk_shape = d_parent_array->get_chunk_dimension_sizes();

    if (!DmrppRequestHandler::d_use_compute_threads) {
#if DMRPP_ENABLE_THREAD_TIMERS
        BES_STOPWATCH_START(SUPER_CHUNK_MODULE, prolog + "Serial Chunk Processing. sc_id: " + d_id);
#endif
        for(const auto &chunk: d_chunks){
            process_one_chunk_unconstrained(chunk, chunk_shape, d_parent_array, array_shape);
        }
    }
    else {
#if DMRPP_ENABLE_THREAD_TIMERS
        stringstream timer_tag;
        timer_tag << prolog << "Concurrent Chunk Processing. sc_id: " << d_id;
        BES_STOPWATCH_START(SUPER_CHUNK_MODULE, timer_tag.str());
#endif
        queue<shared_ptr<Chunk>> chunks_to_process;
        for (const auto &chunk: d_chunks) {
            chunks_to_process.push(chunk);
        }
        process_chunks_unconstrained_concurrent(d_id, chunks_to_process, chunk_shape, d_parent_array, array_shape);
    }
}


/**
 * @brief Makes a string representation of the SuperChunk.
 * @param verbose If set true then details of the subordinate Chunks will be included.
 * @return  A string representation of the SuperChunk.
 */
string SuperChunk::to_string(bool verbose=false) const {
    stringstream msg;
    msg << "[SuperChunk: " << (void **)this;
    msg << " offset: " << d_offset;
    msg << " size: " << d_size ;
    msg << " chunk_count: " << d_chunks.size();
    //msg << " parent: " << d_parent->name();
    msg << "]";
    if (verbose) {
        msg << endl;
        for (auto chunk: d_chunks) {
            msg << chunk->to_string() << endl;
        }
    }
    return msg.str();
}

/**
 * @brief Writes the to_string() output to the stream strm.
 * @param strm
 */
void SuperChunk::dump(ostream & strm) const {
    strm << to_string(false) ;
}

// direct chunk method to read unconstrained variables.
void SuperChunk::read_unconstrained_dio() {

    //Retrieve data for the direct IO case.
    retrieve_data_dio();

    // The size in element of each of the array's dimensions
    const vector<unsigned long long> array_shape = d_parent_array->get_shape(true);
    // The size, in elements, of each of the chunk's dimensions
    const vector<unsigned long long> chunk_shape = d_parent_array->get_chunk_dimension_sizes();

    if(!DmrppRequestHandler::d_use_compute_threads){
#if DMRPP_ENABLE_THREAD_TIMERS
        BES_STOPWATCH_START(SUPER_CHUNK_MODULE, prolog + "Serial Chunk Processing. sc_id: " + d_id);
#endif
        for(const auto &chunk : d_chunks){
            process_one_chunk_unconstrained_dio(chunk, chunk_shape, d_parent_array, array_shape);
        }
    }
    else {
#if DMRPP_ENABLE_THREAD_TIMERS
        stringstream timer_tag;
        timer_tag << prolog << "Concurrent Chunk Processing. sc_id: " << d_id;
        BES_STOPWATCH_START(SUPER_CHUNK_MODULE, timer_tag.str());
#endif
        queue<shared_ptr<Chunk>> chunks_to_process;
        for (const auto &chunk : d_chunks)
            chunks_to_process.push(chunk);

        process_chunks_unconstrained_concurrent_dio(d_id,chunks_to_process, chunk_shape, d_parent_array, array_shape);
    }
}

} // namespace dmrpp
