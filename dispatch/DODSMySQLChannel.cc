// DODSMySQLChannel.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSMySQLChannel.h"
#include "DODSException.h"
#include "TheDODSLog.h"

DODSMySQLChannel::DODSMySQLChannel()
{
    _channel_open = false ;
    _has_log = false ;
    _the_channel = 0 ;
    mysql_init( &_mysql ) ;
}

DODSMySQLChannel::~DODSMySQLChannel()
{
    if( _channel_open )
    {
	if( TheDODSLog->is_verbose() )
	{
	    (*TheDODSLog) << "MySQL channel disconnected from:" << endl
			  << "  server = " << _server << endl
			  << "  user = " << _user << endl
			  << "  database = " << _database << endl ;
	}
	mysql_close( _the_channel ) ;
    }
}

bool
DODSMySQLChannel::open( const char* server, const char* user,
			const char* password, const char* database)
{
    if( _channel_open )
	return true ;
    else
    {
	_server = server ;
	_user = user ;
	_database = database ;
	_the_channel = mysql_real_connect( &_mysql, server, user,
					   password, database, 0, 0, 0 ) ;
	if( !_the_channel )
	    return false ;
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
	    return true ;
	}
    }
}

void
DODSMySQLChannel::close()
{
    if( _channel_open )
    {
	mysql_close( _the_channel ) ;
	_channel_open = 0 ;
    }
}

string
DODSMySQLChannel::get_error()
{
    if( _channel_open )
	_error = mysql_error( _the_channel ) ;      
    else
	_error = mysql_error( &_mysql ) ;
    return _error ;
}

// $Log: DODSMySQLChannel.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
