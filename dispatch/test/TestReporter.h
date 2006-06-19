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

    virtual void	report( const BESDataHandlerInterface &dhi ) ;
    virtual string	get_name() { return _name ; }
} ;

#endif // A_TestReporter_h

