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
#include <rdf_hash.h>
#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_digest.h>
#include <rdf_statement.h>
#include <rdf_model.h>


/* prototype that should really be elsewhere */
void rdf_init_world(char *digest_factory_name);


void rdf_init_world(char *digest_factory_name) 
{
  rdf_digest_factory* digest_factory;

  digest_factory=rdf_get_digest_factory(digest_factory_name);

  rdf_init_digest();
  rdf_init_hash();
  rdf_init_uri(digest_factory);
  rdf_init_node(digest_factory);
  rdf_init_statement();
  rdf_init_model();
}
