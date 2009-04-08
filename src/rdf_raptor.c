/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_raptor.c - librdf Raptor integration
 *
 * Copyright (C) 2008, David Beckett http://www.dajobe.org/
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 * 
 */
 
 
#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <redland.h>


/**
 * librdf_world_set_raptor:
 * @world: librdf_world object
 * @raptor_world_ptr: raptor_world object
 * 
 * Set the #raptor_world instance to be used with this #librdf_world.
 *
 * If no raptor_world instance is set with this function,
 * librdf_world_open() creates a new instance.
 *
 * Ownership of the raptor_world is not taken. If the raptor library
 * instance is set with this function, librdf_free_world() will not
 * free it.
 *
 **/
#ifdef RAPTOR_V2_AVAILABLE
void
librdf_world_set_raptor(librdf_world* world, raptor_world* raptor_world_ptr)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(world, librdf_world);
  world->raptor_world_ptr = raptor_world_ptr;
}
#endif


/**
 * librdf_world_get_raptor:
 * @world: librdf_world object
 * 
 * Get the #raptor_world instance used by this #librdf_world.
 *
 * Return value: raptor_world object or NULL on failure (e.g. not initialized)
 **/
#ifdef RAPTOR_V2_AVAILABLE
raptor_world*
librdf_world_get_raptor(librdf_world* world)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  return world->raptor_world_ptr;
}
#endif


static raptor_uri*
librdf_raptor_new_uri(void *context, const unsigned char *uri_string) 
{
  return (raptor_uri*)librdf_new_uri((librdf_world*)context, uri_string);
}

static raptor_uri*
librdf_raptor_new_uri_from_uri_local_name(void *context,
                                          raptor_uri *uri,
                                          const unsigned char *local_name)
{
   return (raptor_uri*)librdf_new_uri_from_uri_local_name((librdf_uri*)uri, local_name);
}

static raptor_uri*
librdf_raptor_new_uri_relative_to_base(void *context,
                                       raptor_uri *base_uri,
                                       const unsigned char *uri_string)
{
  return (raptor_uri*)librdf_new_uri_relative_to_base((librdf_uri*)base_uri, uri_string);
}


static raptor_uri*
librdf_raptor_new_uri_for_rdf_concept(void *context, const char *name) 
{
  librdf_uri *uri=NULL;
  librdf_get_concept_by_name((librdf_world*)context, 1, name, &uri, NULL);
  if(!uri)
    return NULL;
  return (raptor_uri*)librdf_new_uri_from_uri(uri);
}

static void
librdf_raptor_free_uri(void *context, raptor_uri *uri) 
{
  librdf_free_uri((librdf_uri*)uri);
}


static int
librdf_raptor_uri_equals(void *context, raptor_uri* uri1, raptor_uri* uri2)
{
  return librdf_uri_equals((librdf_uri*)uri1, (librdf_uri*)uri2);
}


static int
librdf_raptor_uri_compare(void *context, raptor_uri* uri1, raptor_uri* uri2)
{
  return librdf_uri_compare((librdf_uri*)uri1, (librdf_uri*)uri2);
}


static raptor_uri*
librdf_raptor_uri_copy(void *context, raptor_uri *uri)
{
  return (raptor_uri*)librdf_new_uri_from_uri((librdf_uri*)uri);
}


static unsigned char*
librdf_raptor_uri_as_string(void *context, raptor_uri *uri)
{
  return librdf_uri_as_string((librdf_uri*)uri);
}


static unsigned char*
librdf_raptor_uri_as_counted_string(void *context, raptor_uri *uri, size_t *len_p)
{
  return librdf_uri_as_counted_string((librdf_uri*)uri, len_p);
}


static const raptor_uri_handler librdf_raptor_uri_handler = {
  librdf_raptor_new_uri,
  librdf_raptor_new_uri_from_uri_local_name,
  librdf_raptor_new_uri_relative_to_base,
  librdf_raptor_new_uri_for_rdf_concept,
  librdf_raptor_free_uri,
  librdf_raptor_uri_equals,
  librdf_raptor_uri_copy,
  librdf_raptor_uri_as_string,
  librdf_raptor_uri_as_counted_string,
  2,
  librdf_raptor_uri_compare
};


/**
 * librdf_init_raptor:
 * @world: librdf_world object
 * 
 * INTERNAL - Initialize the raptor library.
 * 
 * Initializes the raptor library unless a raptor instance is provided
 * externally with librdf_world_set_raptor() (and using raptor v2 APIs).
 * Sets raptor uri handlers to work with #librdf_uri objects.
 **/
void
librdf_init_raptor(librdf_world* world)
{
#ifdef RAPTOR_V2_AVAILABLE
  if(!world->raptor_world_ptr) {
    world->raptor_world_ptr = raptor_new_world();
    world->raptor_world_allocated_here = 1;
    if(!world->raptor_world_ptr || raptor_world_open(world->raptor_world_ptr))
      abort();
    raptor_uri_set_handler_v2(world->raptor_world_ptr, &librdf_raptor_uri_handler, world);
  }
#else
  raptor_init();
  raptor_uri_set_handler(&librdf_raptor_uri_handler, world);
#endif
}


/**
 * librdf_finish_raptor:
 * @world: librdf_world object
 *
 * INTERNAL - Terminate the raptor library.
 *
 * Frees the raptor library resources unless a raptor instance was provided
 * externally via librdf_world_set_raptor() (and using raptor v2 APIs) prior
 * to librdf_init_raptor().
 **/
void
librdf_finish_raptor(librdf_world* world)
{
#ifdef RAPTOR_V2_AVAILABLE
  if(world->raptor_world_ptr && world->raptor_world_allocated_here) {
    raptor_free_world(world->raptor_world_ptr);
    world->raptor_world_ptr = NULL;
  }
#else
  raptor_finish();
#endif
}
