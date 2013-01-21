#!/usr/bin/perl -w

# This program generates X.{das|dds|ddx|data}.bescmd files for each X.hdf file
# in the input directory.
#
# Author: Hyo-Kyung Lee 
# Customize the following input and output directory..

use File::Basename;

my $input_dir = "../../data.hdfeos2/";
my $output_dir = ".";
my $handle;

sub copy {
    @suffixes = ("das", "dds", "ddx", "data");
    foreach $suffix (@suffixes) {
        $command = "cp ".$_[1]."/X.hdf.".$suffix.".bescmd ".$_[1]."/".$_[0].".".$suffix.".bescmd";
        print $command."\n";
        system($command);
    }
}

sub replace {
    $command = "/usr/bin/perl -p -i -e \"s/X/".basename($_[0], ".hdf")."/g\" ".$_[1]."/".$_[0].".*";
    print $command."\n";
    system($command);
}

# Open the directory where HDF4 files reside.
opendir($handle, $input_dir) or  die "Could not open $input_dir: $!";
while($filename = readdir($handle)){
# Filter out some files if necessary using the conditional statement
# like below.
    if($filename =~ /.hdf/) {
        # Copy the template files with a new file name. 
        copy($filename, $output_dir);
        # Replace the contents of the new files.
	replace($filename, $output_dir);
    }
}
