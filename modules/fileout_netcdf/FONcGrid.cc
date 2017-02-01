// FONcGrid.cc

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

#include <BESInternalError.h>
#include <BESDebug.h>

#include "FONcGrid.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief global list of maps that could be shared amongst the
 * different grids
 */
vector<FONcMap *> FONcGrid::Maps;

/** @brief tells whether we are converting or defining a grid.
 *
 * This is used by FONcArray to tell if any single dimension arrays
 * where the name of the array and the name of the dimension are the
 * same. If they are, then that array is saved as a possible shared map
 */
bool FONcGrid::InGrid = false;

/** @brief Constructor for FONcGrid that takes a DAP Grid
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Grid instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a grid
 * @throws BESInternalError if the BaseType is not a Grid
 */
FONcGrid::FONcGrid(BaseType *b) : FONcBaseType(), _grid(0), _arr(0)
{
    _grid = dynamic_cast<Grid *>(b);
    if (!_grid) {
        string s = (string) "File out netcdf, FONcGrid was passed a " + "variable that is not a DAP Grid";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
}

/** @brief Destructor that cleans up the grid
 *
 * The DAP Grid instance does not belong to the FONcGrid instance, so it
 * is not deleted.
 *
 * Since maps can be shared by grids, FONcMap uses reference counting.
 * So instead of deleting the FONcMap instance, its reference count is
 * decremented.
 */
FONcGrid::~FONcGrid()
{
    vector<FONcMap *>::iterator i = _maps.begin();
    while (i != _maps.end()) {
        // These are the FONc types, not DAP types
        (*i)->decref();
        ++i;
    }

    // Added jhrg 8/28/13
    delete _arr;
}

/** @brief define the DAP Grid in the netcdf file
 *
 * Iterates through the maps for this grid and defines each of those, if
 * they haven't already been defined by a grid that shares the map. Then
 * it defines the grid's array in the netcdf file.
 *
 * Any attributes for the grid will be written out for each of the maps
 * and the array.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * Byte
 */
void FONcGrid::define(int ncid)
{
    if (!_defined) {
        BESDEBUG("fonc", "FOncGrid::define - defining grid " << _varname << endl);

        // Only variables that should be sent are in _maps. jhrg 11/3/16
        vector<FONcMap *>::iterator i = _maps.begin();
        vector<FONcMap *>::iterator e = _maps.end();
        for (; i != e; i++) {
            (*i)->define(ncid);
        }

        // Only define if this should be sent. jhrg 11/3/16
        if (_arr)
            _arr->define(ncid);

        _defined = true;

        BESDEBUG("fonc", "FOncGrid::define - done defining grid " << _varname << endl);
    }
}


/** @brief convert the DAP Grid to a set of embedded variables
 *
 * A DAP Grid contains one or more maps (arrays) and an array of values.
 * The convert method creates a FONcMap for each of the grid's maps,
 * and a FONcArray for the grid's array.
 *
 * A map can be shared by other grids if the name of the map is the
 * same, the size of the map is the same, the type of the map is the
 * same, and the values of the map are the same. If they are the same,
 * then it references that shared map instead of creating a new one.
 *
 * @param embed The list of parent names for this grid
 * @throws BESInternalError if there is a problem defining the
 * Byte
 */
void FONcGrid::convert(vector<string> embed)
{
    FONcGrid::InGrid = true;
    FONcBaseType::convert(embed);
    _varname = FONcUtils::gen_name(embed, _varname, _orig_varname);
    BESDEBUG("fonc", "FONcGrid::convert - converting grid " << _varname << endl);

    // A grid has maps, which are single dimnension arrays, and an array
    // with that many maps for dimensions.
    Grid::Map_iter mi = _grid->map_begin();
    Grid::Map_iter me = _grid->map_end();
    for (; mi != me; mi++) {

        // Only add FONcBaseType instances to _maps if the Frid Map is
        // supposed to be sent. See Hyrax-282. jhrg 11/3/16
        if ((*mi)->send_p()) {

        Array *map = dynamic_cast<Array *>((*mi));
        if (!map) {
            string err = (string) "file out netcdf, grid " + _varname + " map is not an array";
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        vector<string> map_embed;

        FONcMap *map_found = FONcGrid::InMaps(map);

        // if we didn't find a match then found is still false. Add the
        // map to the vector of maps. If they are the same then create a
        // new FONcMap, add the grid name to the shared list and add the
        // FONcMap to the FONcGrid.
        if (!map_found) {
            FONcArray *fa = new FONcArray(map);
            fa->convert(map_embed);
            map_found = new FONcMap(fa, true);
            FONcGrid::Maps.push_back(map_found);
        }
        else {
            // it's the same ... we are sharing. Add the grid name fo
            // the list of grids sharing this map and set the embedded
            // name to empty, just using the name of the map.
            map_found->incref();
            map_found->add_grid(_varname);
            map_found->clear_embedded();
        }
        _maps.push_back(map_found);

        }
    }

    // Only set _arr if the Grid Array should be sent. See Hyrax-282.
    // jhrg 11/3/16
    if (_grid->get_array()->send_p()) {
        _arr = new FONcArray(_grid->get_array());
        _arr->convert(_embed);
    }

    BESDEBUG("fonc", "FONcGrid::convert - done converting grid " << _varname << endl);
    FONcGrid::InGrid = false;
}

/** @brief Write the maps and array for the grid
 *
 * Once defined, the values of the maps and the values of the grid's
 * array can be written out to the netcdf file.
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the grid out
 * to the netcdf file
 */
void FONcGrid::write(int ncid)
{
    BESDEBUG("fonc", "FOncGrid::define - writing grid " << _varname << endl);

    // FONcBaseType instances are added only if the corresponding DAP variable
    // should be sent. See Hyrax-282. jhrg 11/3/16
    vector<FONcMap *>::iterator i = _maps.begin();
    vector<FONcMap *>::iterator e = _maps.end();
    for (; i != e; i++) {
        (*i)->write(ncid);
    }

    // only write this if is have been convert()ed and define()ed.
    // See above and Hyrax-282. jhrg  11/3/16
    if (_arr)
        _arr->write(ncid);

    _defined = true;

    BESDEBUG("fonc", "FOncGrid::define - done writing grid " << _varname << endl);
}

/** @brief returns the name of the DAP Grid
 *
 * @returns The name of the DAP Grid
 */
string FONcGrid::name()
{
    return _grid->name();
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data.
 * Included is each FONcMap instance as well as the FONcArray
 * representing the grid's array.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONcGrid::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FONcGrid::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name = " << _grid->name() << " { " << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "maps:";
    if (_maps.size()) {
        strm << endl;
        BESIndent::Indent();
        vector<FONcMap *>::const_iterator i = _maps.begin();
        vector<FONcMap *>::const_iterator e = _maps.end();
        for (; i != e; i++) {
            FONcMap *m = (*i);
            m->dump(strm);
        }
        BESIndent::UnIndent();
    }
    else {
        strm << " empty" << endl;
    }
    BESIndent::UnIndent();
    strm << BESIndent::LMarg << "}" << endl;
    strm << BESIndent::LMarg << "array:";
    if (_arr) {
        strm << endl;
        BESIndent::Indent();
        _arr->dump(strm);
        BESIndent::UnIndent();
    }
    else {
        strm << " not set" << endl;
    }
    BESIndent::UnIndent();
}

FONcMap *
FONcGrid::InMaps(Array *array)
{
    bool found = false;
    vector<FONcMap *>::iterator vi = FONcGrid::Maps.begin();
    vector<FONcMap *>::iterator ve = FONcGrid::Maps.end();
    FONcMap *map_found = 0;
    for (; vi != ve && !found; vi++) {
        map_found = (*vi);
        if (!map_found) {
            throw BESInternalError("map_found is null.", __FILE__, __LINE__);
        }
        found = map_found->compare(array);
    }
    if (!found) {
        map_found = 0;
    }
    return map_found;
}

