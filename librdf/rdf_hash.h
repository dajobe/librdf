/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash.h - RDF Hash Factory and Hash interfaces and definitions
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


#ifndef LIBRDF_HASH_H
#define LIBRDF_HASH_H

#ifdef __cplusplus
extern "C" {
#endif


/** data type used to describe hash key and data */
struct librdf_hash_datum_s
{
  void *data;
  size_t size;
  /* used internally to build lists of these  */
  struct librdf_hash_datum_s *next;
};
typedef struct librdf_hash_datum_s librdf_hash_datum;

/* constructor / destructor for above */
librdf_hash_datum* librdf_new_hash_datum(void *data, size_t size);
void librdf_free_hash_datum(librdf_hash_datum *ptr);
  


/** A hash object */
struct librdf_hash_s
{
  char* identifier; /* as passed in during open(), used by clone() */
  void* context;
  int   is_open;
  struct librdf_hash_factory_s* factory;
};


/** A Hash Factory */
struct librdf_hash_factory_s {
  struct librdf_hash_factory_s* next;
  char* name;

  /* the rest of this structure is populated by the
     hash-specific register function */
  size_t context_length;

  /* size of the cursor context */
  size_t cursor_context_length;

  /* clone an existing storage */
  int (*clone)(librdf_hash* new_hash, void* new_context, char* new_name, void* old_context);

  /* create / destroy a hash implementation */
  int (*create)(librdf_hash* hash, void* context);
  int (*destroy)(void* context);

  /* open/create hash with identifier and options  */
  int (*open)(void* context, char *identifier, int mode, int is_writable, int is_new, librdf_hash* options);
  /* end hash association */
  int (*close)(void* context);

  /* insert key/value pairs according to flags */
  int (*put)(void* context, librdf_hash_datum *key, librdf_hash_datum *data);

  /* returns true if key exists in hash, without returning value */
  int (*exists)(void* context, librdf_hash_datum *key);

  int (*delete_key)(void* context, librdf_hash_datum *key);

  /* flush any cached information to disk */
  int (*sync)(void* context);

  /* get the file descriptor for the hash, if it is file based (for locking) */
  int (*get_fd)(void* context);

  /* create a cursor and operate on it */
  int (*cursor_init)(void *cursor_context, void* hash_context);
#define LIBRDF_HASH_CURSOR_SET 0
#define LIBRDF_HASH_CURSOR_NEXT_VALUE 1
#define LIBRDF_HASH_CURSOR_FIRST 2
#define LIBRDF_HASH_CURSOR_NEXT 3
  int (*cursor_get)(void *cursor, librdf_hash_datum *key, librdf_hash_datum *value, unsigned int flags);
  void (*cursor_finish)(void *context);
};

typedef struct librdf_hash_factory_s librdf_hash_factory;



/* factory class methods */
void librdf_hash_register_factory(const char *name, void (*factory) (librdf_hash_factory*));
librdf_hash_factory* librdf_get_hash_factory(const char *name);


/* module init */
void librdf_init_hash(void);

/* module terminate */
void librdf_finish_hash(void);

/* constructors */
librdf_hash* librdf_new_hash(char *name);
librdf_hash* librdf_new_hash_from_factory(librdf_hash_factory* factory);
librdf_hash* librdf_new_hash_from_hash (librdf_hash* old_hash);

/* destructor */
void librdf_free_hash(librdf_hash *hash);


/* methods */

/* open/create hash with identifier and options  */
int librdf_hash_open(librdf_hash* hash, char *identifier, int mode, int is_writable, int is_new, librdf_hash* options);
/* end hash association */
int librdf_hash_close(librdf_hash* hash);

/* retrieve one value for a given hash key either as char or hash datum */
char* librdf_hash_get(librdf_hash* hash, char *key);
librdf_hash_datum* librdf_hash_get_one(librdf_hash* hash, librdf_hash_datum *key);


/* retrieve all values for a given hash key according to flags */
librdf_iterator* librdf_hash_get_all(librdf_hash* hash, librdf_hash_datum *key, librdf_hash_datum *value);

/* insert a key/value pair */
int librdf_hash_put(librdf_hash* hash, void *key, size_t key_len, void *value, size_t value_len);

  /* returns true if key exists in hash, without returning value */
int librdf_hash_exists(librdf_hash* hash, void *key, size_t key_len);

int librdf_hash_delete(librdf_hash* hash, void *key, size_t key_len);
librdf_iterator* librdf_hash_keys(librdf_hash* hash, librdf_hash_datum *key);
/* flush any cached information to disk */
int librdf_hash_sync(librdf_hash* hash);
/* get the file descriptor for the hash, if it is file based (for locking) */
int librdf_hash_get_fd(librdf_hash* hash);

/* extra methods */
void librdf_hash_print(librdf_hash* hash, FILE *fh);
void librdf_hash_print_keys(librdf_hash* hash, FILE *fh);
void librdf_hash_print_values(librdf_hash* hash, char *key, FILE *fh);

/* import a hash from a string representation */
int librdf_hash_from_string (librdf_hash* hash, char *string);

/* import a hash from an array of strings */
int librdf_hash_from_array_of_strings (librdf_hash* hash, char *array[]);

/* lookup a hash key and decode value as a boolean */
int librdf_hash_get_as_boolean (librdf_hash* hash, char *key);

/* lookup a hash key and decode value as a long */
long librdf_hash_get_as_long (librdf_hash* hash, char *key);


/* cursor methods from rdf_hash_cursor.c */

librdf_hash_cursor* librdf_new_hash_cursor (librdf_hash* hash);
void librdf_free_hash_cursor (librdf_hash_cursor* cursor);
int librdf_hash_cursor_set(librdf_hash_cursor *cursor, librdf_hash_datum *key,librdf_hash_datum *value);
int librdf_hash_cursor_get_next_value(librdf_hash_cursor *cursor, librdf_hash_datum *key,librdf_hash_datum *value);
int librdf_hash_cursor_get_first(librdf_hash_cursor *cursor, librdf_hash_datum *key, librdf_hash_datum *value);
int librdf_hash_cursor_get_next(librdf_hash_cursor *cursor, librdf_hash_datum *key, librdf_hash_datum *value);




#ifdef __cplusplus
}
#endif

#endif
