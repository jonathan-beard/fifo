#!/usr/bin/env perl
use strict;
use warnings;

my $count = 0;
for( $count < 10 )
{
   my $logfile = "/project/mercury/svardata/log_5eNeg10_".$count;
   `./ringb 2> $logfile`;
}
