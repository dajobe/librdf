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


struct librdf_parser_factory_s 
{
  struct librdf_parser_factory_s* next;
  /* factory name - required */
  char *name;
  /* handle this MIME type/ Internet Media Type - optional */
  char *mime_type;
  /* handles the syntax defined by this URI - optional */
  librdf_uri *type_uri;

  /* the rest of this structure is populated by the
     parser-specific register function */
  size_t  context_length;

  /* create a new parser */
  int (*init)(librdf_parser* parser, void *c);

  /* destroy a parser */
  void (*terminate)(void *c);

  /* get/set features of parser (think of Java properties) */
  const char * (*get_feature)(void *c, librdf_uri *feature);
  int (*set_feature)(void *c, librdf_uri *feature, const char *value);
  
  librdf_stream* (*parse_uri_as_stream)(void *c, librdf_uri *uri, librdf_uri* base_uri);
  int (*parse_uri_into_model)(void *c, librdf_uri *uri, librdf_uri* base_uri, librdf_model *model);
  librdf_stream* (*parse_file_as_stream)(void *c, librdf_uri *uri, librdf_uri *base_uri);
  int (*parse_file_into_model)(void *c, librdf_uri *uri, librdf_uri *base_uri, librdf_model *model);
};


struct librdf_parser_s {
  void *context;

  void *error_user_data;
  void *warning_user_data;
  void (*error_fn)(void *user_data, const char *msg, ...);
  void (*warning_fn)(void *user_data, const char *msg, ...);

  librdf_parser_factory* factory;
};


/* factory static methods */
void librdf_parser_register_factory(librdf_world *world, const char *name, const char *mime_type, const char *uri_string, void (*factory) (librdf_parser_factory*));

librdf_parser_factory* librdf_get_parser_factory(librdf_world *world, const char *name, const char *mime_type, librdf_uri *type_uri);


/* module init */
void librdf_init_parser(librdf_world *world);
/* module finish */
void librdf_finish_parser(librdf_world *world);
                    

/* constructor */
librdf_parser* librdf_new_parser(const char *name, const char *mime_type, librdf_uri *type_uri);
librdf_parser* librdf_new_parser_from_factory(librdf_parser_factory *factory);

/* destructor */
void librdf_free_parser(librdf_parser *parser);


/* methods */
librdf_stream* librdf_parser_parse_as_stream(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri);
int librdf_parser_parse_into_model(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri, librdf_model* model);
void librdf_parser_set_error(librdf_parser* parser, void *user_data, void (*error_fn)(void *user_data, const char *msg, ...));
void librdf_parser_set_warning(librdf_parser* parser, void *user_data, void (*warning_fn)(void *user_data, const char *msg, ...));

const char *librdf_parser_get_feature(librdf_parser* parser, librdf_uri *feature);
int librdf_parser_set_feature(librdf_parser* parser, librdf_uri *feature, const char *value);

/* internal callbacks used by parsers invoking errors/warnings upwards to user */
void librdf_parser_error(librdf_parser* parser, const char *message, ...);
void librdf_parser_warning(librdf_parser* parser, const char *message, ...);

/* in librdf_parser_sirpac.c */
#ifdef HAVE_SIRPAC_RDF_PARSER
void librdf_parser_sirpac_constructor(librdf_world* world);
#endif
#ifdef HAVE_LIBWWW_RDF_PARSER
void librdf_parser_libwww_constructor(librdf_world* world);
#endif
#ifdef HAVE_RAPIER_RDF_PARSER
void librdf_parser_rapier_constructor(librdf_world* world);
#endif
#ifdef HAVE_REPAT_RDF_PARSER
void librdf_parser_repat_constructor(librdf_world* world);
#endif


#ifdef __cplusplus
}
#endif

#endif
