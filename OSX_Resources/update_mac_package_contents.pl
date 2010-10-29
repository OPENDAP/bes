#!/usr/bin/perl
#

use strict 'vars';
my $debug = 0;

my $readme = $ARGV[0];
my $version_file = $ARGV[1];
my $package_dir = $ARGV[2];

unix2mac($readme, "OSX_Resources/ReadMe.txt");

###################################################################

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

