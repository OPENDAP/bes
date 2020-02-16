//
// Created by ndp on 2/15/20.
//

#ifndef HYRAX_GIT_CURL_UTILS_H
#define HYRAX_GIT_CURL_UTILS_H

#include <curl/curl.h>
#include <string>

namespace curl {

    std::string error_message(const CURLcode response_code, char *error_buf);

    std::string probe_easy_handle(CURL *c_handle);
\
}


#endif //HYRAX_GIT_CURL_UTILS_H
