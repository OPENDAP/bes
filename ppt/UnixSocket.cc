// UnixSocket.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>
//      szednik     Stephan Zednik <zednik@ucar.edu>

#include <unistd.h>   // for unlink
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdio>
#include <cerrno>
#include <cstring>

#include "UnixSocket.h"
#include "BESInternalError.h"
#include "SocketUtilities.h"

using std::endl;
using std::ostream;
using std::string;

void UnixSocket::connect()
{
    if (_listening) {
        string err("Socket is already listening");
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    if (_connected) {
        string err("Socket is already connected");
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    struct sockaddr_un client_addr;
    struct sockaddr_un server_addr;

    // what is the max size of the path to the unix socket
    unsigned int max_len = sizeof(client_addr.sun_path);

    char path[107] = "";
    getcwd(path, sizeof(path));
    _tempSocket = path;
    _tempSocket += "/";
    _tempSocket += SocketUtilities::create_temp_name();
    _tempSocket += ".unixSocket";
    // maximum path for struct sockaddr_un.sun_path is 108
    // get sure we will not exceed to max for creating sockets
    // 107 characters in pathname + '\0'
    if (_tempSocket.size() > max_len - 1) {
        string msg = "path to temporary unix socket ";
        msg += _tempSocket + " is too long";
        throw(BESInternalError(msg, __FILE__, __LINE__));
    }
    if (_unixSocket.size() > max_len - 1) {
        string msg = "path to unix socket ";
        msg += _unixSocket + " is too long";
        throw(BESInternalError(msg, __FILE__, __LINE__));
    }

    strncpy(server_addr.sun_path, _unixSocket.c_str(), _unixSocket.size());
    server_addr.sun_path[_unixSocket.size()] = '\0';
    server_addr.sun_family = AF_UNIX;

    int descript = socket( AF_UNIX, SOCK_STREAM, 0);
    if (descript != -1) {
        strncpy(client_addr.sun_path, _tempSocket.c_str(), _tempSocket.size());
        client_addr.sun_path[_tempSocket.size()] = '\0';
        client_addr.sun_family = AF_UNIX;

        int clen = sizeof(client_addr.sun_family);
        clen += strlen(client_addr.sun_path) + 1;

        if (bind(descript, (struct sockaddr*) &client_addr, clen + 1) != -1) {
            int slen = sizeof(server_addr.sun_family);
            slen += strlen(server_addr.sun_path) + 1;

            // we aren't setting the send and receive buffer sizes for a
            // unix socket. These will default to a set value

            if (::connect(descript, (struct sockaddr*) &server_addr, slen) != -1) {
                _socket = descript;
                _connected = true;
            }
            else {
                ::close(descript);
                string msg = "could not connect via ";
                msg += _unixSocket;
                char *err = strerror( errno);
                if (err)
                    msg = msg + "\n" + err;
                else
                    msg = msg + "\nCould not retrieve error message";
                throw BESInternalError(msg, __FILE__, __LINE__);
            }
        }
        else {
            ::close(descript);
            string msg = "could not bind to Unix socket ";
            msg += _tempSocket;
            char *err = strerror( errno);
            if (err)
                msg = msg + "\n" + err;
            else
                msg = msg + "\nCould not retrieve error message";
            throw BESInternalError(msg, __FILE__, __LINE__);
        }
    }
    else {
        string msg = "could not create a Unix socket";
        char *err = strerror( errno);
        if (err)
            msg = msg + "\n" + err;
        else
            msg = msg + "\nCould not retrieve error message";
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}

void UnixSocket::listen()
{
    if (_connected) {
        string err("Socket is already connected");
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    if (_listening) {
        string err("Socket is already listening");
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    int on = 1;
    static struct sockaddr_un server_add;
    _socket = socket( AF_UNIX, SOCK_STREAM, 0);
    if (_socket >= 0) {
        server_add.sun_family = AF_UNIX;
        // Changed the call below to strncpy; sockaddr_un.sun_path is a char[104]
        // on OS/X. jhrg 5/26/06
        strncpy(server_add.sun_path, _unixSocket.c_str(), 103);
        server_add.sun_path[103] = '\0';

        (void) unlink(_unixSocket.c_str());
        if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on))) {
            string error("could not set SO_REUSEADDR on Unix socket");
            const char *error_info = strerror( errno);
            if (error_info) error += " " + (string) error_info;
            throw BESInternalError(error, __FILE__, __LINE__);
        }

        // we aren't setting the send and receive buffer sizes for a unix
        // socket. These will default to a set value

        // Added a +1 to the size computation. jhrg 5/26/05
        if (bind(_socket, (struct sockaddr*) &server_add,
            sizeof(server_add.sun_family) + strlen(server_add.sun_path) + 1) != -1) {
            if (::listen(_socket, 5) == 0) {
                _listening = true;
            }
            else {
                string error("could not listen Unix socket");
                const char* error_info = strerror( errno);
                if (error_info) error += " " + (string) error_info;
                throw BESInternalError(error, __FILE__, __LINE__);
            }
        }
        else {
            string error("could not bind Unix socket");
            const char* error_info = strerror( errno);
            if (error_info) error += " " + (string) error_info;
            throw BESInternalError(error, __FILE__, __LINE__);
        }
    }
    else {
        string error("could not get Unix socket");
        const char *error_info = strerror( errno);
        if (error_info) error += " " + (string) error_info;
        throw BESInternalError(error, __FILE__, __LINE__);
    }
}

void UnixSocket::close()
{
    Socket::close();
    if (_tempSocket != "") {
	(void) remove(_tempSocket.c_str());
        _connected = false;
    }
    if (_listening && _unixSocket != "") {
	(void) remove(_unixSocket.c_str());
        _listening = false;
    }
}

/** @brief is there any wrapper code for unix sockets
 *
 */
bool UnixSocket::allowConnection()
{
    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void UnixSocket::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "UnixSocket::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "unix socket: " << _unixSocket << endl;
    strm << BESIndent::LMarg << "temp socket: " << _tempSocket << endl;
    Socket::dump(strm);
    BESIndent::UnIndent();
}

