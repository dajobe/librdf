/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash.c - RDF Hash Cursor Implementation
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
#include <ctype.h>
#include <sys/types.h>

#include <librdf.h>

#include <rdf_hash.h>


/* private structure */
struct librdf_hash_cursor_s {
  librdf_hash *hash;
  void *context;
};



/**
 * librdf_new_hash_cursor - Constructor - Create a new hash cursor over a hash
 * @hash: the hash object
 *
 * Return value: a new &librdf_hash_cursor or NULL on failure
 **/
librdf_hash_cursor*
librdf_new_hash_cursor (librdf_hash* hash) 
{
  librdf_hash_cursor* cursor;
  void *cursor_context;

  cursor=(librdf_hash_cursor*)LIBRDF_CALLOC(librdf_hash_cursor, 1, 
                                            sizeof(librdf_hash_cursor));
  if(!cursor)
    return NULL;

  cursor_context=(char*)LIBRDF_CALLOC(librdf_hash_cursor_context, 1,
                                      hash->factory->cursor_context_length);
  if(!cursor_context) {
    LIBRDF_FREE(librdf_hash_cursor, cursor);
    return NULL;
  }

  cursor->hash=hash;
  cursor->context=cursor_context;

  if(hash->factory->cursor_init(cursor->context, hash->context)) {
    librdf_free_hash_cursor(cursor);
    cursor=NULL;
  }

  return cursor;
}


/**
 * librdf_free_hash_cursor - Destructor - destroy a librdf_hash_cursor object
 *
 * @hash: hash cursor object
 **/
void
librdf_free_hash_cursor (librdf_hash_cursor* cursor) 
{
  if(cursor->context) {
    cursor->hash->factory->cursor_finish(cursor->context);
    LIBRDF_FREE(librdf_hash_cursor_context, cursor->context);
  }

  LIBRDF_FREE(librdf_hash_cursor, cursor);
}


int
librdf_hash_cursor_set(librdf_hash_cursor *cursor,
                       librdf_hash_datum *key,
                       librdf_hash_datum *value)
{
  return cursor->hash->factory->cursor_get(cursor->context, key, value, 
                                           LIBRDF_HASH_CURSOR_SET);
}


int
librdf_hash_cursor_get_next_value(librdf_hash_cursor *cursor, 
                                  librdf_hash_datum *key,
                                  librdf_hash_datum *value)
{
  return cursor->hash->factory->cursor_get(cursor->context, key, value, 
                                           LIBRDF_HASH_CURSOR_NEXT_VALUE);
}


int
librdf_hash_cursor_get_first(librdf_hash_cursor *cursor,
                             librdf_hash_datum *key, librdf_hash_datum *value)
{
  return cursor->hash->factory->cursor_get(cursor->context, key, value, 
                                           LIBRDF_HASH_CURSOR_FIRST);
}


int
librdf_hash_cursor_get_next(librdf_hash_cursor *cursor, librdf_hash_datum *key,
                            librdf_hash_datum *value)
{
  return cursor->hash->factory->cursor_get(cursor->context, key, value, 
                                           LIBRDF_HASH_CURSOR_NEXT);
}
