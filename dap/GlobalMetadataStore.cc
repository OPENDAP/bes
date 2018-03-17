// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of HYrax, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <functional>

#include <DapObj.h>
#include <DDS.h>
#include <DMR.h>
#include <XMLWriter.h>
#include <D4BaseTypeFactory.h>

#include "PicoSHA2/picosha2.h"

#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESLog.h"
#include "BESDebug.h"

#include "BESInternalError.h"
#include "BESInternalFatalError.h"

#include "GlobalMetadataStore.h"

#define DEBUG_KEY "metadata_store"
#define MAINTAIN_STORE_SIZE_EVEN_WHEN_UNLIMITED 0

#ifdef HAVE_ATEXIT
#define AT_EXIT(x) atexit((x))
#else
#define AT_EXIT(x)
#endif

using namespace std;
using namespace libdap;
using namespace bes;

static const unsigned int default_cache_size = 20; // 20 GB
static const string default_cache_prefix = "mds";
static const string default_cache_dir = ""; // I'm making the default empty so that no key == no caching. jhrg 9.26.16
static const string default_ledger_name = "mds_ledger.txt";   ///< In the CWD of the BES process

static const string PATH_KEY = "DAP.GlobalMetadataStore.path";
static const string PREFIX_KEY = "DAP.GlobalMetadataStore.prefix";
static const string SIZE_KEY = "DAP.GlobalMetadataStore.size";
static const string LEDGER_KEY = "DAP.GlobalMetadataStore.ledger";
static const string LOCAL_TIME_KEY = "BES.LogTimeLocal";

GlobalMetadataStore *GlobalMetadataStore::d_instance = 0;
bool GlobalMetadataStore::d_enabled = true;

/**
 * Hacked from GNU wc (in coreutils). This was found to be
 * faster than a memory mapped file read.
 *
 * https://stackoverflow.com/questions/17925051/fast-textfile-reading-in-c
 *
 * @param fd Open file descriptor to read from; assumed open and
 * positioned at the start of the file.
 * @param os C++ stream to write to
 * @exception BESInternalError Thrown if there's a problem reading or writing.
 */
static void transfer_bytes(int fd, ostream &os)
{
    static const int BUFFER_SIZE = 16*1024;

#if _POSIX_C_SOURCE >= 200112L
    /* Advise the kernel of our access pattern.  */
    posix_fadvise(fd, 0, 0, 1);  // FDADVICE_SEQUENTIAL
#endif

    char buf[BUFFER_SIZE + 1];

    while(size_t bytes_read = read(fd, buf, BUFFER_SIZE))
    {
        if(bytes_read == (size_t)-1)
            throw BESInternalError("Could not read dds from the metadata store.", __FILE__, __LINE__);
        if (!bytes_read)
            break;

        os.write(buf, bytes_read);
    }
}

unsigned long GlobalMetadataStore::get_cache_size_from_config()
{
    bool found;
    string size;
    unsigned long size_in_megabytes = default_cache_size;
    TheBESKeys::TheKeys()->get_value(SIZE_KEY, size, found);
    if (found) {
        BESDEBUG(DEBUG_KEY,
            "GlobalMetadataStore::getCacheSizeFromConfig(): Located BES key " << SIZE_KEY << "=" << size << endl);
        istringstream iss(size);
        iss >> size_in_megabytes;
    }

    return size_in_megabytes;
}

string GlobalMetadataStore::get_cache_prefix_from_config()
{
    bool found;
    string prefix = default_cache_prefix;
    TheBESKeys::TheKeys()->get_value(PREFIX_KEY, prefix, found);
    if (found) {
        BESDEBUG(DEBUG_KEY,
            "GlobalMetadataStore::getCachePrefixFromConfig(): Located BES key " << PREFIX_KEY << "=" << prefix << endl);
        prefix = BESUtil::lowercase(prefix);
    }

    return prefix;
}

// If the cache prefix is the empty string, the cache is turned off.
string GlobalMetadataStore::get_cache_dir_from_config()
{
    bool found;

    string cacheDir = default_cache_dir;
    TheBESKeys::TheKeys()->get_value(PATH_KEY, cacheDir, found);
    if (found) {
        BESDEBUG(DEBUG_KEY,
            "GlobalMetadataStore::getCacheDirFromConfig(): Located BES key " << PATH_KEY<< "=" << cacheDir << endl);
    }

    return cacheDir;
}

/**
 * @name Get an instance of GlobalMetadataStore
 * @brief  There are two ways to get an instance of GlobalMetadataStore singleton.
 *
 * @return A pointer to a GlobalMetadataStore object; null if the cache is disabled.
 */
///@{
/**
 * @brief Get an instance of the GlobalMetadataStore object.
 *
 * This class is a singleton, so the first call to any of two 'get_instance()' methods
 * makes an instance and subsequent calls return a pointer to that instance.
 *
 * @note If the cache_dir parameter is the empty string, get_instance() will return null
 * for the pointer to the singleton and caching is disabled. This means that if the cache
 * directory is not set in the bes.conf file(s), then the cache will be disabled. If
 * the cache directory is given (or set in bes.conf) but the prefix or size is not,
 * that's an error. Similarly, if the directory is named but does not exist, it will
 * be made. If the BES cannot make it, then an error will be signaled.
 *
 * @param cache_dir_key Key to use to get the value of the cache directory. If this is
 * the empty string, return null right away.
 * @param prefix_key Key for the item/file prefix. Each item added to the cache uses this
 * as a prefix so cached items can be easily identified when the same directory is used for
 * several caches or /tmp is used for the cache.
 * @param size_key The maximum size of the data stored in the cache, in megabytes
 *
 * @return A pointer to a GlobalMetadataStore object. If the cache is disabled (because the
 * directory is not set or does not exist), then the pointer returned will be null and
 * the cache will be marked as not enabled. Subsequent calls will return immediately.
 */
GlobalMetadataStore *
GlobalMetadataStore::get_instance(const string &cache_dir, const string &prefix, unsigned long long size)
{
    if (d_enabled && d_instance == 0) {
        d_instance = new GlobalMetadataStore(cache_dir, prefix, size); // never returns null_ptr
        d_enabled = d_instance->cache_enabled();
        if (!d_enabled) {
            delete d_instance;
            d_instance = 0;

            BESDEBUG(DEBUG_KEY, "GlobalMetadataStore::"<<__func__ << "() - " << "Cache is DISABLED"<< endl);
        }
        else {
            AT_EXIT(delete_instance);

            BESDEBUG(DEBUG_KEY, "GlobalMetadataStore::"<<__func__ << "() - " << "Cache is ENABLED"<< endl);
        }
    }

    BESDEBUG(DEBUG_KEY, "GlobalMetadataStore::get_instance(dir,prefix,size) - d_instance: " << d_instance << endl);

    return d_instance;
}

/**
 * Get an instance of the GlobalMetadataStore using the default values for the cache directory,
 * prefix and size.
 *
 * @return A pointer to a GlobalMetadataStore object; null if the cache is disabled.
 */
GlobalMetadataStore *
GlobalMetadataStore::get_instance()
{
    if (d_enabled && d_instance == 0) {
        d_instance = new GlobalMetadataStore(get_cache_dir_from_config(), get_cache_prefix_from_config(),
            get_cache_size_from_config());
        d_enabled = d_instance->cache_enabled();
        if (!d_enabled) {
            delete d_instance;
            d_instance = NULL;
            BESDEBUG(DEBUG_KEY, "GlobalMetadataStore::"<<__func__ << "() - " << "Cache is DISABLED"<< endl);
        }
        else {
            AT_EXIT(delete_instance);

            BESDEBUG(DEBUG_KEY, "GlobalMetadataStore::"<<__func__ << "() - " << "Cache is ENABLED"<< endl);
        }
    }

    BESDEBUG(DEBUG_KEY, "GlobalMetadataStore::get_instance() - d_instance: " << (void *) d_instance << endl);

    return d_instance;
}
///@}

/**
 * Private constructor that calls BESFileLockingCache's constructor;
 * lookup cache directory, item prefix and max cache size in BESKeys
 *
 * @note Use the get_instance() methods to get a pointer to the singleton for
 * this class. Do not use this method except in derived classes. This method
 * either builds a valid object or throws an exception.
 *
 * @param cache_dir key to find cache dir
 * @param prefix key to find the cache prefix
 * @param size key to find the cache size (in MBytes)
 * @throws BESSyntaxUserError if the keys are not set in the BESKeys, if the key
 * are either the empty string or zero, respectively, or if the cache directory does not exist.
 */
GlobalMetadataStore::GlobalMetadataStore(const string &cache_dir, const string &prefix,
    unsigned long long size) : BESFileLockingCache(cache_dir, prefix, size)
{
    bool found;

    TheBESKeys::TheKeys()->get_value(LEDGER_KEY, d_ledger_name, found);
    if (found) {
        BESDEBUG(DEBUG_KEY, "Located BES key " << LEDGER_KEY << "=" << d_ledger_name << endl);
    }
    else {
        d_ledger_name = default_ledger_name;
    }

    // By default, use UTC in the logs.
    string local_time = "no";
    TheBESKeys::TheKeys()->get_value(LOCAL_TIME_KEY, local_time, found);
    d_use_local_time = (local_time == "YES" || local_time == "Yes" || local_time == "yes");
}

/**
 * Copied from BESLog, where that code writes to an internal object, not a stream.
 * @param os
 */
static void dump_time(ostream &os, bool use_local_time)
{
    time_t now;
    time(&now);
    char buf[sizeof "YYYY-MM-DDTHH:MM:SSzone"];
    int status = 0;

    // From StackOverflow:
    // This will work too, if your compiler doesn't support %F or %T:
    // strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S%Z", gmtime(&now));
    //
    // Apologies for the twisted logic - UTC is the default. Override to
    // local time using BES.LogTimeLocal=yes in bes.conf. jhrg 11/15/17
    if (!use_local_time)
        status = strftime(buf, sizeof buf, "%FT%T%Z", gmtime(&now));
    else
        status = strftime(buf, sizeof buf, "%FT%T%Z", localtime(&now));

    if (!status)
        LOG("Error getting time for Metadata Store ledger.");

    os << buf;
}

/**
 * Write the current text of d_ledger_entry to the metadata store ledger
 */
void
GlobalMetadataStore::write_ledger()
{
    // TODO open just once
    ofstream of(d_ledger_name.c_str(), ios::app);
    if (of) {
        dump_time(of, d_use_local_time);
        of << " " << d_ledger_entry << endl;
        VERBOSE("MD Ledger name: '" << d_ledger_name << "', entry: '" << d_ledger_entry + "'.");
    }
    else {
        LOG("Warning: Metadata store could not write to is ledger file.");
    }
}

/**
 * Compute the SHA256 hash for the item name
 *
 * @param name The name to hash
 * @return The SHA256 hash of the name.
 */
inline string
GlobalMetadataStore::get_hash(const string &name)
{
    return picosha2::hash256_hex_string(name);
}

// TODO Document
void GlobalMetadataStore::StreamDDS::operator()(ostream &os) {
    if (d_dds)
        d_dds->print(os);
    else if (d_dmr)
        d_dmr->getDDS()->print(os);
    else
        throw BESInternalFatalError("Unknown DAP object type.", __FILE__, __LINE__);
}

// TODO Document
void GlobalMetadataStore::StreamDAS::operator()(ostream &os) {
    if (d_dds)
        d_dds->print_das(os);
    else if (d_dmr)
        d_dmr->getDDS()->print_das(os);
    else
        throw BESInternalFatalError("Unknown DAP object type.", __FILE__, __LINE__);
}

/**
 * Specialization of StreamDAP that prints a DMR using the information
 * in a DDS instance.
 *
 * Look at the GlobalMetadataStore class definition to see how the StreamDAP
 * functor is used to parameterize writing the DAP metadata response for the
 * store_dap_response() method.
 *
 * @note Most of the three three child classes are defined in the GlobalMetadataStore
 * header; only this one method is defined in the implementation file to keep
 * the libdap headers out of GlobalMetadataStore.h
 *
 * @param os Write the DMR to this stream
 * @see StreamDAP
 * @see StreamDDS
 * @see StreamDAS
 */
void GlobalMetadataStore::StreamDMR::operator()(ostream &os)
{
    if (d_dds) {
        D4BaseTypeFactory factory;
        DMR dmr(&factory, *d_dds);
        XMLWriter xml;
        dmr.print_dap4(xml);
        os << xml.get_doc();
    }
    else if (d_dmr) {
        XMLWriter xml;
        d_dmr->print_dap4(xml);
        os << xml.get_doc();
    }
    else {
        throw BESInternalFatalError("Unknown DAP object type.", __FILE__, __LINE__);
    }
}

/**
 * Store the DAP metadata responses
 *
 * @param writer A child instance of StreamDAP, instantiated using a DDS.
 * An instance of StreamDDS will write a DDS response, StreamDAS a DAS
 * response and StreamDMR a DMR response.
 * @param key Unique Id for this response; used to store the response in the
 * MDS.
 * @param name The granule/file name or pathname
 * @param response_name The name of the particular response (DDS, DAS, DMR).
 * Used for log messages.
 * @return True if the operation succeeded, False if the key is in use.
 * @throw BESInternalError If ...
 */
bool
GlobalMetadataStore::store_dap_response(StreamDAP &writer, const string &key, const string &name,
    const string &response_name)
{
    BESDEBUG(DEBUG_KEY, __FUNCTION__ << " BEGIN " << key << endl);

    string item_name = get_cache_file_name(key, false /*mangle*/);

    int fd;
    if (create_and_lock(item_name, fd)) {
        // If here, the cache_file_name could not be locked for read access;
        // try to build it. First make an empty files and get an exclusive lock on them.
        BESDEBUG(DEBUG_KEY,__FUNCTION__ << " Storing " << item_name << endl);

        // Get an output stream directed at the locked cache file
        ofstream response(item_name.c_str(), ios::out|ios::app);
        if (!response.is_open())
            throw BESInternalError("Could not open '" + key + "' to write the response.", __FILE__, __LINE__);

        try {
            // for the different writers, look at the StreamDAP struct in the class
            // definition. jhrg 2.27.18
            writer(response);   // different writers can write the DDS, DAS or DMR

            // Compute/update/maintain the cache size? This is extra work
            // that might never be used. It also locks the cache...
            if (!is_unlimited() || MAINTAIN_STORE_SIZE_EVEN_WHEN_UNLIMITED) {
                // This enables the call to update_cache_info() below.
                exclusive_to_shared_lock(fd);

                unsigned long long size = update_cache_info(item_name);
                if (!is_unlimited() && cache_too_big(size)) update_and_purge(item_name);
            }

            unlock_and_close(item_name);
        }
        catch (...) {
            // Bummer. There was a problem doing The Stuff. Now we gotta clean up.
            response.close();
            this->purge_file(item_name);
            unlock_and_close(item_name);
            throw;
        }

        VERBOSE("Metadata store: Wrote " << response_name << " response for '" << name << "'." << endl);
        d_ledger_entry.append(" ").append(key);

        return true;
    }
    else if (get_read_lock(item_name, fd)) {
        // We found the key; didn't add this because it was already here
        BESDEBUG(DEBUG_KEY,__FUNCTION__ << " Found " << item_name << " in the store already." << endl);
        unlock_and_close(item_name);

        LOG("Metadata store: unable to store the " << response_name << " response for '" << name << "'." << endl);

        return false;
    }
    else {
        throw BESInternalError("Could neither create or open '" + item_name + "'  in the metadata store.", __FILE__, __LINE__);
    }
}

// Documented in the header file - I could not get doxygen comments to work
// for these two methods in ths file (but al the others are fine). jhrg 2.28.18
bool
GlobalMetadataStore::add_responses(DDS *dds, const string &name)
{
    // Start the index entry
    d_ledger_entry = string("add DDS ").append(name);

    // I'm appending the 'dds r' string to the name before hashing so that
    // the different hashes for the file's DDS, DAS, ..., are all very different.
    // This will be useful if we use S3 instead of EFS for the Metadata Store.
    //
    // The helper() also updates the ledger string.
    StreamDDS write_the_dds_response(dds);
    bool stored_dds = store_dap_response(write_the_dds_response, get_hash(name + "dds_r"), name, "DDS");

    StreamDAS write_the_das_response(dds);
    bool stored_das = store_dap_response(write_the_das_response, get_hash(name + "das_r"), name, "DAS");

    StreamDMR write_the_dmr_response(dds);
    bool stored_dmr = store_dap_response(write_the_dmr_response, get_hash(name + "dmr_r"), name, "DMR");

    write_ledger(); // write the index line

    return (stored_dds && stored_das && stored_dmr);
}

bool
GlobalMetadataStore::add_responses(DMR *dmr, const string &name)
{
    // Start the index entry
    d_ledger_entry = string("add DMR ").append(name);

    // I'm appending the 'dds r' string to the name before hashing so that
    // the different hashes for the file's DDS, DAS, ..., are all very different.
    // This will be useful if we use S3 instead of EFS for the Metadata Store.
    //
    // The helper() also updates the ledger string.
    StreamDDS write_the_dds_response(dmr);
    bool stored_dds = store_dap_response(write_the_dds_response, get_hash(name + "dds_r"), name, "DDS");

    StreamDAS write_the_das_response(dmr);
    bool stored_das = store_dap_response(write_the_das_response, get_hash(name + "das_r"), name, "DAS");

    StreamDMR write_the_dmr_response(dmr);
    bool stored_dmr = store_dap_response(write_the_dmr_response, get_hash(name + "dmr_r"), name, "DMR");

    write_ledger(); // write the index line

    return (stored_dds && stored_das && stored_dmr);

    return false;
}

/**
 * Common code to acquire a read lock on a MDS item. The caller must use unlock_and_close()
 *
 * @param name Granule name
 * @param suffix One of 'dds_r', 'das_r' or 'dmr_r'
 * @param object_name One of DDS, DAS or DMR (used for verbose logging only)
 * @return True if the object was locked, false otherwise
 */
GlobalMetadataStore::MDSReadLock
GlobalMetadataStore::get_read_lock_helper(const string &name, const string &suffix, const string &object_name)
{
    string item_name = get_cache_file_name(get_hash(name + suffix), false);
    int fd;
    MDSReadLock lock(item_name, get_read_lock(item_name, fd));
    if (lock()) {
        VERBOSE("Metadata store: Lock " << object_name << " response for '" << name << "'." << endl);
        BESDEBUG(DEBUG_KEY, __FUNCTION__ << " Locked " << item_name << " in the store." << endl);
    }

    return lock;
 }

GlobalMetadataStore::MDSReadLock
GlobalMetadataStore::is_dmr_available(const string &name)
{
    return get_read_lock_helper(name, "dmr_r", "DMR");
}


/**
 * Common code to copy a response to an output stream.
 *
 * @param name Granule name
 * @param os Write the response to this stream
 * @param suffix One of 'dds_r', 'das_r' or 'dmr_r'
 * @param object_name One of DDS, DAS or DMR
 */
void
GlobalMetadataStore::get_response_helper(const string &name, ostream &os, const string &suffix, const string &object_name)
{
    string item_name = get_cache_file_name(get_hash(name + suffix), false);
    int fd; // value-result parameter;
    if (get_read_lock(item_name, fd)) {
        VERBOSE("Metadata store: Read " << object_name << " response for '" << name << "'." << endl);
        BESDEBUG(DEBUG_KEY, __FUNCTION__ << " Found " << item_name << " in the store." << endl);
        transfer_bytes(fd, os);
        unlock_and_close(item_name); // closes fd
    }
    else {
        throw BESInternalError("Could not open '" + item_name + "'  in the metadata store.", __FILE__, __LINE__);
    }
}

/**
 * @brief Write the stored DDS response to a stream
 *
 * @param name The (path)name of the granule
 * @param os Write to this stream
 */
void
GlobalMetadataStore::get_dds_response(const std::string &name, ostream &os)
{
    get_response_helper(name, os, "dds_r", "DDS");
}

/**
 * @brief Write the stored DAS response to a stream
 *
 * @param name The (path)name of the granule
 * @param os Write to this stream
 */
void
GlobalMetadataStore::get_das_response(const std::string &name, ostream &os)
{
    get_response_helper(name, os, "das_r", "DAS");
}

/**
 * @brief Write the stored DMR response to a stream
 *
 * @param name The (path)name of the granule
 * @param os Write to this stream
 */
void
GlobalMetadataStore::get_dmr_response(const std::string &name, ostream &os)
{
    get_response_helper(name, os, "dmr_r", "DMR");
}

/**
 * Common code to remove a stored response.
 *
 * @param name Granule name
 * @param suffix One of 'dds_r', 'das_r' or 'dmr_r'
 * @param object_name One of DDS, DAS or DMR
 */
bool
GlobalMetadataStore::remove_response_helper(const string& name, const string &suffix, const string &object_name)
{
    string hash = get_hash(name + suffix);
    if (unlink(get_cache_file_name(hash, false).c_str()) == 0) {
        VERBOSE("Metadata store: Removed " << object_name << " response for '" << hash << "'." << endl);
        d_ledger_entry.append(" ").append(hash);
        return true;
    }
    else {
        LOG("Metadata store: unable to remove the " << object_name << " response for '" << name << "' (" << strerror(errno) << ")."<< endl);
    }

    return false;
}

/**
 * @brief Remove all cached responses and objects for a granule
 *
 * @param name
 * @return
 */
bool
GlobalMetadataStore::remove_responses(const string &name)
{
    // Start the index entry
     d_ledger_entry = string("remove ").append(name);

     bool removed_dds = remove_response_helper(name, "dds_r", "DDS");

     bool removed_das = remove_response_helper(name, "das_r", "DAS");

     bool removed_dmr = remove_response_helper(name, "dmr_r", "DMR");

     write_ledger(); // write the index line

     return  (removed_dds && removed_das && removed_dmr);
}

