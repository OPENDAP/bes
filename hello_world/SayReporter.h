// SayReporter.h

#ifndef A_SayReporter_h
#define A_SayReporter_h 1

#include <fstream>

using std::ofstream ;
using std::ios ;
using std::endl ;

#include "BESReporter.h"
#include "BESDataHandlerInterface.h"

class SayReporter : public BESReporter
{
private:
    ofstream *		_file_buffer ;
    string		_log_name ;
public:
			SayReporter() ;
    virtual		~SayReporter() ;

    virtual void	report( const BESDataHandlerInterface &dhi ) ;

    virtual void	dump( ostream &strm ) const ;
} ;

#endif // A_SayReporter_h

