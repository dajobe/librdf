/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_init.c - Overall librdf initialisation / termination
 *
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


#include <rdf_config.h>

#include <stdio.h>

#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_hash.h>
#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_digest.h>
#include <rdf_stream.h>
#include <rdf_statement.h>
#include <rdf_storage.h>
#include <rdf_model.h>


void
librdf_init_world(char *digest_factory_name) 
{
  librdf_digest_factory* digest_factory;
  
  librdf_init_digest();

  digest_factory=librdf_get_digest_factory(digest_factory_name);
 
  librdf_init_uri(digest_factory);
  librdf_init_node(digest_factory);

  librdf_init_hash();
  librdf_init_statement();
  librdf_init_model();
  librdf_init_storage();
  librdf_init_parser();
}


void
librdf_destroy_world(void)
{
  librdf_finish_digest();
  librdf_finish_hash();
  librdf_finish_uri();
  /* librdf_finish_node(); */
  librdf_finish_statement();
  librdf_finish_model();
  librdf_finish_storage();
  librdf_finish_parser();
}
