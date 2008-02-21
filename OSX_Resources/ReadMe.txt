

 $Id: README 17516 2007-11-29 22:58:21Z pwest $

See the INSTALL file for build instructions.

For more information on Hyrax and the BES, please visit our documentationwiki at docs.opendap.org. This will include the latest install and buildinstructions, the latest configuration inforamtion, tutorials, how todevelop new modules for the BES, and more.

Contents  What's here: What files are in the distribution  About the BES: What exactly is this  Configuration: How to configure the BES  Testing: Once built and configured, how do you know it works?

* What's here:

Here there's a bes-config script which will be installing in $prefix/bin thatcan be used to determine where the libraries and their header files have beeninstalled. This directory also contains some documentation files.

dispatch: This is where the bulk of the BES source code resides.server: The BES server and standalone executables; build using dispatchcmdln: A command line client which can communicate with the bes server.

command: The BES/dispatch software is a framework. The basic commands	 recognized by  it are defined and implemented by software in this	 directory.apache: An Apache module, currently not part of the default buildppt: The PPT implementation. The BES uses PPT for its communicationtemplates: A collection of source files which can be used as templates when	 you write your own handlers/modules, et cetera.conf: Where the automake and autoconf configuration files livedocs: Where some bes documentation resides

* About the BES

* Configuration

Once the BES software has been installed, you will need to build and installat least one data handler and edit the bes.conf configuration file so thathandler can be used to serve data. I'll use the netcdf handler as an example.

Get the netcdf handler source code from http://www.opendap.org/download/,making sure that the version supports Hyrax (anything past 3.7.0 should).Expand the tar.gz file and follow the instructions with the following caveat:If you have installed the BES using a prefix other than /usr/local (thedefault), make sure that the correct bes-config is being run. If you arehaving problems compiling or linking the handler, try using not only--prefix=... but also --with-bes=... when you configure the handler. 

Install the newly-built handler.

Configure the BES to load the handler by editing the bes.conf file. You canfind this file in <prefix>/etc/bes/bes.conf. Look for the section withBES.modules and follow the example. The modules parameter contains a listof the modules that should be loaded by the server. You can use any name yowant for the modules; the BES.module.<name> parameter tells the serverwhich module to load and use for that name. Even though you _can_ use anyname you'd like, try to pick mnemonic ones since they will need to be usedlater on in the configuration file.

Next, find the point in the configuration file where theBES.catalog.TypeMatch parameter is located. The value of this parameteris a comma separated list that matches the name you used in theBES.modules parameter with different datasets. The BES uses regularexpressions to identify different types of datasets. In the example given inthe file, any dataset name that ends in '.nc' will be accessed using thenetcdf hander (because the name 'nc' is used here with the regular expression'.*\.nc'; note that the BES uses regular expressions like Unix grep, not fileglobbing patterns like a command shell). Since the name 'nc' was associatedwith the netcdf module in the modules section, the netcdf module will be usedto access any dataset whose name that ends in '.nc'. 

The regular expressions shown in the examples are simple. However, thenentire dataset name is used, so it's easy to associate different modules withdatasets based on much more than just the 'file name extension' even thoughthat is the most common case.

The renaming configuration parameters affect the BES as a whole, you are donewith the handler/module configuration.

Find the BES.Catalog.catalog.RootDirectory parameter. Set this to the rootdirectory of you data. If you're serving data stored in files, this is theplace in the file system where those files are stored. If you're servingdata from a relational database, see the documentation for the handler usedto read from the database about what value to use for this parameter.

Check the values of BES.Help.TXT and BES.Help.HTTP and BES.Help.XML to makesure they are correct.

At the top of the file, set the value of the ServerAdministrator parameterto a valid email address. This is the email address that will be displayedto users if errors arise.

If you would like to use the ascii, info, and html responses of BES, like inthe Server3 handlers when you append .ascii, .info, and .html to the URLrespectively, then you will need to get the dap-server source from OPeNDAPat http://www.opendap.org/download/#SVN. Follow the instructions for buildingand installing dap-server. When you install dap-server, three modules willbe installed into the <prefix>/lib/bes directory, ascii_module,usage_module, and www_module.  Add these modules to the BES.modulesparameter in the BES configuration file. An example is provided for youthere.

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

    [jimg@zoe sbin]$ besctl start


Note: Constraints and the bes command line client

Constraints are added to the 'define' command using the modifier 'with' likeso: 

    define d as nscat with nscat.constraint="WVC_LAT";


If there is a list of containers instead of just one, then there can be alist of <container>.constraint="" clauses.

COPYRIGHT INFORMATION

  The OPeNDAP BES code is copyrighted using the GNU Lesser GPL. See the  file COPYING or contact the Free Software Foundation, Inc., at 59 Temple  Place, Suite 330, Boston, MA 02111-1307 USA. Older versions of the BES were  copyrighted by the University Corporation for Atmospheric Research;  see the file COPYRIGHT_UCAR.