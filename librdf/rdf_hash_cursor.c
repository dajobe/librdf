/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_hash.c - RDF Hash Cursor Implementation
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
#include <ctype.h>
#include <sys/types.h>

#include <redland.h>

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
