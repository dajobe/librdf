/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_parser.h - RDF Parser Factory / Parser interfaces and definition
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
  librdf_stream* (*parse_as_stream)(void *c, librdf_uri *uri);

  int (*parse_into_model)(void *c, librdf_uri *uri, librdf_model *model);
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
librdf_parser* librdf_new_parser(char *name);
librdf_parser* librdf_new_parser_from_factory(librdf_parser_factory *factory);

/* destructor */
void librdf_free_parser(librdf_parser *parser);


/* methods */
librdf_stream* librdf_parser_parse_as_stream(librdf_parser* parser, librdf_uri* uri);
int librdf_parser_parse_into_model(librdf_parser* parser, librdf_uri* uri, librdf_model* model);


/* in librdf_parser_sirpac.c */
#ifdef HAVE_SIRPAC_RDF_PARSER
void librdf_parser_sirpac_constructor(void);
#endif
#ifdef HAVE_LIBWWW_RDF_PARSER
void librdf_parser_libwww_constructor(void);
#endif
#ifdef HAVE_REDLAND_RDF_PARSER
void librdf_parser_redland_constructor(void);
#endif
#ifdef HAVE_REPAT_RDF_PARSER
void librdf_parser_repat_constructor(void);
#endif


#ifdef __cplusplus
}
#endif

#endif
