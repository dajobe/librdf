/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_init.h - Overall library initialisation / termination prototypes
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


#ifndef LIBRDF_INIT_H
#define LIBRDF_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

struct librdf_world_s
{
  void *error_user_data;
  void *warning_user_data;
  void (*error_fn)(void *user_data, const char *msg, ...);
  void (*warning_fn)(void *user_data, const char *msg, ...);

  char *digest_factory_name;
  librdf_digest_factory* digest_factory;

  librdf_hash* uris_hash;
  int uris_hash_allocated_here;

  /* List of parser factories */
  librdf_parser_factory* parsers;

  /* List of digest factories */
  librdf_digest_factory *digests;

  /* list of hash factories */
  librdf_hash_factory* hashes;

  /* list of free librdf_hash_datums is kept */
  librdf_hash_datum* hash_datums_list;

   /* hash load_factor out of 1000 */
  int hash_load_factor;
};

librdf_world* librdf_new_world(void);
void librdf_free_world(librdf_world *world);
void librdf_world_open(librdf_world *world);
  
void librdf_world_set_error(librdf_world* world, void *user_data, void (*error_fn)(void *user_data, const char *msg, ...));
void librdf_world_set_warning(librdf_world* world, void *user_data, void (*warning_fn)(void *user_data, const char *msg, ...));

void librdf_world_set_digest(librdf_world*, const char *name);
void librdf_world_set_uris_hash(librdf_world* world, librdf_hash* uris_hash);

const char *librdf_world_get_feature(librdf_world* world, librdf_uri *feature);
int librdf_world_set_feature(librdf_world* world, librdf_uri *feature, const char *value);

#ifdef LIBRDF_INTERNAL
/* internal routines used to invoking errors/warnings upwards to user */
void librdf_error(librdf_world* world, const char *message, ...);
void librdf_warning(librdf_world* world, const char *message, ...);
#endif

/* OLD INTERFACES */
void librdf_init_world(char *digest_factory_name, librdf_hash* uris_hash);
void librdf_destroy_world(void);

#ifdef __cplusplus
}
#endif

#endif
