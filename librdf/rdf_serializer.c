/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_serializer.c - RDF Serializer Factory implementation
 *
 * $Id$
 *
 * Copyright (C) 2002-2003 David Beckett - http://purl.org/net/dajobe/
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


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <redland.h>


#ifndef STANDALONE

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
                                   const unsigned char *uri_string,
                                   void (*factory) (librdf_serializer_factory*))
{
  librdf_serializer_factory *serializer_factory;
  char *name_copy;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2("Received registration for serializer %s\n", name);
#endif
  
  serializer_factory=(librdf_serializer_factory*)LIBRDF_CALLOC(librdf_serializer_factory, 1,
					       sizeof(librdf_serializer_factory));
  if(!serializer_factory)
    LIBRDF_FATAL1(world, LIBRDF_FROM_SERIALIZER, "Out of memory");
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, 1, strlen(name)+1);
  if(!name_copy) {
    librdf_free_serializer_factory(serializer_factory);
    LIBRDF_FATAL1(world, LIBRDF_FROM_SERIALIZER, "Out of memory");
  }
  strcpy(name_copy, name);
  serializer_factory->name=name_copy;

  /* register mime type if any */
  if(mime_type) {
    char *mime_type_copy;
    mime_type_copy=(char*)LIBRDF_CALLOC(cstring, 1, strlen(mime_type)+1);
    if(!mime_type_copy) {
      librdf_free_serializer_factory(serializer_factory);
      LIBRDF_FATAL1(world, LIBRDF_FROM_SERIALIZER, "Out of memory");
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
      LIBRDF_FATAL1(world, LIBRDF_FROM_SERIALIZER, "Out of memory");
    }
    serializer_factory->type_uri=uri;
  }
  
        
  /* Call the serializer registration function on the new object */
  (*factory)(serializer_factory);


#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3("%s has context size %d\n", name, serializer_factory->context_length);
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
      LIBRDF_DEBUG1("No serializers available\n");
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
         ! librdf_uri_equals(factory->type_uri, type_uri))
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

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(factory, librdf_serializer_factory, NULL);

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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(serializer, librdf_serializer);

  if(serializer->context) {
    if(serializer->factory->terminate)
      serializer->factory->terminate(serializer->context);
    LIBRDF_FREE(serializer_context, serializer->context);
  }
  LIBRDF_FREE(librdf_serializer, serializer);
}



/* methods */

/**
 * librdf_serializer_serialize_model - Write a serialized librdf_mode to a FILE*
 * @serializer: the serializer
 * @handle: file handle to serialize to
 * @base_uri: the base URI to use or NULL
 * @model: the &librdf_model model to use
 * 
 * DEPRECATED: Use librdf_serializer_serialize_model_to_file_handle
 *
 * Return value: non 0 on failure
 **/
int
librdf_serializer_serialize_model(librdf_serializer* serializer,
                                  FILE *handle, librdf_uri* base_uri,
                                  librdf_model* model) 
{
  return librdf_serializer_serialize_model_to_file_handle(serializer,
                                                          handle, base_uri,
                                                          model);
}


/**
 * librdf_serializer_serialize_model_to_file_handle - Write a serialized librdf_model to a FILE*
 * @serializer: the serializer
 * @handle: file handle to serialize to
 * @base_uri: the base URI to use or NULL
 * @model: the &librdf_model model to use
 * 
 * Return value: non 0 on failure
 **/
int
librdf_serializer_serialize_model_to_file_handle(librdf_serializer* serializer,
                                                 FILE *handle, 
                                                 librdf_uri* base_uri,
                                                 librdf_model* model) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(serializer, librdf_serializer, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);

  return serializer->factory->serialize_model_to_file_handle(serializer->context,
                                                             handle, base_uri, model);
}


/**
 * librdf_serializer_serialize_model_to_file - Write a serialized librdf_model to a file
 * @serializer: the serializer
 * @name: filename to serialize to
 * @base_uri: the base URI to use or NULL
 * @model: the &librdf_model model to use
 * 
 * Return value: non 0 on failure
 **/
int
librdf_serializer_serialize_model_to_file(librdf_serializer* serializer,
                                          const char *name, 
                                          librdf_uri* base_uri,
                                          librdf_model* model) 
{
  FILE* fh;
  int status;
  
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(serializer, librdf_serializer, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(name, string, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);

  fh=fopen(name, "w+");
  if(!fh)
    return 1;
  status=librdf_serializer_serialize_model_to_file_handle(serializer, fh, 
                                                          base_uri, model);
  fclose(fh);
  return status;
}


/**
 * librdf_serializer_serialize_model_to_string - Write a serialized librdf_model to a string
 * @serializer: the serializer
 * @base_uri: the base URI to use
 * @model: the &librdf_model model to use
 * 
 * Return value: non 0 on failure
 **/
unsigned char*
librdf_serializer_serialize_model_to_string(librdf_serializer* serializer,
                                            librdf_uri* base_uri,
                                            librdf_model* model) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(serializer, librdf_serializer, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(base_uri, librdf_uri, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, NULL);

  return serializer->factory->serialize_model_to_string(serializer->context,
                                                        base_uri, model);
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


/**
 * librdf_serializer_set_error - Set the serializer error handling function
 * @serializer: the serializer
 * @user_data: user data to pass to function
 * @error_fn: pointer to the function
 * 
 * DEPRECATED: Does nothing
 * 
 **/
void
librdf_serializer_set_error(librdf_serializer* serializer, void *user_data,
                            void (*error_fn)(void *user_data, const char *msg, ...))
{
}


/**
 * librdf_serializer_set_warning - Set the serializer warning handling function
 * @serializer: the serializer
 * @user_data: user data to pass to function
 * @warning_fn: pointer to the function
 * 
 * DEPRECATED: Does nothing
 * 
 **/
void
librdf_serializer_set_warning(librdf_serializer* serializer, void *user_data,
                              void (*warning_fn)(void *user_data, const char *msg, ...))
{
}


/**
 * librdf_serializer_get_feature - Get the value of a serializer feature
 * @serializer: serializer object
 * @feature: URI of feature
 * 
 * Return value: the value of the feature or NULL if no such feature
 * exists or the value is empty.
 **/
librdf_node*
librdf_serializer_get_feature(librdf_serializer* serializer, librdf_uri *feature) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(serializer, librdf_serializer, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(feature, librdf_uri, NULL);

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
                              librdf_uri *feature, librdf_node* value) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(serializer, librdf_serializer, -1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(feature, librdf_uri, -1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(value, librdf_node, -1);

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
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(serializer, librdf_serializer, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(uri, librdf_uri, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(prefix, string, 1);

  if(serializer->factory->set_namespace)
    return serializer->factory->set_namespace(serializer->context, uri, prefix);
  return 1;
}

#endif


/* TEST CODE */


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


struct log_data {
  int errors;
  int warnings;
} LogData;


static int
log_handler(void *user_data, librdf_log_message *message) 
{
  struct log_data* ld=(struct log_data*)user_data;

  switch(message->level) {
    case LIBRDF_LOG_ERROR:
      ld->errors++;
      break;
    case LIBRDF_LOG_WARN:
      ld->warnings++;
      break;
    default:
      break;
  }

  return 1;
}


#define EXPECTED_ERRORS 3
#define EXPECTED_WARNINGS 0


int
main(int argc, char *argv[]) 
{
  char *program=argv[0];
  char *test_serializer_types[]={"ntriples", NULL};
  int i;
  char *type;
  FILE *fh;
  librdf_world *world;
  librdf_storage *storage;
  librdf_model* model;
  librdf_uri* base_uri;
  librdf_statement* statement;
  librdf_serializer* serializer;

  world=librdf_new_world();
  librdf_world_open(world);

  librdf_world_set_logger(world, &LogData, log_handler);

  for(i=0; (type=test_serializer_types[i]); i++) {
    fprintf(stderr, "%s: Trying to create new %s serializer\n", program, type);
    serializer=librdf_new_serializer(world, type, NULL, NULL);
    if(!serializer) {
      fprintf(stderr, "%s: Failed to create new serializer type %s\n", program, type);
      continue;
    }
    
    fprintf(stderr, "%s: Freeing serializer\n", program);
    librdf_free_serializer(serializer);
  }
  

#ifdef WIN32
#define NULL_FILENAME "nul"
#else
#define NULL_FILENAME "/dev/null"
#endif

  fh=fopen(NULL_FILENAME, "w");
  if(!fh) {
    fprintf(stderr, "%s: Cannot test serialising errors, cannot write to '%s'\n", program, NULL_FILENAME);
    goto skip;
  }
  

  storage=librdf_new_storage(world, NULL, NULL, NULL);
  model=librdf_new_model(world, storage, NULL);

  /* ERROR: Subject URI is bad UTF-8 */
  statement=librdf_new_statement_from_nodes(world,
    librdf_new_node_from_uri_string(world, "http://example.org/foo\xfc"),
    librdf_new_node_from_uri_string(world, "http://example.org/bar"),
    librdf_new_node_from_literal(world, "blah", NULL, 0));

  librdf_model_add_statement(model, statement);
  librdf_free_statement(statement);

  /* ERROR: Predicate URI is not serializable */
  statement=librdf_new_statement_from_nodes(world,
    librdf_new_node_from_uri_string(world, "http://example.org/foo"),
    librdf_new_node_from_uri_string(world, "http://bad.example.org/"),
    librdf_new_node_from_literal(world, "blah", NULL, 0));

  librdf_model_add_statement(model, statement);
  librdf_free_statement(statement);

  /* ERROR: Object literal is bad UTF-8 */
  statement=librdf_new_statement_from_nodes(world,
    librdf_new_node_from_uri_string(world, "http://example.org/foo"),
    librdf_new_node_from_uri_string(world, "http://example.org/abc"),
    librdf_new_node_from_literal(world, "\xfc", NULL, 0));

  librdf_model_add_statement(model, statement);
  librdf_free_statement(statement);

  serializer=librdf_new_serializer(world, "rdfxml", NULL, NULL);
  base_uri=librdf_new_uri(world,"http://example.org/base#");

  librdf_serializer_serialize_model_to_file_handle(serializer, fh, 
                                                   base_uri, model);
  librdf_free_serializer(serializer);
  fclose(fh);

  librdf_free_uri(base_uri);
  
  librdf_free_model(model);
  librdf_free_storage(storage);

  if(LogData.errors != EXPECTED_ERRORS) {
    fprintf(stderr, "%s: Serialising to RDF/XML returned %d errors, expected %d\n", program,
            LogData.errors, EXPECTED_ERRORS);
    return 1;
  }

  if(LogData.warnings != EXPECTED_WARNINGS) {
    fprintf(stderr, "%s: Serialising to RDF/XML returned %d warnings, expected %d\n", program,
            LogData.warnings, EXPECTED_WARNINGS);
    return 1;
  }
  
  skip:
  
  librdf_free_world(world);
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
