// CmdClient.h

#ifndef CmdClient_h
#define CmdClient_h 1

#include <iostream>
#include <string>
#include <fstream>

using std::ostream ;
using std::ifstream ;
using std::string ;

class PPTClient ;

/**
 * CmdClient is an object that handles the connection to, sending requests
 * to, and receiving response from a specified OpenDAP server running either
 * on this machine or another machine.
 * <p>
 * Requests to the OpenDAP server can be taken in different ways by the
 * CmdClient object.
 * <UL>
 * <LI>One request, ending with a semicolon.</LI>
 * <LI>Multiple requests, each ending with a semicolon.</LI>
 * <LI>Requests listed in a file, each request can span multiple lines in
 * the file and there can be more than one request per line. Each request
 * ends with a semicolon.</LI>
 * <LI>Interactive mode where the user inputs requests on the command line,
 * each ending with a semicolon, with multiple requests allowed per
 * line.</LI>
 * </UL>
 * <p>
 * Response from the requests can sent to any File or OutputStream as
 * specified by using the setOutput methods. If no output is specified using
 * the setOutput methods thent he output is ignored.
 *
 * Thread safety of this object has not yet been determined.
 *
 * @author Patrick West <A * HREF="mailto:pwest@hao.ucar.edu">pwest@hao.ucar.edu</A>
*/

class CmdClient
{
private:
    PPTClient *			_client ;
    ostream *			_strm ;
    bool			_strmCreated ;

    int				readLine( string &str ) ;
public:
    				CmdClient( )
				    : _client( 0 ),
				      _strm( 0 ),
				      _strmCreated( false ) {}
				~CmdClient() ;

    void			startClient( const string &host, int portVal ) ;
    void			startClient( const string &unixSocket ) ;
    void			shutdownClient() ;
    void			setOutput( ostream *strm, bool created ) ;
    void			executeClientCommand( const string &cmd ) ;
    void			executeCommand( const string &cmd ) ;
    void			executeCommands( const string &cmd_list ) ;
    void			executeCommands( ifstream &inputFile ) ;
    void			interact() ;
    bool			isConnected() ;
    void			brokenPipe() ;
} ;

#endif // CmdClient_h

