// DefineResponseHandler.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
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

#ifndef I_DefineResponseHandler_h
#define I_DefineResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that creates a definition given container names
 * and optionally constraints and attributes for each of those containers.
 *
 * A request looks like:
 *
 * define &lt;def_name&gt; as &lt;container_list&gt;
 * <BR />
 * &nbsp;&nbsp;[where &lt;container_x&gt;.constraint="&lt;constraint&gt;"]
 * <BR />
 * &nbsp;&nbsp;[,&lt;container_x&gt;.attributes="&lt;attrs&gt;"]
 * <BR />
 * &nbsp;&nbsp;[aggregate by "&lt;aggregation_command&gt;"];
 *
 * where container_list is a list of containers representing points of data,
 * such as a file. For each container in the container_list the user can
 * specify a constraint and a list of attributes. You need not specify a
 * constraint for a given container or a list of attributes. If just
 * specifying a constraint then leave out the attributes. If just specifying a
 * list of attributes then leave out the constraint. For example:
 *
 * define d1 as container_1,container_2
 * <BR />
 * &nbsp;&nbsp;where container_1.constraint="constraint1"
 * <BR />
 * &nbsp;&nbsp;,container_2.constraint="constraint2"
 * <BR />
 * &nbsp;&nbsp;,container_2.attributes="attr1,attr2";
 *
 * It adds the new definition to the list. If a definition already exists
 * in the list with the given name then it is removed first and the new one is
 * added. The definition is currently available through the life of the server
 * process and is not persistent.
 *
 * @see DODSDefine
 * @see DODSDefineList
 * @see DODSContainer
 * @see DODSTransmitter
 */
class DefineResponseHandler : public DODSResponseHandler
{
public:
				DefineResponseHandler( string name ) ;
    virtual			~DefineResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *DefineResponseBuilder( string handler_name ) ;
};

#endif // I_DefineResponseHandler_h

// $Log: DefineResponseHandler.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
