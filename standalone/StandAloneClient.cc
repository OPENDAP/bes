// StandAloneClient.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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

#include "config.h"

#include <iostream>
#include <fstream>

using std::cout;
using std::endl;
using std::cerr;
using std::ofstream;
using std::ios;
using std::flush;

#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else                         /* !defined(HAVE_READLINE_H) */
extern "C" {
    char *readline(const char *);
}
#  endif                        /* !defined(HAVE_READLINE_H) */
char *cmdline = NULL;
#else                           /* !defined(HAVE_READLINE_READLINE_H) */
  /* no readline */
#endif                          /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  else                         /* !defined(HAVE_HISTORY_H) */
extern "C" {
    int add_history(const char *);
    int write_history(const char *);
    int read_history(const char *);
}
#  endif                        /* defined(HAVE_READLINE_HISTORY_H) */
  /* no history */
#endif                          /* HAVE_READLINE_HISTORY */
#define SIZE_COMMUNICATION_BUFFER 4096*4096
#include "StandAloneClient.h"
#include "BESDebug.h"
#include "BESCmdInterface.h"

StandAloneClient::~StandAloneClient()
{
    if (_strmCreated && _strm) {
        _strm->flush();
        delete _strm;
        _strm = 0;
    } else if (_strm) {
        _strm->flush();
    }
}

/** @brief Set the output stream for responses from the BES server.
* 
* Specify where the response output from your BES request will be
* sent. Set to null if you wish to ignore the response from the BES
* server.
* 
* @param  strm  an OutputStream specifying where to send responses from
*               the BES server. If null then the output will not be
*               output but will be thrown away.
* @param created true of the passed stream was created and can be deleted
*                either by being replaced ro in the destructor
* @throws BESError catches any problems with opening or writing to
*                      the output stream and creates a BESError
* @see    OutputStream
* @see    BESError
*/
void
StandAloneClient::setOutput(ostream * strm, bool created)
{
    if (_strmCreated && _strm) {
        _strm->flush();
        delete _strm;
    } else if (_strm) {
        _strm->flush();
    }
    _strm = strm;
    _strmCreated = created;
}

/** @brief Executes a client side command
* 
* Client side commands include
* client suppress;
* client output to screen;
* client output to &lt;filename&gt;;
*
* @param  cmd  The BES client side command to execute
* @see    BESError
*/
void
StandAloneClient::executeClientCommand(const string & cmd)
{
    string suppress = "suppress";
    if (cmd.compare(0, suppress.length(), suppress) == 0) {
        setOutput(NULL, false);
    } else {
        string output = "output to";
        if (cmd.compare(0, output.length(), output) == 0) {
            string subcmd = cmd.substr(output.length() + 1);
            string screen = "screen";
            if (subcmd.compare(0, screen.length(), screen) == 0) {
                setOutput(&cout, false);
            } else {
                // subcmd is the name of the file - the semicolon
                string file = subcmd.substr(0, subcmd.length() - 1);
                ofstream *fstrm = new ofstream(file.c_str(), ios::app);
                if (!(*fstrm)) {
                    cerr << "Unable to set client output to file " << file
                        << endl;
                } else {
                    setOutput(fstrm, true);
                }
            }
        } else {
            cerr << "Improper client command " << cmd << endl;
        }
    }
}

/** @brief Sends a single OpeNDAP request ending in a semicolon (;) to the
* OpeNDAP server.
* 
* The response is written to the output stream if one is specified,
* otherwise the output is ignored.
*
* @param  cmd  The BES request, ending in a semicolon, that is sent to
*              the BES server to handle.
* @param repeat Number of times to repeat the command
* @throws BESError Thrown if there is a problem sending the request
*                      to the server or a problem receiving the response
*                      from the server.
* @see    BESError
*/
void
StandAloneClient::executeCommand(const string & cmd, int repeat )
{
    string client = "client";
    if (cmd.compare(0, client.length(), client) == 0) {
        executeClientCommand(cmd.substr(client.length() + 1));
    } else {
	if( repeat < 1 ) repeat = 1 ;
	for( int i = 0; i < repeat; i++ )
	{
	    BESDEBUG( "standalone", "cmdclient sending " << cmd << endl )
	    BESCmdInterface interface( cmd, _strm ) ;
	    int status = interface.execute_request( "standalone" ) ;

	    if( status == 0 )
	    {
		BESDEBUG( "standalone", "BESServerHandler::execute - executed successfully" << endl )
	    }
	    else
	    {
		// an error has occurred.
		BESDEBUG( "standalone", "BESServerHandler::execute - error occurred" << endl )

		// flush what we have in the stream to the client
		*_strm << flush ;

		// transmit the error message. finish_with_error will transmit
		// the error
		interface.finish_with_error( status ) ;

		switch (status)
		{
		    case BES_INTERNAL_FATAL_ERROR:
			{
			    cerr << "BES server " << getpid()
				 << ": Status not OK, dispatcher returned value "
				 << status << endl ;
			    //string toSend = "FATAL ERROR: server must exit!" ;
			    //c->send( toSend ) ;
			    exit( 1 ) ;
			}
			break;
		    case BES_INTERNAL_ERROR: 
		    case BES_SYNTAX_USER_ERROR:
		    case BES_FORBIDDEN_ERROR:
		    case BES_NOT_FOUND_ERROR:
		    default:
			break;
		}
	    }
	}
    }
}

/** @brief Execute each of the commands in the cmd_list, separated by a * semicolon.
* 
* The response is written to the output stream if one is specified,
* otherwise the output is ignored.
*
* @param  cmd_list  The list of BES requests, separated by semicolons
*                   and ending in a semicolon, that will be sent to the
*                   BES server to handle, one at a time.
* @param repeat     Number of times to repeat the list of commands
* @throws BESError Thrown if there is a problem sending any of the
*                      request to the server or a problem receiving any
*                      of the responses from the server.
* @see    BESError
*/
void
StandAloneClient::executeCommands(const string & cmd_list, int repeat)
{
    if( repeat < 1 ) repeat = 1 ;
    for( int i = 0; i < repeat; i++ )
    {
	std::string::size_type start = 0;
	std::string::size_type end = 0;
	while ((end = cmd_list.find(';', start)) != string::npos) {
	    string cmd = cmd_list.substr(start, end - start + 1);
	    executeCommand(cmd, 1);
	    start = end + 1;
	}
    }
}

/** @brief Sends the requests listed in the specified file to the BES server,
* each command ending with a semicolon.
* 
* The requests do not have to be one per line but can span multiple
* lines and there can be more than one command per line.
* 
* The response is written to the output stream if one is specified,
* otherwise the output is ignored.
*
* @param  istrm  The file holding the list of BES requests, each
*                ending with a semicolon, that will be sent to the
*                BES server to handle.
* @param repeat  Number of times to repeat the series of commands
                 from the file.
* @throws BESError Thrown if there is a problem opening the file to
*                      read, reading the requests from the file, sending
*                      any of the requests to the server or a problem
*                      receiving any of the responses from the server.
* @see    File
* @see    BESError
*/
void
StandAloneClient::executeCommands(ifstream & istrm, int repeat)
{
    if( repeat < 1 ) repeat = 1 ;
    for( int i = 0; i < repeat; i++ )
    {
	istrm.clear( ) ;
	istrm.seekg( 0, ios::beg ) ;
	string cmd;
	bool done = false;
	while (!done) {
	    char line[4096];
	    line[0] = '\0';
	    istrm.getline(line, 4096, '\n');
	    string nextLine = line;
	    if (nextLine == "") {
		if (cmd != "") {
		    this->executeCommands(cmd, 1);
		}
		done = true;
	    } else {
		std::string::size_type i = nextLine.find_last_of(';');
		if (i == string::npos) {
		    if (cmd == "") {
			cmd = nextLine;
		    } else {
			cmd += " " + nextLine;
		    }
		} else {
		    string sub = nextLine.substr(0, i + 1);
		    if (cmd == "") {
			cmd = sub;
		    } else {
			cmd += " " + sub;
		    }
		    this->executeCommands(cmd, 1);
		    if (i == nextLine.length() || i == nextLine.length() - 1) {
			cmd = "";
		    } else {
			cmd = nextLine.substr(i + 1, nextLine.length());
		    }
		}
	    }
	}
    }
}

/** @brief An interactive BES client that takes BES requests on the command line.
* 
* There can be more than one command per line, but commands can NOT span
* multiple lines. The user will be prompted to enter a new BES request.
* 
* OpenDAPClient:
* 
* The response is written to the output stream if one is specified,
* otherwise the output is ignored.
*
* @throws BESError Thrown if there is a problem sending any of the
*                      requests to the server or a problem receiving any
*                      of the responses from the server.
* @see    BESError
*/
void
StandAloneClient::interact()
{
    cout << endl << endl
        << "Type 'exit' to exit the command line client and 'help' or '?' "
        << "to display the help screen" << endl << endl;

    bool done = false;
    while (!done) {
        string message = "";
        size_t len = this->readLine(message);
        if (len == -1 || message == "exit" || message == "exit;") {
            done = true;
        } else if (message == "help" || message == "help;"
                   || message == "?") {
            this->displayHelp();
        } else if (len != 0 && message != "") {
            if (message[message.length() - 1] != ';') {
                cerr << "Commands must end with a semicolon" << endl;
            } else {
                this->executeCommands(message, 1);
                message = "";   // Needed? Added during debugging. jhrg 9/8/05
                len = 0;
            }
        }
    }
}

/** @brief Read a line from the interactive terminal using the readline library
 *
 * @param msg read the line into this string
 * @return number of characters read
 */
size_t
StandAloneClient::readLine(string & msg)
{
    size_t len = 0;
    char *buf = (char *) NULL;
    buf =::readline("BESClient> ");
    if (buf && *buf) {
        len = strlen(buf);
#ifdef HAVE_READLINE_HISTORY
        add_history(buf);
#endif
        if (len > SIZE_COMMUNICATION_BUFFER) {
            cerr << __FILE__ << __LINE__
                <<
                ": incoming data buffer exceeds maximum capacity with lenght "
                << len << endl;
            exit(1);
        } else {
            msg = buf;
        }
    } else {
        if (!buf) {
            // If a null buffer is returned then this means that EOF is
            // returned. This is different from the user just hitting enter,
            // which means a character buffer is returned, but is empty.
            
            // Problem: len is unsigned.
            len = -1;
        }
    }
    if (buf) {
        free(buf);
        buf = (char *) NULL;
    }
    return len;
}

/** @brief display help information for the command line client
 */
void
StandAloneClient::displayHelp()
{
    cout << endl;
    cout << endl;
    cout << "BES Command Line Client Help" << endl;
    cout << endl;
    cout << "Client commands available:" << endl;
    cout <<
        "    exit                     - exit the command line interface" <<
        endl;
    cout << "    help                     - display this help screen" <<
        endl;
    cout <<
        "    client suppress;         - suppress output from the server" <<
        endl;
    cout <<
        "    client output to screen; - display server output to the screen"
        << endl;
    cout <<
        "    client output to <file>; - display server output to specified file"
        << endl;
    cout << endl;
    cout <<
        "Any commands beginning with 'client' must end with a semicolon" <<
        endl;
    cout << endl;
    cout << "To display the list of commands available from the server "
        << "please type the command 'show help;'" << endl;
    cout << endl;
    cout << endl;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
StandAloneClient::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "StandAloneClient::dump - ("
        << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "stream: " << (void *) _strm << endl;
    strm << BESIndent::LMarg << "stream created? " << _strmCreated << endl;
    BESIndent::UnIndent();
}
