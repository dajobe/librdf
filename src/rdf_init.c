/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_init.c - Redland library initialisation / termination and memory
 *              management
 *
 * Copyright (C) 2000-2012, David Beckett http://www.dajobe.org/
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
#include <string.h>
#ifdef WITH_THREADS
#include <pthread.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* for getpid() */
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
/* for getpid */
#include <sys/types.h>
#ifdef HAVE_UNISTD
#include <unistd.h>
#endif

#ifdef MODULAR_LIBRDF
#include <ltdl.h>
#endif

#ifndef STANDALONE

const char * const librdf_short_copyright_string = "Copyright 2000-2012 David Beckett. Copyright 2000-2005 University of Bristol";

const char * const librdf_copyright_string = "Copyright (C) 2000-2012 David Beckett - http://www.dajobe.org/\nCopyright (C) 2000-2005 University of Bristol - http://www.bristol.ac.uk/";

const char * const librdf_license_string = "LGPL 2.1 or newer, GPL 2 or newer, Apache 2.0 or newer.\nSee http://librdf.org/LICENSE.html for full terms.";

const char * const librdf_home_url_string = "http://librdf.org/";


/**
 * librdf_version_string:
 *
 * Library full version as a string.
 *
 * See also #librdf_version_decimal.
 */
const char * const librdf_version_string = VERSION;

/**
 * librdf_version_major:
 *
 * Library major version number as a decimal integer.
 */
const unsigned int librdf_version_major = LIBRDF_VERSION_MAJOR;

/**
 * librdf_version_minor:
 *
 * Library minor version number as a decimal integer.
 */
const unsigned int librdf_version_minor = LIBRDF_VERSION_MINOR;

/**
 * librdf_version_release:
 *
 * Library release version number as a decimal integer.
 */
const unsigned int librdf_version_release = LIBRDF_VERSION_RELEASE;

/**
 * librdf_version_decimal:
 *
 * Library full version as a decimal integer.
 *
 * See also #librdf_version_string.
 */
const unsigned int librdf_version_decimal = LIBRDF_VERSION_DECIMAL;




/**
 * librdf_new_world:
 *
 * Create a new Redland execution environment.
 *
 * Once this constructor is called to build a #librdf_world object
 * several functions may be called to set some parameters such as
 * librdf_world_set_error(), librdf_world_set_warning(),
 * librdf_world_set_logger(), librdf_world_set_digest(),
 * librdf_world_set_feature().
 *
 * The world object needs initializing using librdf_world_open()
 * whether or not the above functions are called.  It will be
 * automatically called by all object constructors in Redland 1.0.6
 * or later, but for earlier versions it MUST be called before using
 * any other part of Redland.
 *
 * Returns: a new #librdf_world or NULL on failure
 */
librdf_world*
librdf_new_world(void)
{
  librdf_world *world;
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;
  struct timezone tz;
#endif

  world = LIBRDF_CALLOC(librdf_world*, 1, sizeof(*world));

  if(!world)
    return NULL;

#ifdef HAVE_GETTIMEOFDAY
  if(!gettimeofday(&tv, &tz)) {
    world->genid_base = LIBRDF_GOOD_CAST(unsigned long, tv.tv_sec);
  } else
    world->genid_base = 1;
#else
  world->genid_base = 1;
#endif
  world->genid_counter = 1;
  
#ifdef MODULAR_LIBRDF
  world->ltdl_opened = !(lt_dlinit());
  if (world->ltdl_opened)
    lt_dlsetsearchpath(REDLAND_MODULE_PATH);
#ifdef LIBRDF_DEBUG
  else
    LIBRDF_DEBUG1("lt_dlinit() failed\n");
#endif
#endif

  return world;
  
}


/**
 * librdf_free_world:
 * @world: redland world object
 *
 * Terminate the library and frees all allocated resources.
 **/
void
librdf_free_world(librdf_world *world)
{
  if(!world)
    return;
  
  librdf_finish_serializer(world);
  librdf_finish_parser(world);

  librdf_finish_storage(world);
  librdf_finish_query(world);
  librdf_finish_model(world);
  librdf_finish_statement(world);

  librdf_finish_concepts(world);

  librdf_finish_node(world);
  librdf_finish_uri(world);

  /* Uses rdf_hash so free before destroying hashes */
  librdf_finish_raptor(world);

  librdf_finish_hash(world);

  librdf_finish_digest(world);

#ifdef WITH_THREADS

  if(world->hash_datums_mutex) {
    pthread_mutex_destroy(world->hash_datums_mutex);
    SYSTEM_FREE(world->hash_datums_mutex);
    world->hash_datums_mutex = NULL;
  }

  if(world->statements_mutex) {
    pthread_mutex_destroy(world->statements_mutex);
    SYSTEM_FREE(world->statements_mutex);
    world->statements_mutex = NULL;
  }

  if(world->nodes_mutex) {
    pthread_mutex_destroy(world->nodes_mutex);
    SYSTEM_FREE(world->nodes_mutex);
    world->nodes_mutex = NULL;
  }

  if(world->mutex) {
    pthread_mutex_destroy(world->mutex);
    SYSTEM_FREE(world->mutex);
    world->mutex = NULL;
  }


#endif

#ifdef MODULAR_LIBRDF
  if (world->ltdl_opened)
    lt_dlexit();
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

  world->hash_datums_mutex = (pthread_mutex_t *) SYSTEM_MALLOC(sizeof(pthread_mutex_t));
  pthread_mutex_init(world->hash_datums_mutex, NULL);

#else
#endif
}


/**
 * librdf_world_open:
 * @world: redland world object
 *
 * Open a created redland world environment.
 **/
void
librdf_world_open(librdf_world *world)
{
  int rc = 0;
  
  if(world->opened++)
    return;
  
  librdf_world_init_mutex(world);

  /* Digests second, lots of things use these */
  librdf_init_digest(world);

  /* Hash next, needed for URIs */
  librdf_init_hash(world);

  /* Initialize raptor; uses rdf_hash for bnode mapping */
  librdf_init_raptor(world);
  
  librdf_init_uri(world);
  librdf_init_node(world);

  librdf_init_concepts(world);

  librdf_init_statement(world);
  librdf_init_model(world);
  librdf_init_storage(world);

  librdf_init_parser(world);
  librdf_init_serializer(world);

  rc = librdf_init_query(world);
  if(rc)
    goto failed;
  
  return;
  
failed:
  /* should return an error state */
  return;
}


/**
 * librdf_world_set_rasqal:
 * @world: librdf_world object
 * @rasqal_world_ptr: rasqal_world object
 *
 * Set the #rasqal_world instance to be used with this #librdf_world.
 *
 * If no rasqal_world instance is set with this function,
 * librdf_world_open() creates a new instance.
 *
 * Ownership of the rasqal_world is not taken. If the rasqal library
 * instance is set with this function, librdf_free_world() will not
 * free it.
 *
 **/
void
librdf_world_set_rasqal(librdf_world* world, rasqal_world* rasqal_world_ptr)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(world, librdf_world);
  world->rasqal_world_ptr = rasqal_world_ptr;
  world->rasqal_world_allocated_here = 0;
}


/**
 * librdf_world_get_rasqal:
 * @world: librdf_world object
 *
 * Get the #rasqal_world instance used by this #librdf_world.
 *
 * Return value: rasqal_world object or NULL on failure (e.g. not initialized)
 **/
rasqal_world*
librdf_world_get_rasqal(librdf_world* world)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(world, librdf_world, NULL);
  return world->rasqal_world_ptr;
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
  world->error_user_data = user_data;
  world->error_handler = error_handler;
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
  world->warning_user_data = user_data;
  world->warning_handler = warning_handler;
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
  world->log_user_data = user_data;
  world->log_handler = log_handler;
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
librdf_world_set_digest(librdf_world* world, const char *name)
{
  world->digest_factory_name = (char*)name;
}


/**
 * librdf_world_get_feature:
 * @world: #librdf_world object
 * @feature: #librdf_uri feature property
 * 
 * Get the value of a world feature.
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
  librdf_uri* genid_base;
  librdf_uri* genid_counter;
  int rc= -1;

  genid_counter = librdf_new_uri(world,
                                 (const unsigned char*)LIBRDF_WORLD_FEATURE_GENID_COUNTER);
  genid_base = librdf_new_uri(world,
                              (const unsigned char*)LIBRDF_WORLD_FEATURE_GENID_BASE);

  if(librdf_uri_equals(feature, genid_base)) {
    if(!librdf_node_is_resource(value))
      rc=1;
    else {
      long lid = atol((const char*)librdf_node_get_literal_value(value));
      if(lid < 1)
        lid = 1;

#ifdef WITH_THREADS
      pthread_mutex_lock(world->mutex);
#endif
      world->genid_base = LIBRDF_GOOD_CAST(unsigned long, lid);
#ifdef WITH_THREADS
      pthread_mutex_unlock(world->mutex);
#endif
      rc = 0;
    }
  } else if(librdf_uri_equals(feature, genid_counter)) {
    if(!librdf_node_is_resource(value))
      rc = 1;
    else {
      long lid = atol((const char*)librdf_node_get_literal_value(value));
      if(lid < 1)
        lid = 1;

#ifdef WITH_THREADS
      pthread_mutex_lock(world->mutex);
#endif
      world->genid_counter = LIBRDF_GOOD_CAST(unsigned long, lid);
#ifdef WITH_THREADS
      pthread_mutex_unlock(world->mutex);
#endif
      rc = 0;
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
  unsigned long id, tmpid, counter, tmpcounter, pid, tmppid;
  size_t length;
  unsigned char *buffer;

  /* This is read-only and thread safe */
  tmpid = (id = world->genid_base);

#ifdef WITH_THREADS
  pthread_mutex_lock(world->mutex);
#endif
  tmpcounter = (counter = world->genid_counter++);
#ifdef WITH_THREADS
  pthread_mutex_unlock(world->mutex);
#endif

  /* Add the process ID to the seed to differentiate between
   * simultaneously executed child processes.
   */
  pid = LIBRDF_GOOD_CAST(unsigned long, getpid());
  if(!pid)
    pid = 1;
  tmppid = pid;


  /* min length 1 + "r" + min length 1 + "r" + min length 1 + "r" \0 */
  length = 7;

  while(tmpid /= 10)
    length++;
  while(tmpcounter /= 10)
    length++;
  while(tmppid /= 10)
    length++;
  
  buffer = LIBRDF_MALLOC(unsigned char*, length);
  if(!buffer)
    return NULL;

  sprintf((char*)buffer, "r%lur%lur%lu", id, pid, counter);
  return buffer;
}



/* OLD INTERFACES BELOW HERE */

/* For old interfaces below ONLY */
#ifndef NO_STATIC_DATA
static librdf_world* RDF_World;
#endif

#ifndef REDLAND_DISABLE_DEPRECATED
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
 * See librdf_world_set_digest() for documentation on arguments.
 **/
void
librdf_init_world(char *digest_factory_name, void* not_used2)
{
#ifndef NO_STATIC_DATA
  RDF_World = librdf_new_world();
  if(!RDF_World)
    return;

  if(digest_factory_name)
    librdf_world_set_digest(RDF_World, digest_factory_name);
  librdf_world_open(RDF_World);
#else
  /* fail if NO_STATIC_DATA is defined */
  abort();
#endif
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
#ifndef NO_STATIC_DATA
  librdf_free_world(RDF_World);
#else
  /* fail if NO_STATIC_DATA is defined */
  abort();
#endif
}
#endif

/**
 * librdf_basename:
 * @name: path
 * 
 * Get the basename of a path
 * 
 * Return value: filename part of a pathname
 **/
const char*
librdf_basename(const char *name)
{
  const char *p;
  if((p = strrchr(name, '/')))
    name = p+1;
  else if((p = strrchr(name, '\\')))
    name = p+1;

  return name;
}


/**
 * librdf_free_memory:
 * @ptr: pointer to free
 *
 * Free memory allocated in the library.
 *
 * Required for some runtimes where memory must be freed within the same
 * shared object it was allocated in.
 **/
void
librdf_free_memory(void *ptr)
{
  raptor_free_memory(ptr);
}


/**
 * librdf_alloc_memory:
 * @size: alloc size
 *
 * Allocate memory inside the library similar to malloc().
 *
 * Required for some runtimes where memory must be freed within the same
 * shared object it was allocated in.
 *
 * Return value: pointer to memory or NULL on failure
 **/
void*
librdf_alloc_memory(size_t size)
{
  return raptor_alloc_memory(size);
}

/**
 * librdf_calloc_memory:
 * @nmemb: number of members
 * @size: size of member
 *
 * Allocate zeroed array of items inside the library similar to calloc().
 *
 * Required for some runtimes where memory must be freed within the same
 * shared object it was allocated in.
 * 
 * Return value: pointer to memory or NULL on failure
 **/ 
void*
librdf_calloc_memory(size_t nmemb, size_t size)
{
  return raptor_calloc_memory(nmemb, size);
}


#endif /* STANDALONE */


/* TEST CODE */


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_world *world;
  rasqal_world *rasqal_world;
  unsigned char* id;
  const char *program=librdf_basename((const char*)argv[0]);

  /* Minimal setup-cleanup test without opening the world */
  fprintf(stdout, "%s: Creating new world\n", program);
  world = librdf_new_world();
  if(!world) {
    fprintf(stderr, "%s: librdf_new_world failed\n", program);
    return 1;
  }
  fprintf(stdout, "%s: Deleting world\n", program);
  librdf_free_world(world);


  /* Test setting and getting a the rasqal world */
  fprintf(stdout, "%s: Setting and getting rasqal_world\n", program);
  world = librdf_new_world();
  if(!world) {
    fprintf(stderr, "%s: librdf_new_world failed\n", program);
    return 1;
  }

  rasqal_world = rasqal_new_world();
  if(!rasqal_world) {
    fprintf(stderr, "%s: rasqal_new_world failed\n", program);
    return 1;
  }

  librdf_world_set_rasqal(world, rasqal_world);
  if (librdf_world_get_rasqal(world) != rasqal_world) {
    fprintf(stderr, "%s: librdf_world_set_rasqal/librdf_world_get_rasqal failed\n", program);
    return 1;
  }
  rasqal_free_world(rasqal_world);
  librdf_free_world(world);


  /* Test id generation */
  fprintf(stdout, "%s: Creating new world\n", program);
  world = librdf_new_world();
  if(!world) {
    fprintf(stderr, "%s: librdf_new_world failed\n", program);
    return 1;
  }

  fprintf(stdout, "%s: Generating an identifier\n", program);
  id = librdf_world_get_genid(world);
  if(id == NULL || strlen((const char*)id) < 6) {
    fprintf(stderr, "%s: librdf_world_get_genid failed\n", program);
    return 1;
  }
  fprintf(stdout, "%s: New identifier is: '%s'\n", program, id);
  LIBRDF_FREE(char*, id);

  fprintf(stdout, "%s: Deleting world\n", program);
  librdf_free_world(world);

  /* keep gcc -Wall happy */
  return(0);
}

#endif
