// resplistT.h

#ifndef I_RESPLISTT_H
#define I_RESPLISTT_H

#include "baseApp.h"

class resplistT : public baseApp {
public:
                                resplistT(void) : baseApp() {}
    virtual                     ~resplistT(void) {}
    virtual int			run(void);
};

#endif

