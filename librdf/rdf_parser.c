/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - RDF Parser Factory implementation
 *
 * $Id$
 *
 * Copyright (C) 2000 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology, University of Bristol.
 *
 *    This package is Free Software available under either of two licenses
 *    (see FAQS.html to see why):
 * 
 * 1. The GNU Lesser General Public License (LGPL)
 * 
 *    See http://www.gnu.org/copyleft/lesser.html or COPYING.LIB for the
 *    full license text.
 *      _________________________________________________________________
 * 
 *      Copyright (C) 2000 David Beckett, Institute for Learning and
 *      Research Technology, University of Bristol. All Rights Reserved.
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public License
 *      as published by the Free Software Foundation; either version 2 of
 *      the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *      USA
 *      _________________________________________________________________
 * 
 *    NOTE - under Term 3 of the LGPL, you may choose to license the entire
 *    library under the GPL. See COPYING for the full license text.
 * 
 * 2. The Mozilla Public License
 * 
 *    See http://www.mozilla.org/MPL/MPL-1.1.html or MPL.html for the full
 *    license text.
 * 
 *    Under MPL section 13. I declare that all of the Covered Code is
 *    Multiple Licensed:
 *      _________________________________________________________________
 * 
 *      The contents of this file are subject to the Mozilla Public License
 *      version 1.1 (the "License"); you may not use this file except in
 *      compliance with the License. You may obtain a copy of the License
 *      at http://www.mozilla.org/MPL/
 * 
 *      Software distributed under the License is distributed on an "AS IS"
 *      basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 *      the License for the specific language governing rights and
 *      limitations under the License.
 * 
 *      The Initial Developer of the Original Code is David Beckett.
 *      Portions created by David Beckett are Copyright (C) 2000 David
 *      Beckett, Institute for Learning and Research Technology, University
 *      of Bristol. All Rights Reserved.
 * 
 *      Alternatively, the contents of this file may be used under the
 *      terms of the GNU Lesser General Public License, in which case the
 *      provisions of the LGPL License are applicable instead of those
 *      above. If you wish to allow use of your version of this file only
 *      under the terms of the LGPL License and not to allow others to use
 *      your version of this file under the MPL, indicate your decision by
 *      deleting the provisions above and replace them with the notice and
 *      other provisions required by the LGPL License. If you do not delete
 *      the provisions above, a recipient may use your version of this file
 *      under either the MPL or the LGPL License.
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
 * librdf_parser_register_factory - Register a parser factory
 * @name: the name of the parser
 * @factory: function to be called to register the factor parameters
 * 
 **/
void
librdf_parser_register_factory(const char *name,
			       void (*factory) (librdf_parser_factory*))
{
  librdf_parser_factory *d, *parser;
  char *name_copy;

#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG2(librdf_parser_register_factory,
		"Received registration for parser %s\n", name);
#endif
  
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


#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  LIBRDF_DEBUG3(librdf_parser_register_factory,
		"%s has context size %d\n", name,
		parser->context_length);
#endif
  
  parser->next = parsers;
  parsers = parser;
  
}


/**
 * librdf_get_parser_factory - Get a parser factory by name
 * @name: the name of the factory
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
 * librdf_new_parser - Constructor - create a new librdf_parser object
 * @name: the parser factory name
 * 
 * Return value: new &librdf_parser object or NULL
 **/
librdf_parser*
librdf_new_parser(char *name)
{
  librdf_parser_factory* factory;

  factory=librdf_get_parser_factory(name);
  if(!factory)
    return NULL;

  return librdf_new_parser_from_factory(factory);
}


/**
 * librdf_new_parser_from_factory - Constructor - create a new librdf_parser object
 * @factory: the parser factory to use to create this parser
 * 
 * Return value: new &librdf_parser object or NULL
 **/
librdf_parser*
librdf_new_parser_from_factory(librdf_parser_factory *factory)
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
 * librdf_free_parser - Destructor - destroys a librdf_parser object
 * @parser: the parser
 * 
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
 * librdf_parser_parse_as_stream - Parse a URI to a librdf_stream of statements
 * @parser: the parser
 * @uri: the URI to read
 * 
 * Return value: &librdf_stream of statements or NULL
 **/
librdf_stream*
librdf_parser_parse_as_stream(librdf_parser* parser, librdf_uri* uri) 
{
  return parser->factory->parse_as_stream(parser->context, uri);
}


/**
 * librdf_parser_parse_into_model - Parse a URI of RDF/XML content into an librdf_model
 * @parser: the parser
 * @uri: the URI to read
 * @model: the model to use
 * 
 * Return value: non 0 on failure
 **/
int
librdf_parser_parse_into_model(librdf_parser* parser, librdf_uri* uri,
                               librdf_model* model) 
{
  if(!parser->factory->parse_into_model)
    return 1;
  
  return parser->factory->parse_into_model(parser->context, uri, model);
}


/**
 * librdf_init_parser - Initialise the librdf_parser class
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


/**
 * librdf_finish_parser - Terminate the librdf_parser class
 **/
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
  librdf_parser* d;
  char *test_parser_types[]={"sirpac", "libwww", NULL};
  int i;
  char *type;
  char *program=argv[0];
        
  
  /* initialise parser module */
  librdf_init_parser();
  
  for(i=0; (type=test_parser_types[i]); i++) {
    fprintf(stderr, "%s: Trying to create new %s parser\n", program, type);
    d=librdf_new_parser(type);
    if(!d) {
      fprintf(stderr, "%s: Failed to create new parser type %s\n", program, type);
      continue;
    }
    
    fprintf(stderr, "%s: Freeing parser\n", program);
    librdf_free_parser(d);
  }
  
  
  /* finish parser module */
  librdf_finish_parser();
  
#ifdef LIBRDF_MEMORY_DEBUG 
  librdf_memory_report(stderr);
#endif
  
  /* keep gcc -Wall happy */
  return(0);
}

#endif
