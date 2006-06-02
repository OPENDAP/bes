// DODSGlobalInit.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

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
				int lvl )
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
