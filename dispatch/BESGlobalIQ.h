// BESGlobalIQ.h

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

#ifndef E_BESGlobalIQ_h
#define E_BESGlobalIQ_h 1

/** @brief Mechanism to provide for the orderly initialization and
 * termination of global objects/functions.
 *
 * C++ does not provide a good method for the initialization and termination
 * of global objects. The main problem is ordering. This mechanism provides
 * for the order of initialization and an easy to use means of initializing
 * and terminating global objects. Only those global objects that are used
 * within the application are linked into the application, just as in normal
 * global objects. Termination of global objects using this mechanism occurs
 * in the reverse order that they were initialized.
 *
 * SETTING UP FOR GLOBAL INITIALIZATION
 *
 * To set up a global object to be initialized you first need to determine
 * in what order the global object should be initialized. The header file
 * BESInitOrder lists the current order of initialization for BES
 * objects.
 *
 * Levels of initialization can have multiple objects being initialized.
 * Initialization occurs in the order of the level. Within the same level
 * initialization is random.
 *
 * INTEGRATING THE INITIALIZATION CODE
 *
 * Once you have the correct order for your object, it is time to set up the
 * code that will initialize and terminate your global object. This code
 * should reside in a header and source file named for the global object.
 * For example, TheBESLog.[hC] is used to initialize TheBESLog instance.
 * The header file merely declares a pointer to the global object just as
 * you would for normal global objects.
 *
 * <pre>
    // TheGlobalObject.h

    #ifndef E_TheGlobalObject_H
    #define E_TheGlobalObject_H

    #include <GlobalObject.h>

    extern GlobalObject *TheGlobalObject;

    #endif
 * </pre>
 *
 * The source file is where you implement the initialization function and,
 * if necessary, the termination function.
 *
 * <pre>
    // TheGlobalObject.C

    #include <TheGlobalObject.h>
    #include <globalObject.h>
    #include <BESInitList.h>

    GlobalObject *TheGlobalObject = 0;

    static bool
    buildTheGlobalObject(int argc, char **argv)
    {
	TheGlobalObject = new globalObject(...) ;
	return true ;
    }

    static bool
    destroyTheGlobalObject(void)
    {
	if(TheGlobalObject) delete TheGlobalObject ;
	return true ;
    }

    FUNINITQUIT(buildTheGlobalObject, destroyTheGlobalObject, APPL_INIT2)
 * </pre>
 *
 * The macro FUNINITQUIT takes an initialization function and a termination
 * function to be used in the order specified by the order macro APPL_INIT2,
 * which would be defined in one of TheInit.h header files. If there is
 * nothing to terminate then you could have used the following macro:
 *
 * <pre>
    FUNINIT(buildTheGlobalObject, APPL_INIT2)
 * </pre>
 *
 * The macro FUNINIT only takes an initialization function, no termination
 * function, and the order macro APPL_INIT2.
 *
 * Each time one of these two macros is used a BESGlobalInit object is
 * instantiated, passing the specified initialization function and
 * terminiation function (if one is needed) to the constructor along with
 * the level at which these functions will be called and the current
 * BESGlobalInit object first in line for that level. Remember, multiple
 * initialization functions can be called at any given level, and the
 * calling of these functions within a level is random. What this mechanism
 * provides is an ordering for objects in different levels.
 *
 * HOW TO INCORPORATE THE CODE INTO YOUR APPLICATION
 *
 * In the main routine of your application the function BESGlobalInit is
 * run, which causes the initialization of the global objects to occur.
 * Before exiting your application the function BESGlobalQuit is run, which
 * causues the terminiation functions to be called, cleaning up all of your
 * global objects.
 */

class BESGlobalIQ
{
private:
    static bool is_initialized ;
public:
    static bool BESGlobalInit( int argc, char **argv ) ;
    static bool BESGlobalQuit( void ) ;
} ;

#endif // E_BESGlobalIQ_h

