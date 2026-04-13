# Building a New OPeNDAP BES Module

National Center for Atmospheric Research (NCAR)  
High Altitude Observatory (HAO)  
Boulder, CO

Patrick West  
[pwest@hao.ucar.edu](mailto:pwest@hao.ucar.edu)

## Overview

This manual guides you through creating a new OPeNDAP BES module using the `besCreateModule` script. It explains which files are created, which functions you need to implement, and how to build and run your module. It assumes you understand the OPeNDAP data model. If not, see [http://www.opendap.org/](http://www.opendap.org/).

For more information on the server architecture, see `docs/BES_Framework.md`. For more information on configuration, see `docs/BES_Configuration.md`.

## 1. Creating a New OPeNDAP Module

We provide a script that generates code that will compile immediately. It creates the source files you will extend, along with configuration files and makefiles.

### 1.1 Using the `besCreateModule` Script

Run the script from the command line:

```sh
./besCreateModule
```

The script asks four questions.

**Question 1**

```
Enter server type, e.g. cedar, fits, netcdf:
```

This is a short, lowercase identifier for your data type. For example, use `cedar` or `hdf5`.

**Question 2**

```
Enter C++ class prefix, e.g. Cedar, Fits, Netcdf:
```

This prefix is used for generated C++ class names. For example, `Cedar` or `HDF5`. We recommend starting with an uppercase letter, but it is not required.

**Question 3**

```
Enter response types handled by this server (das dds data):
(space separated. help and version are added for you, no need to include them here)
```

If your module provides DAS, DDS, and Data objects, press Enter to accept the default (`das dds data`). If your module only provides data responses, enter `data`. You can also add new response types. For example, Cedar adds `tab`, `flat`, `info`, and `stream`, so you would enter:

```
das dds data tab flat info stream
```

**Question 4**

```
Enter new commands you are implementing:
```

If you are adding custom BES commands (beyond the built-in `show`, `set`, `get`, etc.), list them here. Entering nothing skips command generation. Each command creates a stub XML command parser and a response handler.

After you answer the questions, the script creates files. For example, if you enter `hdf`, `HDF`, and accept the default responses, it creates:

- `HDFModule.cc`
- `HDFModule.h`
- `HDFRequestHandler.cc`
- `HDFRequestHandler.h`
- `HDFResponseNames.h`
- `Makefile.am`
- `configure.ac`
- `bes-hdf-data.sh.in` (install-time helper script)
- `COPYRIGHT`, `COPYING`, `NEWS`, `README` (if present in templates)
- `hdf_module.spec` (if the spec template is present)

If you add new response types, the script also creates two files per response type:

- `HDF<response_type>ResponseHandler.cc`
- `HDF<response_type>ResponseHandler.h`

If you add new commands, the script also creates two files per command:

- `HDF<command>XMLCommand.cc`
- `HDF<command>XMLCommand.h`

A new `conf` directory is also created with build configuration files **if** the templates include a `conf/` directory. If you are missing those templates, update the templates before running the script.

The script then runs `autoreconf`, `configure`, and `make`. The code builds a BES module library that can be loaded dynamically into the BES. This assumes you have already built the OPeNDAP code (libdap and BES).

The script runs `./configure` with default settings. If you need a specific install prefix or dependency path, rerun `./configure` yourself with the appropriate flags before building.

Now you need to implement the code that builds your responses. You can update `configure.ac` and `Makefile.am` to include any libraries or headers your module needs.

## 2. Writing Your Code

### 2.1 Request Handlers

If you did not add new response types, you only need to modify the `RequestHandler.cc` file. This is where response objects are populated. For example, if the response object is a DAS object, your request handler will add attribute tables and attributes to the passed DAS object in the `build_das` function.

We recommend creating separate source files and functions for each response type. For example, if you provide DAS, DDS, and Data responses, you can implement separate functions in separate files and call those functions from within your `RequestHandler` class.

### 2.2 New Response Types

If you added new response types, you must modify the `<response_type>ResponseHandler.cc` file for each one. This is where you tell the system how to create the response object.

The distinction is important:

- The **request handler** populates the response object.
- The **response handler** creates the response object and decides how it will be populated (for example, by calling a specific request handler, or all request handlers).

For example, a DAS response handler creates a DAS response object and, for each container specified in the request, calls the appropriate build function in the request handler for that container type.

Another example is the `help` response. For each request handler (data type) served by this server, the response handler calls the request handler's help build function. By default, your new module handles `help` and `version` requests.

The response handler also knows how to transmit the response object. For example, the DAS response handler calls the `send_das` method on the transmitter object passed to its `transmit` method. This supports different transmission mechanisms, such as HTML for browsers, XDR for binary data, or plain text. You can create new transmitter classes and specify them by name using the request syntax `return as <unique_name>`.

### 2.3 Other Source Code Extensions

You can extend the BES in additional ways through your dynamically loaded module. See Section 7, "Other Add-ons for Your New Server." for details.

## 3. The `bes.conf` Configuration File

Before you run the server, update `bes.conf`. This file contains key/value pairs used by the BES. For more information, see `docs/BES_Configuration.md`.

## 4. Starting and Stopping the Server

Once you have built the module and updated `bes.conf`, you are ready to start the server.

First, point the BES at your configuration file. The BES default configuration file is installed under `etc/bes`. For example, if you installed the BES into `/usr/local`, the default file is at `/usr/local/etc/bes`. The `besCreateModule` script does **not** generate `bes.conf`; it generates a `bes-<type>-data.sh` helper script (from `bes-<type>-data.sh.in`) that can update an existing `bes.conf` after install. Edit `bes.conf` as needed and pass that file to `besctl` using `-c`. If you prefer, you can pass an install prefix with `-i` and the BES will look for `etc/bes/bes.conf` under that prefix. Some helper scripts still honor `BES_CONF` and translate it into `-c` for `besstandalone`, but `besd`/`besctl` do not read `BES_CONF` directly.

Start the server:

```sh
besctl start -c /full/path/to/bes.conf
```

To see debug information while running the server daemon:

```sh
besctl start -d cerr
```

If you have problems with installation or requests, it can be helpful to run with debug enabled and turn on verbose logging in the configuration file.

Stop the server:

```sh
besctl stop
```

This sends a signal to shut down the BES.

## 5. Running the Client

Start the command-line client:

```sh
bescmdln -h localhost -p 10002
```

The `-h` option specifies the host running the BES. The `-p` option specifies the port. The default port, set in the BES configuration file, is `10002`. If you changed this, or started the server with `-p`, use the updated port here.

If you only use `-h` and `-p`, the client starts in interactive mode and displays a prompt. Each command must end with a semicolon.

Example commands:

```
BESClient> show status;
BESClient> show help;
BESClient> exit
```

The first command shows server status, the second shows help information, and `exit` ends the session.

### 5.1 Command-Line Options (`bescmdln`)

- `-u` specifies a Unix socket for connecting to the server.
- `-h` specifies the host for TCP/IP connections.
- `-p` specifies the port for TCP/IP connections.
- `-x` executes a query and exits. This requires `-f`.
- `-f` sets the target file name for the return stream from the server.
- `-i` sets the target file name for a sequence of input commands.
- `-t` sets the timeout in seconds (optional).
- `-d cerr|<filename>` enables client debugging (optional).
- `-v` prints the version and exits.

**Connection flags:** Use either `-u` or `-h` and `-p` to connect.

**Input/Output flags:** You can supply input on the command line with `-x` or from a file with `-i`. If you use `-x` or `-i`, you must also provide an output file with `-f`. If neither `-x` nor `-i` is specified, the client runs in interactive mode. To exit interactive mode, type `exit` at the `BESClient>` prompt.

## 6. Client Commands

If you add new response types, additional commands will appear under `get`. The defaults are shown here.

### 6.1 Core Commands

```
show help;
```
Shows help.

```
show version;
```
Shows the version of OPeNDAP and each data type served by this server.

```
show process;
```
Shows the process number of this application.

```
show status;
```
Shows server status.

```
show keys;
```
Shows all keys defined in the OPeNDAP initialization file.

```
show containers;
```
Shows all containers currently defined.

```
show context;
```
Shows all context name/value pairs set in the BES.

```
set container [in <storage_name>] values <symbolic_name>,<real_name>,<container_type>;
```
Defines a symbolic name for a data container, usually a file, to be used by definitions. `storage_name` defaults to volatile storage. `real_name` is the full path to the data file. `container_type` is the type of data in the file (for example, `nc` for NetCDF or `cedar` for Cedar).

```
set context <context_name> to <context_value>;
```
Sets a context name/value pair. No default contexts are available in the BES.

```
define <def_name> [in <storage_name>] as <container_list> [where <container_x>.constraint="<constraint>",<container_x>.attributes="<attribute_list>"] [aggregate by "<aggregation_command>"];
```
Creates a definition using one or more containers, with optional constraints, attributes, and aggregation.

```
delete container <container_name> [from <storage_name>];
```
Deletes the specified container from the specified container storage (defaults to volatile storage).

```
delete containers [from <storage_name>];
```
Deletes all currently defined containers from the specified container storage (defaults to volatile storage).

```
delete definition <definition_name> [from <storage_name>];
```
Deletes the specified definition from the specified container storage (defaults to volatile storage).

```
delete definitions;
```
Deletes all currently defined definitions from the specified container storage (defaults to volatile storage).

Remember to terminate each command with a semicolon (`;`). For more information, contact Patrick West at [pwest@ucar.edu](mailto:pwest@ucar.edu).

### 6.2 Commands for DAP-Enabled Servers

If you are serving OPeNDAP data responses (DAS, DDS, DataDDS), you will have loaded the DAP commands in your configuration file. Available commands include:

```
show catalog [for 'node'];
```
Shows catalog information, including contents if `node` is a container. If `node` is not specified, root node information is returned. The node name must be in single quotes.

```
show info [for 'node'];
```
Shows catalog information for a node (or the root node if no node is specified). If the node is a container, contents are not displayed. The node name must be in single quotes.

```
get das|dds|dods|ddx for <definition_name> [return as <return_name>];
```
- `dds`: request the data descriptor structure
- `das`: request the data attributes
- `dods`: request the data stream. This output is an octet binary stream and requires analysis by the client DODS library.

```
set context errors to <dap2|xml|html|txt>;
```
Sets the `errors` context to format exceptions and errors as DAP2 error messages in the response.

Remember to terminate each command with a semicolon (`;`). For more information, contact Patrick West at [pwest@ucar.edu](mailto:pwest@ucar.edu).

## 7. Other Add-ons for Your New Server

In addition to new response types, you can add other modules to your server. Options include:

- New response handlers for additional commands (for example, user info or storage manipulation).
- New container storage implementations. The default is volatile storage (per session). You can add persistent storage (for example, MySQL).
- New definition storage implementations for persistent definitions.
- Aggregation handlers for combining results.
- Reporters that receive request summaries for logging, billing, or auditing.
- New transmission mechanisms via the `return as` command (for example, email, file output, or alternative formats).
- Initialization and termination callbacks for server lifecycle hooks.
- Custom exception handling callbacks before default handling.

If you create a useful add-on, consider contributing it back to the project.

### 7.1 New Response Handlers

You can add commands and response handlers beyond `get`. For example, you might implement user information queries, storage manipulation, or debugging commands.

To create your own response handler, add a command to the command list and write a response handler class that creates the response object. You may also add a request handler method to populate the response object.

See the "hello world" example module in the SVN repository for a simple example.

### 7.2 Creating a New Way of Storing Container Information

Currently, persistent container storage is not provided (storage is volatile per session). You can add user-specific storage or store container information in a database such as MySQL. The key steps include:

- Create a persistence class.
- Add the persistence class to the persistence list.
- Define how containers are referenced and stored.

The user method can serve as an example; this section is a placeholder for future expansion.

### 7.3 Aggregating Your Data

By default, the OPeNDAP server provides no aggregation. You can register an aggregation handler that is invoked after data retrieval to combine or transform results.

### 7.4 Reporting Mechanism (Reporter)

OPeNDAP allows you to report on server activity. You can write reports to a file (as Cedar does), a MySQL database, or integrate with a billing system. Reportable information includes user names, timestamps, request details, and accessed files.

### 7.5 Transmitting Your Data with the "return as" Command

By default, responses are sent to `stdout`, which is connected to the client socket for the standalone server. You can instead transmit to a file, write a different data format, or send results via email.

### 7.6 Connecting a Different Way

The provided client and server use command syntax like `show help;` or `get das;`. You can build alternative clients. For example:

- A web interface that constructs commands and renders results.
- A SOAP interface with server-invoked methods.
- A Java interface connected through Tomcat (as in Server 4), or a more user-friendly Java client.
