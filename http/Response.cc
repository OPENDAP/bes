//
// Created by ndp on 2/22/24.
//

#include "config.h"

#include <string>
#include <vector>
#include <sstream>


#include "Response.h"

#include "BESError.h"

namespace http {

/**
 *
 * @param http_status
 * @param response_headers
 * @param response_body
 * @param msg
 */
void Response::write_response_details(std:: stringstream &msg) const
{
    msg << "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
    msg << "# http::Response Details\n";
    msg << "# origin_url: " << d_origin_url << "\n";
    msg << "# curl_code: " << d_curl_code << "\n";
    msg << "# http_status: " << d_http_status << "\n";
    msg << "# last accessed url: " << d_redirect_url << "\n";
    msg << "# Response Headers -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
    for (const auto &hdr: d_headers) {
        msg << "  "<<  hdr << "\n";
    }
    msg << "# BEGIN Response Body -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
    msg << d_body << "\n";
    msg << "# END Response Body   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n";
}



std::string Response::dump( ) const
{
    std::stringstream msg;

    msg << BESIndent::LMarg << "http::Response::dump - (" << (void *)this << ")\n";
    BESIndent::Indent() ;
    msg << BESIndent::LMarg << "        curl_code: " << d_curl_code << "\n" ;
    msg << BESIndent::LMarg << "      http_status: " << d_http_status << "\n" ;
    msg << BESIndent::LMarg << "       origin_url: " << d_origin_url << "\n" ;
    msg << BESIndent::LMarg << "last_accessed_url: " << d_redirect_url << "\n" ;

    for(const auto &hdr: d_headers) {
        msg << BESIndent::LMarg << "  response_header: " << hdr << "\n";
    }

    msg << BESIndent::LMarg << "    response_body: " << d_body << "\n" ;
    BESIndent::UnIndent() ;

    return msg.str();
}






} // http