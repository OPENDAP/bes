

Updated for version 1.1.2

See the INSTALL file for build and installation instructions.

fileout_json is a module to be loaded by the BES that will allow users to return a 'get dods' request as a JSON encoded response. 

An example is provided in the BES command line input file infile, which generates the file outfile. This is how one would use this input file:

bescmdln -h localhost -p 10002 -i infile -f outfile.test

And then you can compare the results of outfile with outfile.test. 

How does it work?

The get dods command in the input file tells the BES to generate a DataDDSresponse object. Typically, this DataDDS response object is "transmitted" back to the caller using a function that is registered with a basic transmitter that knows how to transmit a dods response.

If the caller adds to the command 'return as "json"' then the BES will try to find a transmitter called fojson and call the function registered with it that knows how to transmit a dods response.

Any questions, please contact Patrick West at ndp@opendap.org.

