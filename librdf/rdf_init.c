/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_init.c - Overall library initialisation / termination
 *
 * $Id$
 *
 * Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/
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
#ifdef WITH_THREADS
#include <pthread.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <librdf.h>


const char * const redland_copyright_string = "Copyright (C) 2000-2003 David Beckett - http://purl.org/net/dajobe/ - Institute for Learning and Research Technology, University of Bristol.";

const char * const redland_version_string = VERSION;

const int redland_version_major = LIBRDF_VERSION_MAJOR;
const int redland_version_minor = LIBRDF_VERSION_MINOR;
const int redland_version_release = LIBRDF_VERSION_RELEASE;




/**
 * librdf_new_world - Creates a new Redland execution environment
 */
librdf_world*
librdf_new_world(void) {
  librdf_world *world=(librdf_world*)LIBRDF_CALLOC(librdf_world, sizeof(librdf_world), 1);
  
  return world;
  
}


/**
 * librdf_free_world - Terminate the library
 * @world: redland world object
 * 
 * Terminates and frees the resources.
 **/
void
librdf_free_world(librdf_world *world)
{
  librdf_finish_serializer(world);
  librdf_finish_parser(world);
  librdf_finish_storage(world);
  librdf_finish_query(world);
  librdf_finish_model(world);
  librdf_finish_statement(world);

  librdf_finish_concepts(world);

  librdf_finish_node(world);
  librdf_finish_uri(world);

  librdf_finish_hash(world);

  librdf_finish_digest(world);

#ifdef WITH_THREADS
   if (world->mutex)
   {
     pthread_mutex_destroy(world->mutex);
     SYSTEM_FREE(world->mutex);
     world->mutex = NULL;
   }
#endif

  LIBRDF_FREE(librdf_world, world);
}


/**
 * Internal
 */
void
librdf_world_init_mutex(librdf_world* world)
{
#ifdef WITH_THREADS
  world->mutex = (pthread_mutex_t *) SYSTEM_MALLOC(sizeof(pthread_mutex_t));
  pthread_mutex_init(world->mutex, NULL);
#else
#endif
}


/**
 * librdf_world_open - Open an environment
 * @world: redland world object
 * 
 **/
void
librdf_world_open(librdf_world *world)
{
  librdf_world_init_mutex(world);
  
  /* Digests first, lots of things use these */
  librdf_init_digest(world);

  /* Hash next, needed for URIs */
  librdf_init_hash(world);

  librdf_init_uri(world);

  librdf_init_concepts(world);

  librdf_init_statement(world);
  librdf_init_model(world);
  librdf_init_query(world);
  librdf_init_storage(world);
  librdf_init_parser(world);
  librdf_init_serializer(world);
}


/*
 * librdf_error - Error - Internal
 * @world: redland world object or NULL
 * @message: message arguments
 *
 * If world is NULL, the error ocurred in redland startup before
 * the world was created.
 **/
void
librdf_error(librdf_world* world, const char *message, ...)
{
  va_list arguments;

  va_start(arguments, message);

  if(world && world->error_fn) {
    world->error_fn(world->error_user_data, message, arguments);
    return;
  }
  
  fputs("librdf error - ", stderr);
  vfprintf(stderr, message, arguments);
  fputc('\n', stderr);

  va_end(arguments);
}


/*
 * librdf_warning - Warning - Internal
 * @world: redland world object or NULL
 * @message: message arguments
 *
 * If world is NULL, the warning happened in redland startup
 * before the world was created
 **/
void
librdf_warning(librdf_world* world, const char *message, ...)
{
  va_list arguments;

  va_start(arguments, message);

  if(world && world->warning_fn) {
    world->warning_fn(world->warning_user_data, message, arguments);
    return;
  }
  
  fputs("librdf warning - ", stderr);
  vfprintf(stderr, message, arguments);
  fputc('\n', stderr);

  va_end(arguments);
}


/**
 * librdf_world_set_error - Set the world error handling function
 * @world: redland world object
 * @user_data: user data to pass to function
 * @error_fn: pointer to the function
 * 
 * The function will receive callbacks when the world fails.
 * 
 **/
void
librdf_world_set_error(librdf_world* world, void *user_data,
                       void (*error_fn)(void *user_data, const char *message, va_list arguments))
{
  world->error_user_data=user_data;
  world->error_fn=error_fn;
}


/**
 * librdf_world_set_warning - Set the world warning handling function
 * @world: redland world object
 * @user_data: user data to pass to function
 * @warning_fn: pointer to the function
 * 
 * The function will receive callbacks when the world gives a warning.
 * 
 **/
void
librdf_world_set_warning(librdf_world* world, void *user_data,
                         void (*warning_fn)(void *user_data, const char *message, va_list arguments))
{
  world->warning_user_data=user_data;
  world->warning_fn=warning_fn;
}



/**
 * librdf_world_set_digest - Set the default digest name
 * @world: redland world object
 * @name: Digest factory name
 *
 * Sets the digest factory for various modules that need to make
 * digests of their objects.
 * 
 */
void
librdf_world_set_digest(librdf_world* world, const char *name) {
  world->digest_factory_name=(char*)name;
}


/**
 * librdf_world_set_uris_hash - Set the hash object to use for URI class
 * @world: redland world object
 * @uris_hash: librdf_hash* hash
 *
 * If a uris_hash is given, that is passed to the URIs class
 * initialisation and used to store hashes rather than the default
 * one, currently an in memory hash.  See librdf_init_uri() for details.
 */
void 
librdf_world_set_uris_hash(librdf_world* world, librdf_hash* uris_hash)
{
  world->uris_hash=uris_hash;
}


const char *
librdf_world_get_feature(librdf_world* world, librdf_uri *feature) 
{
  return NULL; /* none retrievable */
}

int
librdf_world_set_feature(librdf_world* world, librdf_uri *feature,
                         const char *value) 
{
  return 1; /* none available */
}



/* OLD INTERFACES BELOW HERE */

/* For old interfaces below ONLY */
static librdf_world* RDF_World;

/**
 * librdf_init_world - Initialise the library (DEPRECATED)
 * @digest_factory_name: Name of digest factory to use
 * @uris_hash: Hash to store URIs in
 *
 * Use librdf_new_world and librdf_world_open on librdf_world object
 * 
 * See librdf_world_set_digest_factory_name and
 * librdf_world_set_uris_hash for documentation on arguments.
 **/
void
librdf_init_world(char *digest_factory_name, librdf_hash* uris_hash)
{
  RDF_World=librdf_new_world();
  if(!RDF_World)
    return;
  if(digest_factory_name)
    librdf_world_set_digest(RDF_World, digest_factory_name);
  librdf_world_set_uris_hash(RDF_World, uris_hash);
  librdf_world_open(RDF_World);
}


/**
 * librdf_destroy_world - Terminate the library (DEPRECATED)
 *
 * Use librdf_free_world on librdf_world object
 * 
 * Terminates and frees the resources.
 **/
void
librdf_destroy_world(void)
{
  librdf_free_world(RDF_World);
}

#if defined (LIBRDF_DEBUG) && defined(HAVE_DMALLOC_H) && defined(LIBRDF_MEMORY_DEBUG_DMALLOC)

#undef malloc
void*
librdf_system_malloc(size_t size)
{
  return malloc(size);
}

#undef free
void
librdf_system_free(void *ptr)
{
  return free(ptr);
  
}

#endif

