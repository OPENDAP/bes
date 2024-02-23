//
// Created by ndp on 2/22/24.
//

#ifndef BES_RESPONSE_H
#define BES_RESPONSE_H

#include <string>
#include <vector>

#include <curl/curl.h>

namespace http {

class Response {
    CURLcode d_curl_code = CURLE_OK;
    unsigned int d_http_status = 0;
    std::string d_origin_url;
    std::string d_redirect_url;
    std::vector<std::string> d_headers;
    std::string d_body;

public:

    Response() = default;
    ~Response() = default;

    Response(CURLcode curl_code,
             unsigned int http_status,
             std::string origin_url,
             std::string redirect_url,
             std::vector<std::string> response_headers,
             std::string response_body
             ) :
             d_curl_code(curl_code),
             d_http_status(http_status),
             d_origin_url(std::move(origin_url)),
             d_redirect_url(std::move(redirect_url)),
             d_headers(std::move(response_headers)),
             d_body(std::move(response_body))
             {}

    Response(const Response &r) = default;
    Response(Response &&r) = default;

    Response &operator=(const Response &r) = default;
    Response &operator=(Response &&r) = default;

    void curl_code(CURLcode code)  {  d_curl_code = code; }
    CURLcode curl_code() const { return d_curl_code; }

    void status(unsigned int status)  { d_http_status = status; }
    unsigned int status() const { return d_http_status; }

    void origin_url(std::string url) { d_origin_url = std::move(url); }
    std::string origin_url() const { return d_origin_url; }

    std::string redirect_url(std::string url)  { d_redirect_url = std::move(url); }
    std::string redirect_url() const { return d_redirect_url; }

    void headers(std::vector<std::string> hdrs)  {  d_headers = std::move(hdrs); }
    std::vector<std::string> &headers() { return d_headers; }

    void body(std::string &response_body)  {  d_body = response_body; }
    std::string &body() { return d_body; }

    void write_response_details(std:: stringstream &msg) const;

    std::string dump() const;
};

} // http

#endif //BES_RESPONSE_H
