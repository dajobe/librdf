/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser Factory implementation
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


#include <rdf_config.h>

#include <stdio.h>

#define LIBRDF_INTERNAL 1
#include <librdf.h>
#include <rdf_parser.h>


/* prototypes for helper functions */
static void librdf_delete_parser_factories(void);

/** List of parser factories */
static librdf_parser_factory *parsers=NULL;


/* helper functions */
static void
librdf_delete_parser_factories(void)
{
  librdf_parser_factory *factory, *next;
  
  for(factory=parsers; factory; factory=next) {
    next=factory->next;
    LIBRDF_FREE(librdf_parser_factory, factory->name);
    LIBRDF_FREE(librdf_parser_factory, factory);
  }
}


/**
 * librdf_parser_register_factory:
 * @name: the name of the hash
 * @factory: function to be called to register the factor parameters
 * 
 * Register a hash factory
 **/
void
librdf_parser_register_factory(const char *name,
			       void (*factory) (librdf_parser_factory*))
{
  librdf_parser_factory *d, *parser;
  char *name_copy;

  LIBRDF_DEBUG2(librdf_parser_register_factory,
		"Received registration for parser %s\n", name);
  
  parser=(librdf_parser_factory*)LIBRDF_CALLOC(librdf_parser_factory, 1,
					       sizeof(librdf_parser_factory));
  if(!parser)
    LIBRDF_FATAL1(librdf_parser_register_factory,
		  "Out of memory\n");
  
  name_copy=(char*)LIBRDF_CALLOC(cstring, 1, strlen(name)+1);
  if(!name_copy) {
    LIBRDF_FREE(librdf_parser, parser);
    LIBRDF_FATAL1(librdf_parser_register_factory, "Out of memory\n");
  }
  strcpy(name_copy, name);
  parser->name=name_copy;
        
  for(d = parsers; d; d = d->next ) {
    if(!strcmp(d->name, name_copy)) {
      LIBRDF_FATAL2(librdf_parser_register_factory,
		    "parser %s already registered\n", d->name);
    }
  }

  
        
  /* Call the parser registration function on the new object */
  (*factory)(parser);


  LIBRDF_DEBUG3(librdf_parser_register_factory,
		"%s has context size %d\n", name,
		parser->context_length);
  
  parser->next = parsers;
  parsers = parser;
  
}


/**
 * librdf_get_parser_factory:
 * @name: the name of the factor
 * 
 * get a parser factory
 * 
 * Return value: the factory or NULL if not found
 **/
librdf_parser_factory*
librdf_get_parser_factory(const char *name) 
{
  librdf_parser_factory *factory;

  /* return 1st parser if no particular one wanted - why? */
  if(!name) {
    factory=parsers;
    if(!factory) {
      LIBRDF_DEBUG1(librdf_get_parser_factory, "No parsers available\n");
      return NULL;
    }
  } else {
    for(factory=parsers; factory; factory=factory->next) {
      if(!strcmp(factory->name, name)) {
	break;
      }
    }
    /* else FACTORY with name parser_name not found */
    if(!factory) {
      LIBRDF_DEBUG2(librdf_get_parser_factory,
		    "No parser with name %s found\n", name);
      return NULL;
    }
  }
  
  return factory;
}


/**
 * librdf_new_parser:
 * @factory: the parser factory to use to create this parser
 * 
 * Constructor: create a new &librdf_parser object
 * 
 * Return value: new &librdf_parser object or NULL
 **/
librdf_parser*
librdf_new_parser(librdf_parser_factory *factory)
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
        
  d->factory=factory;

  factory->init(d->context);

  return d;
}


/**
 * librdf_free_parser:
 * @parser: the parser
 * 
 * Destructor: destroys a &librdf_parser object
 **/
void
librdf_free_parser(librdf_parser *parser) 
{
  if(parser->context)
    LIBRDF_FREE(parser_context, parser->context);
  LIBRDF_FREE(librdf_parser, parser);
}



/* methods */

/**
 * librdf_parser_parse_from_uri:
 * @parser: the parser
 * @uri: the URI to read
 * 
 * Parse a URI to a stream of statements
 * 
 * Return value: &librdf_stream of statements or NULL
 **/
librdf_stream*
librdf_parser_parse_from_uri(librdf_parser* parser, librdf_uri* uri) 
{
  return parser->factory->parse_from_uri(parser->context, uri);
}


/**
 * librdf_init_parser:
 * @void: 
 * 
 * Initialise the librdf_parser class
 **/
void
librdf_init_parser(void) 
{
#ifdef HAVE_SIRPAC_RDF_PARSER
  librdf_parser_sirpac_constructor();
#endif
#ifdef HAVE_LIBWWW_RDF_PARSER
  librdf_parser_libwww_constructor();
#endif
}


void
librdf_finish_parser(void) 
{
  librdf_delete_parser_factories();
}


#ifdef STANDALONE

/* one more prototype */
int main(int argc, char *argv[]);


int
main(int argc, char *argv[]) 
{
  librdf_parser_factory* factory;
  librdf_parser* d;
  char *test_parser_types[]={"SIRPAC", "LIBWWW", NULL};
  int i;
  char *type;
  char *program=argv[0];
        
  
  /* initialise parser module */
  librdf_init_parser();
  
  for(i=0; (type=test_parser_types[i]); i++) {
    fprintf(stderr, "%s: Trying to create new %s parser\n", program, type);
    factory=librdf_get_parser_factory(type);
    if(!factory) {
      fprintf(stderr, "%s: No parser factory called %s\n", program, type);
      continue;
    }
    
    d=librdf_new_parser(factory);
    if(!d) {
      fprintf(stderr, "%s: Failed to create new parser type %s\n", program, type);
      continue;
    }
    
    fprintf(stderr, "%s: Freeing parser\n", program);
    librdf_free_parser(d);
  }
  
  
  /* finish parser module */
  librdf_finish_parser();
  
#ifdef LIBRDF_DEBUG 
  librdf_memory_report(stderr);
#endif
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
