#!/usr/bin/env perl
use strict;
use warnings;

my $timesuffix = ".0e-6";
my $machine    = `uname -n`;
$machine =~ tr/\./_/;


for my $times( 1 ... 10 )
{
   for my $count( 0 ... 19 )
   {
      my $cmd = "ringb 10.0e-6 ".$times.$timesuffix." 1000000000";
      my $file = $cmd;
      $file =~ tr/ /_/;
      $file .= "_".$machine;
      print "./".$cmd." 2> /project/mercury/svardata/servicedata/$file\_$count.dat \n";
   }
}
