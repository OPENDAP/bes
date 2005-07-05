// encodeT.h

#ifndef I_encodeT_H
#define I_encodeT_H

#include <string>

using std::string ;

#include "baseApp.h"

class encodeT : public baseApp
{
private:
    bool			_test ;
    string			_app ;
    string			_action ;
    string			_var1 ;
    string			_var2 ;
public:
                                encodeT(void) : baseApp(), _test( false ) {}
    virtual                     ~encodeT(void) {}
    virtual int			initialize(int argC, char **argV);
    virtual int			run_test(void);
    virtual int			run(void);
};

#endif

