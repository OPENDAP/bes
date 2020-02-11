# Fileout CoverageJSON Response Handler Module

## About

CoverageJSON is a format for encoding coverage data like grids, time series, and vertical profiles, distinguished by the geometry of their spatiotemporal domain. A CoverageJSON object represents a domain, a range, a coverage, or a collection of coverages. A range in CoverageJSON represents coverage values. A coverage in CoverageJSON is the combination of a domain, parameters, ranges, and additional metadata. A coverage collection represents a list of coverages.

A complete CoverageJSON data structure is always an object (in JSON terms). In CoverageJSON, an object consists of a collection of name/value pairs – also called members. For each member, the name is always a string. Member values are either a string, number, object, array or one of the literals: true, false, and null. An array consists of elements where each element is a value as described above.

See https://covjson.org/spec/ for the full CovJSON specification.

## Contributors

This module was originally developed by Corey Hemphill, River Hendriksen, and Riley Rimer as an Oregon State University senior capstone project in 2018.
