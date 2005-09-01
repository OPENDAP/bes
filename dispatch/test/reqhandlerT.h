// reqhandlerT.h

#ifndef I_REQHANDLERT_H
#define I_REQHANDLERT_H

#include "baseApp.h"

class reqhandlerT : public baseApp {
public:
                                reqhandlerT(void) : baseApp() {}
    virtual                     ~reqhandlerT(void) {}
    virtual int			run(void);
};

#endif

