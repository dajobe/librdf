#!/usr/bin/perl -Tw
#
# demo.pl - Redland CGI database and query demo Perl program
#
# $Id$
#
# Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/
# Institute for Learning and Research Technology - http://www.ilrt.org/
# University of Bristol - http://www.bristol.ac.uk/
# 
# This package is Free Software or Open Source available under the
# following licenses (these are alternatives):
#   1. GNU Lesser General Public License (LGPL)
#   2. GNU General Public License (GPL)
#   3. Mozilla Public License (MPL)
# 
# See LICENSE.html or LICENSE.txt at the top of this package for the
# full license terms.
# 
# 
#

# CHANGE THIS FOR YOUR CONFIGURATION
$::ROOT_DIR='/somewhere';

use strict;

# Helps with broken web requests (missing headers)
$ENV{'Content-Length'}||=0;

# Tainting, dontcha know
$ENV{'PATH'}="/bin:/usr/bin:/usr/local/bin";

# Standard perl modules
use CGI;
use LWP::Simple;
use URI::URL;

# Configuration

my(@commands)=qw(destroy query parse add print),
my(%command_labels)=('destroy'  =>'Delete database',
		     'query'    =>'Find statements',
		     'add',     =>'Add a statement',
		     'parse',   =>'Parse RDF',
		     'print',   =>'Print database');
my(@parsers)=qw(raptor ntriples);
my(%parser_labels)=('raptor' => 'Raptor RDF/XML (Dave Beckett)',
		    'ntriples' => 'Raptor N-Triples (Dave Beckett)'
		    );
my(%parser_just_file_uris)=('raptor' => 1,
			    'ntriples' => 1
			    );
my $default_command='parse';

my $db_dir="$::ROOT_DIR/db";
my $tmp_dir="$::ROOT_DIR/tmp";
my $log_file="$::ROOT_DIR/logs/demo.log";


# Used for deleting databases
my @suffixes=qw(po2s so2p sp2o);
my $db_format="%s-%s.db"; # args: db suffix

# 1: Use stream parsing (print as it goes) or
# 0: parse into model (prints resulting model)
my $use_parse_stream=1;

my $max_stream_size=200;

my(%namespaces)=(
  'rdf' => 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
  'rdfs' => 'http://www.w3.org/2000/01/rdf-schema#',
  'rss' => 'http://purl.org/rss/1.0/',
  'synd' => 'http://purl.org/rss/1.0/modules/syndication/',
  'dc' => 'http://purl.org/dc/elements/1.1/',
);


# Redland perl modules

use RDF::Redland;


######################################################################
# Subroutines

sub log_action ($$$;$) {
  my($host, $db, $message, $now)=@_;
  $now ||= time;
  return unless open (LOG, ">>$log_file");
  my($sec,$min,$hour,$mday,$mon,$year)=gmtime $now;
  my $date=sprintf("%04d-%02d-%02dT%02d:%02d:%02dZ",1900+$year,$mon+1,$mday,$hour,$min,$sec);
  print LOG "$host $date $db $message\n";
  close(LOG);
}

sub end_page($) {
  my $q=shift;

  print <<'EOT';
<p>The source code of this demonstration is available in the Redland
distribution as <tt>demos/demo.pl</tt> or from the
<a href="http://www.redland.opensource.ac.uk/">Redland</a> website</p>
EOT

  print qq{<hr />\n\n<p class="copyright"><a href="http://purl.org/net/dajobe/">Dave Beckett</a></p>\n\n</body>\n</html>\n};
}

######################################################################
# Main code

my $q = new CGI;

# CGI parameter paranoia
my $val;
my $db;

$val=$q->param('db');
if(defined $val && $val =~ /^(\w+)$/) {
  $db=$1;
} else {
  $db=undef;
}

my $command;
$val=$q->param('command');
if(defined $val && $val =~ /^([a-z]+)$/) {
  $command=$1;
} else {
  $command=undef;
}

my $statement_string;
$val=$q->param('statement');
if(defined $val && $val =~ /^([ -~]+)$/) {
  $statement_string=$1;
} else {
  $statement_string='';
}

my $uri_string;
$val=$q->param('uri');
if(defined $val && $val =~ /^([ -~]+)$/) {
  $uri_string=$1;
} else {
  $uri_string=undef;
}

my $parser_string;
$val=$q->param('parser');
if(defined $val && $val =~ /^([a-z]+)$/) {
  $parser_string=$1;
} else {
  $parser_string=undef;
}

my $rdf_content='';
$val=$q->param('content');
# WARNING: pass through of all data
if(defined $val && $val =~ /^(.*)$/s) {
  $rdf_content=$1;
}

my $format_namespaces='';
$val=$q->param('format_namespaces');
if(defined $val) {
  $format_namespaces=($val eq 'yes');
}

# End of parameter decoding


# Used in logging
my $host=$q->remote_host;


######################################################################
# Emit content

print $q->header(-type  =>  'text/html', -charset=>'utf-8');

# Always print header
print <<"EOT";
<?xml version="1.0" encoding="iso-8859-1"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
  <title>Redland RDF Database Demo</title>
  <!-- HTML STYLE -->
</head>
<body>

<!-- LOGO START -->
<h1>Redland RDF Database Demo</h1>
<!-- LOGO END -->

<!-- DB BLURB HERE -->

<ol>
<li>Pick a database name</li>
<li><p>Add some RDF statements to the database in one of these ways:</p>
<ol>
  <li>Enter a statement by hand in the <em>Statement</em> field and
  use the <em>Add a statement</em> command.</li>
  <li>Enter some RDF/XML or N-Triples in the <em>Content</em> area and
  use the <em>Parse RDF</em> command.</li>
  <li>Give the URI of the RDF/XML or N-Triples on the web to parse in the <em>URI</em> field and
  use the <em>Parse RDF</em> command.</li>
</ol>
<li>Print the database - select the <em>Print database</em> command.</li>
<li>Query the database - either click on the statements from the result of the
previous command</li> <strong>or</strong> enter a statement in the <em>statement</em> field and use the <em>Find statements</em> command to return matching statements</li>
<li>Delete the database when you are done - thanks!</li>
</ol>


<p>The syntax of statements is as follows. A statement about a resource is written as:</p>
<pre>  [subject]--[predicate]-->[object]</pre>

<p>and a statement with a string value is written as:</p>
<pre>  [subject]--[predicate]-->"object"</pre>

<p>When used as a query, replace any of the parts with <tt>?</tt>
to match anything e.g.: <tt>?--?-->"foo"</tt> to
return all statements which have the object string <em>foo</em>.</p>

<!-- DB PRIVACY BLURB HERE -->

<hr />
EOT

# use q->url() to get URL of this script without any query parameters
# since we are using a POST here and don't want them added to the
# submission URL.
my $action_url="/".$q->url(-relative=>1);

print $q->start_form(-method=>'POST', -action => $action_url),"\n";

print "\n\n<p>Database <em>name</em> (<strong>required</strong>):\n";
print $q->textfield(-name=>'db',
		    -default=>'',
		    -size=>20,
		    -maxlength=>20);

print "\n\n with <em>Command</em> \n";
print $q->popup_menu(-name=>'command', 
		     -values=>\@commands,
		     -default=>$default_command, 
		     -labels=>\%command_labels);

print "\n\n<p><em>Statement</em> (for <em>Find</em> and <em>Add Statements</em> commands):\n";
print $q->textfield(-name=>'statement',
		    -default=>'',
		    -size=>80);

print "\n\n<p>RDF Parser for <em>Parse RDF</em> command\n";
print $q->popup_menu(-name=>'parser', 
		     -values=>\@parsers,
		     -default=>$parsers[0], 
		     -labels=>\%parser_labels);

print "<ol>\n<li><em>URI</em> of RDF content (or URI of content pasted below)<br/>\n";

print $q->textfield(-name=>'uri',
		    -default=>'',
		    -size=>80,
		    -maxlength=>1024);

print "</li>\n\n<li>RDF/XML or N-Triples <em>Content</em><br/>\n";
print $q->textarea(-name=>'content',
		    -default=>'',
		    -columns=>80,
		    -rows=>10);
print "</li>\n\n</ol>\n\n";

print "<p>Present RDF, RDFS, RSS, DC properties in namespace:qname form?\n";
print $q->popup_menu(-name=>'format_namespaces',
                     -values=>['yes','no'], -default=>'no');
print "<br />(This can make the tables a lot narrower and easier to read)</p>\n\n";

print "\n\n<p>";
print $q->submit('Go'),"\n";

print "</p>\n";
print $q->endform,"\n";

# Any parameters?
if(!$q->param) {
  end_page($q);
  exit 0;
}


print "<hr>\n";

######################################################################

# Validate me
if(!$db) {
  print "\n\n<p>You must enter a database name.</p>\n";
  end_page($q);
  exit 0;
}

if($command && !grep($_ eq $command, @commands)) {
  print "\n\n<p>No such command '$command'</p>\n";
  $command=undef;
}

if(!$command) {
  end_page($q);
  exit 0;
}

if($parser_string && !grep($_ eq $parser_string, @parsers)) {
  print "\n\n<p>No such parser '$parser_string' - valid list are @parsers</p>\n";
  $parser_string='';
}



my $db0="$db_dir/".sprintf($db_format, $db, $suffixes[0]);
if(! -r $db0) {
  my $storage=new RDF::Redland::Storage("hashes", $db, 
			       "new='yes',write='yes',hash-type='bdb',dir='$db_dir'");
  my $model;
  if($storage) {
    $model=new RDF::Redland::Model($storage, "");
  }
  if(!$storage && !$model) {
    log_action($host, $db, "Failed to create database");
    print "\n\n<p>Sorry - failed to create RDF Database $db.  This problem has been recorded.</p>\n";
    end_page($q);
    exit 0;
  }
  log_action($host, $db, "Created new database");
}


if($command eq 'destroy') {
  for my $suffix (@suffixes) {
    unlink "$db_dir/".sprintf($db_format, $db, $suffix);
  }
  log_action($host, $db, "Destroyed database");
  print "\n\n<p>Destroyed RDF database $db OK.</p>\n";
  end_page($q);
  exit 0;
}


# Remaining commands need an open database
# Write needed?
my $write=($command eq 'parse' || $command eq 'add') ? 'yes' : 'no';
my $storage=new RDF::Redland::Storage("hashes", $db, 
			     "new='no',write='$write',hash-type='bdb',dir='$db_dir'");
if(!$storage) {
  log_action($host, $db, "Failed to open storage");
  print "\n\n<p>Sorry - failed to open RDF Database $db.  This problem has been recorded.</p>\n";
  end_page($q);
  exit 0;
}

my $model=new RDF::Redland::Model($storage, "");
if(!$model) {
  log_action($host, $db, "Failed to create model");
  print "\n\n<p>Sorry - failed to open RDF Model for RDF Database $db.  This problem has been recorded.</p>\n";
  end_page($q);
  exit 0;
}

my $statement=undef;
my $stream=undef;

if($command ne 'print'  && $command ne 'parse') {
  if ($statement_string !~ m%^ (?: \? | \[[^]]+\])
                               \s*--\s*
                               (?: \? | \[[^]]+\])
                               \s*--\>\s*
                               (?: \? | [\["] [^]"]+ [\]"]) $%x) { # "
    print <<"EOT";
\n\n<p>Sorry, I do not understand the statement syntax; it should match that
described above.</p>
EOT
    end_page($q);
    exit 0;
  }

  my($subject_string,$predicate_string,$object_string)=(split(/-->?/, $statement_string, 3));

  if($subject_string =~ m%^\[(.+)\]$%) {
    $subject_string=$1;
  } else {
    $subject_string='?';
  }
  if($predicate_string =~ m%^\[(.+)\]$%) {
    $predicate_string=$1;
  } else {
    $predicate_string='?';
  }
  my($delim1,$delim2);
  if($object_string =~ m%^([\["])(.+)([\]"])$%) {
    ($delim1,$object_string,$delim2)=($1,$2,$3);
  } else {
    $object_string='?';
  }

  my $subject=($subject_string ne '?') ? RDF::Redland::Node->new_from_uri_string($subject_string) : undef;
  my $predicate=($predicate_string ne '?') ? RDF::Redland::Node->new_from_uri_string($predicate_string) : undef;
  my $object;
  if($object_string ne '?') { 
    if($delim1 eq '[') { # Assumes delim2 eq ']'
      $object=RDF::Redland::Node->new_from_uri_string($object_string);
    } else {
      $object=RDF::Redland::Node->new_from_literal($object_string, "", 0, 0);
    }
  } else {
    $object=undef;
  }

  $statement=RDF::Redland::Statement->new_from_nodes($subject, $predicate, $object);
  if(!$statement) {
    print "\n\n<p>Failed to create RDF statement from nodes</p>\n";
    end_page($q);
    exit 0;
  }

  # Now $statement is a RDF statement object if required
}

if($command eq 'add') {
  $model->add_statement($statement);
  print "\n\n<p>Added statement '$statement_string' to model</p>\n";
  log_action($host, $db, "Added statement '$statement_string'");
  end_page($q);
  exit 0;
}

my $temp_file;

if($command eq 'print') {
  $stream=$model->serialise;
  log_action($host, $db, "Printing database");
} elsif ($command eq 'query') {
  $stream=$model->find_statements($statement);
  log_action($host, $db, "Querying for statement '$statement_string'");
} else {
  my $uri;
  my $source_uri;
  my $uri_from_form=0;

  $parser_string ||= $parsers[0];

  if($uri_string) {
    eval "\$uri=new URI::URL(q{$uri_string});";
    if($@ || !$uri) {
      print "\n\n<p>URI \"$uri\" is not a legal URI (according to perl)</p>\n";
      end_page($q);
      exit 0;
    }

    if(!$uri->scheme || $uri->scheme ne 'http') {
      print "\n\n<p>Cannot use URI $uri_string - must be a web http URI.</p>\n";
      end_page($q);
      exit 0;
    }
  }

  if($rdf_content) { # have content on web page, maybe with URI
    $temp_file="$tmp_dir/redland-demo-$$.rdf";
    if(open(OUT, ">$temp_file")) {
      print OUT $rdf_content;
      close(OUT);
    } else {
      log_action($host, $db, "Failed to create $temp_file - $!");
      print "\n\n<p>Sorry - cannot create temporary file for RDF content.  This problem has been recorded.</p>\n";
      end_page($q);
      exit 0;
    }
    $source_uri=new URI::URL("file:$temp_file");
    if($uri) { # already validated above
      $uri=new URI::URL $uri;
    } else {
      $uri=new URI::URL("http://somewhere/");
    }
    $uri_from_form=1;
  } elsif($uri_string) { # else have only URI
    $source_uri=new URI::URL $uri;

    # Must fetch and copy to temp file
    if($parser_just_file_uris{$parser_string}) {
      $temp_file="$tmp_dir/redland-demo-$$.rdf";
      my $rc=getstore($uri_string, $temp_file);
      if(!is_success($rc)) {
	print "\n\n<p>Failed to read URI $uri_string - HTTP error $rc</p>\n";
	end_page($q);
	exit 0;
      }
      $source_uri=new URI::URL("file:$temp_file");
    }
  } else { # no content or URI
    print "\n\n<p>Must give a URI or RDF content for parse command</p>\n";
    end_page($q);
    exit 0;
  }

  my $parser=new RDF::Redland::Parser($parser_string);
  if(!$parser) {
    log_action($host, $db, "Failed to create RDF parser $parser_string");
    print "\n\n<p>Sorry - failed to create RDF parser $parser_string.  This problem has been logged.</p>\n";
    end_page($q);
    #unlink $temp_file if $temp_file;
    exit 0;
  }

  my $redland_base_uri=new RDF::Redland::URI $uri;
  my $redland_source_uri=new RDF::Redland::URI $source_uri;

  my $uri_label=$uri_from_form ? qq{RDF from form} : qq{URI "$uri"};

  log_action($host, $db, "Parsing $uri_label with parser $parser_string");

  if($use_parse_stream) {
    $stream=$parser->parse_as_stream($redland_source_uri, $redland_base_uri);
    if(!$stream || $stream->end) {
      print "\n\n<p>$uri_label failed to parse as RDF into model via stream with $parser_string parser</p>\n";
      end_page($q);
      #unlink $temp_file if $temp_file;
      exit 0;
    } else {
      print "\n\n<p>$uri_label parsed as RDF into model via stream with $parser_string parser OK</p>\n";
    }
  } else {
    $parser->parse_into_model($redland_source_uri, $redland_base_uri, $model);
    print "\n\n<p>$uri_label parsed as RDF into model with $parser_string parser OK</p>\n";
    #unlink $temp_file if $temp_file;
    $stream=$model->serialise;
  }
}


print <<"EOT";
<center>
<table align="center" border="1">
<tr>
<th>Subject</th>
<th>Predicate</th>
<th>Object</th>
<th>Follow<br />Object</th>
</tr>
EOT

#$RDF::Debug=1;
my $count=0;
for(;!$stream->end ;  $stream->next) {
  # Must use a new statement object here since $statement may be 
  # used/shared by the model's find_statements method
  my $statement2=$stream->current;

  last if !$statement2;

  my $subject=$statement2->subject->as_string;
  my $predicate=$statement2->predicate->as_string;
  my $object=$statement2->object->as_string;

  my $subject_label=$subject;
  my $predicate_label=$predicate;
  my $object_label=$object;
  if($format_namespaces) {
    for my $label_ref (\$subject_label, \$predicate_label, \$object_label) {
      while(my($ns_prefix,$ns_uri)=each %namespaces) {
        $$label_ref =~ s%^\[${ns_uri}([^/]+)\]$%[${ns_prefix}:$1\]%;
      }
    }
  }

  $q->param('command', 'query');

  $q->delete('content');  # HACK - makes generated URIs just too large

  $q->param('statement', "$subject--?-->?");
  my $subject_query=$q->self_url;

  $q->param('statement', "?--$predicate-->?");
  my $predicate_query=$q->self_url;

  if($object =~ /^\[(.+)\]$/) {
    $q->param('statement', "?--?-->[$1]");
  } else {
    $object=qq{"$object"};
    $q->param('statement', "?--?-->$object");
  }

  my $object_query=$q->self_url;

  my $from_object='';
  if($object =~ /^\[(.+)\]$/) {
    $q->param('statement', "[$1]--?-->?");
    $from_object=qq{<a href="}.$q->self_url.qq{">Follow</a>};
  } else {
    $from_object='&nbsp;';
  }

  print << "EOT";
<tr>
<td><a href="$subject_query">$subject_label</a></td>
<td><a href="$predicate_query">$predicate_label</a></td>
<td><a href="$object_query">$object_label</a></td>
<td>$from_object</td>
</tr>
EOT

  $count++;
  if ($count >= $max_stream_size) {
    print << "EOT";
<tr>
<td colspan="3">Truncated at $max_stream_size items - sorry, this is just a demonstration.</td>
</tr>
EOT
    last;
  }
  if($use_parse_stream) {
    if($command eq 'parse') {
      $model->add_statement($statement2);
    }
  }

} # while

if($use_parse_stream) {
  if($command eq 'parse') {
    #unlink $temp_file if $temp_file;
  }
}

print <<"EOT";
</table>
</center>
EOT

my $pl=($count != 1) ? 's' : '';
print "\n\n<p>Found $count statement$pl</p>\n";

if($format_namespaces) {
 print "<p>Where the following namespace prefixes were used:";
 while(my($ns_prefix,$ns_uri)=each %namespaces) {
   print qq{<br />\n${ns_prefix}: is namespace <a href="${ns_uri}">${ns_uri}</a>};
 }
 print "\n</p>\n\n";
}

end_page($q);
exit 0;
