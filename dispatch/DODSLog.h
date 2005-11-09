// DODSLog.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#ifndef DODSLog_h_
#define DODSLog_h_ 1

#include <fstream>
#include <string>

using std::ofstream ;
using std::ios ;
using std::ostream ;
using std::string ;

/** @brief Provides a mechanism for applications to log information to an
 * external file.
 *
 * DODSLog provides a mechanism for applications to log information to an
 * external file, such as debugging information. This file is defined in the
 * DODSKeys mechanism using the key DODS.LogName.
 *
 * Also provides a mechanism to define whether debugging information should be
 * verbose or not using the DODSKeys key/value pair DODS.LogVerbose.
 *
 * Logging can also be suspended and resumed using so named methods.
 *
 * DODSLog is used similar to cerr and cout using the overloaded operator <<.
 *
 * <PRE>
 * if( DODSLog::TheLog()->is_verbose() )
 * {
 *     *(DODSLog::TheLog()) << "This is some information to be logged"
 * 		  << endl ;
 * }
 * </PRE>
 *
 * Types of data that can be logged include:
 * <UL>
 * <LI>string
 * <LI>char *
 * <LI>const char *
 * <LI>int
 * <LI>char
 * <LI>long
 * <LI>unsigned long
 * <LI>double
 * <LI>stream manipulators endl, ends and flush
 * <LI>ios manipulators hex, oct, dec
 * </UL>
 *
 * DODSLog provides a static method for access to a single DODSLog object,
 * TheLog.
 *
 * @see DODSKeys
 */
class DODSLog 
{
private:
    static DODSLog *	_instance ;
    int			_flushed ;
    ofstream *		_file_buffer;
    // Flag to indicate the object is not routing data to its associated stream
    int			_suspended ;
    // Flag to indicate whether to log verbose messages
    bool		_verbose ;
protected:
    DODSLog(); 

    // Dumps the current system time.
    void		dump_time() ;
public:
    ~DODSLog();

    /** @brief Suspend logging of any information until resumed.
     *
     * This method suspends any logging of information. If already suspended
     * then nothing changes, logging is still suspended.
     */
    void suspend()
    {
	_suspended = 1 ;
    }

    /** @brief Resumes logging after being suspended.
     *
     * This method resumes logging after suspended by the user. If logging was
     * not already suspended this method does nothing.
     */
    void resume()
    {
	_suspended = 0 ;
    }

    /** @brief turn on verbose logging
     *
     * This method turns on verbose logging, providing applications the
     * ability to log more detailed debugging information. If verbose is
     * already turned on then nothing is changed.
     */
    void verbose_on()
    {
	_verbose = true ;
    }

    /** @brief turns off verbose logging
     *
     * This methods turns off verbose logging. If verbose logging was not
     * already turned on then nothing changes.
     */
    void verbose_off()
    {
	_verbose = false ;
    }

    /** @brief Returns true if verbose logging is requested.
     *
     * This method returns true if verbose logging has been requested either
     * by setting the DODSKeys key/value pair DODS.LogVerbose=value or by
     * turning on verbose logging using the method verbose_on.
     *
     * If DODS.LogVerbose is set to Yes, YES, or yes then verbose logging is
     * turned on. If set to anything else then verbose logging is not turned
     * on.
     *
     * @return true if verbose logging has been requested.
     * @see verbose_on
     * @see verbose_off
     * @see DODSKeys
     */
    bool is_verbose()
    {
	return _verbose ;
    }

    /// Defines a data type p_ios_manipulator "pointer to function that takes ios& and returns ios&".
    typedef ios& (*p_ios_manipulator) (ios&);
    /// Defines a data type p_ostream_manipulator "pointer to function that takes ostream& and returns ostream&".
    typedef ostream& (*p_ostream_manipulator) (ostream&);

    DODSLog& operator << (string&);
    DODSLog& operator << (const string&);
    DODSLog& operator << (char*);
    DODSLog& operator << (const char*);
    DODSLog& operator << (int);
    DODSLog& operator << (char);
    DODSLog& operator << (long);
    DODSLog& operator << (unsigned long);
    DODSLog& operator << (double);

    DODSLog& operator<<(p_ostream_manipulator); 
    DODSLog& operator<<(p_ios_manipulator); 

    static DODSLog *TheLog() ;
};

#endif // DODSLog_h_

// $Log: DODSLog.h,v $
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
