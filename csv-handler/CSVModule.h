// CSVModule.h

#ifndef I_CSVModule_H
#define I_CSVModule_H 1

#include "BESAbstractModule.h"

class CSVModule : public BESAbstractModule
{
public:
    				CSVModule() {}
    virtual		    	~CSVModule() {}
    virtual void		initialize( const string &modname ) ;
    virtual void		terminate( const string &modname ) ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // I_CSVModule_H

