/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_serializer.c - RDF Serializer Factory implementation
 *
 * $Id$
 *
 * Copyright (C) 2002 David Beckett - http://purl.org/net/dajobe/
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
#include <string.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include <librdf.h>
#include <rdf_uri.h>
#include <rdf_serializer.h>


/* prototypes for helper functions */
static void librdf_delete_serializer_factories(librdf_world *world);


/* helper functions */
static void
librdf_free_serializer_factory(librdf_serializer_factory *factory) 
{
  if(factory->name)
    LIBRDF_FREE(librdf_serializer_factory, factory->name);
  if(factory->mime_type)
    LIBRDF_FREE(librdf_serializer_factory, factory->mime_type);
  if(factory->type_uri)
    librdf_free_uri(factory->type_uri);
  LIBRDF_FREE(librdf_serializer_factory, factory);
}


static void
librdf_delete_serializer_factories(librdf_world *world)
{
  librdf_serializer_factory *factory, *next;
  
  for(factory=world->serializers; factory; factory=next) {
    next=factory->next;
    librdf_free_serializer_factory(factory);
  }
  world->serializers=NULL;
}


/**
 * librdf_serializer_register_factory - Register a serializer factory 
 * @world: redland world object
 * @name: the name of the serializer
 * @mime_type: MIME type of the syntax (optional)
 * @uri_string: URI of the syntax (optional)
 * @factory: function to be called to register the factor parameters
 * 
 **/
void
librdf_serializer_register_factory(librdf_world *world,
                                   const char *name, const char *mime_type,
                                   const char *uri_string,
                                   void (*factory) (librdf_serializer_factory*))
{
  librdf_serializer_factory *serializer_factory;
  char *name_copy;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2(librdf_serializer_register_factory,
		"Received registration for serializer %s\n", name);
#endif
  
  serializer_factory=(librdf_serializer_factory*)LIBRDF_CALLOC(librdf_serializer_factory, 1,
					       sizeof(librdf_serializer_factory));
  if(!serializer_factory)
    LIBRDF_FATAL1(librdf_serializer_register_factory,
		  "Out of memory\n");
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, 1, strlen(name)+1);
  if(!name_copy) {
    librdf_free_serializer_factory(serializer_factory);
    LIBRDF_FATAL1(librdf_serializer_register_factory, "Out of memory\n");
  }
  strcpy(name_copy, name);
  serializer_factory->name=name_copy;

  /* register mime type if any */
  if(mime_type) {
    char *mime_type_copy;
    mime_type_copy=(char*)LIBRDF_CALLOC(cstring, 1, strlen(mime_type)+1);
    if(!mime_type_copy) {
      librdf_free_serializer_factory(serializer_factory);
      LIBRDF_FATAL1(librdf_serializer_register_factory, "Out of memory\n");
    }
    strcpy(mime_type_copy, mime_type);
    serializer_factory->mime_type=mime_type_copy;
  }

  /* register URI if any */
  if(uri_string) {
    librdf_uri *uri;

    uri=librdf_new_uri(world, uri_string);
    if(!uri) {
      librdf_free_serializer_factory(serializer_factory);
      LIBRDF_FATAL1(librdf_serializer_register_factory, "Out of memory\n");
    }
    serializer_factory->type_uri=uri;
  }
  
        
  /* Call the serializer registration function on the new object */
  (*factory)(serializer_factory);


#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3(librdf_serializer_register_factory,
		"%s has context size %d\n", name,
		serializer_factory->context_length);
#endif
  
  serializer_factory->next = world->serializers;
  world->serializers = serializer_factory;
  
}


/**
 * librdf_get_serializer_factory - Get a serializer factory by name
 * @world: redland world object
 * @name: the name of the factory (NULL or empty string if don't care)
 * @mime_type: the MIME type of the syntax (NULL or empty string if not used)
 * @type_uri: URI of syntax (NULL if not used)
 * 
 * If all fields are NULL, this means any parser supporting
 * MIME Type "application/rdf+xml"
 *
 * Return value: the factory or NULL if not found
 **/
librdf_serializer_factory*
librdf_get_serializer_factory(librdf_world *world,
                              const char *name, const char *mime_type,
                              librdf_uri *type_uri) 
{
  librdf_serializer_factory *factory;
  
  if(name && !*name)
    name=NULL;
  if(!mime_type || (mime_type && !*mime_type)) {
    if(!name && !type_uri)
      mime_type="application/rdf+xml";
    else
      mime_type=NULL;
  }

  /* return 1st serializer if no particular one wanted */
  if(!name && !mime_type && !type_uri) {
    factory=world->serializers;
    if(!factory) {
      LIBRDF_DEBUG1(librdf_get_serializer_factory, "No serializers available\n");
      return NULL;
    }
  } else {
    for(factory=world->serializers; factory; factory=factory->next) {
      /* next if name does not match */
      if(name && strcmp(factory->name, name))
	continue;

      /* MIME type may need to match */
      if(mime_type && factory->mime_type &&
         strcmp(factory->mime_type, mime_type))
        continue;
      
      /* URI may need to match */
      if(type_uri && factory->type_uri &&
         librdf_uri_equals(factory->type_uri, type_uri))
        continue;

      /* found it */
      break;
    }
    /* else FACTORY with given arguments not found */
    if(!factory)
      return NULL;
  }
  
  return factory;
}


/**
 * librdf_new_serializer - Constructor - create a new librdf_serializer object
 * @world: redland world object
 * @name: the serializer factory name
 * @mime_type: the MIME type of the syntax (NULL if not used)
 * @type_uri: URI of syntax (NULL if not used)
 * 
 * Return value: new &librdf_serializer object or NULL
 **/
librdf_serializer*
librdf_new_serializer(librdf_world *world, 
                      const char *name, const char *mime_type,
                      librdf_uri *type_uri)
{
  librdf_serializer_factory* factory;

  factory=librdf_get_serializer_factory(world, name, mime_type, type_uri);
  if(!factory)
    return NULL;

  return librdf_new_serializer_from_factory(world, factory);
}


/**
 * librdf_new_serializer_from_factory - Constructor - create a new librdf_serializer object
 * @world: redland world object
 * @factory: the serializer factory to use to create this serializer
 * 
 * Return value: new &librdf_serializer object or NULL
 **/
librdf_serializer*
librdf_new_serializer_from_factory(librdf_world *world, 
                                   librdf_serializer_factory *factory)
{
  librdf_serializer* d;

  d=(librdf_serializer*)LIBRDF_CALLOC(librdf_serializer, 1, sizeof(librdf_serializer));
  if(!d)
    return NULL;
        
  d->context=(char*)LIBRDF_CALLOC(serializer_context, 1, factory->context_length);
  if(!d->context) {
    librdf_free_serializer(d);
    return NULL;
  }

  d->world=world;
  
  d->factory=factory;

  if(factory->init)
    factory->init(d, d->context);

  return d;
}


/**
 * librdf_free_serializer - Destructor - destroys a librdf_serializer object
 * @serializer: the serializer
 * 
 **/
void
librdf_free_serializer(librdf_serializer *serializer) 
{
  if(serializer->context) {
    if(serializer->factory->terminate)
      serializer->factory->terminate(serializer->context);
    LIBRDF_FREE(serializer_context, serializer->context);
  }
  LIBRDF_FREE(librdf_serializer, serializer);
}



/* methods */

/**
 * librdf_serializer_serialize_model - Turn a librdf_model into a serialized form
 * @serializer: the serializer
 * @base_uri: the base URI to use
 * @model: the &librdf_model model to use
 * 
 * Return value: non 0 on failure
 **/
int
librdf_serializer_serialize_model(librdf_serializer* serializer,
                                  FILE *handle, librdf_uri* base_uri,
                                  librdf_model* model) 
{
  return serializer->factory->serialize_model(serializer->context,
                                              handle, base_uri, model);
}


/**
 * librdf_init_serializer - Initialise the librdf_serializer class
 * @world: redland world object
 **/
void
librdf_init_serializer(librdf_world *world) 
{
  librdf_serializer_raptor_constructor(world);
}


/**
 * librdf_finish_serializer - Terminate the librdf_serializer class
 * @world: redland world object
 **/
void
librdf_finish_serializer(librdf_world *world) 
{
  librdf_delete_serializer_factories(world);
}


/*
 * librdf_serializer_error - Error from a serializer - Internal
 **/
void
librdf_serializer_error(librdf_serializer* serializer, const char *message, ...)
{
  va_list arguments;

  if(serializer->error_fn) {
    serializer->error_fn(serializer->error_user_data, message);
    return;
  }
  
  va_start(arguments, message);

  fprintf(stderr, "%s serializer error - ", serializer->factory->name);
  vfprintf(stderr, message, arguments);
  fputc('\n', stderr);

  va_end(arguments);
}


/*
 * librdf_serializer_warning - Warning from a serializer - Internal
 **/
void
librdf_serializer_warning(librdf_serializer* serializer, const char *message, ...)
{
  va_list arguments;

  if(serializer->warning_fn) {
    serializer->warning_fn(serializer->warning_user_data, message);
    return;
  }
  
  va_start(arguments, message);

  fprintf(stderr, "%s serializer warning - ", serializer->factory->name);
  vfprintf(stderr, message, arguments);
  fputc('\n', stderr);

  va_end(arguments);
}


/**
 * librdf_serializer_set_error - Set the serializer error handling function
 * @serializer: the serializer
 * @user_data: user data to pass to function
 * @error_fn: pointer to the function
 * 
 * The function will receive callbacks when the serializer fails.
 * 
 **/
void
librdf_serializer_set_error(librdf_serializer* serializer, void *user_data,
                            void (*error_fn)(void *user_data, const char *msg, ...))
{
  serializer->error_user_data=user_data;
  serializer->error_fn=error_fn;
}


/**
 * librdf_serializer_set_warning - Set the serializer warning handling function
 * @serializer: the serializer
 * @user_data: user data to pass to function
 * @warning_fn: pointer to the function
 * 
 * The function will receive callbacks when the serializer gives a warning.
 * 
 **/
void
librdf_serializer_set_warning(librdf_serializer* serializer, void *user_data,
                              void (*warning_fn)(void *user_data, const char *msg, ...))
{
  serializer->warning_user_data=user_data;
  serializer->warning_fn=warning_fn;
}


/**
 * librdf_serializer_get_feature - Get the value of a serializer feature
 * @serializer: serializer object
 * @feature: URI of feature
 * 
 * Return value: the value of the feature or NULL if no such feature
 * exists or the value is empty.
 **/
const char *
librdf_serializer_get_feature(librdf_serializer* serializer, librdf_uri *feature) 
{
  if(serializer->factory->get_feature)
    return serializer->factory->get_feature(serializer->context, feature);

  return NULL;
}

/**
 * librdf_serializer_set_feature - Set the value of a serializer feature
 * @serializer: serializer object
 * @feature: URI of feature
 * @value: value to set
 * 
 * Return value: non 0 on failure (negative if no such feature)
 **/
  
int
librdf_serializer_set_feature(librdf_serializer* serializer,
                              librdf_uri *feature, const char *value) 
{
  if(serializer->factory->set_feature)
    return serializer->factory->set_feature(serializer->context, feature, value);

  return (-1);
}

/**
 * librdf_serializer_set_namespace - Set a namespace URI/prefix mapping
 * @serializer: serializer object
 * @uri: URI of namespace
 * @prefix: prefix to use
 * 
 * Return value: non 0 on failure
 **/
  
int
librdf_serializer_set_namespace(librdf_serializer* serializer,
                                librdf_uri *uri, const char *prefix) 
{
  if(serializer->factory->set_namespace)
    return serializer->factory->set_namespace(serializer->context, uri, prefix);
  return 1;
}



#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_serializer* d;
  char *test_serializer_types[]={"ntriples", "raptor-rdfxml", NULL};
  int i;
  char *type;
  char *program=argv[0];
  librdf_world *world;

  world=librdf_new_world();

  /* Needed for URI use when registering factories */
  librdf_init_digest(world);
  librdf_init_hash(world);
  librdf_init_uri(world);

  /* initialise serializer module */
  librdf_init_serializer(world);
  
  for(i=0; (type=test_serializer_types[i]); i++) {
    fprintf(stderr, "%s: Trying to create new %s serializer\n", program, type);
    d=librdf_new_serializer(world, type, NULL, NULL);
    if(!d) {
      fprintf(stderr, "%s: Failed to create new serializer type %s\n", program, type);
      continue;
    }
    
    fprintf(stderr, "%s: Freeing serializer\n", program);
    librdf_free_serializer(d);
  }
  
  
  /* finish serializer module */
  librdf_finish_serializer(world);

  librdf_finish_uri(world);
  librdf_finish_hash(world);
  librdf_finish_digest(world);

  LIBRDF_FREE(librdf_world, world);
  
#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
