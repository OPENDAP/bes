# CoverageJSON-Response-Handler-for-OPeNDAP

## Problem

OPeNDAP is a data transport protocol that supplies its users a way to provide and request data across the web. One of the main functions
of the protocol is the ability to pull data in multiple different formats including ASCII, netCDF3, netCDF4, binary (DAP2), and several
other object serializations. OPeNDAP does not currently have a response handler in place to serve data in the CoverageJSON data format,
which is a JavaScript Object Notation (JSON) data format for describing coverages. CoverageJSON would be a useful format to have
available from OPeNDAP due to it encoding data values based upon a spatial temporal domain similar to how satellite data is collected
from NASA's satellites. The integration of the CoverageJSON format into OPeNDAP would help support the creation of coverage data driven
web applications by NASA JPL(Jet Propulsion Lab) and any other users of OPeNDAP.

## Solution

This module contains the code for the implementation of a CoverageJSON response handler which is integrated into the flow of Hyrax and OPeNDAP.
Users and developers can either used the module via the OLFS or the BES standalone on the backend.
This project was completed in early 2018.
