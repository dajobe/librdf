/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage.h - RDF Storage Factory and Storage interfaces and definitions
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


#ifndef LIBRDF_STORAGE_H
#define LIBRDF_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif


/** A storage object */
struct librdf_storage_s
{
  librdf_model *model;
  void *context;
  struct librdf_storage_factory_s* factory;
};


/** A Storage Factory */
struct librdf_storage_factory_s {
  struct librdf_storage_factory_s* next;
  char* name;
  
  /* the rest of this structure is populated by the
     storage-specific register function */
  size_t context_length;
  
  /* create a new storage */
  int (*init)(librdf_storage* storage, char *name, librdf_hash* options);

  /* destroy a storage */
  void (*terminate)(librdf_storage* storage);
  
  /* make storage be associated with model */
  int (*open)(librdf_storage* storage, librdf_model* model);
  
  /* close storage/model context */
  int (*close)(librdf_storage* storage);
  
  /* return the number of statements in the storage for model */
  int (*size)(librdf_storage* storage);
  
  /* add a statement to the storage from the given model */
  int (*add_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /* add a statement to the storage from the given model */
  int (*add_statements)(librdf_storage* storage, librdf_stream* statement_stream);
  
  /* remove a statement from the storage  */
  int (*remove_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /* true if statement in storage  */
  int (*contains_statement)(librdf_storage* storage, librdf_statement* statement);
  
  /* serialise the model in storage  */
  librdf_stream* (*serialise)(librdf_storage* storage);
  
  /* serialise the results of a query */
  librdf_stream* (*find_statements)(librdf_storage* storage, librdf_statement* statement);

  /* return a list of Nodes marching given arc, target */
  librdf_iterator* (*find_sources)(librdf_storage* storage, librdf_node *arc, librdf_node *target);

  /* return a list of Nodes marching given source, target */
  librdf_iterator* (*find_arcs)(librdf_storage* storage, librdf_node *source, librdf_node *target);

  /* return a list of Nodes marching given source, target */
  librdf_iterator* (*find_targets)(librdf_storage* storage, librdf_node *source, librdf_node *target);
};

typedef struct librdf_storage_factory_s librdf_storage_factory;



/* module init */
void librdf_init_storage(void);

/* module terminate */
void librdf_finish_storage(void);

/* class methods */
void librdf_storage_register_factory(const char *name, void (*factory) (librdf_storage_factory*));
librdf_storage_factory* librdf_get_storage_factory(const char *name);

/* constructor */
librdf_storage* librdf_new_storage(char *storage_name, char *name, char *options_string);
librdf_storage* librdf_new_storage_from_factory(librdf_storage_factory* factory, char *name, librdf_hash* options);

/* destructor */
void librdf_free_storage(librdf_storage *storage);


/* methods */
int librdf_storage_open(librdf_storage* storage, librdf_model *model);
int librdf_storage_close(librdf_storage* storage);
int librdf_storage_get(librdf_storage* storage, void *key, size_t key_len, void **value, size_t* value_len, unsigned int flags);

int librdf_storage_size(librdf_storage* storage);
int librdf_storage_add_statement(librdf_storage* storage, librdf_statement* statement);
int librdf_storage_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
int librdf_storage_remove_statement(librdf_storage* storage, librdf_statement* statement);
int librdf_storage_contains_statement(librdf_storage* storage, librdf_statement* statement);
librdf_stream* librdf_storage_serialise(librdf_storage* storage);
librdf_stream* librdf_storage_find_statements(librdf_storage* storage, librdf_statement* statement);
librdf_iterator* librdf_storage_get_sources(librdf_storage *storage, librdf_node *arc, librdf_node *target);
librdf_iterator* librdf_storage_get_arcs(librdf_storage *storage, librdf_node *source, librdf_node *target);
librdf_iterator* librdf_storage_get_targets(librdf_storage *storage, librdf_node *source, librdf_node *arc);


#ifdef __cplusplus
}
#endif

#endif
