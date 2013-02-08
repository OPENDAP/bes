/*
 * grid_utils.h
 *
 *  Created on: Feb 8, 2013
 *      Author: ndp
 */

#ifndef GRID_UTILS_H_
#define GRID_UTILS_H_

#include "Basetype.h"
#include "Grid.h"

namespace libdap {


void getGrids(BaseType *bt, vector<Grid *> *grids);
void getGrids(DDS &dds, vector<Grid *> *grids);
bool isGeoGrid(Grid *d_grid);

}



#endif /* GRID_UTILS_H_ */
