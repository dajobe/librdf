/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_statement.h - RDF Statement definition
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



#ifndef LIBRDF_STATEMENT_H
#define LIBRDF_STATEMENT_H


#ifdef __cplusplus
extern "C" {
#endif


struct librdf_statement_s
{
  librdf_node* subject;
  librdf_node* predicate;
  librdf_node* object;
  librdf_context *context;
};



/* class methods */
void librdf_init_statement(void);
void librdf_finish_statement(void);


/* initialising functions / constructors */

/* Create a new Statement. */
librdf_statement* librdf_new_statement(void);

/* Create a new Statement from an existing Statement - CLONE */
librdf_statement* librdf_new_statement_from_statement(librdf_statement* statement);

/* destructor */
void librdf_free_statement(librdf_statement* statement);


/* functions / methods */

librdf_node* librdf_statement_get_subject(librdf_statement *statement);
void librdf_statement_set_subject(librdf_statement *statement, librdf_node *subject);

librdf_node* librdf_statement_get_predicate(librdf_statement *statement);
void librdf_statement_set_predicate(librdf_statement *statement, librdf_node *predicate);

librdf_node* librdf_statement_get_object(librdf_statement *statement);
void librdf_statement_set_object(librdf_statement *statement, librdf_node *object);

librdf_context* librdf_statement_get_context(librdf_statement *statement);
void librdf_statement_set_context(librdf_statement *statement, librdf_context *context);

/* convert to a string */
char *librdf_statement_to_string(librdf_statement *statement);
/* print it prettily */
void librdf_statement_print(librdf_statement *statement, FILE *fh);

/* compare two statements */
int librdf_statement_equals(librdf_statement* statement1, librdf_statement* statement2);
/* match statement against one with partial content */
int librdf_statement_match(librdf_statement* statement, librdf_statement* partial_statement);

/* serialising/deserialising */
size_t librdf_statement_encode(librdf_statement* statement, unsigned char *buffer, size_t length);
size_t librdf_statement_encode_parts(librdf_statement* statement, unsigned char *buffer, size_t length, int fields);
size_t librdf_statement_decode(librdf_statement* statement, unsigned char *buffer, size_t length);


#ifdef LIBRDF_INTERNAL

/* Or-ed to provide fields argument to librdf_statement_encode() */
#define LIBRDF_STATEMENT_SUBJECT 1
#define LIBRDF_STATEMENT_PREDICATE 2
#define LIBRDF_STATEMENT_OBJECT 4
#define LIBRDF_STATEMENT_CONTEXT 8

#define LIBRDF_STATEMENT_ALL 15 /* must be or of all of the above */

#endif

#ifdef __cplusplus
}
#endif

#endif
