// DODSLog.h

// 2004 Copyright University Corporation for Atmospheric Research

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
 * if( TheDODSLog && TheDODSLog->is_verbose() )
 * {
 *     *(TheDODSLog) << "This is some information to be logged"
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
 * OpenDAP provides a global variable for access to a single DODSLog object,
 * TheDODSLog. This global object is created during global initialization
 * using DODSGlobalIQ.
 *
 * @see DODSKeys
 * @see DODSGlobalIQ
 */
class DODSLog 
{
private:
    int			_flushed ;
    ofstream *		_file_buffer;
    // Flag to indicate the object is not routing data to its associated stream
    int			_suspended ;
    // Flag to indicate whether to log verbose messages
    bool		_verbose ;
protected:
    // Dumps the current system time.
    void		dump_time() ;
public:
    DODSLog(); 
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
    DODSLog& operator << (char*);
    DODSLog& operator << (const char*);
    DODSLog& operator << (int);
    DODSLog& operator << (char);
    DODSLog& operator << (long);
    DODSLog& operator << (unsigned long);
    DODSLog& operator << (double);

    DODSLog& operator<<(p_ostream_manipulator); 
    DODSLog& operator<<(p_ios_manipulator); 
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
