// BESExceptionManager.h

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

#ifndef BESExceptionManager_h_
#define BESExceptionManager_h_ 1

#include <list>

using std::list ;

#include "BESObj.h"
#include "BESDataHandlerInterface.h"
#include "BESInfo.h"

class BESException ;

typedef int (*p_bes_ehm)( BESException &e, BESDataHandlerInterface &dhi ) ;

class BESExceptionManager : public BESObj
{
private:
    typedef list< p_bes_ehm >::const_iterator ehm_citer ;
    typedef list< p_bes_ehm >::iterator ehm_iter ;
    list< p_bes_ehm >	_ehm_list ;
    static BESExceptionManager *_instance ;
protected:
    				BESExceptionManager() ;
    virtual			~BESExceptionManager() ;
public:
    virtual void		add_ehm_callback( p_bes_ehm ehm ) ;
    virtual int			handle_exception( BESException &e,
					      BESDataHandlerInterface &dhi ) ;

    virtual void		dump( ostream &strm ) const ;

    static BESExceptionManager *TheEHM() ;
} ;

#endif // BESExceptionManager_h_

