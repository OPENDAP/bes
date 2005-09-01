// tokenizerT.h

#ifndef I_REQLISTT_H
#define I_REQLISTT_H

#include "baseApp.h"

class tokenizerT : public baseApp {
public:
                                tokenizerT(void) : baseApp() {}
    virtual                     ~tokenizerT(void) {}
    virtual int			run(void);
};

#endif

