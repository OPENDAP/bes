// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2011 OPeNDAP, Inc.
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

//#define DODS_DEBUG

#include <sys/stat.h>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include <libdap/DDS.h>
#include <libdap/ConstraintEvaluator.h>
#include <libdap/DDXParserSAX2.h>
#include <libdap/XDRStreamMarshaller.h>
#include <libdap/XDRStreamUnMarshaller.h>

#include <libdap/debug.h>
#include <libdap/mime_util.h>	// for last_modified_time() and rfc_822_date()
#include <libdap/util.h>

#include "BESDapResponseCache.h"
#include "BESDapResponseBuilder.h"
#include "BESInternalError.h"

#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "BESDebug.h"

#define CRLF "\r\n"

using namespace std;
using namespace libdap;

BESDapResponseCache *BESDapResponseCache::d_instance = 0;
const string BESDapResponseCache::PATH_KEY = "DAP.ResponseCache.path";
const string BESDapResponseCache::PREFIX_KEY = "DAP.ResponseCache.prefix";
const string BESDapResponseCache::SIZE_KEY = "DAP.ResponseCache.size";

unsigned long BESDapResponseCache::getCacheSizeFromConfig()
{
    bool found;
    string size;
    unsigned long size_in_megabytes = 0;
    TheBESKeys::TheKeys()->get_value(SIZE_KEY, size, found);
    if (found) {
        BESDEBUG("dap_response_cache",
                "BESDapResponseCache::getCacheSizeFromConfig(): Located BES key " << SIZE_KEY<< "=" << size << endl);
        istringstream iss(size);
        iss >> size_in_megabytes;
    }
    else {
        // FIXME This should not throw an exception. jhrg 10/20/15
        string msg = "[ERROR] BESDapResponseCache::getCacheSizeFromConfig() - The BES Key " + SIZE_KEY
                + " is not set! It MUST be set to utilize the DAP response cache. ";
        BESDEBUG("dap_response_cache", msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    return size_in_megabytes;
}

string BESDapResponseCache::getCachePrefixFromConfig()
{
    bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value(PREFIX_KEY, prefix, found);
    if (found) {
        BESDEBUG("dap_response_cache",
                "BESDapResponseCache::getCachePrefixFromConfig(): Located BES key " << PREFIX_KEY<< "=" << prefix << endl);
        prefix = BESUtil::lowercase(prefix);
    }
    else {
        string msg = "[ERROR] BESDapResponseCache::getCachePrefixFromConfig() - The BES Key " + PREFIX_KEY
                + " is not set! It MUST be set to utilize the DAP response cache. ";
        BESDEBUG("dap_response_cache", msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    return prefix;
}

string BESDapResponseCache::getCacheDirFromConfig()
{
    bool found;

    string cacheDir = "";
    TheBESKeys::TheKeys()->get_value(PATH_KEY, cacheDir, found);
    if (found) {
        BESDEBUG("dap_response_cache",
                "BESDapResponseCache::getCacheDirFromConfig(): Located BES key " << PATH_KEY<< "=" << cacheDir << endl);
    }
    else {
        string msg = "[ERROR] BESDapResponseCache::getCacheDirFromConfig() - The BES Key " + PATH_KEY
                + " is not set! It MUST be set to utilize the DAP response cache. ";
        BESDEBUG("dap_response_cache", msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    return cacheDir;
}

BESDapResponseCache::BESDapResponseCache()
{
    BESDEBUG("dap_response_cache", "BESDapResponseCache::BESDapResponseCache() - BEGIN" << endl);

    string cacheDir = getCacheDirFromConfig();
    string prefix = getCachePrefixFromConfig();
    unsigned long size_in_megabytes = getCacheSizeFromConfig();

    BESDEBUG("dap_response_cache",
            "BESDapResponseCache::BESDapResponseCache() - Cache config params: " << cacheDir << ", " << prefix << ", " << size_in_megabytes << endl);

    // The required params must be present. If initialize() is not called,
    // then d_cache will stay null and is_available() will return false.
    // Also, the directory 'path' must exist, or d_cache will be null.
    if (!cacheDir.empty() && size_in_megabytes > 0)
    	initialize(cacheDir, prefix, size_in_megabytes);

    BESDEBUG("dap_response_cache", "BESDapResponseCache::BESDapResponseCache() - END" << endl);
}

/** Get an instance of the BESDapResponseCache object. This class is a singleton, so the
 * first call to any of three 'get_instance()' methods makes an instance and subsequent calls
 * return a pointer to that instance.
 *
 *
 * @param cache_dir_key Key to use to get the value of the cache directory
 * @param prefix_key Key for the item/file prefix. Each file added to the cache uses this
 * as a prefix so cached items can be easily identified when /tmp is used for the cache.
 * @param size_key How big should the cache be, in megabytes
 * @return A pointer to a BESDapResponseCache object
 */
BESDapResponseCache *
BESDapResponseCache::get_instance(const string &cache_dir, const string &prefix, unsigned long long size)
{
    if (d_instance == 0) {
        try {
            if (dir_exists(cache_dir)) {
                d_instance = new BESDapResponseCache(cache_dir, prefix, size);
#ifdef HAVE_ATEXIT
                atexit(delete_instance);
#endif
            }
        }
        catch (BESInternalError &bie) {
            BESDEBUG("dap_response_cache",
                    "BESDapResponseCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
        }

    }

    BESDEBUG("dap_response_cache", "BESDapResponseCache::get_instance(dir,prefix,size) - d_instance: " << d_instance << endl);

    return d_instance;
}

/** Get the default instance of the BESDapResponseCache object. This will read "TheBESKeys" looking for the values
 * of FUNCTION_CACHE_PATH, FUNCTION_CACHE_PREFIX, an FUNCTION_CACHE_SIZE to initialize the cache.
 */
BESDapResponseCache *
BESDapResponseCache::get_instance()
{
    if (d_instance == 0) {
        try {
            if (dir_exists(getCacheDirFromConfig())) {
                d_instance = new BESDapResponseCache();
#ifdef HAVE_ATEXIT
                atexit(delete_instance);
#endif
            }
        }
        catch (BESInternalError &bie) {
            BESDEBUG("dap_response_cache",
                    "BESDapResponseCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
            d_instance = 0;
        }
    }
    BESDEBUG("dap_response_cache", "BESDapResponseCache::get_instance() - d_instance: " << d_instance << endl);

    return d_instance;
}

/**
 * Is the item named by cache_entry_name valid? This code tests that the
 * cache entry is non-zero in size (returns false if that is the case, although
 * that might not be correct) and that the dataset associated with this
 * ResponseBuilder instance is at least as old as the cached entry.
 *
 * @param cache_file_name File name of the cached entry
 * @return True if the thing is valid, false otherwise.
 */
bool BESDapResponseCache::is_valid(const string &cache_file_name, const string &dataset)
{
    // If the cached response is zero bytes in size, it's not valid. This is true
    // because a DAP data object, even if it has no data still has a metadata part.
    // jhrg 10/20/15

    off_t entry_size = 0;
    time_t entry_time = 0;
    struct stat buf;
    if (stat(cache_file_name.c_str(), &buf) == 0) {
        entry_size = buf.st_size;
        entry_time = buf.st_mtime;
    }
    else {
        return false;
    }

    if (entry_size == 0) return false;

    time_t dataset_time = entry_time;
    if (stat(dataset.c_str(), &buf) == 0) {
        dataset_time = buf.st_mtime;
    }

    // Trick: if the d_dataset is not a file, stat() returns error and
    // the times stay equal and the code uses the cache entry.

    // TODO Fix this so that the code can get a LMT from the correct handler.
    if (dataset_time > entry_time) return false;

    return true;
}

/**
 * Read the data from the saved response document.
 *
 * @note this method is made of code copied from Connect (process_data(0)
 * but this copy assumes it is reading a DDX with data written using the
 * code in ResponseCache::cache_data_ddx().
 *
 * @param data The input stream
 * @parma fdds Load this DDS object with the variables, attributes and
 * data values from the cached DDS.
 */
void BESDapResponseCache::read_data_from_cache(const string &cache_file_name, DDS *fdds)
{
    BESDEBUG("dap_response_cache", __PRETTY_FUNCTION__ << " Opening cache file: " << cache_file_name << endl);

    ifstream data(cache_file_name.c_str());

    // Rip off the MIME headers from the response if they are present
    string mime = get_next_mime_header(data);
    while (!mime.empty()) {
        mime = get_next_mime_header(data);
    }

    // Parse the DDX; throw an exception on error.
    DDXParser ddx_parser(fdds->get_factory());
#if 1
    // Read the MPM boundary and then read the subsequent headers
    string boundary = read_multipart_boundary(data);

    read_multipart_headers(data, "text/xml", dods_ddx);
#endif
    // Parse the DDX, reading up to and including the next boundary.
    // Return the CID for the matching data part
    string data_cid;
    try {
        ddx_parser.intern_stream(data, fdds, data_cid, boundary);
    }
    catch (Error &e) {
        BESDEBUG("dap_response_cache", "BESDapResponseCache::read_data_from_cache() - [ERROR] DDX Parser Error: " << e.get_error_message() << endl);
        throw;
    }
#if 1
    // Munge the CID into something we can work with
    data_cid = cid_to_header_value(data_cid);

    // Read the data part's MPM part headers (boundary was read by
    // DDXParse::intern)
    read_multipart_headers(data, "application/octet-stream", dods_data_ddx /* old value? dap4_data */, data_cid);
#endif
    // Now read the data

    // XDRFileUnMarshaller um(data);
    XDRStreamUnMarshaller um(data);
    for (DDS::Vars_iter i = fdds->var_begin(); i != fdds->var_end(); i++) {
        (*i)->deserialize(um, fdds);
    }
}

/**
 * Read data from cache. Allocates a new DDS using the given factory.
 *
 */
DDS *
BESDapResponseCache::get_cached_data_ddx(const string &cache_file_name, BaseTypeFactory *factory,
        const string &datasset_name)
{
    BESDEBUG("dap_response_cache", __PRETTY_FUNCTION__ << " Reading cache for " << cache_file_name << endl);

    DDS *fdds = new DDS(factory);

    fdds->filename(datasset_name);

    read_data_from_cache(cache_file_name, fdds);

    fdds->set_factory(0);

    // mark everything as read. And 'to send.' That is, make sure that when a response
    // is retrieved from the cache, all of the variables are marked as 'to be sent.'
    DDS::Vars_iter i = fdds->var_begin();
    while (i != fdds->var_end()) {
        (*i)->set_read_p(true);
        (*i++)->set_send_p(true);
    }

    return fdds;
}

DDS *
BESDapResponseCache::cache_dataset(DDS &dds, const string &constraint, BESDapResponseBuilder *rb,
        ConstraintEvaluator *eval, string &cache_token)
{
    // These are used for the cached or newly created DDS object
    BaseTypeFactory factory;
    DDS *fdds;

    // Build the response_id. Since the response content is a function of both the dataset AND the constraint,
    // glue them together to get a unique id for the response.
    string response_id = dds.filename() + "#" + constraint;

    // Get the cache filename for this thing.
    string cache_file_name = get_cache_file_name(response_id, /*mangle*/true);

	BESDEBUG("dap_response_cache", __PRETTY_FUNCTION__ << " cache_file_name: " << cache_file_name << endl);
    int fd;
    try {
        // If the object in the cache is not valid, remove it. The read_lock will
        // then fail and the code will drop down to the create_and_lock() call.
        // is_valid() tests for a non-zero object and for d_dateset newer than
        // the cached object.
        if (!is_valid(cache_file_name, dds.filename()))
        	purge_file(cache_file_name);

        if (get_read_lock(cache_file_name, fd)) {
            BESDEBUG("dap_response_cache", __PRETTY_FUNCTION__ << " Cache hit (1) for: " << cache_file_name << endl);
            fdds = get_cached_data_ddx(cache_file_name, &factory, dds.filename());
        }
        else if (create_and_lock(cache_file_name, fd)) {
            // If here, the cache_file_name could not be locked for read access;
            // try to build it. First make an empty file and get an exclusive lock on it.
            BESDEBUG("dap_response_cache", __PRETTY_FUNCTION__ << " Caching " << cache_file_name << ", constraint: " << constraint << endl);

            fdds = new DDS(dds);
            eval->parse_constraint(constraint, *fdds);

            // FIXME fix the function eval to allow functions and CEs together
            if (eval->function_clauses()) {
                DDS *temp_fdds = eval->eval_function_clauses(*fdds);
                delete fdds;
                fdds = temp_fdds;
            }

            ofstream data_stream(cache_file_name.c_str());
            if (!data_stream) {
                throw BESInternalError("Could not open '" + cache_file_name + "' to write cached response.", __FILE__, __LINE__);
            }

#if 1
            // FIXME Write a better 'serialize' for caching - can just dump data using the
            // local word order.

            string start = "dataddx_cache_start", boundary = "dataddx_cache_boundary";

            // This is a bit of a hack, but it effectively uses ResponseBuilder to write the
            // cached object/response without calling the machinery in one of the send_*()
            // methods. Those methods assume they need to evaluate the BESDapResponseBuilder's
            // CE, which is not necessary and will alter the values of the send_p property
            // of the DDS's variables.
            set_mime_multipart(data_stream, boundary, start, dods_data_ddx, x_plain,
                    last_modified_time(rb->get_dataset_name()));

            // Setting the version to 3.2 causes send_data_ddx to write the MIME headers that
            // the cache expects.
            fdds->set_dap_version("3.2");

            // Use a ConstraintEvaluator that has not parsed a CE so the code can use
            // the send method(s)
            ConstraintEvaluator eval;

            rb->serialize_dap2_data_ddx(data_stream, *fdds, eval, boundary, start);

            data_stream << CRLF << "--" << boundary << "--" << CRLF;
#endif
            data_stream.close();

            // Change the exclusive lock on the new file to a shared lock. This keeps
            // other processes from purging the new file and ensures that the reading
            // process can use it.
            exclusive_to_shared_lock(fd);

            // Now update the total cache size info and purge if needed. The new file's
            // name is passed into the purge method because this process cannot detect its
            // own lock on the file.
            unsigned long long size = update_cache_info(cache_file_name);
            if (cache_too_big(size)) update_and_purge(cache_file_name);
        }
        // get_read_lock() returns immediately if the file does not exist,
        // but blocks waiting to get a shared lock if the file does exist.
        else if (get_read_lock(cache_file_name, fd)) {
            BESDEBUG("dap_response_cache", __PRETTY_FUNCTION__ << " cache hit (2) for: " << cache_file_name << endl);
            fdds = get_cached_data_ddx(cache_file_name, &factory, dds.get_dataset_name());
        }
        else {
            throw BESInternalError("Cache error! Unable to acquire DAP Response cache.", __FILE__, __LINE__);
        }
    }
    catch (...) {
        BESDEBUG("dap_response_cache", __PRETTY_FUNCTION__ << " Caught exception, unlocking cache and re-throw." << endl);
        // I think this call is not needed. jhrg 10/23/12
        unlock_cache();
        throw;
    }

    cache_token = cache_file_name;  // Set this value-result parameter
    return fdds;
}

