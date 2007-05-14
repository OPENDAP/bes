// TestReporter.cc

#include <iostream>

using std::cout ;
using std::endl ;

#include "TestReporter.h"

TestReporter::TestReporter( string name )
    : BESReporter(),
      _name( name )
{
}

TestReporter::~TestReporter()
{
}

void
TestReporter::report( const BESDataHandlerInterface &dhi )
{
    cout << _name << " reporting" << endl ;
}

