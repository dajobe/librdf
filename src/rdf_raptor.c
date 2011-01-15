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
 * librdf_world_open() creates a new instance.  This function
 * is only useful when librdf is built with the Raptor V2 APIs, which
 * use this world object.  It has no effect with a Raptor V1 build.
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
 * This value is only used with the Raptor V2 APIs.
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
  if(!world->raptor_world_ptr) {
    world->raptor_world_ptr = raptor_new_world();
    world->raptor_world_allocated_here = 1;

    if(!world->raptor_world_ptr || raptor_world_open(world->raptor_world_ptr)) {
      LIBRDF_FATAL1(world, LIBRDF_FROM_PARSER, "failed to initialize raptor");
      return;
    }
  }

  /* set up log handler */
  raptor_world_set_log_handler(world->raptor_world_ptr, world,
                               librdf_raptor_log_handler);
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
}
