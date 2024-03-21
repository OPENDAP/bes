<a href="https://travis-ci.org/OPENDAP/bes">
  <img alt="TravisCI" src="https://travis-ci.org/OPENDAP/bes.svg?branch=master"/>
</a> 

README for the OPeNDAP BES 
==========================
# Version 3.21.0-46
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.10564745.svg)](https://doi.org/10.5281/zenodo.10564745)

This version of the BES is part of Hyrax 1.17.0, a data server that supports the
OPeNDAP data access protocols. See
[opendap.org/software/hyrax/1.17](https://www.opendap.org/software/hyrax/1.17)
for information about Hyrax.

For specific information about the BES, see the file _NEWS_ for a summary of new
features and important updates. See _ChangeLog_ for a complete listing of changes/fixes
since the previous release.

Major changes in this release are substantially improved support for HDF5 data stored
in S3 via the DMR++ much faster generation of NetCDF4 response files. There are many 
other improvements and bug fixes.

This version of the BES requires *libdap-3.21.0*
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.10564122.svg)](https://doi.org/10.5281/zenodo.10564122)

# Version 3.20.13
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.6884800.svg)](https://doi.org/10.5281/zenodo.6884800)

This version of the BES is part of Hyrax 1.16.8, a data server that supports the
OPeNDAP data access protocols. See
[opendap.org/software/hyrax/1.16](https://www.opendap.org/software/hyrax/1.16)
for information about Hyrax.

For specific information about the BES, see the file _NEWS_ for a summary of new
features and important updates. See _ChangeLog_ for a complete listing of changes/fixes
since the previous release.

This version of the BES requires *libdap-3.20.11*
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.6878992.svg)](https://doi.org/10.5281/zenodo.6878992)

# Version 3.20.12
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.6803473.svg)](https://doi.org/10.5281/zenodo.6803473)

_DOI available at https://zenodo.org/ search for OPENDAP/bes version 3.20.12_

This version of the BES is part of Hyrax 1.16.7, a data server that supports the
OPeNDAP data access protocols. See
[opendap.org/software/hyrax/1.16](https://www.opendap.org/software/hyrax/1.16)
for information about Hyrax.

For specific information about the BES, see the file _NEWS_ for a summary of new
features and important updates. See _ChangeLog_ for a complete listing of changes/fixes
since the previous release.

This version of the BES requires *libdap-3.20.10*
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.6789103.svg)](https://doi.org/10.5281/zenodo.6789103)


# Version 3.20.11
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.6799677.svg)](https://doi.org/10.5281/zenodo.6799677)

This version of the BES is part of Hyrax 1.16.6, a data server that supports the
OPeNDAP data access protocols. See 
[opendap.org/software/hyrax/1.16](https://www.opendap.org/software/hyrax/1.16)
for information about Hyrax.
 
For specific information about the BES, see the file _NEWS_ for a summary of new 
features and important updates. See _ChangeLog_ for a complete listing of changes/fixes 
since the previous release. 

This version of the BES requires *libdap-3.20.10*
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.6789103.svg)](https://doi.org/10.5281/zenodo.6789103)



# Introduction
The Back-end Server (BES) for Hyrax is a unix daemon that builds DAP2 and DAP4 response
for various kinds of data. Since the daemon runs on Unix hosts, it often works with data
that are stored in files or collections of files found on file systems. However, the BES
can also use data stored in database systems, web object stores (e.g., S3), other kinds
of web APIs and remote data accessed using plain HTTP.

In addition to the DAP2/4 protocols, the BES can package responses to queries for data in
a number of well-known binary file types, including NetCDF3/4, GeoTIFF and JPEG2000.

All of the functionality specific to DAP or particular types of data is implemented using
a group of 'plugin modules.' These modules isolate the operations for specific kinds of
daa from the BES software itself. Each kind of data that can be read is accessed using 
a different module and each response other than the DAP2/4 responses is returned its own
module.

The BES does not contain (much) software that implements the DAP2/4 protocols. Instead it
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

## Contents
* What's here: What files are in the distribution
* Configuration: How to configure the BES
* Testing: Once built and configured, how do you know it works?
* Various features the BES supports

# What's here
Here there's a bes-config script which will be installing in $prefix/bin that
can be used to determine where the libraries and their header files have been
installed. This directory also contains some documentation files.

_dispatch_: This is where the bulk of the BES source code resides.
server: The BES server and standalone executables; build using dispatch
cmdln: A command line client which can communicate with the bes server.

_dap_: A module that implements the (OPeN)DAP access.

_modules_: All the modules that are part of a standard Hyrax distribution.
The configure script looks for their dependencies and only tries to build
the ones that can be built. Note that the HDF4 and 5 modules are pulled in
from separate _git_ repositories.

_xmlcommand_: The BES/dispatch software is a framework. The basic commands
recognized by  it are defined and implemented by software in this directory.

_standalone_: The 'standalone version of the server; used for testing.

_apache_: An Apache module, currently not part of the default build

_ppt_: The PPT implementation. The BES uses PPT for its communication

_templates_: A collection of source files which can be used as templates when
you write your own handlers/modules, et cetera.

_conf_: Where the automake and autoconf configuration files live

_docs_: Where some bes documentation resides

# Configuration
The BES configuration is controlled by a set of configuration files. While the complete
configuration could be held in one file, it is easier to store information
specific to each module in a different file. The main configuration file then
reads all of the module configuration files, following a common Unix pattern.

## Basic Configuration
Once the BES software has been installed, you will need to make a few
changes to the BES configuration file, which is the bes.conf located in
_prefix_`/etc/bes/bes.conf`. Module and handler configuration files will
be installed in the _prefix_`/etc/bes/modules` directory.

Only a few parameters need to be modified to get the BES up and running
for the first time. These parameters are located at the top of the
configuration file are are:

**BES.ServerAdministrator**
- set this to an email address that can be used for users to contact if
  there are issues with your installation of the server.

**BES.User=user_name**

**BES.Group=group_name**
- set these to a valid username and groupname on your system. We
  recommend that you create a username and groupname called bes that has
  permissions to write only to the BES installation directory. We'll
  need to write to the log file, and that's about it.

**BES.LogName=./bes.log**
- Set this to the full path and file name for where you want the BES log
  file to be written.

With this configuration you will be able to start the BES. No handlers
or modules have been installed yet, so you won't be able to serve data,
but the BES should run.

## Configuration for Hyrax
For the BES to run with Hyrax, additional changes will need to be made
to the dap.conf and dap-server.conf files, which are located in the
modules directory _prefix_`/etc/bes/modules`. The dap module should be
installed by default with the BES. If the dap.conf file is not found, be
sure that you have libdap installed. The dap-server module, known as the
General purpose handlers, adds responses for the DAP ascii response, the
DAP info response, and the DAP html form response. You will need to get
the dap-server module. You should see a dap-server.conf file in the
modules directory.

The changes required for Hyrax are:

**BES.Catalog.catalog.RootDirectory** = _path_to_root_data_directory_
- Find the BES.Catalog.catalog.RootDirectory parameter in dap.conf. Set
  this to the root directory of your data. If you're serving data stored
  in files, this is the place in the file system where those files are
  stored.

**BES.Catalog.catalog.Include**=;
- This parameter specifies what files/directories are included in the
  list of nodes in the catalog. The default is to show everything. After
  this parameter is looked at, the Exclude parameter is then looked at
  to see what files you might want to exclude.

__BES.Catalog.catalog.Exclude=^\..*;__
- This parameter specifies what files/directories to include in the list
  of nodes in the catalog. The default, shown here, is to exclude any
  files or directories that starts with a dot (.)

The only possible configuration parameter that you may need to change is
the one that maps a file to a data handler. This parameter is called
BES.catalog.TypeMatch, and is found in each of the data handler
configuration files, such as nc.conf. The default values should work.

The value of this parameter is a semicolon separated list that matches
the name you used in the BES.modules parameter with different datasets.
The BES uses regular expressions to identify different types of
datasets. In the example given in the file, any dataset name that ends
in '.nc' will be accessed using the netcdf hander (because the name 'nc'
is used here with the regular expression '.*\.nc'; note that the BES
uses regular expressions like Unix grep, not file globbing patterns like
a command shell). Since the name 'nc' was associated with the netcdf
module in the modules section, the netcdf module will be used to access
any dataset whose name that ends in '.nc'.

The regular expressions shown in the examples are simple. However, the
entire dataset name is used, so it's easy to associate different modules with
datasets based on much more than just the 'file name extension' even though
that is the most common case.

To test your regular expression for the TypeMatch parameter, or the
Include and Exclude parameters, use the supplied besregtest program.
Simply run besregtest to discover its usage.

### Installing a custom handler/module
By default, The BES for Hyrax comes with a suite of handlers that read
a number of data formats. However, you can install custom handlers that 
are not distributed by default. I'll use the SQL handler as an example.

Get the SQL  handler source code from http://github.com/opendap/,
making sure that the version supports Hyrax. Expand the tar.gz file 
and follow the instructions with the following caveat:
If you have installed the BES using a prefix other than /usr/local (the
default), make sure that the correct bes-config is being run. If you are
having problems compiling or linking the handler, try using not only
--prefix=... but also --with-bes=... when you configure the handler. 

#### Install the newly-built handler.
Once built, install the handler using 'make install'. This will install
the BES module and a configuration file to use for that module. Each
module will have its own configuration file. In this case it is nc.conf
and installed in the _prefix_`/etc/bes/modules` directory. The next time
the BES is run, this configuration file will be read and the netcdf
module loaded. No modifications are necessary.

## Testing
To test the server, open a new terminal window and start the bes by using
the bes control script besctl, which is installed in _prefix_`/bin`.
using the -c switch to name the configuration file. If the server standalone
starts correctly it should print something like the following to stdout:

```
    [jimg@zoe sbin]$ besctl start
    BES install directory: <prefix>/bin
    Starting the BES daemon
    PID: <process_id> UID: <uid of process>
```

Go back to your first window and run the bescmdln program. Use the -h (host)
and -p (port) switches to tell it how to connect to the BES.

```
    [jimg@zoe bin]$ bescmdln -p 10002 -h localhost

    Type 'exit' to exit the command line client and 'help' or '?' to display
    the help screen

    BESClient>
```

Try some simple commands:

```
    BESClient> show help;
    <showHelp>
	<response>

	...

	</response>
    </showHelp>
    BESClient>
```

Note that all commands must end with a semicolon except 'exit'.

Now try to get a DAP response using a handler you registered (the BES
supports a fairly complex syntax which I won't explain fully here):

```
    BESClient> set container in catalog values n1, data/nc/fnoc1.nc;
    BESClient> define d1 as n1;
    BESClient> get das for d1;
    Attributes {
	u {
	    String units "meter per second";
	    String long_name "Vector wind eastward component";
	    String missing_value "-32767";
	    String scale_factor "0.005";
	}
	v {
	    String units "meter per second";
	    String long_name "Vector wind northward component";
	    String missing_value "-32767";
	    String scale_factor "0.005";
	}
	lat {
	    String units "degree North";
	}
	lon {
	    String units "degree East";
	}
	time {
	    String units "hours from base_time";
	}
	NC_GLOBAL {
	    String base_time "88- 10-00:00:00";
	    String title " FNOC UV wind components from 1988- 10 to 1988- 13.";
	}
	DODS_EXTRA {
	    String Unlimited_Dimension "time_a";
	}
    }
    BESClient> exit
    [jimg@zoe bin]$
```

If you got something similar (you would have used a different dataset name
from "data'nc'fnoc1.nc" and thus would get a different DAS response) the BES
is configured correctly and is running. 

To stop the BES use the bes control script with the stop option:

```
    [jimg@zoe sbin]$ besctl stop
```

Note: Constraints and the bes command line client

Constraints are added to the 'define' command using the modifier 'with' like
so: 

```
    define d as nscat with nscat.constraint="WVC_LAT";
```

If there is a list of containers instead of just one, then there can be a
list of <container>.constraint="" clauses.

# Various features the BES supports
Here we highlight some important features of the BES software.
This list is far from comprehensive and it is not intended to be 
a 'new features' list. Most of these features are supported by the
BES because they are important to the Hyrax data server. However,
all of these are accessible when the BES is used by itself or in
conjunction with other software.

## DAP2 versus DAP4
The code on the master branch now supports both DAP2 and DAP4
responses, and the data format handlers do as well. Because of that,
it must be linked with libdap 3.14, not libdap older versions of
libdap (libdap 3.14 contains support for both DAP2 and DAP4, while the
older 'libdap' supports only DAP2). If you need to get the DAP2-only
version of the software, use the branch named 'dap2'. Please commit
only fixes there.

Each of the handlers, which can be built as submodules within this
code, also has DAP2 and DAP4 support on their master branch and a
'dap2' branch for the DAP2-only code.

## Support for data stored on Amazon's S3 Web Object Store
Hyrax 1.16 has prototype support for subset-in-place of HDF5 and NetCDF4
data files that are stored on AWS S3. See the preliminary documentation in
https://github.com/OPENDAP/bes/blob/master/modules/dmrpp_module/data/README.md.

The new support includes software that can configure data in S3 and on
disk so that it can be served (and subset) in-place from S3 without reformatting
the original data files. Support for other web object stores besides S3
has also been demonstrated.

## Support for dataset crawler/indexer systems
We have added support for Datasets served by Hyrax now provide
information Google and other search engines need to make these data
findable. All dataset landing pages and catalog navigation
(contents.html) pages now contain embedded json-ld which crawlers such
as Google Dataset Search, NSF's GeoCODES, and other data sensitive web
crawlers use for indexing datasets. In order to facilitate this,
certain steps can be taken by the server administrator to bring the
Hyrax service to Google (and other) crawlers attention. LINK TO
JSON-LD README.MD. Our work on JSON-LD support was funded by NSF Grant
number 1740704.

## Experimental support for STARE indexing
We have added experimental support for STARE (Spatio Temporal
Adaptive-Resolution Encoding) as part of our work on NASA ACCESS
grant 17-ACCESS17-0039.

## Server function roi() improvements

## Site-specific configuration
The BES uses a number of configuration files, and until now, a site has to
customize these for their server. Each server installation would overwrite
those files. No more. Now you can put all your configuration values in 

> /etc/bes/site.conf

And be configent that they will never be overwritten by a new install. The
`site.conf` file is always read last, so parameters set there override values
set elsewhere.

## The Metadata Store (MDS)
A new cache has been added to the BES for Metadata Responses (aka, the MDS
or Metadata Store). This cache is unlike the other BES caches in that it is
intended to be operated as either a 'cache' or a 'store.' In the latter case,
items added will never be removed - it is an open-ended place where metadata
response objects will be kept indefinitely. The MDS contents (as a cache or 
a store) will survive Hyrax restarts.

The MDS was initially built to speed responses when data are stored on high-
latency devices like Amazon's S3 storage system. We have special features in
Hyrax to handle data kept on those kinds of data stores and the MDS can provide
clients with fast access to the metadata for those data very quickly. After
our initial tests of the MDS, we decided to make it a general feature of the
server, available to data served from data stored on spinning disk in addition
to S3.

Note: The MDS is not used for requests that using server-side function processing.

The MDS configuration can be found in the dap.conf configuration file. Here 
are the default settings:

* Setting the 'path' to null disables uses of the MDS. 

> DAP.GlobalMetadataStore.path = @datadir@/mds
> DAP.GlobalMetadataStore.prefix = mds

* Setting 'size' to zero makes the MDS hold objects forever; setting a positive 
non-zero size makes the MDS behave like a cache, purging responses when the 
size is exceeded. The number is the size of the cache volume in megabytes.

> DAP.GlobalMetadataStore.size = 200

* The MDS writes a ledger of additions and removals. By default the ledger is
kept in 'mds_ledger.txt' in the directory used to start the BES.

> DAP.GlobalMetadataStore.ledger = @datadir@/mds_ledger.txt

## COVJSON Responses
For datasets that contain geo-spatial data, we now provide the option to
get those data (and related metadata) encoded using the covjson format. 
(See https://covjson.org/). Thanks to Corey Hemphill, Riley Rimer, and 
Lewis McGibbney for this contribution.

## Improved catalog support
It has long been possible to define 'catalogs' for data that reside on other
kinds of data stores (e.g., relational data base systems). The datasets defined
by these catalogs appear(ed) in the directory listings as if they are 
directories of data files, just like datasets that actually are files on a
spinning disk. 

We have generalized this system so that it is much easier to use. As an example
of new Catalog sub-system's ease of use, we have implemented a new module that
reads information about datasets from NASA's Common Metadata Repository (CMR)
and uses that to display a Virtual Directory for NASA data, where the hierarchical
relationships between data files is derived entirely from data read a CMR-support
web API.

This software is currently available in source form only - contact us if you
would like to extend the BES Catalog system for your own data collections. 
To build Hyrax with this feature enabled, build either in developer mode 
(./configure ... --enable-developer) or using the spepcial configuration option
--enable-cmr.

## For HDF4 data
* CF option: Enhance the support of handling the scale_factor and add_offset to follow the CF. The scale_factor and
add_offset rule for the MOD16A3 product is different than other MODIS products. We make an exception for 
this product only to ensure the scale_factor and add_offset follow the CF conventions.
 
 
## For HDF5 data

CF option:
1. Add the support of the HDF-EOS5 Polar Stereographic(PS) and Lambert Azimuthal Equal Area(LAMAZ) grid projection files. 
   Both projection files can be found in NASA LANCE products.
2. Add the HDF-EOS5 grid latitude and longitude cache support. This is the same as what we did for the HDF-EOS2 grid. 
3. Add the support for TROP-OMI, new OMI level 2 and OMPS-NPP product.
4. Removed the internal reserved netCDF-4 attributes for DAP output.
5. Make the behavior of the drop long string BES key consistent with the current limitation of netCDF Java.

# COPYRIGHT INFORMATION

    The OPeNDAP BES code is copyrighted using the GNU Lesser GPL. See the
    file COPYING or contact the Free Software Foundation, Inc., at 59 Temple
    Place, Suite 330, Boston, MA 02111-1307 USA. Older versions of the BES were
    copyrighted by the University Corporation for Atmospheric Research;
    see the file COPYRIGHT_UCAR.
    
## PicoSHA2 license

    The BES uses PicoSHA2 - a library that provides an implementation of the 
    SHA256 hashing algorithm; its copyright follows
    
    Copyright (C) 2017 okdshin

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    The BES uses pugixml - a source code header library that provides xml parsing.
    Its license explicitly includes such use:

## pugixml license

    MIT License

    Copyright (c) 2006-2020 Arseny Kapoulkine

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

## rapidjson license

// Tencent is pleased to support the open source community by making RapidJSON available.
// 
// Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip. All rights reserved.
//
// Licensed under the MIT License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, software distributed 
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
// CONDITIONS OF ANY KIND, either express or implied. See the License for the 
// specific language governing permissions and limitations under the License.
//
