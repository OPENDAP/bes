// TestReporter.h

#ifndef A_TestReporter_h
#define A_TestReporter_h 1

#include "BESReporter.h"

class TestReporter : public BESReporter
{
private:
    string		_name ;
public:
			TestReporter( string name ) ;
    virtual		~TestReporter() ;

    virtual void	report( BESDataHandlerInterface &dhi ) ;
    virtual string	get_name() { return _name ; }

    virtual void	dump( ostream &strm ) const { }
} ;

#endif // A_TestReporter_h

