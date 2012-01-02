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
#include <stdlib.h>
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
void
librdf_world_set_raptor(librdf_world* world, raptor_world* raptor_world_ptr)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(world, librdf_world);
  world->raptor_world_ptr = raptor_world_ptr;
  world->raptor_world_allocated_here = 0;
}


/**
 * librdf_world_get_raptor:
 * @world: librdf_world object
 * 
 * Get the #raptor_world instance used by this #librdf_world.
 *
 * Return value: raptor_world object or NULL on failure (e.g. not initialized)
 **/
raptor_world*
librdf_world_get_raptor(librdf_world* world)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  return world->raptor_world_ptr;
}


static void
librdf_raptor_log_handler(void *data, raptor_log_message *message)
{
  librdf_world *world = (librdf_world *)data;
  librdf_log_level level;

  /* Map raptor2 fatal/error/warning log levels to librdf log levels,
     ignore others */
  switch(message->level) {
    case RAPTOR_LOG_LEVEL_FATAL:
      level = LIBRDF_LOG_FATAL;
      break;

    case RAPTOR_LOG_LEVEL_ERROR:
      level = LIBRDF_LOG_ERROR;
      break;

    case RAPTOR_LOG_LEVEL_WARN:
      level = LIBRDF_LOG_WARN;
      break;

    case RAPTOR_LOG_LEVEL_INFO:
    case RAPTOR_LOG_LEVEL_DEBUG:
    case RAPTOR_LOG_LEVEL_TRACE:
    case RAPTOR_LOG_LEVEL_NONE:
    default:
      return;
  }

  librdf_log_simple(world, 0, level, LIBRDF_FROM_RAPTOR, message->locator,
                    message->text);
}


int
librdf_raptor_free_bnode_hash(librdf_world* world)
{
  if(world->bnode_hash) {
    librdf_free_hash(world->bnode_hash);
    world->bnode_hash = NULL;
  }

  return 0;
}


int
librdf_raptor_reset_bnode_hash(librdf_world* world)
{
  librdf_raptor_free_bnode_hash(world);

  world->bnode_hash = librdf_new_hash(world, NULL);
  if(!world->bnode_hash)
    return 1;

  return 0;
}

static unsigned char*
librdf_raptor_generate_id_handler(void *user_data,
                                  unsigned char *user_bnodeid)
{
  librdf_world* world = (librdf_world*)user_data;

  if(user_bnodeid && world->bnode_hash) {
    unsigned char *mapped_id;

    mapped_id = (unsigned char*)librdf_hash_get(world->bnode_hash,
                                                (const char*)user_bnodeid);
    if(!mapped_id) {
      mapped_id = librdf_world_get_genid(world);

      if(mapped_id &&
         librdf_hash_put_strings(world->bnode_hash,
                                 (char*)user_bnodeid, (char*)mapped_id)) {
        /* error -> free mapped_id and return NULL */
        LIBRDF_FREE(char*, mapped_id);
        mapped_id = NULL;
      }
    }
    /* always free passed in bnodeid */
    raptor_free_memory(user_bnodeid);

    return mapped_id;
  } else
    return librdf_world_get_genid(world);
}


/**
 * librdf_init_raptor:
 * @world: librdf_world object
 * 
 * INTERNAL - Initialize the raptor library.
 * 
 * Initializes the raptor library unless a raptor instance is provided
 * externally with librdf_world_set_raptor() (and using raptor v2 APIs).
 * Sets raptor uri handlers to work with #librdf_uri objects.
 *
 * Return value: non-0 on failure
 **/
int
librdf_init_raptor(librdf_world* world)
{
  if(!world->raptor_world_ptr) {
    world->raptor_world_ptr = raptor_new_world();
    world->raptor_world_allocated_here = 1;

    if(world->raptor_world_ptr && world->raptor_init_handler) {
      world->raptor_init_handler(world->raptor_init_handler_user_data,
                                 world->raptor_world_ptr);
    }

    if(!world->raptor_world_ptr || raptor_world_open(world->raptor_world_ptr)) {
      LIBRDF_FATAL1(world, LIBRDF_FROM_PARSER, "failed to initialize raptor");
      return 1;
    }
  }

  /* New in-memory hash for mapping bnode IDs */
  world->bnode_hash = librdf_new_hash(world, NULL);
  if(!world->bnode_hash)
    return 1;


  /* set up log handler */
  raptor_world_set_log_handler(world->raptor_world_ptr, world,
                               librdf_raptor_log_handler);

  /* set up blank node identifier generation handler */
  raptor_world_set_generate_bnodeid_handler(world->raptor_world_ptr,
                                            world,
                                            librdf_raptor_generate_id_handler);

  return 0;
}


/**
 * librdf_world_set_raptor_init_handler:
 * @world: librdf_world object
 * @user_data: user data
 * @handler: handler to call
 *
 * Set the raptor initialization handler to be called if a new raptor_world is constructed.
 */
void
librdf_world_set_raptor_init_handler(librdf_world* world, 
                                     void* user_data,
                                     librdf_raptor_init_handler handler)
{
  world->raptor_init_handler = handler;
  world->raptor_init_handler_user_data = user_data;
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
  if(world->raptor_world_ptr && world->raptor_world_allocated_here) {
    raptor_free_world(world->raptor_world_ptr);
    world->raptor_world_ptr = NULL;
  }

  if(world->bnode_hash)
    librdf_free_hash(world->bnode_hash);
}
