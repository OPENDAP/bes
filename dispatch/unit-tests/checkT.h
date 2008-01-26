// checkT.h

#ifndef I_checkT_H
#define I_checkT_H

#include "baseApp.h"

class checkT : public baseApp {
public:
                                checkT(void) : baseApp() {}
    virtual                     ~checkT(void) {}
    virtual int			run(void);
};

#endif

