
// https://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <functional>
#include <time.h>

#include "BESDebug.h"
#include "BESUtil.h"
#include "HttpNames.h"

#include "url_impl.h"

using namespace std;
#define MODULE "foo"

namespace http {

const  std::string url::PROTOCOL_KEY = "url::protocol";
const  std::string url::HOST_KEY = "url::host";
const  std::string url::PATH_KEY = "url::path";
const  std::string url::QUERY_KEY = "url::query";
const  std::string url::SOURCE_URL_KEY = "url::target_url";
const  std::string url::INGEST_TIME_KEY = "url::ingest_time";

#define prolog string("url_parser::").append(__func__).append("() - ")



url::url(const map<string,string> &kvp)
{
    map<string,string> kvp_copy = kvp;
    map<string,string>::const_iterator it;
    map<string,string>::const_iterator itc;

    it = kvp.find(PROTOCOL_KEY);
    itc = kvp_copy.find(PROTOCOL_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_protocol = it->second;
        kvp_copy.erase(itc);
    }
    it = kvp.find(HOST_KEY);
    itc = kvp_copy.find(HOST_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_host = it->second;
        kvp_copy.erase(itc);
    }
    it = kvp.find(PATH_KEY);
    itc = kvp_copy.find(PATH_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_path = it->second;
        kvp_copy.erase(itc);
    }
    it = kvp.find(QUERY_KEY);
    itc = kvp_copy.find(QUERY_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_query = it->second;
        kvp_copy.erase(itc);
    }
    it = kvp.find(SOURCE_URL_KEY);
    itc = kvp_copy.find(SOURCE_URL_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_source_url = it->second;
        kvp_copy.erase(itc);
    }

    for(itc = kvp_copy.begin(); itc != kvp_copy.end(); itc++){
        string key =  itc->first;
        string value = itc->second;
        map<string, vector<string>* >::const_iterator record_it;
        record_it = d_query_kvp.find(key);
        if(record_it != d_query_kvp.end()){
            vector<string> *values = record_it->second;
            values->push_back(value);
        }
        else {
            vector<string> *values = new vector<string>();
            values->push_back(value);
            d_query_kvp.insert(pair<string, vector<string>*>(key, values));
        }
    }

}

/**
 *
 */
url::~url()
{
    if(!d_query_kvp.empty()){
        map<string, vector<string>* >::const_iterator it;
        for(it = d_query_kvp.begin() ; it != d_query_kvp.end(); it++){
            delete it->second;
        }
    }
}


/**
 *
 * @param url_s
 */
void url::parse(const string &url_s) {
    const string prot_end("://");
    string::const_iterator prot_i = search(url_s.begin(), url_s.end(),
                                           prot_end.begin(), prot_end.end());
    d_protocol.reserve(distance(url_s.begin(), prot_i));
    transform(url_s.begin(), prot_i,
              back_inserter(d_protocol),
              ptr_fun<int, int>(tolower)); // protocol is icase
    if (prot_i == url_s.end())
        return;
    advance(prot_i, prot_end.length());
    string::const_iterator path_i = find(prot_i, url_s.end(), '/');
    d_host.reserve(distance(prot_i, path_i));
    transform(prot_i, path_i,
              back_inserter(d_host),
              ptr_fun<int, int>(tolower)); // host is icase
    string::const_iterator query_i = find(path_i, url_s.end(), '?');
    d_path.assign(path_i, query_i);
    if (query_i != url_s.end())
        ++query_i;
    d_query.assign(query_i, url_s.end());


    if(!d_query.empty()){
        vector<string> records;
        string delimiters = "&";
        BESUtil::tokenize(d_query, records, delimiters);
        vector<string>::iterator i = records.begin();
        for(; i!=records.end(); i++){
            size_t index = i->find('=');
            if(index != string::npos) {
                string key = i->substr(0, index);
                string value = i->substr(index+1);
                BESDEBUG(MODULE, prolog << "key: " << key << " value: " << value << endl);
                map<string, vector<string>* >::const_iterator record_it;
                record_it = d_query_kvp.find(key);
                if(record_it != d_query_kvp.end()){
                    vector<string> *values = record_it->second;
                    values->push_back(value);
                }
                else {
                    vector<string> *values = new vector<string>();
                    values->push_back(value);
                    d_query_kvp.insert(pair<string, vector<string>*>(key, values));
                }
            }
        }
    }
    time(&d_ingest_time);
}

/**
 *
 * @param key
 * @return
 */
string url::query_parameter_value(const string &key) const
{
    string value;
    map<string, vector<string>* >::const_iterator it;
    it = d_query_kvp.find(key);
    if(it != d_query_kvp.end()){
        vector<string> *values = it->second;
        if(!values->empty()){
            value = (*values)[0];
        }
    }
    return value;
}

/**
 *
 * @param key
 * @param values
 */
void url::query_parameter_values(const string &key, vector<string> &values) const
{
    map<string, vector<string>* >::const_iterator it;
    it = d_query_kvp.find(key);
    if(it != d_query_kvp.end()){
        values = *it->second;
    }
}


/**
 *
 * @param kvp
 */
void url::kvp(map<string,string>  &kvp){
    stringstream ss;

    // Do the basic stuff
    kvp.insert(pair<string,string>(PROTOCOL_KEY, d_protocol));
    kvp.insert(pair<string,string>(HOST_KEY, d_host));
    kvp.insert(pair<string,string>(PATH_KEY, d_path));
    kvp.insert(pair<string,string>(QUERY_KEY, d_query));
    kvp.insert(pair<string,string>(SOURCE_URL_KEY, d_source_url));
    ss << d_ingest_time;
    kvp.insert(pair<string,string>(INGEST_TIME_KEY,ss.str()));

    // Now grab the query string. Only the first value of multi valued keys is used.
    map<string, vector<string>* >::const_iterator it;
    for(it=d_query_kvp.begin(); it != d_query_kvp.end(); it++){
        kvp.insert(pair<string,string>(it->first,(*it->second)[0]));
    }
}


} // namespace http