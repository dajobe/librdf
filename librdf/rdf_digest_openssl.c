/*
 * rdf_digest_openssl.c - RDF Digest OpenSSL Digest interface
 *
 * $Source$
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


#include <config.h>

#include <openssl/crypto.h>

#define LIBRDF_INTERNAL 1
#include <rdf_config.h>
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
 * librdf_digest_openssl_constructor:
 * @void: 
 * 
 * Initialise the OpenSSL digest module
 **/
void
librdf_digest_openssl_constructor(void)
{
#ifdef HAVE_OPENSSL_CRYPTO_MD5_DIGEST
        librdf_digest_register_factory("MD5",
                                       &librdf_openssl_md5_register_factory);
#endif
#ifdef HAVE_OPENSSL_CRYPTO_RIPEMD160_DIGEST
        librdf_digest_register_factory("RIPEMD160",
                                       &librdf_openssl_ripemd160_register_factory);
#endif
#ifdef HAVE_OPENSSL_CRYPTO_SHA1_DIGEST
        librdf_digest_register_factory("SHA1",
                                       &librdf_openssl_sha1_register_factory);
#endif

}
