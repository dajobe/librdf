#!/usr/bin/perl -pi~
#
# Perl script to help update the Redland code (in most languages) for
# the API changes in 0.9.12
# 1) zapping xml_space arguments in literal constructors, methods
# 2) iterator and stream method changes - can only warn about where the
#    problem is.
# 3) librdf_model_add_statement statement argument remains owned by caller
#
# $Id$
#
# USAGE: update-api-0912.pl <source files - C, perl, python, Java, ruby>
#

# The literal method arg fixing works for most of the languages except
# for Tcl which doesn't use ()s for args.  You'll need to do that by
# hand for these methods:
#
# Class Node
#   librdf_new_node_from_literal
#   librdf_node_set_literal_value
# Class Model
#   librdf_model_add_string_literal_statement


# General subsitutions - C, Java
s/librdf_new_node_from_literal\(([^,]+),([^,]+),([^,]+),([^,]+),([^\)]+)\)/librdf_new_node_from_literal\($1,$2,$3,$5\)/g;
s/librdf_node_set_literal_value\(([^,]+),([^,]+),([^,]+),([^,]+),([^\)]+)\)/librdf_node_set_literal_value\($1,$2,$3,$5\)/g;
s/librdf_model_add_string_literal_statement\(([^,]+),([^,]+),([^,]+),([^,]+),([^,]+),([^,]+),([^,]+)\)/librdf_model_add_string_literal_statement\($1,$2,$3,$4,$5,$7\)/g;

if(m%librdf_(stream|iterator)_(next|get_next)\(([^\)]+)\)%) {
  my($class,$method,$name)=($1,$2,$3);
  warn "$.: This probably needs updating for $class API changes.  Replace librdf_${class}_$method($name) with librdf_${class}_get_object($name) and add librdf_${class}_next($name) at the end of the $class loop\n  $_\n";
}

if(m%librdf_model_add_statement\([^,]+,\s*([^\)]+)\)%) {
  my($name)=$1;
  warn "$.: The caller of this method now remains owner of the statement argument.  You may reuse the argument and must free it later with librdf_free_statement($name) later.\n  $_\n";
}


# Perl
s/RDF::Redland::Node->new_from_literal\(([^,]+),([^,]+),([^,]+),([^\)]+)\)/RDF::Redland::Node->new_from_literal\($1,$2,$4\)/g;
# Method on a node object
s/set_literal_value\(([^,]+),([^,]+),([^,]+),([^\)]+)\)/set_literal_value\($1,$2,$4\)/g;
# Method on a model object
s/add_string_literal_statement\(([^,]+),([^,]+),([^,]+),([^,]+),([^,]+),([^,]+)\)/add_string_literal_statement\($1,$2,$3,$4,$6\)/g;

if(m%\$(\w+)-\>next%) {
  my $name=$1;
  if($name =~ /(stream|iterator)/i) {
    my $class=$1;
    warn "$.: This probably needs updating for $class API changes.  Replace \$$name->next with \$$name->current and add \$$name->next at the end of the $class loop\n  $_\n";
  }
}

if(m%-\>add_statement\(([^\)]+)\)%) {
  my($args)=$1;
  warn "$.: The caller of this method now remains owner of the statement argument.  You may reuse the argument or free it.\n  $_\n";
}


# Python
# the xml_space is a dictionary key/value, not a method that needs changed

# This might also catch other languages
if(m%(\w+)\.next\(\)%) {
  my $name=$1;
  if($name =~ /(stream|iterator)/i) {
    my $class=$1;
    warn "$.: This probably needs updating for $class API changes.  Replace $name.next() with $name.current() and add $name.next() at the end of the $class loop\n  $_\n";
  }
}

if(m%\w+\.add_statement\(([^\)]+)\)%) {
  my($args)=$1;
  warn "$.: The caller of this method now remains owner of the statement argument.  You may reuse the argument or free it.\n  $_\n";
}

# Tcl
# has to be mostly done by hand



if (/librdf_node_get_literal_xml_space/) {
  # C, Tcl, Java - librdf_node_get_literal_xml_space has been deleted
  warn "$.: librdf_node_get_literal_xml_space seen - this method has been removed\n  $_\n";
} elsif (/get_literal_xml_space/) {
  # Perl
  warn "$.: get_literal_xml_space model seen - this method has been removed\n  $_\n";
} elsif (/xml_space/) {
  # Python probably
  warn "$.: xml_space - this part of a literal has been removed\n  $_\n";
}

