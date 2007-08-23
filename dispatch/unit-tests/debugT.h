// debugT.h

#ifndef I_debugT_H
#define I_debugT_H

#include <string>

using std::string ;

#include "baseApp.h"

class debugT : public baseApp {
private:
    string			_tryme ;
    				debugT() {}
public:
                                debugT( const string &s )
				    : _tryme( s ),
				      baseApp() {}
    virtual                     ~debugT(void) {}
    virtual int			run(void);
};

#endif

