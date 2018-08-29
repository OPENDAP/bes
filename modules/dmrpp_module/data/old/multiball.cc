// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>
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

// Added copyright, config.h and removed unused headers. jhrg

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <cstdlib>
#include <GetOpt.h>
#include <cassert>
#include <map>
#include <curl/curl.h>

static bool debug = false;
static bool deep_debug = false;
using namespace std;

long long get_file_size(string filename) {
    long long size;
    ifstream in(filename.c_str(), ifstream::ate | ifstream::binary);
    size = in.tellg();
    in.close();
    return size;
}

long long get_remote_resource_size(string url) {
    long long size;
    ifstream in(url.c_str(), ifstream::ate | ifstream::binary);
    size = in.tellg();
    in.close();
    return size;
}
std::string get_curl_range_arg_string(unsigned long offset,unsigned long size)
{
    ostringstream range;   // range-get needs a string arg for the range
    range << offset << "-" << offset + size - 1;
    return range.str();
}

size_t do_the_multiball(void *buffer, size_t size, size_t nmemb, void *data);

class Shard {

public:
    string d_url;
    unsigned long long d_offset;
    unsigned long long d_size;
    CURL *d_curl_easy_handle;
    char d_curl_error_buf[CURL_ERROR_SIZE];
    FILE *d_fd;
    string d_output_filename;

    // libcurl call back uses these.
    unsigned long long d_bytes_read;
    char *d_read_buffer;
    fstream *d_fstream;


    Shard() :
        d_url("http://www.opendap.org"),
        d_offset(0),
        d_size(0),
        d_curl_easy_handle(0),
        d_fd(0),
        d_output_filename("/dev/null"),
        d_bytes_read(0),
        d_read_buffer(0),
        d_fstream(0)
    {
    }
    ~Shard(){
        delete[] d_read_buffer;
        if(d_fd){
            fclose(d_fd);
            d_fd = 0;
        }
    }


    void dump(ostream &oss) const
    {
        bool pretty=true;
        oss << "Shard" << (pretty?"\n":"");
        oss << "[ptr='" << (void *)this << "']"<< (pretty?"\n":"");
        oss << "[url='" << d_url << "']"<< (pretty?"\n":"");
        oss << "[offset=" << d_offset << "]"<< (pretty?"\n":"");
        oss << "[size=" << d_size << "]"<< (pretty?"\n":"");
        oss << "[output_filename=" << d_output_filename << "]"<< (pretty?"\n":"");
        oss << "[curl_handle=" << (void *)d_curl_easy_handle << "]"<< (pretty?"\n":"");
        oss << "[bytes_read=" << d_bytes_read << "]"<< (pretty?"\n":"");
        oss << "[read_buffer=" << (void *)d_read_buffer << "]"<< (pretty?"\n":"");
        oss << "[fstream=" << (void *)d_fstream << "]"<< (pretty?"\n":"");
    }

    string to_string()
    {std::ostringstream oss;  dump(oss); return oss.str();}

    void allocate()
    {
        // Calling delete on a null pointer is fine, so we don't need to check
        // to see if this is the first call.
        delete[] d_read_buffer;
        d_read_buffer = new char[d_size];
        d_bytes_read = 0;
    }



    void init_curl_handle()
    {
        allocate();
        //d_fstream = new fstream();
        //d_fstream->open(d_output_filename, std::fstream::out);
        d_fd = fopen(d_output_filename.c_str(),"w");

        string range = get_curl_range_arg_string(d_offset,d_size);
        if(debug)
            cerr << __func__ << "() - Initializing CuRL handle. url: " << d_url << " offset: "<< d_offset <<
            " size: " << d_size << " curl_range: " << range << endl;
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw string("Shard: Unable to initialize libcurl! '");
        }
        CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, d_url.c_str());
        if (res != CURLE_OK)
            throw string(curl_easy_strerror(res));

        // Use CURLOPT_ERRORBUFFER for a human-readable message
        //
        res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, d_curl_error_buf);
        if (res != CURLE_OK)
            throw string(curl_easy_strerror(res));

        // get the offset to offset + size bytes
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str() /*"0-199"*/))
            throw  string("HTTP Error: ").append(d_curl_error_buf);

        // Pass all data to the 'write_data' function
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, do_the_multiball))
            throw string("HTTP Error: ").append(d_curl_error_buf);

        // Pass this to write_data as the fourth argument
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, this))
            throw string("HTTP Error: ").append(d_curl_error_buf);

        d_curl_easy_handle = curl;
    }

    void clean_up_curl(){
        if(d_curl_easy_handle!=0)
            curl_easy_cleanup(d_curl_easy_handle);
        d_curl_easy_handle = 0;
    }


};

size_t do_the_multiball(void *buffer, size_t size, size_t nmemb, void *data)
{
    Shard *shard = reinterpret_cast<Shard*>(data);

    // cerr << __func__ << "() - BEGIN  curl_buffer: "<< (void*)buffer<< "  shard: " << (void*)shard << endl;
    // rbuf: |******++++++++++----------------------|
    //              ^        ^ bytes_read + nbytes
    //              | bytes_read

    unsigned long long bytes_read = shard->d_bytes_read;
    size_t nbytes = size * nmemb;
/*
    cerr <<  __func__ << "() -"
        << " bytes_read: " << bytes_read
        << ", size: " << size
        << ", nmemb: " << nmemb
        << ", nbytes: " << nbytes
        << ", rbuf_size: " << shard->d_size
        << endl;
       */
    // If this fails, the code will write beyond the buffer.
    assert(bytes_read + nbytes <= shard->d_size);

    // memcpy(shard->d_read_buffer + bytes_read, buffer, nbytes);
    // shard->d_fstream->write((const char *)buffer,nbytes);
    fwrite (buffer , sizeof(char), nbytes, shard->d_fd);

    shard->d_bytes_read += nbytes;
    if(deep_debug) cerr << shard->d_output_filename << ":" << shard->d_bytes_read << ":"<< nbytes << endl;

    return nbytes;
}




string usage(string prog_name) {
    ostringstream ss;
    ss << "usage: " << prog_name << "[-d] -o output -u <url> ]" << endl;
    return ss.str();
}


void make_shards(map<CURL*, Shard *> *shards_map, unsigned int shard_count, string url, unsigned int file_size, string output_filename){

    if(debug) cerr << __func__ << "() - Target size: " << file_size << endl;

    unsigned long long int shard_size = file_size/shard_count;
    long long byte_count = 0;
    for(unsigned int i=0; i<shard_count ; i++){
        unsigned long long my_size=shard_size;
        unsigned long long offset = i * shard_size;
        if((offset + shard_size) > file_size){
            my_size=file_size - offset;
        }
        Shard *shard = new Shard();
        shard->d_url = url;
        shard->d_offset = offset;
        shard->d_size = my_size;
        ostringstream oss;
        oss << output_filename << "_" << (i<10?"0":"")<< i<< "_shard";
        shard->d_output_filename = oss.str();
        shard->init_curl_handle();
        shards_map->insert(
            std::pair<CURL*,Shard*>(shard->d_curl_easy_handle,shard));
        byte_count+=shard_size;
    }

}

int main(int argc, char **argv) {
    string output_file;
    string url;
    long long file_size;
    unsigned int shard_count;


    url = "";
    shard_count=1;
    file_size = 0;


    GetOpt getopt(argc, argv, "Ddc:s:o:u:");
    int option_char;
    while ((option_char = getopt()) != EOF) {
        switch (option_char) {
        case 'D':
            deep_debug = true;
            break;
        case 'd':
            debug = true;
            break;
        case 'o':
            output_file = getopt.optarg;
            break;
        case 'u':
            url = getopt.optarg;
            break;
        case 'c':
            std::istringstream(getopt.optarg) >> shard_count;
            break;
        case 's':
            std::istringstream(getopt.optarg) >> file_size;
            break;
        case '?':
            cerr << usage(argv[0]);
            break;
        }
    }
//    cerr << "debug:     " << (debug ? "ON" : "OFF") << endl;
    if (debug) {
        cerr << "url: '" << url << "'" << endl;
        cerr << "output_file: '" << output_file << "'" << endl;
        cerr << "file_size: '" << file_size << "'" << endl;
        cerr << "shard_count: '" << shard_count << "'" << endl;
    }

    bool qc_flag = false;
    if (output_file.empty()) {
        cerr << "Output File name must be specified using the -o option" << endl;
        qc_flag = true;
    }
    if (url.empty()) {
        cerr << "Ingest URL must be specified using the -u option" << endl;
        qc_flag = true;
    }
    if (qc_flag) {
        return 1;
    }
    map<CURL*, Shard *> shards_map;

    make_shards(&shards_map,shard_count,url,file_size,output_file);

    CURLM *curl_multi_handle = curl_multi_init();
    // curl_multi_add_handle(curl_multi_handle, shard->d_curl_easy_handle);
    // Show shards
    std::map<CURL*, Shard*>::iterator it;
    for(it=shards_map.begin(); it!=shards_map.end(); ++it){
        curl_multi_add_handle(curl_multi_handle, it->second->d_curl_easy_handle);
        if(debug) cerr << it->second->d_output_filename << " added to CuRL multi_handle."<< endl;
    }

    int repeats = 0;
    long long lap_counter = 0;
    CURLMcode mcode;
    int still_running=0;

    do {
        int numfds;
        // cerr << __func__ <<"() Calling curl_multi_perform() still_running: "<< still_running << "lap: "<< lap_counter << endl;
        lap_counter++;
        mcode = curl_multi_perform(curl_multi_handle, &still_running);
        // cerr << __func__ <<"() Completed curl_multi_perform() mcode: " << mcode << endl;

        if(mcode == CURLM_OK ) {
            /* wait for activity, timeout or "nothing" */
            // cerr << __func__ <<"() Calling curl_multi_wait() still_running: "<< still_running<< endl;
            mcode = curl_multi_wait(curl_multi_handle, NULL, 0, 1000, &numfds);
            // cerr << __func__ <<"() Completed curl_multi_wait() mcode: " << mcode << endl;
        }

        if(mcode != CURLM_OK) {
            break;
        }

        /* 'numfds' being zero means either a timeout or no file descriptors to
         wait for. Try timeout on first occurrence, then assume no file
         descriptors and no file descriptors to wait for means wait for 100
         milliseconds. */

        if(!numfds) {
            repeats++; /* count number of repeated zero numfds */
            if(repeats > 1) {
                /* sleep 100 milliseconds */
                usleep(100 * 1000);   // usleep takes sleep time in us (1 millionth of a second)
            }
        }
        else
            repeats = 0;

    } while(still_running);


    // cerr << endl;
    cerr << "CuRL multi_perfom has finished. mcode: "<< mcode <<
        " multi_wait_laps: " << lap_counter <<
        " shards: "<< shards_map.size() << endl;

    if(mcode == CURLM_OK) {
        CURLMsg *msg; /* for picking up messages with the transfer status */
        int msgs_left; /* how many messages are left */

        /* See how the transfers went */
        while ((msg = curl_multi_info_read(curl_multi_handle, &msgs_left))) {
            if(debug) cerr << "CuRL Messages remaining: " << msgs_left << endl;
            string shard_str = "No Chunk Found For Handle!";
            /* Find out which handle this message is about */
            it = shards_map.find(msg->easy_handle);
            if(it==shards_map.end()){
                ostringstream oss;
                oss << "OUCH! Failed to locate Shard instance for curl_easy_handle: " << (void*)(msg->easy_handle)<<endl;
                cerr << oss.str() << endl;
                throw oss.str();
            }
            Shard *shard = it->second;

            if (msg->msg == CURLMSG_DONE) {
                if(debug) cerr << __func__ <<"() Chunk Read Completed For Chunk: " << shard->to_string() << endl;
            }
            else {
                ostringstream oss;
                oss << "DmrppArray::" << __func__ <<"() Chunk Read Did Not Complete. CURLMsg.msg: "<< msg->msg <<
                    " Chunk: " << shard->to_string();
                cerr << oss.str() << endl;
                throw oss.str();
            }
        }
        if(debug) cerr << "CuRL Messages remaining: " << msgs_left << endl;

    }
    else {
        cerr << "CuRL MultiFAIL! mcode: " << mcode << endl;
    }
    if(debug) cerr << "Cleaning up CuRL" << endl;

    for(it=shards_map.begin(); it!=shards_map.end(); ++it){
        Shard *shard = it->second;
        if(debug) cerr << shard->to_string() << endl;
        curl_multi_remove_handle(curl_multi_handle, it->second->d_curl_easy_handle);
        shard->clean_up_curl();
    }
    curl_multi_cleanup(curl_multi_handle);

    if(mcode != CURLM_OK) {
        ostringstream oss;
        oss << "DmrppArray: CuRL operation Failed!. multi_code: " << mcode  << endl;
        cerr << oss.str() << endl;
        throw oss.str();
    }





    return 0;
}






