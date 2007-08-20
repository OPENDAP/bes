// encodeT.h

#ifndef I_encodeT_H
#define I_encodeT_H

#include "baseApp.h"

class encodeT : public baseApp {
public:
                                encodeT(void) : baseApp() {}
    virtual                     ~encodeT(void) {}
    virtual int			run(void);
};

#endif

