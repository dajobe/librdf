/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.h - RDF Parser Factory / Parser interfaces and definition
 *
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 *                                       
 * This program is free software distributed under either of these licenses:
 *   1. The GNU Lesser General Public License (LGPL)
 * OR ALTERNATIVELY
 *   2. The modified BSD license
 *
 * See LICENSE.html or LICENSE.txt for the full license terms.
 */



#ifndef LIBRDF_PARSER_H
#define LIBRDF_PARSER_H


#ifdef __cplusplus
extern "C" {
#endif


struct librdf_parser_factory_s 
{
  struct librdf_parser_factory_s* next;
  char *   name;

  /* the rest of this structure is populated by the
     parser-specific register function */
  size_t  context_length;

  int (*init)(void *c);
  librdf_stream* (*parse_from_uri)(void *c, librdf_uri *uri);
};
typedef struct librdf_parser_factory_s librdf_parser_factory;


struct librdf_parser_s {
  void *context;
  librdf_parser_factory* factory;
};


/* factory static methods */
void librdf_parser_register_factory(const char *name, void (*factory) (librdf_parser_factory*));

librdf_parser_factory* librdf_get_parser_factory(const char *name);


/* module init */
void librdf_init_parser(void);
/* module finish */
void librdf_finish_parser(void);
                    

/* constructor */
librdf_parser* librdf_new_parser(librdf_parser_factory *factory);

/* destructor */
void librdf_free_parser(librdf_parser *parser);


/* methods */
librdf_stream* librdf_parser_parse_from_uri(librdf_parser* parser, librdf_uri* uri);


/* in librdf_parser_sirpac.c */
#ifdef HAVE_SIRPAC_RDF_PARSER
void librdf_parser_sirpac_constructor(void);
#endif
#ifdef HAVE_LIBWWW_RDF_PARSER
void librdf_parser_libwww_constructor(void);
#endif


#ifdef __cplusplus
}
#endif

#endif
