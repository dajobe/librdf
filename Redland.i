/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * Redland.i - SWIG interface file for Perl / Python interfaces to Redland
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

%module Redland
%{

#ifdef SWIGPERL
/* for perl, these are passed in by MakeMaker derived makefile */
#undef PACKAGE
#undef VERSION
#endif

#include <rdf_config.h>
#include <redland.h>


/* C world - link to external declarations in rdf_init.h */
extern const char * const redland_copyright_string;
extern const char * const redland_version_string;
extern const int redland_version_major;
extern const int redland_version_minor;
extern const int redland_version_release;

#ifdef SWIGPYTHON
/* swig doesn't declare all prototypes */
static PyObject *_wrap_redland_copyright_string_get(void);
static PyObject *_wrap_redland_version_string_get(void);

static PyObject *_wrap_redland_version_major_get(void);
static PyObject *_wrap_redland_version_minor_get(void);
static PyObject *_wrap_redland_version_release_get(void);

#endif

%}

typedef struct librdf_hash_s librdf_hash;
typedef struct librdf_uri_s librdf_uri;
typedef struct librdf_iterator_s librdf_iterator;
typedef struct librdf_node_s librdf_node;
typedef struct librdf_statement_s librdf_statement;
typedef struct librdf_model_s librdf_model;
typedef struct librdf_storage_s librdf_storage;
typedef struct librdf_stream_s librdf_stream;
typedef struct librdf_parser_s librdf_parser;

/* rdf_init.h */
void librdf_init_world(char *digest_factory_name, librdf_hash* uris_hash);
void librdf_destroy_world(void);

/* rdf_iterator.h */
void librdf_free_iterator(librdf_iterator*);
int librdf_iterator_have_elements(librdf_iterator* iterator);
librdf_node* librdf_iterator_get_next(librdf_iterator* iterator);

/* rdf_uri.h */
librdf_uri* librdf_new_uri (char *string);
librdf_uri* librdf_new_uri_from_uri (librdf_uri* uri);
void librdf_free_uri(librdf_uri *uri);
char* librdf_uri_to_string (librdf_uri* uri);

/* rdf_node.h */
librdf_node* librdf_new_node(void);
librdf_node* librdf_new_node_from_uri_string(char *string);
librdf_node* librdf_new_node_from_uri(librdf_uri *uri);
librdf_node* librdf_new_node_from_literal(char *string, char *xml_language, int xml_space, int is_wf_xml);
librdf_node* librdf_new_node_from_node(librdf_node *node);
void librdf_free_node(librdf_node *r);
librdf_uri* librdf_node_get_uri(librdf_node* node);
int librdf_node_set_uri(librdf_node* node, librdf_uri *uri);
int librdf_node_get_type(librdf_node* node);
void librdf_node_set_type(librdf_node* node, int type);
char* librdf_node_get_literal_value(librdf_node* node);
char* librdf_node_get_literal_value_language(librdf_node* node);
int librdf_node_get_literal_value_xml_space(librdf_node* node);
int librdf_node_get_literal_value_is_wf_xml(librdf_node* node);
int librdf_node_set_literal_value(librdf_node* node, char* value, char *xml_language, int xml_space, int is_wf_xml);
char *librdf_node_to_string(librdf_node* node);

/* rdf_statement.h */
librdf_statement* librdf_new_statement(void);
librdf_statement* librdf_new_statement_from_statement(librdf_statement* statement);
librdf_statement* librdf_new_statement_from_nodes(librdf_node* subject, librdf_node* predicate, librdf_node* object);
void librdf_free_statement(librdf_statement* statement);

librdf_node* librdf_statement_get_subject(librdf_statement *statement);
void librdf_statement_set_subject(librdf_statement *statement, librdf_node *subject);

librdf_node* librdf_statement_get_predicate(librdf_statement *statement);
void librdf_statement_set_predicate(librdf_statement *statement, librdf_node *predicate);
librdf_node* librdf_statement_get_object(librdf_statement *statement);
void librdf_statement_set_object(librdf_statement *statement, librdf_node *object);

char *librdf_statement_to_string(librdf_statement *statement);

/* rdf_model.h */
librdf_model* librdf_new_model(librdf_storage *storage, char* options_string);
librdf_model* librdf_new_model_with_options(librdf_storage *storage, librdf_hash* options);
librdf_model* librdf_new_model_from_model(librdf_model* model);
void librdf_free_model(librdf_model *model);
int librdf_model_size(librdf_model* model);
int librdf_model_add(librdf_model* model, librdf_node* subject, librdf_node* predicate, librdf_node* object);
int librdf_model_add_string_literal_statement(librdf_model* model, librdf_node* subject, librdf_node* predicate, char* string, char *xml_language, int xml_space, int is_wf_xml);
int librdf_model_add_statement(librdf_model* model, librdf_statement* statement);
int librdf_model_add_statements(librdf_model* model, librdf_stream* statement_stream);
int librdf_model_remove_statement(librdf_model* model, librdf_statement* statement);
int librdf_model_contains_statement(librdf_model* model, librdf_statement* statement);
librdf_stream* librdf_model_serialise(librdf_model* model);
librdf_stream* librdf_model_find_statements(librdf_model* model, librdf_statement* statement);
librdf_iterator* librdf_model_get_sources(librdf_model *model, librdf_node *arc, librdf_node *target);
librdf_iterator* librdf_model_get_arcs(librdf_model *model, librdf_node *source, librdf_node *target);
librdf_iterator* librdf_model_get_targets(librdf_model *model, librdf_node *source, librdf_node *arc);
librdf_node* librdf_model_get_source(librdf_model *model, librdf_node *arc, librdf_node *target);
librdf_node* librdf_model_get_arc(librdf_model *model, librdf_node *source, librdf_node *target);
librdf_node* librdf_model_get_target(librdf_model *model, librdf_node *source, librdf_node *arc);

/* rdf_storage.h */
librdf_storage* librdf_new_storage(char *storage_name, char *name, char *options_string);
librdf_storage* librdf_new_storage_from_storage (librdf_storage* old_storage);
void librdf_free_storage(librdf_storage *storage);

/* rdf_parser.h */
librdf_parser* librdf_new_parser(const char *name, const char *mime_type, librdf_uri *type_uri);
void librdf_free_parser(librdf_parser *parser);
librdf_stream* librdf_parser_parse_as_stream(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri);
int librdf_parser_parse_into_model(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri, librdf_model* model);
const char *librdf_parser_get_feature(librdf_parser* parser, librdf_uri *feature);
int librdf_parser_set_feature(librdf_parser* parser, librdf_uri *feature, const char *value);

/* rdf_stream.h */
void librdf_free_stream(librdf_stream* stream);
int librdf_stream_end(librdf_stream* stream);
librdf_statement* librdf_stream_next(librdf_stream* stream);


/* SWIG world - declare variables wanted from rdf_init.h */

%readonly
/* Note: most consts lost for SWIG to remain happy */
extern const char * redland_copyright_string;
extern const char * redland_version_string;
extern int redland_version_major;
extern int redland_version_minor;
extern int redland_version_release;
%readwrite
