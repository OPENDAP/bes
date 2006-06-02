// DODSInitializer.h

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

#ifndef A_DODSInitializer_h
#define A_DODSInitializer_h 1

/** @brief Mechanism for the orderly initialization and termination of objects.
 *
 *  The DODSInitializer abstraction provides a mechanism for the
 *  initialization and termination of objects in an orderly fasion.  In
 *  many instances C++ does not provide an orderly means of initializing and
 *  destroying objects, such as during global initialization. This
 *  interface provides that mechanism and can be used for such things as
 *  global initialization and termination, thread initialization and
 *  termination, initialization of RPC calls and termination upon return,
 *  etc...
 *
 * @see DODSGlobalIQ
 * @see DODSInitOrder
 * @see DODSInitList
 * @see DODSInitFuns
 */
class DODSInitializer {
public:
    virtual			~DODSInitializer() {}
    /** @brief function for the initialization of objects, such as globals.
     * 
     * @param argc number of arguments passed on the command line, same as command line argc.
     * @param argv command line arguments passed to the C++ application that can be used to initialize the object.
     * @return returns true if initialization was successful, false if failed and application should exit.
     * @see GlobalIQ
     */
    virtual bool           	initialize(int argc, char **argv) = 0;

    /** @brief function for the termination of objects, such as global objects.
     *
     * @return returns true if termination was successful, false otherwise
     * @see GlobalIQ
     */
    virtual bool           	terminate(void) = 0;
};

#endif // A_DODSInitializer_h

// $Log: DODSInitializer.h,v $
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
