/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.c - Redland RDF Parser implementation
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
#include <string.h>

#ifdef STANDALONE
#define LIBRDF_DEBUG 1
#endif

#define LIBRDF_INTERNAL 1
#include <librdf.h>

#include <rdf_parser.h>
#include <rdf_node.h>
#include <rdf_stream.h>
#include <rdf_statement.h>

#ifdef NEED_EXPAT
#include <xmlparse.h>
#endif

#ifdef NEED_LIBXML
#ifdef HAVE_GNOME_XML_PARSER_H
#include <gnome-xml/parser.h>
/* translate names from expat to libxml */
#define XML_Char xmlChar
#else
#include <parser.h>
#endif
#endif


/* namespace stack node */
struct ns_map_s {
  struct ns_map_s* next; /* next down the stack, NULL at bottom */
  char *prefix;
  char *uri;
  int depth;             /* parse depth that this was added, delete when parser leaves this */
};
typedef struct ns_map_s ns_map;
  

/*
 * Redland parser context
 */
typedef struct {
#ifdef NEED_EXPAT
  XML_Parser xp;
#endif
#ifdef NEED_LIBXML
  /* structure holding sax event handlers */
  xmlSAXHandler sax;
  /* parser context */
  /* xmlParserCtxtPtr xc; */
#endif  
  /* stack depth */
  int depth;
  /* stack of namespaces, most recently added at top */
  ns_map *namespaces;
} librdf_parser_redland_context;


/* Prototypes for common expat/libxml parsing event-handling functions */
static void librdf_parser_redland_start_element_handler(void *userData,
                                                        const XML_Char *name,
                                                        const XML_Char **atts);

static void librdf_parser_redland_end_element_handler(void *userData,
                                               const XML_Char *name);


/* s is not 0 terminated. */
static void librdf_parser_redland_cdata_handler(void *userData,
                                         const XML_Char *s,
                                         int len);
static void librdf_parser_redland_start_namespace_decl_handler(void *userData,
                                                        const XML_Char *prefix,
                                                        const XML_Char *uri);

static void librdf_parser_redland_end_namespace_decl_handler(void *userData,
                                                      const XML_Char *prefix);

/* libxml-only prototypes */
#ifdef NEED_LIBXML
static void librdf_parser_redland_warning(void *ctx, const char *msg, ...);
static void librdf_parser_redland_error(void *ctx, const char *msg, ...);
static void librdf_parser_redland_fatal_error(void *ctx, const char *msg, ...);
#endif


static void librdf_parser_redland_free(void *ctx);
  


/**
 * librdf_parser_redland_init - Initialise the Redland RDF parser
 * @context: context
 *
 * Return value: non 0 on failure
 **/
static int
librdf_parser_redland_init(void *context) 
{
  librdf_parser_redland_context* scontext=(librdf_parser_redland_context*)context;
#ifdef NEED_EXPAT
  XML_Parser xp;
  xp=XML_ParserCreate(NULL);

  /* create a new parser in the specified encoding */
  XML_SetUserData(xp, context);

  /* XML_SetEncoding(xp, "..."); */
  /* XML_SetBase(xp, ...); */

  XML_SetElementHandler(xp, librdf_parser_redland_start_element_handler,
                        librdf_parser_redland_end_element_handler);
  XML_SetCharacterDataHandler(xp, librdf_parser_redland_cdata_handler);
  XML_SetNamespaceDeclHandler(xp,
                              librdf_parser_redland_start_namespace_decl_handler,
                              librdf_parser_redland_end_namespace_decl_handler);
  scontext->xp=xp;
#endif
#ifdef NEED_LIBXML
  xmlDefaultSAXHandlerInit();
  scontext->sax.startElement=librdf_parser_redland_start_element_handler;
  scontext->sax.endElement=librdf_parser_redland_end_element_handler;

  scontext->sax.characters=librdf_parser_redland_cdata_handler;
  scontext->sax.ignorableWhitespace=librdf_parser_redland_cdata_handler;

  scontext->sax.warning=librdf_parser_redland_warning;
  scontext->sax.error=librdf_parser_redland_error;
  scontext->sax.fatalError=librdf_parser_redland_fatal_error;

  /* xmlInitParserCtxt(&scontext->xc); */
#endif

  scontext->depth=0;

  return 0;
}


static void librdf_parser_redland_start_element_handler(void *userData,
                                                        const XML_Char *name,
                                                        const XML_Char **atts)
{
  librdf_parser_redland_context* scontext=(librdf_parser_redland_context*)userData;
/*  XML_Parser xp=scontext->xp;*/

  scontext->depth++;
  
  fprintf(stderr, "redland: saw start element '%s'", name);
  if (atts != NULL) {
    int i;

    fputs(" attrs: ", stderr);

    for (i = 0;(atts[i] != NULL);i+=2) {
      /* synthesise the XML NS events */
      if(!strncmp(atts[i], "xmlns", 5)) {
        const char *prefix;
        int prefix_length;
        int uri_length;
        int len;
        ns_map *map;
        
        if(atts[i][5]) { /* there is more i.e. xmlns:foo */
          prefix=&atts[i][6];
          prefix_length=strlen(prefix);
        }
        else
          prefix=NULL;

#if 0
        librdf_parser_redland_start_namespace(userData, prefix, atts[i+1]);
#endif
        fprintf(stderr, "redland: synthesised start namespace prefix %s uri %s\n", prefix, atts[i+1]);

        uri_length=strlen(atts[i+1]);
        len=sizeof(ns_map) + uri_length+1;
        if(prefix_length)
          len+=prefix_length+1;

        /* Just one malloc */
        map=(ns_map*)LIBRDF_CALLOC(ns_map, len, 1);
        if(!map) {
          fprintf(stderr, "OUT OF MEMORY\n"); /* FIXME */
          abort();
        }
        
        map->uri=strcpy((char*)map+sizeof(ns_map), atts[i+1]);
        if(prefix)
          map->prefix=strcpy((char*)map+sizeof(ns_map)+uri_length+1, prefix);
        map->depth=scontext->depth;

        if(scontext->namespaces)
          map->next=scontext->namespaces;
        scontext->namespaces=map;

      }

      if(i)
        fputc(' ', stderr);
      fprintf(stderr, "%s='", atts[i]);
      fprintf(stderr, "%s'", atts[i+1]);
    }
  }
  fputc('\n', stderr);

}


static void
librdf_parser_redland_end_element_handler(void *userData,
                                          const XML_Char *name)
{
  librdf_parser_redland_context* scontext=(librdf_parser_redland_context*)userData;
/*  XML_Parser xp=scontext->xp;*/

  fprintf(stderr, "redland: saw end element '%s'\n", name);

  while(scontext->namespaces && scontext->namespaces->depth == scontext->depth) {
    ns_map* ns=scontext->namespaces;
    ns_map* next=ns->next;

#if 0
    librdf_parser_redland_end_namespace(userData, ns->prefix, ns->uri);
#endif
    fprintf(stderr, "redland: synthesised end namespace prefix %s uri \"%s\"\n", 
            ns->prefix ? ns->prefix : "(None)", ns->uri);

    LIBRDF_FREE(ns_map, ns);
    scontext->namespaces=next;
  }

  scontext->depth--;
}


/* cdata (and ignorable whitespace for libxml). 
 * s is not 0 terminated for expat, is for libxml - grrrr.
 */
static void
librdf_parser_redland_cdata_handler(void *userData, const XML_Char *s,
                                    int len)
{
  librdf_parser_redland_context* scontext=(librdf_parser_redland_context*)userData;
  /* XML_Parser xp=scontext->xp;*/
  fputs("redland: saw cdata: '", stderr);
  fwrite(s, 1, len, stderr);
  fputs("'\n", stderr);
}


static void
librdf_parser_redland_start_namespace_decl_handler(void *userData,
                                                   const XML_Char *prefix,
                                                   const XML_Char *uri)
{
  librdf_parser_redland_context* scontext=(librdf_parser_redland_context*)userData;
  /* XML_Parser xp=scontext->xp;*/

  fprintf(stderr, "redland: saw namespace %s URI %s\n", prefix, uri);
}


static void
librdf_parser_redland_end_namespace_decl_handler(void *userData,
                                                 const XML_Char *prefix)
{
  librdf_parser_redland_context* scontext=(librdf_parser_redland_context*)userData;
  /* XML_Parser xp=scontext->xp;*/

  fprintf(stderr, "redland: saw end namespace prefix %s\n", prefix);
}


#ifdef NEED_LIBXML
#include <stdarg.h>

static void
librdf_parser_redland_warning(void *ctx, const char *msg, ...) 
{
  va_list args;
  
  va_start(args, msg);
  fputs("libxml parser warning - ", stderr);
  vfprintf(stderr, msg, args);
  fputc('\n', stderr);
  va_end(args);
}

static void
librdf_parser_redland_error(void *ctx, const char *msg, ...) 
{
  va_list args;
  
  va_start(args, msg);
  fputs("libxml parser error - ", stderr);
  vfprintf(stderr, msg, args);
  fputc('\n', stderr);
  va_end(args);
}

static void
librdf_parser_redland_fatal_error(void *ctx, const char *msg, ...) 
{
  va_list args;
  
  va_start(args, msg);
  fputs("libxml parser fatal error - ", stderr);
  vfprintf(stderr, msg, args);
  fputc('\n', stderr);
  va_end(args);
  abort();
}

#endif

  


/**
 * librdf_parser_redland_parse_as_stream - Retrieve the RDF/XML content at URI and parse it into a librdf_stream
 * @context: serialisation context
 * @uri: URI of RDF content
 * 
 * Return value: a new &librdf_stream or NULL if the parse failed.
 **/
static librdf_stream*
librdf_parser_redland_parse_as_stream(void *context, librdf_uri *uri) {
  librdf_parser_redland_context* scontext=(librdf_parser_redland_context*)context;

  return NULL; /* FIXME: NOT IMPLEMENTED */
}

#ifdef NEED_LIBXML
/* from http://www.daa.com.au/~james/gnome/xml-sax/implementing.html */
#include <parserInternals.h>

static int myXmlSAXParseFile(xmlSAXHandlerPtr sax, void *user_data, const char *filename);

static int
myXmlSAXParseFile(xmlSAXHandlerPtr sax, void *user_data, const char *filename) {
  int ret = 0;
  xmlParserCtxtPtr ctxt;
  
  ctxt = xmlCreateFileParserCtxt(filename);
  if (ctxt == NULL) return -1;
  ctxt->sax = sax;
  ctxt->userData = user_data;
  
  xmlParseDocument(ctxt);
  
  if (ctxt->wellFormed)
    ret = 0;
  else
    ret = -1;
  if (sax != NULL)
    ctxt->sax = NULL;
  xmlFreeParserCtxt(ctxt);
  
  return ret;
}
#endif


/**
 * librdf_parser_redland_parse_into_model - Retrieve the RDF/XML content at URI and store in a librdf_model
 * @context: serialisation context
 * @uri: URI of RDF content
 * @model: &librdf_model
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_parser_redland_parse_into_model(void *context, librdf_uri *uri,
                                       librdf_model *model) {
  librdf_parser_redland_context* scontext=(librdf_parser_redland_context*)context;
  char *filename;
#ifdef NEED_EXPAT
  XML_Parser xp;
#endif
#ifdef NEED_LIBXML
  /* parser context */
  xmlParserCtxtPtr xc;
#endif
#define RBS 1024
  FILE *fh;
  char buffer[RBS];
  int rc=1;
  int len;
  
#ifdef NEED_EXPAT
  xp=scontext->xp;
#endif

  if(strncmp(librdf_uri_as_string(uri), "file:", 5)) {
    fprintf(stderr, "librdf_parser_redland_parse_into_model: parser cannot handle non file: URI %s\n",
            librdf_uri_as_string(uri));
    librdf_parser_redland_free(context);
    return 1;
  }
  

  filename=librdf_uri_as_string(uri)+5; /* FIXME - copy this */

  fh=fopen(filename, "r");
  if(!fh) {
    fprintf(stderr, "librdf_parser_redland_parse_into_model: failed to open file %s (URI %s) - ", 
            filename, librdf_uri_as_string(uri));
    perror(NULL);
    librdf_parser_redland_free(context);
    return 1;
  }
  
#ifdef NEED_LIBXML
  /* libxml needs at least 4 bytes from the XML content to allow
   * content encoding detection to work */
  len=fread(buffer, 1, 4, fh);
  if(len>0) {
    xc = xmlCreatePushParserCtxt(&scontext->sax, scontext,
                                 buffer, len, filename);
  } else {
    fclose(fh);
    fh=NULL;
  }
  
#endif

  while(fh && !feof(fh)) {
    len=fread(buffer, 1, RBS, fh);
    if(len <= 0) {
#ifdef NEED_EXPAT
      XML_Parse(xp, buffer, 0, 1);
#endif
#ifdef NEED_LIBXML
      xmlParseChunk(xc, buffer, 0, 1);
#endif
      break;
    }
#ifdef NEED_EXPAT
    rc=XML_Parse(xp, buffer, len, (len < RBS));
    if(len < RBS)
      break;
    if(!rc) /* expat: 0 is failure */
      break;
#endif
#ifdef NEED_LIBXML
    rc=xmlParseChunk(xc, buffer, len, 0);
    if(len < RBS)
      break;
    if(rc) /* libxml: non 0 is failure */
      break;
#endif
  }
  fclose(fh);

#ifdef NEED_EXPAT
  if(!rc) {
    int xe=XML_GetErrorCode(xp);
    
    fprintf(stderr, "%s:%d XML Parsing failed at column %d byte %ld - %s\n",
            filename, XML_GetCurrentLineNumber(xp),
            XML_GetCurrentColumnNumber(xp), XML_GetCurrentByteIndex(xp),
            XML_ErrorString(xe));
  }

  XML_ParserFree(xp);
#endif /* EXPAT */

  librdf_parser_redland_free(context);

  return (rc != 0);
}


static void
librdf_parser_redland_free(void *context) 
{
  librdf_parser_redland_context* scontext=(librdf_parser_redland_context*)context;

  fprintf(stderr, "Entering librdf_parser_redland_free\n");
  
  while(scontext->namespaces) {
    ns_map* next_map=scontext->namespaces->next;

    LIBRDF_FREE(ns_map, scontext->namespaces);
    scontext->namespaces=next_map;
  }
}



/**
 * librdf_parser_redland_register_factory - Register the Redland RDF parser with the RDF parser factory
 * @factory: prototype rdf parser factory
 **/
static void
librdf_parser_redland_register_factory(librdf_parser_factory *factory) 
{
  factory->context_length = sizeof(librdf_parser_redland_context);
  
  factory->init  = librdf_parser_redland_init;
  factory->parse_as_stream = librdf_parser_redland_parse_as_stream;
  factory->parse_into_model = librdf_parser_redland_parse_into_model;
}


/**
 * librdf_parser_redland_constructor - Initialise the Redland RDF parser module
 **/
void
librdf_parser_redland_constructor(void)
{
  librdf_parser_register_factory("redland", &librdf_parser_redland_register_factory);
}
