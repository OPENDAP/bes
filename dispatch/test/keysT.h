// keysT.h

#ifndef I_keysT_H
#define I_keysT_H

#include <string>

using std::string ;

#include "baseApp.h"

class keysT : public baseApp {
private:
    string			_keyFile ;
public:
                                keysT(void) : baseApp() {}
    virtual                     ~keysT(void) {}
    virtual int			initialize( int argC, char **argV ) ;
    virtual int			run(void);
};

#endif

