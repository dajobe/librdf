/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_digest_openssl.c - RDF Digest OpenSSL Digest interface
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

#include <openssl/crypto.h>

#include <librdf.h>
#include <rdf_digest.h>

#ifdef HAVE_OPENSSL_CRYPTO_MD5_DIGEST
#include <openssl/md5.h>


/* The new struct contains the old one at start (so casting works) plus
 * a space for the digest to be stored once calculated
 */
typedef struct 
{
  MD5_CTX contex;
  unsigned char digest[MD5_DIGEST_LENGTH];
} MD5_CTX_2;


static void
md5_final(MD5_CTX_2 *c) 
{
  MD5_Final(c->digest, (MD5_CTX*)c);
}

static unsigned char *
md5_get_digest(MD5_CTX_2 *c)
{
  return c->digest;
}


static void
librdf_openssl_md5_register_factory(librdf_digest_factory *factory) 
{
  factory->context_length = sizeof(MD5_CTX_2);
  factory->digest_length = MD5_DIGEST_LENGTH;
        
  factory->init  = (void (*)(void *))MD5_Init;
  factory->update = (void (*)(void *, unsigned char*, size_t))MD5_Update;
  factory->final = (void (*)(void *))md5_final;
  factory->get_digest  = (unsigned char *(*)(void *))md5_get_digest;
}
#endif


#ifdef HAVE_OPENSSL_CRYPTO_SHA1_DIGEST
#include <openssl/sha.h>

/* The new struct contains the old one at start (so casting works) plus
 * a space for the digest to be stored once calculated
 */
typedef struct 
{
  SHA_CTX contex;
  unsigned char digest[SHA_DIGEST_LENGTH];
} SHA_CTX_2;


static void
sha1_final(SHA_CTX_2 *c) 
{
  SHA1_Final(c->digest, (SHA_CTX*)c);
}

static unsigned char *
sha1_get_digest(SHA_CTX_2 *c)
{
  return c->digest;
}


static void
librdf_openssl_sha1_register_factory(librdf_digest_factory *factory) 
{
  factory->context_length = sizeof(SHA_CTX_2);
  factory->digest_length = SHA_DIGEST_LENGTH;
  
  factory->init  = (void (*)(void *))SHA1_Init;
  factory->update = (void (*)(void *, unsigned char*, size_t))SHA1_Update;
  factory->final = (void (*)(void *))sha1_final;
  factory->get_digest  = (unsigned char *(*)(void *))sha1_get_digest;
}
#endif


#ifdef HAVE_OPENSSL_CRYPTO_RIPEMD160_DIGEST
#include <openssl/ripemd.h>

/* The new struct contains the old one at start (so casting works) plus
 * a space for the digest to be stored once calculated
 */
typedef struct 
{
  RIPEMD160_CTX contex;
  unsigned char digest[RIPEMD160_DIGEST_LENGTH];
} RIPEMD160_CTX_2;


static void
ripemd160_final(RIPEMD160_CTX_2 *c) 
{
  RIPEMD160_Final(c->digest, (RIPEMD160_CTX*)c);
}

static unsigned char *
ripemd160_get_digest(RIPEMD160_CTX_2 *c)
{
  return c->digest;
}


static void
librdf_openssl_ripemd160_register_factory(librdf_digest_factory *factory) 
{
  factory->context_length = sizeof(RIPEMD160_CTX_2);
  factory->digest_length = SHA_DIGEST_LENGTH;
  
  factory->init  = (void (*)(void *))RIPEMD160_Init;
  factory->update = (void (*)(void *, unsigned char*, size_t))RIPEMD160_Update;
  factory->final = (void (*)(void *))ripemd160_final;
  factory->get_digest  = (unsigned char *(*)(void *))ripemd160_get_digest;
}
#endif


/**
 * librdf_digest_openssl_constructor - Initialise the OpenSSL digest module
 * 
 **/
void
librdf_digest_openssl_constructor(void)
{
#ifdef HAVE_OPENSSL_CRYPTO_MD5_DIGEST
  librdf_digest_register_factory("MD5", &librdf_openssl_md5_register_factory);
#endif
#ifdef HAVE_OPENSSL_CRYPTO_RIPEMD160_DIGEST
  librdf_digest_register_factory("RIPEMD160", &librdf_openssl_ripemd160_register_factory);
#endif
#ifdef HAVE_OPENSSL_CRYPTO_SHA1_DIGEST
  librdf_digest_register_factory("SHA1", &librdf_openssl_sha1_register_factory);
#endif

}
