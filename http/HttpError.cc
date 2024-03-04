//
// Created by ndp on 2/9/24.
//

#include "HttpError.h"

namespace http {


std::string HttpError::dump( ) const
{
    std::stringstream msg;

    msg << BESIndent::LMarg << "http::Response::dump - (" << (void *)this << ")\n";
    BESIndent::Indent() ;
    msg << BESIndent::LMarg << "        curl_code: " << d_curl_code << "\n" ;
    msg << BESIndent::LMarg << "      http_status: " << d_http_status << "\n" ;
    msg << BESIndent::LMarg << "       origin_url: " << d_origin_url << "\n" ;
    msg << BESIndent::LMarg << "last_accessed_url: " << d_redirect_url << "\n" ;

    if(!d_response_headers.empty()) {
        msg << BESIndent::LMarg << " response_headers: " << d_response_headers.size() << "\n";
        for (const auto &hdr: d_response_headers) {
            msg << BESIndent::LMarg << "  response_header: " << hdr << "\n";
        }
    }
    else {
        msg << BESIndent::LMarg << " response_headers: [NONE]\n";
    }

    if(!d_response_body.empty()) {
        msg << BESIndent::LMarg << "    response_body: " << d_response_body << "\n";
    }
    else {
        msg << BESIndent::LMarg << "    response_body: [NONE]\n";
    }
    BESIndent::UnIndent() ;

    return msg.str();
}

} // http