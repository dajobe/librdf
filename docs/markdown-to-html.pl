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

open(IN, "$MARKDOWN $input |") or die "$program: pipe from $MARKDOWN $input failed - $\n";
open(OUT, ">$output") or die "$program: Cannot create $output - $\n";
my $title;
while(<IN>) {
  # Use first H1 to trigger head and title
  if(!defined $title && m%<h1>([^<]+)</h1%) {
    $title = $1;

    print OUT <<"HEADER";
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
  <title>Redland RDF Application Framework - $title</title>
  <style type="text/css">
<!--
pre
{
	margin: 1em 4em 1em 4em;
	background-color: #eee;
	padding: 0.5em;
	border-color: #006;
	border-width: 1px;
	border-style: dotted;
}
-->
  </style>
</head>
<body>
HEADER
  }

  # Fix markdown's removal of whitespace before first code line
  # Assumes all code indents with 2 initial spaces
  s%<pre><code>(.*)$%<pre><code>\n  $1%;

  print OUT "  $_";
}
close(IN);

print OUT <<"FOOTER";
<hr />

<p>Copyright (C) 2010 <a href="http://www.dajobe.org/">Dave Beckett</a></p>

</body>
</html>
FOOTER

close(OUT);

exit 0;
