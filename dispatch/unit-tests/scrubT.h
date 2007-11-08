// scrubT.h

#ifndef I_scrubT_H
#define I_scrubT_H

#include <string>

using std::string ;

#include "baseApp.h"

class scrubT : public baseApp {
private:
    string			_keyFile ;
public:
                                scrubT(void) : baseApp() {}
    virtual                     ~scrubT(void) {}
    virtual int			run(void);
};

#endif

