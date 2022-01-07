#!/usr/bin/perl -w

# Customize the following inputs.
my $InputDir = ".";
my $TargetFile = "../hdf4_handlerTest.hdfeos2.at";

sub Identify{
 my $WorkDir = $_[0];
 my $Handle;
 my $textfile = $_[1];
 my $WriteText;

 opendir($Handle, $WorkDir) or die "Could not open $WorkDir: $!";
 @files = readdir($Handle);
 closedir($Handle);
 @files_to_use = sort @files;

# while($filename = readdir($Handle)){
 foreach(@files_to_use){
     $filename = $_;
   unless ("$filename" eq "." or "$filename" eq ".." or "$filename" eq ".svn" or $filename =~/X/ ){
     open($WriteText, ">>", "$textfile") or die "Could not open $textfile: $!";
     if ($filename =~ /data.bescmd$/){
        print $WriteText "AT_BESCMD_BINARYDATA_RESPONSE_TEST([$filename])\n";
     }
     elsif (($filename =~ /das.bescmd$/) or ($filename =~ /dds.bescmd$/) or ($filename =~ /ddx.bescmd$/)) {
	print $WriteText "AT_BESCMD_RESPONSE_TEST([$filename])\n";
     }
     close $WriteText;
   }
 }
} 

Identify("$InputDir", "$TargetFile");
