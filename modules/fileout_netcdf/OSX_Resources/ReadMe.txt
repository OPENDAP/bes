

Updated for version 1.1.2

See the INSTALL file for build and installation instructions.

fileout_netcdf is a module to be loaded by the BES that will allow users toreturn a 'get dods' request as a netcdf file. A file is generated and thenstreamed back in the response.

An example is provided in the BES command line input file infile, whichgenerates the file outfile. This is how one would use this input file:

bescmdln -h localhost -p 10002 -i infile -f outfile.test

And then you can compare the results of outfile with outfile.test. Youshould be able to run ncdump against this output file and it should besomewhat similar to the fnoc1.nc file in share/hyrax/data/nc.

How does it work?

The get dods command in the input file tells the BES to generate a DataDDSresponse object. Typically, this DataDDS response object is "transmitted"back to the caller using a function that is registered with a basictransmitter that knows how to transmit a dods response.

If the caller adds to the command 'return as "netcdf"' then the BES will tryto find a transmitter called netcdf and call the function registered with itthat knows how to transmit a dods response. So, if you look at FONcModuleyou will see the FONcTransmitter is registered with the ReturnManager withthe name "netcdf".  The constructor of this new transmitter adds a functionthat knows how to transmit a dods response, see FONcTransmitter.cc. So, whenit comes time to transmit the response it sees that the caller has specifieda specific transmitter ("netcdf"), the BES looks up that transmitter, thenlooks up a function to handle the response ("dods"), calls that function andthe function transmits the response using the output stream from theBESDataHandlerInterface (getOutputStream).

The FONcTransmitter first takes the response object passed, grabs theDataDDS object, and calls the read method on it to make sure that all of thedata is read into the response object. Remember, this is lazy evaluation, sowhen the DataDDS response object is created, data structures are added toit, but no data is actually read into it. At least, this is the case formost of the handlers. Some read the data in directly, so the read call justreturns.

After calling read, the FONcTransmitter hands over everything to theFONcGridTransform class to write the data structure and data out to thenetcdf file, and then that file is streamed (transmitted) back to the client.

Any questions, please contact Patrick West at pwest@opendap.org.

