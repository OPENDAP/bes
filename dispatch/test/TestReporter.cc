// TestReporter.cc

#include <iostream>

using std::cout ;
using std::endl ;

#include "TestReporter.h"

TestReporter::TestReporter( string name )
    : DODSReporter(),
      _name( name )
{
}

TestReporter::~TestReporter()
{
}

void
TestReporter::report( const DODSDataHandlerInterface &dhi )
{
    cout << _name << " reporting" << endl ;
}

