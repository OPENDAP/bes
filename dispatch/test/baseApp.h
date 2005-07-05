// baseApp.h

#ifndef B_baseApp_H
#define B_baseApp_H

#include <Application.h>

class baseApp : public Application {
public:
                                baseApp(void);
    virtual                     ~baseApp(void);
    virtual int			main(int argC, char **argV);
    virtual int			initialize(int argC, char **argV);
    virtual int			run(void);
    virtual int			terminate(int sig = 0);
private:
};

#endif

