/*
 * RDF Digest OpenSSL Digest interface
 *
 * $Source$
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <config.h>

#include <openssl/crypto.h>

#include <rdf_config.h>
#include <rdf_digest.h>

#ifdef HAVE_OPENSSL_CRYPTO_MD5_DIGEST
#include <openssl/md5.h>

static unsigned char md5_digest[MD5_DIGEST_LENGTH];

static void
md5_final(MD5_CTX *c) 
{
  MD5_Final(md5_digest, c);
}

static unsigned char *
md5_get_digest(void *c)
{
  return md5_digest;
}


static void
rdf_openssl_md5_register_factory(rdf_digest_factory *factory) 
{
  factory->context_length = sizeof(MD5_CTX);
  factory->digest_length = MD5_DIGEST_LENGTH;

  factory->init  = (void (*)(void *))MD5_Init;
  factory->update = (void (*)(void *, unsigned char*, size_t))MD5_Update;
  factory->final = (void (*)(void *))md5_final;
  factory->get_digest  = (unsigned char *(*)(void *))md5_get_digest;
}
#endif

#ifdef HAVE_OPENSSL_CRYPTO_SHA1_DIGEST
#include <openssl/sha.h>

static unsigned char sha1_digest[SHA_DIGEST_LENGTH];

static void
sha1_final(SHA_CTX *c) 
{
  SHA1_Final(sha1_digest, c);
}

static unsigned char *
sha1_get_digest(void *c)
{
  return sha1_digest;
}


static void
rdf_openssl_sha1_register_factory(rdf_digest_factory *factory) 
{
  factory->context_length = sizeof(SHA_CTX);
  factory->digest_length = SHA_DIGEST_LENGTH;

  factory->init  = (void (*)(void *))SHA1_Init;
  factory->update = (void (*)(void *, unsigned char*, size_t))SHA1_Update;
  factory->final = (void (*)(void *))sha1_final;
  factory->get_digest  = (unsigned char *(*)(void *))sha1_get_digest;
}
#endif


#ifdef HAVE_OPENSSL_CRYPTO_RIPEMD160_DIGEST
#include <openssl/ripemd.h>

static unsigned char ripemd160_digest[RIPEMD160_DIGEST_LENGTH];

static void
ripemd160_final(RIPEMD160_CTX *c) 
{
  RIPEMD160_Final(ripemd160_digest, c);
}

static unsigned char *
ripemd160_get_digest(void *c)
{
  return ripemd160_digest;
}


static void
rdf_openssl_ripemd160_register_factory(rdf_digest_factory *factory) 
{
  factory->context_length = sizeof(RIPEMD160_CTX);
  factory->digest_length = SHA_DIGEST_LENGTH;

  factory->init  = (void (*)(void *))RIPEMD160_Init;
  factory->update = (void (*)(void *, unsigned char*, size_t))RIPEMD160_Update;
  factory->final = (void (*)(void *))ripemd160_final;
  factory->get_digest  = (unsigned char *(*)(void *))ripemd160_get_digest;
}
#endif


void
rdf_digest_openssl_constructor(void)
{
#ifdef HAVE_OPENSSL_CRYPTO_MD5_DIGEST
  rdf_digest_register_factory("MD5", &rdf_openssl_md5_register_factory);
#endif
#ifdef HAVE_OPENSSL_CRYPTO_SHA1_DIGEST
  rdf_digest_register_factory("SHA1", &rdf_openssl_sha1_register_factory);
#endif
#ifdef HAVE_OPENSSL_CRYPTO_RIPEMD160_DIGEST
  rdf_digest_register_factory("RIPEMD160", &rdf_openssl_ripemd160_register_factory);
#endif

}
