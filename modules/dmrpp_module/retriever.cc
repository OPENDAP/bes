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

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <sstream>
#include <iostream>
#include <fstream>
#include <GetOpt.h>

#include <curl/curl.h>
#include <BESLog.h>

#include "BESInternalError.h"
#include "BESUtil.h"
#include "CurlUtils.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "BESDebug.h"
#include "BESStopWatch.h"

#include "Chunk.h"
#include "CredentialsManager.h"
#include "CurlHandlePool.h"
#include "DmrppRequestHandler.h"

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



/**
 *
 * @return
 */
string get_errno()
{
    char *s_err = strerror(errno);
    if (s_err)
        return s_err;
    else
        return "Unknown error.";
}




/**
 *
 * @param url
 * @return
 */
size_t get_size(string url){
    // TODO Use cURL to perform a HEAD on the URL and figure out how big the thing is.
    char error_buffer[CURL_ERROR_SIZE];
    std::vector<std::string> resp_hdrs;
    CURL *ceh = curl::init(url, NULL, &resp_hdrs);

    curl::set_error_buffer(ceh, error_buffer);

    CURLcode curl_status = curl_easy_setopt(ceh, CURLOPT_NOBODY, 1L);
    curl::eval_curl_easy_setopt_result(curl_status, prolog, "CURLOPT_NOBODY", error_buffer, __FILE__, __LINE__);

    if(Debug) cerr << prolog << "HEAD request is configured" << endl;

    curl::super_easy_perform(ceh);
    bool done = false;
    string content_length_hdr_key("content-length: ");
    for(size_t i=0; i<resp_hdrs.size() ;i++){
        if(Debug) cerr << prolog << "HEADER["<<i<<"]: " << resp_hdrs[i] << endl;
        string lc_header = BESUtil::lowercase(resp_hdrs[i]);
        size_t index = lc_header.find(content_length_hdr_key);
        if(index==0){
            string value = lc_header.substr(content_length_hdr_key.size());
            size_t ret_val = stol(value);
            return ret_val;
        }
    }
    throw BESInternalError(prolog + "Failed to determine size of target resource: " + url, __FILE__, __LINE__);

}

/**
 *
 * @param target_url
 * @param output_file
 */
void simple_get(const string target_url, const string output_file_base) {

    string target_file = output_file_base + "_simple_get.log";
    vector<string> resp_hdrs;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd;
    if ((fd = open(target_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC , mode)) < 0) {
        throw BESInternalError(get_errno(), __FILE__, __LINE__);
    }
    {
        BESStopWatch sw;
        sw.start(prolog + "url: " + target_url);
        curl::http_get_and_write_resource(target_url, fd, &resp_hdrs); // Throws BESInternalError if there is a curl error.
    }
    close(fd);

    if(Debug){
        for(size_t i=0; i<resp_hdrs.size() ;i++){
            cerr << prolog << "ResponseHeader["<<i<<"]: " << resp_hdrs[i] << endl;
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
void make_chunks(const string target_url, const size_t target_size, unsigned chunk_count, vector<dmrpp::Chunk *> &chunks){
    size_t chunk_size = target_size/(chunk_count-1);
    size_t chunk_start = 0;
    size_t chunk_index;
    for(chunk_index=0; chunk_index<chunk_count; chunk_index++){
        vector<unsigned int> position_in_array;
        position_in_array.push_back(chunk_index);
        if(debug) cerr << prolog << "chunks[" << chunk_index << "]  chunk_start: " << chunk_start << " chunk_size: " << chunk_size << endl;
        auto chunk = new dmrpp::Chunk(target_url, "LE", chunk_size, chunk_start,position_in_array);
        chunk_start += chunk_size;
        chunks.push_back(chunk);
    }
    size_t last_chunk_size = target_size - chunk_start;
    if(last_chunk_size>0){
        vector<unsigned int> position_in_array;
        position_in_array.push_back(chunk_index);
        if(debug) cerr << prolog << "chunks["<<chunk_index<<"]  chunk_start: " << chunk_start << " chunk_size: " << last_chunk_size << endl;
        auto last_chunk = new dmrpp::Chunk(target_url, "LE", last_chunk_size, chunk_start,position_in_array);
        chunks.push_back(last_chunk);
    }
    if(debug) cerr << prolog << "Built " << chunks.size() << " Chunk objects." << endl;

}


/**
 *
 * @param target_url
 * @param target_size
 * @param chunk_count
 */
void serial_chunky_get(const string target_url, const size_t target_size, unsigned chunk_count, string output_file_base){

    string target_file = output_file_base + "_serial_chunky_get.log";
    vector<dmrpp::Chunk *> chunks;
    make_chunks(target_url, target_size, chunk_count, chunks);

    std::ofstream fs;
    fs.open (target_file, std::fstream::in | std::fstream::out | std::ofstream::trunc | std::ofstream::binary);
    if(fs.fail())
        throw BESInternalError(prolog + "Failed to open file: "+target_file, __FILE__, __LINE__);

    for(size_t i=0; i<chunks.size(); i++){
        stringstream ss;
        ss << prolog << "chunk[" << i <<  "]";
        {
            BESStopWatch sw;
            sw.start(ss.str());
            chunks[i]->read_chunk();
        }
        if(debug) cerr << ss.str() << " has been read from: " << target_url << endl;
        fs.write(chunks[i]->get_rbuf(),chunks[i]->get_rbuf_size());
        if(debug) cerr << ss.str() << " has been written to: " << target_file << endl;
    }

    auto itr = chunks.begin();
    while(itr != chunks.end()){
        delete *itr;
    }

}







/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{

    int result = 0;
    string log_file="retriever.log";
    string target_url="https://www.opendap.org/pub/binary/hyrax-1.16/centos-7.x/bes-debuginfo-3.20.7-1.static.el7.x86_64.rpm";
    string output_file_base("retriever");
    string prefix;

    char *prefixCstr = getenv("prefix");
    if(prefixCstr){
        prefix = prefixCstr;
    }
    else {
        prefix = "/";
    }
    auto bes_conf = BESUtil::assemblePath(prefix, "/etc/bes/bes.conf", true);


    GetOpt getopt(argc, argv, "c:o:u:l:dbD");
    int option_char;
    while ((option_char = getopt()) != -1) {
        switch (option_char) {
            case 'D':
                Debug = true;
                break;
            case 'd':
                debug = true;
                break;
            case 'b':
                debug = true;
                bes_debug = true;
                break;
            case 'c':
                bes_conf = getopt.optarg;
                break;
            case 'u':
                target_url = getopt.optarg;
                break;
            case 'l':
                log_file = getopt.optarg;
                break;
            case 'o':
                output_file_base = getopt.optarg;
                break;
            default:
                break;
        }
    }

    cerr << "debug: " << (debug?"true":"false") << endl;
    cerr << "bes_debug: " << (bes_debug?"true":"false") << endl;
    cerr << "bes.conf: " << bes_conf << endl;
    cerr << "target_url: '" << target_url << "'" << endl;
    cerr << "output_file_base: '" << output_file_base << "'" << endl;


    try {
        TheBESKeys::ConfigFile = bes_conf; // Set the config file for TheBESKeys
        TheBESKeys::TheKeys()->set_key("BES.LogName",log_file); // Set the log file so it goes where we say.
        TheBESKeys::TheKeys()->set_key("AllowedHosts","^https?:\\/\\/.*$"); // Disable AllowedHosts
        if(bes_debug) BESDebug::SetUp("cerr,bes,http,curl,dmrpp"); // Enable BESDebug settings

        // Initialize the dmr++ goodness.
        dmrpp::DmrppRequestHandler *dmrppRH = new dmrpp::DmrppRequestHandler("Chaos");

        size_t target_size = get_size(target_url);

        if(debug) cerr << prolog << "Remote resource is " << target_size << " bytes." << endl;

        simple_get(target_url, output_file_base);

        serial_chunky_get( target_url,  target_size, 1000, output_file_base);


        delete dmrppRH;
    }
    catch(BESError e){
        cerr << "Caught BESError. Message: " << e.get_message() << "  " << e.get_file() << ":"<< e.get_line() << endl;
        result = 1;
    }
    catch(...){
        cerr << "Caught Unknown Exception." << endl;
        result =  2;
    }

    return result;
}
