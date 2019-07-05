

# README
## Overview
This directory (_data_) contains module test data, and the scripts, 
source code, and production rules for tools that can used to create 
and process hdf5/netcdf-4 data files to create portable _dmr++_ files 
whose binary data objects are held in a web object store like AWS S3.

There are three programs for building _dmr++_ files:
- The program `get_dmrpp` builds a single _dmr++_ file from a single 
`netcdf-4`/`hdf5` file. 
- The program `ingest_filesystem` builds a collection of _dmr++_ files
from from data held in the locally mounted filesystem. 
- The program `ingest3bucket` builds a collection of _dmr++_ files
from from data held in Amazon's S3 storage.

NOTE: Organizationally, this directory (_data_) and it's child directory 
_dmrpp_ are arranged in this hierarchy in order to mimic the deployment 
structure resulting from running "make install". Most modules do not 
need to do this but since _dmr++_ files reference other files, and they 
do so using paths relative the BES Catalog Root the mimicry is required.


## Building the software

In order for these programs (shell scripts) to function correctly a 
localization step must take place. This happens when the parent software 
(the `dmrpp_module`) is built and installed as part of the BES. Once this 
is done the scripts will have been installed and should be in `$prefix/bin` 
and ready to use.

## `get_dmrpp` build a _dmr++_ file from an _hdf5_/_nectdf-4_ file.

The `get_dmrpp` shell script generates a single _dmr++_ file from a single 
netcdf-4/hdf5 file. It is used by both `ingest_filesystem` and `ingest_s3bucket`


```
 Usage: get_dmrpp [options] <hdf5_file>

 Write the DMR++ for hdf5_file to stdout
 
 By default the BES Data Root directory is set to the CWD. This 
 utility will add an entry to the bes.log specified in the 
 configuration file. The DMR++ is built using the DMR as returned 
 by the HDF5 handler, using options as set in the bes 
 configuration file found here.
 
 -h: Show help
 -v: Verbose: Print the DMR too
 -V: Very Verbose: print the DMR, the command and the configuration
     file used to build the DMR
 -r: Just print the DMR that will be used to build the DMR++
 -u: The binary object URL for use in the DMR++ file
 -d: Data root directory for the BES.
 -o: The name of the file  to create.

 Limitations: 
 * The pathanme to the hdf5 file must be relative from the
   directory where this command was run; absolute paths will not work. 
 * The build_dmrpp command must be in the CWD. 
 * The bes conf template has to build by hand. jhrg 5/11/18
```

### Example 1

### Example 2
### Example 3

## `ingest_filesystem` - building _dmr++_ files from local files.
The shell script `ingest_filesystem` is used to crawl through a branch of 
the local filesystem, identifying files that match a regular expression 
(default or supplied), and then attempting to build a _dmr++_ file for each 
matching file.

```
 Usage: ingest_filesystem [options] 

 Crawl filesystem and make a DMR++ for every dataset matching the
 default or supplied regex.

 The DMR++ is built using the DMR as returned by the HDF5 handler,
 using options as set in the bes configuration file used by get_dmrpp.
 
 -h: Show help
 -v: Verbose: Print the DMR too
 -V: Very Verbose: Verbose plus so much more!
 -j: Just print the DMR that will be used to build the DMR++
 -u: The base endpoint URL for the DMRPP data objects. The assumption
     is that they will be organized the same way the source dataset 
     files below the "data_root" (see -d)
     (default: file://$CWD)
 -d: The local filesystem root from which the data are to be ingested. 
     The filesystem will be searched beginning at this point for files 
     whose names match the dataset match regex (see -r).
     (default: $CWD)
 -t: The target directory for the dmrpp files. Below this point
     the organization of the data files vis-a-vis their "/" path
     separator divided names will be replicated and dmr++ files placed 
     accordingly.
     (default: $CWD)
 -r: The dataset match regex used to screen the base filesystem 
     for datasets. 
     (default: "^.*\\.(h5|he5|nc4)(\\.bz2|\\.gz|\\.Z)?$")
 -f: Use "find" to list all regular files below the data root directory 
     and store the list in "${ALL_FILES}" The the dataset match regex is applied
     to each line in ${ALL_FILES} and the matching data files list is placed in
     "${DATA_FILES}". If this option is omitted the files named in "${DATA_FILES}"
      (if any) will be processed.
     (default: Not Set)
```

## `ingest_s3bucket` - building _dmr++_ files from files held in S3.
The shell script `ingest_s3bucket` utilizes the AWS CLI to list the contents of an S3 bucket. The name of each object in the bucket checked against a (defaukt or supplied) regex. Each matching file is retrieved from S3 and then a _dmr++_ is built from the retrived data object. Once the _dmr++_ file is built the downloaded object is deleted.


``` 
 Usage: ingest_s3bucket [options] 
 
 List an AWS S3 bucket and make a DMR++ for every dataset matching the
 default (or supplied) regex.

 The DMR++ is built using the DMR as returned by the HDF5 handler,
 using options as set in the bes configuration file used by get_dmrpp.
  
 -h: Show help
 -v: Verbose: Print the DMR too
 -V: Very Verbose: Verbose plus so much more. Your eyes will water from
     the scanning of it all.
 -j: Just print the DMR that will be used to build the DMR++
 -s: The endpoint URL for the S3 datastore. 
     (default: ${s3_service_endpoint})
 -b: The S3 bucket name. 
     (default: ${s3_bucket_name})
 -d: The "local" filesystem root for the downloaded data. 
     (default: ./s3_data/bucket_name})
 -t: The target directory for the dmrpp files. Below this point
     the structure of the bucket objects vis-a-vis their "/" path
     separator divided names will be replicted and dmr++ placed into
     it accordingly.
     (default: ${target_dir})
 -f: Retrieve object list from S3 bucket into the list file for the bucket,
     apply the dataset match regex to the object names to create
     the data files list. If this is omitted the files named in an 
     exisiting bucket list file (if any) will be processed.
     (default: Not Set)
 -r: The dataset match regex used to screen the filenames 
     for matching datasets. 
     (default: ${dataset_regex_match})
 -k: Keep the downloaded datafiles after the dmr++ file has been 
     created. Be careful! S3 buckets can be quite large!

Dependencies:
This script requires that:
 - The bes installation directory is on the PATH.
 - The AWS Commandline Tools are installed and on the path.
 - The AWS Commandline Tools have been configured for this user with
   AWS access_key_id and aws_secret_access_key that have adequate permissions
   to access the target AWS S3 bucket.

```



## ChangeLog


5/25/18

- Added *.bescmd files for a number of new test files so that we can
tell if the hdf5 handler can read these files. If it cannot, then
it will be impossible to get a DMR file for input to build_dmrpp to
build the DMR++ file for this code. The new test files were made
using the mkChunkNewTypes.py script. jhrg 

4/3/19
- Renamed conf files to something less tedious, bes.hdf5.cf.conf(.in)
- Removed  bes.hdf5.cf.conf from git as it will now be built.
- Created "check-local" and "clean-local" targets that cause the 
bes.hdf5.cf.conf gile to be built correctly for use with get_dmrpp.sh
This all happens when running "make check"
