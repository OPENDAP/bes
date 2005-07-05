// TestReporter.h

#ifndef A_TestReporter_h
#define A_TestReporter_h 1

#include "DODSReporter.h"

class TestReporter : public DODSReporter
{
private:
    string		_name ;
public:
			TestReporter( string name ) ;
    virtual		~TestReporter() ;

    virtual void	report( const DODSDataHandlerInterface &dhi ) ;
    virtual string	get_name() { return _name ; }
} ;

#endif // A_TestReporter_h

