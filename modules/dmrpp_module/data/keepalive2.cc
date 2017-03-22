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
#include <vector>
#include <curl/curl.h>

static bool debug = false;
static bool dry_run = false;
using namespace std;

char curl_error_buf[CURL_ERROR_SIZE];


size_t do_the_multiball(void *buffer, size_t size, size_t nmemb, void *data);

class Shard {

public:
    string d_url;
    unsigned long long d_offset;
    unsigned long long d_size;
    CURL *d_curl_easy_handle;
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

    void open(){
        allocate();
        open_output_file();
    }

    void close(){
        delete[] d_read_buffer;
        d_read_buffer = 0;
        d_bytes_read = 0;

        if(d_fd)
            fclose(d_fd);
        d_fd=0;
    }


    void open_output_file(){
        d_fd = fopen(d_output_filename.c_str(),"w");
    }
    std::string get_curl_range_arg_string(){
        ostringstream range;   // range-get needs a string arg for the range
        range << d_offset << "-" << d_offset + d_size - 1;
        return range.str();
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
    // cerr << shard->d_output_filename << ":" << shard->d_bytes_read << ":"<< nbytes << endl;

    return nbytes;
}



void make_shards(vector<Shard *> *shards, unsigned int shard_count, string url, unsigned int file_size, string output_filename){

    unsigned long long int shard_size = file_size/shard_count;

    cerr << __func__ << "() - Target size: " << file_size <<
        " shard_size: " << shard_size <<
        " shard_count: " << shard_count << endl;

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
        oss << output_filename << "_" <<
            (i<10?"0":"") << (i<100?"0":"") <<
            (i<1000?"0":"") << (i<10000?"0":"") <<
            i << "_shard";
        shard->d_output_filename = oss.str();
        shards->push_back(shard);
        byte_count+=shard_size;
    }

}

void groom_curl_handle(CURL *curl, Shard *shard, bool keep_alive)
{

    string url = shard->d_url;
    string range = shard->get_curl_range_arg_string();
    if(debug)
        cerr << __func__ << "() - Initializing CuRL handle. url: " << url <<
        " curl_range: " << range <<
        " keep_alive: " << (keep_alive?"true":"false") <<
        endl;

    CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    if (res != CURLE_OK)
        throw string(curl_easy_strerror(res));

    // Use CURLOPT_ERRORBUFFER for a human-readable message
    //
    res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error_buf);
    if (res != CURLE_OK)
        throw string(curl_easy_strerror(res));

    // get the offset to offset + size bytes
    if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str() /*"0-199"*/))
        throw  string("HTTP Error: ").append(curl_error_buf);

    // Pass all data to the 'write_data' function
    if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, do_the_multiball))
        throw string("HTTP Error: ").append(curl_error_buf);

    // Pass this to write_data as the fourth argument
    if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, shard))
        throw string("HTTP Error: ").append(curl_error_buf);

    if(keep_alive){
        /* enable TCP keep-alive for this transfer */
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L))
            throw string("HTTP Error: ").append(curl_error_buf);

        /* keep-alive idle time to 120 seconds */
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L))
            throw string("HTTP Error: ").append(curl_error_buf);

        /* interval time between keep-alive probes: 60 seconds */
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L))
            throw string("HTTP Error: ").append(curl_error_buf);
    }
}


void run_multi_perform(CURLM *curl_multi_handle, map<CURL*,Shard*> *shards_map){
    int repeats;
    long long lap_counter = 0;
    CURLMcode mcode;
    int still_running=0;

    if(debug) cerr << __func__ << "() - BEGIN" << endl;

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
    cerr << __func__ << "() - CuRL multi_perfom has finished. mcode: "<< mcode <<
        " multi_wait_laps: " << lap_counter <<
        " shards: "<< shards_map->size() << endl;

    if(mcode == CURLM_OK) {
        CURLMsg *msg; /* for picking up messages with the transfer status */
        int msgs_left; /* how many messages are left */

        /* See how the transfers went */
        while ((msg = curl_multi_info_read(curl_multi_handle, &msgs_left))) {
            if(debug) cerr << "CuRL Messages remaining: " << msgs_left << endl;
            string shard_str = "No Chunk Found For Handle!";
            /* Find out which handle this message is about */
            std::map<CURL*, Shard*>::iterator it = shards_map->find(msg->easy_handle);
            if(it==shards_map->end()){
                ostringstream oss;
                oss << "OUCH! Failed to locate Shard instance for curl_easy_handle: " << (void*)(msg->easy_handle)<<endl;
                cerr << oss.str() << endl;
                throw oss.str();
            }
            Shard *shard = it->second;

            if (msg->msg == CURLMSG_DONE) {
                if(debug) cerr << __func__ <<"() Shard Read Completed For Chunk: " << shard->to_string() << endl;
            }
            else {
                ostringstream oss;
                oss << __func__ <<"() Shard Read Did Not Complete. CURLMsg.msg: "<< msg->msg <<
                    " Chunk: " << shard->to_string();
                cerr << oss.str() << endl;
                throw oss.str();
            }
        }
        if(debug) cerr << "CuRL Messages remaining: " << msgs_left << endl;

    }
    else {
        ostringstream oss;
        oss << __func__ <<"() CuRL MultiFAIL. mcode: "<< mcode <<
            " msg: " << curl_multi_strerror(mcode);
        cerr << oss.str() << endl;
        throw oss.str();
    }
    if(debug) cerr << __func__ << "() - END" << endl;

}

void get_shards_no_curl_handle_reuse(vector<Shard*>*shards, unsigned int max_easy_handles, bool keep_alive){

    cerr << __func__ << "() - Curl easy handles will NOT be recycled. keep_alive: " << (keep_alive?"true":"false") << endl;
    if(dry_run) return;

    map<CURL*, Shard *> shards_map;
    vector<CURL *> all_easy_handles;
    vector<CURL *> current_easy_handles;
    CURLM *curl_multi_handle = curl_multi_init();

    unsigned long int n = 1;
    std::vector<Shard*>::iterator sit;
    for(sit=shards->begin(); sit!=shards->end(); sit++, n++){
        // Set up the shard to be read .
        CURL* curl = curl_easy_init();
        Shard *shard = *sit;
        shard->open();
        groom_curl_handle(curl,shard,keep_alive);
        current_easy_handles.push_back(curl);
        shards_map.insert(std::pair<CURL*,Shard*>(curl,shard));
        // Add it to our current multi handle
        curl_multi_add_handle(curl_multi_handle, curl);

        /**
         * If it's time, pull the trigger and get the stuff.
         */
        if(n && !(n%max_easy_handles)){
            // get the stuff
            run_multi_perform(curl_multi_handle,&shards_map);
            // clean up
            std::vector<CURL*>::iterator cit;
            for(cit=current_easy_handles.begin(); cit!=current_easy_handles.end(); ++cit){
                CURL *easy_handle = *cit;
                std::map<CURL*, Shard*>::iterator mit = shards_map.find(easy_handle);
                if(mit==shards_map.end()){
                    ostringstream oss;
                    oss << "OUCH! Failed to locate Shard instance for curl_easy_handle: " << (void*)(easy_handle)<<endl;
                    cerr << oss.str() << endl;
                    throw oss.str();
                }
                Shard *shard = mit->second;
                shard->close();
                curl_multi_remove_handle(curl_multi_handle, easy_handle);
                curl_easy_cleanup(easy_handle);

            }
            current_easy_handles.clear();
            shards_map.clear();
            curl_multi_cleanup(curl_multi_handle);
            curl_multi_handle = curl_multi_init();
        }
    }
    if(current_easy_handles.size()){
        run_multi_perform(curl_multi_handle,&shards_map);
        // clean up
        std::vector<CURL*>::iterator cit;
        for(cit=current_easy_handles.begin(); cit!=current_easy_handles.end(); ++cit){
            CURL *easy_handle = *cit;
            curl_multi_remove_handle(curl_multi_handle, easy_handle);
            curl_easy_cleanup(easy_handle);
        }
        current_easy_handles.clear();
        curl_multi_cleanup(curl_multi_handle);
        // curl_multi_handle = curl_multi_init();
    }

}

/**
 * Gets all the shards from whereever while reusing the curl easy handles and setting keep-alive
 *
 */
void get_shards_reuse_curl_handles(vector<Shard*>*shards, unsigned int max_easy_handles, bool keep_alive){

    cerr << __func__ << "() - Recycling curl easy handles. keep_alive: " << (keep_alive?"true":"false") << endl;
    if(dry_run) return;

    map<CURL*, Shard *> shards_map;
    vector<CURL *> all_easy_handles;
    vector<CURL *> active_easy_handles;
    CURLM *curl_multi_handle = curl_multi_init();

    unsigned long int n = 1;
    std::vector<Shard*>::iterator sit;
    for(sit=shards->begin(); sit!=shards->end(); sit++, n++){
        Shard *shard = *sit;
        shard->open();

        CURL* curl;
        if(all_easy_handles.size()<max_easy_handles){
            curl = curl_easy_init();
            all_easy_handles.push_back(curl);
        }
        else {
            curl = all_easy_handles[n%max_easy_handles];
        }
        active_easy_handles.push_back(curl);
        groom_curl_handle(curl,shard,keep_alive);
        shards_map.insert(std::pair<CURL*,Shard*>(curl,shard));
        // Add it to our current multi handle
        curl_multi_add_handle(curl_multi_handle, curl);

        /**
         * If it's time, pull the trigger and get the stuff.
         */
        if(n && !(n%max_easy_handles)){
            // get the stuff
            run_multi_perform(curl_multi_handle,&shards_map);
            // clean up
            std::vector<CURL*>::iterator cit;
            for(cit=active_easy_handles.begin(); cit!=active_easy_handles.end(); ++cit){
                CURL *easy_handle = *cit;
                std::map<CURL*, Shard*>::iterator mit = shards_map.find(easy_handle);
                if(mit==shards_map.end()){
                    ostringstream oss;
                    oss << "OUCH! Failed to locate Shard instance for curl_easy_handle: " << (void*)(easy_handle)<<endl;
                    cerr << oss.str() << endl;
                    throw oss.str();
                }
                Shard *shard = mit->second;
                shard->close();
                curl_multi_remove_handle(curl_multi_handle, easy_handle);
                curl_easy_reset(easy_handle);

            }
            active_easy_handles.clear();
            shards_map.clear();
        }
    }
    if(active_easy_handles.size()){
        run_multi_perform(curl_multi_handle,&shards_map);
        // clean up
        std::vector<CURL*>::iterator cit;
        for(cit=active_easy_handles.begin(); cit!=active_easy_handles.end(); ++cit){
            CURL *easy_handle = *cit;
            std::map<CURL*, Shard*>::iterator mit = shards_map.find(easy_handle);
            if(mit==shards_map.end()){
                ostringstream oss;
                oss << "OUCH! Failed to locate Shard instance for curl_easy_handle: " << (void*)(easy_handle)<<endl;
                cerr << oss.str() << endl;
                throw oss.str();
            }
            Shard *shard = mit->second;
            shard->close();
            curl_multi_remove_handle(curl_multi_handle, easy_handle);
            curl_easy_reset(easy_handle);

        }
        active_easy_handles.clear();
        shards_map.clear();
    }

}










string usage(string prog_name) {
    ostringstream ss;
    ss << "usage: " << prog_name << "[-d] -o output -u <url> ]" << endl;
    return ss.str();
}

int main(int argc, char **argv) {
    string output_file;
    string url;
    long long file_size;
    unsigned int shard_count;
    bool reuse_curl_easy_handles;
    bool keep_alive;
    unsigned int max_easy_handles;


    url = "https://s3.amazonaws.com/opendap.test/MVI_1803.MOV";
    file_size = 1647477620;
    shard_count=10000;
    reuse_curl_easy_handles=false;
    keep_alive = false;
    max_easy_handles = 16;


    GetOpt getopt(argc, argv, "Ddkrc:s:o:u:m:");
    int option_char;
    while ((option_char = getopt()) != EOF) {
        switch (option_char) {
        case 'D':
            dry_run = true;
            break;
        case 'd':
            debug = true;
            break;
        case 'k':
            keep_alive = true;
            break;
        case 'r':
            reuse_curl_easy_handles = true;
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
        case 'm':
            std::istringstream(getopt.optarg) >> max_easy_handles;
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
        cerr << "reuse_easy_curl_handles: " << (reuse_curl_easy_handles?"true":"false") << endl;
        cerr << "keep_alive: " << (keep_alive?"true":"false") << endl;
        cerr << "max_easy_handles: " << (max_easy_handles?"true":"false") << endl;
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

    vector<Shard *> shards;
    make_shards(&shards,shard_count,url,file_size,output_file);

    if(reuse_curl_easy_handles){
        get_shards_reuse_curl_handles(&shards,max_easy_handles,keep_alive);
    }
    else {
        get_shards_no_curl_handle_reuse(&shards,max_easy_handles,keep_alive);
    }

#if 0
    map<CURL*, Shard *> shards_map;
    vector<CURL *> all_easy_handles;
    vector<CURL *> current_easy_handles;
    CURLM *curl_multi_handle = curl_multi_init();

    unsigned long int n = 0;
    std::vector<Shard*>::iterator sit;
    for(sit=shards.begin(); sit!=shards.end(); sit++, n++){
        // Set up the shard to be read .
        CURL* curl = curl_easy_init();
        Shard *shard = *sit;
        shard->open();
        groom_curl_handle(curl,shard,keep_alive);
        current_easy_handles.push_back(curl);
        shards_map.insert(std::pair<CURL*,Shard*>(curl,shard));
        // Add it to our current multi handle
        curl_multi_add_handle(curl_multi_handle, curl);

        /**
         * If it's time, pull the trigger and get the stuff.
         */
        if(n && !(n%max_easy_handles)){
            // get the stuff
            run_multi_perform(curl_multi_handle,&shards_map);
            // clean up
            std::vector<CURL*>::iterator cit;
            for(cit=current_easy_handles.begin(); cit!=current_easy_handles.end(); ++cit){
                CURL *easy_handle = *cit;
                std::map<CURL*, Shard*>::iterator mit = shards_map.find(easy_handle);
                if(mit==shards_map.end()){
                    ostringstream oss;
                    oss << "OUCH! Failed to locate Shard instance for curl_easy_handle: " << (void*)(easy_handle)<<endl;
                    cerr << oss.str() << endl;
                    throw oss.str();
                }
                Shard *shard = mit->second;
                shard->close();
                curl_multi_remove_handle(curl_multi_handle, easy_handle);
                curl_easy_cleanup(easy_handle);

            }
            current_easy_handles.clear();
            shards_map.clear();
            curl_multi_cleanup(curl_multi_handle);
            curl_multi_handle = curl_multi_init();
        }
    }
    if(current_easy_handles.size()){
        run_multi_perform(curl_multi_handle,&shards_map);
        // clean up
        std::vector<CURL*>::iterator cit;
        for(cit=current_easy_handles.begin(); cit!=current_easy_handles.end(); ++cit){
            CURL *easy_handle = *cit;
            curl_multi_remove_handle(curl_multi_handle, easy_handle);
            curl_easy_cleanup(easy_handle);
        }
        current_easy_handles.clear();
        curl_multi_cleanup(curl_multi_handle);
        // curl_multi_handle = curl_multi_init();
    }
#endif


    return 0;
}






