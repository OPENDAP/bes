/**
 * @brief utility class for the HTTP catalog module
 *
 * This class provides utilities that extract information from a URL
 * or the returned headers of an HTTP response. It also provides
 * storage for a number of values read from the httpd_catalog.conf
 * configuration file.
 *
 * @note This class holds only static methods and fields. It has no
 * constructor or destructor. Use the initialize() method to configure
 * the various static fields based on the values of the BES configuration
 * file(s).
 */
#ifndef _REMOTE_UTILS_H_
#define _REMOTE_UTILS_H_ 1

#include <string>
#include <map>
#include <vector>

namespace http {

/** @brief utility class for the gateway remote request mechanism
 *
 */
    class BESRemoteUtils {
    public:
        static std::map<std::string, std::string> MimeList;
        static std::string ProxyProtocol;
        static std::string ProxyHost;
        static std::string ProxyUserPW;
        static std::string ProxyUser;
        static std::string ProxyPassword;
        static int ProxyPort;
        static int ProxyAuthType;
        static bool useInternalCache;

        static std::string NoProxyRegex;

        static void Initialize();

        static void Get_type_from_disposition(const std::string &disp, std::string &type);

        static void Get_type_from_content_type(const std::string &ctype, std::string &type);

        static void Get_type_from_url(const std::string &url, std::string &type);
    };

} // namespace http

#endif // _REMOTE_UTILS_H_

