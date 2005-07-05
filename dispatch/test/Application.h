// Application.h

#ifndef A_Application_H
#define A_Application_H

class Application {
public:
    virtual int			main(int argC, char **argV) = 0;
    virtual int			initialize(int argC, char **argV) = 0;
    virtual int			run(void) = 0;
    virtual int			terminate(int sig = 0) = 0;
    static Application *	TheApplication(void) { return _theApplication; }
protected:
    static Application *	_theApplication;
                                Application(void) {};
};

#endif

