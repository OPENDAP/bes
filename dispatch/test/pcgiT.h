// pcgiT.h

#ifndef I_pcgiT_H
#define I_pcgiT_H

#include "baseApp.h"

class pcgiT : public baseApp {
public:
                                pcgiT(void) : baseApp() {}
    virtual                     ~pcgiT(void) {}
    virtual int			run(void);
};

#endif

