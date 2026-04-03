# Introduction

## The Hyrax server architecture

## The BES
The Back-end Server (BES) for Hyrax is a unix daemon that builds DAP2 and DAP4 responses
for various kinds of data. Since the daemon runs on Unix hosts, it often works with data
that are stored in files or collections of files found on file systems. However, the BES
can also use data stored in database systems, web object stores (e.g., S3), other kinds
of web APIs and remote data accessed using plain HTTP.

In addition to the DAP2/4 protocols, the BES can package responses to queries for data in
a number of well-known binary file types, including NetCDF3/4, GeoTIFF and ASCII/CSV.

All the functionality specific to DAP or particular types of data is implemented using
a group of 'plugin modules.' These modules isolate the operations for specific kinds of
daa from the BES software itself. Each kind of data that can be read is accessed using
a different module and each response other than the DAP2/4 responses is returned using
a module.

The BES does not contain (much) software that implements the DAP2/4 protocols. Instead, it
uses the libdap4 library for that. See [github.com/opendap/libdap4](github.com/opendap/libdap4).

The BES does not implement the WEB API for DAP2/4, instead the OLFS (OPeNDAP Lightweight Front-end Server)
is used to do that. See [github.com/opendap/olfs](github.com/opendap/olfs).

The BES framework is designed to be extensible and can be used to combine reading various
kinds of data with building different kinds of responses. The framework is designed to
support lazy data read operations, only reading those data that are needed and only when
they are needed. Note that some kinds of well-known binary responses must be built in full
before they are returned, but this is not a requirement of the framework but the responses,
or their APIs.

Information about the Hyrax data server can be found here in the
<a href="https://opendap.github.io/hyrax_guide/Master_Hyrax_Guide.html">
latest and most comprehensive Hyrax documentation.</a>
<br /> <br/>
<a href="https://opendap.github.io/bes/html/">The BES API Documentation is here</a>
