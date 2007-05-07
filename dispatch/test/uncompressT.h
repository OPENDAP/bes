// uncompressT.h

#ifndef I_uncompressT_H
#define I_uncompressT_H

#include "baseApp.h"

class uncompressT : public baseApp {
public:
                                uncompressT(void) : baseApp() {}
    virtual                     ~uncompressT(void) {}
    virtual int			run(void);
};

#endif

