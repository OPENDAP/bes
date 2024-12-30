// BESServerHandler.cc

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

#include "config.h"

#include <unistd.h>    // for getpid fork sleep
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>  // for waitpid
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <sstream>
#include <iostream>
#include <map>

#include "BESServerHandler.h"
#include "Connection.h"
#include "Socket.h"
#include "BESXMLInterface.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"
#include "ServerExitConditions.h"
#include "BESUtil.h"
#include "PPTStreamBuf.h"
#include "PPTProtocolNames.h"
#include "BESLog.h"
#include "BESDebug.h"
#include "BESStopWatch.h"

using namespace std;

#define MODULE "server"
#define prolog std::string("BESServerHandler::").append(__func__).append("() - ")

// Default is to not exit on internal error. A bad idea, but the original
// behavior of the server. jhrg 10/4/18
#define EXIT_ON_INTERNAL_ERROR "BES.ExitOnInternalError"

BESServerHandler::BESServerHandler()
{
    bool found = false;
    try {
        TheBESKeys::TheKeys()->get_value("BES.ProcessManagerMethod", _method, found);
    }
    catch (BESError &e) {
        cerr << "Unable to determine method to handle clients, "
            << "single or multiple as defined by BES.ProcessManagerMethod" << ": " << e.get_message() << endl;
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }

    if (_method != "multiple" && _method != "single") {
        cerr << "Unable to determine method to handle clients, "
            << "single or multiple as defined by BES.ProcessManagerMethod" << endl;
        exit(SERVER_EXIT_FATAL_CANNOT_START);
    }
}

// I'm not sure that we need to fork twice. jhrg 11/14/05
// The reason that we fork twice is explained in Advanced Programming in the
// Unit Environment by W. Richard Stevens. In the 'multiple' case we don't
// want to leave any zombie processes.
//
// I've changed this substantially. See daemon.cc, ServerApp.cc and
// DaemonCommandHanlder.cc. jhrg 7/15/11
void BESServerHandler::handle(Connection *c)
{
    if (_method == "single") {
        // we're in single mode, so no for and exec is needed. One
        // client connection and we are done.
        execute(c);
    }
    // _method is "multiple" which means, for each connection request, make a
    // new beslistener daemon. The OLFS can send many commands to each of these
    // before it closes the socket. In theory this should not be necessary, but
    // in practice some handlers will have resource (memory) leaks and nothing
    // cures that like exit().
    else {
        pid_t pid;
        if ((pid = fork()) < 0) { // error
            string error("fork error");
            const char* error_info = strerror(errno);
            if (error_info) error += " " + (string) error_info;
            throw BESInternalError(error, __FILE__, __LINE__);
        }
        else if (pid == 0) { // child
            execute(c);
        }
    }
}

void BESServerHandler::execute(Connection *connection)
{
    BESLog::TheLog()->update_pid();

    // TODO This seems like a waste of time - do we really need to log this information?
    // jhrg 11/13/17
    ostringstream strm;
    strm << "ip " << connection->getSocket()->getIp() << ", port " << connection->getSocket()->getPort();
    string from = strm.str();

    map<string, string> extensions;

    int socket_d = connection->getSocket()->getSocketDescriptor();
    unsigned int bufsize = connection->getSendChunkSize();
    PPTStreamBuf fds(socket_d, bufsize);
    ostream my_ostrm(&fds);

#if !NDEBUG
    stringstream msg;
    msg << prolog << "Using ostream: " << (void *) &my_ostrm << " cout: " << (void *) &cout << endl;
    BESDEBUG(MODULE,  msg.str());
#endif

    // we loop continuously waiting for messages. The only way we exit
    // this loop is: 1. we receive a status of exit from the client, 2.
    // the client drops the connection, the process catches the signal
    // and exits, 3. a fatal error has occurred in the server so exit,
    // 4. the server process is killed.
    for (;;) {
        ostringstream ss;

        bool done = false;
        BESDEBUG(MODULE,prolog << "Waiting for client to send commands." << endl);
        while (!done)
            done = connection->receive(extensions, &ss);

        BESDEBUG(MODULE,prolog << "Received client command. status: '" << extensions["status"] << "'" << endl);

        // The server has been sent a message that the client is exiting
        // and closing the connection. So exit this process.
        if (extensions["status"] == connection->exit()) {
            // The protocol docs indicate that the EXIT_NOW 'token' is followed
            // by a zero-length chunk (a chunk that has type 'd'). See section
            // 4.3 of the documentation (http://docs.opendap.org/index.php/Hyrax_-_BES_PPT).
            // jhrg 10/30/13
            // Note, however, that PPTConnection::receive() continues to read chunks until
            // the end chunk is read. That means that it will read the end chunk for the
            // PPT_EXIT_NOW chunk (and so we don't need to).

            BESDEBUG(MODULE,prolog << "Received PPT_EXIT_NOW in an extension chunk." << endl);

            // This call closes the socket - it does minimal bookkeeping and
            // calls the the kernel's close() function. NB: The method is
            // implemented in PPTServer.cc and that calls Socket::close() on the
            // Socket instance held by the Connection.
            connection->closeConnection();

            BESDEBUG(MODULE,prolog << "Calling exit(CHILD_SUBPROCESS_READY) which has a value of " << CHILD_SUBPROCESS_READY << endl);

            INFO_LOG("Received exit command.");

            exit(CHILD_SUBPROCESS_READY);
        }

        string cmd_str = ss.str();

        BESDEBUG(MODULE, prolog << "Processing client command:" << endl << cmd_str << endl);

        BESStopWatch sw;
        if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start("BESServerHandler::execute");


        // This is where we actually save/assign the output stream used for the response
        BESXMLInterface cmd(cmd_str, &my_ostrm);

        int status = cmd.execute_request(from);
        if (status == 0) {
            cmd.finish(status);
            fds.finish();
            BESDEBUG(MODULE, prolog << "Client command successfully processed." << endl);
        }
        else {
            BESDEBUG(MODULE, prolog << "ERROR - cmd.execute_request() returned: " << status << endl);

            // Send the extension status=error to the client so that it can reset. The finish()
            // method is called _after_ this so that the error response will be recognizable.
            // At least, I think that's what is happening... jhrg 11/12/17
            map<string, string> extensions;
            extensions["status"] = "error";
            if (status == BES_INTERNAL_FATAL_ERROR) {
                extensions["exit"] = "true";
            }
            connection->sendExtensions(extensions);

            cmd.finish(status);
            // we are finished, send the last chunk
            fds.finish();

            // If the status is fatal, then we want to exit. Otherwise,
            // continue, wait for the next request.
            switch (status) {
            case BES_INTERNAL_FATAL_ERROR:
                ERROR_LOG("BES Internal Fatal Error; child returning "
                    + std::to_string(SERVER_EXIT_ABNORMAL_TERMINATION) + " to the master listener.");

                connection->closeConnection();
                exit(SERVER_EXIT_ABNORMAL_TERMINATION);

                break;

            case BES_INTERNAL_ERROR:
                // This should be an option. The default is to not exit. jhrg 10/4/18
                if (TheBESKeys::TheKeys()->read_bool_key(EXIT_ON_INTERNAL_ERROR, false)) {
                    ERROR_LOG("BES Internal Error; child returning "
                        + std::to_string(SERVER_EXIT_ABNORMAL_TERMINATION) + " to the master listener.");

                    connection->closeConnection();
                    exit(SERVER_EXIT_ABNORMAL_TERMINATION);
                }
                break;

                // These are recoverable errors
            case BES_SYNTAX_USER_ERROR:
            case BES_FORBIDDEN_ERROR:
            case BES_NOT_FOUND_ERROR:
            default:
                break;
            }
        }
    }	// This is the end of the infinite loop that processes commands.
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESServerHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESServerHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "server method: " << _method << endl;
    BESIndent::UnIndent();
}

