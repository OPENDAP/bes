// DODSReturnManager.h

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

#ifndef I_DODSReturnManager_h
#define I_DODSReturnManager_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "DODSDataHandlerInterface.h"
#include "DODSTransmitter.h"

class DODSTransmitter ;

/** @brief ReturnManager holds the list of response object transmitter that
 * knows how to transmit response objects in particular ways.
 *
 */
class DODSReturnManager {
private:
    static DODSReturnManager *	_instance ;

    map< string, DODSTransmitter * > _transmitter_list ;
protected:
				DODSReturnManager() ;
public:
    virtual			~DODSReturnManager() ;

    typedef map< string, DODSTransmitter * >::const_iterator Transmitter_citer ;
    typedef map< string, DODSTransmitter * >::iterator Transmitter_iter ;

    virtual bool		add_transmitter( const string &name,
						 DODSTransmitter *transmitter );
    virtual bool		del_transmitter( const string &name) ;
    virtual DODSTransmitter *	find_transmitter( const string &name ) ;

    static DODSReturnManager *	TheManager() ;
};

#endif // I_DODSReturnManager_h

// $Log: DODSReturnManager.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
