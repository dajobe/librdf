/*
 * rdf_init.c - RDF Node implementation
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
 *                                       
 * This program is free software distributed under either of these licenses:
 *   1. The GNU Lesser General Public License (LGPL)
 * OR ALTERNATIVELY
 *   2. The modified BSD license
 *
 * See LICENSE.html or LICENSE.txt for the full license terms.
 */


#include <config.h>

#include <stdio.h>

#define LIBRDF_INTERNAL 1
#include <rdf_config.h>
#include <rdf_hash.h>
#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_digest.h>
#include <rdf_statement.h>
#include <rdf_model.h>


/* prototype that should really be elsewhere */
void librdf_init_world(char *digest_factory_name);


void librdf_init_world(char *digest_factory_name) 
{
  librdf_digest_factory* digest_factory;

  digest_factory=librdf_get_digest_factory(digest_factory_name);

  librdf_init_digest();
  librdf_init_hash();
  librdf_init_uri(digest_factory);
  librdf_init_node(digest_factory);
  librdf_init_statement();
  librdf_init_model();
}
