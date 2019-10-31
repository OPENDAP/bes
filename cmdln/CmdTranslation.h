// CmdTranslation.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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

#ifndef CmdTranslation_h
#define CmdTranslation_h 1

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include <string>
#include <map>
#include <iostream>

class BESTokenizer ;

class CmdTranslation
{
private:
    typedef bool (*p_cmd_translator)( BESTokenizer &tokenizer,
				      xmlTextWriterPtr writer ) ;
    static std::map< std::string, p_cmd_translator > _translations ;

    static bool			_is_show ;

    static bool			translate_show( BESTokenizer &tokenizer,
					    xmlTextWriterPtr writer ) ;
    static bool			translate_show_error( BESTokenizer &tokenizer,
					    xmlTextWriterPtr writer ) ;
    static bool			translate_catalog( BESTokenizer &tokenizer,
					       xmlTextWriterPtr writer ) ;
    static bool			translate_set( BESTokenizer &tokenizer,
					   xmlTextWriterPtr writer ) ;
    static bool			translate_context( BESTokenizer &tokenizer,
					       xmlTextWriterPtr writer ) ;
    static bool			translate_container( BESTokenizer &tokenizer,
						 xmlTextWriterPtr writer ) ;
    static bool			translate_define( BESTokenizer &tokenizer,
					      xmlTextWriterPtr writer ) ;
    static bool			translate_delete( BESTokenizer &tokenizer,
					      xmlTextWriterPtr writer ) ;
    static bool			translate_get( BESTokenizer &tokenizer,
					       xmlTextWriterPtr writer ) ;

    static bool			do_translate( BESTokenizer &tokenizer,
					      xmlTextWriterPtr writer ) ;
public:
    static int			initialize( int argc, char **argv ) ;
    static int			terminate( void ) ;

    static bool			is_show() { return _is_show ; }
    static void			set_show( bool val ) { _is_show = val ; }

    static void			add_translation( const std::string &name,
						 p_cmd_translator func ) ;
    static void			remove_translation( const std::string &name ) ;

    static std::string		translate( const std::string &commands ) ;

    static void			dump( std::ostream &strm ) ;
} ;

#endif // CmdTranslation_h

