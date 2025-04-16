#!/usr/bin/perl
#

use strict 'vars';
my $debug = 0;

my $readme = $ARGV[0];
my $version_file = $ARGV[1];
my $package_dir = $ARGV[2];

unix2mac($readme, "OSX_Resources/ReadMe.txt");

###################################################################

# Substitute values into the named plist file. This probably only works 
# with an Info.plist file.
sub substitute_values {
  my ($info_plist, $version_number, $package_size) = @_;

  my $get_info = 0;
  my $short_version = 0;
  my $installed_size = 0;

  open INFO_PLIST, $info_plist or die("Could not open $info_plist\n");
  open OUT, ">$info_plist.out"  or die("Could not open $info_plist.out\n");

  while (<INFO_PLIST>) {
    if ( /CFBundleGetInfoString/ ) {
      $get_info = 1;
      print OUT $_ ;
    } elsif ( /CFBundleShortVersionString/ ) {
      $short_version = 1;
      print OUT $_ ;
    } elsif ( /IFPkgFlagInstalledSize/ ) {
      $installed_size = 1;
      print OUT $_ ;
    } elsif ( $get_info == 1 && /^(\s*)<string>[0-9.]+, (.*)<\/string>/ ) {
      print OUT "$1<string>$version_number, $2</string>\n" ;
      $get_info = 0;
    } elsif ( $short_version == 1 && /^(\s*)<string>[0-9.]+<\/string>/ ) {
      print OUT "$1<string>$version_number</string>\n" ;
      $short_version = 0;
    } elsif ( $installed_size == 1 && /^(\s*)<integer>.*/ ) {
      print OUT "$1<integer>$package_size</integer>\n" ;
      $installed_size = 0;
    } else {
      print OUT $_ ;
    }
  }

  close INFO_PLIST;
  close OUT;

  rename "$info_plist.out", $info_plist 
    or die("Could not rename $info_plist\n");
}

# Substitute architecture into the named InstallCheck script.
sub substitute_arch {
  my ($inst_check) = @_;

  my $sys_arch = `arch`;
  my $my_arch = "" ;
  if ( $sys_arch =~ "ppc.*" ) {
    $my_arch = "ppc.*" ;
  } elsif ( $sys_arch =~ "i386.*" ) {
    $my_arch = "i386.*" ;
  }

  open INST_CHECK, $inst_check or die("Could not open $inst_check\n");
  open OUT, ">$inst_check.out"  or die("Could not open $inst_check.out\n");

  while (<INST_CHECK>) {
    if ( /^architecture=/ ) {
      print OUT "architecture=\"$my_arch\"\n" ;
    } else {
      print OUT $_ ;
    }
  }

  close INST_CHECK;
  close OUT;

  rename "$inst_check.out", $inst_check 
    or die("Could not rename $inst_check\n");
}

# Find the size in kilobytes of the given directory
sub get_package_size {
  my ($package_dir) = @_;
  my $result = `du -ks $package_dir`;

  my ($package_size) = $result =~ m/(\s*[0-9]+).*/
    or die("Could not figure out the package size!\n");
  return $package_size;
}

# Look for the version number in the Makefile or configure.ac 
sub get_version_number {
  my ($infile_name) = @_;
  my $version_number = "0.0.0";

  open IN, $infile_name or die("Could not open $infile_name\n");

  while (<IN>) {
    if ( /PACKAGE_VERSION=([0-9.ab]+)/ ) {
      $version_number = $1;
    }
    elsif ( /AC_INIT\(bes, *([0-9.]+)/ ) {
      $version_number = $1;
    }
  }

  close IN;

  return $version_number;
}

# Read a textfile where each line is terminated by a newline and
# paragraphs are terminated by an otherwise blank line. Write the text
# out without those pesky line-terminating newlines.
sub unix2mac {
  my ($infile_name, $outfile_name) = @_;

  open IN, $infile_name or die("Could not open $infile_name!\n");
  open OUT, ">$outfile_name" 
    or die("Could not open output for $outfile_name!\n");

  my $code = 0;

  while (<IN>) {
    if ( /^<code>\s*$/ ) {
      $code = 1;
    } elsif ( /^<\/code>\s*$/ ) {
      $code = 0;
    } elsif ( $code eq 1 ) {
      print OUT $_ ;
    } elsif ( /^\s*$/ ) {
      print OUT "\n\n" ;	# Blank line
    } else {
      # the [\015] is the DOS ^M character.
      chomp $_ ; tr/[\015]/ / ; print OUT $_ ; # Character line
    }
  }

  close IN;
  close OUT;
}

