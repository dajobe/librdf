/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.h - RDF Parser Factory / Parser interfaces and definition
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



#ifndef LIBRDF_PARSER_H
#define LIBRDF_PARSER_H


#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBRDF_INTERNAL

struct librdf_parser_factory_s 
{
  struct librdf_parser_factory_s* next;
  /* syntax name - required */
  char *name;
  /* handle this MIME type/ Internet Media Type - optional */
  char *mime_type;
  /* handles the syntax defined by this URI - optional */
  librdf_uri *type_uri;

  /* the rest of this structure is populated by the
     parser-specific register function */
  size_t  context_length;

  /* create a new parser */
  int (*init)(librdf_parser* parser, void *_context);

  /* destroy a parser */
  void (*terminate)(void *_context);

  /* get/set features of parser (think of Java properties) */
  librdf_node* (*get_feature)(void *_context, librdf_uri *feature);
  int (*set_feature)(void *_context, librdf_uri *feature, librdf_node *value);
  
  librdf_stream* (*parse_uri_as_stream)(void *_context, librdf_uri *uri, librdf_uri* base_uri);
  int (*parse_uri_into_model)(void *_context, librdf_uri *uri, librdf_uri* base_uri, librdf_model *model);
  librdf_stream* (*parse_file_as_stream)(void *_context, librdf_uri *uri, librdf_uri *base_uri);
  int (*parse_file_into_model)(void *_context, librdf_uri *uri, librdf_uri *base_uri, librdf_model *model);
  int (*parse_string_into_model)(void *_context, const unsigned char *string, librdf_uri* base_uri, librdf_model *model);
  librdf_stream* (*parse_string_as_stream)(void *_context, const unsigned char *string, librdf_uri *base_uri);
};


struct librdf_parser_s {
  librdf_world *world;
  
  void *context;

  librdf_parser_factory* factory;
};

/* class methods */
librdf_parser_factory* librdf_get_parser_factory(librdf_world *world, const char *name, const char *mime_type, librdf_uri *type_uri);


/* module init */
void librdf_init_parser(librdf_world *world);
/* module finish */
void librdf_finish_parser(librdf_world *world);
                    
#ifdef HAVE_RAPTOR_RDF_PARSER
void librdf_parser_raptor_constructor(librdf_world* world);
void librdf_parser_raptor_destructor(void);
#endif


#endif


/* class methods */
REDLAND_API void librdf_parser_register_factory(librdf_world *world, const char *name, const char *mime_type, const unsigned char *uri_string, void (*factory) (librdf_parser_factory*));

/* constructor */
REDLAND_API librdf_parser* librdf_new_parser(librdf_world* world, const char *name, const char *mime_type, librdf_uri *type_uri);
REDLAND_API librdf_parser* librdf_new_parser_from_factory(librdf_world* world, librdf_parser_factory *factory);

/* destructor */
REDLAND_API void librdf_free_parser(librdf_parser *parser);


/* methods */
REDLAND_API librdf_stream* librdf_parser_parse_as_stream(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri);
REDLAND_API int librdf_parser_parse_into_model(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri, librdf_model* model);
REDLAND_API librdf_stream* librdf_parser_parse_string_as_stream(librdf_parser* parser, const unsigned char* string, librdf_uri* base_uri);
REDLAND_API int librdf_parser_parse_string_into_model(librdf_parser* parser, const unsigned char *string, librdf_uri* base_uri, librdf_model* model);
REDLAND_API REDLAND_DEPRECATED void librdf_parser_set_error(librdf_parser* parser, void *user_data, void (*error_fn)(void *user_data, const char *msg, ...));
REDLAND_API REDLAND_DEPRECATED void librdf_parser_set_warning(librdf_parser* parser, void *user_data, void (*warning_fn)(void *user_data, const char *msg, ...));

#define LIBRDF_PARSER_FEATURE_ERROR_COUNT "http://feature.librdf.org/parser-error-count"
#define LIBRDF_PARSER_FEATURE_WARNING_COUNT "http://feature.librdf.org/parser-warning-count"

REDLAND_API librdf_node* librdf_parser_get_feature(librdf_parser* parser, librdf_uri *feature);
REDLAND_API int librdf_parser_set_feature(librdf_parser* parser, librdf_uri* feature, librdf_node* value);

#ifdef __cplusplus
}
#endif

#endif
