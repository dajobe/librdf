/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * Redland.i - SWIG interface file for Perl / Python interfaces to Redland
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

%module Redland
%include typemaps.i
%{

#ifdef SWIGPERL
/* for perl, these are passed in by MakeMaker derived makefile */
#undef PACKAGE
#undef VERSION
#endif

/* SWIG BUG - no SWIGTCL is defined - duh */
#ifdef TCL_MAJOR_VERSION
  /* want symbols starting librdf_ not _librdf_ */
#undef SWIG_prefix
#define SWIG_prefix
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


/* Turning a native file handle to a C/stdio FILE* */

#ifdef SWIGPERL_NOT_USED
%typemap(in) FILE * {
  /* http://archive.develooper.com/perl5-porters@perl.org/msg78879.html */
  // $1 = stderr;
  //char buf[8];
  // $1 = fdopen(PerlIO_fileno($input), PerlIO_modestr($input,buf));

  /* see perldoc perlapio */
  fprintf(stderr, "input is %d\n", $input);
  $1 = PerlIO_exportFILE($input, 0);
}
#endif

#ifdef SWIGPYTHON_NOT_USED
%typemap(in) FILE * {
  if (!PyFile_Check($source)) {
    PyErr_SetString(PyExc_TypeError, "Need a file!");
    return NULL;
  }
  $1 = PyFile_AsFile($input);
}
#endif


%init %{
#ifdef TCL_MAJOR_VERSION
Tcl_PkgProvide(interp, PACKAGE, (char*)redland_version_string);
#endif
%}
  

typedef struct librdf_world_s librdf_world;
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
librdf_world* librdf_new_world(void);
void librdf_free_world(librdf_world *world);
void librdf_world_open(librdf_world *world);

/* OLD: */
void librdf_init_world(char *digest_factory_name, librdf_hash* uris_hash);
void librdf_destroy_world(void);

/* rdf_iterator.h */
void librdf_free_iterator(librdf_iterator*);
int librdf_iterator_have_elements(librdf_iterator* iterator);
int librdf_iterator_end(librdf_iterator* iterator);
librdf_node* librdf_iterator_get_next(librdf_iterator* iterator);

/* rdf_uri.h */
librdf_uri* librdf_new_uri (librdf_world *world, char *string);
librdf_uri* librdf_new_uri_from_uri (librdf_uri* uri);
void librdf_free_uri(librdf_uri *uri);
char* librdf_uri_to_string (librdf_uri* uri);
int librdf_uri_equals(librdf_uri* first_uri, librdf_uri* second_uri);

/* rdf_node.h */
librdf_node* librdf_new_node(librdf_world *world);
librdf_node* librdf_new_node_from_uri_string(librdf_world *world, char *string);
librdf_node* librdf_new_node_from_uri(librdf_world *world, librdf_uri *uri);
librdf_node* librdf_new_node_from_literal(librdf_world *world, char *string, char *xml_language, int xml_space, int is_wf_xml);
librdf_node* librdf_new_node_from_node(librdf_node *node);
librdf_node* librdf_new_node_from_blank_identifier(librdf_world *world, const char *identifier);
void librdf_free_node(librdf_node *r);
librdf_uri* librdf_node_get_uri(librdf_node* node);
int librdf_node_set_uri(librdf_node* node, librdf_uri *uri);
int librdf_node_get_type(librdf_node* node);
void librdf_node_set_type(librdf_node* node, int type);
char* librdf_node_get_literal_value(librdf_node* node);
char* librdf_node_get_literal_value_as_latin1(librdf_node* node);
char* librdf_node_get_literal_value_language(librdf_node* node);
int librdf_node_get_literal_value_xml_space(librdf_node* node);
int librdf_node_get_literal_value_is_wf_xml(librdf_node* node);
int librdf_node_set_literal_value(librdf_node* node, char* value, char *xml_language, int xml_space, int is_wf_xml);
char *librdf_node_to_string(librdf_node* node);
int librdf_node_equals(librdf_node* first_node, librdf_node* second_node);

/* rdf_statement.h */
librdf_statement* librdf_new_statement(librdf_world *world);
librdf_statement* librdf_new_statement_from_statement(librdf_statement* statement);
librdf_statement* librdf_new_statement_from_nodes(librdf_world *world, librdf_node* subject, librdf_node* predicate, librdf_node* object);
void librdf_free_statement(librdf_statement* statement);

librdf_node* librdf_statement_get_subject(librdf_statement *statement);
void librdf_statement_set_subject(librdf_statement *statement, librdf_node *subject);

librdf_node* librdf_statement_get_predicate(librdf_statement *statement);
void librdf_statement_set_predicate(librdf_statement *statement, librdf_node *predicate);
librdf_node* librdf_statement_get_object(librdf_statement *statement);
void librdf_statement_set_object(librdf_statement *statement, librdf_node *object);

char *librdf_statement_to_string(librdf_statement *statement);

/* rdf_model.h */
librdf_model* librdf_new_model(librdf_world *world, librdf_storage *storage, char* options_string);
librdf_model* librdf_new_model_with_options(librdf_world *world, librdf_storage *storage, librdf_hash* options);
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
librdf_storage* librdf_new_storage(librdf_world *world, char *storage_name, char *name, char *options_string);
librdf_storage* librdf_new_storage_from_storage (librdf_storage* old_storage);
void librdf_free_storage(librdf_storage *storage);

/* rdf_parser.h */
librdf_parser* librdf_new_parser(librdf_world *world, const char *name, const char *mime_type, librdf_uri *type_uri);
void librdf_free_parser(librdf_parser *parser);
librdf_stream* librdf_parser_parse_as_stream(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri);
int librdf_parser_parse_into_model(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri, librdf_model* model);
const char *librdf_parser_get_feature(librdf_parser* parser, librdf_uri *feature);
int librdf_parser_set_feature(librdf_parser* parser, librdf_uri *feature, const char *value);

/* rdf_serializer.h */
librdf_serializer* librdf_new_serializer(librdf_world* world, const char *name, const char *mime_type, librdf_uri *type_uri);
void librdf_free_serializer(librdf_serializer *serializer);
int librdf_serializer_serialize_model(librdf_serializer* serializer, FILE *handle, librdf_uri* base_uri, librdf_model* model);
const char *librdf_serializer_get_feature(librdf_serializer* serializer, librdf_uri *feature);
int librdf_serializer_set_feature(librdf_serializer* serializer, librdf_uri *feature, const char *value);

/* rdf_stream.h */
void librdf_free_stream(librdf_stream* stream);
int librdf_stream_end(librdf_stream* stream);
librdf_statement* librdf_stream_next(librdf_stream* stream);

/* SWIG world - declare variables wanted from rdf_init.h */

%immutable;
/* Note: most consts lost for SWIG to remain happy */
extern const char * redland_copyright_string;
extern const char * redland_version_string;
extern int redland_version_major;
extern int redland_version_minor;
extern int redland_version_release;
%mutable;
