// reqlistT.h

#ifndef I_REQLISTT_H
#define I_REQLISTT_H

#include "baseApp.h"

class reqlistT : public baseApp {
public:
                                reqlistT(void) : baseApp() {}
    virtual                     ~reqlistT(void) {}
    virtual int			run(void);
};

#endif

