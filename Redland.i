/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * Redland.i - SWIG interface file for interfaces to Redland
 *
 * $Id$
 *
 * Copyright (C) 2000-2004 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.bris.ac.uk/
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

/* Delete one of many names polluted by Perl in embed.h via perl.h */
#ifdef list
#undef list
#endif
#endif

/* SWIG BUG - no SWIGTCL is defined - duh */
#ifdef TCL_MAJOR_VERSION
  /* want symbols starting librdf_ not _librdf_ */
#undef SWIG_prefix
#define SWIG_prefix
#endif

#if defined(SWIGRUBY) || defined (PHP_VERSION)
/* Ruby and PHP pollute the #define space with these names */
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef PACKAGE_BUGREPORT
#endif

#include <rdf_config.h>
#include <redland.h>

/* Internal prototypes */
/* FOR TESTING ERRORS ONLY - NOT PART OF API */
void librdf_internal_test_error(librdf_world *world);
void librdf_internal_test_warning(librdf_world *world);

#ifdef SWIGPYTHON
void librdf_python_world_init(librdf_world *world);
#endif
#ifdef SWIGPERL
void librdf_perl_world_init(librdf_world *world);
void librdf_perl_world_finish(void);
#endif
#ifdef SWIGPHP
librdf_world* librdf_php_get_world(void);
void librdf_php_world_finish(void);
#endif


/* 
 * Thanks to the patch in this Debian bug for the solution
 * to the crash inside vsnprintf on some architectures.
 *
 * "reuse of args inside the while(1) loop is in violation of the
 * specs and only happens to work by accident on other systems."
 *
 * http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=104325 
 */

#ifndef va_copy
#ifdef __va_copy
#define va_copy(dest,src) __va_copy(dest,src)
#else
#define va_copy(dest,src) (dest) = (src)
#endif
#endif



#ifdef SWIGPYTHON
/* swig doesn't seem to declare prototypes of get accessors for statics */
static PyObject *_wrap_librdf_copyright_string_get(void);
static PyObject *_wrap_librdf_version_string_get(void);
static PyObject *_wrap_librdf_short_copyright_string_get(void);
static PyObject *_wrap_librdf_version_decimal_get(void);

static PyObject *_wrap_librdf_version_major_get(void);
static PyObject *_wrap_librdf_version_minor_get(void);
static PyObject *_wrap_librdf_version_release_get(void);

SWIGEXPORT(void) SWIG_init(void);

static PyObject *librdf_python_callback = NULL;

static PyObject * librdf_python_set_callback(PyObject *dummy, PyObject *args);

/*
 * set the Python function object callback
 */
static PyObject *
librdf_python_set_callback(dummy, args)
  PyObject *dummy, *args;
{
  PyObject *result = NULL;
  PyObject *temp;
  
  if (PyArg_ParseTuple(args, "O:set_callback", &temp)) {
    if (!PyCallable_Check(temp)) {
      PyErr_SetString(PyExc_TypeError, "parameter must be callable");
      return NULL;
    }
    Py_XINCREF(temp);         /* Add a reference to new callback */
    Py_XDECREF(librdf_python_callback);  /* Dispose of previous callback */
    librdf_python_callback = temp;       /* Remember new callback */
    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    result = Py_None;
  }
  return result;
}


/* Declare a table of methods that python can call */
static PyMethodDef librdf_python_methods [] = {
    {"set_callback",  librdf_python_set_callback, METH_VARARGS,
     "Set python message callback."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


/*
 * calls a python function defined as:
 *   RDF.message($$)
 * where first argument is an integer, second is a (scalar) string
 * with an integer return value indicating if the message was handled.
 */
static int
librdf_call_python_message(int type, const char *message, va_list arguments)
{
  char empty_buffer[1];
  PyObject *arglist;
  PyObject *result;
  char *buffer;
  int len;
  va_list args_copy;
  int rc=0;

  if(!librdf_python_callback)
    return 0;

  /* ask vsnprintf size of buffer required */
  va_copy(args_copy, arguments);
  len=vsnprintf(empty_buffer, 1, message, args_copy)+1;
  va_end(args_copy);
  buffer=(char*)malloc(len);
  if(!buffer) {
    fprintf(stderr, "librdf_call_python_message: Out of memory\n");
    return 0;
  }
  
  va_copy(args_copy, arguments);
  vsnprintf(buffer, len, message, args_copy);
  va_end(args_copy);

  /* call the callback */
  arglist = Py_BuildValue("(is)", type, buffer);
  if(!arglist) {
    fprintf(stderr, "librdf_call_python_message: Out of memory\n");
    free(buffer);
    return 0;
  }
  result = PyEval_CallObject(librdf_python_callback, arglist);
  Py_DECREF(arglist);
  if(result) {
    if(PyInt_Check(result))
      rc=(int)PyInt_AS_LONG(result);
    
    Py_DECREF(result);
  }

  free(buffer);

  rc=1;
  
  return rc;
}

static int
librdf_python_error_handler(void *user_data, 
                            const char *message, va_list arguments)
{
  return librdf_call_python_message(0, message, arguments);
}


static int
librdf_python_warning_handler(void *user_data,
                              const char *message, va_list arguments)
{
  return librdf_call_python_message(1, message, arguments);
}

void
librdf_python_world_init(librdf_world *world)
{
  (void) Py_InitModule("Redland_python", librdf_python_methods);
  librdf_world_set_error(world, NULL, librdf_python_error_handler);
  librdf_world_set_warning(world,  NULL, librdf_python_warning_handler);
}
 

#endif

#ifdef SWIGPERL
/*
 * calls a perl subroutine defined as:
 *   RDF::Redland::World::message($$)
 * where first argument is an integer, second is a (scalar) string
 */
static int
librdf_call_perl_message(int type, const char *message, va_list arguments)
{
  char empty_buffer[1];
  dSP;
  char *buffer;
  int len;
  va_list args_copy;
  int rc=0;
  
  SAVETMPS;

  /* ask vsnprintf size of buffer required */
  va_copy(args_copy, arguments);
  len=vsnprintf(empty_buffer, 1, message, args_copy)+1;
  va_end(args_copy);
  buffer=(char*)malloc(len);
  if(!buffer)
    fprintf(stderr, "librdf_call_perl_message: Out of memory\n");
  else {
    int count;
    
    va_copy(args_copy, arguments);
    vsnprintf(buffer, len, message, args_copy);
    va_end(args_copy);

    PUSHMARK(SP) ;
    XPUSHs(sv_2mortal(newSViv(type)));
    XPUSHs(sv_2mortal(newSVpv(buffer, 0)));
    PUTBACK;
  
    count=call_pv("RDF::Redland::World::message", G_SCALAR);

    SPAGAIN;
    if(count == 1)
      rc=POPi;
    PUTBACK;
    
    free(buffer);
  }
  
  FREETMPS;
  
  return rc;
}

static int
librdf_perl_error_handler(void *user_data, 
                          const char *message, va_list arguments)
{
  return librdf_call_perl_message(0, message, arguments);
}


static int
librdf_perl_warning_handler(void *user_data,
                            const char *message, va_list arguments)
{
  return librdf_call_perl_message(1, message, arguments);
}

static librdf_world* librdf_perl_world=NULL;

void
librdf_perl_world_init(librdf_world *world)
{
  librdf_world_set_error(world, NULL, librdf_perl_error_handler);
  librdf_world_set_warning(world,  NULL, librdf_perl_warning_handler);

  librdf_perl_world=world;
}

void
librdf_perl_world_finish(void)
{
  librdf_free_world(librdf_perl_world);
}
#endif


/* When in PHP when being compiled by C */
#if defined (PHP_VERSION)
static librdf_world* librdf_php_world;

static librdf_world*
librdf_php_get_world(void)
{
  return librdf_php_world;
}

void
librdf_php_world_finish(void)
{
  librdf_free_world(librdf_php_world);
}
#endif    


/* prototypes for internal routines called below - NOT PART OF API */
void librdf_test_error(librdf_world* world, const char *message);
void librdf_test_warning(librdf_world* world, const char *message);

/* FOR TESTING ERRORS ONLY - NOT PART OF API */
void
librdf_internal_test_error(librdf_world *world) 
{
  librdf_test_error(world, "test error message number 1.");
}

void
librdf_internal_test_warning(librdf_world *world) 
{
  librdf_test_warning(world, "test warning message number 2.");
}

%}



%init %{
#ifdef TCL_MAJOR_VERSION
Tcl_PkgProvide(interp, PACKAGE, (char*)librdf_version_string);
#endif

#ifdef PHP_VERSION
  /* PHP seems happier if this happens at module init time */
  if(!librdf_php_world) {
    librdf_php_world=librdf_new_world();
    librdf_world_open(librdf_php_world);
  }
#endif
%}

/* optional input strings - can be NULL, need special conversions */
%typemap(ruby,in) const char *inStrOrNull {
  $1 = ($input == Qnil) ? NULL : STR2CSTR($input);
}
/* returning char* or NULL, need special conversions */
%typemap(ruby,out) char *{
 $result = ($1 == NULL) ? Qnil : rb_str_new2($1);
}


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
typedef struct librdf_serializer_s librdf_serializer;

/* rdf_init.h */
%newobject librdf_new_world;

librdf_world* librdf_new_world(void);
void librdf_free_world(librdf_world *world);
void librdf_world_open(librdf_world *world);

/* rdf_iterator.h */
void librdf_free_iterator(librdf_iterator*);
int librdf_iterator_end(librdf_iterator* iterator);
librdf_node* librdf_iterator_get_object(librdf_iterator* iterator);
librdf_node* librdf_iterator_get_context(librdf_iterator* iterator);
int librdf_iterator_next(librdf_iterator* iterator);

/* rdf_uri.h */
%newobject librdf_new_uri;
%newobject librdf_new_uri_from_uri;
%newobject librdf_new_uri_from_filenam;

librdf_uri* librdf_new_uri (librdf_world *world, char *string);
librdf_uri* librdf_new_uri_from_uri (librdf_uri* uri);
librdf_uri* librdf_new_uri_from_filename(librdf_world* world, const char *filename);
void librdf_free_uri(librdf_uri *uri);
%newobject librdf_uri_to_string;
char* librdf_uri_to_string (librdf_uri* uri);
int librdf_uri_equals(librdf_uri* first_uri, librdf_uri* second_uri);

/* rdf_node.h */
%newobject librdf_new_node;
%newobject librdf_new_node_from_uri_string;
%newobject librdf_new_node_from_uri;
%newobject librdf_new_node_from_literal;
%newobject librdf_new_node_from_typed_literal;
%newobject librdf_new_node_from_node;
%newobject librdf_new_node_from_blank_identifier;

librdf_node* librdf_new_node(librdf_world *world);
librdf_node* librdf_new_node_from_uri_string(librdf_world *world, const char *string);
librdf_node* librdf_new_node_from_uri(librdf_world *world, librdf_uri *uri);
librdf_node* librdf_new_node_from_literal(librdf_world *world, const char *string, const char *inStrOrNull=NULL, int is_wf_xml=0);
librdf_node* librdf_new_node_from_typed_literal(librdf_world *world, const char *string, const char *inStrOrNull=NULL, librdf_uri* datatype_uri=NULL);
librdf_node* librdf_new_node_from_node(librdf_node *node);
librdf_node* librdf_new_node_from_blank_identifier(librdf_world *world, const char *inStrOrNull=NULL);
void librdf_free_node(librdf_node *r);
librdf_uri* librdf_node_get_uri(librdf_node* node);
int librdf_node_get_type(librdf_node* node);
char* librdf_node_get_literal_value(librdf_node* node);
char* librdf_node_get_literal_value_as_latin1(librdf_node* node);
char* librdf_node_get_literal_value_language(librdf_node* node);
librdf_uri* librdf_node_get_literal_value_datatype_uri(librdf_node* node);
int librdf_node_get_literal_value_is_wf_xml(librdf_node* node);

%newobject librdf_node_to_string;
char *librdf_node_to_string(librdf_node* node);
char *librdf_node_get_blank_identifier(librdf_node* node);

int librdf_node_is_resource(librdf_node* node);
int librdf_node_is_literal(librdf_node* node);
int librdf_node_is_blank(librdf_node* node);

int librdf_node_equals(librdf_node* first_node, librdf_node* second_node);


/* rdf_statement.h */
%newobject librdf_new_statement;
%newobject librdf_new_statement_from_statement;
%newobject librdf_new_statement_from_nodes;

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
int librdf_statement_equals(librdf_statement* statement1, librdf_statement* statement2);
int librdf_statement_match(librdf_statement* statement, librdf_statement* partial_statement);

%newobject librdf_statement_to_string;
char *librdf_statement_to_string(librdf_statement *statement);

/* rdf_model.h */
%newobject librdf_new_model;
%newobject librdf_new_model_with_options;
%newobject librdf_new_model_from_model;

librdf_model* librdf_new_model(librdf_world *world, librdf_storage *storage, char* options_string);
librdf_model* librdf_new_model_with_options(librdf_world *world, librdf_storage *storage, librdf_hash* options);
librdf_model* librdf_new_model_from_model(librdf_model* model);
void librdf_free_model(librdf_model *model);
int librdf_model_size(librdf_model* model);
int librdf_model_add(librdf_model* model, librdf_node* subject, librdf_node* predicate, librdf_node* object);
int librdf_model_add_typed_literal_statement(librdf_model* model, librdf_node* subject, librdf_node* predicate, char* string, char *xml_language, librdf_uri *datatype_uri);
int librdf_model_add_statement(librdf_model* model, librdf_statement* statement);
int librdf_model_add_statements(librdf_model* model, librdf_stream* statement_stream);
int librdf_model_remove_statement(librdf_model* model, librdf_statement* statement);
int librdf_model_contains_statement(librdf_model* model, librdf_statement* statement);
%newobject librdf_model_as_stream;
librdf_stream* librdf_model_as_stream(librdf_model* model);
%newobject librdf_model_find_statements;
librdf_stream* librdf_model_find_statements(librdf_model* model, librdf_statement* statement);
%newobject librdf_model_find_statements_in_context;
librdf_stream* librdf_model_find_statements_in_context(librdf_model* model, librdf_statement* statement, librdf_node* context_node);
%newobject librdf_model_get_sources;
librdf_iterator* librdf_model_get_sources(librdf_model *model, librdf_node *arc, librdf_node *target);
%newobject librdf_model_get_arcs;
librdf_iterator* librdf_model_get_arcs(librdf_model *model, librdf_node *source, librdf_node *target);
%newobject librdf_model_get_targets;
librdf_iterator* librdf_model_get_targets(librdf_model *model, librdf_node *source, librdf_node *arc);
%newobject librdf_model_get_source;
librdf_node* librdf_model_get_source(librdf_model *model, librdf_node *arc, librdf_node *target);
%newobject librdf_model_get_arc;
librdf_node* librdf_model_get_arc(librdf_model *model, librdf_node *source, librdf_node *target);
%newobject librdf_model_get_arcs_out;
librdf_iterator* librdf_model_get_arcs_out(librdf_model *model,librdf_node *node);
%newobject librdf_model_get_arcs_in;
librdf_iterator* librdf_model_get_arcs_in(librdf_model *model,librdf_node *node);
librdf_iterator* librdf_model_has_arc_in(librdf_model *model,librdf_node *node,librdf_node *property);
librdf_iterator* librdf_model_has_arc_out(librdf_model *model,librdf_node *node,librdf_node *property);
%newobject librdf_model_get_target;
librdf_node* librdf_model_get_target(librdf_model *model, librdf_node *source, librdf_node *arc);
int librdf_model_context_add_statement(librdf_model* model, librdf_node* context, librdf_statement* statement);
int librdf_model_context_add_statements(librdf_model* model, librdf_node* context, librdf_stream* stream);
int librdf_model_context_remove_statement(librdf_model* model, librdf_node* context, librdf_statement* statement);
int librdf_model_context_remove_statements(librdf_model* model, librdf_node* context);
%newobject librdf_model_context_as_stream;
librdf_stream* librdf_model_context_as_stream(librdf_model* model, librdf_node* context);
void librdf_model_sync(librdf_model* model);
%newobject librdf_model_get_contexts;
librdf_iterator* librdf_model_get_contexts(librdf_model* model);
librdf_node* librdf_model_get_feature(librdf_model* model, librdf_uri* feature);
int librdf_model_set_feature(librdf_model* model, librdf_uri* feature, librdf_node* value);
int librdf_model_load(librdf_model* model, librdf_uri *uri, const char *inStrOrNull=NULL, const char *inStrOrNull=NULL, librdf_uri *type_uri=NULL);
%newobject librdf_model_query_execute;
librdf_query_results* librdf_model_query_execute(librdf_model* model, librdf_query* query);

/* rdf_storage.h */
%newobject librdf_new_storage;
%newobject librdf_new_storage_from_storage;

librdf_storage* librdf_new_storage(librdf_world *world, char *storage_name, char *name, char *options_string);
librdf_storage* librdf_new_storage_from_storage (librdf_storage* old_storage);
void librdf_free_storage(librdf_storage *storage);

/* rdf_parser.h */
%newobject librdf_new_parser;

librdf_parser* librdf_new_parser(librdf_world *world, const char *name, const char *mime_type, librdf_uri *type_uri);
void librdf_free_parser(librdf_parser *parser);

%newobject librdf_parser_parse_as_stream;
librdf_stream* librdf_parser_parse_as_stream(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri);
int librdf_parser_parse_into_model(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri, librdf_model* model);
%newobject librdf_parser_parse_string_as_stream;
librdf_stream* librdf_parser_parse_string_as_stream(librdf_parser* parser, const char *string, librdf_uri* base_uri);
int librdf_parser_parse_string_into_model(librdf_parser* parser, const char *string, librdf_uri* base_uri, librdf_model* model);
librdf_node* librdf_parser_get_feature(librdf_parser* parser, librdf_uri *feature);
int librdf_parser_set_feature(librdf_parser* parser, librdf_uri *feature, librdf_node* value);


/* rdf_query.h */
%newobject librdf_new_query;
%newobject librdf_new_query_from_query;

librdf_query* librdf_new_query(librdf_world* world, const char *name, librdf_uri* uri, const char *query_string);
librdf_query* librdf_new_query_from_query (librdf_query* old_query);
void librdf_free_query(librdf_query *query);

/* methods */
%newobject librdf_query_execute;
librdf_query_results* librdf_query_execute(librdf_query* query, librdf_model *model);

%newobject librdf_query_results_as_stream;
librdf_stream* librdf_query_results_as_stream(librdf_query_results* query_results);
int librdf_query_results_get_count(librdf_query_results* query_results);
int librdf_query_results_next(librdf_query_results* query_results);
int librdf_query_results_finished(librdf_query_results* query_results);
%newobject librdf_query_get_result_binding_value;
librdf_node* librdf_query_results_get_binding_value(librdf_query_results* query_results, int offset);
const char* librdf_query_results_get_binding_name(librdf_query_results* query_results, int offset);
%newobject librdf_query_results_get_binding_value_by_name;
librdf_node* librdf_query_results_get_binding_value_by_name(librdf_query_results* query_results, const char *name);
int librdf_query_results_get_bindings_count(librdf_query_results* query_results);
void librdf_free_query_results(librdf_query_results* query_results);


/* rdf_serializer.h */
%newobject librdf_new_serializer;

librdf_serializer* librdf_new_serializer(librdf_world* world, const char *name, const char *mime_type, librdf_uri *type_uri);
void librdf_free_serializer(librdf_serializer *serializer);
int librdf_serializer_serialize_model_to_file(librdf_serializer* serializer, const char *name, librdf_uri* base_uri, librdf_model* model);
librdf_node* librdf_serializer_get_feature(librdf_serializer* serializer, librdf_uri *feature);
int librdf_serializer_set_feature(librdf_serializer* serializer, librdf_uri *feature, librdf_node* value);
int librdf_serializer_set_namespace(librdf_serializer* serializer, librdf_uri *nspace, const char * prefix);

/* rdf_stream.h */
void librdf_free_stream(librdf_stream* stream);
int librdf_stream_end(librdf_stream* stream);
int librdf_stream_next(librdf_stream* stream);
librdf_statement* librdf_stream_get_object(librdf_stream* stream);
librdf_node* librdf_stream_get_context(librdf_stream* stream);

/* here */
#ifdef SWIGPYTHON
void librdf_python_world_init(librdf_world *world);
#endif
#ifdef SWIGPERL
void librdf_perl_world_init(librdf_world *world);
void librdf_perl_world_finish(void);
#endif
#ifdef SWIGPHP
librdf_world* librdf_php_get_world(void);
void librdf_php_world_finish(void);
#endif

/* FOR TESTING ERRORS ONLY - NOT PART OF API */
void librdf_internal_test_error(librdf_world *world);
void librdf_internal_test_warning(librdf_world *world);


/* SWIG world - declare variables wanted from rdf_init.h */

%immutable;
extern const char * const librdf_short_copyright_string;
extern const char * const librdf_copyright_string;
extern const char * const librdf_version_string;
extern const unsigned int librdf_version_major;
extern const unsigned int librdf_version_minor;
extern const unsigned int librdf_version_release;
extern const unsigned int librdf_version_decimal;
%mutable;

