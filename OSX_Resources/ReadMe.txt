

 $Id: README 23572 2010-09-14 20:44:44Z pwest $

Updated for version 3.9.0 of the OPeNDAP BES.

See the INSTALL file for build instructions.

For more information on Hyrax and the BES, please visit our documentationwiki at docs.opendap.org. This will include the latest install and buildinstructions, the latest configuration inforamtion, tutorials, how todevelop new modules for the BES, and more.

Contents

  * What's here: What files are in the distribution

  * About the BES: What exactly is this

  * Configuration: How to configure the BES

  * Testing: Once built and configured, how do you know it works?

* What's here:

Here there's a bes-config script which will be installing in $prefix/bin thatcan be used to determine where the libraries and their header files have beeninstalled. This directory also contains some documentation files.

dispatch: This is where the bulk of the BES source code resides.server: The BES server and standalone executables; build using dispatchcmdln: A command line client which can communicate with the bes server.

command: The BES/dispatch software is a framework. The basic commandsrecognized by  it are defined and implemented by software in this directory.

apache: An Apache module, currently not part of the default build

ppt: The PPT implementation. The BES uses PPT for its communication

templates: A collection of source files which can be used as templates whenyou write your own handlers/modules, et cetera.

conf: Where the automake and autoconf configuration files live

docs: Where some bes documentation resides

* About the BES

* Configuration

** Basic Configuration

Once the BES software has been installed, you will need to make a fewchanges to the BES configuration file, which is the bes.conf located in<prefix>/etc/bes/bes.conf. Module and handler configuration files willbe installed in the <prefix>/etc/bes/modules directory.

Only a few parameters need to be modified to get the BES up and runningfor the first time. These parameters are located at the top of theconfiguration file are are:

BES.ServerAdministrator- set this to an email address that can be used for users to contact if  there are issues with your installation of the server.

BES.User=user_nameBES.Group=group_name- set these to a valid username and groupname on your system. We  recommend that you create a username and groupname called bes that has  permissions to write only to the BES installation directory. We'll  need to write to the log file, and that's about it.

BES.LogName=./bes.log- Set this to the full path and file name for where you want the BES log  file to be written.

With this configuration you will be able to start the BES. No handlersor modules have been installed yet, so you won't be able to serve data,but the BES should run.

** Configuration for Hyrax

For the BES to run with Hyrax, additional changes will need to be madeto the dap.conf and dap-server.conf files, which are located in themodules directory <prefix>/etc/bes/modules. The dap module should beinstalled by default with the BES. If the dap.conf file is not found, besure that you have libdap installed. The dap-server module, known as theGeneral purpose handlers, adds responses for the DAP ascii response, theDAP info response, and the DAP html form response. You will need to getthe dap-server module. You should see a dap-server.conf file in themodules directory.

The changes required for Hyrax are:

BES.Catalog.catalog.RootDirectory=<path_to_root_data_directory>- Find the BES.Catalog.catalog.RootDirectory parameter in dap.conf. Set  this to the root directory of your data. If you're serving data stored  in files, this is the place in the file system where those files are  stored.

BES.Catalog.catalog.Include=;- This parameter specifies what files/directories are included in the  list of nodes in the catalog. The default is to show everything. After  this parameter is looked at, the Exclude parameter is then looked at  to see what files you might want to exclude.

BES.Catalog.catalog.Exclude=^\..*;- This parameter specifies what files/directories to include in the list  of nodes in the catalog. The default, shown here, is to exclude any  files or directories that starts with a dot (.)

The only possible configuration parameter that you may need to change isthe one that maps a file to a data handler. This parameter is calledBES.catalog.TypeMatch, and is found in each of the data handlerconfiguration files, such as nc.conf. The default values should work.

The value of this parameter is a semicolon separated list that matchesthe name you used in the BES.modules parameter with different datasets.The BES uses regular expressions to identify different types ofdatasets. In the example given in the file, any dataset name that endsin '.nc' will be accessed using the netcdf hander (because the name 'nc'is used here with the regular expression '.*\.nc'; note that the BESuses regular expressions like Unix grep, not file globbing patterns likea command shell). Since the name 'nc' was associated with the netcdfmodule in the modules section, the netcdf module will be used to accessany dataset whose name that ends in '.nc'.

The regular expressions shown in the examples are simple. However, theentire dataset name is used, so it's easy to associate different modules withdatasets based on much more than just the 'file name extension' even thoughthat is the most common case.

To test your regular expression for the TypeMatch parameter, or theInclude and Exclude parameters, use the supplied besregtest program.Simply run besregtest to discover its usage.

** Installing a handler/module

Once you have this configuration done you will need to build and installat least one data handler. I'll use the netcdf handler as an example.

Get the netcdf handler source code from http://www.opendap.org/download/,making sure that the version supports Hyrax (anything past 3.7.0 should).Expand the tar.gz file and follow the instructions with the following caveat:If you have installed the BES using a prefix other than /usr/local (thedefault), make sure that the correct bes-config is being run. If you arehaving problems compiling or linking the handler, try using not only--prefix=... but also --with-bes=... when you configure the handler. 

Install the newly-built handler.

Once built, install the handler using 'make install'. This will installthe BES module and a configuration file to use for that module. Eachmodule will have its own configuration file. In this case it is nc.confand installed in the <prefix>/etc/bes/modules directory. The next timethe BES is run, this configuration file will be read and the netcdfmodule loaded. No modifications are necessary.

* Testing

To test the server, open a new terminal window and start the bes by usingthe bes control script besctl, which is installed in <prefix>/bin.using the -c switch to name the configuration file. If the server standalonestarts correctly it should print something like the following to stdout:

    [jimg@zoe sbin]$ besctl start
    BES install directory: <prefix>/bin
    Starting the BES daemon
    PID: <process_id> UID: <uid of process>


Go back to your first window and run the bescmdln program. Use the -h (host)and -p (port) switches to tell it how to connect to the BES.

    [jimg@zoe bin]$ bescmdln -p 10002 -h localhost

    Type 'exit' to exit the command line client and 'help' or '?' to display
    the help screen

    BESClient>


Try some simple commands:

    BESClient> show help;
    <showHelp>
	<response>

	...

	</response>
    </showHelp>
    BESClient>


Note that all commands must end with a semicolon except 'exit'.

Now try to get a DAP response using a handler you registered (the BESsupports a fairly complex syntax which I won't explain fully here):

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


If you got something similar (you would have used a different dataset namefrom "data'nc'fnoc1.nc" and thus would get a different DAS response) the BESis configured correctly and is running. 

To stop the BES use the bes control script with the stop option:

    [jimg@zoe sbin]$ besctl stop


Note: Constraints and the bes command line client

Constraints are added to the 'define' command using the modifier 'with' likeso: 

    define d as nscat with nscat.constraint="WVC_LAT";


If there is a list of containers instead of just one, then there can be alist of <container>.constraint="" clauses.

COPYRIGHT INFORMATION

  The OPeNDAP BES code is copyrighted using the GNU Lesser GPL. See the  file COPYING or contact the Free Software Foundation, Inc., at 59 Temple  Place, Suite 330, Boston, MA 02111-1307 USA. Older versions of the BES were  copyrighted by the University Corporation for Atmospheric Research;  see the file COPYRIGHT_UCAR.