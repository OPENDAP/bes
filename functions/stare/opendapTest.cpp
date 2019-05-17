// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2019 OPeNDAP, Inc.
// Author: Kodi Neumiller <kneumiller@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// Tests for the AISResources class.

#include "SpatialVector.h"
#include "SpatialIndex.h"
#include "SpatialInterface.h"
#include "RangeConvex.h"
#include "HtmRange.h"
#include "HtmRangeIterator.h"
#include "VarStr.h"

#include <unistd.h> //For getopt


int main(int argc, char *argv[]) {
	int args = 1;
    bool quiet = false;
    size_t level=5;
    size_t savelevel=10;

    int c;
    while ((c = getopt (argc, argv, "l:")) != -1) {
    		switch (c) {
    		case 'l':
    			level = atoi(argv[1]);
    		default:
    			abort ();
    		}
    }

    float64 lat[2];
    float64 lon[2];
    if (args<argc-2){
    		//level = atoi(argv[args++]);
        // Get corners from arguments
        for (int i = 0 ; i < 2 ; i++ ) {
            lat[i] = atof(argv[args++]);
            lon[i] = atof(argv[args++]);
        }
    }


    //Set the four corners of the rectangular area that will be covered
    SpatialVector* cnrs[4];

	cnrs[0] = new SpatialVector();
	cnrs[0]->setLatLonDegrees(lat[0], lon[0]);
	cnrs[1] = new SpatialVector();
	cnrs[1]->setLatLonDegrees(lat[0], lon[1]);
	cnrs[2] = new SpatialVector();
	cnrs[2]->setLatLonDegrees(lat[1], lon[1]);
	cnrs[3] = new SpatialVector();
	cnrs[3]->setLatLonDegrees(lat[1], lon[0]);





    RangeConvex* conv;
    HtmRange htmRange;
    uint64 intVal = 0;

    // create htm interface
    htmInterface htm(level,savelevel);

    // Create spatial index at according level
    //si = new SpatialIndex(level,savelevel);
    const SpatialIndex &index = htm.index();

    // create spatial domain
    SpatialDomain domain(&index);   // initialize empty domain

    // Create convex to represent the geometry
    conv = new RangeConvex(cnrs[0],cnrs[1],cnrs[2],cnrs[3]);

    // Add convex to domain
    domain.add(*conv);

    // Purge HTMRange
    htmRange.purge();

    // Do the intersect. The boolean is for varlength ids or not
    //conv->intersect(si,htmRange,false);
    domain.intersect(&index, &htmRange, false);	  // intersect with range


    // Iterate through the indices
    HtmRangeIterator iter(&htmRange);
    char buffer[80];
    while (iter.hasNext()) {
        //intVal = iter.nextSymbolicInt(buffer);
    		iter.nextSymbolic(buffer);
        intVal = htmRange.encoding->idByName(buffer);
        cout << "int64: " << intVal << endl;
    }
    //htmRange.print(HtmRange::LOWS, cout, true);
    cout << "resolution level: " << index.getMaxlevel() << endl;
    cout << "done" << endl;

}

