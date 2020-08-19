
// https://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform

#ifndef URL_HH_
#define URL_HH_
#include <string>
#include <map>
#include <time.h>


namespace http {



    class  url {
private:
    void parse(const std::string &source_url);

    std::string d_source_url;
    std::string d_protocol;
    std::string d_host;
    std::string d_path;
    std::string d_query;
    std::map<std::string, std::vector<std::string>* > d_query_kvp;
    time_t d_ingest_time;

public:

    // omitted copy, ==, accessors, ...
    explicit url(const std::string &url_s):d_source_url(url_s) {
        parse(url_s);
    }

    url(const std::map<std::string,std::string> &kvp);

    ~url();
    std::string str() { return d_source_url; }

    std::string protocol() const { return d_protocol; }

    std::string host() const { return d_host; }

    std::string path() const { return d_path; }

    std::string query() const { return d_query; }

    time_t ingest_time() const { return d_ingest_time; }

    void set_ingest_time(const time_t itime){
        d_ingest_time = itime;
    }

    std::string query_parameter_value(const std::string &key) const;
    void query_parameter_values(const std::string &key, std::vector<std::string> &values) const;

    void kvp(std::map<std::string,std::string> &kvp);

    bool is_expired();

    std::string to_string();

};

} // namespace http
#endif /* URL_HH_ */