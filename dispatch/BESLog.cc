// BESLog.cc

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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <iostream>
#include <time.h>
#include <string>
#include <unistd.h>
#include "BESLog.h"
#include "TheBESKeys.h"
#include "BESLogException.h"

using std::cerr ;
using std::endl ;
using std::flush ;

BESLog *BESLog::_instance = 0 ;

/** @brief constructor that sets up logging for the application.
 *
 * Sets up logging for the application by opening up the logging file and
 * determining verbose logging.
 *
 * The file name is determined using the BESKeys mechanism. The key used is
 * OPeNDAP.LogName. The application must be able to write to this directory/file.
 *
 * Verbose logging is determined also using the BESKeys mechanism. The key
 * used is OPeNDAP.LogVerbose.
 *
 * @throws BESLogException if BESLogName is not set or if there are
 * problems opening or writing to the log file.
 * @see BESKeys
 */
BESLog::BESLog()
    : _flushed( 1 ),
      _file_buffer( 0 ),
      _suspended( 0 ),
      _verbose( false )
{
    _suspended = 0 ;
    bool found = false ;
    string log_name = TheBESKeys::TheKeys()->get_key( "OPeNDAP.LogName", found ) ;
    if( log_name=="" )
    {
	string err = (string)"OPeNDAP Fatal: unable to determine log file name."
	             + " Please set OPeNDAP.LogName in your initialization file" ;
	cerr << err << endl ;
	BESLogException e;
	e.set_error_description( err ) ;
	throw e;
    }
    else
    {
	_file_buffer=new ofstream(log_name.c_str(), ios::out | ios::app);
	if (!(*_file_buffer))
	{
	    string err = (string)"OPeNDAP Fatal; can not open log file "
	                 + log_name + "." ;
	    cerr << err << endl ;
	    BESLogException le;
	    le.set_error_description( err ) ;
	    throw le;
	} 
	/*
	if (_flushed)
	{
	    dump_time();
	    _flushed=0;
	}
	*/
    }
    string verbose = TheBESKeys::TheKeys()->get_key( "OPeNDAP.LogVerbose", found ) ;
    if( verbose == "YES" || verbose == "Yes" || verbose == "yes" )
    {
	_verbose = true ;
    }
}

/** @brief Cleans up the logging mechanism
 *
 * Cleans up the logging mechanism by closing the log file.
 */
BESLog:: ~BESLog()
{
    _file_buffer->close();
    delete _file_buffer;
    _file_buffer = 0 ;
}

/** @brief Protected method that dumps the date/time to the log file
 *
 * The time is dumped to the log file in the format:
 *
 * [MDT Thu Sep  9 11:05:16 2004 id: &lt;pid&gt;]
 */
void
BESLog::dump_time()
{
    const time_t sctime=time(NULL);
    const struct tm *sttime=localtime(&sctime); 
    char zone_name[10];
    strftime(zone_name, sizeof(zone_name), "%Z", sttime);
    char *b=asctime(sttime);
    (*_file_buffer)<<"["<<zone_name<<" ";
    for (register int j=0; b[j]!='\n'; j++)
	(*_file_buffer)<<b[j];
    pid_t thepid = getpid() ;
    (*_file_buffer)<<" id: "<<thepid<<"] ";
    _flushed = 0 ;
}

/** @brief Overloaded inserter that writes the specified string.
 *
 * @param s string to write to the log file
 */
BESLog& BESLog::operator<<(string &s) 
{
    if (!_suspended)
    {
	if (_flushed)
	    dump_time();
	(*_file_buffer) << s;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified const string.
 *
 * @param s const string to write to the log file
 */
BESLog& BESLog::operator<<(const string &s) 
{
    if (!_suspended)
    {
	if (_flushed)
	    dump_time();
	(*_file_buffer) << s;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified char *.
 *
 * @param val char * to write to the log file
 */
BESLog& BESLog::operator<<(char *val) 
{
    if (!_suspended)
    {
	if (_flushed)
	    dump_time();
	if( val )
	    (*_file_buffer) << val;
	else
	    (*_file_buffer) << "NULL" ;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified const char *.
 *
 * @param val const char * to write to the log file
 */
BESLog& BESLog::operator<<(const char *val) 
{
    if (!_suspended)
    {
	if (_flushed)
	{
	    dump_time();
	}
	if( val )
	    (*_file_buffer) << val;
	else
	    (*_file_buffer) << "NULL" ;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified int value.
 *
 * @param val int value to write to the log file
 */
BESLog& BESLog::operator<<(int val) 
{
    if (!_suspended)
    {
	if (_flushed)
	    dump_time();
	(*_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified char value.
 *
 * @param val char value to write to the log file
 */
BESLog& BESLog::operator<<(char val) 
{ 
    if (!_suspended)
    {
	if (_flushed)
	    dump_time();
	(*_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified long value.
 *
 * @param val long value to write to the log file
 */
BESLog& BESLog::operator<<(long val) 
{
    if (!_suspended)
    {
	if (_flushed)
	    dump_time();
	(*_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified unsigned long value.
 *
 * @param val unsigned long value to write to the log file
 */
BESLog& BESLog::operator<<(unsigned long val) 
{
    if (!_suspended)
    {
	if (_flushed)
	    dump_time();
	(*_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified double value.
 *
 * @param val double value to write to the log file
 */
BESLog& BESLog::operator<<(double val) 
{
    if (!_suspended)
    {
	if (_flushed)
	    dump_time();
	(*_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that takes stream manipulation methods.
 *
 * Overloaded inserter that can take the address of endl, flush and ends
 * functions. This inserter is based on p_ostream_manipulator, therefore
 * the C++ standard functions for I/O endl, flush, and ends can be applied
 * to objects of the class BESLog.
 */
BESLog& BESLog::operator<<(p_ostream_manipulator val) 
{
    if (!_suspended)
    {
	(*_file_buffer) << val ;
	if ((val==(p_ostream_manipulator)endl) || (val==(p_ostream_manipulator)flush))
	    _flushed=1;
    }
    return *this;
}

/** @brief Overloaded inserter that takes ios methods
 *
 * Overloaded inserter that can take the address oct, dec and hex functions.
 * This inserter is based on p_ios_manipulator, therefore the C++ standard
 * functions oct, dec and hex can be applied to objects of the class BESLog.
 */ 
BESLog& BESLog::operator<<(p_ios_manipulator val) 
{ 
    if (!_suspended)
	(*_file_buffer)<<val;
    return *this;
}

BESLog *
BESLog::TheLog()
{
    if( _instance == 0 )
    {
	_instance = new BESLog ;
    }
    return _instance ;
}

// $Log: BESLog.cc,v $
// Revision 1.4  2005/02/09 19:49:33  pwest
// was trying to print null pinter
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
