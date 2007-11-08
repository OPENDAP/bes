// regexT.h

#ifndef I_regexT_H
#define I_regexT_H

#include <string>

using std::string ;

#include "baseApp.h"

class regexT : public baseApp {
private:
    string			_keyFile ;
public:
                                regexT(void) : baseApp() {}
    virtual                     ~regexT(void) {}
    virtual int			run(void);
};

#endif

