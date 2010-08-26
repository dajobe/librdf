#!/usr/bin/perl -w
#
# Convert 'markdown' program output to legal XHTML that redland
# website building scripts can deal with
#
#

use strict;
use File::Basename;

our $program = basename $0;

our $MARKDOWN=$ENV{'MARKDOWN'} || 'markdown';

die "USAGE: $program: INPUT-MD OUTPUT-HTML\n"
  unless @ARGV == 2;

my($input,$output)=@ARGV;

die "$program: $input not found - $!\n" unless -r $input;

our $cmd = "$MARKDOWN $input | tidy -quiet -asxhtml -indent -utf8 --show-warnings 0 --add-xml-decl yes --wrap 78 -output $output";

warn "$program: $cmd\n";
system($cmd);
our $status = $?;
if ($? == -1) {
  die "$program: system $cmd failed to execute: $!\n";
}
elsif ($? & 127) {
  die "$program: system $cmd child died with signal %d, %s coredump\n",
  ($? & 127),  ($? & 128) ? 'with' : 'without';
}

$status = ($? >> 8);
# Tidy exit status: 1 on warnings, 2 on errors
die "$program: system $cmd failed returning exit $status\n"
  if $status > 1;

exit 0;
