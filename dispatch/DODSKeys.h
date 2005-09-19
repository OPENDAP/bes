// DODSKeys.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSKeys_h_
#define DODSKeys_h_ 1

#include <fstream>
#include <map>
#include <string>

using std::string ;
using std::map ;
using std::ifstream ;

/** @brief mapping of key/value pairs defining different behaviors of an
 * application.
 *
 * DODSKeys provides a mechanism to define the behavior of an application
 * given key/value paris. For example, how authentication will work, database
 * access information, level of debugging and where log files are to be
 * located.
 *
 * Key/value pairs can be loaded from an external initialization file or set
 * within the application itself, for example from the command line.
 *
 * If from a file the key/value pair is set one per line and cannot span
 * multiple lines. Comments are allowed using the pound (#) character.
 *
 * <PRE>
 * #
 * # These keys define the behavior of database authentication
 * #
 * OpenDAP.Authentication.MySQL.username=username
 * OpenDAP.Authentication.MySQL.password=password
 * OpenDAP.Authentication.MySQL.server=myMachine
 * OpenDAP.Authentication.MySQL.database=authDB
 * </PRE>
 *
 * Key/value pairs can also be set by passing in a key=value string, or by
 * passing in a key and value string to the object.
 *
 * OpenDAP provides a single object for access to a single DODSKeys object,
 * TheDODSKeys.
 */
class DODSKeys
{
private:
    ifstream *		_keys_file ;
    string		_keys_file_name ;
    map<string,string> *	_the_keys ;

    void		clean() ;
    void 		load_keys() ;
    bool 		break_pair( const char* b,
				    string& key,
				    string &value ) ;
    bool		only_blanks( const char *line ) ;
    			DODSKeys() {}
protected:
    			DODSKeys( const string &keys_file_name ) ;
public:
    			~DODSKeys() ;

    string		keys_file_name() { return _keys_file_name ; }

    string		set_key( const string &key, const string &val ) ;
    string		set_key( const string &pair ) ;
    string		get_key( const string& s, bool &found ) ;
    void		show_keys();

    typedef map< string, string >::const_iterator Keys_citer ;
    Keys_citer		keys_begin() { return _the_keys->begin() ; }
    Keys_citer		keys_end() { return _the_keys->end() ; }
};

#endif // DODSKeys_h_

// $Log: DODSKeys.h,v $
// Revision 1.4  2005/04/19 17:59:25  pwest
// providing mechanism to iterate through keys using const iterator from outside the class
//
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
