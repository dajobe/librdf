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

/* internal routines used to invoking errors/warnings upwards to user */
void librdf_error(librdf_world* world, const char *message, ...);
void librdf_warning(librdf_world* world, const char *message, ...);

#ifdef SWIGPYTHON
/* swig doesn't declare all prototypes */
static PyObject *_wrap_redland_copyright_string_get(void);
static PyObject *_wrap_redland_version_string_get(void);

static PyObject *_wrap_redland_version_major_get(void);
static PyObject *_wrap_redland_version_minor_get(void);
static PyObject *_wrap_redland_version_release_get(void);


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
 */
static void
librdf_call_python_message(int type, const char *message, va_list arguments)
{
  PyObject *arglist;
  PyObject *result;
  char *buffer;
  int len;

  if(!librdf_python_callback) {
    fprintf(stderr, "librdf_call_python_message: No message callback registered\n");
    return;
  }

  /* ask vsnprintf size of buffer required */
  len=vsnprintf(NULL, 0, message, arguments)+1;
  buffer=(char*)malloc(len);
  if(!buffer)
    fprintf(stderr, "librdf_call_python_message: Out of memory\n");
  else {
    vsnprintf(buffer, len, message, arguments);

    if(type == 0) {
#ifdef PYTHON_EXCEPTIONS_WORKING
      PyObject *error = PyErr_NewException("Redland.error", NULL, NULL);
      /* error */
      PyErr_SetString(error, buffer);
#else
      PyErr_Warn(NULL, buffer);
#endif
    } else {
      /* warning */
       PyErr_Warn(NULL, buffer);
    }

#ifdef PYTHON_EXCEPTIONS_WORKING
    /* call the callback */
    arglist = Py_BuildValue("(is)", type, buffer);
    if(!arglist) {
      fprintf(stderr, "librdf_call_python_message: Out of memory\n");
      free(buffer);
      return;
    }
    result = PyEval_CallObject(librdf_python_callback, arglist);
    Py_DECREF(arglist);
    if (result == NULL) {
      free(buffer);
      return;
    }
    
    /* no result */
    Py_DECREF(result);
#endif
    free(buffer);
  }
}

static void
librdf_python_error_handler(void *user_data, 
                            const char *message, va_list arguments)
{
  librdf_call_python_message(0, message, arguments);
}


static void
librdf_python_warning_handler(void *user_data,
                              const char *message, va_list arguments)
{
  librdf_call_python_message(1, message, arguments);
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
static void
librdf_call_perl_message(int type, const char *message, va_list arguments)
{
  dSP;
  char *buffer;
  int len;
  
  ENTER;
  SAVETMPS;

  /* ask vsnprintf size of buffer required */
  len=vsnprintf(NULL, 0, message, arguments)+1;
  buffer=(char*)malloc(len);
  if(!buffer)
    fprintf(stderr, "librdf_call_perl_message: Out of memory\n");
  else {
    vsnprintf(buffer, len, message, arguments);

    PUSHMARK(SP) ;
    XPUSHs(sv_2mortal(newSViv(type)));
    XPUSHs(sv_2mortal(newSVpv(buffer, 0)));
    PUTBACK;
  
    call_pv("RDF::Redland::World::message", G_DISCARD);

    free(buffer);
  }
  
  FREETMPS;
  LEAVE;
}

static void
librdf_perl_error_handler(void *user_data, 
                          const char *message, va_list arguments)
{
  librdf_call_perl_message(0, message, arguments);
}


static void
librdf_perl_warning_handler(void *user_data,
                            const char *message, va_list arguments)
{
  librdf_call_perl_message(1, message, arguments);
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


/* FOR TESTING ERRORS ONLY - NOT PART OF API */
void
librdf_internal_test_error(librdf_world *world) 
{
  librdf_error(world, "test error message number %d.", 1);
}

void
librdf_internal_test_warning(librdf_world *world) 
{
  librdf_warning(world, "test warning message number %d.", 2);
}

%}



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
librdf_node* librdf_iterator_get_object(librdf_iterator* iterator);
librdf_node* librdf_iterator_get_context(librdf_iterator* iterator);
int librdf_iterator_next(librdf_iterator* iterator);

/* rdf_uri.h */
librdf_uri* librdf_new_uri (librdf_world *world, char *string);
librdf_uri* librdf_new_uri_from_uri (librdf_uri* uri);
librdf_uri* librdf_new_uri_from_filename(librdf_world* world, const char *filename);
void librdf_free_uri(librdf_uri *uri);
%newobject librdf_uri_to_string;
char* librdf_uri_to_string (librdf_uri* uri);
int librdf_uri_equals(librdf_uri* first_uri, librdf_uri* second_uri);

/* rdf_node.h */
librdf_node* librdf_new_node(librdf_world *world);
librdf_node* librdf_new_node_from_uri_string(librdf_world *world, char *string);
librdf_node* librdf_new_node_from_uri(librdf_world *world, librdf_uri *uri);
librdf_node* librdf_new_node_from_literal(librdf_world *world, char *string, char *xml_language, int is_wf_xml);
librdf_node* librdf_new_node_from_typed_literal(librdf_world *world, const char *string, const char *xml_language, librdf_uri* datatype_uri);
librdf_node* librdf_new_node_from_node(librdf_node *node);
librdf_node* librdf_new_node_from_blank_identifier(librdf_world *world, const char *identifier);
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

%newobject librdf_statement_to_string;
char *librdf_statement_to_string(librdf_statement *statement);

/* rdf_model.h */
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
librdf_stream* librdf_model_serialise(librdf_model* model);
librdf_stream* librdf_model_find_statements(librdf_model* model, librdf_statement* statement);
librdf_iterator* librdf_model_get_sources(librdf_model *model, librdf_node *arc, librdf_node *target);
librdf_iterator* librdf_model_get_arcs(librdf_model *model, librdf_node *source, librdf_node *target);
librdf_iterator* librdf_model_get_targets(librdf_model *model, librdf_node *source, librdf_node *arc);
librdf_node* librdf_model_get_source(librdf_model *model, librdf_node *arc, librdf_node *target);
librdf_node* librdf_model_get_arc(librdf_model *model, librdf_node *source, librdf_node *target);
librdf_node* librdf_model_get_target(librdf_model *model, librdf_node *source, librdf_node *arc);
int librdf_model_context_add_statement(librdf_model* model, librdf_node* context, librdf_statement* statement);
int librdf_model_context_add_statements(librdf_model* model, librdf_node* context, librdf_stream* stream);
int librdf_model_context_remove_statement(librdf_model* model, librdf_node* context, librdf_statement* statement);
int librdf_model_context_remove_statements(librdf_model* model, librdf_node* context);
librdf_stream* librdf_model_context_serialize(librdf_model* model, librdf_node* context);
void librdf_model_sync(librdf_model* model);

/* rdf_storage.h */
librdf_storage* librdf_new_storage(librdf_world *world, char *storage_name, char *name, char *options_string);
librdf_storage* librdf_new_storage_from_storage (librdf_storage* old_storage);
void librdf_free_storage(librdf_storage *storage);

/* rdf_parser.h */
librdf_parser* librdf_new_parser(librdf_world *world, const char *name, const char *mime_type, librdf_uri *type_uri);
void librdf_free_parser(librdf_parser *parser);
librdf_stream* librdf_parser_parse_as_stream(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri);
int librdf_parser_parse_into_model(librdf_parser* parser, librdf_uri* uri, librdf_uri* base_uri, librdf_model* model);
librdf_stream* librdf_parser_parse_string_as_stream(librdf_parser* parser, const char *string, librdf_uri* base_uri);
int librdf_parser_parse_string_into_model(librdf_parser* parser, const char *string, librdf_uri* base_uri, librdf_model* model);
const char *librdf_parser_get_feature(librdf_parser* parser, librdf_uri *feature);
int librdf_parser_set_feature(librdf_parser* parser, librdf_uri *feature, const char *value);

/* rdf_serializer.h */
librdf_serializer* librdf_new_serializer(librdf_world* world, const char *name, const char *mime_type, librdf_uri *type_uri);
void librdf_free_serializer(librdf_serializer *serializer);
int librdf_serializer_serialize_model_to_file(librdf_serializer* serializer, const char *name, librdf_uri* base_uri, librdf_model* model);
const char *librdf_serializer_get_feature(librdf_serializer* serializer, librdf_uri *feature);
int librdf_serializer_set_feature(librdf_serializer* serializer, librdf_uri *feature, const char *value);

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

/* FOR TESTING ERRORS ONLY - NOT PART OF API */
void librdf_internal_test_error(librdf_world *world);
void librdf_internal_test_warning(librdf_world *world);


/* SWIG world - declare variables wanted from rdf_init.h */

%immutable;
/* Note: most consts lost for SWIG to remain happy */
extern const char * redland_copyright_string;
extern const char * redland_version_string;
extern int redland_version_major;
extern int redland_version_minor;
extern int redland_version_release;
%mutable;

