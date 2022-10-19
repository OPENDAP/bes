// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
#include <time.h>

#include <memory>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <sstream>
#include <iostream>
#include <fstream>
#include <GetOpt.h>

#include <curl/curl.h>


#include <libdap/D4Dimensions.h>
#include <libdap/D4StreamMarshaller.h>

#include "BESInternalError.h"
#include "BESUtil.h"
#include "CurlUtils.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "BESDebug.h"
#include "BESStopWatch.h"

#include "awsv4.h"
#include "HttpNames.h"
#include "url_impl.h"
#include "EffectiveUrl.h"
#include "EffectiveUrlCache.h"
#include "RemoteResource.h"

#include "Chunk.h"
#include "CredentialsManager.h"
#include "AccessCredentials.h"
#include "CredentialsManager.h"
#include "CurlHandlePool.h"
#include "DmrppCommon.h"
#include "DmrppRequestHandler.h"
#include "DmrppByte.h"
#include "DmrppArray.h"
#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"
#include "DmrppParserSax2.h"

//#include <memory>
//#include <iterator>
//#include <algorithm>


bool Debug = false;
bool debug = false;
bool bes_debug = false;

using std::cerr;
using std::endl;
using std::string;

#define prolog std::string("retriever::").append(__func__).append("() - ")

#define NULL_BODY_HASH "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"


/**
 *
 * @return
 */
string get_errno() {
    char *s_err = strerror(errno);
    if (s_err)
        return s_err;
    else
        return "Unknown error.";
}


/**
 *
 * @param bes_config_file
 * @param bes_log_file
 * @param bes_debug_log_file
 * @param bes_debug_keys
 * @param http_netrc_file
 * @return
 */
dmrpp::DmrppRequestHandler *bes_setup(
        const string &bes_config_file,
        const string &bes_log_file,
        const string &bes_debug_log_file,
        const string &bes_debug_keys,
        const string &http_netrc_file,
        const string &http_cache_dir
) {
    if (debug) cerr << prolog << "BEGIN" << endl;

    TheBESKeys::ConfigFile = bes_config_file; // Set the config file for TheBESKeys
    TheBESKeys::TheKeys()->set_key("BES.LogName", bes_log_file); // Set the log file so it goes where we say.
    TheBESKeys::TheKeys()->set_key("AllowedHosts", "^https?:\\/\\/.*$", false); // Set AllowedHosts to allow any URL
    TheBESKeys::TheKeys()->set_key("AllowedHosts", "^file:\\/\\/\\/.*$", true); // Set AllowedHosts to allow any file

    if (bes_debug) BESDebug::SetUp(bes_debug_log_file + "," + bes_debug_keys); // Enable BESDebug settings


    if (!http_netrc_file.empty()) {
        TheBESKeys::TheKeys()->set_key(HTTP_NETRC_FILE_KEY, http_netrc_file, false); // Set the netrc file
    }

    if (!http_cache_dir.empty()) {
        TheBESKeys::TheKeys()->set_key(HTTP_CACHE_DIR_KEY, http_cache_dir, false); // Set the netrc file
    }

    // Initialize the dmr++ goodness.
    auto foo = new dmrpp::DmrppRequestHandler("Chaos");

    if (debug) cerr << prolog << "END" << endl;
    return foo;
}

curl_slist *aws_sign_request_url(shared_ptr<http::url> &target_url, curl_slist *request_headers) {

    if (debug) cerr << prolog << "BEGIN" << endl;

    AccessCredentials *credentials = CredentialsManager::theCM()->get(target_url);
    if (credentials && credentials->is_s3_cred()) {
        if (debug)
            cerr << prolog << "Got AWS S3 AccessCredentials instance: " << endl << credentials->to_json() << endl;
        // If there are available credentials, and they are S3 credentials then we need to sign
        // the request
        const std::time_t request_time = std::time(0);

        const std::string auth_header =
                AWSV4::compute_awsv4_signature(
                        target_url,
                        request_time,
                        credentials->get(AccessCredentials::ID_KEY),
                        credentials->get(AccessCredentials::KEY_KEY),
                        credentials->get(AccessCredentials::REGION_KEY),
                        "s3");

        // passing nullptr for the first call allocates the curl_slist
        // The following code builds the slist that holds the headers. This slist is freed
        // once the URL is dereferenced in dmrpp_easy_handle::read_data(). jhrg 11/26/19
        request_headers = curl::append_http_header(request_headers, "Authorization", auth_header);

        // We pre-compute the sha256 hash of a null message body
        request_headers = curl::append_http_header(request_headers, "x-amz-content-sha256", NULL_BODY_HASH);
        request_headers = curl::append_http_header(request_headers, "x-amz-date", AWSV4::ISO8601_date(request_time));
    }
    if (debug) cerr << prolog << "END" << endl;
    return request_headers;
}

/**
 *
 * @param url
 * @return
 */
size_t get_remote_size(shared_ptr<http::url> &target_url, bool aws_signing) {
    if (debug) cerr << prolog << "BEGIN" << endl;

    char error_buffer[CURL_ERROR_SIZE];
    std::vector<std::string> resp_hdrs;
    curl_slist *request_headers = nullptr;

    request_headers = curl::add_edl_auth_headers(request_headers);

    if (aws_signing)
        request_headers = aws_sign_request_url(target_url, request_headers);

    CURL *ceh = curl::init(target_url->str(), request_headers, &resp_hdrs);
    curl::set_error_buffer(ceh, error_buffer);

    // In cURLville, CURLOPT_NOBODY means a HEAD request i.e. Don't send the response body a.k.a. "NoBody"
    CURLcode curl_status = curl_easy_setopt(ceh, CURLOPT_NOBODY, 1L);
    curl::eval_curl_easy_setopt_result(curl_status, prolog, "CURLOPT_NOBODY", error_buffer, __FILE__, __LINE__);

    if (Debug) cerr << prolog << "cURL HEAD request is configured" << endl;

    curl::super_easy_perform(ceh);

    curl::unset_error_buffer(ceh);
    if (request_headers)
        curl_slist_free_all(request_headers);
    if (ceh)
        curl_easy_cleanup(ceh);

    bool done = false;
    size_t how_big_it_is = 0;
    string content_length_hdr_key("content-length: ");
    for (size_t i = 0; !done && i < resp_hdrs.size(); i++) {
        if (Debug) cerr << prolog << "HEADER[" << i << "]: " << resp_hdrs[i] << endl;
        string lc_header = BESUtil::lowercase(resp_hdrs[i]);
        size_t index = lc_header.find(content_length_hdr_key);
        if (index == 0) {
            string value = lc_header.substr(content_length_hdr_key.size());
            how_big_it_is = stol(value);
            done = true;
        }
    }
    if (!done)
        throw BESInternalError(prolog + "Failed to determine size of target resource: " + target_url->str(), __FILE__, __LINE__);

    if (debug) cerr << prolog << "END" << endl;

    return how_big_it_is;
}
size_t get_max_retrival_size(const size_t &max_target_size, shared_ptr<http::url> &target_url) {
    size_t target_size = max_target_size;
    if (max_target_size == 0) {
        target_size = get_remote_size(target_url, true);
        if (debug) cerr << prolog << "Remote resource size is " << max_target_size << " bytes.  " << endl;
    }
    return target_size;
}

/**
 *
 * @param target_url
 * @param output_file
 */
void simple_get(const string target_url_str, const string output_file_base) {

    string output_file = output_file_base + "_simple_get.out";
    vector<string> resp_hdrs;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd;
    if ((fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, mode)) < 0) {
        throw BESInternalError(get_errno(), __FILE__, __LINE__);
    }
    {
        BESStopWatch sw;
        sw.start(prolog + "url: " + target_url_str);
        shared_ptr<http::url> target_url(new http::url(target_url_str));
        curl::http_get_and_write_resource(target_url, fd,
                                          &resp_hdrs); // Throws BESInternalError if there is a curl error.
    }
    close(fd);

    if (Debug) {
        for (size_t i = 0; i < resp_hdrs.size(); i++) {
            cerr << prolog << "ResponseHeader[" << i << "]: " << resp_hdrs[i] << endl;
        }
    }
}


/**
 *
 * @param target_url
 * @param target_size
 * @param chunk_count
 * @param chunks
 */
void make_chunks(shared_ptr<http::url> &target_url, const size_t &target_size, const size_t &chunk_count,
                 vector<dmrpp::Chunk *> &chunks) {
    if (debug) cerr << prolog << "BEGIN" << endl;
    size_t chunk_size = target_size / chunk_count;
    size_t chunk_start = 0;
    size_t chunk_index;
    for (chunk_index = 0; chunk_index < chunk_count; chunk_index++) {
        vector<unsigned long long> position_in_array;
        position_in_array.push_back(chunk_index);
        if (debug)
            cerr << prolog << "chunks[" << chunk_index << "]  chunk_start: " << chunk_start << " chunk_size: "
                 << chunk_size << endl;
        auto chunk = new dmrpp::Chunk(target_url, "LE", chunk_size, chunk_start, position_in_array);
        chunk_start += chunk_size;
        chunks.push_back(chunk);
    }
    if (target_size % chunk_size) {
        // So there's a remainder and we should make a final chunk for it too.
        size_t last_chunk_size = target_size - chunk_start;
        if (debug)
            cerr << prolog << "Remainder chunk. chunk[" << chunks.size() << "] last_chunk_size: " << last_chunk_size
                 << endl;
        if (debug)
            cerr << prolog << "Remainder chunk! target_size: " << target_size << "  index: " << chunk_index
                 << " last_chunk_start: " << chunk_start << " last_chunk_size: " << last_chunk_size << endl;
        if (last_chunk_size > 0) {
            vector<unsigned long long> position_in_array;
            position_in_array.push_back(chunk_index);
            if (debug)
                cerr << prolog << "chunks[" << chunk_index << "]  chunk_start: " << chunk_start << " chunk_size: "
                     << last_chunk_size << endl;
            auto last_chunk = new dmrpp::Chunk(target_url, "LE", last_chunk_size, chunk_start, position_in_array);
            chunks.push_back(last_chunk);
        }
    }
    if (debug) cerr << prolog << "END chunks: " << chunks.size() << endl;
}


/**
 *
 * @param target_url
 * @param target_size
 * @param chunk_count
 */
void serial_chunky_get(shared_ptr<http::url> &target_url, const size_t target_size, const unsigned long chunk_count,
                       const string &output_file_base) {

    shared_ptr<http::url> effectiveUrl = http::EffectiveUrlCache::TheCache()->get_effective_url(target_url);
    if (debug) cerr << prolog << "curl::retrieve_effective_url() returned:  " << effectiveUrl->str() << endl;
    size_t retrieval_size =  get_max_retrival_size(target_size, effectiveUrl);

    string output_file = output_file_base + "_serial_chunky_get.out";
    vector<dmrpp::Chunk *> chunks;
    make_chunks(target_url, retrieval_size, chunk_count, chunks);

    std::ofstream ofs;
    ofs.open(output_file, std::fstream::in | std::fstream::out | std::ofstream::trunc | std::ofstream::binary);
    if (ofs.fail())
        throw BESInternalError(prolog + "Failed to open file: " + output_file, __FILE__, __LINE__);

    for (size_t i = 0; i < chunks.size(); i++) {
        stringstream ss;
        ss << prolog << "chunk={index: " << i << ", offset: " << chunks[i]->get_offset() << ", size: "
           << chunks[i]->get_size() << "}";

        {
            BESStopWatch sw;
            sw.start(ss.str());
            chunks[i]->read_chunk();
        }

        if (debug) cerr << ss.str() << " retrieval from: " << target_url << " completed, timing finished." << endl;
        ofs.write(chunks[i]->get_rbuf(), chunks[i]->get_rbuf_size());
        if (debug) cerr << ss.str() << " has been written to: " << output_file << endl;
    }
    auto itr = chunks.begin();
    while (itr != chunks.end()) {
        delete *itr;
        *itr = 0;
        itr++;
    }

}


void parse_dmrpp(const string &dmrpp_filename_url){
    if(debug)  cerr << prolog << "BEGIN"  << endl;

    dmrpp::DmrppParserSax2 parser;
    string target_file_url = dmrpp_filename_url;
    string target_file;

    const string http_protocol("http://");
    const string https_protocol("https://");
    const string file_protocol("file://");

    if(debug)  cerr << prolog << "dmrpp_filename_url: " << dmrpp_filename_url << endl;

    if(target_file_url.empty())
        throw BESInternalError(prolog + "The dmr++ filename was empty.", __FILE__, __LINE__);


    if(target_file_url.rfind(http_protocol,0)==0 || target_file_url.rfind(https_protocol,0)==0 ){
        // Use RemoteResource to get the thing.
        shared_ptr<http::url> tfile_url(new http::url(target_file_url));
        http::RemoteResource target_resource(tfile_url,prolog+"Timer");
        target_resource.retrieveResource();
        target_file = target_resource.getCacheFileName();
    }
    else if(target_file_url.rfind(file_protocol,0)==0){
        target_file = target_file_url.substr(file_protocol.size());
    }
    else {
        target_file_url = file_protocol + target_file_url;
    }

    if(debug)  cerr << prolog << "       target_file: "  << target_file << endl;

    ifstream ifs(target_file);
    if(ifs.fail())
        throw BESInternalError(prolog + "Failed open to dmr++ file: " + dmrpp_filename_url, __FILE__, __LINE__);

    dmrpp::DmrppTypeFactory factory;
    dmrpp::DMRpp dmr(&factory);
    dmr.set_href(target_file_url);
    stringstream msg;
    msg << prolog << dmrpp_filename_url;
    {
        BESStopWatch sw;
        sw.start(msg.str());
        parser.intern(ifs, &dmr);
    }

    if (Debug) {
        cerr << prolog << "Built dataset: " << endl;
        dmrpp::DmrppCommon::d_print_chunks = true;
        libdap::XMLWriter xmlWriter;
        dmr.print_dmrpp(xmlWriter, dmr.get_href());
        cerr << xmlWriter.get_doc() << endl;
    }
    if(debug)  cerr << prolog << "END"  << endl;


}



/**
 *
 * @param target_url
 * @param target_size
 * @param chunk_count
 */
void add_chunks(shared_ptr<http::url> &target_url, const size_t &target_size, const size_t &chunk_count,
                dmrpp::DmrppArray *target_array) {

    if (debug) cerr << prolog << "BEGIN" << endl;

    size_t chunk_size = target_size / chunk_count;
    if (chunk_size == 0)
        throw BESInternalError(prolog + "Chunk size was zero.", __FILE__, __LINE__);
    stringstream chunk_dim_size;
    chunk_dim_size << chunk_size;
    target_array->parse_chunk_dimension_sizes(chunk_dim_size.str());

    size_t chunk_start = 0;
    size_t chunk_index;
    for (chunk_index = 0; chunk_index < chunk_count; chunk_index++) {
        vector<unsigned long long> position_in_array;
        position_in_array.push_back(chunk_start);
        if (debug)
            cerr << prolog << "chunks[" << chunk_index << "]  chunk_start: " << chunk_start << " chunk_size: "
                 << chunk_size << " chunk_poa: " << position_in_array[0] << endl;
        target_array->add_chunk(target_url, "LE", chunk_size, chunk_start, position_in_array);
        chunk_start += chunk_size;
    }
    if (target_size % chunk_size) {
        // So there's a remainder and we should make a final chunk for it too.
        size_t last_chunk_size = target_size - chunk_start;
        if (debug)
            cerr << prolog << "Remainder chunk! target_size: " << target_size << "  index: " << chunk_index
                 << " last_chunk_start: " << chunk_start << " last_chunk_size: " << last_chunk_size << endl;
        if (last_chunk_size > 0) {
            vector<unsigned long long> position_in_array;
            position_in_array.push_back(chunk_start);
            if (debug)
                cerr << prolog << "chunks[" << chunk_index << "]  chunk_start: " << chunk_start << " chunk_size: "
                     << last_chunk_size << " chunk_poa: " << position_in_array[0] << endl;
            target_array->add_chunk(target_url, "LE", last_chunk_size, chunk_start, position_in_array);
        }
    }
    if (debug) cerr << prolog << "END" << endl;
}



/**
 *
 * @param target_url
 * @param target_size
 * @param chunk_count
 * @param output_file_base
 */
size_t array_get(shared_ptr<http::url> &target_url, const size_t &target_size, const size_t &chunk_count,
                 const string &output_file_base) {

    if (debug) cerr << prolog << "BEGIN" << endl;
    string output_file = output_file_base + "_array_get.out";
    std::ofstream ofs;
    ofs.open(output_file, std::fstream::in | std::fstream::out | std::ofstream::trunc | std::ofstream::binary);
    if (ofs.fail())
        throw BESInternalError(prolog + "Failed to open file: " + output_file, __FILE__, __LINE__);

    auto *tmplt = new dmrpp::DmrppByte("data");
    auto *target_array = new dmrpp::DmrppArray("data", tmplt);
    delete tmplt; // Because the Vector() constructor made a copy and it's our problem...

    target_array->append_dim(target_size);
    add_chunks(target_url, target_size, chunk_count, target_array);
    target_array->set_send_p(true); // Mark it to be sent so that it will be read.

    dmrpp::DmrppTypeFactory factory;
    dmrpp::DMRpp dmr(&factory);
    dmr.set_href(target_url->str());
    dmrpp::DmrppD4Group *root = dynamic_cast<dmrpp::DmrppD4Group *>(dmr.root());
    root->add_var_nocopy(target_array);
    root->set_in_selection(true);

    if (debug) {
        cerr << prolog << "Built dataset: " << endl;
        dmrpp::DmrppCommon::d_print_chunks = true;
        libdap::XMLWriter xmlWriter;
        dmr.print_dmrpp(xmlWriter, dmr.get_href());
        cerr << xmlWriter.get_doc() << endl;
    }

    {
        stringstream timer_msg;
        timer_msg << prolog << "DmrppD4Group.intern_data() for " << target_size << " bytes in " << chunk_count <<
                  " chunks, parallel transfers ";
        if (dmrpp::DmrppRequestHandler::d_use_transfer_threads) {
            timer_msg << "enabled.  (max: " << dmrpp::DmrppRequestHandler::d_max_transfer_threads << ")";
        } else {
            timer_msg << "disabled.";
        }
        BESStopWatch sw;
        sw.start(timer_msg.str());
        root->intern_data();
    }

    size_t started = ofs.tellp();
    libdap::D4StreamMarshaller streamMarshaller(ofs);
    root->serialize(streamMarshaller, dmr);

    size_t stopped = ofs.tellp();
    size_t numberOfBytesWritten = stopped - started;
    if (debug) cerr << prolog << "target_size: " << target_size << "  numberOfBytesWritten: " << numberOfBytesWritten << endl;

    // delete target_array; // Don't have to delete this because we added it to the DMR using add_var_nocopy()
    if (debug) cerr << prolog << "END" << endl;
    return numberOfBytesWritten;

}



/**
 * function retriever_test_02() {
    # 184290 chunks in the dmr++ file for this granule
    target_dataset="https://harmony.uat.earthdata.nasa.gov/service-results/harmony-uat-staging/public/sds/staged/ATL03_20200714235814_03000802_003_01.h5"
    output_prefix="foo";

    # Cleanup before running
    # rm -fv ${output_prefix}*

    for chunks in 8192 16384 32768 65536 131072  262144 ;
    do
        # No parallel reads in this set
        for reps in {1..10};
        do
            retriever -u "${target_dataset}" -d  -o $output_prefix -C $chunks -S 2147483600;
        done;

        for threads in 2 4 8;
        do
            for reps in {1..10};
            do
                retriever -u "${target_dataset}" -d  -o $output_prefix -p $threads -C $chunks -S 2147483600;
            done;
        done
    done
}

 */
#if 0
int test_plan_01(const string &target_url,
                  const string &output_prefix,
                  const unsigned int reps,
                  const size_t retrieval_size,
                  const unsigned int power_of_two_chunk_count,
                  const unsigned int power_of_two_threads_max,
                  const string &output_file_base
                  ) {
    int result = 0;
    if (debug)
        cerr << prolog << "BEGIN" << endl;

    try {
        string effectiveUrl = http::EffectiveUrlCache::TheCache()->get_effective_url(target_url);
        if (debug)
            cerr << prolog << "curl::retrieve_effective_url() returned:  " << effectiveUrl << endl;
        size_t target_size =  get_max_retrival_size(retrieval_size, effectiveUrl);

        // Outer loop on chunk size
        size_t chunk_count = 2;
        for (size_t chunk_pwr = 1; chunk_pwr <= power_of_two_chunk_count; chunk_pwr++) {

            // We turn off parallel transfers to get a baseline that is the single threaded, serial retrieval of the chunks.
            dmrpp::DmrppRequestHandler::d_use_transfer_threads = false;
            for ( unsigned int rep = 0; rep < reps; rep++) {
                array_get(effectiveUrl, target_size, chunk_count, output_file_base );
            }

            // Now we enable threads and starting with 2 work up to power_of_two_threads_max
            dmrpp::DmrppRequestHandler::d_use_transfer_threads = true;
            unsigned int thread_count = 2;
            for ( unsigned int tpwr = 1; tpwr <= power_of_two_threads_max; tpwr++) {
                dmrpp::DmrppRequestHandler::d_max_transfer_threads = thread_count;
                for ( unsigned int rep = 0; rep < reps; rep++) {
                    array_get(effectiveUrl, target_size, chunk_count, output_file_base);
                }
                thread_count *= 2;
            }
            chunk_count *= 2;
        }
    }
    catch (
            BESError e
    ) {
        cerr << prolog << "Caught BESError. Message: " << e.get_message() << "  " << e.get_file()<< ":" << e. get_line() << endl;
        result = 1;
    }
    catch (...) {
        cerr << prolog << "Caught Unknown Exception." <<
             endl;
        result = 2;
    }
    cerr << prolog << "END" << endl;
    return result;
}
#endif

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {

    int result = 0;
    string bes_log_file;
    string bes_debug_log_file = "cerr";
    string bes_debug_keys = "bes,http,curl,dmrpp,dmrpp:3,dmrpp:4,rr";
    shared_ptr<http::url> target_url(new http::url("https://www.opendap.org/pub/binary/hyrax-1.16/centos-7.x/bes-debuginfo-3.20.7-1.static.el7.x86_64.rpm"));
    string output_file_base("retriever");
    string http_cache_dir;
    string prefix;
    size_t pwr2_number_o_chunks = 18;
    size_t max_target_size = 0;
    string http_netrc_file;
    unsigned int reps=10;
    unsigned pwr2_parallel_reads = 0;
    // Unused bool aws_sign_request_url = false;

    char *prefixCstr = getenv("prefix");
    if (prefixCstr) {
        prefix = prefixCstr;
    } else {
        prefix = "/";
    }
    auto bes_config_file = BESUtil::assemblePath(prefix, "/etc/bes/bes.conf", true);


    GetOpt getopt(argc, argv, "h:r:n:C:c:o:u:l:S:dbDp:");   // Removed A. Unused jhrg 11/23/21
    int option_char;
    while ((option_char = getopt()) != -1) {
        switch (option_char) {
            case 'D':
                Debug = true;
                debug = true;
                break;
            case 'd':
                debug = true;
                break;
            case 'b':
                bes_debug = true;
                break;
#if 0
            case 'A':
                // Unused aws_sign_request_url = true;
                break;
#endif
            case 'c':
                bes_config_file = getopt.optarg;
                break;
            case 'u':
                target_url = shared_ptr<http::url>(new http::url(getopt.optarg));
                break;
            case 'l':
                bes_log_file = getopt.optarg;
                break;
            case 'n':
                http_netrc_file = getopt.optarg;
                break;
            case 'o':
                output_file_base = getopt.optarg;
                break;
            case 'C':
                pwr2_number_o_chunks = atol(getopt.optarg);
                break;
            case 'S':
                max_target_size = atol(getopt.optarg);
                break;
            case 'p':
                pwr2_parallel_reads = atol(getopt.optarg);
                break;
            case 'r':
                reps = atol(getopt.optarg);
                break;
            case 'h':
                http_cache_dir = getopt.optarg;
                break;

            default:
                break;
        }
    }

    if (bes_log_file.empty()) {
        bes_log_file = output_file_base + "_bes.log";
    }

    cerr << prolog << "-- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - " << endl;
    cerr << prolog << "debug: " << (debug ? "true" : "false") << endl;
    cerr << prolog << "Debug: " << (Debug ? "true" : "false") << endl;
    cerr << prolog << "bes_debug: " << (bes_debug ? "true" : "false") << endl;
    cerr << prolog << "output_file_base: '" << output_file_base << "'" << endl;
    cerr << prolog << "bes_config_file: '" << bes_config_file << "'" << endl;
    cerr << prolog << "bes_log_file: '" << bes_log_file << "'" << endl;
    cerr << prolog << "bes_debug_log_file: '" << bes_debug_log_file << "'" << endl;
    cerr << prolog << "bes_debug_keys: '" << bes_debug_keys << "'" << endl;
    cerr << prolog << "http_netrc_file: '" << http_netrc_file << "'" << endl;
    cerr << prolog << "target_url: '" << target_url->str() << "'" << endl;
    cerr << prolog << "max_target_size: '" << max_target_size << "'" << endl;
    cerr << prolog << "number_o_chunks: 2^" << pwr2_number_o_chunks << endl;
    cerr << prolog << "reps: " << reps << endl;
    if (pwr2_parallel_reads)
        cerr << prolog << "parallel_reads: ENABLED (max: 2^" << pwr2_parallel_reads << ")" << endl;
    else
        cerr << prolog << "parallel_reads: DISABLED" << endl;
    cerr << prolog << "-- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - " << endl;


    try {
        if(pwr2_parallel_reads){
            unsigned long long int max_threads = 1ULL << pwr2_parallel_reads;
            dmrpp::DmrppRequestHandler::d_use_transfer_threads = true;
            dmrpp::DmrppRequestHandler::d_max_transfer_threads = max_threads;
        }
        else {
            dmrpp::DmrppRequestHandler::d_use_transfer_threads = false;
            dmrpp::DmrppRequestHandler::d_max_transfer_threads = 1;
        }

        dmrpp::DmrppRequestHandler *dmrppRH = bes_setup(bes_config_file, bes_log_file, bes_debug_log_file,
                                                        bes_debug_keys, http_netrc_file,http_cache_dir);
        
        shared_ptr<http::url> effectiveUrl = http::EffectiveUrlCache::TheCache()->get_effective_url(target_url);
        if (debug)  cerr << prolog << "curl::retrieve_effective_url() returned:  " << effectiveUrl << endl;
        size_t target_size =  get_max_retrival_size(max_target_size, effectiveUrl);

        unsigned long long int chunks = 1ULL << pwr2_number_o_chunks;
        if (debug)  cerr << prolog << "Dividing target into " << chunks << " chunks." << endl;



        array_get(effectiveUrl, target_size, chunks, output_file_base);


#if 0 // these work but are parked a.t.m.
        result = test_plan_01(
                target_url,
                output_file_base,
                reps,
                max_target_size,
                pwr2_number_o_chunks,
                pwr2_parallel_reads,
                output_file_base) ;

        simple_get(effectiveUrl, output_file_base);
        serial_chunky_get( effectiveUrl,  max_target_size, pwr2_number_o_chunks, output_file_base);

        parse_dmrpp(target_url);


        string effectiveUrl = http::EffectiveUrlCache::TheCache()->get_effective_url(target_url);
        if (debug)
            cerr << prolog << "curl::retrieve_effective_url() returned:  " << effectiveUrl << endl;
        target_size =  get_max_retrival_size(retrieval_size, effectiveUrl);
        array_get(effectiveUrl, max_target_size, pwr2_number_o_chunks, output_file_base);
#endif

        curl_global_cleanup();
        delete dmrppRH;
    }
    catch (BESError e) {
        cerr << prolog << "Caught BESError. Message: " << e.get_message() << "  " << e.get_file() << ":" << e.get_line()
             << endl;
        result = 1;
    }
    catch (...) {
        cerr << prolog << "Caught Unknown Exception." << endl;
        result = 2;
    }

    return result;
}
