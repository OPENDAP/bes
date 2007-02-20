// SayReporter.cc

#include "SayReporter.h"
#include "TheBESKeys.h"
#include "BESLogException.h"
#include "SayResponseNames.h"

SayReporter::SayReporter()
    : BESReporter(),
      _file_buffer( 0 )
{
    bool found = false ;
    _log_name = TheBESKeys::TheKeys()->get_key( "Say.LogName", found );
    if( _log_name == "" )
    {
	throw BESLogException( "can not determine Say log name", __FILE__, __LINE__ ) ;
    }
    else
    {
	_file_buffer = new ofstream( _log_name.c_str(), ios::out | ios::app ) ;
	if( !(*_file_buffer) )
	{
	    string s = "can not open Say log file " + _log_name ;;
	    throw BESLogException( s, __FILE__, __LINE__ ) ;
	} 
    }
}

SayReporter::~SayReporter()
{
    if( _file_buffer )
    {
	delete _file_buffer ;
	_file_buffer = 0 ;
    }
}

void
SayReporter::report( const BESDataHandlerInterface &dhi )
{
    const time_t sctime = time( NULL ) ;
    const struct tm *sttime = localtime( &sctime ) ; 
    char zone_name[10] ;
    strftime( zone_name, sizeof( zone_name ), "%Z", sttime ) ;
    char *b = asctime( sttime ) ;
    *(_file_buffer) << "[" << zone_name << " " ;
    for(register int j = 0; b[j] != '\n'; j++ )
	*(_file_buffer) << b[j] ;
    *(_file_buffer) << "] " ;

    BESDataHandlerInterface::data_citer i = dhi.data_c().find( SAY_WHAT ) ;
    string say_what = (*i).second ;
    i = dhi.data_c().find( SAY_TO ) ;
    string say_to = (*i).second ;
    *(_file_buffer) << "\"" << say_what << "\" said to \"" << say_to << "\""
                    << endl ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * the containers stored in this volatile list.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
SayReporter::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SayReporter::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "Say log name: " << _log_name << endl ;
    BESIndent::UnIndent() ;
}

