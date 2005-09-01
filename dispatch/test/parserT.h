// parserT.h

#ifndef I_REQLISTT_H
#define I_REQLISTT_H

#include "baseApp.h"

class parserT : public baseApp {
public:
                                parserT(void) : baseApp() {}
    virtual                     ~parserT(void) {}
    virtual int			run(void);
};

#endif

