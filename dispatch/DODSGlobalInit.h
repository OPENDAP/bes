// DODSGlobalInit.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSGlobalInit_h
#define I_DODSGlobalInit_h 1

#include "DODSInitializer.h"
#include "DODSInitFuns.h"

/** @brief Provides for the orderly initialization and termination of global objects.
 *
 * DODSGlobalInit is an implementation of the abstration DODSInitializer
 * that provides the orderly initialization and termination of global
 * objects. C++ does not provide such a mechanism, as global objects are
 * created in random order. This gives the user more control over that
 * ordering.
 *
 * For a complete understanding of this global initialization mechanism
 * please see the DODSGlobalIQ documentation.
 *
 * @see DODSGlobalIQ
 */
class DODSGlobalInit : public DODSInitializer {
public:
                                DODSGlobalInit(DODSInitFun, DODSTermFun,
					       DODSInitializer *nextInit,
					       long lvl);
    virtual                     ~DODSGlobalInit(void);
    virtual bool           	initialize(int argc, char **argv);
    virtual bool           	terminate(void);
private:
    DODSInitFun                 _initFun;
    DODSTermFun                 _termFun;
    DODSInitializer *           _nextInit;
};

#endif // I_DODSGlobalInit_h

// $Log: DODSGlobalInit.h,v $
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
