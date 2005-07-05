// initT.h

#ifndef I_APPT_H
#define I_APPT_H

#include <baseApp.h>

class initT : public baseApp {
public:
                                initT(void) : baseApp() {}
    virtual                     ~initT(void) {}
    virtual int			run(void);
};

#endif

