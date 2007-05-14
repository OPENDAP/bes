// infoT.h

#ifndef I_REQLISTT_H
#define I_REQLISTT_H

#include "baseApp.h"

class infoT : public baseApp {
public:
                                infoT(void) : baseApp() {}
    virtual                     ~infoT(void) {}
    virtual int			run(void);
};

#endif

