// BESUsage.h

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef I_BESUsage_h
#define I_BESUsage_h 1

#include "BESResponseObject.h"
#include "BESDASResponse.h"
#include "BESDDSResponse.h"

/** @brief container for a DAS and DDS needed to write out the usage
 * information for a dataset.
 *
 * This is a container for the usage response information, which needs a DAS
 * and a DDS. An instances of BESUsage takes ownership of the das and dds
 * passed to it and deletes it in the destructor.
 *
 * @see BESResponseObject
 * @see DAS
 * @see DDS
 */
class BESUsage : public BESResponseObject
{
private:
    BESDASResponse *_das ;
    BESDDSResponse *_dds ;

	BESUsage() {}

public:
    BESUsage( BESDASResponse *das, BESDDSResponse *dds )
	: _das( das ), _dds( dds ) {}
    virtual ~BESUsage() {
	   if( _das ) delete _das ;
	   if( _dds ) delete _dds ;
    }

    BESDASResponse *get_das() { return _das ; }
    BESDDSResponse *get_dds() { return _dds ; }

    /** @brief dumps information about this object
     *
     * Displays the pointer value of this instance along with the das object
     * created
     *
     * @param strm C++ i/o stream to dump the information to
     */
    virtual void dump( ostream &strm ) const {
        strm << BESIndent::LMarg << "BESUsage::dump - ("
			 << (void *)this << ")" << endl ;

	BESIndent::Indent() ;
	strm << BESIndent::LMarg << "das: " << *_das << endl ;
	strm << BESIndent::LMarg << "dds: " << *_dds << endl ;
	BESIndent::UnIndent() ;
    }
} ;

#endif // I_BESUsage_h

