
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

#include "url_parser.h"

using namespace std;
#define MODULE "foo"

namespace AWSV4 {
#define prolog string("url_parser::").append(__func__).append("() - ")

url_parser::~url_parser()
{
    if(!kvp_.empty()){
        map<string, vector<string>* >::const_iterator it;
        for(it = kvp_.begin() ; it != kvp_.end(); it++){
            delete it->second;
        }
    }


}

void url_parser::parse(const string &url_s) {
    const string prot_end("://");
    string::const_iterator prot_i = search(url_s.begin(), url_s.end(),
                                           prot_end.begin(), prot_end.end());
    protocol_.reserve(distance(url_s.begin(), prot_i));
    transform(url_s.begin(), prot_i,
              back_inserter(protocol_),
              ptr_fun<int, int>(tolower)); // protocol is icase
    if (prot_i == url_s.end())
        return;
    advance(prot_i, prot_end.size());
    string::const_iterator path_i = find(prot_i, url_s.end(), '/');
    host_.reserve(distance(prot_i, path_i));
    transform(prot_i, path_i,
              back_inserter(host_),
              ptr_fun<int, int>(tolower)); // host is icase
    string::const_iterator query_i = find(path_i, url_s.end(), '?');
    path_.assign(path_i, query_i);
    if (query_i != url_s.end())
        ++query_i;
    query_.assign(query_i, url_s.end());


    if(!query_.empty()){
        vector<string> records;
        string delimiters = "&";
        BESUtil::tokenize(query_,records, delimiters);
        vector<string>::iterator i = records.begin();
        for(; i!=records.end(); i++){
            size_t index = i->find('=');
            if(index != string::npos) {
                string key = i->substr(0, index);
                string value = i->substr(index+1);
                BESDEBUG(MODULE, prolog << "key: " << key << " value: " << value << endl);
                map<string, vector<string>* >::const_iterator record_it;
                record_it = kvp_.find(key);
                if(record_it != kvp_.end()){
                    vector<string> *values = record_it->second;
                    values->push_back(value);
                }
                else {
                    vector<string> *values = new vector<string>();
                    values->push_back(value);
                    kvp_.insert(pair<string, vector<string>*>(key,values));
                }
            }
        }
    }
    time(&d_ingest_time);  /* get current time; same as: timer = time(NULL)  */
}

/**
 * Looks at the query string KVP and returns the first value for the specified key
 * other values are ignored.
 * @param key
 * @return
 */
std::string url_parser::query_parameter_value(const std::string &key)
{
    string value;
    map<string, vector<string>* >::const_iterator it;
    it = kvp_.find(key);
    if(it != kvp_.end()){
        vector<string> *values = it->second;
        if(!values->empty()){
            value = (*values)[0];
        }
    }
    return value;
}

/**
 * Evaluates the query string KVP and adds all values for the passed key to the values vector.
 * @param key The key string to search the for in the query string.
 * @param values The vector into which ot place the values associated with key.
 */
void url_parser::query_parameter_values(const std::string &key, std::vector<std::string> &values)
{
    map<string, vector<string>* >::const_iterator it;
    it = kvp_.find(key);
    if(it != kvp_.end()){
        values = *it->second;
    }
}


} // namespace AWSV4