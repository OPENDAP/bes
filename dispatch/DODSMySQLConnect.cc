// DODSMySQLConnect.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSMySQLConnect.h"
#include "DODSMySQLConnectException.h"
#include "DODSMemoryException.h"
#include "TheDODSLog.h"
#include "DODSEncode.h"

DODSMySQLConnect::DODSMySQLConnect()
{
    _count = 0 ;
    _channel_open = false ;
    _has_log = false ;
    _the_channel = 0 ;
    mysql_init( &_mysql ) ;
}

DODSMySQLConnect::~DODSMySQLConnect()
{
    if( _channel_open )
    {
	mysql_close( _the_channel ) ;
	if( TheDODSLog->is_verbose() )
	{
	    (*TheDODSLog) << "MySQL channel disconnected from:" << endl
			  << "  server = " << _server << endl
			  << "  user = " << _user << endl
			  << "  database = " << _database << endl ;
	}
    }
}

void
DODSMySQLConnect::open( const string &server, const string &user,
		        const string &password, const string &database )
{
    _count++ ;
    if( !_channel_open )
    {
	_server = server ;
	_user = user ;
	_database = database ;
	string dp = DODSEncode::decode( password.c_str(), "disp_key" ) ;
	_the_channel = mysql_real_connect( &_mysql, server.c_str(),
					   user.c_str(), dp.c_str(),
					   database.c_str(), 0, 0, 0 ) ;
	if( !_the_channel )
	{
	    DODSMySQLConnectException ce ;
	    string error = "can not get connected to MySQL engine." ;
	    string mysql_error = get_error() ;
	    error += "\n" + mysql_error ;
	    ce.set_error_description( error ) ;
	    throw ce ;
	}
	else
	{
	    if( TheDODSLog->is_verbose() )
	    {
		(*TheDODSLog) << "MySQL channel connected to:" << endl
			      << "  server = " << _server << endl
			      << "  user = " << _user << endl
			      << "  database = " << _database << endl ;
	    }
	    _channel_open = true ;
	}
    }
}

void
DODSMySQLConnect::close()
{
    _count-- ;
    if( _count == 0 && _channel_open )
    {
	mysql_close( _the_channel ) ;
	_channel_open = false ;
	(*TheDODSLog) << "MySQL channel disconnected from:" << endl
		      << "  server = " << _server << endl
		      << "  user = " << _user << endl
		      << "  database = " << _database << endl ;
    }
}

string
DODSMySQLConnect::get_error()
{
    if( _channel_open )
	_error = mysql_error( _the_channel ) ;      
    else
	_error = mysql_error( &_mysql ) ;
    return _error ;
}

// $Log: DODSMySQLConnect.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
