//
// Created by ndp on 2/15/20.
//

#ifndef HYRAX_GIT_CURL_UTILS_H
#define HYRAX_GIT_CURL_UTILS_H

#include <curl/curl.h>
#include <string>
#include "../rapidjson/document.h"

namespace curl {

    void http_get(const std::string &url, char *response_buf);
    std::string http_get_as_string(const std::string &url);
    rapidjson::Document http_get_as_json(const std::string &target_url);
    std::string get_range_arg_string(const unsigned long long &offset, const unsigned long long &size);

    std::string error_message(const CURLcode response_code, char *error_buf);

    std::string probe_easy_handle(CURL *c_handle);

    CURL *set_up_easy_handle(const std::string &target_url, const std::string &cookies_file, char *response_buff);
}


#endif //HYRAX_GIT_CURL_UTILS_H
