/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser (syntax to RDF triples) interface
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
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <redland.h>


#ifndef STANDALONE

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
 * librdf_parser_register_factory:
 * @world: redland world object
 * @name: the name of the parser
 * @mime_type: MIME type of the syntax (optional)
 * @uri_string: URI of the syntax (optional)
 * @factory: function to be called to register the factor parameters
 *
 * Register a parser factory .
 * 
 **/
void
librdf_parser_register_factory(librdf_world *world,
                               const char *name, const char *mime_type,
                               const unsigned char *uri_string,
			       void (*factory) (librdf_parser_factory*))
{
  librdf_parser_factory *parser_factory;
  char *name_copy;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2("Received registration for parser %s\n", name);
#endif
  
  parser_factory=(librdf_parser_factory*)LIBRDF_CALLOC(librdf_parser_factory, 1,
					       sizeof(librdf_parser_factory));
  if(!parser_factory)
    LIBRDF_FATAL1(world, LIBRDF_FROM_PARSER, "Out of memory");
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, 1, strlen(name)+1);
  if(!name_copy) {
    librdf_free_parser_factory(parser_factory);
    LIBRDF_FATAL1(world, LIBRDF_FROM_PARSER, "Out of memory");
  }
  strcpy(name_copy, name);
  parser_factory->name=name_copy;

  /* register mime type if any */
  if(mime_type) {
    char *mime_type_copy;
    mime_type_copy=(char*)LIBRDF_CALLOC(cstring, 1, strlen(mime_type)+1);
    if(!mime_type_copy) {
      librdf_free_parser_factory(parser_factory);
      LIBRDF_FATAL1(world, LIBRDF_FROM_PARSER, "Out of memory");
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
      LIBRDF_FATAL1(world, LIBRDF_FROM_PARSER, "Out of memory");
    }
    parser_factory->type_uri=uri;
  }
  
        
  /* Call the parser registration function on the new object */
  (*factory)(parser_factory);


#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3("%s has context size %d\n", name, parser_factory->context_length);
#endif
  
  parser_factory->next = world->parsers;
  world->parsers = parser_factory;
  
}


/**
 * librdf_get_parser_factory:
 * @world: redland world object
 * @name: the name of the factory (NULL or empty string if don't care)
 * @mime_type: the MIME type of the syntax (NULL or empty if don't care)
 * @type_uri: URI of syntax (NULL if not used)
 *
 * Get a parser factory by name.
 *
 * If all fields are NULL, this means any parser supporting
 * MIME Type "application/rdf+xml"
 * 
 * Return value: the factory or NULL if not found
 **/
librdf_parser_factory*
librdf_get_parser_factory(librdf_world *world,
                          const char *name, const char *mime_type,
                          librdf_uri *type_uri) 
{
  librdf_parser_factory *factory;
  
  if(name && !*name)
    name=NULL;
  if(!mime_type || (mime_type && !*mime_type)) {
    if(!name && !type_uri)
      mime_type="application/rdf+xml";
    else
      mime_type=NULL;
  }

  /* return 1st parser if no particular one wanted */
  if(!name && !mime_type && !type_uri) {
    factory=world->parsers;
    if(!factory) {
      LIBRDF_DEBUG1("No parsers available\n");
      return NULL;
    }
  } else {
    for(factory=world->parsers; factory; factory=factory->next) {
      /* next if name does not match */
      if(name && strcmp(factory->name, name))
	continue;

      /* MIME type may need to match */
      if(mime_type) {
        if(!factory->mime_type)
          continue;
        if(strcmp(factory->mime_type, mime_type))
          continue;
      }
      
      /* URI may need to match */
      if(type_uri) {
        if(!factory->type_uri)
          continue;
        if(librdf_uri_equals(factory->type_uri, type_uri))
          continue;
      }

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
 * librdf_new_parser:
 * @world: redland world object
 * @name: the parser factory name
 * @mime_type: the MIME type of the syntax (NULL if not used)
 * @type_uri: URI of syntax (NULL if not used)
 *
 * Constructor - create a new #librdf_parser object.
 * 
 * If all fields are NULL, this means any parser supporting
 * MIME Type "application/rdf+xml"
 * 
 * Return value: new #librdf_parser object or NULL
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
 * librdf_new_parser_from_factory:
 * @world: redland world object
 * @factory: the parser factory to use to create this parser
 *
 * Constructor - create a new #librdf_parser object.
 * 
 * Return value: new #librdf_parser object or NULL
 **/
librdf_parser*
librdf_new_parser_from_factory(librdf_world *world, 
                               librdf_parser_factory *factory)
{
  librdf_parser* d;

  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(factory, librdf_parser_factory, NULL);

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
 * librdf_free_parser:
 * @parser: the parser
 *
 * Destructor - destroys a #librdf_parser object.
 * 
 **/
void
librdf_free_parser(librdf_parser *parser) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN(parser, librdf_parser);

  if(parser->context) {
    if(parser->factory->terminate)
      parser->factory->terminate(parser->context);
    LIBRDF_FREE(parser_context, parser->context);
  }
  LIBRDF_FREE(librdf_parser, parser);
}



/* methods */

/**
 * librdf_parser_parse_as_stream:
 * @parser: the parser
 * @uri: the URI to read
 * @base_uri: the base URI to use (or NULL if the same)
 *
 * Parse a URI to a librdf_stream of statements.
 * 
 * Return value: #librdf_stream of statements or NULL
 **/
librdf_stream*
librdf_parser_parse_as_stream(librdf_parser* parser, librdf_uri* uri,
                              librdf_uri* base_uri) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(parser, librdf_parser, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(uri, librdf_uri, NULL);

  if(parser->factory->parse_uri_as_stream)
    return parser->factory->parse_uri_as_stream(parser->context,
                                                uri, base_uri);

  if(!librdf_uri_is_file_uri(uri)) {
    LIBRDF_DEBUG2("%s parser can only handle file: URIs\n", parser->factory->name);
    return NULL;
  }
  return parser->factory->parse_file_as_stream(parser->context,
                                               uri, base_uri);
}


/**
 * librdf_parser_parse_into_model:
 * @parser: the parser
 * @uri: the URI to read the content
 * @base_uri: the base URI to use (or NULL if the same as @uri)
 * @model: the model to use
 *
 * Parse a URI of content into an librdf_model.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_parser_parse_into_model(librdf_parser* parser, librdf_uri* uri,
                               librdf_uri* base_uri, librdf_model* model) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(parser, librdf_parser, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(uri, librdf_uri, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);

  if(parser->factory->parse_uri_into_model)
    return parser->factory->parse_uri_into_model(parser->context,
                                                 uri, base_uri, model);
  
  if(!librdf_uri_is_file_uri(uri)) {
    LIBRDF_DEBUG2("%s parser can only handle file: URIs\n", parser->factory->name);
    return 1;
  }
  return parser->factory->parse_file_into_model(parser->context,
                                                uri, base_uri, model);
}


/**
 * librdf_parser_parse_string_as_stream:
 * @parser: the parser
 * @string: the string to parse
 * @base_uri: the base URI to use
 *
 * Parse a string of content to a librdf_stream of statements.
 * 
 * Return value: #librdf_stream of statements or NULL
 **/
librdf_stream*
librdf_parser_parse_string_as_stream(librdf_parser* parser, 
                                     const unsigned char *string,
                                     librdf_uri* base_uri) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(parser, librdf_parser, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(string, string, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(base_uri, librdf_uri, NULL);

  if(parser->factory->parse_string_as_stream)
    return parser->factory->parse_string_as_stream(parser->context,
                                                   string, base_uri);

  return NULL;
}


/**
 * librdf_parser_parse_string_into_model:
 * @parser: the parser
 * @string: the content to parse
 * @base_uri: the base URI to use
 * @model: the model to use
 *
 * Parse a string of content into an librdf_model.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_parser_parse_string_into_model(librdf_parser* parser, 
                                      const unsigned char *string,
                                      librdf_uri* base_uri, librdf_model* model) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(parser, librdf_parser, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(string, string, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(base_uri, librdf_uri, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);

  if(parser->factory->parse_string_into_model)
    return parser->factory->parse_string_into_model(parser->context,
                                                    string, base_uri, model);
  
  return 1;
}


/**
 * librdf_parser_parse_counted_string_as_stream:
 * @parser: the parser
 * @string: the string to parse
 * @length: length of the string content (must be >0)
 * @base_uri: the base URI to use
 *
 * Parse a counted string of content to a librdf_stream of statements.
 * 
 * Return value: #librdf_stream of statements or NULL
 **/
librdf_stream*
librdf_parser_parse_counted_string_as_stream(librdf_parser* parser, 
                                             const unsigned char *string,
                                             size_t length,
                                             librdf_uri* base_uri)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(parser, librdf_parser, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(string, string, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(base_uri, librdf_uri, NULL);
  LIBRDF_ASSERT_RETURN(length < 1, "string length is not greater than zero", 
                       NULL);

  if(parser->factory->parse_counted_string_as_stream)
    return parser->factory->parse_counted_string_as_stream(parser->context,
                                                           string, length,
                                                           base_uri);

  return NULL;
}


/**
 * librdf_parser_parse_counted_string_into_model:
 * @parser: the parser
 * @string: the content to parse
 * @length: length of content (must be >0)
 * @base_uri: the base URI to use
 * @model: the model to use
 *
 * Parse a counted string of content into an librdf_model.
 * 
 * Return value: non 0 on failure
 **/
int
librdf_parser_parse_counted_string_into_model(librdf_parser* parser, 
                                              const unsigned char *string,
                                              size_t length,
                                              librdf_uri* base_uri, 
                                              librdf_model* model)
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(parser, librdf_parser, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(string, string, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(base_uri, librdf_uri, 1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(model, librdf_model, 1);
  LIBRDF_ASSERT_RETURN(length < 1, "string length is not greater than zero", 1);

  if(parser->factory->parse_counted_string_into_model)
    return parser->factory->parse_counted_string_into_model(parser->context,
                                                            string, length,
                                                            base_uri, model);
  
  return 1;
}


/**
 * librdf_init_parser:
 * @world: redland world object
 *
 * INTERNAL - Initialise the librdf_parser class.
 *
 **/
void
librdf_init_parser(librdf_world *world) 
{
#ifdef HAVE_RAPTOR_RDF_PARSER
  librdf_parser_raptor_constructor(world);
#endif
}


/**
 * librdf_finish_parser:
 * @world: redland world object
 *
 * INTERNAL - Terminate the librdf_parser class.
 *
 **/
void
librdf_finish_parser(librdf_world *world) 
{
  librdf_delete_parser_factories(world);
#ifdef HAVE_RAPTOR_RDF_PARSER
  librdf_parser_raptor_destructor();
#endif
}


/**
 * librdf_parser_set_error:
 * @parser: the parser
 * @user_data: user data to pass to function
 * @error_fn: pointer to the function
 *
 * @Deprecated: Does nothing
 *
 * Set the parser error handling function.
 * 
 **/
void
librdf_parser_set_error(librdf_parser* parser, void *user_data,
                        void (*error_fn)(void *user_data, const char *msg, ...))
{
}


/**
 * librdf_parser_set_warning:
 * @parser: the parser
 * @user_data: user data to pass to function
 * @warning_fn: pointer to the function
 *
 * @Deprecated: Does nothing.
 *
 * Set the parser warning handling function.
 *
 **/
void
librdf_parser_set_warning(librdf_parser* parser, void *user_data,
                          void (*warning_fn)(void *user_data, const char *msg, ...))
{
}


/**
 * librdf_parser_get_feature:
 * @parser: #librdf_parser object
 * @feature: #librdf_Uuri feature property
 *
 * Get the value of a parser feature.
 * 
 * Return value: new #librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
librdf_node*
librdf_parser_get_feature(librdf_parser* parser, librdf_uri* feature) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(parser, librdf_parser, NULL);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(feature, librdf_uri, NULL);

  if(parser->factory->get_feature)
    return parser->factory->get_feature(parser->context, feature);

  return NULL;
}

/**
 * librdf_parser_set_feature:
 * @parser: #librdf_parser object
 * @feature: #librdf_uri feature property
 * @value: #librdf_node feature property value
 *
 * Set the value of a parser feature.
 * 
 * Return value: non 0 on failure (negative if no such feature)
 **/
  
int
librdf_parser_set_feature(librdf_parser* parser, librdf_uri* feature, 
                          librdf_node* value) 
{
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(parser, librdf_parser, -1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(feature, librdf_uri, -1);
  LIBRDF_ASSERT_OBJECT_POINTER_RETURN_VALUE(value, librdf_node, -1);

  if(parser->factory->set_feature)
    return parser->factory->set_feature(parser->context, feature, value);

  return (-1);
}

#endif


/* TEST CODE */


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


#define RDFXML_CONTENT \
"<?xml version=\"1.0\"?>\n" \
"<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n" \
"         xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n" \
"  <rdf:Description rdf:about=\"http://purl.org/net/dajobe/\">\n" \
"    <dc:title>Dave Beckett's Home Page</dc:title>\n" \
"    <dc:creator>Dave Beckett</dc:creator>\n" \
"    <dc:description>The generic home page of Dave Beckett.</dc:description>\n" \
"  </rdf:Description>\n" \
"</rdf:RDF>"

#define NTRIPLES_CONTENT \
"<http://purl.org/net/dajobe/> <http://purl.org/dc/elements/1.1/creator> \"Dave Beckett\" .\n" \
"<http://purl.org/net/dajobe/> <http://purl.org/dc/elements/1.1/description> \"The generic home page of Dave Beckett.\" .\n" \
"<http://purl.org/net/dajobe/> <http://purl.org/dc/elements/1.1/title> \"Dave Beckett's Home Page\" . \n"


#define TURTLE_CONTENT \
"@prefix dc: <http://purl.org/dc/elements/1.1/> .\n" \
"\n" \
"<http://purl.org/net/dajobe/>\n" \
"      dc:creator \"Dave Beckett\" ;\n" \
"      dc:description \"The generic home page of Dave Beckett.\" ; \n" \
"      dc:title \"Dave Beckett's Home Page\" . \n"

/* All the examples above give the same three triples */
#define EXPECTED_TRIPLES_COUNT 3


int
main(int argc, char *argv[]) 
{
  char *test_parser_types[]={"rdfxml", "ntriples", "turtle", NULL};
  #define URI_STRING_COUNT 3
  const unsigned char *file_uri_strings[URI_STRING_COUNT]={(const unsigned char*)"http://example.org/test1.rdf", (const unsigned char*)"http://example.org/test2.nt", (const unsigned char*)"http://example.org/test3.ttl"};
  const unsigned char *file_content[URI_STRING_COUNT]={(const unsigned char*)RDFXML_CONTENT, (const unsigned char*)NTRIPLES_CONTENT, (const unsigned char*)TURTLE_CONTENT};
  librdf_uri* uris[URI_STRING_COUNT];
  int i;
  char *type;
  char *program=argv[0];
  librdf_world *world;

  world=librdf_new_world();
  librdf_world_open(world);

  for (i=0; i<URI_STRING_COUNT; i++) {
    uris[i]=librdf_new_uri(world, file_uri_strings[i]);
  }

  for(i=0; (type=test_parser_types[i]); i++) {
    librdf_storage* storage;
    librdf_model *model;
    librdf_parser* parser;
    librdf_stream *stream=NULL;
    size_t length=strlen((const char*)file_content[i]);
    int size;

    fprintf(stderr, "%s: Testing parsing syntax '%s'\n", program, type);

    fprintf(stderr, "%s: Creating storage and model\n", program);
    storage=librdf_new_storage(world, NULL, NULL, NULL);
    if(!storage) {
      fprintf(stderr, "%s: Failed to create new storage\n", program);
      return(1);
    }
    model=librdf_new_model(world, storage, NULL);
    if(!model) {
      fprintf(stderr, "%s: Failed to create new model\n", program);
      return(1);
    }

    fprintf(stderr, "%s: Creating %s parser\n", program, type);
    parser=librdf_new_parser(world, type, NULL, NULL);
    if(!parser) {
      fprintf(stderr, "%s: Failed to create new parser type %s\n", program, type);
      continue;
    }
    

    fprintf(stderr, "%s: Adding %s counted string content as stream\n", 
            program, type);
    if(!(stream=librdf_parser_parse_counted_string_as_stream(parser, 
                                                             file_content[i],
                                                             length,
                                                             uris[i]))) {
      fprintf(stderr, "%s: Failed to parse RDF from counted string %d as stream\n", program, i);
      exit(1);
    }
    librdf_model_add_statements(model, stream);
    librdf_free_stream(stream);

    size=librdf_model_size(model);
    fprintf(stderr, "%s: Model size is %d triples\n", program, size);
    if(size != EXPECTED_TRIPLES_COUNT) {
      fprintf(stderr, "%s: Returned %d triples, not %d as expected\n",
              program, size, EXPECTED_TRIPLES_COUNT);
      exit(1);
    }

    fprintf(stderr, "%s: Adding %s string content as stream\n", program, type);
    if(!(stream=librdf_parser_parse_string_as_stream(parser, 
                                                     file_content[i],
                                                     uris[i]))) {
      fprintf(stderr, "%s: Failed to parse RDF from string %d as stream\n", program, i);
      exit(1);
    }
    librdf_model_add_statements(model, stream);
    librdf_free_stream(stream);
    
    size=librdf_model_size(model);
    fprintf(stderr, "%s: Model size is %d triples\n", program, size);
    if(size != EXPECTED_TRIPLES_COUNT) {
      fprintf(stderr, "%s: Returned %d triples, not %d as expected\n",
              program, size, EXPECTED_TRIPLES_COUNT);
      exit(1);
    }
    

    fprintf(stderr, "%s: Adding %s counted string content\n", program, type);
    if(librdf_parser_parse_counted_string_into_model(parser, 
                                                     file_content[i],
                                                     length,
                                                     uris[i], model)) {
      fprintf(stderr, "%s: Failed to parse RDF from counted string %d into model\n", program, i);
      exit(1);
    }
    
    size=librdf_model_size(model);
    fprintf(stderr, "%s: Model size is %d triples\n", program, size);
    if(size != EXPECTED_TRIPLES_COUNT) {
      fprintf(stderr, "%s: Returned %d triples, not %d as expected\n",
              program, size, EXPECTED_TRIPLES_COUNT);
      exit(1);
    }

    
    fprintf(stderr, "%s: Adding %s string content\n", program, type);
    if(librdf_parser_parse_string_into_model(parser, 
                                             file_content[i],
                                             uris[i], model)) {
      fprintf(stderr, "%s: Failed to parse RDF from string %d into model\n", program, i);
      exit(1);
    }

    size=librdf_model_size(model);
    fprintf(stderr, "%s: Model size is %d triples\n", program, size);
    if(size != EXPECTED_TRIPLES_COUNT) {
      fprintf(stderr, "%s: Returned %d triples, not %d as expected\n",
              program, size, EXPECTED_TRIPLES_COUNT);
      exit(1);
    }

    
    fprintf(stderr, "%s: Freeing parser, model and storage\n", program);
    librdf_free_parser(parser);
    librdf_free_model(model);
    librdf_free_storage(storage);
  }
  

  fprintf(stderr, "%s: Freeing URIs\n", program);
  for (i=0; i<URI_STRING_COUNT; i++) {
    librdf_free_uri(uris[i]);
  }
  
  librdf_free_world(world);
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
