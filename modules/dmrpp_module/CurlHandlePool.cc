// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: James Gallagher<jgallagher@opendap.org>
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
#include <locale>
#include <sstream>

#include <cstring>
#include <unistd.h>

#include <curl/curl.h>

#if HAVE_CURL_MULTI_H
#include <curl/multi.h>
#endif

#include "CurlUtils.h"

#include <time.h>

#include "util.h"   // long_to_string()

#include "BESLog.h"
#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESForbiddenError.h"
#include "TheBESKeys.h"
#include "AllowedHosts.h"

#include "DmrppRequestHandler.h"
#include "DmrppCommon.h"
#include "DmrppNames.h"
#include "awsv4.h"
#include "CurlHandlePool.h"
#include "Chunk.h"
#include "CredentialsManager.h"
#include "AccessCredentials.h"

#define KEEP_ALIVE 1   // Reuse libcurl easy handles (1) or not (0).
#define CURL_VERBOSE 0  // Logs curl info to the bes.log

static const int MAX_WAIT_MSECS = 30 * 1000; // Wait max. 30 seconds
static const unsigned int retry_limit = 10; // Amazon's suggestion
static const useconds_t uone_second = 1000 * 1000; // one second

namespace dmrpp {
#if HAVE_CURL_MULTI_API
const bool have_curl_multi_api = true;
#else
const bool have_curl_multi_api = false;
#endif
}

using namespace dmrpp;
using namespace std;
using namespace bes;

#define prolog std::string("CurlHandlePool::").append(__func__).append("() - ")

Lock::Lock(pthread_mutex_t &lock) : m_mutex(lock) {
    int status = pthread_mutex_lock(&m_mutex);
    if (status != 0) throw BESInternalError("Could not lock in CurlHandlePool", __FILE__, __LINE__);
}

Lock::~Lock() {
    int status = pthread_mutex_unlock(&m_mutex);
    if (status != 0)
        ERROR("Could not unlock in CurlHandlePool");
}

/**
 * @brief Build a string with hex info about stuff libcurl gets
 *
 * Unused, but might be useful someday.
 */
#if 0
static
string dump(const char *text, unsigned char *ptr, size_t size)
{
    size_t i;
    size_t c;
    unsigned int width=0x10;
 
    ostringstream oss;
    oss << text << ", " << std::setw(10) << (long)size << std::setbase(16) << (long)size << endl;

    for(i=0; i<size; i+= width) {
    oss << std::setw(4) << (long)i;
    // fprintf(stream, "%4.4lx: ", (long)i);

    /* show hex to the left */
    for(c = 0; c < width; c++) {
        if(i+c < size) {
        oss << std::setw(2) << ptr[i+c];
        //fprintf(stream, "%02x ", ptr[i+c]);
        }
        else {
        oss << "   ";
        // fputs("   ", stream);
        }
    }

    /* show data on the right */
    for(c = 0; (c < width) && (i+c < size); c++) {
        char x = (ptr[i+c] >= 0x20 && ptr[i+c] < 0x80) ? ptr[i+c] : '.';
        // fputc(x, stream);
        oss << std::setw(1) << x;
    }

    // fputc('\n', stream); /* newline */
    oss << endl;
    }

    return oss.str();
}
#endif

#if CURL_VERBOSE
/**
 * @brief print verbose info from curl.
 *
 * Copied from the libcurl docs, then hacked to remove most of the output.
 */ 
static
int curl_trace(CURL */*handle*/, curl_infotype type, char *data, size_t /*size*/, void */*userp*/)
{
    string text = "";
    switch (type) {
        // print info
        case CURLINFO_TEXT:
        case CURLINFO_HEADER_OUT:
        case CURLINFO_HEADER_IN: {
            text = data;
            size_t pos;
            while ((pos = text.find('\n')) != string::npos)
                text = text.substr(0, pos);
            break;
        }

        // Do not build up 'text' for the data transfers
        case CURLINFO_DATA_OUT:
        case CURLINFO_SSL_DATA_OUT:
        case CURLINFO_DATA_IN:
        case CURLINFO_SSL_DATA_IN:
        default: /* in case a new one is introduced to shock us */
            break;
    }

    switch (type) {
        // print info
        case CURLINFO_TEXT:
            LOG("libcurl == Info: " << text << endl);
            break;

        case CURLINFO_HEADER_OUT:
            LOG("libcurl == Send header: " << text << endl);
            break;
        case CURLINFO_HEADER_IN:
            LOG("libcurl == Recv header: " << text << endl);
            break;
#if 0
        // Only print these if we're desperate and the above code has been hacked to match
        case CURLINFO_DATA_OUT:
            LOG("libcurl == Send data" << text << endl);
            break;
        case CURLINFO_SSL_DATA_OUT:
            LOG("libcurl == Send SSL data" << text << endl);
            break;
        case CURLINFO_DATA_IN:
            LOG("libcurl == Recv data" << text << endl);
            break;
        case CURLINFO_SSL_DATA_IN:
            LOG("libcurl == Recv SSL data" << text << endl);
            break;
#endif
        default:
            break;
     }

     return 0;
}
#endif

dmrpp_easy_handle::dmrpp_easy_handle() : d_request_headers(0) {
    d_handle = curl_easy_init();
    if (!d_handle) throw BESInternalError("Could not allocate CURL handle", __FILE__, __LINE__);

    CURLcode res;

    if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_ERRORBUFFER, d_errbuf)))
        throw BESInternalError(string("CURL Error: ").append(curl_easy_strerror(res)), __FILE__, __LINE__);

#if CURL_VERBOSE
    if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_DEBUGFUNCTION, curl_trace)))
        throw BESInternalError(string("CURL Error: ").append(curl::error_message(res, d_errbuf)), __FILE__, __LINE__);
    // Many tests fail with this option, but it's still useful to see how connections
    // are treated. jhrg 10/2/18
    if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_VERBOSE, 1L)))
        throw BESInternalError(string("CURL Error: ").append(curl::error_message(res, d_errbuf)), __FILE__, __LINE__);
#endif

    if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_HEADERFUNCTION, chunk_header_callback)))
        throw BESInternalError(string("CURL Error: ").append(curl::error_message(res, d_errbuf)), __FILE__, __LINE__);

    // Pass all data to the 'write_data' function
    if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_WRITEFUNCTION, chunk_write_data)))
        throw BESInternalError(string("CURL Error: ").append(curl::error_message(res, d_errbuf)), __FILE__, __LINE__);

#ifdef CURLOPT_TCP_KEEPALIVE
    /* enable TCP keep-alive for this transfer */
    if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_TCP_KEEPALIVE, 1L)))
        throw BESInternalError(string("CURL Error: ").append(curl::error_message(res)), __FILE__, __LINE__);
#endif

#ifdef CURLOPT_TCP_KEEPIDLE
    /* keep-alive idle time to 120 seconds */
    if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_TCP_KEEPIDLE, 120L)))
        throw BESInternalError(string("CURL Error: ").append(curl::error_message(res)), __FILE__, __LINE__);
#endif

#ifdef CURLOPT_TCP_KEEPINTVL
    /* interval time between keep-alive probes: 120 seconds */
    if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_TCP_KEEPINTVL, 120L)))
        throw BESInternalError(string("CURL Error: ").append(curl::error_message(res)), __FILE__, __LINE__)
#endif

    d_in_use = false;
    d_url = "";
    d_chunk = 0;
}

dmrpp_easy_handle::~dmrpp_easy_handle() {
    if (d_handle) curl_easy_cleanup(d_handle);
    if (d_request_headers) curl_slist_free_all(d_request_headers);
}

/**
 * @brief This is the read_data() method for serial transfers.
 *
 * See below for the code used for parallel transfers. Note that
 * this code is also used for the pthreads parallel transfers.
 */
void dmrpp_easy_handle::read_data() {
    // Treat HTTP/S requests specially; retry some kinds of failures.
    if (d_url.find("https://") == 0 || d_url.find("http://") == 0) {
        unsigned int tries = 0;
        bool success = true;
        useconds_t retry_time = uone_second/4;
        d_errbuf[0] = 0; // Initialize to empty string.

        // Perform the request
        do {
            BESDEBUG(DMRPP_CURL, prolog << "Requesting URL: " << d_url << endl);
            CURLcode curl_code = curl_easy_perform(d_handle);
            ++tries;

            if (CURLE_OK != curl_code) {
                stringstream msg;
                msg << "Data transfer error: " << curl::error_message(curl_code, d_errbuf);
                char *effective_url = 0;
                curl_easy_getinfo(d_handle, CURLINFO_EFFECTIVE_URL, &effective_url);
                msg << " last_url: " << effective_url;
                BESDEBUG(DMRPP_CURL, prolog << msg.str() << endl);
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
            }

            success = curl::eval_get_response(d_handle, d_url);

            if (!success) {
                if (tries == retry_limit) {
                    string msg = prolog + "Data transfer error: Number of re-tries exceeded: "+ curl::error_message(curl_code, d_errbuf);
                    LOG(msg << endl);
                    throw BESInternalError(msg, __FILE__, __LINE__);
                }
                else {
                    LOG(prolog << "HTTP transfer 500 error, will retry (trial " << tries << " for: " << d_url << ")." << endl);
                    usleep(retry_time);
                    retry_time *= 2;
                }
            }

            curl_slist_free_all(d_request_headers);
            d_request_headers = 0;
        } while (!success);
    }
    else {
        CURLcode curl_code = curl_easy_perform(d_handle);
        if (CURLE_OK != curl_code) {
            throw BESInternalError(string("Data transfer error: ").append(curl::error_message(curl_code, d_errbuf)),
                                   __FILE__, __LINE__);
        }
    }
    d_chunk->set_is_read(true);
}

/**
 * The implementation of the dmrpp_multi_handle field. It can be either
 * a CURLM* or a vector of dmrpp_easy_handle*, depending on whether libcurl
 * has the CULRM* interface.
 *
 * @note This uses the pimpl pattern.
 */
struct dmrpp_multi_handle::multi_handle {
#if HAVE_CURL_MULTI_API
    CURLM *curlm;
#else
    std::vector<dmrpp_easy_handle *> ehandles;
#endif
};

dmrpp_multi_handle::dmrpp_multi_handle() {
    p_impl = new multi_handle;
#if HAVE_CURL_MULTI_API
    p_impl->curlm = curl_multi_init();
#endif
}

dmrpp_multi_handle::~dmrpp_multi_handle() {
#if HAVE_CURL_MULTI_API
    curl_multi_cleanup(p_impl->curlm);
#endif
    delete p_impl;
}

/**
 * @brief Add an Easy Handle to a Multi Handle object.
 *
 * @note It is the responsibility of the caller to make sure there are not
 * too many handles added to the 'multi handle' object.
 *
 * @param eh The CURL easy handle to add
 */
void dmrpp_multi_handle::add_easy_handle(dmrpp_easy_handle *eh) {
#if HAVE_CURL_MULTI_API
    curl_multi_add_handle(p_impl->curlm, eh->d_handle);
#else
    p_impl->ehandles.push_back(eh);
#endif
}

void dmrpp_multi_handle::remove_easy_handle(dmrpp_easy_handle *eh) {
#if HAVE_CURL_MULTI_API
    CURLMcode mres = curl_multi_remove_handle(p_impl->curlm, eh);
    if (mres != CURLM_OK)
        LOG("While cleaning up CURL multihandle during a parallel data request, could not remove a CURL handle.");
#else
    for (vector<dmrpp_easy_handle *>::iterator i = p_impl->ehandles.begin(), e = p_impl->ehandles.end(); i != e; ++i) {
        if (eh == *i)  {
            p_impl->ehandles.erase(i);
            break;
        }
    }
#endif
}

// This is only used if we don't have the Multi API and have to use pthreads.
// It is a callback used in dmrpp_multi_handle::read_data() below.
// jhrg 8/27/18
#if !HAVE_CURL_MULTI_API

static void *easy_handle_read_data(void *handle) {
    dmrpp_easy_handle *eh = reinterpret_cast<dmrpp_easy_handle *>(handle);

    try {
        eh->read_data();
        pthread_exit(0);
    }
    catch (BESError &e) {
        string *error = new string(e.get_verbose_message());
        pthread_exit(error);
    }
}

#endif

/**
 * @brief The read_data() method for parallel transfers
 *
 * This uses either the CURL Multi API or pthreads to read N
 * dmrpp_easy_handle instances in parallel.
 *
 * @todo This has to be fixed to restart 500 HTTP errors and to clean up
 * after threads if there's an exception.
 *
 * @note It's the responsibility of the caller to make sure that no more than
 * d_max_parallel_transfers are added to the 'multi' handle.
 */
void dmrpp_multi_handle::read_data() {
#if 1 //HAVE_CURL_MULTI_API
    useconds_t retry_time = uone_second/4;
    // Use the libcurl Multi API here. Alternate version follows...
    try {
        int still_running = 0;
        CURLMcode mres = curl_multi_perform(p_impl->curlm, &still_running);
        if (mres != CURLM_OK)
            throw BESInternalError(string("Could not initiate data read: ").append(curl_multi_strerror(mres)), __FILE__,
                __LINE__);

        do {
            int numfds = 0;
            mres = curl_multi_wait(p_impl->curlm, NULL, 0, MAX_WAIT_MSECS, &numfds);
            if (mres != CURLM_OK)
                throw BESInternalError(string("Could not wait on data read: ").append(curl_multi_strerror(mres)), __FILE__,
                    __LINE__);

            mres = curl_multi_perform(p_impl->curlm, &still_running);
            if (mres != CURLM_OK)
                throw BESInternalError(string("Could not iterate data read: ").append(curl_multi_strerror(mres)), __FILE__,
                    __LINE__);

        } while (still_running);

        CURLMsg *msg = 0;
        int msgs_left = 0;
        while ((msg = curl_multi_info_read(p_impl->curlm, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                CURL *eh = msg->easy_handle;

                CURLcode res = msg->data.result;
                if (res != CURLE_OK)
                    throw BESInternalError(string("Error HTTP: ").append(curl_easy_strerror(res)), __FILE__, __LINE__);

                // Note: 'eh' is the easy handle returned by culr_multi_info_read(),
                // but in it's private field is our dmrpp_easy_handle object. We need
                // both to mark this data read operation as complete.
                dmrpp_easy_handle *dmrpp_easy_handle = 0;
                res = curl_easy_getinfo(eh, CURLINFO_PRIVATE, &dmrpp_easy_handle);
                if (res != CURLE_OK)
                    throw BESInternalError(string("Could not access easy handle: ").append(curl_easy_strerror(res)), __FILE__, __LINE__);

                // This code has to work with both http/s: and file: protocols. Here we check the
                // HTTP status code. If the protocol is not HTTP, we assume since msg->data.result
                // returned CURLE_OK, that the transfer worked. jhrg 5/1/18
                if (dmrpp_easy_handle->d_url.find("http://") == 0 || dmrpp_easy_handle->d_url.find("https://") == 0) {
                    unsigned int attempts = 0;
                    bool success = false;
                    while(!success && attempts<retry_limit) {
                        success = curl::eval_get_response(eh, dmrpp_easy_handle->d_url);
                        LOG(prolog << "ERROR - HTTP transfer error, will retry (attempt: " << attempts << " for: " << dmrpp_easy_handle->d_url << ")." << endl);
                        usleep(retry_time);
                        retry_time *= 2;
                        attempts++;
                    }
                    if (!success) {
                        string msg = prolog + "Data transfer error: Number of re-tries exceeded.";
                        LOG(msg << endl);
                        throw BESInternalError(msg, __FILE__, __LINE__);
                    }
                }

                // If we are here, the request was successful.
                dmrpp_easy_handle->d_chunk->set_is_read(true);  // Set the is_read() property for chunk here.

                // NB: Remove the handle from the CURLM* and _then_ call release_handle()
                // so that the KEEP_ALIVE 0 (off) works. Calling delete on the dmrpp_easy_handle
                // will invalidate 'eh', so call that after removing 'eh'.
                mres = curl_multi_remove_handle(p_impl->curlm, eh);
                if (mres != CURLM_OK)
                    throw BESInternalError(string("Could not remove libcurl handle: ").append(curl_multi_strerror(mres)),  __FILE__, __LINE__);

                DmrppRequestHandler::curl_handle_pool->release_handle(dmrpp_easy_handle);
            }
            else {  // != CURLMSG_DONE
                throw BESInternalError("Error getting HTTP or FILE responses.", __FILE__, __LINE__);
            }
        }
    }
    catch (...) {
        // If we throw here, remove all of the easy handles from this multihandle
        // That will prevent old URLs part that were part of a failed request from
        // being part of a subsequent request.
        // With C++11 this could be for (auto &i: DmrppRequestHandler::curl_handle_pool->d_easy_handles) { ... }
        for (std::vector<dmrpp_easy_handle *>::iterator i = DmrppRequestHandler::curl_handle_pool->d_easy_handles.begin(),
                e = DmrppRequestHandler::curl_handle_pool->d_easy_handles.end(); i != e; ++i) {
            CURLMcode mres = curl_multi_remove_handle(p_impl->curlm, (*i)->d_handle);
            if (mres != CURLM_OK)
                LOG("While cleaning up from an error during a parallel data request, could not remove a CURL handle." << endl);
            DmrppRequestHandler::curl_handle_pool->release_handle(*i);
        }

        throw;  // re-throw the exception
    }
#else
    // Start the processing pipelines using pthreads - there is no Multi API

    pthread_t threads[p_impl->ehandles.size()];
    unsigned int num_threads = 0;
    try {
        for (unsigned int i = 0; i < p_impl->ehandles.size(); ++i) {
            int status = pthread_create(&threads[i], NULL, easy_handle_read_data, (void *) p_impl->ehandles[i]);
            if (status == 0) {
                ++num_threads;
            }
            else {
                ostringstream oss("Could not start process_one_chunk_unconstrained thread for chunk ", std::ios::ate);
                oss << i << ": " << strerror(status);
                throw BESInternalError(oss.str(), __FILE__, __LINE__);
            }
        }

        // Now join the child threads.
        for (unsigned int i = 0; i < num_threads; ++i) {
            string *error;
            int status = pthread_join(threads[i], (void **) &error);
            if (status != 0) {
                ostringstream oss("Could not join process_one_chunk_unconstrained thread for chunk ", std::ios::ate);
                oss << i << ": " << strerror(status);
                throw BESInternalError(oss.str(), __FILE__, __LINE__);
            }
            else if (error != 0) {
                BESInternalError e(*error, __FILE__, __LINE__);
                delete error;
                throw e;
            }
        }
        // Now remove the easy_handles, mimicking the behavior when using the real Multi API
        p_impl->ehandles.clear();
    }
    catch (...) {
        // Returning all the easy handles to the pool is handled for the 'no HAVE_CURL_MULTI_API
        // case in Chunk's chunk_write_data() libcurl callback.
        DmrppRequestHandler::curl_handle_pool->release_all_handles();

        // Now remove the easy_handles, mimicking the behavior when using the real Multi API
        p_impl->ehandles.clear();

        join_threads(threads, num_threads);

        throw;
    }
#endif
}

CurlHandlePool::CurlHandlePool() : d_multi_handle(0) {
    d_max_easy_handles = DmrppRequestHandler::d_max_parallel_transfers;
    d_multi_handle = new dmrpp_multi_handle();

    for (unsigned int i = 0; i < d_max_easy_handles; ++i) {
        d_easy_handles.push_back(new dmrpp_easy_handle());
    }

    if (pthread_mutex_init(&d_get_easy_handle_mutex, 0) != 0)
        throw BESInternalError("Could not initialize mutex in CurlHandlePool", __FILE__, __LINE__);
}

/**
 * @brief Add the given header & value to the curl slist.
 *
 * The call must free the slist after the curl_easy_perform() is called, not after
 * the headers are added to the curl handle.
 *
 * @param slist The list; initially pass nullptr to create a new list
 * @param header The header
 * @param value The value
 * @return The modified slist pointer or nullptr if an error occurred.
 */
static struct curl_slist *
append_http_header(curl_slist *slist, const string &header, const string &value) {
    string full_header = header;
    full_header.append(" ").append(value);

    struct curl_slist *temp = curl_slist_append(slist, full_header.c_str());
    return temp;
}

/**
 * Get a CURL easy handle to transfer data from \arg url into the given \arg chunk.
 *
 * @note This method and release_handle() use the same lock to prevent the handle's
 * chunk pointer from being cleared by another thread after a thread running this
 * method has set it. However, there's no protection against calling this when no
 * more handles are available. If that happens a thread calling release_handle()
 * will block until this code returns (and this code will return NULL).
 *
 * @param chunk Use this Chunk to set a libcurl easy handle so that it
 * will fetch the Chunk's data.
 * @return A CURL easy handle configured to transfer data, or null if
 * there are no more handles in the pool.
 */
dmrpp_easy_handle *
CurlHandlePool::get_easy_handle(Chunk *chunk) {
    // Here we check to make sure that the we are only going to
    // access an approved location with this easy_handle
    if (!AllowedHosts::theHosts()->is_allowed(chunk->get_data_url())) {
        string msg = "ERROR!! The chunk url " + chunk->get_data_url() + " does not match any white-list rule. ";
        throw BESForbiddenError(msg, __FILE__, __LINE__);
    }

    Lock lock(d_get_easy_handle_mutex); // RAII

    dmrpp_easy_handle *handle = 0;
    for (vector<dmrpp_easy_handle *>::iterator i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
        if (!(*i)->d_in_use) {
            handle = *i;
            break;
        }
    }

    if (handle) {
        // Once here, d_easy_handle holds a CURL* we can use.
        handle->d_in_use = true;
        handle->d_url = chunk->get_data_url();

        handle->d_chunk = chunk;

        CURLcode res = curl_easy_setopt(handle->d_handle, CURLOPT_URL, chunk->get_data_url().c_str());
        if (res != CURLE_OK)
            throw BESInternalError(string("HTTP Error setting URL: ").append(
                    curl::error_message(res, handle->d_errbuf)), __FILE__, __LINE__);

        // get the offset to offset + size bytes
        if (CURLE_OK !=
            (res = curl_easy_setopt(handle->d_handle, CURLOPT_RANGE, chunk->get_curl_range_arg_string().c_str())))
            throw BESInternalError(
                    string("HTTP Error setting Range: ").append(curl::error_message(res, handle->d_errbuf)), __FILE__,
                    __LINE__);

        // Pass this to chunk_header_callback as the fourth argument
        if (CURLE_OK != (res = curl_easy_setopt(handle->d_handle, CURLOPT_HEADERDATA, reinterpret_cast<void *>(chunk))))
            throw BESInternalError(string("CURL Error setting chunk as header callback data: ").append(
                    curl::error_message(res, handle->d_errbuf)),
                                   __FILE__, __LINE__);

        // Pass this to chunk_write_data as the fourth argument
        if (CURLE_OK != (res = curl_easy_setopt(handle->d_handle, CURLOPT_WRITEDATA, reinterpret_cast<void *>(chunk))))
            throw BESInternalError(string("CURL Error setting chunk as response callback data: ").append(
                    curl::error_message(res, handle->d_errbuf)),
                                   __FILE__, __LINE__);

        // store the easy_handle so that we can call release_handle in multi_handle::read_data()
        if (CURLE_OK != (res = curl_easy_setopt(handle->d_handle, CURLOPT_PRIVATE, reinterpret_cast<void *>(handle))))
            throw BESInternalError(string("CURL Error setting easy_handle as private data: ").append(
                    curl::error_message(res, handle->d_errbuf)), __FILE__,
                                   __LINE__);

        // Enabled cookies
        // #TODO #FIXME Make these file names configuration based.
        curl_easy_setopt(handle->d_handle, CURLOPT_COOKIEFILE, curl::get_cookie_filename().c_str());
        curl_easy_setopt(handle->d_handle, CURLOPT_COOKIEJAR, curl::get_cookie_filename().c_str());

        // Follow 302 (redirect) responses
        curl_easy_setopt(handle->d_handle, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(handle->d_handle, CURLOPT_MAXREDIRS, 20);

        // Set the user agent something otherwise TEA will never redirect to URS.
        curl_easy_setopt(handle->d_handle, CURLOPT_USERAGENT, "Hyrax"/* curl_version()*/);

        // This means libcurl will use Basic, Digest, GSS Negotiate, or NTLM,
        // choosing the the 'safest' one supported by the server.
        // This requires curl 7.10.6 which is still in pre-release. 07/25/03 jhrg
        curl_easy_setopt(handle->d_handle, CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY);

        // Enable using the .netrc credentials file.
        curl_easy_setopt(handle->d_handle, CURLOPT_NETRC, 1);

        // If the configuration specifies a particular .netrc credentials file, use it.
        // TODO move this operation into constructor and stash the value.
        string netrc_file = curl::get_netrc_filename();
        if (!netrc_file.empty()) {
            curl_easy_setopt(handle->d_handle, CURLOPT_NETRC_FILE, netrc_file.c_str());
        }
        VERBOSE(__FILE__ << "::get_easy_handle() is using the netrc file '"
                         << ((!netrc_file.empty()) ? netrc_file : "~/.netrc") << "'" << endl);

        AccessCredentials *credentials = CredentialsManager::theCM()->get(handle->d_url);
        if (credentials && credentials->isS3Cred()) {
            BESDEBUG(DMRPP_CURL, prolog << "Got AccessCredentials instance: " << endl << credentials->to_json() << endl);
            // If there are available credentials, and they are S3 credentials then we need to sign
            // the request
            const std::time_t request_time = std::time(0);

            const std::string auth_header =
                    AWSV4::compute_awsv4_signature(
                            handle->d_url,
                            request_time,
                            credentials->get(AccessCredentials::ID_KEY),
                            credentials->get(AccessCredentials::KEY_KEY),
                            credentials->get(AccessCredentials::REGION_KEY),
                            "s3");

            // passing nullptr for the first call allocates the curl_slist
            // The following code builds the slist that holds the headers. This slist is freed
            // once the URL is dereferenced in dmrpp_easy_handle::read_data(). jhrg 11/26/19
            handle->d_request_headers = append_http_header(0, "Authorization:", auth_header);
            if (!handle->d_request_headers)
                throw BESInternalError(
                        string("CURL Error setting Authorization header: ").append(
                                curl::error_message(res, handle->d_errbuf)), __FILE__, __LINE__);

            // We pre-compute the sha256 hash of a null message body
            curl_slist *temp = append_http_header(handle->d_request_headers, "x-amz-content-sha256:",
                                                  "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
            if (!temp)
                throw BESInternalError(
                        string("CURL Error setting x-amz-content-sha256: ").append(
                                curl::error_message(res, handle->d_errbuf)),
                        __FILE__, __LINE__);
            handle->d_request_headers = temp;

            temp = append_http_header(handle->d_request_headers, "x-amz-date:", AWSV4::ISO8601_date(request_time));
            if (!temp)
                throw BESInternalError(
                        string("CURL Error setting x-amz-date header: ").append(
                                curl::error_message(res, handle->d_errbuf)),
                        __FILE__, __LINE__);
            handle->d_request_headers = temp;


            if (CURLE_OK != (res = curl_easy_setopt(handle->d_handle, CURLOPT_HTTPHEADER, handle->d_request_headers)))
                throw BESInternalError(string("CURL Error setting HTTP headers for S3 authentication: ").append(
                        curl::error_message(res, handle->d_errbuf)), __FILE__, __LINE__);
        }
    }

    return handle;
}

/**
 * Release a DMR++ easy_handle. This returns the handle to the pool of handles
 * that can be used for serial transfers or, with multi curl, for parallel transfers.
 *
 * @param handle
 */
void CurlHandlePool::release_handle(dmrpp_easy_handle *handle) {
    // In get_easy_handle, it's possible that d_in_use could be false and d_chunk
    // could not be set to 0 (because a separate thread could be running these
    // methods). In that case, the thread running get_easy_handle could set d_chunk,
    // and then this thread could clear it (... unlikely, but an optimizing compiler is
    // free to reorder statements so long as they don't alter the function's behavior).
    // Timing tests indicate this lock does not cost anything that can be measured.
    // jhrg 8/21/18
    Lock lock(d_get_easy_handle_mutex);

#if KEEP_ALIVE
    handle->d_url = "";
    handle->d_chunk = 0;
    handle->d_in_use = false;
#else
    // This is to test the effect of libcurl Keep Alive support
    // Find the handle; erase from the vector; delete; allocate a new handle and push it back on
    for (std::vector<dmrpp_easy_handle *>::iterator i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
        if (*i == handle) {
            BESDEBUG("dmrpp:5", "Found a handle match for the " << i - d_easy_handles.begin() << "th easy handle." << endl);
            delete handle;
            *i = new dmrpp_easy_handle();
            break;
        }
    }
#endif
}

/**
 * @brief Release the handle associated with a given chunk
 * This is intended for use in error clean up code.
 * @param chunk Find the handle for this chunk and release it.
 */
void CurlHandlePool::release_handle(Chunk *chunk) {
    for (std::vector<dmrpp_easy_handle *>::iterator i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
        if ((*i)->d_chunk == chunk) {
            release_handle(*i);
            break;
        }
    }
}

/**
 * @breif release all outstanding curl handles
 * If one access in a multi-transfer fails because of an error such as
 * Access Denied, end the entire process and free all curl handles. This
 * is different from an Internal Server Error response, which should be
 * retried without ending the other accesses.
 */
void CurlHandlePool::release_all_handles() {
    for (std::vector<dmrpp_easy_handle *>::iterator i = d_easy_handles.begin(), e = d_easy_handles.end(); i != e; ++i) {
        release_handle(*i);
    }
}
