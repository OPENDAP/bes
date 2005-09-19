// DODSLog.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>
#include <time.h>
#include <string>
#include <unistd.h>
#include "DODSLog.h"
#include "TheDODSKeys.h"
#include "DODSLogException.h"

using std::cerr ;
using std::endl ;
using std::flush ;

DODSLog *DODSLog::_instance = 0 ;

/** @brief constructor that sets up logging for the application.
 *
 * Sets up logging for the application by opening up the logging file and
 * determining verbose logging.
 *
 * The file name is determined using the DODSKeys mechanism. The key used is
 * DODS.LogName. The application must be able to write to this directory/file.
 *
 * Verbose logging is determined also using the DODSKeys mechanism. The key
 * used is DODS.LogVerbose.
 *
 * @throws DODSLogException if DODSLogName is not set or if there are
 * problems opening or writing to the log file.
 * @see DODSKeys
 */
DODSLog::DODSLog()
    : _flushed( 1 ),
      _file_buffer( 0 ),
      _suspended( 0 ),
      _verbose( false )
{
    _suspended = 0 ;
    bool found = false ;
    string log_name = TheDODSKeys::TheKeys()->get_key( "DODS.LogName", found ) ;
    if( log_name=="" )
    {
	cerr << "DODS: can not start log facility because can not determine DODS log name.\n";
	DODSLogException e;
	e.set_error_description("can not determine DODS log name");
	throw e;
    }
    else
    {
	_file_buffer=new ofstream(log_name.c_str(), ios::out | ios::app);
	if (!(*_file_buffer))
	{
	    cerr << "DODS Fatal; can not open DODS log file.\n";
	    DODSLogException le;
	    le.set_error_description("can not open log file");
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
    string verbose = TheDODSKeys::TheKeys()->get_key( "DODS.LogVerbose", found ) ;
    if( verbose == "YES" || verbose == "Yes" || verbose == "yes" )
    {
	_verbose = true ;
    }
}

/** @brief Cleans up the logging mechanism
 *
 * Cleans up the logging mechanism by closing the log file.
 */
DODSLog:: ~DODSLog()
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
DODSLog::dump_time()
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
DODSLog& DODSLog::operator<<(string &s) 
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
DODSLog& DODSLog::operator<<(const string &s) 
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
DODSLog& DODSLog::operator<<(char *val) 
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
DODSLog& DODSLog::operator<<(const char *val) 
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
DODSLog& DODSLog::operator<<(int val) 
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
DODSLog& DODSLog::operator<<(char val) 
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
DODSLog& DODSLog::operator<<(long val) 
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
DODSLog& DODSLog::operator<<(unsigned long val) 
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
DODSLog& DODSLog::operator<<(double val) 
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
 * to objects of the class DODSLog.
 */
DODSLog& DODSLog::operator<<(p_ostream_manipulator val) 
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
 * functions oct, dec and hex can be applied to objects of the class DODSLog.
 */ 
DODSLog& DODSLog::operator<<(p_ios_manipulator val) 
{ 
    if (!_suspended)
	(*_file_buffer)<<val;
    return *this;
}

DODSLog *
DODSLog::TheLog()
{
    if( _instance == 0 )
    {
	_instance = new DODSLog ;
    }
    return _instance ;
}

// $Log: DODSLog.cc,v $
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
