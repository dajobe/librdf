/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_init.c - Overall library initialisation / termination
 *
 * $Id$
 *
 * Copyright (C) 2000-2001 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.org/
 * University of Bristol - http://www.bristol.ac.uk/
 * 
 * This package is Free Software or Open Source available under the
 * following licenses (these are alternatives):
 *   1. GNU Lesser General Public License (LGPL)
 *   2. GNU General Public License (GPL)
 *   3. Mozilla Public License (MPL)
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * full license terms.
 * 
 * 
 */


#include <rdf_config.h>

#include <stdio.h>

#include <librdf.h>
#include <rdf_init.h>
#include <rdf_hash.h>
#include <rdf_uri.h>
#include <rdf_node.h>
#include <rdf_digest.h>
#include <rdf_stream.h>
#include <rdf_statement.h>
#include <rdf_storage.h>
#include <rdf_model.h>


const char * const redland_copyright_string = "Copyright (C) 2000 David Beckett - http://purl.org/net/dajobe/ - Institute for Learning and Research Technology, University of Bristol.";

const char * const redland_version_string = VERSION;

const int redland_version_major = LIBRDF_VERSION_MAJOR;
const int redland_version_minor = LIBRDF_VERSION_MINOR;
const int redland_version_release = LIBRDF_VERSION_RELEASE;


/**
 * librdf_init_world - Initialise the library
 * @digest_factory_name: &librdf_digest_factory
 * @uris_hash: &librdf_hash for URIs
 * 
 * Initialises various classes and uses the digest factory
 * for various modules that need to make digests of their objects.
 * See librdf_init_node() for details.
 *
 * If a uris_hash is given, that is passed to the URIs class
 * initialisation and used to store hashes rather than the default
 * one, currently an in memory hash.  See librdf_init_uri() for details.
 *
 **/
void
librdf_init_world(char *digest_factory_name, librdf_hash* uris_hash)
{
  librdf_digest_factory* digest_factory;

  /* Digests first, lots of things use these */
  librdf_init_digest();
  digest_factory=librdf_get_digest_factory(digest_factory_name);
 
  /* Hash next, needed for URIs */
  librdf_init_hash();

  /* URIs */
  librdf_init_uri(digest_factory, uris_hash);
  librdf_init_node(digest_factory);

  librdf_init_concepts();

  librdf_init_statement();
  librdf_init_model();
  librdf_init_storage();
  librdf_init_parser();
}


/**
 * librdf_destroy_world - Terminate the library
 * 
 * Terminates and frees the resources.
 **/
void
librdf_destroy_world(void)
{
  librdf_finish_parser();
  librdf_finish_storage();
  librdf_finish_model();
  librdf_finish_statement();

  librdf_finish_concepts();

  librdf_finish_node();
  librdf_finish_uri();

  librdf_finish_hash();

  librdf_finish_digest();
}
