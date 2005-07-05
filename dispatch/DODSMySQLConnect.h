// DODSMySQLConnect.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSMySQLConnect_h_
#define DODSMySQLConnect_h_ 1

#include <string>

using std::string ;

#include <mysql.h>

class DODSMySQLQuery ;

class DODSMySQLConnect
{
private:
    string		_server, _user, _database ;
    string		_error ;
    bool		_channel_open ;
    bool		_has_log ;
    MYSQL *		_the_channel ;
    MYSQL		_mysql ;
    int			_count ;

public:
    			DODSMySQLConnect() ;
			~DODSMySQLConnect() ;
    void		open(const string &server, const string &user,
			     const string &password, const string &database ) ;
    void		close ();
    int			is_channel_open() const { return _channel_open ; }
    string		get_error();
    MYSQL *		get_channel() { return _the_channel ; }
    string		get_server() { return _server ; }
    string		get_user() { return _user ; }
    string		get_database() { return _database ; }
};

#endif // DODSMySQLConnect_h_

// $Log: DODSMySQLConnect.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
