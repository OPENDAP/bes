// DODSKeys.cc

// 2004 Copyright University Corporation for Atmospheric Research

#ifdef __cplusplus
extern "C" {
#include <sys/types.h>
#include "regex.h"
}
#endif

#include <stdio.h>
#include <unistd.h>
#include <iostream>

using std::endl ;
using std::cout ;

#include "DODSKeys.h"
#include "DODSKeysException.h"

/** @brief default constructor that reads the environment variable "DODS_INI"
 * in order to load key/value pairs.
 *
 * This constructor reads the environment variable DODS_INI to determine the
 * initialization file to load. This file holds different key/value pairs for
 * the application, on key/value pair per line separated by an equal (=) sign.
 *
 * key=value
 *
 * Comments are allowed in the file and must begin with a pound (#) sign at
 * the beginning of the line. No comments are allowed at the end of lines.
 *
 * @throws DODSKeysException thrown if there is an error reading the
 * initialization file or a syntax error in the file, i.e. a malformed
 * key/value pair.
 */
DODSKeys::DODSKeys( const string &keys_file_name )
    : _keys_file( 0 ),
      _keys_file_name( keys_file_name ),
      _the_keys( 0 )
{
    _keys_file = new ifstream( _keys_file_name.c_str() ) ;
    if( !(*_keys_file) )
    {
	char path[500] ;
	getcwd( path, sizeof( path ) ) ;
	string s = string("DODS: fatal, can not open initialization file ")
		   + _keys_file_name + "\n"
		   + "The current working directory is " + path + "\n" ;
	throw DODSKeysException( s ) ;
    }

    _the_keys = new map<string,string,less<string> >;
    try
    {
	load_keys();
    }
    catch(DODSKeysException &ex)
    {
	clean();
	throw;
    }
    catch(...)
    {
	clean() ;
	throw DODSKeysException("Undefined exception while trying to load keys");
    }
}

/** @brief cleans up the key/value pair mapping
 */
DODSKeys::~DODSKeys()
{
    clean() ;
}

void
DODSKeys::clean()
{
    if( _keys_file )
    {
	_keys_file->close() ;
	delete _keys_file ;
    }
    if( _the_keys )
    {
	delete _the_keys ;
    }
}

inline void
DODSKeys::load_keys()
{
    char buffer[255];
    string key,value;
    while(!(*_keys_file).eof())
    {
	if((*_keys_file).getline(buffer,255))
	{
	    if( break_pair( buffer, key, value ) )
	    {
		(*_the_keys)[key]=value;
	    }
	}
    }
}

inline bool
DODSKeys::break_pair(const char* b, string& key, string &value)
{
    if((b[0]!='#') && (!only_blanks(b)))//Ignore comments a lines with only spaces
    {
	register int l=strlen(b);
	if(l>1)
	{
	    register int how_many_equals=0;
	    int pos=0;
	    for (register int j=0;j<l;j++)
	    {
		if(b[j] == '=')
		{
		    how_many_equals++;
		    pos=j;
		}
	    }

	    if(how_many_equals!=1)
	    {
		char howmany[256] ;
		sprintf( howmany, "%d", how_many_equals ) ;
		string s = string( "DODS: invalid entry " ) + b
		           + "; there are " + howmany
			   + " = characters.\n";
		throw DODSKeysException( s );
	    }
	    else
	    {
		string s=b;
		key=s.substr(0,pos);
		value=s.substr(pos+1,s.size());

		return true;
	    }
	}

	return false;
    }

    return false;
}

bool
DODSKeys::only_blanks(const char *line)
{
    int val;
    regex_t rx;
    val=regcomp (&rx, "[^[:space:]]",REG_ICASE);

    if(val!=0)
	throw DODSKeysException("Regular expression did not compile correclty");
    val=regexec (&rx,line,0,0, REG_NOTBOL);
    if (val==0)
    {
	regfree(&rx);
	return false;
    }
    else
    {
	if (val==REG_NOMATCH)
	{
	    regfree(&rx);
	    return true;
	}
	else if (val==REG_ESPACE)
	    throw DODSKeysException("Regular expression out of space");
	else
	    throw DODSKeysException("Regular expression has an undefined problem");
    }
}

/** @brief allows the user to set key/value pairs from within the application.
 *
 * This method allows users of DODSKeys to set key/value pairs from within the
 * application, such as for testing purposes, key/value pairs from the command
 * line, etc...
 *
 * If the key is already set then this value replaces the value currently held
 * in the keys map.
 *
 * @param key variable name that can be accessed using the get_key method
 * @param val value of the variable returned when get_key is called for this
 * key
 * @return returns the value of the key, empty string if unsuccessful
 */
string
DODSKeys::set_key( const string &key, const string &val )
{
    map< string, string, less<string> >::iterator i ;
    i = _the_keys->find( key ) ;
    if( i == _the_keys->end() )
    {
	(*_the_keys)[key] = val ;
	return val ;
    }
    else
    {
	(*i).second = val ;
	return val ;
    }
    return "" ;
}

/** @brief allows the user to set key/value pairs from within the application.
 *
 * This method allows users of DODSKeys to set key/value pairs from within the
 * application, such as for testing purposes, key/value pairs from the command
 * line, etc...
 *
 * If the key is already set then this value replaces the value currently held
 * in the keys map.
 *
 * @param pair the key/value pair passed as key=value
 * @return returns the value for the key, empty string if unsuccessful
 */
string
DODSKeys::set_key( const string &pair )
{
    string key ;
    string val ;
    break_pair( pair.c_str(), key, val ) ;
    return set_key( key, val ) ;
}

/** @brief Retrieve the value of a given key, if set.
 *
 * This method allows the user of DODSKeys to retrieve the value of the
 * specified key.
 *
 * @param s The key the user is looking for
 * @param found Set to true of the key is set or false if the key is not set.
 * The value of a key can be set to the empty string, which is why this
 * boolean is provided.
 * @return Returns the value of the key, empty string if the key is not set.
 */
string
DODSKeys::get_key( const string& s, bool &found ) 
{
    map<string,string,less<string> >::iterator i;
    i=_the_keys->find(s);
    if(i!=_the_keys->end())
    {
	found = true ;
	return (*i).second;
    }
    else
    {
	found = false ;
	return "";
    }
}

/** @brief displays all key/value pairs defined to standard output.
 *
 * This method allows the user to see all of the key/value pairs that are
 * currently defined. The output looks like:
 *
 * <PRE>
 * key: "key", value: "value"
 * </PRE>
 */
void
DODSKeys::show_keys()
{
    map<string,string,less<string> >::iterator i ;
    for( i= _the_keys->begin(); i != _the_keys->end(); ++i )
    {
	cout << "key: \"" << (*i).first
	     << "\", value: \"" << (*i).second << "\""
	     << endl ;
    }
}

// $Log: DODSKeys.cc,v $
// Revision 1.5  2005/04/19 17:59:25  pwest
// providing mechanism to iterate through keys using const iterator from outside the class
//
// Revision 1.4  2005/03/23 23:45:00  pwest
// including stdio.h for Sun machines, okay on Linux and Win32
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
