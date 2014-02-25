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

#include <DDS.h>
#include <ConstraintEvaluator.h>
#include <DDXParserSAX2.h>
#include <XDRStreamMarshaller.h>
#include <XDRStreamUnMarshaller.h>
//<XDRFileUnMarshaller.h>
#include <debug.h>
#include <mime_util.h>	// for last_modified_time() and rfc_822_date()
#include <util.h>


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


BESStoredDapResultCache *BESStoredDapResultCache::d_instance = 0;
const string BESStoredDapResultCache::SUBDIR_KEY = "DAP.StoredResultsCache.subdir";
const string BESStoredDapResultCache::PREFIX_KEY = "DAP.StoredResultsCache.prefix";
const string BESStoredDapResultCache::SIZE_KEY   = "DAP.StoredResultsCache.size";

unsigned long BESStoredDapResultCache::getCacheSizeFromConfig(){

	bool found;
    string size;
    unsigned long size_in_megabytes = 0;
    TheBESKeys::TheKeys()->get_value( SIZE_KEY, size, found ) ;
    if( found ) {
    	istringstream iss(size);
    	iss >> size_in_megabytes;
    }
    else {
    	string msg = "[ERROR] BESStoreResultCache::getCacheSize() - The BES Key " + SIZE_KEY + " is not set! It MUST be set to utilize the Stored Result Caching system. ";
    	BESDEBUG("cache", msg);
        throw BESInternalError(msg , __FILE__, __LINE__);
    }
    return size_in_megabytes;
}

string BESStoredDapResultCache::getSubDirFromConfig(){
	bool found;
    string subdir = "";
    TheBESKeys::TheKeys()->get_value( SUBDIR_KEY, subdir, found ) ;

	if( !found ) {
    	string msg = "[ERROR] BESStoreResultCache::getDefaultSubDir() - The BES Key " + SUBDIR_KEY + " is not set! It MUST be set to utilize the Stored Result Caching system. ";
    	BESDEBUG("cache", msg);
        throw BESInternalError(msg , __FILE__, __LINE__);
	}
	else {
		while(*subdir.begin() == '/' && subdir.length()>0){
			subdir = subdir.substr(1);
		}
		// So if it's value is "/" or the empty string then the subdir will default to the root
		// directory of the BES data system.
	}

    return subdir;
}


string BESStoredDapResultCache::getResultPrefixFromConfig(){
	bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value( PREFIX_KEY, prefix, found ) ;
	if( found ) {
		prefix = BESUtil::lowercase( prefix ) ;
	}
	else {
    	string msg = "[ERROR] BESStoreResultCache::getResultPrefix() - The BES Key " + PREFIX_KEY + " is not set! It MUST be set to utilize the Stored Result Caching system. ";
    	BESDEBUG("cache", msg);
        throw BESInternalError(msg , __FILE__, __LINE__);
	}

    return prefix;
}


string BESStoredDapResultCache::getBesDataRootDirFromConfig(){
	bool found;
    string cacheDir = "";
    TheBESKeys::TheKeys()->get_value( BES_CATALOG_ROOT, cacheDir, found ) ;
    if( !found ) {
        TheBESKeys::TheKeys()->get_value( BES_DATA_ROOT, cacheDir, found ) ;
        if( !found ) {
        	string msg = ((string)"[ERROR] BESStoreResultCache::getStoredResultsDir() - Neither the BES Key ") + BES_CATALOG_ROOT +
        			"or the BES key " + BES_DATA_ROOT + " have been set! One MUST be set to utilize the Stored Result Caching system. ";
        	BESDEBUG("cache", msg);
            throw BESInternalError(msg , __FILE__, __LINE__);
        }
    }
    return cacheDir;

}
string BESStoredDapResultCache::assemblePath(const string &firstPart, const string &secondPart, bool addLeadingSlash){

	BESDEBUG("cache", "BESStoreResultCache::assemblePath() -  BEGIN" << endl);
	BESDEBUG("cache", "BESStoreResultCache::assemblePath() -  firstPart: "<< firstPart << endl);
	BESDEBUG("cache", "BESStoreResultCache::assemblePath() -  secondPart: "<< secondPart << endl);

	string firstPathFragment = firstPart;
	string secondPathFragment = secondPart;


	if(addLeadingSlash){
	    if(*firstPathFragment.begin() != '/')
	    	firstPathFragment = "/" + firstPathFragment;
	}

	// make sure there are not multiple slashes at the end of the first part...
	while(*firstPathFragment.rbegin() == '/' && firstPathFragment.length()>0){
		firstPathFragment = firstPathFragment.substr(0,firstPathFragment.length()-1);
		// BESDEBUG("cache", "BESStoreResultCache::assemblePath() -  firstPathFragment: "<< firstPathFragment << endl);
	}

	// make sure first part ends with a "/"
    if(*firstPathFragment.rbegin() != '/'){
    	firstPathFragment += "/";
    }
	BESDEBUG("cache", "BESStoreResultCache::assemblePath() -  firstPathFragment: "<< firstPathFragment << endl);

	// make sure second part does not BEGIN with a slash
	while(*secondPathFragment.begin() == '/' && secondPathFragment.length()>0){
		secondPathFragment = secondPathFragment.substr(1);
	}

	BESDEBUG("cache", "BESStoreResultCache::assemblePath() -  secondPathFragment: "<< secondPathFragment << endl);

	string newPath = firstPathFragment + secondPathFragment;

	BESDEBUG("cache", "BESStoreResultCache::assemblePath() -  newPath: "<< newPath << endl);
	BESDEBUG("cache", "BESStoreResultCache::assemblePath() -  END" << endl);

	return newPath;

}



BESStoredDapResultCache::BESStoredDapResultCache(){
	BESDEBUG("cache", "BESStoreResultCache::BESStoreResultCache() -  BEGIN" << endl);
#if 0
	bool found;
#endif

	d_storedResultsSubdir = getSubDirFromConfig();
	d_dataRootDir = getBesDataRootDirFromConfig();
    string resultsDir = assemblePath(d_dataRootDir,d_storedResultsSubdir);

    d_resultFilePrefix = getResultPrefixFromConfig();
    d_maxCacheSize = getCacheSizeFromConfig();

    BESDEBUG("cache", "BESStoreResultCache() - Stored results cache configuration params: " << resultsDir << ", " << d_resultFilePrefix << ", " << d_maxCacheSize << endl);

  	initialize(resultsDir, d_resultFilePrefix, d_maxCacheSize);

    BESDEBUG("cache", "BESStoreResultCache::BESStoreResultCache() -  END" << endl);
}


/**
 *
 */
BESStoredDapResultCache::BESStoredDapResultCache( const string &data_root_dir, const string &stored_results_subdir, const string &result_file_prefix, unsigned long long max_cache_size){

	d_storedResultsSubdir = stored_results_subdir;
	d_dataRootDir = data_root_dir;
	d_resultFilePrefix = result_file_prefix;
	d_maxCacheSize = max_cache_size;
	initialize(assemblePath(d_dataRootDir,stored_results_subdir), d_resultFilePrefix, d_maxCacheSize);

}


BESStoredDapResultCache *
BESStoredDapResultCache::get_instance(const string &data_root_dir, const string &stored_results_subdir, const string &result_file_prefix, unsigned long long max_cache_size)
{
    if (d_instance == 0){
    	if(dir_exists(data_root_dir)){
        	try {
                d_instance = new BESStoredDapResultCache(data_root_dir, stored_results_subdir, result_file_prefix, max_cache_size);
        	}
        	catch(BESInternalError &bie){
        	    BESDEBUG("cache", "BESStoreResultCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
        	}
    	}
    }
    return d_instance;
}

/** Get the default instance of the BESStoredDapResultCache object. This will read "TheBESKeys" looking for the values
 * of SUBDIR_KEY, PREFIX_KEY, an SIZE_KEY to initialize the cache.
 */
BESStoredDapResultCache *
BESStoredDapResultCache::get_instance()
{
    if (d_instance == 0) {
		try {
			d_instance = new BESStoredDapResultCache();
		}
		catch(BESInternalError &bie){
			BESDEBUG("cache", "BESStoreResultCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
		}
    }

    return d_instance;
}



void BESStoredDapResultCache::delete_instance() {
    BESDEBUG("cache","BESStoreResultCache::delete_instance() - Deleting singleton BESStoreResultCache instance." << endl);
    delete d_instance;
    d_instance = 0;
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

    if (entry_size == 0)
        return false;

    time_t dataset_time = entry_time;
    if (stat(dataset.c_str(), &buf) == 0) {
        dataset_time = buf.st_mtime;
    }

    // Trick: if the d_dataset is not a file, stat() returns error and
    // the times stay equal and the code uses the cache entry.

    // TODO Fix this so that the code can get a LMT from the correct
    // handler.
    if (dataset_time > entry_time)
        return false;

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
void BESStoredDapResultCache::read_dap2_data_from_cache(const string &cache_file_name, DDS *fdds)
{
	BESDEBUG("cache", "BESStoredDapResultCache::read_dap2_data_from_cache() - Opening cache file: " << cache_file_name << endl);
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
    BESDEBUG("cache", "BESStoredDapResultCache::read_dap2_data_from_cache() - MPM Boundary: " << boundary << endl);

    read_multipart_headers(data, "text/xml", dods_ddx);

    BESDEBUG("cache", "BESStoredDapResultCache::read_dap2_data_from_cache() - Read the multipart haeaders" << endl);

    // Parse the DDX, reading up to and including the next boundary.
    // Return the CID for the matching data part
    string data_cid;
    try {
    	ddx_parser.intern_stream(data, fdds, data_cid, boundary);
    	BESDEBUG("cache", "BESStoredDapResultCache::read_dap2_data_from_cache() - Dataset name: " << fdds->get_dataset_name() << endl);
    }
    catch(Error &e) {
    	BESDEBUG("cache", "BESStoredDapResultCache::read_dap2_data_from_cache() - DDX Parser Error: " << e.get_error_message() << endl);
    	throw;
    }

    // Munge the CID into something we can work with
    BESDEBUG("cache", "BESStoredDapResultCache::read_dap2_data_from_cache() - Data CID (before): " << data_cid << endl);
    data_cid = cid_to_header_value(data_cid);
    BESDEBUG("cache", "BESStoredDapResultCache::read_dap2_data_from_cache() - Data CID (after): " << data_cid << endl);

    // Read the data part's MPM part headers (boundary was read by
    // DDXParse::intern)
    read_multipart_headers(data, "application/octet-stream", dods_data_ddx, data_cid);

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
BESStoredDapResultCache::get_cached_data_ddx(const string &cache_file_name, BaseTypeFactory *factory, const string &filename)
{
    BESDEBUG("cache", "Reading cache for " << cache_file_name << endl);

    DDS *fdds = new DDS(factory);

    fdds->filename(filename) ;
    //fdds->set_dataset_name( "function_result_" + name_path(filename) ) ;

    read_dap2_data_from_cache(cache_file_name, fdds);

    BESDEBUG("cache", "DDS Filename: " << fdds->filename() << endl);
    BESDEBUG("cache", "DDS Dataset name: " << fdds->get_dataset_name() << endl);

    fdds->set_factory( 0 ) ;

    // mark everything as read. and send. That is, make sure that when a response
    // is retrieved from the cache, all of the variables are marked as to be sent
    DDS::Vars_iter i = fdds->var_begin();
    while(i != fdds->var_end()) {
        (*i)->set_read_p( true );
        (*i++)->set_send_p(true);
    }

    return fdds;
}




#if 0
/**
 *
 * @return The DDS that resulted from calling the server functions
 * in the original CE.
 */
string BESStoredDapResultCache::store_dap2_result(DDS &dds, const string &constraint, BESDapResponseBuilder *rb, ConstraintEvaluator *eval)
{
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - BEGIN" << endl );
    // These are used for the cached or newly created DDS object
    BaseTypeFactory factory;
    DDS *fdds;

    // Get the cache filename for this thing. Do not use the default
    // name mangling; instead use what build_cache_file_name() does.
    string local_id = get_stored_result_local_id(dds.filename(), constraint);
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - local_id: "<< local_id << endl );
    string cache_file_name = get_cache_file_name(local_id, /*mangle*/false);
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - cache_file_name: "<< cache_file_name << endl );
    int fd;
    try {
        // If the object in the cache is not valid, remove it. The read_lock will
        // then fail and the code will drop down to the create_and_lock() call.
        // is_valid() tests for a non-zero object and for d_dateset newer than
        // the cached object.
        if (!is_valid(cache_file_name, dds.filename()))
        	purge_file(cache_file_name);

        if (get_read_lock(cache_file_name, fd)) {
            BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - function ce (change)- cached hit: " << cache_file_name << endl);
            fdds = get_cached_data_ddx(cache_file_name, &factory, dds.filename());
        }
        else if (create_and_lock(cache_file_name, fd)) {
            // If here, the cache_file_name could not be locked for read access;
            // try to build it. First make an empty file and get an exclusive lock on it.
            BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - function ce - caching " << cache_file_name << ", constraint: " << constraint << endl);

            fdds = new DDS(dds);
            eval->parse_constraint(constraint, *fdds);

           if (eval->function_clauses()) {
            	DDS *temp_fdds = eval->eval_function_clauses(*fdds);
            	delete fdds;
            	fdds = temp_fdds;
            }

            ofstream data_stream(cache_file_name.c_str());
            if (!data_stream)
            	throw InternalErr(__FILE__, __LINE__, "Could not open '" + cache_file_name + "' to write cached response.");

            string start="dataddx_cache_start", boundary="dataddx_cache_boundary";

            // Use a ConstraintEvaluator that has not parsed a CE so the code can use
            // the send method(s)
            ConstraintEvaluator eval;

            // Setting the version to 3.2 causes send_data_ddx to write the MIME headers that
            // the cache expects.
            fdds->set_dap_version("3.2");

            // This is a bit of a hack, but it effectively uses ResponseBuilder to write the
            // cached object/response without calling the machinery in one of the send_*()
            // methods. Those methods assume they need to evaluate the BESDapResponseBuilder's
            // CE, which is not necessary and will alter the values of the send_p property
            // of the DDS's variables.
			set_mime_multipart(data_stream, boundary, start, dods_data_ddx, x_plain, last_modified_time(rb->get_dataset_name()));
			//data_stream << flush;
			rb->serialize_dap2_data_ddx(data_stream, *fdds, eval, boundary, start);
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
            if (cache_too_big(size))
            	update_and_purge(cache_file_name);
        }
        // get_read_lock() returns immediately if the file does not exist,
        // but blocks waiting to get a shared lock if the file does exist.
        else if (get_read_lock(cache_file_name, fd)) {
            BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - function ce - cached hit: " << cache_file_name << endl);
            fdds = get_cached_data_ddx(cache_file_name, &factory, dds.get_dataset_name());
        }
        else {
            throw InternalErr(__FILE__, __LINE__, "BESStoredDapResultCache::store_dap2_result() - Cache error during function invocation.");
        }


        /**
         * FIXME I think we need to delete this here, before we close and unlock the cache.
         */
    	delete fdds;

        /**
         * FIXME Is it cool to unlock and close the cache file at this point? I think so...
         */
        BESDEBUG("cache", "BESStoredDapResultCache::cache_dap2_dataset() - unlocking and closing cache file "<< cache_file_name  << endl );
		unlock_and_close(cache_file_name);

    }
    catch (...) {
        BESDEBUG("cache", "BESStoredDapResultCache::cache_dap2_dataset() - caught exception, unlocking cache and re-throw." << endl );
        // I think this call is not needed. jhrg 10/23/12
        unlock_cache();
        throw;
    }


    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - END (local_id=`"<< local_id << "'" << endl );
    return local_id;
}
#endif




/**
 *
 * @return The DDS that resulted from calling the server functions
 * in the original CE.
 */
string BESStoredDapResultCache::store_dap2_result(DDS &dds, const string &constraint, BESDapResponseBuilder *rb, ConstraintEvaluator *eval)
{
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - BEGIN" << endl );
    // These are used for the cached or newly created DDS object
    BaseTypeFactory factory;

    // Get the cache filename for this thing. Do not use the default
    // name mangling; instead use what build_cache_file_name() does.
    string local_id = get_stored_result_local_id(dds.filename(), constraint);
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - local_id: "<< local_id << endl );
    string cache_file_name = get_cache_file_name(local_id, /*mangle*/false);
    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - cache_file_name: "<< cache_file_name << endl );
    int fd;
    try {
        // If the object in the cache is not valid, remove it. The read_lock will
        // then fail and the code will drop down to the create_and_lock() call.
        // is_valid() tests for a non-zero object and for d_dateset newer than
        // the cached object.
        if (!is_valid(cache_file_name, dds.filename()))
        	purge_file(cache_file_name);

        if (get_read_lock(cache_file_name, fd)) {
            BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - Stored Result already exists. Not rewriting file: " << cache_file_name << endl);
        }
        else if (create_and_lock(cache_file_name, fd)) {
            // If here, the cache_file_name could not be locked for read access;
            // try to build it. First make an empty file and get an exclusive lock on it.
            BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - function ce - caching " << cache_file_name << ", constraint: " << constraint << endl);
            DDS *fdds;

            fdds = new DDS(dds);
            eval->parse_constraint(constraint, *fdds);

           if (eval->function_clauses()) {
            	DDS *temp_fdds = eval->eval_function_clauses(*fdds);
            	delete fdds;
            	fdds = temp_fdds;
            }

            ofstream data_stream(cache_file_name.c_str());
            if (!data_stream)
            	throw InternalErr(__FILE__, __LINE__, "Could not open '" + cache_file_name + "' to write cached response.");

            string start="dataddx_cache_start", boundary="dataddx_cache_boundary";

            // Use a ConstraintEvaluator that has not parsed a CE so the code can use
            // the send method(s)
            ConstraintEvaluator eval;

            // Setting the version to 3.2 causes send_data_ddx to write the MIME headers that
            // the cache expects.
            fdds->set_dap_version("3.2");

            // This is a bit of a hack, but it effectively uses ResponseBuilder to write the
            // cached object/response without calling the machinery in one of the send_*()
            // methods. Those methods assume they need to evaluate the BESDapResponseBuilder's
            // CE, which is not necessary and will alter the values of the send_p property
            // of the DDS's variables.
			set_mime_multipart(data_stream, boundary, start, dods_data_ddx, x_plain, last_modified_time(rb->get_dataset_name()));
			//data_stream << flush;
			rb->serialize_dap2_data_ddx(data_stream, *fdds, eval, boundary, start);
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
            if (cache_too_big(size))
            	update_and_purge(cache_file_name);
            /**
             * FIXME I think we need to delete this here, before we close and unlock the cache.
             */
        	delete fdds;
        }
        // get_read_lock() returns immediately if the file does not exist,
        // but blocks waiting to get a shared lock if the file does exist.
        else if (get_read_lock(cache_file_name, fd)) {
            BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - Stored Result already exists. Not rewriting file: " << cache_file_name << endl);
        }
        else {
            throw InternalErr(__FILE__, __LINE__, "BESStoredDapResultCache::store_dap2_result() - Cache error during function invocation.");
        }



        /**
         * FIXME Is it cool to unlock and close the cache file at this point? I think so...
         */
        BESDEBUG("cache", "BESStoredDapResultCache::cache_dap2_dataset() - unlocking and closing cache file "<< cache_file_name  << endl );
		unlock_and_close(cache_file_name);

    }
    catch (...) {
        BESDEBUG("cache", "BESStoredDapResultCache::cache_dap2_dataset() - caught exception, unlocking cache and re-throw." << endl );
        // I think this call is not needed. jhrg 10/23/12
        unlock_cache();
        throw;
    }


    BESDEBUG("cache", "BESStoredDapResultCache::store_dap2_result() - END (local_id=`"<< local_id << "')" << endl );
    return local_id;
}



/**
 * Use the dataset name and the function-part of the CE to build a name
 * that can be used to index the result of that CE on the dataset. This
 * name can be used both to store a result for later retrieval or to access
 * a previously-stored result.
 *
 */
string
BESStoredDapResultCache::get_stored_result_local_id(const string &dataset, const string &ce)
{
    BESDEBUG("cache", "get_stored_result_local_id() - BEGIN. dataset: " << dataset << ", ce: " << ce << endl);
    std::ostringstream ostr;
    HASH_OBJ<std::string> str_hash;
    string name = dataset + "#" + ce;
    ostr << str_hash(name);
    string hashed_name = ostr.str();
    BESDEBUG("cache", "get_stored_result_local_id() - hashed_name: " << hashed_name << endl);


    string local_id = d_resultFilePrefix + hashed_name;
    BESDEBUG("cache", "get_stored_result_local_id() - file: " << local_id << endl);

    local_id = assemblePath(d_storedResultsSubdir,local_id);

    BESDEBUG("cache", "get_stored_result_local_id() - END. local_id: " << local_id << endl);
    return local_id;
}


/** Build the name of file that will holds the uncompressed data from
 * 'src' in the cache.
 *
 * @note This method overrides the parent methid and DOES NOT support mangling the name.
 * The "mangle" parameter will be ignored.
 *
 * @param src The source name to cache
 * @param mangle if True, assume the name is a file pathname and mangle it.
 * If false, do not mangle the name (assume the caller has sent a suitable
 * string) but do turn the string into a pathname located in the cache directory
 * with the cache prefix. the 'mangle' param is true by default.
 */
string BESStoredDapResultCache::get_cache_file_name(const string &local_id, bool mangle)
{
	if(local_id.empty()){
        throw BESInternalError("BESStoredDapResultCache: The target cache file name must not be an empty string. Srsly.", __FILE__, __LINE__);
	}

    string cacheFile = assemblePath(d_dataRootDir,local_id);

    BESDEBUG("cache", "BESStoredDapResultCache::get_cache_file_name() - local_id:   '" << local_id   << "'" << endl);
    BESDEBUG("cache", "BESStoredDapResultCache::get_cache_file_name() - cacheDir: '" << cacheFile << "'" << endl);

    if(mangle){
        BESDEBUG("cache", "[WARNING] BESStoredDapResultCache::get_cache_file_name() - The parameter 'mangle' is ignored!" << endl);
    }


    return cacheFile;
}

