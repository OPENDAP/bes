// DODSGlobalInit.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <DODSGlobalInit.h>
#include <DODSInitList.h>

/** @brief Construct an initializer object that will handle the
 * initialization and termination of a global object.
 *
 * To use this object please refer to the DODSGlobalIQ documentation. It
 * will provide the information necessary to order the initialization of
 * your global objects including step-by-step instructions.
 *
 * @param initFun Function used to initialize your global object
 * @param termFun Function used to terminate, destroy, or clean up your
 * global object
 * @param nextInit The Next DODSInitializer object that holds on to the
 * initialization and termination functions for another global object
 * @param lvl initialization level. There are different levels of
 * initialization, which provides the ordering. Objects at the same level
 * are initialized in random order.
 * @see DODSGlobalIQ
 */
DODSGlobalInit::DODSGlobalInit( DODSInitFun initFun,
				DODSTermFun termFun,
				DODSInitializer *nextInit,
				long lvl )
{
    _initFun = initFun;
    _termFun = termFun;
    _nextInit = nextInit;
    DODSGlobalInitList[lvl] = this;
}

DODSGlobalInit::~DODSGlobalInit() {
}

/** @brief Method used to traverse a level of initialization functions
 *
 * There can be multiple levels of initialization. Level 0 will be the
 * first global initialization functions run, level 1 will be the next set
 * of initialization functions run. This method will run the level of
 * objects for the level specified in the constructor.
 *
 * Again, see the DODSGlobalIQ documentation for a full description of how to
 * use the global initialization mechanism.
 *
 * @param argc number of arguments passed in the argv argument list. This is
 * the same as the command line argc.
 * @param argv the arguments passed to the initialization function. This is
 * the same as the command line arguments argv.
 * @return Returns true if successful or false if not successful and the
 * application should terminate. If there is a problem but the application
 * can continue to run then return true.
 * @see DODSGlobalIQ
 */
bool
DODSGlobalInit::initialize(int argc, char **argv) {
    bool retVal = true;
    retVal = _initFun(argc, argv);
    if(retVal == true) {
	if(_nextInit) {
	    retVal = _nextInit->initialize(argc, argv);
	}
    }
    return retVal;
}

/** @brief Method used to traverse a level of termination functions
 *
 * There can be multiple levels of initialization. Level 0 will be the
 * first global termination functions run, level 1 will be the next set
 * of termination functions run. This method will run the level of
 * objects for the level specified in the constructor.
 *
 * Again, see the DODSGlobalIQ documentation for a full description of how to
 * use the global initialization mechanism.
 *
 * @return Returns true if successful or false if not successful and the
 * application should terminate. If there is a problem but the application
 * can continue to run then return true.
 * @see DODSGlobalIQ
 */
bool
DODSGlobalInit::terminate() {
    if(_termFun) {
	_termFun();
    }
    if(_nextInit) {
	_nextInit->terminate();
    }
    return true;
}

// $Log: DODSGlobalInit.cc,v $
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
