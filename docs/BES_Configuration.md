# Configuring the OPeNDAP BES

National Center for Atmospheric Research (NCAR)  
High Altitude Observatory (HAO)  
Boulder, CO.

Patrick West  
<mailto:pwest@hao.ucar.edu>

This document describes the settings in the BES configuration file.

The configuration file is installed in the BES installation's `etc/bes` directory. For example, if you install the BES into `/usr/local`, then `bes.conf` will be located in `/usr/local/etc/bes/bes.conf`. If you want to use a different configuration file, set the environment variable `BES_CONF` to the path of that file.

For example, if you used the `createModule` script (refer to `OpeNDAP_Creating_Module.doc`) then a `bes.conf` file is created for you in your module's directory. Set `BES_CONF` to point to that file to test your new module.

## 1. The `bes.conf` configuration file

The BES configuration file, `bes.conf`, contains key/value pairs used by the OPeNDAP BES. For the most part you only need to make four changes to this file, starting with the first three parameters listed below.

### 1.1 BES administrator

Set `OPeNDAP.ServerAdministrator` to the email address of the person who administers your OPeNDAP server installation. If error conditions occur within the BES, this email address will be reported as the contact for the installation.

### 1.2 BES logging

The `OPeNDAP.LogName` key defines the full path to the BES log file. This log file tracks what requests are made and whether they complete. The default is `./bes.log`, which places the log file in the current working directory where the BES was started.

The `OPeNDAP.LogVerbose` key defaults to `no`, meaning the system will not dump debug information to the log file. If you set it to `yes`, the BES writes more detail to the log. This can help determine whether there is a problem with the server and where that problem might be.

### 1.3 BES modules

The BES comes with a default set of commands. You can display help, version, and process information; list what keys are defined in the BES; set containers; create definitions; stream files back; and set context.

The BES provides a mechanism to load new modules into the server, creating ways to access different types of data, new commands, new aggregation, and more. Refer to `OpeNDAP_Creating_Module.doc` for more information on creating a new BES module.

The parameter `BES.modules` is set to the names of the modules you want to load. This is a comma-separated list of names that are used to construct keys in the configuration file used to locate the modules to be loaded. These parameters are called `BES.module.<module_name>` and should be set to the full path to the module library, usually named `lib<module_name>_module.so`.

There are examples in `bes.conf`. Here is one. Suppose you have a netCDF module to be loaded, and this netCDF module handles DAP requests. You would have the following parameters set in the configuration file:

```conf
BES.modules=dap,cmd,ascii,usage,www,nc
BES.module.dap=/usr/local/lib/bes/libdap_module.so
BES.module.cmd=/usr/local/lib/bes/libdap_cmd_module.so
BES.module.ascii=/usr/local/lib/bes/libascii_module.so
BES.module.usage=/usr/local/lib/bes/libusage_module.so
BES.module.www=/usr/local/lib/bes/libwww_module.so
BES.module.nc=/usr/local/lib/bes/libnc_module.so
```

As another example, suppose you are learning to create modules and have downloaded the `hello_world` module. You would have the following parameters set, because this module does not serve DAP requests:

```conf
BES.modules=say
BES.module.say=/usr/local/lib/bes/libsay_module.so
```

### 1.4 Data location

There are two different ways that the BES can serve data. The first is as a standalone server and the other is in coordination with the OLFS, which serves THREDDS catalog information.

For the standalone case, set `BES.Data.RootDirectory` to the full path to the data you will be serving. When creating containers that represent files, the path to the data file will be relative to this parameter.

For a BES that works with the OLFS, which serves THREDDS catalog information, set `BES.Catalog.catalog.RootDirectory` to the full path to the root directory for your catalog. This is used only when the catalog represents a file system. Catalogs can be represented in other ways, such as from a database.

When using the `BES.Catalog.catalog.RootDirectory` variable, you also need to set the following parameters.

The `TypeMatch` parameter is a list of handler/module names and a regular expression separated by a colon. If the regular expression matches an item in the catalog, the BES uses the associated handler/module. Each `<handler>:<regular expression>` pair is followed by a semicolon. This parameter is used when creating containers in the BES.

For example, if you are serving netCDF data and the name of the module is `nc`, then you might have a `TypeMatch`:

```conf
BES.Catalog.catalog.TypeMatch=nc:.*\.nc$;
```

The final two parameters related to catalogs are the `Include` and `Exclude` parameters. These parameters specify what files/directories are included or excluded from the catalog listing. The `Include` parameter is applied first, followed by `Exclude`. Only the `Exclude` parameter is applied to directories.

By default, all files and directories are included except those that begin with a dot (`.`).

### 1.5 Connection to the BES

A client can connect to your server either by specifying a port on the server machine or by using a UNIX socket.

The key `BES.ServerPort` sets the default port that the BES listens on, defaulting to `10002`. This can be overridden from the command line using the `-p` option to `besctl`.

The key `BES.ServerUnixSocket` should be set to the full path of your UNIX socket file. The default is `/tmp/opendap.socket`, which should work fine. This can also be overridden from the command line using the `-u` option to `besctl`.

### 1.6 Running a secure server

The BES can be run as a secure server. If it is, the server requires the client to authenticate using SSL certificates and keys.

Set `BES.ServerSecure` to `yes` if you will be running a secure server.

The SSL connection between the client and the server occurs over the port specified by `BES.ServerSecurePort`, which defaults to `10003`.

The remaining four parameters represent the locations of the SSL certificate and private key files. Two are set when this configuration file is used on the client side, and the other two are set when this configuration is used on the server side. Set these parameters to the full path of the respective files. The parameters are `BES.ServerCertFile`, `BES.ServerKeyFile`, `BES.ClientCertFile`, and `BES.ClientKeyFile`.

### 1.7 Help files

There are three help files that contain information returned to a user who makes a "show help" request. One key represents a plain text version of the file. The second represents an HTML-formatted help file, and the third represents an XML document response. The keys are `BES.Help.TXT`, `BES.Help.HTTP`, and `BES.Help.XML`. By default, these files are installed in the installation's `etc/bes` directory. For example, if BES is installed in `/usr/local`, then these files are located in `/usr/local/etc/bes`.

There are also help files for the DAP responses. These files are represented by the keys `DAP.Help.TXT`, `DAP.Help.HTTP`, and `DAP.Help.XML`.

### 1.8 Buffering

Should the OPeNDAP server buffer informational requests? That is what this key specifies. A value of `no` means that the information is not buffered and is transmitted as it is added to the informational response object. If set to `yes`, then the information is buffered in the object and not transmitted until all information is gathered.

```conf
OPeNDAP.Info.Buffered=no
```

### 1.9 Cache and compression

If a data file is compressed, the BES attempts to uncompress the file using the following parameters.

The BES does this by executing the shell script specified by the `BES.Compressed.Script` parameter. A default script is provided, so you do not need to modify this parameter.

To determine whether the data file is compressed, the BES compares the name of the file to the regular expression specified in `BES.Compressed.Extensions`. If none is specified, the default recognizes `.gz`, `.Z`, and `.bz2` compression styles.

The BES uncompresses the data into the directory specified by `BES.CacheDir`, which defaults to `/tmp`.

The BES also attempts to keep the size of the cache directory at a manageable level. If the size of uncompressed BES files exceeds `BES.CacheDir.MaxSize`, which defaults to `500 MB`, the BES attempts to remove the oldest files in the cache until the size is below the specified number.

### 1.10 Container persistence

A container represents a set of data. For example, a Cedar file is a container of data, and a NetCDF file is a container of data. For the most part, a container represents a file.

When defining a request, you specify what containers will be used for that request using a container's symbolic name. What if, when defining a request, the user specifies a container that does not exist? This key defines what to do. The key `BES.Container.Persistence` can be set to `nice`, meaning just log a message to the BES log file, or to `strict`, which means the system should throw an error message and stop processing the request. The default in the configuration file is `strict`.

### 1.11 Controlling memory usage and supporting dynamic memory requirements for exceptions

The key `BES.Memory.GlobalArea.EmergencyPoolSize` tells the BES how much memory to reserve for memory exceptions. Throwing C++ exceptions when memory is exhausted requires additional dynamically allocated memory, so this provides a small memory pool for exceptions to work if memory ever runs out. The value should be an integer representing the number of MB to reserve.

The key `BES.Memory.GlobalArea.ControlHeap` has possible values `{yes,no}`. This key tells the OPeNDAP server whether you want to control the maximum heap expansion for the BES processes. On most Unix systems, allocating memory expands the heap (if necessary). Even when you free memory (no leaks), the heap does not contract because `free` marks memory blocks as available for your process but does not return that memory to the OS. A BES process serving a huge amount of data can try to expand its heap to the point of creating process starvation (assuming the process stays alive after handling the request) for other processes running on the machine. It is suggested to set this value to `yes` if you expect heavy loads; otherwise keep it set to `no`.

If you set `BES.Memory.GlobalArea.ControlHeap` to `yes`, then you need to set `BES.Memory.GlobalArea.MaximumHeapSize`. This key tells the BES to expand the process heap to a predetermined value upon initialization. This size remains constant and works in two ways: it ensures the server will not take all the memory in your machine and it ensures there is enough memory to work properly. Depending on the size of your containers and the size of the queries you expect to serve (as well as how much memory is available on your machine) you can set this value to an integer between 10 and 400, representing MB.

The key `BES.Memory.GlobalArea.Verbose` has possible values `{yes,no}`. This key tells the BES to write extra debugging information in the log so developers and administrators can track memory management.

### 1.12 Single or multiple client connections at a time

As mentioned in the document `BES_Server_Architecture.doc`, the BES can process a single client connection at a time or multiple client connections at a time. The key `BES.ProcessManagerMethod` can be set to either `single`, which means only one client can connect at any given time, or to `multiple`, which means multiple clients can connect to the server at any one time.

### 1.13 Connecting with a browser

The key `BES.DefaultResponseMethod` is for internal use only and should not be set by the user.
