// ServerApp.cc

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <signal.h>
#include <sys/wait.h> // for wait
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::ostringstream;
using std::ofstream;

#include "config.h"

#include "ServerApp.h"
#include "ServerExitConditions.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "SocketListener.h"
#include "TcpSocket.h"
#include "UnixSocket.h"
#include "BESServerHandler.h"
#include "BESError.h"
#include "PPTServer.h"
#include "BESMemoryManager.h"
#include "BESDebug.h"
#include "BESCatalogUtils.h"
#include "BESServerUtils.h"

#include "BESDefaultModule.h"
#include "BESXMLDefaultCommands.h"
#include "BESDaemonConstants.h"

static int session_id = 0;

ServerApp::ServerApp() :
    BESModuleApp(), _portVal(0), _gotPort(false), _unixSocket(""), _secure(false), _mypid(0), _ts(0), _us(0), _ps(0)
{
    _mypid = getpid();
}

ServerApp::~ServerApp()
{
    delete TheBESKeys::TheKeys();

	BESCatalogUtils::delete_all_catalogs();
}

// This is needed so that the master beslistner will get the exit status of
// all of the child beslisteners (preventing them from becoming zombies).
static void CatchSigChild(int sig)
{
    if (sig == SIGCHLD)
    {
        BESDEBUG("besdaemon", "beslistener: caught sig chld" << endl);
        int stat;
        pid_t pid = wait(&stat);
        BESDEBUG("besdaemon", "beslistener: child pid: " << pid << " exited with status: " << stat << endl);
    }
}

// If the HUP signal is sent to the master beslistener, it should exit and
// return a value indicating to the besdaemon that it should be restarted.
// This also has the side-affect of re-reading the configuration file.
static void CatchSigHup(int sig)
{
    if (sig == SIGHUP)
    {
        int pid = getpid();
        BESDEBUG("besdaemon", "beslisterner: " << pid << " caught SIGHUP." << endl);

        BESApp::TheApplication()->terminate(sig);

        BESDEBUG("besdaemon", "beslisterner: " << pid << " past terminate (SIGHUP)." << endl);

        exit(SERVER_EXIT_RESTART);
    }
}
// This is the default signal sent by 'kill'; when the master beslistener gets
// this signal it should stop. besdaemon should not try to start a new
// master beslistener.
static void CatchSigTerm(int sig)
{
    if (sig == SIGTERM)
    {
        int pid = getpid();
        BESDEBUG("besdaemon", "beslisterner: " << pid << " caught SIGTERM" << endl);

        BESApp::TheApplication()->terminate(sig);

        BESDEBUG("besdaemon", "beslisterner: " << pid << " past terminate (SIGTERM)." << endl);

        exit(SERVER_EXIT_NORMAL_SHUTDOWN);
    }
}

/** Register the signal handlers. This registers handlers for HUP, TERM and
 *  CHLD. For each, if this OS supports restarting 'slow' system calls, enable
 *  that. For the TERM and HUP handlers, block SIGCHLD for the duration of
 *  the handler (we call stop_all_beslisteners() in those handlers and that
 *  function uses wait() to collect the exit status of the child processes).
 *  This ensure that our signal handlers (TERM and HUP) don't themselves get
 *  interrupted.
 */
static void register_signal_handlers()
{
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGCHLD);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGHUP);
    act.sa_flags = 0;
#ifdef SA_RESTART
    BESDEBUG("besdaemon" , "besdaemon: setting restart for sigchld." << endl);
    act.sa_flags |= SA_RESTART;
#endif

    BESDEBUG( "server", "beslisterner: Registering signal handlers ... " << endl );

    act.sa_handler = CatchSigChild;
    if (sigaction(SIGCHLD, &act, 0))
        throw BESInternalFatalError("Could not register a handler to catch beslistener child process status.", __FILE__, __LINE__);

    // For these, block sigchld
    //  sigaddset(&act.sa_mask, SIGCHLD);
    act.sa_handler = CatchSigTerm;
    if (sigaction(SIGTERM, &act, 0) < 0)
        throw BESInternalFatalError("Could not register a handler to catch beslistener terminate signal.", __FILE__, __LINE__);

    act.sa_handler = CatchSigHup;
    if (sigaction(SIGHUP, &act, 0) < 0)
        throw BESInternalFatalError("Could not register a handler to catch beslistener hup signal.", __FILE__, __LINE__);

    BESDEBUG( "server", "beslisterner: OK" << endl );
}

int ServerApp::initialize(int argc, char **argv)
{
    int c = 0;
    bool needhelp = false;
    string dashi;
    string dashc;
    string dashd = "";

    // If you change the getopt statement below, be sure to make the
    // corresponding change in daemon.cc and besctl.in
    while ((c = getopt(argc, argv, "hvsd:c:p:u:i:r:")) != EOF)
    {
        switch (c)
        {
            case 'i':
                dashi = optarg;
                break;
            case 'c':
                dashc = optarg;
                break;
            case 'r':
                break; // we can ignore the /var/run directory option here
            case 'p':
                _portVal = atoi(optarg);
                _gotPort = true;
                break;
            case 'u':
                _unixSocket = optarg;
                break;
            case 'd':
                dashd = optarg;
                // BESDebug::SetUp(optarg);
                break;
            case 'v':
                BESServerUtils::show_version(BESApp::TheApplication()->appName());
                break;
            case 's':
                _secure = true;
                break;
            case 'h':
            case '?':
            default:
                needhelp = true;
                break;
        }
    }

    // before we can do any processing, log any messages, initialize any
    // modules, do anything, we need to determine where the BES
    // configuration file lives. From here we get the name of the log
    // file, group and user id, and information that the modules will
    // need to run properly.

    // If the -c option was passed, set the config file name in TheBESKeys
    if (!dashc.empty())
    {
        TheBESKeys::ConfigFile = dashc;
    }

    // If the -c option was not passed, but the -i option
    // was passed, then use the -i option to construct
    // the path to the config file
    if (dashc.empty() && !dashi.empty())
    {
        if (dashi[dashi.length() - 1] != '/')
        {
            dashi += '/';
        }
        string conf_file = dashi + "etc/bes/bes.conf";
        TheBESKeys::ConfigFile = conf_file;
    }

    if (!dashd.empty())
        BESDebug::SetUp(dashd);

    // register the two debug context for the server and ppt. The
    // Default Module will register the bes context.
    BESDebug::Register("server");
    BESDebug::Register("ppt");

    // Because we are now running as the user specified in the
    // configuration file, we won't be able to listen on system ports.
    // If this is a problem, we may need to move this code above setting
    // the user and group ids.
    bool found = false;
    string port_key = "BES.ServerPort";
    if (!_gotPort)
    {
        string sPort;
        try
        {
            TheBESKeys::TheKeys()->get_value(port_key, sPort, found);
        }
        catch (BESError &e)
        {
            BESDEBUG( "server", "beslisterner: FAILED" << endl );
            string err = (string) "FAILED: " + e.get_message();
            cerr << err << endl;
            (*BESLog::TheLog()) << err << endl;
            exit(SERVER_EXIT_FATAL_CAN_NOT_START);
        }
        if (found)
        {
            _portVal = atoi(sPort.c_str());
            if (_portVal != 0)
            {
                _gotPort = true;
            }
        }
    }

    found = false;
    string socket_key = "BES.ServerUnixSocket";
    if (_unixSocket == "")
    {
        try
        {
            TheBESKeys::TheKeys()->get_value(socket_key, _unixSocket, found);
        }
        catch (BESError &e)
        {
            BESDEBUG( "server", "beslisterner: FAILED" << endl );
            string err = (string) "FAILED: " + e.get_message();
            cerr << err << endl;
            (*BESLog::TheLog()) << err << endl;
            exit(SERVER_EXIT_FATAL_CAN_NOT_START);
        }
    }

    if (!_gotPort && _unixSocket == "")
    {
        string msg = "Must specify a tcp port or a unix socket or both\n";
        msg += "Please specify on the command line with -p <port>";
        msg += " and/or -u <unix_socket>\n";
        msg += "Or specify in the bes configuration file with " + port_key + " and/or " + socket_key + "\n";
        cout << endl << msg;
        (*BESLog::TheLog()) << msg << endl;
        BESServerUtils::show_usage(BESApp::TheApplication()->appName());
    }

    found = false;
    if (_secure == false)
    {
        string key = "BES.ServerSecure";
        string isSecure;
        try
        {
            TheBESKeys::TheKeys()->get_value(key, isSecure, found);
        }
        catch (BESError &e)
        {
            BESDEBUG( "server", "beslisterner: FAILED" << endl );
            string err = (string) "FAILED: " + e.get_message();
            cerr << err << endl;
            (*BESLog::TheLog()) << err << endl;
            exit(SERVER_EXIT_FATAL_CAN_NOT_START);
        }
        if (isSecure == "Yes" || isSecure == "YES" || isSecure == "yes")
        {
            _secure = true;
        }
    }

    try
    {
        register_signal_handlers();
    }
    catch (BESError &e)
    {
        BESDEBUG( "server", "beslisterner: FAILED: " << e.get_message() << endl );
        (*BESLog::TheLog()) << e.get_message() << endl;
        exit(SERVER_EXIT_FATAL_CAN_NOT_START);
    }

    BESDEBUG( "server", "beslisterner: initializing default module ... "
            << endl );
    BESDefaultModule::initialize(argc, argv);
    BESDEBUG( "server", "beslisterner: done initializing default module"
            << endl );

    BESDEBUG( "server", "beslisterner: initializing default commands ... "
            << endl );
    BESXMLDefaultCommands::initialize(argc, argv);
    BESDEBUG( "server", "beslisterner: done initializing default commands"
            << endl );

    // This will load and initialize all of the modules
    BESDEBUG( "server", "beslisterner: initializing loaded modules ... "
            << endl );
    int ret = BESModuleApp::initialize(argc, argv);
    BESDEBUG( "server", "beslisterner: done initializing loaded modules"
            << endl );

    BESDEBUG( "server", "beslisterner: initialized settings:" << *this );

    if (needhelp)
    {
        BESServerUtils::show_usage(BESApp::TheApplication()->appName());
    }

    // This sets the process group to be ID of this process. All children
    // will get this GID. Then use killpg() to send a signal to this process
    // and all of the children.
    session_id = setsid();
    BESDEBUG("besdaemon", "beslisterner: The master beslistener session id (group id): " << session_id << endl);

    return ret;
}

int ServerApp::run()
{
    try
    {
        BESDEBUG( "server", "beslisterner: initializing memory pool ... "
                << endl );
        BESMemoryManager::initialize_memory_pool();
        BESDEBUG( "server", "OK" << endl );

        SocketListener listener;
        if (_portVal)
        {
            _ts = new TcpSocket(_portVal);
            listener.listen(_ts);

            BESDEBUG( "server", "beslisterner: listening on port (" << _portVal << ")" << endl );

            BESDEBUG( "server", "beslisterner: about to write status (4)" << endl );
            // Write to stdout works because the besdaemon is listening on the
            // other end of a pipe where the pipe fd[1] has been dup2'd to
            // stdout. See daemon.cc:start_master_beslistener.
            // NB BESLISTENER_PIPE_FD is 1 (stdout)
            int status = BESLISTENER_RUNNING;
            int res = write(BESLISTENER_PIPE_FD, &status, sizeof(status));

            BESDEBUG( "server", "beslisterner: wrote status (" << res << ")" << endl );
        }

        if (!_unixSocket.empty())
        {
            _us = new UnixSocket(_unixSocket);
            listener.listen(_us);
            BESDEBUG( "server", "beslisterner: listening on unix socket ("
                    << _unixSocket << ")" << endl );
        }

        BESServerHandler handler;

        _ps = new PPTServer(&handler, &listener, _secure);
        _ps->initConnection();

        _ps->closeConnection();
    }
    catch (BESError &se)
    {
        cerr << se.get_message() << endl;
        (*BESLog::TheLog()) << se.get_message() << endl;
        int status = SERVER_EXIT_FATAL_CAN_NOT_START;
        write(BESLISTENER_PIPE_FD, &status, sizeof(status));
        close(BESLISTENER_PIPE_FD);
        return 1;
    }
    catch (...)
    {
        cerr << "caught unknown exception" << endl;
        (*BESLog::TheLog()) << "caught unknown exception initializing sockets" << endl;
        int status = SERVER_EXIT_FATAL_CAN_NOT_START;
        write(BESLISTENER_PIPE_FD, &status, sizeof(status));
        close(BESLISTENER_PIPE_FD);
        return 1;
    }

    close(BESLISTENER_PIPE_FD);
    return 0;
}

int ServerApp::terminate(int sig)
{
    pid_t apppid = getpid();
    if (apppid == _mypid)
    {
        if (_ps)
        {
            _ps->closeConnection();
            delete _ps;
        }
        if (_ts)
        {
            _ts->close();
            delete _ts;
        }
        if (_us)
        {
            _us->close();
            delete _us;
        }

        // Do this in the reverse order that it was initialized. So
        // terminate the loaded modules first, then the default
        // commands, then the default module.
        BESDEBUG( "server", "beslisterner: terminating loaded modules ...  "
                << endl );
        BESModuleApp::terminate(sig);
        BESDEBUG( "server", "beslisterner: done terminating loaded modules"
                << endl );

        BESDEBUG( "server", "beslisterner: terminating default commands ...  "
                << endl );
        BESXMLDefaultCommands::terminate();
        BESDEBUG( "server", "beslisterner: done terminating default commands ...  "
                << endl );

        BESDEBUG( "server", "beslisterner: terminating default module ... "
                << endl );
        BESDefaultModule::terminate();
        BESDEBUG( "server", "beslisterner: done terminating default module ... "
                << endl );
    }
    return sig;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void ServerApp::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "ServerApp::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "got port? " << _gotPort << endl;
    strm << BESIndent::LMarg << "port: " << _portVal << endl;
    strm << BESIndent::LMarg << "unix socket: " << _unixSocket << endl;
    strm << BESIndent::LMarg << "is secure? " << _secure << endl;
    strm << BESIndent::LMarg << "pid: " << _mypid << endl;
    if (_ts)
    {
        strm << BESIndent::LMarg << "tcp socket:" << endl;
        BESIndent::Indent();
        _ts->dump(strm);
        BESIndent::UnIndent();
    }
    else
    {
        strm << BESIndent::LMarg << "tcp socket: null" << endl;
    }
    if (_us)
    {
        strm << BESIndent::LMarg << "unix socket:" << endl;
        BESIndent::Indent();
        _us->dump(strm);
        BESIndent::UnIndent();
    }
    else
    {
        strm << BESIndent::LMarg << "unix socket: null" << endl;
    }
    if (_ps)
    {
        strm << BESIndent::LMarg << "ppt server:" << endl;
        BESIndent::Indent();
        _ps->dump(strm);
        BESIndent::UnIndent();
    }
    else
    {
        strm << BESIndent::LMarg << "ppt server: null" << endl;
    }
    BESModuleApp::dump(strm);
    BESIndent::UnIndent();
}

int main(int argc, char **argv)
{
    try
    {
        ServerApp app;
        return app.main(argc, argv);
    }
    catch (BESError &e)
    {
        cerr << "Caught unhandled exception: " << endl;
        cerr << e.get_message() << endl;
        return 1;
    }
    catch (...)
    {
        cerr << "Caught unhandled, unknown exception" << endl;
        return 1;
    }
    return 0;
}

