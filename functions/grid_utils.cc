/*
 * grid_utils.cc
 *
 *  Created on: Feb 8, 2013
 *      Author: ndp
 */

#include "grid_utils.h"

#include "util.h"
#include "Structure.h"
#include "Sequence.h"
#include "Grid.h"
#include "BaseType.h"
#include "BESDebug.h"
#include "GridGeoConstraint.h"

using namespace libdap;

namespace libdap {

/**
 * Recursively traverses the BaseType bt (if its a conrtuctor type) and collects pointers to all of the Grid and places said pointers
 * into the vector parameter 'grids'. If the BaseType parameter bt is an instance of Grid the it is placed in the vector.
 * @param bt The BaseType to evaluate
 * @param grids A vector into which to place a pointer to every Grid.
 */
void getGrids(BaseType *bt, vector<Grid *> *grids){
	Type type = bt->type();

	BESDEBUG( "libdap::getGrids()","libdap::getGrids() - Searching for Grid variables. Current variable: "<< bt->name() << "  type: "<< bt->type_name() << endl);

	switch(type){

		case dods_grid_c:
		{
			// Yay! It's a Grid!
			Grid *grid = &dynamic_cast<Grid&>(*bt);
			grids->push_back(grid);
			break;
		}
		case dods_array_c:
		{
			// It's an Array - but of what? Check the template variable.
			Array &array = dynamic_cast<Array&>(*bt);
			BaseType *arrayTemplate = array.var("",true,0);
			getGrids(arrayTemplate, grids);
			break;
		}
		case dods_sequence_c:
		{
			// It's an Sequence - but of what? Check each variable in the Sequence.
			Sequence &seq = dynamic_cast<Sequence&>(*bt);
			for(Sequence::Vars_iter i=seq.var_begin(); i!=seq.var_begin(); i++){
				BaseType *sbt = *i;
				getGrids(sbt, grids);
			}
			break;
		}
		case dods_structure_c:
		{
			// It's an Structure - but of what? Check each variable in the Structure.
			Structure &s = dynamic_cast<Structure&>(*bt);
			for(Structure::Vars_iter i=s.var_begin(); i!=s.var_begin(); i++){
				BaseType *sbt = *i;
				getGrids(sbt, grids);
			}
			break;
		}
		default:
			break;
	}

}



/**
 * Recursively traverses the DDS and collects pointers to all of the Grids and places said pointers
 * into the vector parameter 'grids'.
 * @param dds The dds to search
 * @param grids A vector into which to place a pointer to every Grid in the DDS.
 */
void getGrids(DDS &dds, vector<Grid *> *grids){
	BESDEBUG( "libdap::getGrids()","libdap::getGrids() - Searching for Grid variables in DDS." << endl);
    for (DDS::Vars_iter i=dds.var_begin(); i!=dds.var_end(); i++) {
        BaseType *bt = *i;
        getGrids(bt, grids);
    }
}



/**
 * Evaluates a Grid to see if has suitable semantics for use with function_geogrid
 * @param grid the Grid to evaluate.
 */
bool isGeoGrid(Grid *grid){


	BESDEBUG( "libdap::isGeoGrid()","libdap::isGeoGrid() - Checking coordinate map semantics for Grid: "<< grid->name() << endl);

    bool foundUsableMetadata = true;

    Grid::Map_iter m = grid->map_begin();


	BESDEBUG( "libdap::isGeoGrid()","libdap::isGeoGrid() - Attempting to instantiate a GridGeoConstraint using Grid "<< grid->name() << endl);
	try {
		GridGeoConstraint gc(grid);
	} catch (Error *e) {
		BESDEBUG( "libdap::isGeoGrid()","libdap::isGeoGrid() - Failed to instantiate a GridGeoConstraint using Grid "<< grid->name() << endl);
		foundUsableMetadata = false;
	}

    if(foundUsableMetadata){
		BESDEBUG( "libdap::isGeoGrid()","libdap::isGeoGrid() - Grid "<< grid->name() << " MATCHES the semantics of a GeoGrid"<< endl);
    }
    else {
    	BESDEBUG( "libdap::isGeoGrid()","libdap::isGeoGrid() - Grid "<< grid->name() << " DOES NOT match the semantics of a GeoGrid"<< endl);
    }
	return foundUsableMetadata;
}


} //namespace libdap
