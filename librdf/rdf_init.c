/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_init.c - Redland library initialisation / termination
 *
 * $Id$
 *
 * Copyright (C) 2000-2006, David Beckett http://purl.org/net/dajobe/
 * Copyright (C) 2000-2005, University of Bristol, UK http://www.bristol.ac.uk/
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

#include <stdio.h>
#ifdef WITH_THREADS
#include <pthread.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

/* for gettimeofday */
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#include <redland.h>


const char * const librdf_short_copyright_string = "Copyright (C) 2000-2006 David Beckett. (C) 2000-2005 University of Bristol";

const char * const librdf_copyright_string = "Copyright (C) 2000-2006 David Beckett - http://purl.org/net/dajobe/\nCopyright (C) 2000-2005 University of Bristol - http://www.bristol.ac.uk/";

const char * const librdf_version_string = VERSION;

const unsigned int librdf_version_major = LIBRDF_VERSION_MAJOR;
const unsigned int librdf_version_minor = LIBRDF_VERSION_MINOR;
const unsigned int librdf_version_release = LIBRDF_VERSION_RELEASE;

const unsigned int librdf_version_decimal = LIBRDF_VERSION_DECIMAL;




/**
 * librdf_new_world:
 *
 * Create a new redland execution environment.
 *
 * Returns: a new #librdf_world or NULL on failure
 */
librdf_world*
librdf_new_world(void) {
  librdf_world *world=(librdf_world*)LIBRDF_CALLOC(librdf_world, sizeof(librdf_world), 1);
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;
  struct timezone tz;
#endif

#ifdef HAVE_GETTIMEOFDAY
  if(!gettimeofday(&tv, &tz)) {
    world->genid_base=tv.tv_sec;
  } else
    world->genid_base=1;
#else
  world->genid_base=1;
#endif
  world->genid_counter=1;
  
  return world;
  
}


/**
 * librdf_free_world
 * @world: redland world object
 *
 * Terminate the library and frees all allocated resources.
 **/
void
librdf_free_world(librdf_world *world)
{
  /* NOTE: raptor is always initialised as a parser and may
   * be also used as a serializer, but it is NOT finished
   * in the serializer_raptor registration.  Therefore, always
   * keep the parser class finishing after the serializer.
   */
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
     pthread_mutex_destroy(world->statements_mutex);
     SYSTEM_FREE(world->statements_mutex);
     world->statements_mutex = NULL;

     pthread_mutex_destroy(world->nodes_mutex);
     SYSTEM_FREE(world->nodes_mutex);
     world->nodes_mutex = NULL;

     pthread_mutex_destroy(world->mutex);
     SYSTEM_FREE(world->mutex);
     world->mutex = NULL;
   }
#endif

  LIBRDF_FREE(librdf_world, world);
}


/**
 * librdf_world_init_mutex:
 * @world: redland world object
 *
 * INTERNAL - Create the world mutex.
 */
void
librdf_world_init_mutex(librdf_world* world)
{
#ifdef WITH_THREADS
  world->mutex = (pthread_mutex_t *) SYSTEM_MALLOC(sizeof(pthread_mutex_t));
  pthread_mutex_init(world->mutex, NULL);

  world->nodes_mutex = (pthread_mutex_t *) SYSTEM_MALLOC(sizeof(pthread_mutex_t));
  pthread_mutex_init(world->nodes_mutex, NULL);

  world->statements_mutex = (pthread_mutex_t *) SYSTEM_MALLOC(sizeof(pthread_mutex_t));
  pthread_mutex_init(world->statements_mutex, NULL);
#else
#endif
}


/**
 * librdf_world_open
 * @world: redland world object
 *
 * Open a created redland world environment.
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
  librdf_init_node(world);

  librdf_init_concepts(world);

  librdf_init_statement(world);
  librdf_init_model(world);
  librdf_init_storage(world);

  /* NOTE: raptor is always initialised as a parser and may
   * be also used as a serializer, but it is NOT initialised
   * in the serializer_raptor registration.  Therefore, always
   * keep the parser class initialising before the serializer.
   */
  librdf_init_parser(world);
  librdf_init_serializer(world);

  /* NOTE: Since initialising rasqal calls raptor to create URIs,
   * rasqal must be initialised after raptor so that the raptor URI
   * implementation is changed with raptor_uri_set_handler() in
   * librdf_parser_raptor_constructor() before rasqal tries to create
   * URIs.
   *
   * Otherwise URIs would be allocated with raptor as char* strings
   * and then attempted to be freed later as librdf_uri*.  Which
   * would be bad.
   */
  librdf_init_query(world);
}


/**
 * librdf_world_set_error:
 * @world: redland world object
 * @user_data: user data to pass to function
 * @error_handler: pointer to the function
 *
 * Set the world error handling function.
 * 
 * The function will receive callbacks when the world fails.
 * librdf_world_set_logger() provides richer access to all log messages
 * and should be used in preference.
 **/
void
librdf_world_set_error(librdf_world* world, void *user_data,
                       librdf_log_level_func error_handler)
{
  world->error_user_data=user_data;
  world->error_handler=error_handler;
}


/**
 * librdf_world_set_warning:
 * @world: redland world object
 * @user_data: user data to pass to function
 * @warning_handler: pointer to the function
 *
 * Set the world warning handling function.
 * 
 * The function will receive callbacks when the world gives a warning.
 * librdf_world_set_logger() provides richer access to all log messages
 * and should be used in preference.
 **/
void
librdf_world_set_warning(librdf_world* world, void *user_data,
                         librdf_log_level_func warning_handler)
{
  world->warning_user_data=user_data;
  world->warning_handler=warning_handler;
}



/**
 * librdf_world_set_logger:
 * @world: redland world object
 * @user_data: user data to pass to function
 * @log_handler: pointer to the function
 *
 * Set the world log handling function.
 * 
 * The function will receive callbacks when redland generates a log message
 **/
void
librdf_world_set_logger(librdf_world* world, void *user_data,
                        librdf_log_func log_handler)
{
  world->log_user_data=user_data;
  world->log_handler=log_handler;
}



/**
 * librdf_world_set_digest:
 * @world: redland world object
 * @name: Digest factory name
 *
 * Set the default content digest name.
 *
 * Sets the digest factory for various modules that need to make
 * digests of their objects.
 */
void
librdf_world_set_digest(librdf_world* world, const char *name) {
  world->digest_factory_name=(char*)name;
}


/**
 * librdf_world_get_feature - get the value of a world feature 
 * @world: #librdf_world object
 * @feature: #librdf_uri feature property
 * 
 * Return value: new #librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
librdf_node*
librdf_world_get_feature(librdf_world* world, librdf_uri *feature) 
{
  return NULL; /* none retrievable */
}


/**
 * librdf_world_set_feature:
 * @world: #librdf_world object
 * @feature: #librdf_uri feature property
 * @value: #librdf_node feature property value
 *
 * Set the value of a world feature.
 * 
 * Return value: non 0 on failure (negative if no such feature)
 **/
int
librdf_world_set_feature(librdf_world* world, librdf_uri* feature,
                         librdf_node* value) 
{
  librdf_uri* genid_base=librdf_new_uri(world, (const unsigned char*)LIBRDF_WORLD_FEATURE_GENID_BASE);
  librdf_uri* genid_counter=librdf_new_uri(world, (const unsigned char*)LIBRDF_WORLD_FEATURE_GENID_COUNTER);
  int rc= -1;

  if(librdf_uri_equals(feature, genid_base)) {
    if(!librdf_node_is_resource(value))
      rc=1;
    else {
      int i=atoi((const char*)librdf_node_get_literal_value(value));
      if(i<1)
        i=1;
#ifdef WITH_THREADS
      pthread_mutex_lock(world->mutex);
#endif
      world->genid_base=1;
#ifdef WITH_THREADS
      pthread_mutex_unlock(world->mutex);
#endif
      rc=0;
    }
  } else if(librdf_uri_equals(feature, genid_counter)) {
    if(!librdf_node_is_resource(value))
      rc=1;
    else {
      int i=atoi((const char*)librdf_node_get_literal_value(value));
      if(i<1)
        i=1;
#ifdef WITH_THREADS
      pthread_mutex_lock(world->mutex);
#endif
      world->genid_counter=1;
#ifdef WITH_THREADS
      pthread_mutex_unlock(world->mutex);
#endif
      rc=0;
    }
  }

  librdf_free_uri(genid_base);
  librdf_free_uri(genid_counter);

  return rc;
}


/* Internal */
unsigned char*
librdf_world_get_genid(librdf_world* world)
{
  int id, tmpid, counter, tmpcounter;
  int length;
  unsigned char *buffer;

  /* This is read-only and thread safe */
  tmpid= (id= world->genid_base);

#ifdef WITH_THREADS
  pthread_mutex_lock(world->mutex);
#endif
  tmpcounter=(counter=world->genid_counter++);
#ifdef WITH_THREADS
  pthread_mutex_unlock(world->mutex);
#endif


  length=5;  /* min length 1 + "r" + min length 1 + "r" \0 */
  while(tmpid/=10)
    length++;
  while(tmpcounter/=10)
    length++;
  
  buffer=(unsigned char*)LIBRDF_MALLOC(cstring, length);
  if(!buffer)
    return NULL;

  sprintf((char*)buffer, "r%dr%d", id, counter);
  return buffer;
}



/* OLD INTERFACES BELOW HERE */

/* For old interfaces below ONLY */
static librdf_world* RDF_World;

/**
 * librdf_init_world:
 * @digest_factory_name: Name of digest factory to use
 * @not_used2: Not used
 *
 * Initialise the library
 * @deprecated: Do not use.
 *
 * Use librdf_new_world() and librdf_world_open() on #librdf_world object
 * 
 * See librdf_world_set_digest_factory_name() and
 * librdf_world_set_uris_hash() for documentation on arguments.
 **/
void
librdf_init_world(char *digest_factory_name, void* not_used2)
{
  RDF_World=librdf_new_world();
  if(!RDF_World)
    return;
  if(digest_factory_name)
    librdf_world_set_digest(RDF_World, digest_factory_name);
  librdf_world_open(RDF_World);
}


/**
 * librdf_destroy_world:
 *
 * Terminate the library
 * @deprecated: Do not use.
 *
 * Use librdf_free_world() on #librdf_world object
 * 
 * Terminates and frees the resources.
 **/
void
librdf_destroy_world(void)
{
  librdf_free_world(RDF_World);
}


#if defined (LIBRDF_DEBUG) && defined(LIBRDF_MEMORY_SIGN)
void*
librdf_sign_malloc(size_t size)
{
  int *p;
  
  size += sizeof(int);
  
  p=(int*)malloc(size);
  *p++ = LIBRDF_SIGN_KEY;
  return p;
}

void*
librdf_sign_calloc(size_t nmemb, size_t size)
{
  int *p;
  
  /* turn into bytes */
  size = nmemb*size + sizeof(int);
  
  p=(int*)calloc(1, size);
  *p++ = LIBRDF_SIGN_KEY;
  return p;
}

void*
librdf_sign_realloc(void *ptr, size_t size)
{
  int *p;

  if(!ptr)
    return librdf_sign_malloc(size);
  
  p=(int*)ptr;
  p--;

  if(*p != LIBRDF_SIGN_KEY)
    LIBRDF_FATAL1(NULL, LIBRDF_FROM_MEMORY, "memory signature wrong\n");

  size += sizeof(int);
  
  p=(int*)realloc(p, size);
  *p++= LIBRDF_SIGN_KEY;
  return p;
}

void
librdf_sign_free(void *ptr)
{
  int *p;

  if(!ptr)
    return;
  
  p=(int*)ptr;
  p--;

  if(*p != LIBRDF_SIGN_KEY)
    LIBRDF_FATAL1(NULL, LIBRDF_FROM_MEMORY, "memory signature wrong");

  free(p);
}
#endif




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

