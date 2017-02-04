// FONcMap.cc

// This file is part of BES Netcdf File Out Module

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

#include <Array.h>

#include <BESDebug.h>

#include "FONcArray.h"
#include "FONcMap.h"
#include "FONcUtils.h"

/** @brief Constructor for FONcMap that takes an array as the map
 *
 * This constructor takes a FONCArray that could be or is a map for a
 * DAP Grid.
 *
 * If ingrid is true, then the map was created within the context of a
 * DAP Grid. Otherwise, it's an array that could be a map defined
 * outside of a DAP Grid.
 *
 * @param a A FONcArray representing the grid map
 * @param ingrid true if the FONcArray was created in the context of a
 * grid, false otherwise
 */
FONcMap::FONcMap( FONcArray *a, bool ingrid )
    : _arr( a ), _ingrid( ingrid ), _defined( false ), _ref( 1 )
{
}

/** @brief Destructor that cleans up the map
 *
 * If the FONcArray was created within the context of the grid, then the
 * FONcArray is owned by the map and can be deleted. Otherwise, it was
 * created outside the context of a grid and will be cleaned up
 * elsewhere.
 */
FONcMap::~FONcMap()
{
    if( _ingrid )
    {
	delete _arr ;
	_arr = 0 ;
    }
}

/** @brief decrements the reference count for this map
 *
 * Since a map can be shared by different grids, reference counting is
 * used to keep track of any object that has a reference to this
 * instance. When the reference count goes to zero, then this instance
 * is deleted.
 */
void
FONcMap::decref()
{
    _ref-- ;
    if( !_ref ) delete this ;
}

/** @brief a method to compare two grid maps, or possible grid maps.
 *
 * All arrays are saved as a FONcMap if the array has only one dimension
 * and the name of the array and the name of the dimension are the same. The
 * maps are the same if their names are the same, they have the same number
 * of dimensions (arrays of strings written out have 2 dimensions, one for
 * the max length of the string), the type of the maps are the same, the
 * dimension size is the same, the dimension names are the same, and the
 * values of the maps are the same.
 *
 * @param tomap compare the saved map to this provided map
 * @return true if they are the same (shared) or false otherwise
 */
bool
FONcMap::compare( Array *tomap )
{
    bool isequal = true ;

    Array *map = _arr->array() ;

    BESDEBUG( "fonc", "FONcMap::compare - comparing " << tomap->name()
	      << " to " << map->name() << endl ) ;

    // compare the name
    if( isequal && tomap->name() != map->name() )
    {
	isequal = false ;
    }

    // compare the type
    if( isequal && tomap->var()->type() != map->var()->type() )
    {
	isequal =false ;
    }

    // compare the length of the array
    if( isequal && tomap->length() != map->length() )
    {
	isequal = false ;
    }

    // compare the number of dimensions
    if( isequal && tomap->dimensions() != map->dimensions() )
    {
	isequal = false ;
    }

    // the variable name needs to be the same as the dimension name
    if( isequal &&
        map->dimension_name( map->dim_begin() ) != map->name() )
    {
	isequal = false ;
    }

    // compare the dimension name
    if( isequal &&
        tomap->dimension_name( tomap->dim_begin() ) !=
	map->dimension_name( map->dim_begin() ) )
    {
	isequal = false ;
    }

    // compare the dimension size. Is this the same as the length of the array
    if( isequal &&
        tomap->dimension_size( tomap->dim_begin(), true ) !=
	map->dimension_size( map->dim_begin(), true ) )
    {
	isequal = false ;
    }

    if( isequal )
    {
	// compare the values of the array
	switch( tomap->var()->type() )
	{
	    case dods_byte_c:
		{
		    dods_byte my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_byte to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_int16_c:
		{
		    dods_int16 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_int16 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_uint16_c:
		{
		    dods_uint16 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_uint16 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_int32_c:
		{
		    dods_int32 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_int32 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_uint32_c:
		{
		    dods_uint32 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_uint32 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_float32_c:
		{
		    dods_float32 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_float32 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_float64_c:
		{
		    dods_float64 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_float64 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_str_c:
	    case dods_url_c:
		{
		    vector<string> my_values ;
		    map->value( my_values ) ;
		    vector<string> to_values ;
		    tomap->value( to_values ) ;
		    vector<string>::const_iterator mi = my_values.begin() ;
		    vector<string>::const_iterator me = my_values.end() ;
		    vector<string>::const_iterator ti = to_values.begin() ;
		    for( ; mi != me; mi++, ti++ )
		    {
			if( (*mi) != (*ti) )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    default:    // Elide unknown types; this is the current behavior
	                // but I made it explicit. jhrg 12.27.2011
	        break;
	}
    }

    BESDEBUG( "fonc", "FONcMap::compare - done comparing " << tomap->name()
	      << " to " << map->name() << ": " << isequal << endl ) ;
    return isequal ;
}

/** @brief Add the name of the grid as a grid that uses this map
 */
void
FONcMap::add_grid( const string &name )
{
    _shared_by.push_back( name ) ;
}

/** @brief clear the embedded names for the FONcArray kept by this
 * instance
 */
void
FONcMap::clear_embedded()
{
    _arr->clear_embedded() ;
}

/** @brief define the map in the netcdf file by calling define on the
 * FONcArray
 *
 * @param ncid The id of the netcdf file
 */
void
FONcMap::define( int ncid )
{
    if( !_defined )
    {
	_arr->define( ncid ) ;
	_defined = true ;
    }
}

/** @brief writes out the vallues of the map to the netcdf file by
 * calling write on the FONcArray
 *
 * @param ncid The id of the netcdf file
 */
void
FONcMap::write( int ncid )
{
    _arr->write( ncid ) ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including the FONcArray instance kept by this map.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcMap::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcMap::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "array:" ;
    if( _arr )
    {
	strm << endl ;
	BESIndent::Indent() ;
	_arr->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << " not set" << endl ;
    }
    strm << BESIndent::LMarg << "shared by: " ;
    vector<string>::const_iterator i = _shared_by.begin() ;
    vector<string>::const_iterator e = _shared_by.end() ;
    bool first = true ;
    for( ; i != e; i++ )
    {
	if( !first ) strm << ", " ;
	strm << (*i) ;
	first = false ;
    }
    strm << endl ;
    BESIndent::UnIndent() ;
}

