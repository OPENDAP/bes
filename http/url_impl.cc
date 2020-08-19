
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

#define MODULE "http"
#define prolog string("url::").append(__func__).append("() - ")

#define PROTOCOL_KEY "http_url_protocol"
#define HOST_KEY  "http_url_host"
#define PATH_KEY  "http_url_path"
#define QUERY_KEY "http_url_query"
#define SOURCE_URL_KEY  "http_url_target_url"
#define INGEST_TIME_KEY  "http_url_ingest_time"

#define AMS_EXPIRES_HEADER_KEY "X-Amz-Expires"
#define AWS_DATE_HEADER_KEY "X-Amz-Date"
#define CLOUDFRONT_EXPIRES_HEADER_KEY "Expires"
#define REFRESH_THRESHOLD 3600

namespace http {

#if 0
/**
 *
 * @param kvp
 */
url::url(const map<string,string> &kvp)
{
    map<string,string> kvp_copy = kvp;
    map<string,string>::const_iterator it;
    map<string,string>::const_iterator itc;

    it = kvp.find(PROTOCOL_KEY);
    itc = kvp_copy.find(PROTOCOL_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_protocol = it->second;
        kvp_copy.erase(it->first);
        BESDEBUG(MODULE, prolog << "Located PROTOCOL_KEY(" << PROTOCOL_KEY << ") value: " << d_protocol << endl);
    }
    it = kvp.find(HOST_KEY);
    itc = kvp_copy.find(HOST_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_host = it->second;
        kvp_copy.erase(it->first);
        BESDEBUG(MODULE, prolog << "Located HOST_KEY(" << HOST_KEY << ") value: " << d_host << endl);
    }
    it = kvp.find(PATH_KEY);
    itc = kvp_copy.find(PATH_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_path = it->second;
        kvp_copy.erase(it->first);
        BESDEBUG(MODULE, prolog << "Located PATH_KEY(" << PATH_KEY << ") value: " << d_path << endl);
    }
    it = kvp.find(QUERY_KEY);
    itc = kvp_copy.find(QUERY_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_query = it->second;
        kvp_copy.erase(it->first);
        BESDEBUG(MODULE, prolog << "Located QUERY_KEY(" << QUERY_KEY << ") value: " << d_query << endl);
    }
    it = kvp.find(SOURCE_URL_KEY);
    itc = kvp_copy.find(SOURCE_URL_KEY);
    if(it != kvp.end() && itc != kvp_copy.end()){
        d_source_url = it->second;
        kvp_copy.erase(it->first);
        BESDEBUG(MODULE, prolog << "Located SOURCE_URL_KEY(" << SOURCE_URL_KEY << ") value: " << d_source_url << endl);
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
#endif

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
 * @param source_url
 */
void url::parse(const string &source_url) {
    const string prot_end("://");
    string::const_iterator prot_i = search(source_url.begin(), source_url.end(),
                                           prot_end.begin(), prot_end.end());
    d_protocol.reserve(distance(source_url.begin(), prot_i));
    transform(source_url.begin(), prot_i,
              back_inserter(d_protocol),
              ptr_fun<int, int>(tolower)); // protocol is icase
    if (prot_i == source_url.end())
        return;
    advance(prot_i, prot_end.length());
    string::const_iterator path_i = find(prot_i, source_url.end(), '/');
    d_host.reserve(distance(prot_i, path_i));
    transform(prot_i, path_i,
              back_inserter(d_host),
              ptr_fun<int, int>(tolower)); // host is icase
    string::const_iterator query_i = find(path_i, source_url.end(), '?');
    d_path.assign(path_i, query_i);
    if (query_i != source_url.end())
        ++query_i;
    d_query.assign(query_i, source_url.end());


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

#if 0

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
#endif

/**
 *
 * @return True if the URL appears within the REFRESH_THRESHOLD of the
 * expires time read from one of CLOUDFRONT_EXPIRES_HEADER_KEY, AMS_EXPIRES_HEADER_KEY;
 *
 */
bool url::is_expired()
{
    bool is_expired;
    time_t now;
    time(&now);  /* get current time; same as: timer = time(NULL)  */
    BESDEBUG(MODULE, prolog << "now: " << now << endl);

    time_t expires = now;
    string cf_expires = query_parameter_value(CLOUDFRONT_EXPIRES_HEADER_KEY);
    string aws_expires = query_parameter_value(AMS_EXPIRES_HEADER_KEY);

    if(!cf_expires.empty()){ // CloudFront expires header?
        expires = stoll(cf_expires);
        BESDEBUG(MODULE, prolog << "Using "<< CLOUDFRONT_EXPIRES_HEADER_KEY << ": " << expires << endl);
    }
    else if(!aws_expires.empty()){
        // AWS Expires header?
        //
        // By default we'll use the time we made the URL object, ingest_time
        time_t start_time = ingest_time();
        // But if there's an AWS Date we'll parse that and compute the time
        // @TODO move to NgapApi::decompose_url() and add the result to the map
        string aws_date = query_parameter_value(AWS_DATE_HEADER_KEY);
        if(!aws_date.empty()){
            string date = aws_date; // 20200624T175046Z
            string year = date.substr(0,4);
            string month = date.substr(4,2);
            string day = date.substr(6,2);
            string hour = date.substr(9,2);
            string minute = date.substr(11,2);
            string second = date.substr(13,2);

            BESDEBUG(MODULE, prolog << "date: "<< date <<
                                    " year: " << year << " month: " << month << " day: " << day <<
                                    " hour: " << hour << " minute: " << minute  << " second: " << second << endl);

            struct tm *ti = gmtime(&now);
            ti->tm_year = stoll(year) - 1900;
            ti->tm_mon = stoll(month) - 1;
            ti->tm_mday = stoll(day);
            ti->tm_hour = stoll(hour);
            ti->tm_min = stoll(minute);
            ti->tm_sec = stoll(second);

            BESDEBUG(MODULE, prolog << "ti->tm_year: "<< ti->tm_year <<
                                    " ti->tm_mon: " << ti->tm_mon <<
                                    " ti->tm_mday: " << ti->tm_mday <<
                                    " ti->tm_hour: " << ti->tm_hour <<
                                    " ti->tm_min: " << ti->tm_min <<
                                    " ti->tm_sec: " << ti->tm_sec << endl);


            start_time = mktime(ti);
            BESDEBUG(MODULE, prolog << "AWS (computed) start_time: "<< start_time << endl);
        }
        expires = start_time + stoll(aws_expires);
        BESDEBUG(MODULE, prolog << "Using "<< AMS_EXPIRES_HEADER_KEY << ": " << aws_expires <<
                                " (expires: " << expires << ")" << endl);
    }
    time_t remaining = expires - now;
    BESDEBUG(MODULE, prolog << "expires: " << expires <<
                            "  remaining: " << remaining <<
                            " threshold: " << REFRESH_THRESHOLD << endl);

    is_expired = remaining < REFRESH_THRESHOLD;
    BESDEBUG(MODULE, prolog << "is_expired: " << (is_expired?"true":"false") << endl);

    return is_expired;
}


string url::to_string(){
    stringstream ss;
    string indent_inc = "  ";
    string indent = indent_inc;

    ss << "http::url [" << this << "] " << endl;
    ss << indent << "d_source_url: " << d_source_url << endl;
    ss << indent << "d_protocol:   " << d_protocol << endl;
    ss << indent << "d_host:       " << d_host << endl;
    ss << indent << "d_path:       " << d_path << endl;
    ss << indent << "d_query:      " << d_query << endl;

    std::map<std::string, std::vector<std::string>* >::iterator it;

    string idt = indent+indent_inc;
    for(it=d_query_kvp.begin(); it !=d_query_kvp.end(); it++){
        ss << indent << "d_query_kvp["<<it->first<<"]: " << endl;
        std::vector<std::string> *values = it->second;
        for(size_t i=0; i<values->size(); i++){
            ss << idt << "value[" << i << "]: " << (*values)[i] << endl;
        }
    }
    ss << indent << "d_ingest_time:      " << d_ingest_time << endl;
    return ss.str();
}


} // namespace http