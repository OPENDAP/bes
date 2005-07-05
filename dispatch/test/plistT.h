// plistT.h

#ifndef I_plistT_H
#define I_plistT_H

#include "baseApp.h"

class plistT : public baseApp {
public:
                                plistT(void) : baseApp() {}
    virtual                     ~plistT(void) {}
    virtual int			run(void);
};

#endif

