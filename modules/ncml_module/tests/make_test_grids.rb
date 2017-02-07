#/usr/bin/env ruby

###########################################################
# Simple file for generating basic Netcdf Grid files
# with simple known data in order to test joinExisting.
# Author: Michael Patrick Johnson <m.johnson@opendap.org>

# This file requires that the Netcdf bindings for Ruby
# (and it's prereq NArray) be installed in the ruby env!
# Please see: http://ruby.gfd-dennou.org/products/ruby-netcdf/

require "numru/netcdf"
include NumRu

# The directory to output the files, relative to this one.
OutputPath = "../data/nc/simple_test/"

# Size of x dimension
NX = 3

# Size of time dimension 
NT = 1

def make_test_file(filename, nx, nt, offset)
  puts "Creating file: #{filename}..." 
  file = NetCDF.create(filename)

  xdim = file.def_dim("x", nx)
  tdim = file.def_dim("time", nt) 
  
  x = file.def_var("x", "int", [xdim])
  x.put_att("Description", "Test coordinate variable x.");
  time = file.def_var("time", "int", [tdim])
  time.put_att("Description", "Test coordinate variable time.");
  v = file.def_var("v", "sfloat", [xdim, tdim])
  v.put_att("Description", "Test output data array v");
  file.enddef
  
  x.put( NArray.float(nx).indgen! )
  time.put( NArray.float(nt).indgen! + offset )
  v_data = (NArray.float(nx,nt).indgen! + (offset * nx) )*0.1
  v.put(v_data)
  
  file.close
  
  dumpcmd = "ncdump #{filename}"
  puts dumpcmd
  print `#{dumpcmd}`
end

make_test_file(OutputPath + "test_grid_0.nc", NX, NT, 0)
make_test_file(OutputPath + "test_grid_1.nc", NX, NT, 1)
make_test_file(OutputPath + "test_grid_2.nc", NX, NT, 2)

