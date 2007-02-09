// SayModule.h

#ifndef I_SayModule_H
#define I_SayModule_H 1

#include "BESAbstractModule.h"

class SayModule : public BESAbstractModule
{
public:
    				SayModule() {}
    virtual		    	~SayModule() {}
    virtual void		initialize( const string &modname ) ;
    virtual void		terminate( const string &modname ) ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // I_SayModule_H

