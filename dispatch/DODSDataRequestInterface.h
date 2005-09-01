// DODSDataRequestInterface.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSDataRequestInterface_h_
#define DODSDataRequestInterface_h_ 1

/** @brief Structure storing information from the Apache module
 */

typedef struct _DODSDataRequestInterface
{
    /** @brief name of server running Apache server
     */
    const char *server_name;
    /** @brief not used
     */
    const char *server_address;
    /** @brief protocol of the request, such as "HTTP/0.9" or "HTTP/1.1"
     */
    const char *server_protocol;
    /** @brief TCP port number where the server running Apache is listening
     */
    const char *server_port;
    /** @brief uri of the request
     */
    const char *script_name;
    /** @brief remote ip address of client machine
     */
    const char *user_address;
    /** @brief information about the user agent originating the request, e.g. Mozilla/4.04 (X11; I; SunOS 5.4 sun4m)
     */
    const char *user_agent;
    /** @brief OpenDAP request string from URL
     */
    const char *request;
    /** @brief server cookies set in users browser
     */
    const char *cookie;
} DODSDataRequestInterface;

#endif // DODSDataRequestInterface_h_

// $Log: DODSDataRequestInterface.h,v $
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
