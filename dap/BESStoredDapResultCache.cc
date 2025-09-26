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
#ifdef HAVE_TR1_FUNCTIONAL
#include <tr1/functional>
#endif
#include <string>
#include <fstream>
#include <sstream>

#include <libdap/DDS.h>
#include <libdap/DMR.h>
#include <libdap/DapXmlNamespaces.h>
#include <libdap/ConstraintEvaluator.h>
#include <libdap/DDXParserSAX2.h>

// These are needed because D4ParserSax2.h does not properly declare
// the classes. I think. Check on that... jhrg 3/28/14
#include <libdap/D4EnumDefs.h>
#include <libdap/D4Dimensions.h>
#include <libdap/D4Group.h>

#include <libdap/D4ParserSax2.h>

// DAP2 Stored results are not supported by default. If we do start using this.
// It would be better to use the CacheMarshaller and CacheUnMarshaller code
// since that does not translate data into network byte order. Also, there
// may be a bug in the XDRStreamUnMarshaller code - in/with get_opaque() - that
// breaks Sequence::deserialize(). jhrg 5/25/16
#ifdef DAP2_STORED_RESULTS
#include <libdap/XDRStreamMarshaller.h>
#include <libdap/XDRStreamUnMarshaller.h>
#endif

#include <libdap/chunked_istream.h>
#include <libdap/D4StreamUnMarshaller.h>

#include <libdap/debug.h>
#include <libdap/mime_util.h>	// for last_modified_time() and rfc_822_date()
#include <libdap/util.h>

#include "BESStoredDapResultCache.h"
#include "BESDapResponseBuilder.h"
#include "BESInternalError.h"

#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESDebug.h"

#ifdef HAVE_TR1_FUNCTIONAL
#define HASH_OBJ std::tr1::hash
#else
#define HASH_OBJ std::hash
#endif

#define CRLF "\r\n"
#define BES_DATA_ROOT "BES.Data.RootDirectory"
#define BES_CATALOG_ROOT "BES.Catalog.catalog.RootDirectory"


using namespace std;
using namespace libdap;

std::once_flag BESStoredDapResultCache::d_initialize;

BESStoredDapResultCache *BESStoredDapResultCache::d_instance = 0;
bool BESStoredDapResultCache::d_enabled = true;

#if 0
const string BESStoredDapResultCache::SUBDIR_KEY = "DAP.StoredResultsCache.subdir";
const string BESStoredDapResultCache::PREFIX_KEY = "DAP.StoredResultsCache.prefix";
const string BESStoredDapResultCache::SIZE_KEY = "DAP.StoredResultsCache.size";
#endif

unsigned long BESStoredDapResultCache::getCacheSizeFromConfig()
{
    bool found;
    string size;
    unsigned long size_in_megabytes = 0;
    TheBESKeys::TheKeys()->get_value(DAP_STORED_RESULTS_CACHE_SIZE_KEY, size, found);
    if (found) {
        istringstream iss(size);
        iss >> size_in_megabytes;
    }
    else {
        stringstream msg;
        msg << "[ERROR] BESStoredDapResultCache::getCacheSize() - The BES Key " << DAP_STORED_RESULTS_CACHE_SIZE_KEY;
        msg << " is not set! It MUST be set to utilize the Stored Result Caching system. ";
        BESDEBUG("cache", msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    return size_in_megabytes;
}

string BESStoredDapResultCache::getSubDirFromConfig()
{
    bool found;
    string subdir = "";
    TheBESKeys::TheKeys()->get_value(DAP_STORED_RESULTS_CACHE_SUBDIR_KEY, subdir, found);

    if (!found) {
        stringstream msg;
        msg << "[ERROR] BESStoredDapResultCache::getSubDirFromConfig() - The BES Key " << DAP_STORED_RESULTS_CACHE_SUBDIR_KEY;
        msg << " is not set! It MUST be set to utilize the Stored Result Caching system. ";
        BESDEBUG("cache", msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    else {
        while (*subdir.begin() == '/' && !subdir.empty()) {
            subdir = subdir.substr(1);
        }
        // So if it's value is "/" or the empty string then the subdir will default to the root
        // directory of the BES data system.
    }

    return subdir;
}

string BESStoredDapResultCache::getResultPrefixFromConfig()
{
    bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value(DAP_STORED_RESULTS_CACHE_PREFIX_KEY, prefix, found);
    if (found) {
        prefix = BESUtil::lowercase(prefix);
    }
    else {
        stringstream msg;
        msg << "[ERROR] BESStoredDapResultCache::getResultPrefix() - The BES Key " << DAP_STORED_RESULTS_CACHE_PREFIX_KEY;
        msg << " is not set! It MUST be set to utilize the Stored Result Caching system. ";
        BESDEBUG("cache", msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    return prefix;
}

string BESStoredDapResultCache::getBesDataRootDirFromConfig()
{
    bool found;
    string cacheDir = "";
    TheBESKeys::TheKeys()->get_value( BES_CATALOG_ROOT, cacheDir, found);
    if (!found) {
        TheBESKeys::TheKeys()->get_value( BES_DATA_ROOT, cacheDir, found);
        if (!found) {
            string msg = ((string) "[ERROR] BESStoredDapResultCache::getStoredResultsDir() - Neither the BES Key ")
                + BES_CATALOG_ROOT + "or the BES key " + BES_DATA_ROOT
                + " have been set! One MUST be set to utilize the Stored Result Caching system. ";
            BESDEBUG("cache", msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }
    }
    return cacheDir;

}

#if 0
BESStoredDapResultCache::BESStoredDapResultCache()
{
    BESDEBUG("cache", "BESStoredDapResultCache::BESStoredDapResultCache() -  BEGIN" << endl);

    d_storedResultsSubdir = getSubDirFromConfig();
    d_dataRootDir = getBesDataRootDirFromConfig();
    string resultsDir = BESUtil::assemblePath(d_dataRootDir, d_storedResultsSubdir);

    d_resultFilePrefix = getResultPrefixFromConfig();
    d_maxCacheSize = getCacheSizeFromConfig();

    BESDEBUG("cache",
        "BESStoredDapResultCache() - Stored results cache configuration params: " << resultsDir << ", " << d_resultFilePrefix << ", " << d_maxCacheSize << endl);

    initialize(resultsDir, d_resultFilePrefix, d_maxCacheSize);

    BESDEBUG("cache", "BESStoredDapResultCache::BESStoredDapResultCache() -  END" << endl);
}
#endif

/** Get the default instance of the BESStoredDapResultCache object. This will read "TheBESKeys" looking for the values
 * of SUBDIR_KEY, PREFIX_KEY, an SIZE_KEY to initialize the cache.
 */
BESStoredDapResultCache *
BESStoredDapResultCache::get_instance()
{
    static BESStoredDapResultCache cache;
    std::call_once(d_initialize, [](){

        string tmp_resultsDir = BESUtil::assemblePath(getSubDirFromConfig(), getBesDataRootDirFromConfig());

        if(tmp_resultsDir.empty()){
            cache.disable();
        }
        else{
            cache.enable();
            cache.initialize(tmp_resultsDir, getResultPrefixFromConfig(), getCacheSizeFromConfig());
        }
    });
    if (cache.cache_enabled()){
        return &cache;
    }
    else{
        return nullptr;
    }
}

/**
 * Is the item named by cache_entry_name valid? This code tests that the
 * cache entry is non-zero in size (returns false if that is the case, although
 * that might not be correct) and that the dataset associated with this
 * ResponseBulder instance is at least as old as the cached entry.
 *
 * @param cache_file_name File name of the cached entry
 * @return True if the thing is valid, false otherwise.
 */
bool BESStoredDapResultCache::is_valid(const string &cache_file_name, const string &dataset)
{
    // If the cached response is zero bytes in size, it's not valid.
    // (hmmm...)

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

    // TODO Fix this so that the code can get a LMT from the correct
    // handler.
    if (dataset_time > entry_time) return false;

    return true;
}

#ifdef DAP2_STORED_RESULTS
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
bool BESStoredDapResultCache::read_dap2_data_from_cache(const string &cache_file_name, DDS *fdds)
{
    BESDEBUG("cache",
        "BESStoredDapResultCache::read_dap2_data_from_cache() - Opening cache file: " << cache_file_name << endl);

    int fd = 1;

    try {
        if (get_read_lock(cache_file_name, fd)) {

            ifstream data(cache_file_name.c_str());

            // Rip off the MIME headers from the response if they are present
            string mime = get_next_mime_header(data);
            while (!mime.empty()) {
                mime = get_next_mime_header(data);
            }

            // Parse the DDX; throw an exception on error.
            DDXParser ddx_parser(fdds->get_factory());

            // Read the MPM boundary and then read the subsequent headers
            string boundary = read_multipart_boundary(data);
            BESDEBUG("cache",
                "BESStoredDapResultCache::read_dap2_data_from_cache() - MPM Boundary: " << boundary << endl);

            read_multipart_headers(data, "text/xml", dods_ddx);

            BESDEBUG("cache",
                "BESStoredDapResultCache::read_dap2_data_from_cache() - Read the multipart haeaders" << endl);

            // Parse the DDX, reading up to and including the next boundary.
            // Return the CID for the matching data part
            string data_cid;
            try {
                ddx_parser.intern_stream(data, fdds, data_cid, boundary);
                BESDEBUG("cache",
                    "BESStoredDapResultCache::read_dap2_data_from_cache() - Dataset name: " << fdds->get_dataset_name() << endl);
            }
            catch (Error &e) {
                BESDEBUG("cache",
                    "BESStoredDapResultCache::read_dap2_data_from_cache() - DDX Parser Error: " << e.get_error_message() << endl);
                throw;
            }

            // Munge the CID into something we can work with
            BESDEBUG("cache",
                "BESStoredDapResultCache::read_dap2_data_from_cache() - Data CID (before): " << data_cid << endl);
            data_cid = cid_to_header_value(data_cid);
            BESDEBUG("cache",
                "BESStoredDapResultCache::read_dap2_data_from_cache() - Data CID (after): " << data_cid << endl);

            // Read the data part's MPM part headers (boundary was read by
            // DDXParse::intern)
            read_multipart_headers(data, "application/octet-stream", dods_data_ddx, data_cid);

            // Now read the data

            // XDRFileUnMarshaller um(data);
            XDRStreamUnMarshaller um(data);
            for (DDS::Vars_iter i = fdds->var_begin(); i != fdds->var_end(); i++) {
                (*i)->deserialize(um, fdds);
            }

            data.close();
            unlock_and_close(cache_file_name /* was fd */);
            return true;
        }
        else {
            BESDEBUG("cache", "BESStoredDapResultCache - The requested file does not exist. File: " + cache_file_name);

            return false;
        }
    }
    catch (...) {
        BESDEBUG("cache",
            "BESStoredDapResultCache::read_dap4_data_from_cache() - caught exception, unlocking cache and re-throw." << endl);
        // I think this call is not needed. jhrg 10/23/12
        if (fd != -1) unlock_and_close(cache_file_name /* was fd */);
        throw;
    }
}
#endif

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
bool BESStoredDapResultCache::read_dap4_data_from_cache(const string &cache_file_name, libdap::DMR *dmr)
{
    BESDEBUG("cache", "BESStoredDapResultCache::read_dap4_data_from_cache() - BEGIN" << endl);

    int fd = 1;

    try {
        if (get_read_lock(cache_file_name, fd)) {
            BESDEBUG("cache",
                "BESStoredDapResultCache::read_dap4_data_from_cache() - Opening cache file: " << cache_file_name << endl);
            fstream in(cache_file_name.c_str(), ios::in | ios::binary);

            // Gobble up the response's initial set of MIME headers. Normally
            // a client would extract information from these headers.
            // NOTE - I am dumping this call because it basically just
            // slurps up lines until it finds a blank line, regardless of what the
            // lines actually have in the. So basically if the stream DOESN't have
            // a mime header then this call will read (and ignore) the entire
            // XML encoding of the DMR. doh.
            // remove_mime_header(in);

            chunked_istream cis(in, CHUNK_SIZE);

            bool debug = BESDebug::IsSet("parser");

            // parse the DMR, stopping when the boundary is found.
            // force chunk read
            // get chunk size
            int chunk_size = cis.read_next_chunk();

            BESDEBUG("cache",
                "BESStoredDapResultCache::read_dap4_data_from_cache() - First chunk_size: " << chunk_size << endl);

            if (chunk_size == EOF) {
                throw InternalErr(__FILE__, __LINE__,
                    "BESStoredDapResultCache::read_dap4_data_from_cache() - Failed to read first chunk from file. Chunk size = EOF (aka "
                        + libdap::long_to_string(EOF) + ")");
            }

            // get chunk
            char chunk[chunk_size];
            cis.read(chunk, chunk_size);
            BESDEBUG("cache", "BESStoredDapResultCache::read_dap4_data_from_cache() - Read first chunk." << endl);

            // parse char * with given size
            D4ParserSax2 parser;
            // '-2' to discard the CRLF pair
            parser.intern(chunk, chunk_size - 2, dmr, debug);
            BESDEBUG("cache", "BESStoredDapResultCache::read_dap4_data_from_cache() - Parsed first chunk." << endl);

            D4StreamUnMarshaller um(cis, cis.twiddle_bytes());

            dmr->root()->deserialize(um, *dmr);
            BESDEBUG("cache", "BESStoredDapResultCache::read_dap4_data_from_cache() - Deserialized data." << endl);

            BESDEBUG("cache", "BESStoredDapResultCache::read_dap4_data_from_cache() - END" << endl);

            in.close();
            unlock_and_close(cache_file_name /* was fd */);

            return true;

        }
        else {
            BESDEBUG("cache", "BESStoredDapResultCache - The requested file does not exist. File: " + cache_file_name);

            return false;

        }
    }
    catch (...) {
        BESDEBUG("cache",
            "BESStoredDapResultCache::read_dap4_data_from_cache() - caught exception, unlocking cache and re-throw." << endl);
        // I think this call is not needed. jhrg 10/23/12
        if (fd != -1) unlock_and_close(cache_file_name /* was fd */);
        throw;
    }
}

#ifdef DAP2_STORED_RESULTS
/**
 * Read data from cache. Allocates a new DDS using the given factory.
 *
 */
DDS *
BESStoredDapResultCache::get_cached_dap2_data_ddx(const string &cache_file_name, BaseTypeFactory *factory,
    const string &filename)
{
    BESDEBUG("cache",
        "BESStoredDapResultCache::get_cached_dap2_data_ddx() - Reading cache for " << cache_file_name << endl);

    DDS *fdds = new DDS(factory);

    if (read_dap2_data_from_cache(cache_file_name, fdds)) {

        fdds->filename(filename);
        //fdds->set_dataset_name( "function_result_" + name_path(filename) ) ;

        BESDEBUG("cache", "DDS Filename: " << fdds->filename() << endl);
        BESDEBUG("cache", "DDS Dataset name: " << fdds->get_dataset_name() << endl);

        fdds->set_factory(0);

        // mark everything as read. and send. That is, make sure that when a response
        // is retrieved from the cache, all of the variables are marked as to be sent
        DDS::Vars_iter i = fdds->var_begin();
        while (i != fdds->var_end()) {
            (*i)->set_read_p(true);
            (*i++)->set_send_p(true);
        }

        return fdds;
    }
    else {
        delete fdds;
        return 0;
    }

}
#endif

/**
 * Read data from cache. Allocates a new DDS using the given factory. If the file does not exists this will return null (0).
 *
 */
DMR *
BESStoredDapResultCache::get_cached_dap4_data(const string &cache_file_name, libdap::D4BaseTypeFactory *factory,
    const string &filename)
{
    BESDEBUG("cache",
        "BESStoredDapResultCache::get_cached_dap4_data() - Reading cache for " << cache_file_name << endl);

    DMR *fdmr = new DMR(factory);

    BESDEBUG("cache", "BESStoredDapResultCache::get_cached_dap4_data() - DMR Filename: " << fdmr->filename() << endl);
    fdmr->set_filename(filename);

    if (read_dap4_data_from_cache(cache_file_name, fdmr)) {
        BESDEBUG("cache",
            "BESStoredDapResultCache::get_cached_dap4_data() - DMR Dataset name: " << fdmr->name() << endl);

        fdmr->set_factory(0);

        // mark everything as read. and send. That is, make sure that when a response
        // is retrieved from the cache, all of the variables are marked as to be sent
        fdmr->root()->set_send_p(true);
        fdmr->root()->set_read_p(true);

        return fdmr;
    }

    return 0;
}

#ifdef DAP2_STORED_RESULTS
/**
 *
 * @return The local ID (relative to the BES data root directory) of the stored dataset.
 */
string BESStoredDapResultCache::store_dap2_result(DDS &dds, const string &constraint, BESDapResponseBuilder *rb,
    ConstraintEvaluator *eval)
{
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - BEGIN" << endl);
    // These are used for the cached or newly created DDS object
    BaseTypeFactory factory;

    // Get the cache filename for this thing. Do not use the default
    // name mangling; instead use what build_cache_file_name() does.
    string local_id = get_stored_result_local_id(dds.filename(), constraint, DAP_3_2);
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - local_id: "<< local_id << endl);
    string cache_file_name = get_cache_file_name(local_id, /*mangle*/false);
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - cache_file_name: "<< cache_file_name << endl);
    int fd;
    try {
        // If the object in the cache is not valid, remove it. The read_lock will
        // then fail and the code will drop down to the create_and_lock() call.
        // is_valid() tests for a non-zero object and for d_dateset newer than
        // the cached object.
        if (!is_valid(cache_file_name, dds.filename())) purge_file(cache_file_name);

        if (get_read_lock(cache_file_name, fd)) {
            BESDEBUG("cache",
                "BESStoredDapResultCache::store_dap2_result() - Stored Result already exists. Not rewriting file: " << cache_file_name << endl);
        }
        else if (create_and_lock(cache_file_name, fd)) {
            // If here, the cache_file_name could not be locked for read access;
            // try to build it. First make an empty file and get an exclusive lock on it.
            BESDEBUG("cache",
                "BESStoredDapResultCache::store_dap2_result() - cache_file_name " << cache_file_name << ", constraint: " << constraint << endl);

#if 0    // I shut this off because we know that the constraint and functions have already been evaluated - ndp
            DDS *fdds;

            fdds = new DDS(dds);
            eval->parse_constraint(constraint, *fdds);

            if (eval->function_clauses()) {
                DDS *temp_fdds = eval->eval_function_clauses(*fdds);
                delete fdds;
                fdds = temp_fdds;
            }
#endif

            ofstream data_stream(cache_file_name.c_str());
            if (!data_stream)
                throw InternalErr(__FILE__, __LINE__,
                    "Could not open '" + cache_file_name + "' to write cached response.");

            string start = "dataddx_cache_start", boundary = "dataddx_cache_boundary";

            // Use a ConstraintEvaluator that has not parsed a CE so the code can use
            // the send method(s)
            ConstraintEvaluator eval;

            // Setting the version to 3.2 causes send_data_ddx to write the MIME headers that
            // the cache expects.
            dds.set_dap_version("3.2");

            // This is a bit of a hack, but it effectively uses ResponseBuilder to write the
            // cached object/response without calling the machinery in one of the send_*()
            // methods. Those methods assume they need to evaluate the BESDapResponseBuilder's
            // CE, which is not necessary and will alter the values of the send_p property
            // of the DDS's variables.
            set_mime_multipart(data_stream, boundary, start, dods_data_ddx, x_plain,
                last_modified_time(rb->get_dataset_name()));
            //data_stream << flush;
            rb->serialize_dap2_data_ddx(data_stream, (DDS**) &dds, eval, boundary, start);
            //data_stream << flush;

            data_stream << CRLF << "--" << boundary << "--" << CRLF;

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
            BESDEBUG("cache",
                "BESStoredDapResultCache::store_dap2_result() - Stored Result already exists. Not rewriting file: " << cache_file_name << endl);
        }
        else {
            throw InternalErr(__FILE__, __LINE__,
                "BESStoredDapResultCache::store_dap2_result() - Cache error during function invocation.");
        }

        BESDEBUG("cache",
            "BESStoredDapResultCache::store_dap2_result() - unlocking and closing cache file "<< cache_file_name << endl);
        unlock_and_close(cache_file_name);
    }
    catch (...) {
        BESDEBUG("cache",
            "BESStoredDapResultCache::store_dap2_result() - caught exception, unlocking cache and re-throw." << endl);
        // I think this call is not needed. jhrg 10/23/12
        unlock_cache();
        throw;
    }

    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - END (local_id=`"<< local_id << "')" << endl);
    return local_id;
}
#endif

/**
 * Use the dataset name and the function-part of the CE to build a name
 * that can be used to index the result of that CE on the dataset. This
 * name can be used both to store a result for later retrieval or to access
 * a previously-stored result.
 *
 */
string BESStoredDapResultCache::get_stored_result_local_id(const string &dataset, const string &ce,
    libdap::DAPVersion version)
{
    BESDEBUG("cache", "get_stored_result_local_id() - BEGIN. dataset: " << dataset << ", ce: " << ce << endl);
    std::ostringstream ostr;
    HASH_OBJ<std::string> str_hash;
    string name = dataset + "#" + ce;
    ostr << str_hash(name);
    string hashed_name = ostr.str();
    BESDEBUG("cache", "get_stored_result_local_id() - hashed_name: " << hashed_name << endl);

    string suffix = "";
    switch (version) {
#ifdef DAP2_STORED_RESULTS
    case DAP_2_0:
        suffix = ".dods";
        break;

    case DAP_3_2:
        suffix = ".data_ddx";
        break;
#endif
    case DAP_4_0:
        suffix = ".dap";
        break;

    default:
        throw BESInternalError("BESStoredDapResultCache::get_stored_result_local_id() - Unrecognized DAP version!!",
            __FILE__, __LINE__);
        break;
    }

    BESDEBUG("cache", "get_stored_result_local_id() - Data file suffix: " << suffix << endl);

    string local_id = d_resultFilePrefix + hashed_name + suffix;
    BESDEBUG("cache", "get_stored_result_local_id() - file: " << local_id << endl);

    local_id = BESUtil::assemblePath(d_storedResultsSubdir, local_id);

    BESDEBUG("cache", "get_stored_result_local_id() - END. local_id: " << local_id << endl);
    return local_id;
}

/**
 *
 * @return The local ID (relative to the BES data root directory) of the stored dataset.
 */
string BESStoredDapResultCache::store_dap4_result(DMR &dmr, const string &constraint, BESDapResponseBuilder *rb)
{
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap4_result() - BEGIN" << endl);
    // These are used for the cached or newly created DDS object
    BaseTypeFactory factory;

    // Get the cache filename for this thing. Do not use the default
    // name mangling; instead use what build_cache_file_name() does.
    string local_id = get_stored_result_local_id(dmr.filename(), constraint, DAP_4_0);
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap4_result() - local_id: "<< local_id << endl);
    string cache_file_name = get_cache_file_name(local_id, /*mangle*/false);
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap4_result() - cache_file_name: "<< cache_file_name << endl);
    int fd;
#if 0
    try {
#endif
        // If the object in the cache is not valid, remove it. The read_lock will
        // then fail and the code will drop down to the create_and_lock() call.
        // is_valid() tests for a non-zero object and for d_dateset newer than
        // the cached object.
        if (!is_valid(cache_file_name, dmr.filename())) {
            BESDEBUG("cache",
                "BESStoredDapResultCache::store_dap4_result() - File is not valid. Purging file from cache. filename: " << cache_file_name << endl);
            purge_file(cache_file_name);
        }

        if (get_read_lock(cache_file_name, fd)) {
            BESDEBUG("cache",
                "BESStoredDapResultCache::store_dap4_result() - Stored Result already exists. Not rewriting file: " << cache_file_name << endl);
        }
        else if (create_and_lock(cache_file_name, fd)) {
            // If here, the cache_file_name could not be locked for read access;
            // try to build it. First make an empty file and get an exclusive lock on it.
            BESDEBUG("cache",
                "BESStoredDapResultCache::store_dap4_result() - cache_file_name: " << cache_file_name << ", constraint: " << constraint << endl);

            ofstream data_stream(cache_file_name.c_str());
            if (!data_stream)
                throw InternalErr(__FILE__, __LINE__,
                    "Could not open '" + cache_file_name + "' to write cached response.");

            //data_stream << flush;
            rb->serialize_dap4_data(data_stream, dmr, false);
            //data_stream << flush;

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
            BESDEBUG("cache",
                "BESStoredDapResultCache::store_dap4_result() - Couldn't create and lock file, But I got a read lock. " "Result may have been created by another process. " "Not rewriting file: " << cache_file_name << endl);
        }
        else {
            throw InternalErr(__FILE__, __LINE__,
                "BESStoredDapResultCache::store_dap4_result() - Cache error during function invocation.");
        }

        BESDEBUG("cache",
            "BESStoredDapResultCache::store_dap4_result() - unlocking and closing cache file "<< cache_file_name << endl);
        unlock_and_close(cache_file_name);
#if 0
}
    catch (...) {
        BESDEBUG("cache",
            "BESStoredDapResultCache::store_dap4_result() - caught exception, unlocking cache and re-throw." << endl);
        // I think this call is not needed. jhrg 10/23/12
        unlock_cache();
        throw;
    }

#endif

    BESDEBUG("cache", "BESStoredDapResultCache::store_dap4_result() - END (local_id=`"<< local_id << "')" << endl);
    return local_id;
}
