/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser Factory implementation
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
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include <librdf.h>
#include <rdf_parser.h>


/* prototypes for helper functions */
static void librdf_delete_parser_factories(librdf_world *world);


/* helper functions */
static void
librdf_free_parser_factory(librdf_parser_factory *factory) 
{
  if(factory->name)
    LIBRDF_FREE(librdf_parser_factory, factory->name);
  if(factory->mime_type)
    LIBRDF_FREE(librdf_parser_factory, factory->mime_type);
  if(factory->type_uri)
    librdf_free_uri(factory->type_uri);
  LIBRDF_FREE(librdf_parser_factory, factory);
}


static void
librdf_delete_parser_factories(librdf_world *world)
{
  librdf_parser_factory *factory, *next;
  
  for(factory=world->parsers; factory; factory=next) {
    next=factory->next;
    librdf_free_parser_factory(factory);
  }
  world->parsers=NULL;
}


/**
 * librdf_parser_register_factory - Register a parser factory 
 * @world: redland world object
 * @name: the name of the parser
 * @mime_type: MIME type of the syntax (optional)
 * @uri_string: URI of the syntax (optional)
 * @factory: function to be called to register the factor parameters
 * 
 **/
void
librdf_parser_register_factory(librdf_world *world,
                               const char *name, const char *mime_type,
                               const char *uri_string,
			       void (*factory) (librdf_parser_factory*))
{
  librdf_parser_factory *parser_factory;
  char *name_copy;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2(librdf_parser_register_factory,
		"Received registration for parser %s\n", name);
#endif
  
  parser_factory=(librdf_parser_factory*)LIBRDF_CALLOC(librdf_parser_factory, 1,
					       sizeof(librdf_parser_factory));
  if(!parser_factory)
    LIBRDF_FATAL1(librdf_parser_register_factory,
		  "Out of memory\n");
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, 1, strlen(name)+1);
  if(!name_copy) {
    librdf_free_parser_factory(parser_factory);
    LIBRDF_FATAL1(librdf_parser_register_factory, "Out of memory\n");
  }
  strcpy(name_copy, name);
  parser_factory->name=name_copy;

  /* register mime type if any */
  if(mime_type) {
    char *mime_type_copy;
    mime_type_copy=(char*)LIBRDF_CALLOC(cstring, 1, strlen(mime_type)+1);
    if(!mime_type_copy) {
      librdf_free_parser_factory(parser_factory);
      LIBRDF_FATAL1(librdf_parser_register_factory, "Out of memory\n");
    }
    strcpy(mime_type_copy, mime_type);
    parser_factory->mime_type=mime_type_copy;
  }

  /* register URI if any */
  if(uri_string) {
    librdf_uri *uri;

    uri=librdf_new_uri(world, uri_string);
    if(!uri) {
      librdf_free_parser_factory(parser_factory);
      LIBRDF_FATAL1(librdf_parser_register_factory, "Out of memory\n");
    }
    parser_factory->type_uri=uri;
  }
  
        
  /* Call the parser registration function on the new object */
  (*factory)(parser_factory);


#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3(librdf_parser_register_factory,
		"%s has context size %d\n", name,
		parser_factory->context_length);
#endif
  
  parser_factory->next = world->parsers;
  world->parsers = parser_factory;
  
}


/**
 * librdf_get_parser_factory - Get a parser factory by name
 * @world: redland world object
 * @name: the name of the factory
 * @mime_type: the MIME type of the syntax (NULL if not used)
 * @type_uri: URI of syntax (NULL if not used)
 * 
 * Return value: the factory or NULL if not found
 **/
librdf_parser_factory*
librdf_get_parser_factory(librdf_world *world,
                          const char *name, const char *mime_type,
                          librdf_uri *type_uri) 
{
  librdf_parser_factory *factory;

  /* return 1st parser if no particular one wanted - why? */
  if(!name) {
    factory=world->parsers;
    if(!factory) {
      LIBRDF_DEBUG1(librdf_get_parser_factory, "No parsers available\n");
      return NULL;
    }
  } else {
    for(factory=world->parsers; factory; factory=factory->next) {
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
 * librdf_new_parser - Constructor - create a new librdf_parser object
 * @world: redland world object
 * @name: the parser factory name
 * @mime_type: the MIME type of the syntax (NULL if not used)
 * @type_uri: URI of syntax (NULL if not used)
 * 
 * Return value: new &librdf_parser object or NULL
 **/
librdf_parser*
librdf_new_parser(librdf_world *world, 
                  const char *name, const char *mime_type,
                  librdf_uri *type_uri)
{
  librdf_parser_factory* factory;

  factory=librdf_get_parser_factory(world, name, mime_type, type_uri);
  if(!factory)
    return NULL;

  return librdf_new_parser_from_factory(world, factory);
}


/**
 * librdf_new_parser_from_factory - Constructor - create a new librdf_parser object
 * @world: redland world object
 * @factory: the parser factory to use to create this parser
 * 
 * Return value: new &librdf_parser object or NULL
 **/
librdf_parser*
librdf_new_parser_from_factory(librdf_world *world, 
                               librdf_parser_factory *factory)
{
  librdf_parser* d;

  d=(librdf_parser*)LIBRDF_CALLOC(librdf_parser, 1, sizeof(librdf_parser));
  if(!d)
    return NULL;
        
  d->context=(char*)LIBRDF_CALLOC(parser_context, 1, factory->context_length);
  if(!d->context) {
    librdf_free_parser(d);
    return NULL;
  }

  d->world=world;
  
  d->factory=factory;

  if(factory->init)
    factory->init(d, d->context);

  return d;
}


/**
 * librdf_free_parser - Destructor - destroys a librdf_parser object
 * @parser: the parser
 * 
 **/
void
librdf_free_parser(librdf_parser *parser) 
{
  if(parser->context) {
    if(parser->factory->terminate)
      parser->factory->terminate(parser->context);
    LIBRDF_FREE(parser_context, parser->context);
  }
  LIBRDF_FREE(librdf_parser, parser);
}



/* methods */

/**
 * librdf_parser_parse_as_stream - Parse a URI to a librdf_stream of statements
 * @parser: the parser
 * @uri: the URI to read
 * @base_uri: the base URI to use (or NULL if the same)
 * 
 * Return value: &librdf_stream of statements or NULL
 **/
librdf_stream*
librdf_parser_parse_as_stream(librdf_parser* parser, librdf_uri* uri,
                              librdf_uri* base_uri) 
{
  const char *filename;
  
  if(parser->factory->parse_uri_as_stream)
    return parser->factory->parse_uri_as_stream(parser->context,
                                                uri, base_uri);

  filename=librdf_uri_as_filename(uri);
  if(!filename) {
    LIBRDF_DEBUG2(librdf_parser_parse_as_stream, "%s parser can only handle file: URIs\n", parser->factory->name);
    return NULL;
  }
  return parser->factory->parse_file_as_stream(parser->context,
                                               uri, base_uri);
}


/**
 * librdf_parser_parse_into_model - Parse a URI of RDF/XML content into an librdf_model
 * @parser: the parser
 * @uri: the URI to read the content
 * @base_uri: the base URI to use (or NULL if the same)
 * @model: the model to use
 * 
 * Return value: non 0 on failure
 **/
int
librdf_parser_parse_into_model(librdf_parser* parser, librdf_uri* uri,
                               librdf_uri* base_uri, librdf_model* model) 
{
  const char *filename;
  
  if(parser->factory->parse_uri_into_model)
    return parser->factory->parse_uri_into_model(parser->context,
                                                 uri, base_uri, model);
  
  filename=librdf_uri_as_filename(uri);
  if(!filename) {
    LIBRDF_DEBUG2(librdf_parser_parse_into_stream, "%s parser can only handle file: URIs\n", parser->factory->name);
    return 1;
  }
  return parser->factory->parse_file_into_model(parser->context,
                                                uri, base_uri, model);
}




/**
 * librdf_init_parser - Initialise the librdf_parser class
 * @world: redland world object
 **/
void
librdf_init_parser(librdf_world *world) 
{
#ifdef HAVE_RAPTOR_RDF_PARSER
  librdf_parser_raptor_constructor(world);
#endif
#ifdef HAVE_SIRPAC_RDF_PARSER
  librdf_parser_sirpac_constructor(world);
#endif
#ifdef HAVE_LIBWWW_RDF_PARSER
  librdf_parser_libwww_constructor(world);
#endif
#ifdef HAVE_REPAT_RDF_PARSER
  librdf_parser_repat_constructor(world);
#endif
}


/**
 * librdf_finish_parser - Terminate the librdf_parser class
 * @world: redland world object
 **/
void
librdf_finish_parser(librdf_world *world) 
{
  librdf_delete_parser_factories(world);
}


/*
 * librdf_parser_error - Error from a parser - Internal
 **/
void
librdf_parser_error(librdf_parser* parser, const char *message, ...)
{
  va_list arguments;

  if(parser->error_fn) {
    parser->error_fn(parser->error_user_data, message);
    return;
  }
  
  va_start(arguments, message);

  fprintf(stderr, "%s parser error - ", parser->factory->name);
  vfprintf(stderr, message, arguments);
  fputc('\n', stderr);

  va_end(arguments);
}


/*
 * librdf_parser_warning - Warning from a parser - Internal
 **/
void
librdf_parser_warning(librdf_parser* parser, const char *message, ...)
{
  va_list arguments;

  if(parser->warning_fn) {
    parser->warning_fn(parser->warning_user_data, message);
    return;
  }
  
  va_start(arguments, message);

  fprintf(stderr, "%s parser warning - ", parser->factory->name);
  vfprintf(stderr, message, arguments);
  fputc('\n', stderr);

  va_end(arguments);
}


/**
 * librdf_parser_set_error - Set the parser error handling function
 * @parser: the parser
 * @user_data: user data to pass to function
 * @error_fn: pointer to the function
 * 
 * The function will receive callbacks when the parser fails.
 * 
 **/
void
librdf_parser_set_error(librdf_parser* parser, void *user_data,
                        void (*error_fn)(void *user_data, const char *msg, ...))
{
  parser->error_user_data=user_data;
  parser->error_fn=error_fn;
}


/**
 * librdf_parser_set_warning - Set the parser warning handling function
 * @parser: the parser
 * @user_data: user data to pass to function
 * @warning_fn: pointer to the function
 * 
 * The function will receive callbacks when the parser gives a warning.
 * 
 **/
void
librdf_parser_set_warning(librdf_parser* parser, void *user_data,
                          void (*warning_fn)(void *user_data, const char *msg, ...))
{
  parser->warning_user_data=user_data;
  parser->warning_fn=warning_fn;
}


/**
 * librdf_parser_get_feature - Get the value of a parser feature
 * @parser: parser object
 * @feature: URI of feature
 * 
 * Return value: the value of the feature or NULL if no such feature
 * exists or the value is empty.
 **/
const char *
librdf_parser_get_feature(librdf_parser* parser, librdf_uri *feature) 
{
  if(parser->factory->get_feature)
    return parser->factory->get_feature(parser->context, feature);

  return NULL;
}

/**
 * librdf_parser_set_feature - Set the value of a parser feature
 * @parser: parser object
 * @feature: URI of feature
 * @value: value to set
 * 
 * Return value: non 0 on failure (negative if no such feature)
 **/
  
int
librdf_parser_set_feature(librdf_parser* parser, librdf_uri *feature, 
                          const char *value) 
{
  if(parser->factory->set_feature)
    return parser->factory->set_feature(parser->context, feature, value);

  return (-1);
}




#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_parser* d;
  char *test_parser_types[]={"sirpac-stanford", "sirpac-w3c", "libwww", "repat", "raptor", NULL};
  int i;
  char *type;
  char *program=argv[0];
  librdf_world *world;

  world=librdf_new_world();

  /* Needed for URI use when registering factories */
  librdf_init_digest(world);
  librdf_init_hash(world);
  librdf_init_uri(world);

  /* initialise parser module */
  librdf_init_parser(world);
  
  for(i=0; (type=test_parser_types[i]); i++) {
    fprintf(stderr, "%s: Trying to create new %s parser\n", program, type);
    d=librdf_new_parser(world, type, NULL, NULL);
    if(!d) {
      fprintf(stderr, "%s: Failed to create new parser type %s\n", program, type);
      continue;
    }
    
    fprintf(stderr, "%s: Freeing parser\n", program);
    librdf_free_parser(d);
  }
  
  
  /* finish parser module */
  librdf_finish_parser(world);

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
