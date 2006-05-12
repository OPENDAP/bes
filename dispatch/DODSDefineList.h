// DODSDefineList.h

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

#ifndef I_DODSDefineList_h
#define I_DODSDefineList_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

class DODSDefine ;
class DODSInfo ;

class DODSDefineList {
private:
    static DODSDefineList *	_instance ;

    map< string, DODSDefine * > _def_list ;
    typedef map< string, DODSDefine * >::const_iterator Define_citer ;
    typedef map< string, DODSDefine * >::iterator Define_iter ;
protected:
				DODSDefineList(void) {}
public:
    virtual			~DODSDefineList(void) {}

    virtual bool		add_def( const string &def_name,
					 DODSDefine *def ) ;
    virtual bool		remove_def( const string &def_name ) ;
    virtual void		remove_defs( ) ;
    virtual DODSDefine *	find_def( const string &def_name ) ;
    virtual void		show_definitions( DODSInfo &info ) ;

    static DODSDefineList *	TheList() ;
};

#endif // I_DODSDefineList_h

// $Log: DODSDefineList.h,v $
// Revision 1.3  2005/03/17 19:25:29  pwest
// string parameters changed to const references. remove_def now deletes the definition and returns true if removed or false otherwise. Added method remove_defs to remove all definitions
//
// Revision 1.2  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
