/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_digest.h - RDF Digest Factory / Digest interfaces and definition
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



#ifndef LIBRDF_DIGEST_H
#define LIBRDF_DIGEST_H

#ifdef __cplusplus
extern "C" {
#endif


typedef void* LIBRDF_DIGEST;

/* based on the GNUPG cipher/digest registration stuff */
struct librdf_digest_factory_s 
{
  struct librdf_digest_factory_s* next;
  char *   name;

  /* the rest of this structure is populated by the
     digest-specific register function */
  size_t  context_length;
  size_t  digest_length;

  /* functions (over context) */
  void (*init)( void *c );
  void (*update)( void *c, unsigned char *buf, size_t nbytes );
  void (*final)( void *c );
  unsigned char *(*get_digest)( void *c );
};
typedef struct librdf_digest_factory_s librdf_digest_factory;


struct librdf_digest_s {
  char *context;
  unsigned char *digest;
  librdf_digest_factory* factory;
};


/* factory static methods */
void librdf_digest_register_factory(const char *name, void (*factory) (librdf_digest_factory*));

librdf_digest_factory* librdf_get_digest_factory(const char *name);


/* module init */
void librdf_init_digest(void);
/* module finish */
void librdf_finish_digest(void);
                    
/* constructor */
librdf_digest* librdf_new_digest(char *name);
librdf_digest* librdf_new_digest_from_factory(librdf_digest_factory *factory);

/* destructor */
void librdf_free_digest(librdf_digest *digest);


/* methods */
void librdf_digest_init(librdf_digest* digest);
void librdf_digest_update(librdf_digest* digest, unsigned char *buf, size_t length);
void librdf_digest_final(librdf_digest* digest);
void* librdf_digest_get_digest(librdf_digest* digest);

char* librdf_digest_to_string(librdf_digest* digest);
void librdf_digest_print(librdf_digest* digest, FILE* fh);


/* in librdf_digest_openssl.c */
#ifdef HAVE_OPENSSL_DIGESTS
void librdf_digest_openssl_constructor(void);
#endif

/* in librdf_digest_md5.c */
#ifdef HAVE_LOCAL_MD5_DIGEST
void librdf_digest_md5_constructor(void);
#endif

/* in librdf_digest_sha1.c */
#ifdef HAVE_LOCAL_SHA1_DIGEST
void librdf_digest_sha1_constructor(void);
#endif

/* in librdf_digest_ripemd160.c */
#ifdef HAVE_LOCAL_RIPEMD160_DIGEST
void librdf_digest_rmd160_constructor(void);
#endif


#ifdef __cplusplus
}
#endif

#endif
