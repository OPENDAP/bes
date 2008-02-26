This file includes describes the HDF5 DAP server developed by THG andOPeNDAP under a grant from NASA.

For information about building the OPeNDAP HDF5 Data Handler, see theINSTALL or INSTALL_EOS5_GRID file. INSTALL is about building a generalpurpose handler  while INSTALL_EOS5_GRID is about building a specialhandler that can support displaying NASA EOS Grid data via existingOPeNDAP visualization clients like ferret and GrADS.

This data handler includes an executable for use with Server3 and ashared object module for use with Hyrax.

A configuration edition helper script, `bes-hdf5-data.sh' is providedin this package for easy configuration of the Hyrax BES server,designed to edit bes.conf. The script is called using:

   bes-hdf5-data.sh  <bes.conf file to modify>  <bes modules dir>  

The `bes-conf' make target runs the script while trying to select pathscleverly, and should be called using:

   make bes-conf

Test data are also installed, so after installing this handler, Hyraxwill have data to serve providing an easy way to test your newinstallation and to see how a working bes.conf should look. To usethis, make sure that you first install the bes, and that dap-servergets installed too.  Finally, every time you install or reinstallhandlers, make sure to restart the BES and OLFS.

This data handler is one component of the OPeNDAP DAP Server; theserver base software is designed to allow any number of handlers to beconfigured easily.  See the DAP Server README and INSTALL files forinformation about configuration, including how to use this handler.

HDF Version: 

	This release of the server supports HDF5 1.6.6 or above.

Support for HDF5 data types:

	Groups: Group structure is mapped into a special attribute	HDF5_ROOT_GROUP {...}.

	Datasets: Array, Byte, Float32/64, (U)Int16/32, String are	supported.  If an array has dimensional data, Grid will be generated	automatically.

	Comments: Comments are mapped into DAS attributes.

	Attributes: Attributes on groups and dataset are supported

	Compounds: Compound data types are mapped into Structure in DAP.

	Soft/Hard Links: Links are mapped to DAS attributes.

	References: Object or regional references are mapped into URLs.	This feature is only available for HDF5 1.8 beta.

Special Support for NASA EOS data: 

	Grids: Based on the projection specified in a metadata file, some	arrays are automatically mapped into "Grid" instead of "Array".  You	can turn off this behavior through configuration option(e.g.	--enable-eos-grid=no).  If "Grid" is chosen, its map data are	automatically and artificially generated based on	"UpperLeftPointMtrs" and "LowerRightPointMtrs" values.  Please see	set_dimension_array() in H5EOS.cc for details.

	Metadata: Metadatas (e.g. StructMetadata.x or CoreMetada.x) are	parsed and its attributes are generated in a nested structure	format.

Implementation details:

	1. HDF5 Data(including data and metadata structure) retrieval	is implemented at H5Git.cc.

	2. The current dap_h5_handler uses depth-first search to walk	through HDF5 graph. If a loop is detected in a graph, it stops	searching further which makes it a graph into a tree.  The	handler retrieves HDF5 attributes and puts them into DODS DAS	table. DAS table will hold tree structure under HDF5_ROOT_GROUP	attribute.

        By using absolute name of HDF5 objects, we also have a good	view of the structure of the whole HDF5 file. This absolute	name can be suppressed with --enable-short-path configuration	option.

Limitations:

	o No support for HDF5 files that have a '.' in a group/dataset	  name.

	o No support for any variable length array types except for 1	  dimensional variable-length string array.

	o No support for ENUM type.

	o No support for BITFIELD and OPAQUE types.

	o 64 bit integer (array) is not supported.

	o Both signed and unsigned character maps to Byte in OPeNDAP. 

Muqun Yang (ymuqun@hdfgroup.org)

Hyo-Kyung Lee (hyoklee@hdfgroup.org)