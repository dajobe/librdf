/*
 * RDF Node implementation
 *   RDF:Resource
 *   RDF:Property
 *   (object) - RDF:Literal / RDF:Resource
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <config.h>

#include <stdio.h>

#include <rdf_config.h>
#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_digest.h>
#include <rdf_statement.h>


void rdf_init_world(char *digest_factory_name) 
{
  rdf_digest_factory* digest_factory;

  digest_factory=get_rdf_digest_factory(digest_factory_name);
  
  init_rdf_uri(digest_factory);
  init_rdf_node(digest_factory);
  init_rdf_statement();
}
