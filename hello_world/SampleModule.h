// SampleModule.h

#ifndef I_SampleModule_H
#define I_SampleModule_H 1

#include "BESAbstractModule.h"

class SampleModule : public BESAbstractModule
{
public:
    				SampleModule() {}
    virtual		    	~SampleModule() {}
    virtual void		initialize( const string &modname ) ;
    virtual void		terminate( const string &modname ) ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // I_SampleModule_H

