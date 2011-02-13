// DapModule.h

#ifndef I_DapModule_H
#define I_DapModule_H 1

#include "BESAbstractModule.h"

class DapModule : public BESAbstractModule
{
public:
    				DapModule() {}
    virtual		    	~DapModule() {}
    virtual void		initialize( const string &modname ) ;
    virtual void		terminate( const string &modname ) ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // I_DapModule_H

